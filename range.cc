#include "sysinclude.h"
#include "range.h"
#include "accesstypes.h"

/* Debugging - print contents of this mapping range. */
void
Range::print(void)
{
	cout << descriptor_str();
	if (next) { next->print(); }
}

/* Debugging - return a static string describing this mapping in memory. */
char *
Range::descriptor_str(void)
{
	static char buff[80];
	sprintf(buff, "(%lx)[b=%lx e=%lx a=%lx t=%ld p=%lx n=%lx]\n->",
			(unsigned long) this, base, extent, (unsigned long) address,
			type, (unsigned long) prev, (unsigned long) next);
	return buff;
}

uint32
Range::getBase(void)
{
	return base;
}

uint32
Range::getExtent(void)
{
	return extent;
}

caddr_t
Range::getAddress(void)
{
	return address;
}

int32
Range::getType(void)
{
	return type;
}

Range *
Range::getPrev(void)
{
	return prev;
}

Range *
Range::getNext(void)
{
	return next;
}

void
Range::setPrev(Range *newPrev)
{
	prev = newPrev;
}

void
Range::setNext(Range *newNext)
{
	next = newNext;
}

int
Range::getPerms(void)
{
	return perms;
}

void
Range::setPerms(int newPerms)
{
	perms = newPerms;
}

bool
Range::canRead(uint32 offset)
{
	return (perms & MEM_READ);
}

bool
Range::canWrite(uint32 offset)
{
	return (perms & MEM_WRITE);
}

/* Initialization with a range list; insert ourselves into the given list. */
Range::Range(uint32 b, uint32 e, caddr_t a, int32 t, int p, Range *n = NULL)
{
	base = b; extent = e; address = a; type = t; perms = p;
	prev = NULL; next = NULL;
	if (n) n->insert(this);
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
		case UNUSED:
			cerr << "Abort: attempt to delete unused mapping "
				<< descriptor_str() << endl;
			abort();
			break;
		case OTHER:
			/* no destruction routine, the caller will have taken care of it */
			break;
		default:
			cerr << "Abort: attempt to delete unknown type mapping "
				<< descriptor_str() << endl;
			abort();
			break;
	}
}

/* Returns true if ADDR is mapped by this Range object;
 * false otherwise. (A Range object of type UNUSED may not
 * map any addresses.)
 */
bool
Range::incorporates(uint32 addr)
{
	return (getType() != UNUSED) && (addr >= getBase()) &&
		(addr < (getBase() + getExtent()));
}

/* Inserts the new range NEWRANGE into the range list of which
 * this object is the head (sentinel) node. Ranges are kept sorted
 * in ascending order by BASE address.
 */
Range *
Range::insert(Range *newrange)
{
	Range *prev = NULL, *curr = NULL;

	/* I am a list of ranges, fully sorted by BASE address.
	   Incorporate NEWRANGE into myself.
       (after the first element - accounting for sentinel node) */
	prev = this;
	curr = next;
	if (curr != NULL) {
		/* if list is nonempty, search for insertion pt */
		while (curr && curr->base < newrange->base) {
			prev = curr;
			curr = curr->next;
		}
		if (curr)
			curr->prev = newrange;
	}
	newrange->next = curr;
	newrange->prev = prev;
	prev->next = newrange;
	return this;
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

uint32
Range::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
        uint32 *werd;
        /* calculate address */
        werd = ((uint32 *) address) + (offset / 4);
        /* store word */
        *werd = data;
        /* return stored word */
        return *werd;
}

uint16
Range::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	uint16 *halfwerd;
	/* calculate address */
	halfwerd = ((uint16 *) address) + (offset / 2);
	/* store halfword */
	*halfwerd = data;
	/* return stored word */
	return *halfwerd;
}

uint8
Range::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	uint8 *byte;
	byte = ((uint8 *) address) + offset;
	/* store halfword */
	*byte = data;
	/* return stored word */
	return *byte;
}

/* Class ProxyRange implements a system of proxy Range objects, whereby
 * one range can map the same data as another, but with a different physical
 * address in memory. */

ProxyRange::ProxyRange(Range *r, uint32 b, Range *n = NULL) :
	Range(b, r->getExtent(), r->getAddress(), PROXY, r->getPerms(), n)
{
	realRange = r;
}

Range *
ProxyRange::getRealRange(void)
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

uint32
ProxyRange::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
	return realRange->store_word(offset, data, client);
}

uint16
ProxyRange::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	return realRange->store_halfword(offset, data, client);
}

uint8
ProxyRange::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	return realRange->store_byte(offset, data, client);
}

int
ProxyRange::getPerms(void)
{
	return realRange->getPerms();
}

void
ProxyRange::setPerms(int newPerms)
{
	realRange->setPerms(newPerms);
}

bool
ProxyRange::canRead(uint32 offset)
{
	return realRange->canRead(offset);
}

bool
ProxyRange::canWrite(uint32 offset)
{
	return realRange->canWrite(offset);
}
