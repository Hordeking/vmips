#include "cpu.h"
#include "vmips.h"
#include "options.h"
#include "regnames.h"
#include "cpzeroreg.h"

char *const
CPU::strdelaystate(const int state)
{
	static char *statestr[] = { "NORMAL", "DELAYING", "DELAYSLOT" };

	return statestr[state];
}

CPU::CPU(vmips *mch = NULL, Mapper *m = NULL, CPZero *cp0 = NULL)
{
	delay_state = NORMAL;
	attach(mch,m,cp0);
}

void
CPU::attach(vmips *mch = NULL, Mapper *m = NULL, CPZero *cp0 = NULL)
{
	reg[zero] = 0;
	if (mch) machine = mch;
	if (m) mem = m;
	if (cp0) cpzero = cp0;
}

void
CPU::reset(void)
{
#ifdef INTENTIONAL_CONFUSION
	int r;
	for (r = 0; r < 32; r++) {
		reg[r] = random();
	}
#endif /* INTENTIONAL_CONFUSION */
	reg[zero] = 0;
	pc = 0xbfc00000;
	cpzero->reset();
}

void
CPU::dump_regs(FILE *f)
{
	int i;

	fprintf(f,"Reg Dump:  PC=%08lx Last Instr=%08lx HI=%08lx LO=%08lx\n",
		pc,instr,hi,lo);
	fprintf(f,"  DELAY_STATE = %s ; DELAY_PC=%08lx ; NEXT_EPC = %08lx\n",
		strdelaystate(delay_state), delay_pc, next_epc);
	for (i = 0; i < 32; i++) {
		fprintf(f," R%02d=%08lx ",i,reg[i]);
		if (i % 5 == 4 || i == 31) {
			fputc('\n',f);
		}
	}
}

void
CPU::dump_regs_and_stack(FILE *f)
{
	uint32 stackphys;

	dump_regs(f);
	if (cpzero->debug_tlb_translate(reg[sp], &stackphys)) {
		mem->dump_stack(f, stackphys);
	} else {
		fprintf(f, "(stack not mapped in TLB)\n");
	}
}

/* Instruction decoding */
uint16
CPU::opcode(const uint32 i) const
{
	return (i >> 26) & 0x03f;
}

uint16
CPU::rs(const uint32 i) const
{
	return (i >> 21) & 0x01f;
}

uint16
CPU::rt(const uint32 i) const
{
	return (i >> 16) & 0x01f;
}

uint16
CPU::rd(const uint32 i) const
{
	return (i >> 11) & 0x01f;
}

uint16
CPU::immed(const uint32 i) const
{
	return i & 0x0ffff;
}

short
CPU::s_immed(const uint32 i) const
{
	return i & 0x0ffff;
}

uint16
CPU::shamt(const uint32 i) const
{
	return (i >> 6) & 0x01f;
}

uint16
CPU::funct(const uint32 i) const
{
	return i & 0x03f;
}

uint32
CPU::jumptarg(const uint32 i) const
{
	return i & 0x03ffffff;
}

/* exception handling */
char *const
CPU::strexccode(const uint16 excCode)
{
	char *const exception_strs[] =
	{
		/* 0 */ "Interrupt",
		/* 1 */ "TLB modification exception",
		/* 2 */ "TLB exception (load or instr fetch)",
		/* 3 */ "TLB exception (store)",
		/* 4 */ "Address error exception (load or instr fetch)",
		/* 5 */ "Address error exception (store)",
		/* 6 */ "Instruction bus error",
		/* 7 */ "Data (load or store) bus error",
		/* 8 */ "SYSCALL exception",
		/* 9 */ "Breakpoint32 exception (BREAK instr)",
		/* 10 */ "Reserved instr exception",
		/* 11 */ "Coprocessor Unusable",
		/* 12 */ "Arithmetic Overflow",
		/* 13 */ "Trap (R4k/R6k only)",
		/* 14 */ "LDCz or SDCz to uncached address (R6k)",
		/* 14 */ "Virtual Coherency Exception (instr) (R4k)",
		/* 15 */ "Machine check exception (R6k)",
		/* 15 */ "Floating-point32 exception (R4k)",
		/* 16 */ "Exception 16 (reserved)",
		/* 17 */ "Exception 17 (reserved)",
		/* 18 */ "Exception 18 (reserved)",
		/* 19 */ "Exception 19 (reserved)",
		/* 20 */ "Exception 20 (reserved)",
		/* 21 */ "Exception 21 (reserved)",
		/* 22 */ "Exception 22 (reserved)",
		/* 23 */ "Reference to WatchHi/WatchLo address detected (R4k)",
		/* 24 */ "Exception 24 (reserved)",
		/* 25 */ "Exception 25 (reserved)",
		/* 26 */ "Exception 26 (reserved)",
		/* 27 */ "Exception 27 (reserved)",
		/* 28 */ "Exception 28 (reserved)",
		/* 29 */ "Exception 29 (reserved)",
		/* 30 */ "Exception 30 (reserved)",
		/* 31 */ "Virtual Coherency Exception (data) (R4k)"
	};

	return exception_strs[excCode];
}

char *const
CPU::strmemmode(const int memmode)
{
	char *const memmode_strs[] =
	{
		"instruction fetch", /* INSTFETCH */
		"data load", /* DATALOAD */
		"data store", /* DATASTORE */
		"not applicable" /* ANY */
	};

	return memmode_strs[memmode];
}

