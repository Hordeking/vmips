#ifndef __mapper_h__
#define __mapper_h__

#include "range.h"
#include "cache.h"
#include "devicemap.h"
#include "deviceexc.h"
#include "accesstypes.h"
#include "debug.h"

class CPU;
/* avoid circular dep. cpu.h -> mapper.h -> cpu.h -> ... */

class Mapper {
private:
	Range *rangelist;
	Range *last_used_mapping;

public:
	int error;
	CPU *cpu;
	Cache *icache;
	Cache *dcache;

	Mapper();
	~Mapper();
	int setup_icache(uint32 nwords);
	int setup_dcache(uint32 nwords);
	int add_file_mapping(FILE *fp, uint32 base, int perms = MEM_READ_WRITE);
	int add_core_mapping(caddr_t p, uint32 base, uint32 len,
		int maptype = MALLOC, int perms = MEM_READ_WRITE);
	int add_device_mapping(DeviceMap *d, uint32 base);
	int delete_mapping(caddr_t p, uint32 size);
#if defined(BYTESWAPPED) || defined(TARGET_LITTLE_ENDIAN)
	static uint32 swap_word(uint32 w);
	static uint16 swap_halfword(uint16 h);
#endif
	uint32 fetch_word(uint32 addr, int32 mode, bool cacheable,
		DeviceExc *client);
	uint16 fetch_halfword(uint32 addr, bool cacheable, DeviceExc *client);
	uint8 fetch_byte(uint32 addr, bool cacheable, DeviceExc *client);
	uint32 store_word(uint32 addr, uint32 data, bool cacheable,
		DeviceExc *client);
	uint16 store_halfword(uint32 addr, uint16 data, bool cacheable,
		DeviceExc *client);
	uint8 store_byte(uint32 addr, uint8 data, bool cacheable,
		DeviceExc *client);
	void print_rangelist(void);
	int verify_rangelist(void);
	Range *find_mapping_range(uint32 p, Range *first = NULL);
	void attach(CPU *m = NULL);
	void dump_stack(FILE *f, uint32 stackphys);
};

#endif /* __mapper_h__ */
