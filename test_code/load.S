/*  ../vmips -o haltdumpcpu -o haltbreak load.rom */
/*  should end with: R09=deadbeef R10=0000beef  R11=000000ef */
/*  tests instructions: lw lh lb */

#include "asm_regnames.h"


		.text

		.globl __start
__start:
		la	t0, loser
		lw	t1, 0(t0)
		la	t0, hoser
		lh	t2, 0(t0)
		la	t0, booger
		lb	t3, 0(t0)
		break

loser:	.word	0xDEADBEEF
hoser:	.word	0x1234BEEF
booger:	.word	0x123456EF
