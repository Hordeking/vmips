/* R3000 system control coprocessor emulation ("coprocessor zero").
   Copyright 2001 Brian R. Gaeke.

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

/* Code to implement MIPS coprocessor zero (the "system control
 * coprocessor"), which provides for address translation and
 * exception handling.
 */

#include "cpzero.h"
#include "mapper.h"
#include "excnames.h"
#include "cpu.h"
#include "cpzeroreg.h"
#include "intctrl.h"
#include "deviceexc.h"

static uint32 read_masks[] = {
	Index_MASK, Random_MASK, EntryLo_MASK, 0, Context_MASK,
	PageMask_MASK, Wired_MASK, Error_MASK, BadVAddr_MASK, Count_MASK,
	EntryHi_MASK, Compare_MASK, Status_MASK, Cause_MASK, EPC_MASK,
	PRId_MASK, Config_MASK, LLAddr_MASK, WatchLo_MASK, WatchHi_MASK,
	0, 0, 0, 0, 0, 0, ECC_MASK, CacheErr_MASK, TagLo_MASK, TagHi_MASK,
	ErrorEPC_MASK, 0
};

static uint32 write_masks[] = {
	Index_MASK, 0, EntryLo_MASK, 0, Context_MASK & ~Context_BadVPN_MASK,
	PageMask_MASK, Wired_MASK, Error_MASK, 0, Count_MASK,
	EntryHi_MASK, Compare_MASK, Status_MASK,
	Cause_MASK & ~Cause_IP_Ext_MASK, 0, 0, Config_MASK, LLAddr_MASK,
	WatchLo_MASK, WatchHi_MASK, 0, 0, 0, 0, 0, 0, ECC_MASK,
	CacheErr_MASK, TagLo_MASK, TagHi_MASK, ErrorEPC_MASK, 0
};

/* Initialization */
CPZero::CPZero(CPU *m, IntCtrl *i)
{
	attach(m, i);
}

/* Reset (warm or cold) */
void
CPZero::reset(void)
{
	int r;
	for (r = 0; r < 16; r++) {
#ifdef INTENTIONAL_CONFUSION
		reg[r] = random() & (read_masks[r] | write_masks[r]);
#endif /* INTENTIONAL_CONFUSION */
	}
	/* Reset Random register to upper bound (8<=Random<=63) */
	reg[Random] = Random_UPPER_BOUND << 8;
	/* Reset Status register: clear KUc, IEc, SwC (i.e., caches are not
	 * switched), TS (TLB shutdown has not occurred), and set BEV (Bootstrap
	 * exception vectors ARE in effect).
	 */
	reg[Status] = (reg[Status] | Status_DS_BEV_MASK) &
		~(Status_KUc_MASK | Status_IEc_MASK | Status_DS_SwC_MASK |
		  Status_DS_TS_MASK);
}

/* We have been presented with a CPU and/or interrupt controller to
 * associate with. */
void
CPZero::attach(CPU *m, IntCtrl *i)
{
	if (m) cpu = m;
	if (i) intc = i;
}

/* Yow!! Are we in KERNEL MODE yet?? ...Read the Status register. */
bool
CPZero::kernel_mode(void)
{
	return !(reg[Status] & Status_KUc_MASK);
}

void
CPZero::dump_regs(FILE *f)
{
	int x;

	fprintf(f, "CP0 Dump Registers: [       ");
	for (x = 0; x < 16; x++) {
		fprintf(f," R%02d=%08x ",x,reg[x]);
		if (x % 4 == 1) {
			fputc('\n',f);
		}
	}
	fprintf(f, "]\n");
}

void
CPZero::dump_tlb(FILE *f)
{
	int x;

	fprintf(f, "Dump TLB: [\n");
	for (x=0;x<64;x++) {
		TLBEntry e = tlb[x];
		fprintf(f,"Entry %02d: (%08x%08x) V=%05x A=%02x P=%05x %c%c%c%c\n", x,
			e.entryHi, e.entryLo, e.vpn()>>12, e.asid()>>6, e.pfn()>>12,
			e.noncacheable()?'N':'n', e.dirty()?'D':'d',
			e.valid()?'V':'v', e.global()?'G':'g');
	}
	fprintf(f, "]\n");
}

