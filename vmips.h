#ifndef __vmips_h__
#define __vmips_h__

#include "sysinclude.h"
#include "mapper.h"
#include "cpzero.h"
#include "cpu.h"
#include "intctrl.h"
#include "memorymodule.h"
#include "options.h"
#include "testdev.h"

class SPIMConsole;

class vmips : public Periodic
{
public:
	Mapper *physmem;
	CPZero *cpzero;
	CPU *cpu;
	IntCtrl *intc;
	Options *opt;
	MemoryModule *memmod[NUM_MEMORY_MODULES];
	bool host_bigendian;
private:
	bool dumpcpu, dumpcp0;
#if TTY
	bool usetty;
	SPIMConsole *console;
#endif
	Debug *dbgr;
	int32 test_code;
	uint32 num_instrs;
	bool halted;

public:
	vmips();
	void randomize(void);
	void halt(void);
	uint32 auto_size_rom(FILE *rom);
	bool unique(int order[NUM_MEMORY_MODULES], int max);
	int unique_order_deallocate(void);
	void access_check(uint32 begin, uint32 end);
	int rangelist_selftest(void);
	int host_endian_selftest(void);
	int run_self_tests(void);
	void time_diff(struct timeval *d, struct timeval *a, struct timeval *b);
	void periodic(void);
	int run(int argc, char **argv);
};

extern vmips *machine;

#endif /* __vmips_h__ */
