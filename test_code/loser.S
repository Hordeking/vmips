/* Using this to test something about what happens when you
 * have an address trans or inst fetch exception w/ delay state NORMAL
 */

	.text
	.globl __start
__start:
a:	add $0, $0, $0

/* the next instruction just isn't here (bus error on inst fetch).
 * (that's why the makefile has an extra rule for this one -- it
 *  needs not to be page-size-padded, it produces a 4-byte ROM file
 *  of the kind described in ../doc/delay)
 * it should cause a fault with EPC = a, which is wrong;
 * the EPC should be (a + 4)
 */