void
CPZero::dump_regs_and_tlb(FILE *f)
{
	dump_regs(f);
	dump_tlb(f);
}

/* Request for address translation (possibly using the TLB). */
uint32
CPZero::address_trans(uint32 vaddr, int mode, bool *cacheable,
	DeviceExc *client)
{
	if (kernel_mode()) {
		switch(vaddr & KSEG_SELECT_MASK) {
			case KSEG0:
				*cacheable = true;
				return vaddr - KSEG0_CONST_TRANSLATION;
				break;
			case KSEG1:
				*cacheable = false;
				return vaddr - KSEG1_CONST_TRANSLATION;
				break;
			case KSEG2:
			case KSEG2_top:
				return tlb_translate(KSEG2, vaddr, mode, cacheable, client);
				break;
			default: /* KUSEG */
				return tlb_translate(KUSEG, vaddr, mode, cacheable, client);
				break;
		}
	} else /* user mode */ {
		if (vaddr & KERNEL_SPACE_MASK) {
			/* Can't go there. */
			client->exception(mode == DATASTORE ? AdES : AdEL, mode);
			return 0xffffffff;
		} else /* user space address */ {
			return tlb_translate(KUSEG, vaddr, mode, cacheable, client);
		}
	}
}

void
CPZero::load_addr_trans_excp_info(uint32 va, uint32 vpn, TLBEntry *match)
{
	reg[BadVAddr] = va;
	reg[Context] = (reg[Context] & ~Context_BadVPN_MASK) | (vpn >> 6);
	if (match)
		reg[EntryHi] = match->entryHi & write_masks[EntryHi];
	else
		reg[EntryHi] = va & write_masks[EntryHi];
}

TLBEntry *
CPZero::find_matching_tlb_entry(uint32 vpn, uint32 asid)
{
	uint16 x;
	TLBEntry *match = NULL;
	for (x = 0; x < TLB_ENTRIES; x++) {
		if (tlb[x].vpn() == vpn) {
			if (tlb[x].global() || tlb[x].asid() == asid) {
				match = &tlb[x];
				break;
			}
		}
	}
	return match;
}

uint32
CPZero::tlb_translate(uint32 seg, uint32 vaddr, int mode, bool *cacheable,
	DeviceExc *client)
{
	TLBEntry *match = NULL;
	uint32 asid = reg[EntryHi] & EntryHi_ASID_MASK, vpn = vaddr & EntryHi_VPN_MASK;

	match = find_matching_tlb_entry(vpn, asid);
	tlb_miss_user = false;
	if (match) {
		if (!match->valid()) {
			/* TLB Invalid exception */
			load_addr_trans_excp_info(vaddr,vpn,match);
			client->exception(mode == DATASTORE ? TLBS : TLBL, mode);
			return 0xffffffff;
		} else if (match->dirty() && mode == DATASTORE) {
			/* TLB Mod exception */
			load_addr_trans_excp_info(vaddr,vpn,match);
			client->exception(Mod, DATASTORE);
			return 0xffffffff;
		} else {
			/* We have a matching TLB entry which is valid. */
			*cacheable = !match->noncacheable();
			return match->pfn() | (vaddr & ~EntryHi_VPN_MASK);
		}
	} else /* No matching TLB entry */ {
		if (seg == KUSEG) {
			/* User TLB Miss exception */
			load_addr_trans_excp_info(vaddr,vpn,match);
			tlb_miss_user = true;
			client->exception(mode == DATASTORE ? TLBS : TLBL, mode);
			return 0xffffffff;
		} else {
			/* TLB Miss exception */
			load_addr_trans_excp_info(vaddr,vpn,match);
			client->exception(mode == DATASTORE ? TLBS : TLBL, mode);
			return 0xffffffff;
		}
	}
}

void
CPZero::mfc0_emulate(uint32 instr, uint32 pc)
{
	cpu->reg[cpu->rt(instr)] =
		(reg[cpu->rd(instr)] & read_masks[cpu->rd(instr)]);
}

