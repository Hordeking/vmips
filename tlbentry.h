#ifndef __tlbentry_h__
#define __tlbentry_h__

#include "sysinclude.h"

class TLBEntry {
public:
	uint32 entryHi;
	uint32 entryLo;
	TLBEntry();
	uint32 vpn(void);
	uint16 asid(void);
	uint32 pfn(void);
	bool noncacheable(void);
	bool dirty(void);
	bool valid(void);
	bool global(void);
};

#endif /* __tlbentry_h__ */