int
CPU::exception_priority(uint16 excCode, int mode)
{
	/* See doc/excprio for an explanation of this table. */
	struct excPriority *p, prio[] = {
		{1, AdEL, INSTFETCH},
		{2, TLBL, INSTFETCH}, {2, TLBS, INSTFETCH},
		{3, IBE, ANY},
		{4, Ov, ANY}, {4, Tr, ANY}, {4, Sys, ANY},
		{4, Bp, ANY}, {4, RI, ANY}, {4, CpU, ANY},
		{5, AdEL, DATALOAD}, {5, AdES, ANY},
		{6, TLBL, DATALOAD}, {6, TLBS, DATALOAD},
		{6, TLBL, DATASTORE}, {6, TLBS, DATASTORE},
		{7, Mod, ANY},
		{8, DBE, ANY},
		{9, Int, ANY},
		{0, ANY, ANY} /* catch-all */
	};
	bool excpriomsg = machine->opt->option("excpriomsg")->flag;

	for (p = prio; p->priority != 0; p++) {
		if (excCode == p->excCode || p->excCode == ANY) {
			if (mode == p->mode || p->mode == ANY) {
				return p->priority;
			} else if (excpriomsg) {
				fprintf(stderr,
					"exception code matches but mode %d != table %d\n",
					mode,p->mode);
			}
		} else if (excpriomsg) {
			fprintf(stderr, "exception code %d != table %d\n", excCode,
				p->excCode);
		}
	}
	return 0;
}

void
CPU::exception(uint16 excCode, int mode = ANY, int coprocno = -1)
{
	static uint32 last_epc = 0;
	static int last_prio = 0;
	int prio;
	uint32 base, vector, epc;
	bool delaying = (delay_state == DELAYSLOT);
	bool haltbreak = machine->opt->option("haltbreak")->flag,
		haltibe = machine->opt->option("haltibe")->flag,
		excpriomsg = machine->opt->option("excpriomsg")->flag,
		excmsg = machine->opt->option("excmsg")->flag;

	if (haltbreak) {
		if (excCode == Bp) {
			fprintf(stderr,"* BREAK instruction reached -- HALTING *\n");
			machine->halt();
		}
	}
	if (haltibe) {
		if (excCode == IBE) {
			fprintf(stderr,"* Instruction bus error occurred -- HALTING *\n");
			machine->halt();
		}
	}

	/* periodic() ensures that next_epc will always contain the correct
	 * EPC whenever exception() is called.
	 */
	epc = next_epc;

	/* Prioritize exception -- if the last exception to occur _also_ was
	 * caused by this EPC, only report this exception if it has a higher
	 * priority.  Otherwise, exception handling terminates here,
	 * because only one exception will be reported per instruction
	 * (as per MIPS RISC Architecture, p. 6-35). Note that this only
	 * applies IFF the previous exception was caught during the current
	 * _execution_ of the instruction at this EPC, so we check that
	 * EXCEPTION_PENDING is true before aborting exception handling.
	 * (This flag is reset by each call to periodic().)
	 */
	prio = exception_priority(excCode, mode);
	if (epc == last_epc) {
		if (prio <= last_prio && exception_pending) {
			if (excpriomsg) {
				fprintf(stderr,
					"(Ignoring additional lower priority exception...)\n");
			}
			return;
		} else {
			last_prio = prio;
		}
	}
	last_epc = epc;

	/* Set processor to Kernel mode, disable interrupts, and save 
	 * exception PC.
	 */
	cpzero->enter_exception(epc,excCode,coprocno,delaying);

	/* Calculate the exception handler address; this is of the form BASE +
	 * VECTOR. The BASE is determined by whether we're using boot-time
	 * exception vectors, according to the BEV bit in the CP0 Status register.
	 */
	if (cpzero->use_boot_excp_address()) {
		base = 0xbfc00100;
	} else {
		base = 0x80000000;
	}

	/* Do we have a User TLB Miss exception? If so, jump to the
	 * User TLB Miss exception vector, otherwise jump to the
	 * common exception vector.
	 */
	if ((excCode == TLBL || excCode == TLBS) && (cpzero->tlb_miss_user)) {
		vector = 0x000;
	} else {
		vector = 0x080;
	}

	if (excmsg) {
		fprintf(stderr,"Exception %d (%s) triggered, EPC=%lx\n", excCode, 
			strexccode(excCode), epc);
		fprintf(stderr,
			" Priority is %d; delay state is %s; mem access mode is %s\n",
			prio, strdelaystate(delay_state), strmemmode(mode));
	}
	pc = base + vector;
	exception_pending = true;
}

/* emulation of instructions */
void
CPU::cpzero_emulate(uint32 instr, uint32 pc)
{
	cpzero->cpzero_emulate(instr, pc);
}

void
CPU::cpone_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP1 instruction %lx not implemented at pc=0x%lx\n",
		instr,pc);
	exception(CpU,ANY,1);
}

void
CPU::cptwo_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP2 instruction %lx not implemented at pc=0x%lx\n",
		instr,pc);
	exception(CpU,ANY,2);
}

void
CPU::cpthree_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP3 instruction %lx not implemented at pc=0x%lx\n",
		instr,pc);
	exception(CpU,ANY,3);
}

/* Return the address to jump to as a result of the J-format
 * (jump) instruction INSTR at address PC.
 * (PC is the address of the jump instruction, and INSTR is
 * the jump instruction word.)
 */
uint32
CPU::calc_jump_target(uint32 instr, uint32 pc)
{
	/* Must use address of delay slot (pc + 4) to calculate. */
	return ((pc + 4) & 0xf0000000) | (jumptarg(instr) << 2);
}

void
CPU::j_emulate(uint32 instr, uint32 pc)
{
	delay_state = DELAYING;
	delay_pc = calc_jump_target(instr, pc);
}

void
CPU::jal_emulate(uint32 instr, uint32 pc)
{
	delay_state = DELAYING;
	delay_pc = calc_jump_target(instr, pc);
	/* RA gets addr of instr after delay slot (2 words after this one). */
	reg[ra] = pc + 8;
}

/* Take the PC-relative branch for which the offset is specified by
 * the immediate field of the branch instruction word INSTR, with
 * the program counter equal to PC.
 */
void
CPU::branch(uint32 instr, uint32 pc)
{
	delay_state = DELAYING;
	delay_pc = (pc + 4) + (s_immed(instr) << 2);
}