void
CPZero::mtc0_emulate(uint32 instr, uint32 pc)
{
	int regno = cpu->rd(instr);
	uint32 new_data = cpu->reg[cpu->rt(instr)];

	/* DO AS I SAY, NOT AS I DO...
	 * This preserves the bits which are readable but not writable,
     * and writes the bits which are writable with new data, thus
	 * making it suitable for mtc0-type operations.
	 * If you want to write all the bits which are _connected_,
	 * use reg[regno] = new_data & write_masks[regno]; .
     */
	reg[regno] = (reg[regno] & (read_masks[regno] & ~write_masks[regno]))
		| (new_data & write_masks[regno]);
}

void
CPZero::bc0x_emulate(uint32 instr, uint32 pc)
{
	uint16 condition = cpu->rt(instr);
	switch(condition) {
		case 0: /* bc0f */ if (! cpCond) { cpu->branch(instr, pc); } break;
		case 1: /* bc0t */ if (cpCond) { cpu->branch(instr, pc); } break;
		case 2: /* bc0fl - not valid, but not reserved(A-17, H&K) - no-op. */ ;
		case 3: /* bc0tl - not valid, but not reserved(A-21, H&K) - no-op. */ ;
		default: cpu->exception(RI); break; /* reserved */
	}
}

void
CPZero::tlbr_emulate(uint32 instr, uint32 pc)
{
	reg[EntryHi] = (tlb[(reg[Index] & Index_Index_MASK) >> 8].entryHi) &
		write_masks[EntryHi];
	reg[EntryLo] = (tlb[(reg[Index] & Index_Index_MASK) >> 8].entryLo) &
		write_masks[EntryHi];
}

void
CPZero::tlbwi_emulate(uint32 instr, uint32 pc)
{
	tlb[(reg[Index] & Index_Index_MASK) >> 8].entryHi =
		reg[EntryHi] & read_masks[EntryHi];
	tlb[(reg[Index] & Index_Index_MASK) >> 8].entryLo =
		reg[EntryLo] & read_masks[EntryLo];
}

void
CPZero::tlbwr_emulate(uint32 instr, uint32 pc)
{
	tlb[(reg[Random] & Random_Random_MASK) >> 8].entryHi = 
		reg[EntryHi] & read_masks[EntryHi];
	tlb[(reg[Random] & Random_Random_MASK) >> 8].entryLo =
		reg[EntryLo] & read_masks[EntryLo];
}

void
CPZero::tlbp_emulate(uint32 instr, uint32 pc)
{
	int x;
	uint32 vpn, asid;

	reg[Index] = (1 << 31);
	vpn = reg[EntryHi] & EntryHi_VPN_MASK;
	asid = reg[EntryHi] & EntryHi_ASID_MASK;
	for (x = 0; x < TLB_ENTRIES; x++) {
		if (tlb[x].vpn() == vpn) {
			if (tlb[x].global() || tlb[x].asid() == asid) {
				reg[Index] = (x << 8);
				return;
			}
		}
	}
}

void
CPZero::rfe_emulate(uint32 instr, uint32 pc)
{
	reg[Status] = (reg[Status] & 0xfffffff0) | ((reg[Status] >> 2) & 0x0f);
}

void
CPZero::cpzero_emulate(uint32 instr, uint32 pc)
{
	if (cpu->rs(instr) > 15) {
		switch(cpu->funct(instr)) {
			case 1: tlbr_emulate(instr, pc); break;
			case 2: tlbwi_emulate(instr, pc); break;
			case 6: tlbwr_emulate(instr, pc); break;
			case 8: tlbp_emulate(instr, pc); break;
			case 16: rfe_emulate(instr, pc); break;
			default: cpu->exception(RI,ANY,0); break;
		}
	} else {
		switch(cpu->rs(instr)) {
			case 0: mfc0_emulate(instr,pc); break;
			case 2: cpu->exception(RI,ANY,0); break; /* cfc0 - reserved */
			case 4: mtc0_emulate(instr,pc); break;
			case 6: cpu->exception(RI,ANY,0); break; /* ctc0 - reserved */
			case 8: bc0x_emulate(instr,pc); break;
			default: cpu->exception(RI,ANY,0); break;
		}
	}
}

