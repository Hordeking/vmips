/* Main driver program for VMIPS.
   Copyright 2001 Brian R. Gaeke.
   Copyright 2002 Paul Twohey.

This file is part of VMIPS.

VMIPS is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

VMIPS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with VMIPS; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "clock.h"
#include "clockdev.h"
#include "clockreg.h"
#include "cpzeroreg.h"
#include "error.h"
#include "haltreg.h"
#include "intctrl.h"
#include "range.h"
#include "spimconsole.h"
#include "spimconsreg.h"
#include "sysinclude.h"
#include "vmips.h"

#include <exception>


static void vmips_unexpected();
static void vmips_terminate();

vmips *machine;

void
vmips::refresh_options(void)
{
	/* Extract important flags and things. */
	opt_bootmsg = opt->option("bootmsg")->flag;
	opt_clockdevice = opt->option("clockdevice")->flag;
	opt_debug = opt->option("debug")->flag;
	opt_dumpcpu = opt->option("dumpcpu")->flag;
	opt_dumpcp0 = opt->option("dumpcp0")->flag;
	opt_haltdevice = opt->option("haltdevice")->flag;
	opt_haltdumpcpu = opt->option("haltdumpcpu")->flag;
	opt_haltdumpcp0 = opt->option("haltdumpcp0")->flag;
	opt_instcounts = opt->option("instcounts")->flag;
	opt_memdump = opt->option("memdump")->flag;
	opt_realtime = opt->option("realtime")->flag;
 
	opt_clockspeed = opt->option("clockspeed")->num;
	clock_nanos = 1000000000/opt_clockspeed;

	opt_clockintr = opt->option("clockintr")->num;
	opt_clockdeviceirq = opt->option("clockdeviceirq")->num;
	opt_loadaddr = opt->option("loadaddr")->num;
	opt_memsize = opt->option("memsize")->num;
	opt_timeratio = opt->option("timeratio")->num;
 
	opt_memdumpfile = opt->option("memdumpfile")->str;
	opt_image = opt->option("romfile")->str;
#if TTY
	opt_usetty = opt->option("usetty")->flag;
	opt_ttydev = opt->option("ttydev")->str;
	opt_ttydev2 = opt->option("ttydev2")->str;
#endif
}

/* Set up some machine globals, and process command line arguments,
 * configuration files, etc.
 */
vmips::vmips(int argc, char *argv[])
	: opt(new Options(argc, argv)), halted(false),
	  clock(0), clock_device(0), halt_device(0), spim_console(0),
	  num_instrs(0)
{
	refresh_options();
}

vmips::~vmips() throw()
{
#if TTY
	delete spim_console;
#endif

	delete halt_device;
	delete clock_device;
	delete clock;
}

void
vmips::setup_machine(void)
{
	/* Construct the various vmips components. */
	intc = new IntCtrl;
	cpu = new CPU;
	physmem = new Mapper;
	cpzero = new CPZero;

	/* Attach the components to one another. */
	cpzero->attach(cpu, intc);
	physmem->attach(cpu);
	cpu->attach(this, physmem, cpzero);

	/* Set up the debugger interface, if applicable. */
	if (opt_debug) {
		dbgr = new Debug;
		dbgr->attach(cpu,physmem);
	}
}

/* Connect the file or device named NAME to line number L of
 * console device C, or do nothing if NAME is "off".
 */
void vmips::setup_console_line(int l, char *name, SpimConsoleDevice *c) throw()
{
#if TTY
	/* If they said to turn off the tty line, do nothing. */
	if (strcmp(name, "off") == 0)
		return;

	/* Open the file or device in question. */
	int ttyfd = open(name, O_RDWR | O_NONBLOCK);
	if (ttyfd == -1) {
		/* If we can't open it, warn and use stdout instead. */
		error("Opening %s (terminal %d): %s", name, l, strerror(errno));
		warning("using stdout, input disabled\n");
		ttyfd = fileno(stdout);
	}

	/* Connect it to the SPIM-compatible console device. */
	c->connect_terminal(ttyfd, l);
	boot_msg("Connected fd %d to %s line %d.\n", ttyfd,
		  c->descriptor_str(), l);
#endif /* TTY */
}

