/* loadtest_setup.S
 *
 * Starts a new program (loadtest.c) in RAM.
 */
#include "asm_regnames.h"

	.text
	.globl __start

	.ent __start
__start:
	/* Save return address and globals pointer on stack. */
	addiu $sp, $sp, -8
	sw $ra, 4($sp)
	sw $gp, 0($sp)

	/* Set up the globals pointer. */
	la $gp, _gp

	/* Call the function. */
	jal entry

	/* Restore return address and globals pointer. */
	lw $ra, 4($sp)
	lw $gp, 0($sp)

	/* Return to caller (in ROM). */
	jr $31
	.end __start
