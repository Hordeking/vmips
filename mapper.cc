/* Code implementing the physical memory unit of the virtual
 * MIPS machine. Hoo ha.
 */

#include "sysinclude.h"
#include "regnames.h"
#include "mapper.h"
#include "cpu.h"
#include "range.h"
#include "devicemap.h"

/* We have been presented with a machine and/or CPU to associate with. */
void
Mapper::attach(CPU *m = NULL)
{
	if (m) cpu = m;
}

/* Debugging - print the range list. */
void
Mapper::print_rangelist(void)
{
	rangelist->print();
	cout << endl;
}

int
Mapper::verify_rangelist(void)
{
	Range *x;

	/* Must start w/ empty node. */
	if (!(rangelist->getBase()==0 && rangelist->getType()==UNUSED &&
		rangelist->getExtent()==0 && rangelist->getAddress()==0)) {
		return -1;
	}
	/* Check integrity of forward and backward links, and
	 * check sorted-ness of ranges.
	 *
	 * We could check for cycles, but that doesn't seem to me to
	 * be particularly interesting.
	 */
	for (x = rangelist; x; x = x->getNext()) {
		if (x->getPrev()) {
			if (x->getPrev()->getNext() != x)
				return -1;
			if (x->getPrev()->getBase() > x->getBase())
				return -1;
		}
		if (x->getNext()) {
			if (x->getNext()->getPrev() != x)
				return -1;
			if (x->getNext()->getBase() < x->getBase())
				return -1;
		}
	}
	return 0;
}

/* Initialization. Clear all state variables and create a new
 * empty range list.
 */
Mapper::Mapper()
{
	rangelist = new Range(0,0,0,0,UNUSED); /* dummy node */
	last_used_mapping = NULL;
}

/* Deconstruction. Deallocate the range list. */
Mapper::~Mapper()
{
	Range *doomed, *temp;

	/* zap whole rangelist */
	doomed = rangelist;
	while (doomed) {
		temp = doomed->getNext();
		delete doomed;
		doomed = temp;
	}
}

/* The file in FP will be added to the physical memory using
 * mmap(2) at the physical base address BASE.
 * PERMS are inclusive-OR of MEM_READ for read access and
 * MEM_WRITE for write access.
 *
 * Returns 0 on success and -1 on failure.
 */
int
Mapper::add_file_mapping(FILE *fp, uint32 base, int perms = MEM_READ_WRITE)
{
	off_t here;		/* caller's current file position */
	off_t there; 	/* end of file */
	uint32 len;	/* difference between here and there */
	caddr_t pa;		/* mmap return value */
	int mmap_prot = 0; /* mmap protection bits */ 

	here = ftell(fp);
	fseek(fp,0,SEEK_END);
	there = ftell(fp);
	len = there - here;

	if (perms & MEM_READ) mmap_prot |= PROT_READ;
	if (perms & MEM_WRITE) mmap_prot |= PROT_WRITE;

	pa = (caddr_t) mmap(0, len, mmap_prot, MAP_PRIVATE, fileno(fp), here);
	if (pa == MAP_FAILED) {
		perror("Abort: Mapper::add_file_mapping mmap MAP_FAILED");
		return -1;
	}
	return add_core_mapping(pa, base, len, MMAP, perms);
}

/* The block of memory at P of length LEN will be added to
 * the physical memory at the physical base address BASE.
 * MAPTYPE is MALLOC if this was a block which can be free(3)'d;
 * it is MMAP if this block can be munmap(2)'d. Otherwise, MAPTYPE
 * is OTHER.  PERMS are inclusive-OR of MEM_READ for read access and
 * MEM_WRITE for write access.
 *
 * Returns 0 on success and -1 on failure.
 */
int
Mapper::add_core_mapping(caddr_t p, uint32 base, uint32 len,
	int maptype = MALLOC, int perms = MEM_READ_WRITE)
{
	Range *newrange;

	/* add range to rangelist */
	newrange = new Range(base, len, p, maptype, perms, rangelist);
	if (!newrange) {
		perror("Abort: Range::operator new returned NULL");
		return -1;
	}
	/* cout << "added new range " << newrange->descriptor_str() << endl; */
	return 0;
}

int
Mapper::add_device_mapping(DeviceMap *d, uint32 base)
{
	Range *newrange = new ProxyRange(d, base, rangelist);
	if (! newrange) {
		perror("Abort: ProxyRange::operator new returned NULL in add_device_mapping");
		return -1;
	}
	return 0;
}

/* Delete the mapping which maps the block whose base address on
 * the host is P and whose length is LEN into the physical memory.
 *
 * XXX In the future, it may become useful to allow for deleting
 * parts of mappings. But this is not yet implemented.
 * 
 * Returns 0 on success and -1 on failure.
 */
int
Mapper::delete_mapping(caddr_t p, uint32 len)
{
	Range *x;

	for (x = rangelist->getNext(); x; x=x->getNext()) {
		if (x->getAddress() == p && x->getExtent() == len) {
			x->getPrev()->setNext(x->getNext());
			if (x->getNext())
				x->getNext()->setPrev(x->getPrev());
			delete x;
			return 0;
		}
	}
	return -1;
}

/* Returns the first mapping in the rangelist
 * after FIRST (or starting at the beginning if FIRST is NULL)
 * which maps the address P, or NULL if no such mapping exists.
 * If FIRST is not specified, this function uses the LAST_USED_MAPPING
 * instance variable as a cache to speed a succession of accesses to
 * the same area of memory.
 */
