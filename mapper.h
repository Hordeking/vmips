/* Definitions to support the physical memory system.
   Copyright 2001 Brian R. Gaeke.

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

#ifndef _MAPPER_H_
#define _MAPPER_H_

#include <vector>

#include "range.h"
#include "devicemap.h"
#include "deviceexc.h"
#include "accesstypes.h"
#include "debug.h"

class CPU;
/* avoid circular dep. cpu.h -> mapper.h -> cpu.h -> ... */

class Mapper {
public:
	/* We keep lists of ranges in a vector of pointers to range
	   objects. */
	typedef std::vector<Range *> Ranges;

private:
	/* A pointer to the last mapping that was successfully returned by
	   find_mapping_range. */
	Range *last_used_mapping;

	/* A list of all currently mapped ranges. */
	Ranges ranges;

	/* A pointer to the currently associated CPU object. */
	CPU *cpu;

public:
	Mapper();
	~Mapper();

	/* Add range R to the mapping. R must not overlap with any existing
	 * ranges in the mapping. Return 0 if R added sucessfully or -1 if
	 * R overlapped with an existing range. 
	 */
	int insert_into_rangelist(Range *r) throw();
	int add_file_mapping(FILE *fp, uint32 base, int perms = MEM_READ_WRITE)
		throw();
	int add_core_mapping(caddr_t p, uint32 base, uint32 len,
		range_type maptype = MALLOC, int perms = MEM_READ_WRITE) throw();
	int add_device_mapping(DeviceMap *d, uint32 base) throw();
	static uint32 swap_word(uint32 w);
	static uint16 swap_halfword(uint16 h);
	static uint32 mips_to_host_word(uint32 w);
	static uint32 host_to_mips_word(uint32 w);
	static uint16 mips_to_host_halfword(uint16 h);
	static uint16 host_to_mips_halfword(uint16 h);
	uint32 fetch_word(uint32 addr, int32 mode, bool cacheable,
		DeviceExc *client);
	uint16 fetch_halfword(uint32 addr, bool cacheable, DeviceExc *client);
	uint8 fetch_byte(uint32 addr, bool cacheable, DeviceExc *client);
	void store_word(uint32 addr, uint32 data, bool cacheable,
		DeviceExc *client);
	void store_halfword(uint32 addr, uint16 data, bool cacheable,
		DeviceExc *client);
	void store_byte(uint32 addr, uint8 data, bool cacheable,
		DeviceExc *client);
	Range *find_mapping_range(uint32 p) throw();
	void attach(CPU *m = NULL);
	void dump_stack(FILE *f, uint32 stackphys);
};

#endif /* _MAPPER_H_ */
