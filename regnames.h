/* Useful constants for MIPS CPU register names.
   Copyright 2001, 2003 Brian R. Gaeke.

This file is part of VMIPS.

VMIPS is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

VMIPS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with VMIPS; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _REGNAMES_H_
#define _REGNAMES_H_

/* Special names */
#define REG_ZERO 0	/* always zero */
#define REG_AT 1	/* assembler temporary */
#define REG_V0 2	/* function values */
#define REG_V1 3
#define REG_A0 4	/* function arguments */
#define REG_A1 5
#define REG_A2 6
#define REG_A3 7
#define REG_T0 8	/* temporary registers; not preserved across func calls */
#define REG_T1 9
#define REG_T2 10
#define REG_T3 11
#define REG_T4 12
#define REG_T5 13
#define REG_T6 14
#define REG_T7 15
#define REG_S0 16	/* "saved" regs - must preserve these if you use them */
#define REG_S1 17
#define REG_S2 18
#define REG_S3 19
#define REG_S4 20
#define REG_S5 21
#define REG_S6 22
#define REG_S7 23
#define REG_T8 24	/* more temporary regs */
#define REG_T9 25
#define REG_K0 26	/* kernel temporary variables */
#define REG_KT0 26
#define REG_K1 27
#define REG_KT1 27
#define REG_GP 28	/* pointer to globals */
#define REG_SP 29	/* stack pointer */
#define REG_S8 30	/* another "saved" reg */
#define REG_RA 31	/* return address */

#endif /* _REGNAMES_H_ */