Range *
Mapper::find_mapping_range(uint32 p, Range *first = NULL)
{
	Range *x;

	if (!first) {
		x = last_used_mapping;
		if (x && x->incorporates(p)) {
			return x;
		}
	}
	for (x = (first ? first : rangelist->getNext()); x; x=x->getNext()) {
		if (x->incorporates(p)) {
			last_used_mapping = x;
			return x;
		}
	}
	return NULL;
}

#if defined(BYTESWAPPED) || defined(TARGET_LITTLE_ENDIAN)  
/* If the host processor is byte-swapped with respect to the target
 * we are emulating, we will need to swap data bytes around when we
 * do loads and stores. These functions implement the swapping.
 */
uint32
Mapper::swap_word(uint32 w)
{
	return ((w & 0x0ff) << 24) | (((w >> 8) & 0x0ff) << 16) |
    	(((w >> 16) & 0x0ff) << 8) | ((w >> 24) & 0x0ff);
}
uint16
Mapper::swap_halfword(uint16 h)
{
	return ((h & 0x0ff) << 8) | ((h >> 8) & 0x0ff);
}
#endif

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
 * This routine may trigger exceptions IBE and/or DBE in the client
 * processor, if the address is unaligned or unmapped.
 */
uint32
Mapper::fetch_word(uint32 addr, int32 mode, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 4 != 0) {
		client->exception(mode == INSTFETCH ? IBE : DBE,mode);
		return 0xffffffff;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(mode == INSTFETCH ? IBE : DBE,mode);
	}
	offset = addr - l->getBase();
	if (!(l && l->canRead(offset))) {
		/* Reads from nonexistent or write-only ranges return ones */
		return 0xffffffff;
	}
#if defined(BYTESWAPPED)
	return swap_word(l->fetch_word(offset, mode, client));
#else
	return l->fetch_word(offset, mode, client);
#endif
}

/* Fetch a halfword from the physical memory from physical address ADDR.
 * CACHEABLE is true if this access should be routed through the cache,
 * false otherwise.
 * 
 * The routine returns either the specified halfword, if it is mapped
 * and the address is correctly aligned, or else a halfword consisting
 * of all ones is returned.
 * 
 * This routine may trigger exception DBE in the associated processor,
 * if the address is unaligned or unmapped.
 */
uint16
Mapper::fetch_halfword(uint32 addr, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 2 != 0) {
		client->exception(DBE,DATALOAD);
		return 0xffff;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATALOAD);
	}
	offset = addr - l->getBase();
	if (!(l && l->canRead(offset))) {
		/* Reads from nonexistent or write-only ranges return ones */
		return 0xffff;
	}
#if defined(BYTESWAPPED)
	return swap_halfword(l->fetch_halfword(offset, client));
#else
	return l->fetch_halfword(offset, client);
#endif
}

/* Fetch a byte from the physical memory from physical address ADDR.
 * CACHEABLE is true if this access should be routed through the cache,
 * false otherwise.
 * 
 * The routine returns either the specified byte, if it is mapped,
 * or else a byte consisting of all ones is returned.
 * 
 * This routine may trigger exception DBE in the associated processor,
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
	}
	offset = addr - l->getBase();
	if (!(l && l->canRead(offset))) {
		/* Reads from write-only ranges return ones */
		return 0xff;
	}
	return l->fetch_byte(offset, client);
}

uint32
Mapper::store_word(uint32 addr, uint32 data, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 4 != 0) {
		client->exception(DBE,DATASTORE);
		return 0xffffffff;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATASTORE);
	}
	offset = addr - l->getBase();
	if (!(l && l->canWrite(offset))) {
		/* Writes to nonexistent or read-only ranges return ones */
		fprintf(stderr, "Writing bad memory: 0x%lx\n", addr);
		return 0xffffffff;
	}
	return l->store_word(addr - l->getBase(),
#if defined(BYTESWAPPED)
		swap_word(data),
#else
		data,
#endif
		client);
}

uint16
Mapper::store_halfword(uint32 addr, uint16 data, bool cacheable, DeviceExc
	*client)
{
	Range *l = NULL;
	uint32 offset;

	if (addr % 2 != 0) {
		client->exception(DBE,DATASTORE);
		return 0xffff;
	}
	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATASTORE);
	}
	offset = addr - l->getBase();
	if (!(l && l->canWrite(offset))) {
		/* Writes to nonexistent or read-only ranges return ones */
		fprintf(stderr, "Writing bad memory: 0x%lx\n", addr);
		return 0xffff;
	}
	return l->store_halfword(addr - l->getBase(),
#if defined(BYTESWAPPED)
		swap_halfword(data),
#else
		data,
#endif
		client);
}

uint8
Mapper::store_byte(uint32 addr, uint8 data, bool cacheable, DeviceExc *client)
{
	Range *l = NULL;
	uint32 offset;

	l = find_mapping_range(addr);
	if (!l) {
		client->exception(DBE,DATASTORE);
	}
	offset = addr - l->getBase();
	if (!(l && l->canWrite(offset))) {
		/* Writes to nonexistent or read-only ranges return ones */
		fprintf(stderr, "Writing bad memory: 0x%lx\n", addr);
		return 0xff;
	}
	return l->store_byte(addr - l->getBase(), data, client);
}
