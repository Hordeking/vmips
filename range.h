/* Definitions to support mapping ranges.
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

#ifndef _RANGE_H_
#define _RANGE_H_

class CPU;
class DeviceExc;

/* Types of mappings. In the future this can go away and be
 * replaced by subclasses of Range.
 */
typedef enum range_type {
	MALLOC = 1,	/* malloc()'d memory */
	MMAP = 2,	/* mmap()'d memory */
	DEVICE = 3,	/* A device is in this range. See devicemap.h */
	PROXY = 4,	/* A ProxyRange. Points to another Range object */
} range_type;

/* Base class for managing a range of mapped memory. Memory-mapped
 * devices (class DeviceMap) derive from this, as do ProxyRanges.
 */
class Range { 
protected:
	uint32 base;		/* first physical address represented */
	uint32 extent;		/* number of memory words provided */
	caddr_t address;	/* host machine pointer to start of memory */
	range_type type;	/* See above for "Mapping types" */
	int perms;		/* MEM_READ, MEM_WRITE, ... in accesstypes.h */

public:
	Range(uint32 b, uint32 e, caddr_t a, range_type t, int p) throw();
	virtual ~Range();
	
	virtual bool incorporates(uint32 addr) throw();
	virtual bool overlaps(Range *r) throw();

	virtual uint32 getBase() throw();
	virtual uint32 getExtent() throw();
	virtual caddr_t getAddress() throw();
	virtual int32 getType() throw();
	virtual int getPerms() throw();

	virtual void setPerms(int newPerms) throw();
	virtual bool canRead(uint32 offset) throw();
	virtual bool canWrite(uint32 offset) throw();

	virtual uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	virtual uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	virtual uint8 fetch_byte(uint32 offset, DeviceExc *client);
	virtual void store_word(uint32 offset, uint32 data, DeviceExc *client);
	virtual void store_halfword(uint32 offset, uint16 data,
		DeviceExc *client);
	virtual void store_byte(uint32 offset, uint8 data, DeviceExc *client);
};

class ProxyRange: public Range
{
private:
	Range *realRange;
public:
	ProxyRange(Range *r, uint32 b);

	virtual Range *getRealRange(void) throw();

	virtual uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	virtual uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	virtual uint8 fetch_byte(uint32 offset, DeviceExc *client);
	virtual void store_word(uint32 offset, uint32 data, DeviceExc *client);
	virtual void store_halfword(uint32 offset, uint16 data,
		DeviceExc *client);
	virtual void store_byte(uint32 offset, uint8 data, DeviceExc *client);

	virtual int getPerms(void) throw();
	virtual void setPerms(int newPerms) throw();
	virtual bool canRead(uint32 offset) throw();
	virtual bool canWrite(uint32 offset) throw();
};

#endif /* _RANGE_H_ */
