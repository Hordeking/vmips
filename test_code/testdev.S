#include "asm_regnames.h"

      .text
      .globl __start

__start:
      li $a0, 0xa0000000
      li $a1, 0xa1000000
      addiu $a2, $0, 200
loop: lw $t0, 0($a1)
      sw $t0, 0($a0)
      addiu $a0, $a0, 4
      addiu $a1, $a1, 4
      addiu $a2, $a2, -1
      bgtz $a2, loop
      break 0

