
	.text
	.globl __start
__start:
	li $3, 0xffffffff
	addiu $2, $0, 1
	.set noreorder
	beq $2, $2, loozer
	lw $0, 0($3) # cause a branch delay slot bus error
loozer: 
	break

