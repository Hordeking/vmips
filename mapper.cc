/* Physical memory system for the virtual machine.
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

#include "sysinclude.h"
#include "excnames.h"
#include "mapper.h"
#include "cpu.h"
#include "range.h"
#include "devicemap.h"
#include "error.h"

using namespace std;

/* Set the associated CPU of this Mapper object to M, if it is
 * non-NULL.
 */
void
Mapper::attach(CPU *m)
{
	if (m) cpu = m;
}

Mapper::Mapper() :
	last_used_mapping(NULL)
{
}

/* Deconstruction. Deallocate the range list. */
Mapper::~Mapper()
{
	for (Ranges::iterator i = ranges.begin(); i != ranges.end(); i++) {
		delete *i;
	}
}

/* Add range R to the mapping. R must not overlap with any existing
 * ranges in the mapping. Return 0 if R added sucessfully or -1 if
 * R overlapped with an existing range. 
 */
int
Mapper::insert_into_rangelist(Range *r) throw()
{
	assert (r);

	/* Check to make sure the range is non-overlapping. */
	for (Ranges::iterator i = ranges.begin(); i != ranges.end(); i++) {
		if (r->overlaps(*i)) {
			error("Attempt to map two VMIPS components to the "
			       "same memory area.");
			return -1;
		}
	}

	/* Once we're satisfied that it doesn't overlap, add it to the list. */
	ranges.push_back(r);
	return 0;
}

/* The file in FP will be added to the physical memory using mmap(2)
 * at the physical base address BASE.  PERMS are inclusive-OR of MEM_READ
 * for read access and MEM_WRITE for write access.
 *
 * Returns 0 on success and -1 on failure.
 */
int
Mapper::add_file_mapping(FILE *fp, uint32 base, int perms)
	throw()
{
	off_t here;			/* caller's current file position */
	off_t there;		/* end of file */
	uint32 len;			/* difference between here and there */
	caddr_t pa;			/* mmap return value */
	int mmap_prot = 0;	/* mmap protection bits */ 

	/* Find out the size of the file. */
	here = ftell(fp);
	fseek(fp,0,SEEK_END);
	there = ftell(fp);
	len = there - here;

	/* Set the appropriate mmap(2) permissions bits. */
	if (perms & MEM_READ) mmap_prot |= PROT_READ;
	if (perms & MEM_WRITE) mmap_prot |= PROT_WRITE;

	/* Try to map the file into memory. */
	pa = (caddr_t) mmap(0, len, mmap_prot, MAP_PRIVATE, fileno(fp), here);
	if (pa == MAP_FAILED) {
		error("Mapper::add_file_mapping mmap MAP_FAILED: %s",
		       strerror(errno));
		return -1;
	}
	return add_core_mapping(pa, base, len, MMAP, perms);
}

/* The block of memory at P of length LEN will be added to the physical
 * memory at the physical base address BASE.  MAPTYPE is MALLOC if this
 * was a block which can be free(3)'d; it is MMAP if this block can be
 * munmap(2)'d. Otherwise, MAPTYPE is OTHER.  PERMS are inclusive-OR of
 * MEM_READ for read access and MEM_WRITE for write access. Returns 0
 * on success, -1 on failure.
 */
int
Mapper::add_core_mapping(caddr_t p, uint32 base, uint32 len,
	range_type maptype, int perms) throw()
{
	try {
		insert_into_rangelist(new Range(base, len, p, maptype, perms));
	} catch (std::bad_alloc) {
		return -1;
	}
	return 0;
}

/* The memory-mapped device D will be added to the physical memory at
 * the physical base address BASE. Returns 0 on success, -1 on failure.
 */
int
Mapper::add_device_mapping(DeviceMap *d, uint32 base) throw()
{
	try {
		insert_into_rangelist(new ProxyRange(d, base));
	} catch (std::bad_alloc) {
		return -1;
	}
	return 0;
}

/* Returns the first mapping in the rangelist, starting at the beginning,
 * which maps the address P, or NULL if no such mapping exists. This
 * function uses the LAST_USED_MAPPING instance variable as a cache to
 * speed a succession of accesses to the same area of memory.
 */
Range *
Mapper::find_mapping_range(uint32 p) throw()
{
	if (last_used_mapping && last_used_mapping->incorporates(p))
		return last_used_mapping;

	for (Ranges::iterator i = ranges.begin(); i != ranges.end(); i++) {
		if ((*i)->incorporates(p)) {
			last_used_mapping = *i;
			return *i;
		}
	}

	return NULL;
}

