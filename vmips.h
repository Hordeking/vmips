#ifndef __vmips_h__
#define __vmips_h__

#include "sysinclude.h"
#include "mapper.h"
#include "cpzero.h"
#include "cpu.h"
#include "intctrl.h"
#include "memorymodule.h"
#include "options.h"
#include "clockdev.h"

class SPIMConsole;

long timediff(struct timeval *after, struct timeval *before);

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
	bool halted;
private:
	bool dumpcpu, dumpcp0;
#if TTY
	bool usetty;
	SPIMConsole *console;
#endif
	Debug *dbgr;
	ClockDev *clockdev;
	int32 test_code;
	uint32 num_instrs;
	/* Cached versions of options: */
	bool opt_bootmsg;

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
	void periodic(void);
	int run(int argc, char **argv);
};

extern vmips *machine;

#endif /* __vmips_h__ */