void
CPU::beq_emulate(uint32 instr, uint32 pc)
{
	if (reg[rs(instr)] == reg[rt(instr)]) {
		branch(instr, pc);
	}
}

void
CPU::bne_emulate(uint32 instr, uint32 pc)
{
	if (reg[rs(instr)] != reg[rt(instr)]) {
		branch(instr, pc);
	}
}

void
CPU::blez_emulate(uint32 instr, uint32 pc)
{
	if (rt(instr) != 0) {
		exception(RI);
	}
	if (reg[rs(instr)] == 0 || (reg[rs(instr)] & 0x80000000)) {
		branch(instr, pc);
	}
}

void
CPU::bgtz_emulate(uint32 instr, uint32 pc)
{
	if (rt(instr) != 0) {
		exception(RI);
		return;
	}
	if (reg[rs(instr)] != 0 && (reg[rs(instr)] & 0x80000000) == 0) {
		branch(instr, pc);
	}
}

void
CPU::addi_emulate(uint32 instr, uint32 pc)
{
	int32 a, b, sum;

	a = (int32)reg[rs(instr)];
	b = s_immed(instr);
	sum = a + b;
	if ((a < 0 && b < 0 && !(sum < 0)) || (a >= 0 && b >= 0 && !(sum >= 0))) {
		exception(Ov);
		return;
	} else {
		reg[rt(instr)] = (uint32)sum;
	}
}

void
CPU::addiu_emulate(uint32 instr, uint32 pc)
{
	int32 a, b, sum;

	a = (int32)reg[rs(instr)];
	b = s_immed(instr);
	sum = a + b;
	reg[rt(instr)] = (uint32)sum;
}

void
CPU::slti_emulate(uint32 instr, uint32 pc)
{
	int32 s_rs = reg[rs(instr)];

	if (s_rs < s_immed(instr)) {
		reg[rt(instr)] = 1;
	} else {
		reg[rt(instr)] = 0;
	}
}

void
CPU::sltiu_emulate(uint32 instr, uint32 pc)
{
	if (reg[rs(instr)] < immed(instr)) {
		reg[rt(instr)] = 1;
	} else {
		reg[rt(instr)] = 0;
	}
}

void
CPU::andi_emulate(uint32 instr, uint32 pc)
{
	reg[rt(instr)] = (reg[rs(instr)] & 0x0ffff) & immed(instr);
}

void
CPU::ori_emulate(uint32 instr, uint32 pc)
{
	reg[rt(instr)] = reg[rs(instr)] | immed(instr);
}

void
CPU::xori_emulate(uint32 instr, uint32 pc)
{
	reg[rt(instr)] = reg[rs(instr)] ^ immed(instr);
}

void
CPU::lui_emulate(uint32 instr, uint32 pc)
{
	reg[rt(instr)] = immed(instr) << 16;
}

void
CPU::lb_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base;
	int8 byte;
	int32 offset;
	bool cacheable;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATALOAD, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Fetch byte.
	 * Because it is assigned to a signed variable (int32 byte)
	 * it will be sign-extended.
	 */
	byte = mem->fetch_byte(phys, cacheable, this);
	if ((byte & 0xff == 0xff) && exception_pending) return;

	/* Load target register with data. */
	reg[rt(instr)] = byte;
}

void
CPU::lh_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base;
	int16 halfword;
	int32 offset;
	bool cacheable;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* This virtual address must be halfword-aligned. */
	if (virt % 2 != 0) {
		exception(AdEL,DATALOAD);
		return;
	}

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATALOAD, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Fetch halfword.
	 * Because it is assigned to a signed variable (int32 halfword)
	 * it will be sign-extended.
	 */
	halfword = mem->fetch_halfword(phys, cacheable, this);
	if ((halfword & 0xffff == 0xffff) && exception_pending) return;

	/* Load target register with data. */
	reg[rt(instr)] = halfword;
}

/* The lwr and lwl algorithms here are taken from SPIM 6.0,
 * since I didn't manage to come up with a better way to write them.
 * Improvements are welcome.
 */
uint32 lwr(uint32 regval, uint32 memval, uint8 offset)
{
#if defined(TARGET_BIG_ENDIAN)
		switch (offset)
		{
			case 0: return (regval & 0xffffff00) |
						((unsigned)(memval & 0xff000000) >> 24);
			case 1: return (regval & 0xffff0000) |
						((unsigned)(memval & 0xffff0000) >> 16);
			case 2: return (regval & 0xff000000) |
						((unsigned)(memval & 0xffffff00) >> 8);
			case 3: return memval;
		}
#elif defined(TARGET_LITTLE_ENDIAN)
		switch (offset)
		{
			/* The SPIM source claims that "The description of the
			 * little-endian case in Kane is totally wrong." The fact
			 * that I ripped off the LWR algorithm from them could be
			 * viewed as a sort of passive assumption that their claim
			 * is correct.
			 */
			case 0: /* 3 in book */
				return memval;
			case 1: /* 0 in book */
				return (regval & 0xff000000) | ((memval & 0xffffff00) >> 8);
			case 2: /* 1 in book */
				return (regval & 0xffff0000) | ((memval & 0xffff0000) >> 16);
			case 3: /* 2 in book */
				return (regval & 0xffffff00) | ((memval & 0xff000000) >> 24);
		}
#endif
	fprintf(stderr, "Invalid offset %x passed to lwr\n", offset);
	abort();
}

