#ifndef __optiontbl_h__
#define __optiontbl_h__

/* WARNING!
 * This file is parsed by a Perl script (doc/makeoptdoc.pl), and
 * that script is not as smart as the C compiler and preprocessor.
 * Therefore, the format of this file must be as follows...
 *   <junk>
 *   nametable [] = {       // no space btwn nametable and []
 *    { "option-name", TYPE },
 *    Comment explaining option, delimited by slash star star and
 *    star star slash
 *    (repeat as necessary)
 *    { NULL, ... }
 *   }
 *   <junk>
 *   defaults_table [] = {       // no space
 *    "default", "default", ...
 *    NULL
 *   }
 *   <junk>
 * Whitespace is ignored. Stuff marked as <junk> is ignored.
 * In the individual tables, anything after NULL is ignored.
 */

/* This is the official table of options. Each one has a name
 * (an arbitrary character string) and a type, which
 * may be FLAG (i.e., Boolean), STR (string), or NUM (number).
 */

static Option nametable[] = {
	{ "haltdumpcpu", FLAG },
	/** Controls whether the CPU registers will be dumped on halt. **/

	{ "haltdumpcp0", FLAG },
	/** Controls whether the system control coprocessor (CP0) registers
		will be dumped on halt. **/

	{ "excpriomsg", FLAG },
	/** Controls whether exception prioritizing messages will
		be printed.  These messages attempt to explain which of
		a number of exceptions caused by the same instruction
		will be reported. **/

	{ "excmsg", FLAG },
	/** Controls whether every exception will cause a message
		to be printed. The message gives the exception code, a
		short explanation of the exception code, its priority,
		the delay slot state of the virtual CPU, and states
		what type of memory access the exception was caused by,
		if applicable. **/

	{ "bootmsg", FLAG },
	/** Controls whether boot-time and halt-time messages will be printed.
		These include ROM image size, self test messages, reset and halt
		announcements, and possibly other messages. **/

	{ "instdump", FLAG },
	/** Controls whether every instruction executed will be disassembled
		and printed. DEFAULT GOES HERE. The output is in the following format:
        @example
        PC=0xbfc00000 [1fc00000]    24000000 li $zero,0
        @end example
        The first column contains the PC (program counter), followed by
        the physical translation of that address in brackets. The third
        column contains the machine instruction word at that address,
        followed by the assembly language corresponding to that word.
        All of the constants except for the assembly language are in
        hexadecimal. **/

	{ "dumpcpu", FLAG },
	/** Controls whether the CPU registers will be dumped after every
		instruction. DEFAULT GOES HERE. The output is in the following format:
        @example
        Reg Dump:  PC=bfc00080 Last Instr=241f001f HI=00000000 LO=00000000
          DELAY_STATE = NORMAL ; DELAY_PC=00000000 ; NEXT_EPC = bfc0007c
         R00=00000000  R01=00000001  R02=00000002  R03=00000003  R04=00000004 
         ...
         R30=0000001e  R31=0000001f 
        @end example
        (Some values have been omitted for brevity.)
        Here, PC is the program counter, Last Instr is the last instruction
        executed, HI and LO are the multiplication/division result registers,
        DELAY_STATE and DELAY_PC are used in delay slot processing, NEXT_EPC
        is what the Exception PC would be if an exception were to occur, and
        R00 ... R31 are the CPU general purpose registers. All values are in
	    hexadecimal.  **/

	{ "dumpcp0", FLAG },
	/** Controls whether the system control coprocessor (CP0)
		registers will be dumped after every instruction. 
        DEFAULT GOES HERE.  The output is in the following format:
        @example
        CP0 Dump Registers:
         R00=00000100  R01=00001f00  R02=06a5ee00  R03=00000000 
         R04=7fffca10  R05=00000000  R06=00000000  R07=00000000 
         R08=7fb7e0aa  R09=00000000  R10=6f6dd980  R11=00000000 
         R12=00485e18  R13=30002110  R14=4c04a8af  R15=0000703b 
        @end example
        Each of the R00 .. R15 are coprocessor zero registers.
        Their values are displayed in hexadecimal.
        **/

	{ "haltibe", FLAG },
	/** If haltibe is set to TRUE, the virtual machine will halt
		when an instruction fetch causes a bus error (exception
		code 6, Instruction bus error). This is useful if you
		are expecting execution to jump into unmapped areas of
		memory, and you want it to stop instead of calling the
		exception handler. **/

	{ "haltjrra", FLAG },
	/** If haltjrra is set to TRUE, the virtual machine will halt
		when the instruction "jr $31" (also written "jr $ra")
		is encountered.  Since this is the instruction for a
		procedure call to return, this is useful if you have
		a simple procedure to run and you want execution to
		terminate when it finishes. **/

	{ "haltbreak", FLAG },
	/** If haltbreak is set to TRUE, the virtual machine will halt
		when a breakpoint exception is encountered (exception
		code 9). This is equivalent to halting when a "break"
		instruction is encountered. **/

	{ "instcounts", FLAG },
	/** Set instcounts to TRUE if you want to see instruction
		counts, a rough estimate of total runtime, and execution
		speed in instructions per second when the virtual
		machine halts. DEFAULT GOES HERE.  The output is printed
        at the end of the run, and is in the following format:
        @example
        733737 instructions executed in 5.81484 seconds
        126183.545 instructions per second
        @end example
        **/

	{ "romfile", STR },
	/** This is the name of the file which will be initially
		loaded into memory (at the address given in "loadaddr",
		typically 0xbfc00000) and executed when the virtual
		machine is reset. **/

	{ "configfile", STR },
	/** This is the name of the user configuration file. It
		will be ~username-expanded and checked for configuration
		options before the virtual machine boots. **/

	{ "loadaddr", NUM },
	/** This is the virtual address where the ROM will be loaded.
		Note that the MIPS reset exception vector is always 0xbfc00000
		so unless you're doing something incredibly clever you should
		plan to have some executable code at that address. Since the
		caches and TLB are in an indeterminate state at the time of
		reset, the load address must be in uncacheable memory which
		is not mapped through the TLB (kernel segment "kseg1"). This
		effectively constrains the valid range of load addresses to
		between 0xa0000000 and 0xc0000000. **/

	{ "memsize", NUM },
	/** This variable controls the size of the virtual CPU's "physical"
        memory in bytes. You might want to round this off to the nearest
		page; you can determine the pagesize using utils/getpagesize.cc. **/

	{ "memdump", FLAG },
	/** If memdump is set, then the virtual machine will dump its RAM
	    into a file named "memdump.bin" at the end of processing. **/

	{ "reportirq", FLAG },
	/** If reportirq is set, then any change in the interrupt inputs from
	    a device will be reported on stderr. **/

	{ "usetty", FLAG },
	/** If usetty is set, then the SPIM-compatible console device will be
	    configured. If it is not set, then no console device will be
	    available to the virtual machine. **/

	{ "ttydev", STR },
	/** This pathname will be used as the device from which reads
		from the console device will take their data, and to which writes
		to the console device will send their data. **/

	{ "debug", FLAG },
	/** If debug is set, then the gdb remote serial protocol backend will
        be enabled in the virtual machine. This will cause the machine to
	    wait for gdb to attach and "continue" before booting the ROM file.
	    If debug is not set, then the machine will boot the ROM file
	    without pausing. **/

	{ "realtime", FLAG },
	/** If realtime is set, then the clock device will cause simulated
        time to run at some fraction of real time, determined by the
        `timeratio' option. If realtime is not set, then simulated time
        will run at the speed given by the `clockspeed' option.  **/

    { "timeratio", NUM },
    /** If the realtime option is set, this option gives the number of times
        slower than real time at which simulated time will run. It has
        no effect if realtime is not set. **/

    { "clockspeed", NUM },
    /** If the realtime option is not set, this option gives the speed of
        simulated time in Hz.  It has no effect if realtime is set. **/

    { "clockintr", NUM },
    /** This option gives the frequency of clock interrupts, in nanoseconds
        of simulated time. **/

	{ NULL, 0 }
};

/* This is the official default options list. */
static char *defaults_table[] = {
	"nohaltdumpcpu", "nohaltdumpcp0", "noexcpriomsg",
	"noexcmsg", "bootmsg", "instdump", "nodumpcpu", "nodumpcp0",
	"haltibe", "nohaltjrra", "haltbreak", "romfile=romfile.rom",
	"configfile=~/.vmipsrc", "loadaddr=0xbfc00000", "noinstcounts",
	"memsize=0x100000", "nomemdump", "noreportirq", "usetty",
	"ttydev=/dev/tty", "nodebug", "norealtime", "timeratio=200",
	"clockspeed=25000000", "clockintr=1000", NULL
};

#endif /* __optiontbl_h__ */
