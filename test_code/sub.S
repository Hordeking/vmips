/*  ../vmips -o noinstdump -o haltdumpcpu -o haltbreak sub.rom */
/*  should end w/ pc=bfc00180, next_epc=bfc00180, and R04=00000002 */
/*  R05=fffffffe  R06=00000000  R07=00000000  R08=80000001 R09=7fffffff */
/*  tests instructions: sub */

	.text
	.globl __start

__start:
	addi $4, $0, 1
	addi $5, $0, -1
	addi $6, $0, 0
	addi $7, $0, 1

	sub $4, $4, $5   /* $4 <- 2  POS - NEG */
	sub $5, $5, $7   /* $5 <- -2  NEG - POS */
	sub $7, $7, $7   /* $7 <- 0  POS - POS */

	li $8, 0x80000001
	li $9, 0x7fffffff
	sub $8, $8, $9
	break

	.org 0x180
	break

