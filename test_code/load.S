
#include "asm_regnames.h"

		.data

loser:	.word	0x80000000
hoser:	.hword	0x8000
booger:	.byte	0x80

		.text
		.globl __start
__start:
		la	$t0, loser
		lw	$t1, 0($t0)
		la	$t0, hoser
		lw	$t2, 0($t0)
		la	$t0, booger
		lw	$t3, 0($t0)
		break

