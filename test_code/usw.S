/*  ../vmips -o excmsg -o haltdumpcpu -o haltbreak usw.rom */
/*  should end with:  (XXX FIXME) */
/*  tests instructions: swl, swr */

#include "asm_regnames.h"
	.data
foo1:
	.word 0
	.word 0
foo2:
	.word 0
	.word 0
foo3:
	.word 0
	.word 0
foo4:
	.word 0
	.word 0
foo5:
	.word 0
	.word 0

	.text
	.globl entry
entry:
	li a0, 0x01020304
	la t0, foo1
	la t1, foo2
	la t2, foo3
	la t3, foo4
	la t4, foo5
	/* These usw's get translated by the assembler into swl, swr pairs */
	usw a0, 0(t0)
	usw a0, 1(t1)
	usw a0, 2(t2)
	usw a0, 3(t3)
	usw a0, 4(t4)
	lw $3, 0(t0)
	lw $4, 4(t0)
	lw $5, 0(t1)
	lw $6, 4(t1)
	lw $7, 0(t2)
	lw $8, 4(t2)
	lw $9, 0(t3)
	lw $10, 4(t3)
	lw $11, 0(t4)
	lw $12, 4(t4)
	break
