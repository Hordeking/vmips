#ifndef __cacheline_h__
#define __cacheline_h__

#include "sysinclude.h"

class CacheLine {
private:
	uint32 high;
	uint32 low;

	void recalc_tagp(void);
	void recalc_datap(void);

public:
	static uint8 parity(uint16 nbits, uint32 data);
	void set_valid_and_pfn(bool v, uint32 pfn);
	void set_valid(bool v);
	void set_pfn(uint32 pfn);
	void set_data(uint32 data);
	void set_line(bool v, uint32 pfn, uint32 data);
	bool valid(void);
	uint32 pfn(void);
	uint32 data(void);
	uint8 tagp(void);
	uint8 datap(void);
	uint32 get_high(void);
	uint32 get_low(void);
};

#endif /* __cacheline_h__ */
