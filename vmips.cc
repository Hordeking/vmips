#include "vmips.h"
#include "range.h"
#include "cpzeroreg.h"
#include "spimconsole.h"
#include "intctrl.h"
#include "serialhost.h"
#include "sysinclude.h"
#include "clockdev.h"

vmips *machine;

vmips::vmips()
{
	physmem = new Mapper;
	cpzero = new CPZero;
	cpu = new CPU;
	intc = new IntCtrl;
	cpu->attach(this,physmem,cpzero);
	cpzero->attach(cpu,intc);
	physmem->attach(cpu);
	dbgr = new Debug;
	dbgr->attach(cpu,physmem);
	halted = false;
	dumpcpu = false;
	dumpcp0 = false;
	opt_bootmsg = false;
	num_instrs = 0;
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
		if (opt_bootmsg) {
			fprintf(stderr, "Little-Endian host processor detected.\n");
		}
		return 0;
	} else if (x == 0x00010203) {
		machine->host_bigendian = true;
		if (opt_bootmsg) {
			fprintf(stderr, "Big-Endian host processor detected.\n");
		}
		return 0;
	} else {
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
vmips::halt(void)
{
	halted = true;
}

uint32
vmips::auto_size_rom(FILE *rom)
{
	off_t here, there;
	size_t len;

	here = ftell(rom);
	fseek(rom,0,SEEK_END);
	there = ftell(rom);
	fseek(rom,0,SEEK_SET);
	len = there - here;
	len /= 4;
	return len;
}

void
vmips::periodic(void)
{
	/* Process instructions. */
	cpu->periodic();
#if TTY
	if (usetty) console->periodic();
#endif
	clockdev->periodic();

	/* If user requested it, dump registers from CPU and/or CP0. */
	if (dumpcpu)
		cpu->dump_regs_and_stack(stderr);
	if (dumpcp0)
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
vmips::run(int argc, char **argv)
{
	char *image;
	FILE *rom;
	Range *r;
	uint32 loadaddr;
	bool haltdumpcpu, haltdumpcp0, instcounts, memdump, debug;
	extern void setup_disassembler(FILE *stream);
	/* For instruction counting: */
	uint32 memsize;
	struct timeval start, end;
	double elapsed;
#if TTY
	/* For serial emulation: */
	char *ttydev;
	int ttyfd = -1;
	struct termios orig_ts, ts;
	SerialHost *ttyhost;
#endif

	/* Process command line arguments, configuration files, etc. */
	opt = new Options(argc, argv);

	/* Extract important flags and things. */
	image = opt->option("romfile")->str;
	haltdumpcpu = opt->option("haltdumpcpu")->flag;
	haltdumpcp0 = opt->option("haltdumpcp0")->flag;
	dumpcpu = opt->option("dumpcpu")->flag;
	dumpcp0 = opt->option("dumpcp0")->flag;
	instcounts = opt->option("instcounts")->flag;
	opt_bootmsg = opt->option("bootmsg")->flag;
	loadaddr = opt->option("loadaddr")->num;
	memsize = opt->option("memsize")->num;
	memdump = opt->option("memdump")->flag;
#if TTY
	usetty = opt->option("usetty")->flag;
	ttydev = opt->option("ttydev")->str;
#endif
	debug = opt->option("debug")->flag;

	/* Open ROM image. */
	rom = fopen(image,"r");
	if (!rom) {
		fprintf(stderr,"Could not open ROM `%s': %s\n",image,strerror(errno));
		return 2;
	}

	/* Print lots of cutesy information. */
	uint32 rom_size;
	rom_size = auto_size_rom(rom);
	if (opt_bootmsg) {
		fprintf(stderr,"Auto-size ROM image: %ld words.\n",rom_size);
		fprintf(stderr,"Running self tests.\n");
	}

	/* Run self tests. These may or may not become real tests... */
	if (run_self_tests() != 0) {
		fprintf(stderr, "Failed self test %ld\n", test_code);
		return 1;
	} else if (opt_bootmsg) {
		fprintf(stderr,"Self tests passed.\n");
	}

	/* Map the ROM image to the virtual physical memory. */
	if (debug) {
		/* Point the debugger at wherever the user thinks the ROM is. */
		dbgr->setup(loadaddr, rom_size);
	}
	loadaddr -= KSEG1_CONST_TRANSLATION;
	physmem->add_file_mapping(rom, loadaddr, MEM_READ);
	if (opt_bootmsg) {
		r = physmem->find_mapping_range(loadaddr);
		fprintf(stderr,"Mapping ROM image (%s): %lu words at 0x%lx [%lx]\n",
			image, r->getExtent() / 4,
			r->getBase() + KSEG1_CONST_TRANSLATION, r->getBase());
	}

	/* Install RAM at base physical address 0 */
	memmod[0] = new MemoryModule(memsize);
	physmem->add_core_mapping(memmod[0]->addr,0,memmod[0]->len);

	/* Direct the libopcodes disassembler output to stderr. */
	setup_disassembler(stderr);

	/* Microsecond Clock at base physaddr 0x01010000 */
	clockdev = new ClockDev;
	physmem->add_device_mapping(clockdev, 0x01010000);
	intc->connectLine(IRQ7, clockdev);

#if TTY
	if (usetty) {
		/* Serial device at base physaddr 0x02000000 */
		ttyfd = open(ttydev, O_RDWR|O_NONBLOCK);
		if (ttyfd < 0) {
			fprintf(stderr, "open %s: %s\n", ttydev, strerror(errno));
			if (opt_bootmsg)
				fprintf(stderr, "using stdout instead, input disabled\n");
			ttyfd = fileno(stdout);
		}
	/* FIXME:
	 * It would be helpful to restore tty modes on a SIGINT or other
	 * abortive exit. (Sat Mar 25 11:36:55 PST 2000) */
		if (tcgetattr(ttyfd, &orig_ts) < 0) {
			perror("tcgetattr");
		}
		if (tcgetattr(ttyfd, &ts) < 0) {
			perror("tcgetattr");
		}
		ts.c_lflag &= ~(ICANON|ECHO);
		if (tcsetattr(ttyfd, TCSAFLUSH, &ts) < 0) {
			perror("tcsetattr");
		}
		ttyhost = new SerialHost(ttyfd); 
		console = new SPIMConsole(ttyhost, NULL);
		if (opt_bootmsg) {
			fprintf(stderr,
				"Attached SerialHost(fd %d) at 0x%lx to "
				"SPIMConsole [host=0x%lx]\n", ttyfd, (long) ttyhost,
				(long) console);
		}
		physmem->add_device_mapping(console, 0x02000000);
		if (opt_bootmsg) {
			fprintf(stderr,
				"Attached SPIMConsole [host=0x%lx] to phys addr 0x%x\n",
				(long) console, 0x02000000);
			fprintf(stderr, "Connecting IRQ2-IRQ6 to console.\n");
		}
		intc->connectLine(IRQ2, console);
		intc->connectLine(IRQ3, console);
		intc->connectLine(IRQ4, console);
		intc->connectLine(IRQ5, console);
		intc->connectLine(IRQ6, console);
	}
#endif

	if (opt_bootmsg) {
		fprintf(stderr,"Mapped (host=0x%lx) %ldk RAM at base phys addr 0\n",
			(unsigned long) memmod[0]->addr,memmod[0]->len / 1024);

		fprintf(stderr,"\n*************RESET*************\n\n");
	}

	/* Reset the CPU. */
	cpu->reset();

	if (instcounts)
		gettimeofday(&start, NULL);

	if (debug) {
		dbgr->serverloop();
	} else {
		while (! halted) {
			periodic();
		}
	}

	if (instcounts)
		gettimeofday(&end, NULL);

#if TTY
	if (usetty) tcsetattr(ttyfd, TCSAFLUSH, &orig_ts);
#endif

	/* Halt! */
	if (opt_bootmsg) {
		fprintf(stderr,"\n*************HALT*************\n\n");
	}

	/* If user requested it, dump registers from CPU and/or CP0. */
	if (haltdumpcpu || haltdumpcp0) {
		fprintf(stderr,"Dumping:\n");
		if (haltdumpcpu)
			cpu->dump_regs_and_stack(stderr);
		if (haltdumpcp0)
			cpzero->dump_regs_and_tlb(stderr);
	}

	if (instcounts) {
		elapsed = (double) timediff(&end, &start) / 1000000.0;
		fprintf(stderr, "%lu instructions executed in %.5f seconds\n"
			"%.3f instructions per second\n", num_instrs, elapsed,
			((double) num_instrs) / elapsed);
	}

	if (memdump) {
		FILE *ramdump;
		char *dumpfile_name = DUMP_FILE_NAME;

		fprintf(stderr,"Dumping RAM to %s...",dumpfile_name);
		ramdump = fopen(dumpfile_name,"w");
		if (ramdump != NULL) {
			fwrite(memmod[0]->addr,memmod[0]->len,1,ramdump);
			fclose(ramdump);
			fprintf(stderr,"succeeded.\n");
		} else {
			fprintf(stderr,"failed: %s\n",strerror(errno));
		}
	}

	/* We're done. */
	if (opt_bootmsg) {
		fprintf(stderr,"Goodbye.\n");
	}
	return 0;
}

int
main(int argc, char **argv)
{
	int rc;

	machine = new vmips();
	rc = machine->run(argc, argv);
	delete machine; /* No disassemble Number Five!! */
	return rc;
}

