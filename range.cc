/* Mapping ranges, the building blocks of the physical memory system.
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

#include "sysinclude.h"
#include "range.h"
#include "accesstypes.h"
#include "error.h"

uint32
Range::getBase(void) throw()
{
	return base;
}

uint32
Range::getExtent(void) throw()
{
	return extent;
}

caddr_t
Range::getAddress(void) throw()
{
	return address;
}

int32
Range::getType(void) throw()
{
	return type;
}

int
Range::getPerms(void) throw()
{
	return perms;
}

void
Range::setPerms(int newPerms) throw()
{
	perms = newPerms;
}

bool
Range::canRead(uint32 offset) throw()
{
	return (perms & MEM_READ);
}

bool
Range::canWrite(uint32 offset) throw()
{
	return (perms & MEM_WRITE);
}

/* Initialization: set up member data variables. */
Range::Range(uint32 b, uint32 e, caddr_t a, range_type t, int p) throw()
	: base(b), extent(e), address(a), type(t), perms(p)
{
}

/* Destruction: Deallocate (i.e., munmap(2) or free(3)) the block
 * we are mapping, as well.
 */
Range::~Range()
{
	switch(type) {
		case MALLOC:
			free(address);
			break;
		case MMAP:
			munmap(address,extent);
			break;
		case DEVICE:
			break;
		default:
			fatal_error("cannot delete unknown type mapping");
			break;
	}
}

/* Returns true if ADDR is mapped by this Range object; false otherwise. */
bool
Range::incorporates(uint32 addr) throw()
{
	return (addr >= getBase()) && (addr < (getBase() + getExtent()));
}

bool
Range::overlaps(Range *r) throw()
{
	assert(r);
	
	uint32 end = getBase() + getExtent();
	uint32 r_end = r->getBase() + r->getExtent();

	/* Key: --- = this, +++ = r, *** = overlap */
	/* [---[****]+++++++] */
	if (getBase() <= r->getBase() && end >= r->getBase())
		return true;
	/* [+++++[***]------] */
	else if (r->getBase() <= getBase() && r_end >= getBase())
		return true;
	/* [+++++[****]+++++] */
	else if (getBase() >= r->getBase() && end <= r_end)
		return true;
	/* [---[********]---] */
	else if (r->getBase() >= getBase() && r_end <= end)
		return true;

	/* If we got here, we've determined the ranges don't overlap. */
	return false;
}

uint32
Range::fetch_word(uint32 offset, int mode, DeviceExc *client)
{
	return ((uint32 *)address)[offset / 4];
}

uint16
Range::fetch_halfword(uint32 offset, DeviceExc *client)
{
	return ((uint16 *)address)[offset / 2];
}

uint8
Range::fetch_byte(uint32 offset, DeviceExc *client)
{
	return ((uint8 *)address)[offset];
}

void
Range::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
	uint32 *werd;
	/* calculate address */
	werd = ((uint32 *) address) + (offset / 4);
	/* store word */
	*werd = data;
}

void
Range::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	uint16 *halfwerd;
	/* calculate address */
	halfwerd = ((uint16 *) address) + (offset / 2);
	/* store halfword */
	*halfwerd = data;
}

void
Range::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	uint8 *byte;
	byte = ((uint8 *) address) + offset;
	/* store halfword */
	*byte = data;
}

/* Class ProxyRange implements a system of proxy Range objects, whereby
 * one range can map the same data as another, but with a different physical
 * address in memory. */

ProxyRange::ProxyRange(Range *r, uint32 b) :
	Range(b, r->getExtent(), r->getAddress(), PROXY, r->getPerms()),
	realRange(r)
{
}

Range *
ProxyRange::getRealRange(void) throw()
{
	return realRange;
}

uint32
ProxyRange::fetch_word(uint32 offset, int mode, DeviceExc *client)
{
	return realRange->fetch_word(offset, mode, client);
}

uint16
ProxyRange::fetch_halfword(uint32 offset, DeviceExc *client)
{
	return realRange->fetch_halfword(offset, client);
}

uint8
ProxyRange::fetch_byte(uint32 offset, DeviceExc *client)
{
	return realRange->fetch_byte(offset, client);
}

void
ProxyRange::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
	realRange->store_word(offset, data, client);
}

void
ProxyRange::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	realRange->store_halfword(offset, data, client);
}

void
ProxyRange::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	realRange->store_byte(offset, data, client);
}

int
ProxyRange::getPerms(void) throw()
{
	return realRange->getPerms();
}

void
ProxyRange::setPerms(int newPerms) throw()
{
	realRange->setPerms(newPerms);
}

bool
ProxyRange::canRead(uint32 offset) throw()
{
	return realRange->canRead(offset);
}

bool
ProxyRange::canWrite(uint32 offset) throw()
{
	return realRange->canWrite(offset);
}