/* If the host processor is byte-swapped with respect to the target
 * we are emulating, we will need to swap data bytes around when we
 * do loads and stores. These functions implement the swapping.
 *
 * The mips_to_host_word(), etc. functions are aliases for
 * the swap_word() primitives with more semantically-relevant
 * names.
 */

/* Convert word W from big-endian to little-endian, or vice-versa,
 * and return the result of the conversion.
 */
uint32
Mapper::swap_word(uint32 w)
{
	return ((w & 0x0ff) << 24) | (((w >> 8) & 0x0ff) << 16) |
    	(((w >> 16) & 0x0ff) << 8) | ((w >> 24) & 0x0ff);
}

/* Convert halfword H from big-endian to little-endian, or vice-versa,
 * and return the result of the conversion.
 */
uint16
Mapper::swap_halfword(uint16 h)
{
	return ((h & 0x0ff) << 8) | ((h >> 8) & 0x0ff);
}

/* Convert word W from target processor byte-order to host processor
 * byte-order and return the result of the conversion.
 */
uint32
Mapper::mips_to_host_word(uint32 w)
{
#if defined(BYTESWAPPED)
	return swap_word(w);
#else
	return w;
#endif
}

/* Convert word W from host processor byte-order to target processor
 * byte-order and return the result of the conversion.
 */
uint32
Mapper::host_to_mips_word(uint32 w)
{
#if defined(BYTESWAPPED)
	return swap_word(w);
#else
	return w;
#endif
}

/* Convert halfword H from target processor byte-order to host processor
 * byte-order and return the result of the conversion.
 */
uint16
Mapper::mips_to_host_halfword(uint16 h)
{
#if defined(BYTESWAPPED)
	return swap_halfword(h);
#else
	return h;
#endif
}

/* Convert halfword H from host processor byte-order to target processor
 * byte-order and return the result of the conversion.
 */
uint16
Mapper::host_to_mips_halfword(uint16 h)
{
#if defined(BYTESWAPPED)
	return swap_halfword(h);
#else
	return h;
#endif
}

/* Fetch a word from the physical memory from physical address
 * ADDR. MODE is INSTFETCH if this is an instruction fetch; DATALOAD
 * otherwise. CACHEABLE is true if this access should be routed through
 * the cache, false otherwise.  This routine is shared between instruction
 * fetches and word-wide data fetches.
 * 
 * The routine returns either the specified word, if it is mapped and
 * the address is correctly aligned, or else a word consisting of all
 * ones is returned.
 *
 * Words are returned in the endianness of the target processor; since devices
 * are implemented as Ranges, devices should return words in the host
 * endianness.
 * 
 * This routine may trigger exceptions IBE and/or DBE in the client
 * processor, if the address is unmapped.
 * This routine may trigger exception AdEL in the client
 * processor, if the address is unaligned.
 */
uint32
Mapper::fetch_word(uint32 addr, int32 mode, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 4 != 0) {
		client->exception(AdEL,mode);
		return 0xffffffff;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(mode == INSTFETCH ? IBE : DBE,mode);
		return 0xffffffff;
	}
	offset = addr - l->getBase();
	if (!(l && l->canRead(offset))) {
		/* Reads from nonexistent or write-only ranges return ones */
		return 0xffffffff;
	}
	return host_to_mips_word(l->fetch_word(offset, mode, client));
}

/* Fetch a halfword from the physical memory from physical address ADDR.
 * CACHEABLE is true if this access should be routed through the cache,
 * false otherwise.
 * 
 * The routine returns either the specified halfword, if it is mapped
 * and the address is correctly aligned, or else a halfword consisting
 * of all ones is returned.
 *
 * Halfwords are returned in the endianness of the target processor;
 * since devices are implemented as Ranges, devices should return halfwords
 * in the host endianness.
 * 
 * This routine may trigger exception DBE in the client processor,
 * if the address is unmapped.
 * This routine may trigger exception AdEL in the client
 * processor, if the address is unaligned.
 */
