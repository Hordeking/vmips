#ifndef __regnames_h__
#define __regnames_h__

/* Special names */
#define zero 0	/* always zero */
#define at 1	/* assembler temporary */
#define v0 2	/* function values */
#define v1 3
#define a0 4	/* function arguments */
#define a1 5
#define a2 6
#define a3 7
#define t0 8	/* temporary registers; not preserved across func calls */
#define t1 9
#define t2 10
#define t3 11
#define t4 12
#define t5 13
#define t6 14
#define t7 15
#define s0 16	/* "saved" regs - must preserve these if you use them */
#define s1 17
#define s2 18
#define s3 19
#define s4 20
#define s5 21
#define s6 22
#define s7 23
#define t8 24	/* more temporary regs */
#define t9 25
#define k0 26	/* kernel temporary variables */
#define kt0 26
#define k1 27
#define kt1 27
#define gp 28	/* pointer to globals */
#define sp 29	/* stack pointer */
#define s8 30	/* another "saved" reg */
#define ra 31	/* return address */

/* Rn names - these are pretty boring */
#define r0 0
#define r1 1
#define r2 2
#define r3 3
#define r4 4
#define r5 5
#define r6 6
#define r7 7
#define r8 8
#define r9 9
#define r10 10
#define r11 11
#define r12 12
#define r13 13
#define r14 14
#define r15 15
#define r16 16
#define r17 17
#define r18 18
#define r19 19
#define r20 20
#define r21 21
#define r22 22
#define r23 23
#define r24 24
#define r25 25
#define r26 26
#define r27 27
#define r28 28
#define r29 29
#define r30 30
#define r31 31

/* Exceptions - Cause register ExcCode field */
#define Int 0		/* Interrupt */
#define Mod 1		/* TLB modification exception */
#define TLBL 2		/* TLB exception (load or instruction fetch) */
#define TLBS 3		/* TLB exception (store) */
#define AdEL 4		/* Address error exception (load or instruction fetch) */
#define AdES 5		/* Address error exception (store) */
#define IBE 6		/* Instruction bus error */
#define DBE 7		/* Data (load or store) bus error */
#define Sys 8		/* SYSCALL exception */
#define Bp 9		/* Breakpoint exception (BREAK instruction) */
#define RI 10		/* Reserved instruction exception */
#define CpU 11		/* Coprocessor Unusable */
#define Ov 12		/* Arithmetic Overflow */
#define Tr 13		/* Trap (R4k/R6k only) */
#define NCD 14		/* LDCz or SDCz to uncached address (R6k) */
#define VCEI 14		/* Virtual Coherency Exception (instruction) (R4k) */
#define MV 15		/* Machine check exception (R6k) */
#define FPE 15		/* Floating-point exception (R4k) */
/* 16-22 - reserved */
#define WATCH 23	/* Reference to WatchHi/WatchLo address detected (R4k) */
/* 24-30 - reserved */
#define VCED 31		/* Virtual Coherency Exception (data) (R4k) */

#endif /* __regnames_h__ */
