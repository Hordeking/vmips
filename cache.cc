#include "sysinclude.h"
#include "cacheline.h"
#include "cpzero.h"
#include "cpu.h"
#include "cache.h"

void
Cache::attach(CPU *c = NULL, Mapper *m = NULL, CPZero *z = NULL)
{
	if (c) cpu = c;
	if (m) mem = m;
	if (z) cpzero = z;
}

void
Cache::dealloc_cache(void)
{
	uint32 x;

	for (x = 0; x < num_words; x++) {
		delete c[x];
		c[x] = NULL;
	}
	free(c);
	c = NULL;
}

int
Cache::alloc_cache(void)
{
	uint32 x;

	for (x = 0; x < num_words; x++) {
		c[x] = new CacheLine;
		if (!c[x]) {
			return -1;
		}
#ifdef INTENTIONAL_CONFUSION
		c[x]->set_line(random() % 2 ? true : false, random(), random());
#endif /* INTENTIONAL_CONFUSION */
	}
	return 0;
}

int
Cache::init(uint32 nwords, uint8 crefill)
{
	if (nwords < 1024 || nwords > 65536 || CacheLine::parity(32, nwords) != 1) {
		fprintf(stderr,
			"Invalid cache size -- must be 1024 <= 2^k words <= 65536\n");
		return -1;
	}
	if (crefill < 4 || crefill > 32 || CacheLine::parity(8, crefill) != 1) {
		fprintf(stderr,
			"Invalid cache refill -- must be 4 <= 2^k words <= 32\n");
		return -1;
	}

	/* Now we have acceptable values for num_words and cache_refill. */
	num_words = nwords;
	cache_refill = crefill;
	/* Construct cache refill mask from # of cache-refill words. */
	cache_refill_mask = (crefill << 2) - 1;
	/* Construct cache tag mask from # of words (this method
	 * works because the # of words must be a power of 2).
	 */
	cache_tag_mask = (nwords << 2) - 1;
	/* Whatever isn't in the cache tag is in the PFN. */
	pfn_mask = ~cache_tag_mask & 0xfffffffc;

	if (c) {
		/* reinitializing cache */
		dealloc_cache();
	}
	c = (CacheLine **) calloc(nwords, sizeof(CacheLine *));
	if (!c) {
		fprintf(stderr,
			"Cache could not be initialized (insufficient memory)\n");
		return -1;
	}
	if (alloc_cache() < 0) {
		fprintf(stderr,
			"Cache lines could not be initialized (insufficient memory)\n");
		return -1;
	}

	return 0;
}

#define MISS -1
#define HIT 0

int
Cache::read_cache_word(uint32 addr, int mode, uint32 *word)
{
	/* address has form:
	 * pppp pppp pppp pppp pppp cccc cccc cc00
	 * where
	 * p = page frame number (upper bits of physical address)
	 * c = cache tag, used to select the correct cache line
	 * 0 = cache reads force these bits to zero to find correct word
	 *
	 * This is the minimal configuration (with a 10-bit cache tag,
	 * implying 1024 entries). With larger caches, more cache tag bits
	 * and fewer pfn bits are used.
	 */
	uint32 lineno;
	int rc;
	bool caches_isolated = cpzero->caches_isolated();

	lineno = (addr & cache_tag_mask) >> 2;
	if (!c[lineno]->valid() || c[lineno]->pfn() != (addr & pfn_mask)) {
		/* Miss */
		rc = MISS;
		if (!caches_isolated) {
			refill(addr, mode);
		}
	} else {
		rc = HIT;
	}

	*word = c[lineno]->data();
	return rc;
}

int
Cache::read_cache_halfword(uint32 addr, int mode, uint16 *halfword)
{
	uint32 temp;
	int rc = read_cache_word(addr, mode, &temp);

	*halfword = *((uint16 *)((&temp) + (addr & 0x02)));
	return rc;
}

int
Cache::read_cache_byte(uint32 addr, int mode, uint8 *byte)
{
	uint32 temp;
	int rc = read_cache_word(addr, mode, &temp);

	*byte = *((uint8 *)((&temp) + (addr & 0x03)));
	return rc;
}

/* Refill the cache with the CACHE_REFILL words which 
 * have the same starting prefix as ADDR. (CACHE_REFILL
 * must be a power of 2; this is verified by Cache::init().)
 */
void
Cache::refill(uint32 addr, int mode)
{
	uint32 base, x;

	base = addr & ~cache_refill_mask;
	
	for (x = base; x < (base + ((cache_refill - 1) * 4)); x += 4) {
		refill_one_word(x, mode);
	}
}

void
Cache::refill_one_word(uint32 addr, int mode)
{
	uint32 lineno, data;

	lineno = (addr & cache_tag_mask) >> 2;
	data = mem->fetch_word(addr, mode, false, cpu);
	if (data == 0xffffffff && cpu->pending_exception()) return;
	c[lineno]->set_line(true, addr & pfn_mask, data);
}