uint32 lwl(uint32 regval, uint32 memval, uint8 offset)
{
#if defined(TARGET_BIG_ENDIAN)
		switch (offset)
		{
			case 0: return memval;
			case 1: return (memval & 0xffffff) << 8 | (regval & 0xff);
			case 2: return (memval & 0xffff) << 16 | (regval & 0xffff);
			case 3: return (memval & 0xff) << 24 | (regval & 0xffffff);
		}
#elif defined(TARGET_LITTLE_ENDIAN)
		switch (offset)
		{
			case 0: return (memval & 0xff) << 24 | (regval & 0xffffff);
			case 1: return (memval & 0xffff) << 16 | (regval & 0xffff);
			case 2: return (memval & 0xffffff) << 8 | (regval & 0xff);
			case 3: return memval;
		}
#endif
	fprintf(stderr, "Invalid offset %x passed to lwl\n", offset);
	abort();
}

void
CPU::lwl_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, wordvirt, base, memword;
	uint8 which_byte;
	int32 offset;
	bool cacheable;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;
	/* We request the word containing the byte-address requested. */
	wordvirt = virt & ~0x03UL;

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(wordvirt, DATALOAD, &cacheable, this);
	if (phys == 0xffffffff && exception_pending) return;

	/* Fetch word. */
	memword = mem->fetch_word(phys, DATALOAD, cacheable, this);
	if (memword == 0xffffffff && exception_pending) return;
	
	/* Insert bytes into the left side of the register. */
	which_byte = virt & 0x03;
	reg[rt(instr)] = lwl(reg[rt(instr)], memword, which_byte);
}

void
CPU::lw_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base, word;
	int32 offset;
	bool cacheable;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* This virtual address must be word-aligned. */
	if (virt % 4 != 0) {
		exception(AdEL,DATALOAD);
		return;
	}

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATALOAD, &cacheable, this);
	if (phys == 0xffffffff && exception_pending) return;

	/* Fetch word. */
	word = mem->fetch_word(phys, DATALOAD, cacheable, this);
	if (word == 0xffffffff && exception_pending) return;

	/* Load target register with data. */
	reg[rt(instr)] = word;
}

void
CPU::lbu_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base, byte;
	int32 offset;
	bool cacheable;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATALOAD, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Fetch byte.  */
	byte = mem->fetch_byte(phys, cacheable, this) & 0x000000ff;
	if ((byte & 0xff == 0xff) && exception_pending) return;

	/* Load target register with data. */
	reg[rt(instr)] = byte;
}

void
CPU::lhu_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base, halfword;
	int32 offset;
	bool cacheable;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* This virtual address must be halfword-aligned. */
	if (virt % 2 != 0) {
		exception(AdEL,DATALOAD);
		return;
	}

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATALOAD, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Fetch halfword.  */
	halfword = mem->fetch_halfword(phys, cacheable, this) & 0x0000ffff;
	if ((halfword & 0xffff == 0xffff) && exception_pending) return;

	/* Load target register with data. */
	reg[rt(instr)] = halfword;
}

void
CPU::lwr_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, wordvirt, base, memword;
	uint8 which_byte;
	int32 offset;
	bool cacheable;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;
	/* We request the word containing the byte-address requested. */
	wordvirt = virt & ~0x03UL;

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(wordvirt, DATALOAD, &cacheable, this);
	if (phys == 0xffffffff && exception_pending) return;

	/* Fetch word. */
	memword = mem->fetch_word(phys, DATALOAD, cacheable, this);
	if (memword == 0xffffffff && exception_pending) return;
	
	/* Insert bytes into the left side of the register. */
	which_byte = virt & 0x03;
	reg[rt(instr)] = lwr(reg[rt(instr)], memword, which_byte);
}

void
CPU::sb_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base;
	uint8 data;
	int32 offset;
	bool cacheable;

	/* Load data from register. */
	data = reg[rt(instr)] & 0x0ff;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATASTORE, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Store byte. */
	mem->store_byte(phys, data, cacheable, this);
}

void
CPU::sh_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base;
	uint16 data;
	int32 offset;
	bool cacheable;

	/* Load data from register. */
	data = reg[rt(instr)] & 0x0ffff;

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* This virtual address must be halfword-aligned. */
	if (virt % 2 != 0) {
		exception(AdES,DATASTORE);
		return;
	}

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATASTORE, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Store halfword. */
	mem->store_halfword(phys, data, cacheable, this);
}

uint32 swl(uint32 regval, uint32 memval, uint8 offset)
{
#if defined(TARGET_BIG_ENDIAN)
		switch (offset) {
			case 0: return regval; 
			case 1: return (memval & 0xff000000) | (regval >> 8 & 0xffffff); 
			case 2: return (memval & 0xffff0000) | (regval >> 16 & 0xffff); 
			case 3: return (memval & 0xffffff00) | (regval >> 24 & 0xff); 
		}
#elif defined(TARGET_LITTLE_ENDIAN)
		switch (offset) {
			case 0: return (memval & 0xffffff00) | (regval >> 24 & 0xff); 
			case 1: return (memval & 0xffff0000) | (regval >> 16 & 0xffff); 
			case 2: return (memval & 0xff000000) | (regval >> 8 & 0xffffff); 
			case 3: return regval; 
		}
#endif
	fprintf(stderr, "Invalid offset %x passed to swl\n", offset);
	abort();
}

uint32 swr(uint32 regval, uint32 memval, uint8 offset)
{
#if defined(TARGET_BIG_ENDIAN)
		switch (offset) {
			case 0: return ((regval << 24) & 0xff000000) | (memval & 0xffffff); 
			case 1: return ((regval << 16) & 0xffff0000) | (memval & 0xffff); 
			case 2: return ((regval << 8) & 0xffffff00) | (memval & 0xff); 
			case 3: return regval; 
		}
#elif defined(TARGET_LITTLE_ENDIAN)
		switch (offset) {
			case 0: return regval; 
			case 1: return ((regval << 8) & 0xffffff00) | (memval & 0xff); 
			case 2: return ((regval << 16) & 0xffff0000) | (memval & 0xffff); 
			case 3: return ((regval << 24) & 0xff000000) | (memval & 0xffffff); 
		}
#endif
	fprintf(stderr, "Invalid offset %x passed to swr\n", offset);
	abort();
}

