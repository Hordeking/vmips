/* Just a glue function to turn on clock interrupts. */
#include "asm_regnames.h"

	.globl turn_on_clock_interrupts
	.ent turn_on_clock_interrupts
turn_on_clock_interrupts:
	mfc0 a0, Status
	ori a0, a0, 0x8001
	mtc0 a0, Status
	jr ra
	.end turn_on_clock_interrupts