uint16
Mapper::fetch_halfword(uint32 addr, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 2 != 0) {
		client->exception(AdEL,DATALOAD);
		return 0xffff;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATALOAD);
		return 0xffff;
	}
	offset = addr - l->getBase();
	if (!(l && l->canRead(offset))) {
		/* Reads from nonexistent or write-only ranges return ones */
		return 0xffff;
	}
	return host_to_mips_halfword(l->fetch_halfword(offset, client));
}

/* Fetch a byte from the physical memory from physical address ADDR.
 * CACHEABLE is true if this access should be routed through the cache,
 * false otherwise.
 * 
 * The routine returns either the specified byte, if it is mapped,
 * or else a byte consisting of all ones is returned.
 * 
 * This routine may trigger exception DBE in the client processor,
 * if the address is unmapped.
 */
uint8
Mapper::fetch_byte(uint32 addr, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATALOAD);
		return 0xff;
	}
	offset = addr - l->getBase();
	if (!(l && l->canRead(offset))) {
		/* Reads from write-only ranges return ones */
		return 0xff;
	}
	return l->fetch_byte(offset, client);
}

/* Store a word's-worth of DATA to physical address ADDR.
 * CACHEABLE is true if this access should be routed through the cache,
 * false otherwise.
 * 
 * This routine may trigger exception AdES in the client processor,
 * if the address is unaligned.
 * This routine may trigger exception DBE in the client processor,
 * if the address is unmapped.
 */
void
Mapper::store_word(uint32 addr, uint32 data, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 4 != 0) {
		client->exception(AdES,DATASTORE);
		return;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATASTORE);
		return;
	}
	offset = addr - l->getBase();
	if (!(l && l->canWrite(offset))) {
		fprintf(stderr, "Writing bad memory: 0x%08x\n", addr);
		return;
	}
	l->store_word(addr - l->getBase(),
		mips_to_host_word(data),
		client);
}

/* Store half a word's-worth of DATA to physical address ADDR.
 * CACHEABLE is true if this access should be routed through the cache,
 * false otherwise.
 * 
 * This routine may trigger exception AdES in the client processor,
 * if the address is unaligned.
 * This routine may trigger exception DBE in the client processor,
 * if the address is unmapped.
 */
void
Mapper::store_halfword(uint32 addr, uint16 data, bool cacheable, DeviceExc
	*client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 2 != 0) {
		client->exception(AdES,DATASTORE);
		return;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATASTORE);
		return;
	}
	offset = addr - l->getBase();
	if (!(l && l->canWrite(offset))) {
		/* Writes to nonexistent or read-only ranges return ones */
		fprintf(stderr, "Writing bad memory: 0x%08x\n", addr);
		return;
	}
	l->store_halfword(addr - l->getBase(),
		mips_to_host_halfword(data),
		client);
}

/* Store a byte of DATA to physical address ADDR.
 * CACHEABLE is true if this access should be routed through the cache,
 * false otherwise.
 * 
 * This routine may trigger exception DBE in the client processor,
 * if the address is unmapped.
 */
void
Mapper::store_byte(uint32 addr, uint8 data, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATASTORE);
		return;
	}
	offset = addr - l->getBase();
	if (!(l && l->canWrite(offset))) {
		/* Writes to nonexistent or read-only ranges return ones */
		fprintf(stderr, "Writing bad memory: 0x%08x\n", addr);
		return;
	}
	l->store_byte(addr - l->getBase(), data, client);
}

/* Print a hex dump of the first 8 words on top of the stack to the
 * filehandle pointed to by F. The physical address that corresponds to the
 * stack pointer is STACKPHYS. The stack is assumed to grow down in memory;
 * that is, the addresses which are dumped are STACKPHYS, STACKPHYS - 4,
 * STACKPHYS - 8, ...
 */
void
Mapper::dump_stack(FILE *f, uint32 stackphys)
{
	Range *l;

	fprintf(f, "Stack: ");
	if ((l = find_mapping_range(stackphys)) == NULL) {
		fprintf(f, "(points to hole in address space)");
	} else {
		if (l->getType() != MALLOC) {
			fprintf(f, "(points to non-RAM address space)");
		} else {
			for (int i = 0; i > -8; i--) {
				uint32 data =
					((uint32 *) l->
					 getAddress())[(stackphys - l->getBase()) / 4 + i];
#if defined(BYTESWAPPED)
				fprintf(f, "%08x ", swap_word(data));
#else
				fprintf(f, "%08x ", data);
#endif
			}
		}
	}
	fprintf(f, "\n");
}