void
CPU::swl_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, wordvirt, base, regdata, memdata;
	int32 offset;
	uint8 which_byte;
	bool cacheable;

	/* Load data from register. */
	regdata = reg[rt(instr)];

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;
	/* We request the word containing the byte-address requested. */
	wordvirt = virt & ~0x03UL;

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(wordvirt, DATASTORE, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Read data from memory. */
	memdata = mem->fetch_word(phys, DATASTORE, cacheable, this);
	if (wordvirt == 0xffffffffUL && exception_pending) return;

	/* Write back the left side of the register. */
	which_byte = virt & 0x03UL;
	mem->store_word(phys, swl(regdata, memdata, which_byte), cacheable, this);
}

void
CPU::sw_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, base, data;
	int32 offset;
	bool cacheable;

	/* Load data from register. */
	data = reg[rt(instr)];

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;

	/* This virtual address must be word-aligned. */
	if (virt % 4 != 0) {
		exception(AdES,DATASTORE);
		return;
	}

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(virt, DATASTORE, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Store word. */
	mem->store_word(phys, data, cacheable, this);
}

void
CPU::swr_emulate(uint32 instr, uint32 pc)
{
	uint32 phys, virt, wordvirt, base, regdata, memdata;
	int32 offset;
	uint8 which_byte;
	bool cacheable;

	/* Load data from register. */
	regdata = reg[rt(instr)];

	/* Calculate virtual address. */
	base = reg[rs(instr)];
	offset = s_immed(instr);
	virt = base + offset;
	/* We request the word containing the byte-address requested. */
	wordvirt = virt & ~0x03UL;

	/* Translate virtual address to physical address. */
	phys = cpzero->address_trans(wordvirt, DATASTORE, &cacheable, this);
	if (phys == 0xffffffffUL && exception_pending) return;

	/* Read data from memory. */
	memdata = mem->fetch_word(phys, DATASTORE, cacheable, this);
	if (wordvirt == 0xffffffffUL && exception_pending) return;

	/* Write back the right side of the register. */
	which_byte = virt & 0x03UL;
	mem->store_word(phys, swr(regdata, memdata, which_byte), cacheable, this);
}

void
CPU::lwc1_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP1 instruction %lx not implemented at pc=0x%lx",instr,pc);
	exception(CpU,ANY,1);
}

void
CPU::lwc2_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP2 instruction %lx not implemented at pc=0x%lx",instr,pc);
	exception(CpU,ANY,2);
}

void
CPU::lwc3_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP3 instruction %lx not implemented at pc=0x%lx",instr,pc);
	exception(CpU,ANY,3);
}

void
CPU::swc1_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP1 instruction %lx not implemented at pc=0x%lx",instr,pc);
	exception(CpU,ANY,1);
}

void
CPU::swc2_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP2 instruction %lx not implemented at pc=0x%lx",instr,pc);
	exception(CpU,ANY,2);
}

void
CPU::swc3_emulate(uint32 instr, uint32 pc)
{
	fprintf(stderr,"CP3 instruction %lx not implemented at pc=0x%lx",instr,pc);
	exception(CpU,ANY,3);
}

void
CPU::sll_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = reg[rt(instr)] << shamt(instr);
}

int32
srl(int32 a, int32 b)
{
	return (a >> b) & ((1 << (32 - b)) - 1);
}

int32
sra(int32 a, int32 b)
{
	return (a >> b) | (((a >> 31) & 0x01) * (((1 << b) - 1) << (32 - b)));
}

void
CPU::srl_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = srl(reg[rt(instr)], shamt(instr));
}

void
CPU::sra_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = sra(reg[rt(instr)], shamt(instr));
}

void
CPU::sllv_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = reg[rt(instr)] << (reg[rs(instr)] & 0x01f);
}

void
CPU::srlv_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = srl(reg[rt(instr)], reg[rs(instr)] & 0x01f);
}

void
CPU::srav_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = sra(reg[rt(instr)], reg[rs(instr)] & 0x01f);
}

void
CPU::jr_emulate(uint32 instr, uint32 pc)
{
	bool haltjrra = machine->opt->option("haltjrra")->flag;

	if (haltjrra) {
		if (rs(instr) == ra) {
			fprintf(stderr,
				"** Procedure call return instr reached -- HALTING **\n");
			machine->halt();
			return;
		}
	}
	if (reg[rd(instr)] != 0) {
		exception(RI);
	}
	delay_state = DELAYING;
	delay_pc = reg[rs(instr)];
}

void
CPU::jalr_emulate(uint32 instr, uint32 pc)
{
	delay_state = DELAYING;
	delay_pc = reg[rs(instr)];
	/* RA gets addr of instr after delay slot (2 words after this one). */
	reg[rd(instr)] = pc + 8;
}

void
CPU::syscall_emulate(uint32 instr, uint32 pc)
{
	exception(Sys);
}

void
CPU::break_emulate(uint32 instr, uint32 pc)
{
	exception(Bp);
}

void
CPU::mfhi_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = hi;
}

void
CPU::mthi_emulate(uint32 instr, uint32 pc)
{
	if (rd(instr) != 0) {
		exception(RI);
		return;
	}
	hi = reg[rs(instr)];
}

void
CPU::mflo_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = lo;
}

void
CPU::mtlo_emulate(uint32 instr, uint32 pc)
{
	if (rd(instr) != 0) {
		exception(RI);
		return;
	}
	lo = reg[rs(instr)];
}

void
CPU::mult_emulate(uint32 instr, uint32 pc)
{
	if (rd(instr) != 0) {
		exception(RI);
		return;
	}
	mult64s(&hi, &lo, reg[rs(instr)], reg[rt(instr)]);
}

