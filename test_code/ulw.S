/*  ../vmips -o excmsg -o haltdumpcpu -o haltbreak ulw.rom */
/*  should end with:  (XXX FIXME) */
/*  tests instructions: lwl, lwr */

#include "asm_regnames.h"
	.text
	.globl __start
__start:
	la a0, foo
	/* These ulw's get translated by the assembler into lwl, lwr pairs */
	ulw t0, 0(a0)
	ulw t1, 1(a0)
	ulw t2, 2(a0)
	ulw t3, 3(a0)
	ulw t4, 4(a0)
	break
foo:
	.byte 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