void vmips::setup_terminals() throw( std::bad_alloc )
{
#if TTY
	/* FIXME: It would be helpful to restore tty modes on a SIGINT or
	   other abortive exit or when vmips has been foregrounded after
	   being in the background. The restoration mechanism should use
	   TerminalController::reinitialze_terminals() */

	if( !opt_usetty )
		return;
	
	spim_console = new SpimConsoleDevice( clock );
	physmem->add_device_mapping( spim_console, SPIM_BASE );
	boot_msg("Mapping %s to physical address 0x%08x\n",
		  spim_console->descriptor_str(), SPIM_BASE);
	
	intc->connectLine(IRQ2, spim_console);
	intc->connectLine(IRQ3, spim_console);
	intc->connectLine(IRQ4, spim_console);
	intc->connectLine(IRQ5, spim_console);
	intc->connectLine(IRQ6, spim_console);
	boot_msg("Connected IRQ2-IRQ6 to %s\n",spim_console->descriptor_str());

	setup_console_line(0, opt_ttydev, spim_console);
	setup_console_line(1, opt_ttydev2, spim_console);
#endif /* TTY */
}

bool vmips::setup_clockdevice() throw( std::bad_alloc )
{
	if( !opt_clockdevice )
		return true;

	uint32 clock_irq;
	if( !(clock_irq = DeviceInt::num2irq( opt_clockdeviceirq )) ) {
		error( "invalid clockdeviceirq (%u), irq numbers must be 2-7.",
		       opt_clockdeviceirq );
		return false;
	}	

	/* Microsecond Clock at base physaddr CLOCK_BASE */
	clock_device = new ClockDevice( clock, clock_irq, opt_clockintr );
	physmem->add_device_mapping( clock_device, CLOCK_BASE );
	boot_msg( "Mapping %s to physical address 0x%08x\n",
		  clock_device->descriptor_str(), CLOCK_BASE );

	intc->connectLine( clock_irq, clock_device );
	boot_msg( "Connected %s to the %s\n", DeviceInt::strlineno(clock_irq),
		  clock_device->descriptor_str() );

	return true;
}

void vmips::setup_haltdevice() throw( std::bad_alloc )
{
	if( !opt_haltdevice )
		return;

	halt_device = new HaltDevice( this );
	physmem->add_device_mapping( halt_device, HALT_BASE );
	boot_msg( "Mapping %s to physical address 0x%08x\n",
		  halt_device->descriptor_str(), HALT_BASE );
}

void vmips::boot_msg( const char *msg, ... ) throw()
{
	if( !opt_bootmsg )
		return;

	va_list ap;
	va_start( ap, msg );
	vfprintf( stderr, msg, ap );
	va_end( ap );

	fflush( stderr );
}

int
vmips::host_endian_selftest(void)
{
	uint32 x;

	((uint8 *) &x)[0] = 0;
	((uint8 *) &x)[1] = 1;
	((uint8 *) &x)[2] = 2;
	((uint8 *) &x)[3] = 3;
	if (x == 0x03020100) {
		machine->host_bigendian = false;
		boot_msg( "Little-Endian host processor detected.\n" );
		return 0;
	} else if (x == 0x00010203) {
		machine->host_bigendian = true;
		boot_msg( "Big-Endian host processor detected.\n" );
		return 0;
	} else {
		boot_msg( "Unknown processor type.\n" );
		return x;
	}
}

int
vmips::run_self_tests(void)
{
	int fail;

	if ((fail = host_endian_selftest()) != 0) {
		return fail;
	} else {
		return 0;
	}
}

void
vmips::halt(void) throw()
{
	halted = true;
}

uint32
vmips::auto_size_rom(FILE *rom) throw()
{
	off_t here, there;
	size_t len;

	assert(rom);
	here = ftell(rom);
	fseek(rom,0,SEEK_END);
	there = ftell(rom);
	fseek(rom,0,SEEK_SET);
	len = there - here;
	len /= 4;
	return len;
}

void
vmips::step(void)
{
	/* Process instructions. */
	cpu->step();

	/* FIXME: make the cpu mark its time usage, this is just a hack. */
	/* each instruction takes 2us = 2 micro seconds */
	if( !opt_realtime )
	   clock->increment_time(clock_nanos);
	else
	   clock->pass_realtime(opt_timeratio);

	/* If user requested it, dump registers from CPU and/or CP0. */
	if (opt_dumpcpu)
		cpu->dump_regs_and_stack(stderr);
	if (opt_dumpcp0)
		cpzero->dump_regs_and_tlb(stderr);
	num_instrs++;
}

long 
timediff(struct timeval *after, struct timeval *before)
{
    return (after->tv_sec * 1000000 + after->tv_usec) -
        (before->tv_sec * 1000000 + before->tv_usec);
}