void
CPU::mult64(uint32 *hi, uint32 *lo, uint32 n, uint32 m)
{
#ifdef HAVE_LONG_LONG
	uint64 result;
	result = ((uint64)n) * ((uint64)m);
	*hi = (uint32) (result >> 32);
	*lo = (uint32) result;
#else /* HAVE_LONG_LONG */
	/*           n = (w << 16) | x ; m = (y << 16) | z
	 *     w x   g = a + e ; h = b + f ; p = 65535
	 *   X y z   c = (z * x) mod p
	 *   -----   b = (z * w + ((z * x) div p)) mod p
	 *   a b c   a = (z * w + ((z * x) div p)) div p
	 * d e f     f = (y * x) mod p
	 * -------   e = (y * w + ((y * x) div p)) mod p
	 * i g h c   d = (y * w + ((y * x) div p)) div p
	 */
	uint16 w,x,y,z,a,b,c,d,e,f,g,h,i;
	uint32 p;
	p = 65536;
	w = (n >> 16) & 0x0ffff;
	x = n & 0x0ffff;
	y = (m >> 16) & 0x0ffff;
	z = m & 0x0ffff;
	c = (z * x) % p;
	b = (z * w + ((z * x) / p)) % p;
	a = (z * w + ((z * x) / p)) / p;
	f = (y * x) % p;
	e = (y * w + ((y * x) / p)) % p;
	d = (y * w + ((y * x) / p)) / p;
	h = (b + f) % p;
	g = ((a + e) + ((b + f) / p)) % p;
	i = d + (((a + e) + ((b + f) / p)) / p);
	*hi = (i << 16) | g;
	*lo = (h << 16) | c;
#endif /* HAVE_LONG_LONG */
}

void
CPU::mult64s(uint32 *hi, uint32 *lo, int32 n, int32 m)
{
#ifdef HAVE_LONG_LONG
	int64 result;
	result = ((int64)n) * ((int64)m);
	*hi = (uint32) (result >> 32);
	*lo = (uint32) result;
#else /* HAVE_LONG_LONG */
	int32 result_sign = (n<0)^(m<0);
	int32 n_abs = n;
	int32 m_abs = m;

	if (n_abs < 0) n_abs = -n_abs;
	if (m_abs < 0) m_abs = -m_abs;

	mult64(hi,lo,n_abs,m_abs);
	if (result_sign)
	{
		*hi = ~*hi;
		*lo = ~*lo;
		if (*lo & 0x80000000)
		{
			*lo += 1;
			if (!(*lo & 0x80000000))
			{
				*hi += 1;
			}
		}
		else
		{
			*lo += 1;
		}
	}
#endif /* HAVE_LONG_LONG */
}

void
CPU::multu_emulate(uint32 instr, uint32 pc)
{
	if (rd(instr) != 0) {
		exception(RI);
		return;
	}
	mult64(&hi, &lo, reg[rs(instr)], reg[rt(instr)]);
}

void
CPU::div_emulate(uint32 instr, uint32 pc)
{
	int32 signed_rs = (int32)reg[rs(instr)];
	int32 signed_rt = (int32)reg[rt(instr)];
	lo = signed_rs / signed_rt;
	hi = signed_rs % signed_rt;
}

void
CPU::divu_emulate(uint32 instr, uint32 pc)
{
	lo = reg[rs(instr)] / reg[rt(instr)];
	hi = reg[rs(instr)] % reg[rt(instr)];
}

void
CPU::add_emulate(uint32 instr, uint32 pc)
{
	int32 a, b, sum;
	a = (int32)reg[rs(instr)];
	b = (int32)reg[rt(instr)];
	sum = a + b;
	if ((a < 0 && b < 0 && !(sum < 0)) || (a >= 0 && b >= 0 && !(sum >= 0))) {
		exception(Ov);
		return;
	} else {
		reg[rd(instr)] = (uint32)sum;
	}
}

void
CPU::addu_emulate(uint32 instr, uint32 pc)
{
	int32 a, b, sum;
	a = (int32)reg[rs(instr)];
	b = (int32)reg[rt(instr)];
	sum = a + b;
	reg[rd(instr)] = (uint32)sum;
}

void
CPU::sub_emulate(uint32 instr, uint32 pc)
{
	int32 a, b, diff;
	a = (int32)reg[rs(instr)];
	b = (int32)reg[rt(instr)];
	diff = a - b;
	if ((a < 0 && !(b < 0) && !(diff < 0)) || (!(a < 0) && b < 0 && diff < 0)) {
		exception(Ov);
		return;
	} else {
		reg[rd(instr)] = (uint32)diff;
	}
}

void
CPU::subu_emulate(uint32 instr, uint32 pc)
{
	int32 a, b, diff;
	a = (int32)reg[rs(instr)];
	b = (int32)reg[rt(instr)];
	diff = a - b;
	reg[rd(instr)] = (uint32)diff;
}

void
CPU::and_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = reg[rs(instr)] & reg[rt(instr)];
}

void
CPU::or_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = reg[rs(instr)] | reg[rt(instr)];
}

void
CPU::xor_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = reg[rs(instr)] ^ reg[rt(instr)];
}

void
CPU::nor_emulate(uint32 instr, uint32 pc)
{
	reg[rd(instr)] = ~(reg[rs(instr)] | reg[rt(instr)]);
}

void
CPU::slt_emulate(uint32 instr, uint32 pc)
{
	int32 s_rs = (int32)reg[rs(instr)];
	int32 s_rt = (int32)reg[rt(instr)];
	if (s_rs < s_rt) {
		reg[rd(instr)] = 1;
	} else {
		reg[rd(instr)] = 0;
	}
}

void
CPU::sltu_emulate(uint32 instr, uint32 pc)
{
	if (reg[rs(instr)] < reg[rt(instr)]) {
		reg[rd(instr)] = 1;
	} else {
		reg[rd(instr)] = 0;
	}
}

void
CPU::bltz_emulate(uint32 instr, uint32 pc)
{
	if ((int32)reg[rs(instr)] < 0) {
		branch(instr, pc);
	}
}

