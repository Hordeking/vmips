#ifndef __cpu_h__
#define __cpu_h__

#include "sysinclude.h"
#include "periodic.h"
#include "deviceexc.h"
#include "mapper.h"
#include "cpzero.h"
#include "debug.h"

class vmips;
class Cache;

/* Delay states -- see periodic() for details. */
#define NORMAL 0
#define DELAYING 1
#define DELAYSLOT 2

/* Exception priority information -- see exception_priority(). */
struct excPriority {
    int priority;
	int excCode;
	int mode;
};

class CPU : public Periodic, public DeviceExc {
	friend class CPZero;

private:
	uint32 pc;
	uint32 reg[32]; /* general purpose registers */
	uint32 instr;
	uint32 hi;
	uint32 lo;
	Mapper *mem;
	CPZero *cpzero;
	vmips *machine;
	int delay_state;
	uint32 delay_pc;
	uint32 next_epc;
	bool exception_pending;

	int exception_priority(uint16 excCode, int mode);
	uint32 calc_jump_target(uint32 instr, uint32 pc);
	void branch(uint32 instr, uint32 pc);
	void mult64(uint32 *hi, uint32 *lo, uint32 n, uint32 m);
	void mult64s(uint32 *hi, uint32 *lo, int32 n, int32 m);

	void funct_emulate(uint32 instr, uint32 pc);
	void regimm_emulate(uint32 instr, uint32 pc);
	void j_emulate(uint32 instr, uint32 pc);
	void jal_emulate(uint32 instr, uint32 pc);
	void beq_emulate(uint32 instr, uint32 pc);
	void bne_emulate(uint32 instr, uint32 pc);
	void blez_emulate(uint32 instr, uint32 pc);
	void bgtz_emulate(uint32 instr, uint32 pc);
	void addi_emulate(uint32 instr, uint32 pc);
	void addiu_emulate(uint32 instr, uint32 pc);
	void slti_emulate(uint32 instr, uint32 pc);
	void sltiu_emulate(uint32 instr, uint32 pc);
	void andi_emulate(uint32 instr, uint32 pc);
	void ori_emulate(uint32 instr, uint32 pc);
	void xori_emulate(uint32 instr, uint32 pc);
	void lui_emulate(uint32 instr, uint32 pc);
	void cpzero_emulate(uint32 instr, uint32 pc);
	void cpone_emulate(uint32 instr, uint32 pc);
	void cptwo_emulate(uint32 instr, uint32 pc);
	void cpthree_emulate(uint32 instr, uint32 pc);
	void lb_emulate(uint32 instr, uint32 pc);
	void lh_emulate(uint32 instr, uint32 pc);
	void lwl_emulate(uint32 instr, uint32 pc);
	void lw_emulate(uint32 instr, uint32 pc);
	void lbu_emulate(uint32 instr, uint32 pc);
	void lhu_emulate(uint32 instr, uint32 pc);
	void lwr_emulate(uint32 instr, uint32 pc);
	void sb_emulate(uint32 instr, uint32 pc);
	void sh_emulate(uint32 instr, uint32 pc);
	void swl_emulate(uint32 instr, uint32 pc);
	void sw_emulate(uint32 instr, uint32 pc);
	void swr_emulate(uint32 instr, uint32 pc);
	void lwc1_emulate(uint32 instr, uint32 pc);
	void lwc2_emulate(uint32 instr, uint32 pc);
	void lwc3_emulate(uint32 instr, uint32 pc);
	void swc1_emulate(uint32 instr, uint32 pc);
	void swc2_emulate(uint32 instr, uint32 pc);
	void swc3_emulate(uint32 instr, uint32 pc);
	void sll_emulate(uint32 instr, uint32 pc);
	void srl_emulate(uint32 instr, uint32 pc);
	void sra_emulate(uint32 instr, uint32 pc);
	void sllv_emulate(uint32 instr, uint32 pc);
	void srlv_emulate(uint32 instr, uint32 pc);
	void srav_emulate(uint32 instr, uint32 pc);
	void jr_emulate(uint32 instr, uint32 pc);
	void jalr_emulate(uint32 instr, uint32 pc);
	void syscall_emulate(uint32 instr, uint32 pc);
	void break_emulate(uint32 instr, uint32 pc);
	void mfhi_emulate(uint32 instr, uint32 pc);
	void mthi_emulate(uint32 instr, uint32 pc);
	void mflo_emulate(uint32 instr, uint32 pc);
	void mtlo_emulate(uint32 instr, uint32 pc);
	void mult_emulate(uint32 instr, uint32 pc);
	void multu_emulate(uint32 instr, uint32 pc);
	void div_emulate(uint32 instr, uint32 pc);
	void divu_emulate(uint32 instr, uint32 pc);
	void add_emulate(uint32 instr, uint32 pc);
	void addu_emulate(uint32 instr, uint32 pc);
	void sub_emulate(uint32 instr, uint32 pc);
	void subu_emulate(uint32 instr, uint32 pc);
	void and_emulate(uint32 instr, uint32 pc);
	void or_emulate(uint32 instr, uint32 pc);
	void xor_emulate(uint32 instr, uint32 pc);
	void nor_emulate(uint32 instr, uint32 pc);
	void slt_emulate(uint32 instr, uint32 pc);
	void sltu_emulate(uint32 instr, uint32 pc);
	void bltz_emulate(uint32 instr, uint32 pc);
	void bgez_emulate(uint32 instr, uint32 pc);
	void bltzal_emulate(uint32 instr, uint32 pc);
	void bgezal_emulate(uint32 instr, uint32 pc);

public:
	uint16 opcode(const uint32 instr) const;
	uint16 rs(const uint32 instr) const;
	uint16 rt(const uint32 instr) const;
	uint16 rd(const uint32 instr) const;
	uint16 immed(const uint32 instr) const;
	uint16 shamt(const uint32 instr) const;
	uint16 funct(const uint32 instr) const;
	uint32 jumptarg(const uint32 instr) const;
	int16 s_immed(const uint32 instr) const;

	CPU(vmips *mch = NULL, Mapper *m = NULL, CPZero *cp0 = NULL);
	void attach(vmips *mch = NULL, Mapper *m = NULL, CPZero *cp0 = NULL);
	void dump_regs(FILE *f);
	void dump_stack(FILE *f);
	void dump_regs_and_stack(FILE *f);
	void periodic(void);
	char *const strexccode(const uint16 excCode);
	char *const strdelaystate(const int state);
	char *const strmemmode(const int memmode);
	void exception(uint16 excCode, int mode = ANY, int coprocno = -1);
	void reset(void);

	/* Debug functions. */
	char *debug_registers_to_packet(void);
	void debug_packet_to_registers(char *packet);
	uint8 pending_exception(void);
	uint32 debug_get_pc(void);
	void debug_set_pc(uint32 newpc);
	int debug_fetch_region(uint32 addr, uint32 len, char *packet,
		DeviceExc *client);
	int debug_store_region(uint32 addr, uint32 len, char *packet,
		DeviceExc *client);
};

#endif /* __cpu_h__ */
