/* Definitions to support the system control coprocessor.
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

#ifndef _CPZERO_H_
#define _CPZERO_H_

#include "tlbentry.h"

class DeviceExc;
class IntCtrl;
class CPU; /* avoid circular dependency */

#define TLB_ENTRIES 64

class CPZero
{
private:
	TLBEntry tlb[TLB_ENTRIES];
	uint32 reg[32];
	CPU *cpu;
	IntCtrl *intc;

	void mfc0_emulate(uint32 instr, uint32 pc);
	void mtc0_emulate(uint32 instr, uint32 pc);
	void bc0x_emulate(uint32 instr, uint32 pc);
	void tlbr_emulate(uint32 instr, uint32 pc);
	void tlbwi_emulate(uint32 instr, uint32 pc);
	void tlbwr_emulate(uint32 instr, uint32 pc);
	void tlbp_emulate(uint32 instr, uint32 pc);
	void rfe_emulate(uint32 instr, uint32 pc);
	void load_addr_trans_excp_info(uint32 va, uint32 vpn, TLBEntry *match);
	TLBEntry *find_matching_tlb_entry(uint32 asid, uint32 asid);
	uint32 tlb_translate(uint32 seg, uint32 vaddr, int mode,
		bool *cacheable, DeviceExc *client);

public:
	bool tlb_miss_user;
	/* Convention says that CP0's condition is TRUE if the memory write-back
	 * buffer is empty. Because memory writes are fast as far as
	 * the emulation is concerned, the write buffer is always empty for CP0.
	 */
	static const bool cpCond = true;

	CPZero(CPU *m = NULL, IntCtrl *i = NULL);
	void attach(CPU *m = NULL, IntCtrl *i = NULL);
	void reset(void);
	bool kernel_mode(void);
	uint32 address_trans(uint32 vaddr, int mode, bool *cacheable,
		DeviceExc *client);
	void enter_exception(uint32 pc, uint32 excCode, uint32 ce, bool dly);
	bool use_boot_excp_address(void);
	bool caches_isolated(void);
	bool caches_swapped(void);
	void cpzero_emulate(uint32 instr, uint32 pc);
	void dump_regs(FILE *f);
	void dump_tlb(FILE *f);
	void dump_regs_and_tlb(FILE *f);
	void adjust_random(void);
	bool interrupts_enabled(void);
	bool interrupt_pending(void);
	void read_debug_info(uint32 *status, uint32 *bad, uint32 *cause);
	void write_debug_info(uint32 status, uint32 bad, uint32 cause);
	/* TLB translate VADDR without exceptions.  Returns true if a valid
	 * TLB mapping is found, false otherwise. If VADDR has no valid mapping,
	 * PADDR is written with 0xffffffff, otherwise it is written with the
	 * translation.
	 */
	bool debug_tlb_translate(uint32 vaddr, uint32 *paddr);
};

#endif /* _CPZERO_H_ */