void
CPU::bgez_emulate(uint32 instr, uint32 pc)
{
	if ((int32)reg[rs(instr)] >= 0) {
		branch(instr, pc);
	}
}

/* As with JAL, BLTZAL and BGEZAL cause RA to get the address of the
 * instruction two words after the current one (pc + 8).
 */
void
CPU::bltzal_emulate(uint32 instr, uint32 pc)
{
	reg[ra] = pc + 8;
	if ((int32)reg[rs(instr)] < 0) {
		branch(instr, pc);
	}
}

void
CPU::bgezal_emulate(uint32 instr, uint32 pc)
{
	reg[ra] = pc + 8;
	if ((int32)reg[rs(instr)] >= 0) {
		branch(instr, pc);
	}
}

extern void call_disassembler(uint32 pc, uint32 instr);

/* dispatching */
void
CPU::periodic(void)
{
	uint32 real_pc;
	bool cacheable, instdump = machine->opt->option("instdump")->flag,
		excmsg = machine->opt->option("excmsg")->flag;

	/* Check for a (hardware or software) interrupt. */
	if (cpzero->interrupt_pending()) {
		exception(Int);
	} else {
		/* Clear exception_pending flag if it was set by a
		 * prior instruction. */
		exception_pending = false;
	}

	/* decrement Random register */
	cpzero->adjust_random();

	/* save address of instruction responsible for exceptions which may occur */
	if (delay_state != DELAYSLOT) {
		next_epc = pc;
	}

	/* get physical address of next instruction */
	real_pc = cpzero->address_trans(pc,INSTFETCH,&cacheable, this);
	if (real_pc == 0xffffffff && exception_pending) {
		if (excmsg) {
			fprintf(stderr,
				"** PC address translation caused the exception! **\n");
		}
		return;
	}

	/* get next instruction */
	instr = mem->fetch_word(real_pc,INSTFETCH,cacheable, this);
	if (instr == 0xffffffff && exception_pending) {
		if (excmsg) {
			fprintf(stderr, "** Instruction fetch caused the exception! **\n");
		}
		return;
	}

	/* diagnostic output - display disassembly of instr */
	if (instdump) {
		fprintf(stderr,"PC=0x%08lx [%08lx]\t%08lx ",pc,real_pc,instr);
		call_disassembler(pc,instr);
	}

	/* jump to appropriate emulation function
	 * 
	 * this is really bad. how do you declare a value
	 * of type "pointer to method returning void and taking two uint32's"?
	 */
	switch(opcode(instr))
	{
		case 0: funct_emulate(instr, pc); break;
		case 1: regimm_emulate(instr, pc); break;
		case 2: j_emulate(instr, pc); break;
		case 3: jal_emulate(instr, pc); break;
		case 4: beq_emulate(instr, pc); break;
		case 5: bne_emulate(instr, pc); break;
		case 6: blez_emulate(instr, pc); break;
		case 7: bgtz_emulate(instr, pc); break;
		case 8: addi_emulate(instr, pc); break;
		case 9: addiu_emulate(instr, pc); break;
		case 10: slti_emulate(instr, pc); break;
		case 11: sltiu_emulate(instr, pc); break;
		case 12: andi_emulate(instr, pc); break;
		case 13: ori_emulate(instr, pc); break;
		case 14: xori_emulate(instr, pc); break;
		case 15: lui_emulate(instr, pc); break;
		case 16: cpzero_emulate(instr, pc); break;
		case 17: cpone_emulate(instr, pc); break;
		case 18: cptwo_emulate(instr, pc); break;
		case 19: cpthree_emulate(instr, pc); break;
		case 32: lb_emulate(instr, pc); break;
		case 33: lh_emulate(instr, pc); break;
		case 34: lwl_emulate(instr, pc); break;
		case 35: lw_emulate(instr, pc); break;
		case 36: lbu_emulate(instr, pc); break;
		case 37: lhu_emulate(instr, pc); break;
		case 38: lwr_emulate(instr, pc); break;
		case 40: sb_emulate(instr, pc); break;
		case 41: sh_emulate(instr, pc); break;
		case 42: swl_emulate(instr, pc); break;
		case 43: sw_emulate(instr, pc); break;
		case 46: swr_emulate(instr, pc); break;
		case 49: lwc1_emulate(instr, pc); break;
		case 50: lwc2_emulate(instr, pc); break;
		case 51: lwc3_emulate(instr, pc); break;
		case 57: swc1_emulate(instr, pc); break;
		case 58: swc2_emulate(instr, pc); break;
		case 59: swc3_emulate(instr, pc); break;
		default: exception(RI); break; /* reserved instruction */
	}

	/* register zero must always be zero;
	 * this instruction forces this.
	 */
	reg[0] = 0;

	/* If there is an exception pending, we return now, so that we don't
	 * clobber the exception vector.
	 */
	if (exception_pending) {
		/* Instruction at beginning of exception handler is NOT in
		 * delay slot, no matter what the last instruction was.
		 */
		delay_state = NORMAL;
		return;
	}

	/* increment PC */
	if (delay_state == DELAYING) {
		/* This instruction caused a branch to be taken.
		 * The next instruction is in the delay slot.
		 * The next instruction EPC will be PC - 4.
		 */
		delay_state = DELAYSLOT;
		pc = pc + 4;
	} else if (delay_state == DELAYSLOT) {
		/* This instruction was executed in a delay slot.
		 * The next instruction is on the other end of the branch.
		 * The next instruction EPC will be PC.
		 */
		delay_state = NORMAL;
		pc = delay_pc;
	} else if (delay_state == NORMAL) {
		/* No branch; next instruction is next word.
		 * Next instruction EPC is PC.
		 */
		pc = pc + 4;
	}
}