void
CPZero::adjust_random(void)
{
/* For initial == 12, lower bound == 8, upper bound == 63, the
 * sequence looks like this:
 *  12 11 10  9  8 63 62 61 60 ... 12 11 10  9  8 63 ... (x)
 *  51 52 53 54 55  0  1  2  3 ... 51 52 53 54 55  0 ... (63 - x)
 */
	int32 r = (int32) (reg[Random] >> 8);
    r = -(((Random_UPPER_BOUND - r + 1) %
		(Random_UPPER_BOUND - Random_LOWER_BOUND + 1)) -
			Random_UPPER_BOUND);
	reg[Random] = (uint32) (r << 8);
}

void
CPZero::enter_exception(uint32 pc, uint32 excCode, uint32 ce, bool dly)
{
	/* Save exception PC in EPC. */
	reg[EPC] = pc;
	/* Disable interrupts and enter Kernel mode. */
	reg[Status] = (reg[Status] & ~Status_KU_IE_MASK) |
		((reg[Status] & Status_KU_IE_MASK) << 2);
	/* Clear Cause register BD, CE, and ExcCode fields. */
	reg[Cause] &= ~(Cause_BD_MASK|Cause_CE_MASK|Cause_ExcCode_MASK);
	/* Set Cause register BD, CE, and ExcCode fields w/ exception info. */
	reg[Cause] |= (dly << 31) | (ce << 28) | (excCode << 2);
}

bool
CPZero::use_boot_excp_address(void)
{
	return (reg[Status] & Status_DS_BEV_MASK);
}

bool
CPZero::caches_isolated(void)
{
	return (reg[Status] & Status_DS_IsC_MASK);
}

bool
CPZero::caches_swapped(void)
{
	return (reg[Status] & Status_DS_SwC_MASK);
}

bool
CPZero::interrupts_enabled(void)
{
	return (reg[Status] & Status_IEc_MASK);
}

bool
CPZero::interrupt_pending(void)
{
	uint32 HwIP = 0, IP = 0;

	if (! interrupts_enabled())
		return false;	/* Can't very well argue with IEc == 0... */
	if (intc != NULL) {
		HwIP = intc->calculateIP();	/* Check for a hardware interrupt. */
	}
	IP = (reg[Cause] & Cause_IP_SW_MASK) | HwIP;
	/* Mask IP with the interrupt mask, and return true if nonzero: */
	return ((IP & (reg[Status] & Status_IM_MASK)) != 0);
}

void
CPZero::read_debug_info(uint32 *status, uint32 *bad, uint32 *cause)
{
	*status = reg[Status];
	*bad = reg[BadVAddr];
	*cause = reg[Cause];
}

void
CPZero::write_debug_info(uint32 status, uint32 bad, uint32 cause)
{
	reg[Status] = status;
	reg[BadVAddr] = bad;
	reg[Cause] = cause;
}

/* TLB translate VADDR without exceptions.  Returns true if a valid
 * TLB mapping is found, false otherwise. If VADDR has no valid mapping,
 * PADDR is written with 0xffffffff, otherwise it is written with the
 * translation.
 */
bool
CPZero::debug_tlb_translate(uint32 vaddr, uint32 *paddr)
{
	TLBEntry *match = NULL;
	uint32 asid = reg[EntryHi] & EntryHi_ASID_MASK, vpn = vaddr & EntryHi_VPN_MASK;
	bool rv;

	if ((!kernel_mode()) && (vaddr & KERNEL_SPACE_MASK)) {
		*paddr = 0xffffffff;
		rv = false;
	} else if (kernel_mode() && (vaddr & KSEG_SELECT_MASK) == KSEG0) {
		*paddr = vaddr - KSEG0_CONST_TRANSLATION;
		rv = true;
	} else if (kernel_mode() && (vaddr & KSEG_SELECT_MASK) == KSEG1) {
		*paddr = vaddr - KSEG1_CONST_TRANSLATION;
		rv = true;
	} else /* KUSEG */ {
		match = find_matching_tlb_entry(vpn, asid);
		if (!match || !match->valid()) {
			*paddr = 0xffffffff;
			rv = false;
		} else {
			*paddr = match->pfn() | (vaddr & ~EntryHi_VPN_MASK);
			rv = true;
		}
	}
	return rv;
}
