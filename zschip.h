/* Software emulation for a Zilog Z85C30 SCC serial chip. */

#ifndef __zschip_h__
#define __zschip_h__

#include "sysinclude.h"
#include "devicemap.h"

class ZSHost;

#define ZSRR_A_BASE 0
#define ZSRR_B_BASE 16

#define ZSWR_A_BASE 0
#define ZSWR_7PRIME 16
#define ZSWR_B_BASE 17

/* Options for initialization of an emulated serial chip. */
class ZSOpts {
public:
	char *chanA_type;
	char *chanA_flags;
	char *chanB_type;
	char *chanB_flags;
};

/* IO-mappable serial chip. */
class ZSChip : public DeviceMap {
public:
	typedef enum Channel {
		A = 0,
		B
	} ZSChannel;
	unsigned char readregs[32];
	unsigned char writeregs[34];
	ZSHost *host_b;
	ZSHost *host_a;
	uint8 last_csr_b;
	uint8 last_csr_a;
	ZSChip(uint32 base, ZSOpts *options);
	bool status_high(void) const;
	bool extended_read_enable(int32 chanbase) const;
	uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	uint8 fetch_byte(uint32 offset, DeviceExc *client);
	uint32 store_word(uint32 offset, uint32 data, DeviceExc *client);
	uint16 store_halfword(uint32 offset, uint16 data, DeviceExc *client);
	uint8 store_byte(uint32 offset, uint8 data, DeviceExc *client);
	uint8 rr(uint8 regno, ZSChannel chan);
};

#endif /* __zschip_h__ */
