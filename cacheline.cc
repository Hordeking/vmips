#include "sysinclude.h"
#include "cacheline.h"

uint8
CacheLine::parity(uint16 nbits, uint32 data)
{
	uint32 c = 0;
	while (data != 0)
	{
		c += (data & 0x01);
		data >>= 1;
	}
	return c % nbits;
}

void
CacheLine::recalc_tagp(void)
{
	uint8 new_tagp;

	new_tagp = parity(3, (high >> 4));
	high |= (new_tagp << 25);
}

void
CacheLine::recalc_datap(void)
{
	uint8 new_datap;

	new_datap = parity(4, low);
	high = (high & 0xfffffff0) | new_datap;
}

void
CacheLine::set_valid_and_pfn(bool new_v, uint32 new_pfn)
{
	high = (high & 0x00ffffff) | (new_v << 24 & 0x01000000) |
		(new_pfn << 4 & 0x00fffff0);
	recalc_tagp();
}

void
CacheLine::set_valid(bool new_v)
{
	high = (high & 0x0000000f) | (new_v << 24 & 0x01000000);
	recalc_tagp();
}

/* 63-60 blank
 * 59-57 TagP & 56 V
 * 55-52 51-48 47-44 43-40 39-36 PFN
 * 35-32 DataP
 * 31-0 Data
 */
void
CacheLine::set_pfn(uint32 new_pfn)
{
	high = (high & 0x0100000f) | (new_pfn << 4 & 0x00fffff0);
	recalc_tagp();
}

void
CacheLine::set_data(uint32 new_data)
{
	low = new_data;
	recalc_datap();
}

void
CacheLine::set_line(bool new_v, uint32 new_pfn, uint32 new_data)
{
	set_valid_and_pfn(new_v, new_pfn);
	set_data(new_data);
}

bool
CacheLine::valid(void)
{
	return ((high >> 24) & 0x01);
}

uint32
CacheLine::pfn(void)
{
	return ((high >> 4) & 0x0fffff);
}

uint32
CacheLine::data(void)
{
	return (low);
}

uint8
CacheLine::tagp(void)
{
	return ((high >> 25) & 0x07);
}


uint8
CacheLine::datap(void)
{
	return (high & 0x0f);
}

uint32
CacheLine::get_high(void)
{
	return high;
}

uint32
CacheLine::get_low(void)
{
	return low;
}