void
CPU::funct_emulate(uint32 instr, uint32 pc)
{
	switch(funct(instr))
	{
		case 0: sll_emulate(instr, pc); break;
		case 2: srl_emulate(instr, pc); break;
		case 3: sra_emulate(instr, pc); break;
		case 4: sllv_emulate(instr, pc); break;
		case 6: srlv_emulate(instr, pc); break;
		case 7: srav_emulate(instr, pc); break;
		case 8: jr_emulate(instr, pc); break;
		case 9: jalr_emulate(instr, pc); break;
		case 12: syscall_emulate(instr, pc); break;
		case 13: break_emulate(instr, pc); break;
		case 16: mfhi_emulate(instr, pc); break;
		case 17: mthi_emulate(instr, pc); break;
		case 18: mflo_emulate(instr, pc); break;
		case 19: mtlo_emulate(instr, pc); break;
		case 24: mult_emulate(instr, pc); break;
		case 25: multu_emulate(instr, pc); break;
		case 26: div_emulate(instr, pc); break;
		case 27: divu_emulate(instr, pc); break;
		case 32: add_emulate(instr, pc); break;
		case 33: addu_emulate(instr, pc); break;
		case 34: sub_emulate(instr, pc); break;
		case 35: subu_emulate(instr, pc); break;
		case 36: and_emulate(instr, pc); break;
		case 37: or_emulate(instr, pc); break;
		case 38: xor_emulate(instr, pc); break;
		case 39: nor_emulate(instr, pc); break;
		case 42: slt_emulate(instr, pc); break;
		case 43: sltu_emulate(instr, pc); break;
		default: exception(RI); break; /* reserved instruction */
	}
}

void
CPU::regimm_emulate(uint32 instr, uint32 pc)
{
	switch(rt(instr))
	{
		case 0: bltz_emulate(instr, pc); break;
		case 1: bgez_emulate(instr, pc); break;
		case 16: bltzal_emulate(instr, pc); break;
		case 17: bgezal_emulate(instr, pc); break;
		default: exception(RI); break; /* reserved instruction */
	}
}

/* Debug functions.
 *
 * These functions are primarily intended to support the Debug class,
 * which interfaces with GDB's remote serial protocol.
 */

/* Copy registers into an ASCII-encoded packet of hex numbers to send
 * to the remote GDB, and return the packet (allocated with malloc).
 */
char *
CPU::debug_registers_to_packet(void)
{
	char *packet = (char *) malloc(728 * sizeof(char));
	int i, r;

	/* order of regs:  (gleaned from gdb/gdb/config/mips/tm-mips.h)
	 * 
	 * cpu->reg[0]...cpu->reg[31]
	 * cpzero->reg[Status]
	 * cpu->lo
	 * cpu->hi
	 * cpzero->reg[BadVAddr]
	 * cpzero->reg[Cause]
	 * cpu->pc
     * fpu stuff: 35 zeroes (Unimplemented registers read as
     *  all bits zero.)
     */
	packet[0] = '\0';
	r = 0;
	for (i = 0; i < 32; i++) {
		Debug::packet_push_word(packet, reg[i]); r++;
	}
	uint32 sr, bad, cause;
	cpzero->read_debug_info(&sr, &bad, &cause);
	Debug::packet_push_word(packet, sr); r++;
	Debug::packet_push_word(packet, lo); r++;
	Debug::packet_push_word(packet, hi); r++;
	Debug::packet_push_word(packet, bad); r++;
	Debug::packet_push_word(packet, cause); r++;
	Debug::packet_push_word(packet, pc); r++;
	for (; r < 90; r++) { /* unimplemented regs at end */
		Debug::packet_push_word(packet, 0);
	}
	return packet;
}

/* Copy the register values in the ASCII-encoded hex PACKET received from
 * the remote GDB and store them in the appropriate registers.
 */
void
CPU::debug_packet_to_registers(char *packet)
{
	int i;

	for (i = 0; i < 32; i++) {
		reg[i] = Debug::packet_pop_word(&packet);
	}
	uint32 sr, bad, cause;
	sr = Debug::packet_pop_word(&packet);
	lo = Debug::packet_pop_word(&packet);
	hi = Debug::packet_pop_word(&packet);
	bad = Debug::packet_pop_word(&packet);
	cause = Debug::packet_pop_word(&packet);
	pc = Debug::packet_pop_word(&packet);
	cpzero->write_debug_info(sr, bad, cause);
}

/* Returns the exception code of any pending exception, or 0 if no
 * exception is pending.
 */
uint8
CPU::pending_exception(void)
{
	uint32 sr, bad, cause;

	if (! exception_pending) return 0;
	cpzero->read_debug_info(&sr, &bad, &cause);
	return ((cause & Cause_ExcCode_MASK) >> 2);
}

/* Sets the program counter register to the value given in NEWPC. */
void
CPU::debug_set_pc(uint32 newpc)
{
	pc = newpc;
}

/* Returns the current program counter register. */
uint32
CPU::debug_get_pc(void)
{
	return pc;
}

/* XXX -- these functions need to do error checking! */
int
CPU::debug_fetch_region(uint32 addr, uint32 len, char *packet,
	DeviceExc *client)
{
	uint8 byte;
	uint32 real_addr;
	bool cacheable = false;

	for (; len; addr++, len--) {
		real_addr = cpzero->address_trans(addr, DATALOAD, &cacheable, client);
		byte = mem->fetch_byte(real_addr, true, client);
		Debug::packet_push_byte(packet, byte);
	}
	return 0; /* or -1 */
}

int
CPU::debug_store_region(uint32 addr, uint32 len, char *packet,
	DeviceExc *client)
{
	uint8 byte;
	uint32 real_addr;
	bool cacheable = false;

	for (; len; addr++, len--) {
		byte = Debug::packet_pop_byte(&packet);
		real_addr = cpzero->address_trans(addr, DATALOAD, &cacheable, client);
		mem->store_byte(real_addr, byte, true, client);
	}
	return 0; /* or -1 */
}
