#ifndef __cache_h__
#define __cache_h__

#include "sysinclude.h"

class CacheLine;
class CPU;
class CPZero;
class Mapper;

class Cache {
private:
	CPU *cpu;
	CPZero *cpzero;
	Mapper *mem;
	CacheLine **c;
	uint32 num_words;
	uint8 cache_refill;
	uint32 cache_refill_mask;
	uint32 cache_tag_mask;
	uint32 pfn_mask;

public:
	Cache();
	~Cache();
	void attach(CPU *c = NULL, Mapper *m = NULL, CPZero *z = NULL);
	int init(uint32 nwords, uint8 crefill);
	void dealloc_cache(void);
	int alloc_cache(void);
	int read_cache_word(uint32 addr, int mode, uint32 *word);
	int read_cache_halfword(uint32 addr, int mode, uint16 *halfword);
	int read_cache_byte(uint32 addr, int mode, uint8 *byte);
	void refill(uint32 addr, int mode);
	void refill_one_word(uint32 addr, int mode);
};

#endif /* __cache_h__ */
