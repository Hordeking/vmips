/*  ../vmips -o excmsg -o haltdumpcp0 -o haltdumpcpu tlbwr.rom */
/*  should end with:   R08=01234000  R09=00000200 R10=00000000 
/*  PC=bfc00180 NEXT_EPC = bfc00108 */
/*  tests instructions: tlbwr tlbwi tlbr tlbp */

#include "asm_regnames.h"
	.text
	.globl entry
	.ent entry
entry:
	/* test tlbwr */
	li t2, 0x0       /* VPN = 0 */
	mtc0 t2, $10		/* set EntryHi */
	li t2, 0x00000300 /* PPN = 0, Valid, Global bits set */
	mtc0 t2, $2		/* set EntryLo */
	tlbwr           /* write random tlb entry */
	/* try a load and store to VPN 0 */
	sw t2, 0(zero)
	lw t3, 0(zero)
	/* we will have halted by now if tlbwr failed */

	/* test tlbwi */
	li t2, 0x01234000       /* VPN = 0x01234 */
	mtc0 t2, $10		/* set EntryHi */
	li t2, 0x00000200 /* PPN = 0, Valid bit set */
	mtc0 t2, $2		/* set EntryLo */
	mtc0 zero, $0   /* set Index = [probe succeeded] 0 */
	tlbwi            /* write tlb entry 0 */
	
	/* test tlbp */
	li t2, 0x80003f00
	mtc0 t2, $0   /* set Index = [probe failed] 0x3f */
    tlbp             /* probe the tlb */
	mfc0 t2, $0        /* read out Index */
	beqz t2, 1f     /* now, Index should be [probe succeeded] 0 */
	break /* halt if tlbp failed */

	/* test tlbr */
1:	tlbr /* see if we can read out the entry we just read in */
	mfc0 t0, $10   /* read out entryHi */
	mfc0 t1, $2   /* read out entryLo */


	/* we're done*/
	jr ra
	.end entry

