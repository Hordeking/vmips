/* Code to implement a TLB entry for CP0.
 * 
 * The TLB entry is 64 bits wide and contains 7 fields, which may be
 * accessed using the accessor functions implemented here; they are
 * merely a convenience, as the entryHi and entryLo fields are public.
 * In practice, CP0 will do the constructing of TLB entries.
 *
 * (XXX Is this true? Why not just have a constructor w/ def. values?)
 */

#include "tlbentry.h"
#include "cpzeroreg.h"

TLBEntry::TLBEntry()
{
#ifdef INTENTIONAL_CONFUSION
	entryHi = random();
    entryLo = random();
#endif
}

uint32
TLBEntry::vpn(void)
{
	/* return (entryHi >> 12) & 0x0fffff; */
	return (entryHi & EntryHi_VPN_MASK);
}

uint16
TLBEntry::asid(void)
{
	/* return (entryHi >> 5) & 0x02f; */
	return (entryHi & EntryHi_ASID_MASK);
}

uint32
TLBEntry::pfn(void)
{
	/* return (entryLo >> 12) & 0x0fffff; */
	return (entryLo & EntryLo_PFN_MASK);
}

bool
TLBEntry::noncacheable(void)
{
	return (entryLo >> 11) & 0x01;
}

bool
TLBEntry::dirty(void)
{
	return (entryLo >> 10) & 0x01;
}

bool
TLBEntry::valid(void)
{
	return (entryLo >> 9) & 0x01;
}

bool
TLBEntry::global(void)
{
	return (entryLo >> 8) & 0x01;
}