int
vmips::run()
{
	extern void setup_disassembler(FILE *stream);

	/* Set up the rest of the machine components. */
	setup_machine();

	/* Open ROM image. */
	FILE *rom = fopen(opt_image,"r");
	if (!rom) {
		error("Could not open ROM `%s': %s",
		       opt_image, strerror(errno));
		return 2;
	}

	/* Print lots of cutesy information. */
	uint32 rom_size = auto_size_rom(rom);
	boot_msg( "Auto-size ROM image: %d words.\n", rom_size );

	/* Run self tests. These may or may not become real tests... */
	boot_msg( "Running self tests.\n" );
	if (run_self_tests() != 0) {
		error( "Failed self tests." );
		return 1;
	}
	boot_msg( "Self tests passed.\n" );

	/* Map the ROM image to the virtual physical memory. */
	if (opt_debug) {
		/* Point debugger at wherever the user thinks the ROM is. */
		dbgr->setup(opt_loadaddr, rom_size);
	}

	/* Translate loadaddr to physical address. */
	opt_loadaddr -= KSEG1_CONST_TRANSLATION;
	physmem->add_file_mapping(rom, opt_loadaddr, MEM_READ);
	boot_msg( "Mapping ROM image (%s): %u words at 0x%08x [%08x]\n",
		  opt_image,
		  physmem->find_mapping_range(opt_loadaddr)->getExtent() / 4,
		  physmem->find_mapping_range(opt_loadaddr)->getBase() +
		  KSEG1_CONST_TRANSLATION,
		  physmem->find_mapping_range(opt_loadaddr)->getBase() );

	/* Install RAM at base physical address 0 */
	memmod = new MemoryModule(opt_memsize);
	physmem->add_core_mapping(memmod->addr,0,memmod->len);
	boot_msg( "Mapped (host=%p) %uk RAM at physical address 0\n",
		  memmod->addr, memmod->len / 1024 );

	/* Direct the libopcodes disassembler output to stderr. */
	setup_disassembler(stderr);

	/* setup the halt device */
	setup_haltdevice();

	/* setup the clock with the current time */
	timeval start;
	gettimeofday(&start, NULL);
	timespec start_ts;
	TIMEVAL_TO_TIMESPEC( &start, &start_ts );
	clock = new Clock( start_ts );

	if( !setup_clockdevice() )
		return 1;

	setup_terminals();

	/* Reset the CPU. */
	boot_msg( "\n*************RESET*************\n\n" );
	cpu->reset();

	if (opt_instcounts)
		gettimeofday(&start, NULL);

	if (opt_debug) {
		dbgr->serverloop();
	} else {
		while (! halted) {
			step();
		}
	}

	struct timeval end;
	if (opt_instcounts)
		gettimeofday(&end, NULL);

    /* Halt! */
	boot_msg( "\n*************HALT*************\n\n" );

	/* If user requested it, dump registers from CPU and/or CP0. */
	if (opt_haltdumpcpu || opt_haltdumpcp0) {
		fprintf(stderr,"Dumping:\n");
		if (opt_haltdumpcpu)
			cpu->dump_regs_and_stack(stderr);
		if (opt_haltdumpcp0)
			cpzero->dump_regs_and_tlb(stderr);
	}

	if (opt_instcounts) {
		double elapsed = (double) timediff(&end, &start) / 1000000.0;
		fprintf(stderr, "%u instructions in %.5f seconds (%.3f "
			"instructions per second)\n", num_instrs, elapsed,
			((double) num_instrs) / elapsed);
	}

	if (opt_memdump) {
		FILE *ramdump;

		fprintf(stderr,"Dumping RAM to %s...", opt_memdumpfile);
		ramdump = fopen(opt_memdumpfile,"w");
		if (ramdump != NULL) {
			fwrite(memmod->addr,memmod->len,1,ramdump);
			fclose(ramdump);
			fprintf(stderr,"succeeded.\n");
		} else {
			error( "\nRAM dump failed: %s", strerror(errno) );
		}
	}

	/* We're done. */
	boot_msg( "Goodbye.\n" );
	return 0;
}

int
main(int argc, char **argv)
try {
	std::set_unexpected(vmips_unexpected);
	std::set_terminate(vmips_terminate);

	machine = new vmips(argc, argv);
	int rc = machine->run();
	delete machine; /* No disassemble Number Five!! */
	return rc;
}
catch( std::bad_alloc &b ) {
	fatal_error( "unable to allocate memory" );
}


static void vmips_unexpected()
{
	fatal_error( "unexpected exception" );
}

static void vmips_terminate()
{
	fatal_error( "uncaught exception" );
}
