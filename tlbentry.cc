/* Wrapper class for Translation-Lookaside Buffer (TLB) entries.
   Copyright 2001, 2003 Brian R. Gaeke.

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
