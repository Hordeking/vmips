/* Software emulation for a Zilog Z85C30 SCC serial chip. */

#include "sysinclude.h"
#include "devicemap.h"
#include "mapper.h"
#include "regnames.h"
#include "cpu.h"
#include "zschip.h"

/* The other side of a ZSChip device, i.e., the real-world serial
 * device the emulation is talking to. */
class ZSHost {
public:
	enum HostType {
		HT_NONE = 0, /* channel is not connected (yet) */
		HT_PTY,      /* channel is connected to a host pty */
		HT_SERIAL,   /* channel is connected to a host serial port */
		HT_DEV_NULL  /* channel is connected to the bit bucket */
	};
	HostType htype;
	char *hdevname;
	int hdevfd;
	ZSChip::Channel chan;
	ZSHost(ZSChip::Channel id, char *hostType, char *hostFlags);
	int openHostDevice(char *filename, char *type);
};

ZSHost::ZSHost(ZSChip::Channel id, char *hostType, char *hostFlags)
{
	chan = id;
	htype = HT_NONE;
	if (strcasecmp(hostType,"pty")==0) {
		htype = HT_PTY;
		hdevfd = openHostDevice(hostFlags, hostType);
	} else if (strcasecmp(hostType,"serial")==0) {
		htype = HT_SERIAL;
		hdevfd = openHostDevice(hostFlags, hostType);
	} else if (strcasecmp(hostType,"null")==0) {
		htype = HT_DEV_NULL;
	}
}

int
ZSHost::openHostDevice(char *filename, char *type)
{
	int fd;
	fd = open(filename, O_RDWR|O_NONBLOCK, 0666);
	if (fd < 0) {
		fprintf(stderr,"Could not open %s %s: %s\n",
			type, filename, strerror(errno));
		htype = HT_NONE;
	}
	return fd;
}

/* The memory mapping of the emulated Z85C30 is all of four bytes. It looks like
 *   |<---byte #0----><---byte #1-----><---byte #2----><---byte #3----->|
 *   |<--CSR bits A--><--Data bits A--><--CSR bits B--><--Data bits B-->|
 *   where   CSR bits   are what you get when you access the bus with D//C low
 *   where   data bits  are what you get when you access the bus with D//C high
 */

uint32
ZSChip::fetch_word(uint32 offset, int mode, DeviceExc *client)
{
	uint8 d4, d3, d2, d1;
	offset -= base;
	if (offset != 0) {
		client->exception(mode == INSTFETCH ? IBE : DBE, mode);
		return 0xff;
	}
	d4 = fetch_byte(0, client);
	d3 = fetch_byte(1, client);
	d2 = fetch_byte(2, client);
	d1 = fetch_byte(3, client);
	return (uint32) ((d4 << 24) | (d3 << 16) | (d2 << 8) | d1);
}

uint16
ZSChip::fetch_halfword(uint32 offset, DeviceExc *client)
{
	uint8 d2, d1;
	offset -= base;
	if (offset != 0 && offset != 2) {
		client->exception(DBE, DATALOAD); return 0xff;
	}
	d2 = fetch_byte(offset, client);
	d1 = fetch_byte(offset + 1, client);
	return (uint16) ((d2 << 8) | d1);
}

uint8
ZSChip::fetch_byte(uint32 offset, DeviceExc *client)
{
	int regno = base - offset;
	uint8 val;

	switch(regno) {
		case 0: val = rr(last_csr_b, B); last_csr_b = 0; return val;
		case 1: return rr(8, B);
		case 2: val = rr(last_csr_a, A); last_csr_a = 0; return val;
		case 3: return rr(8, A);
		default: client->exception(DBE, DATALOAD); return 0xff;
	}
}

uint32
ZSChip::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
	return 0xffffffff;
}

uint16
ZSChip::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	return 0xffff;
}

uint8
ZSChip::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	return 0xff;
}

ZSChip::ZSChip(uint32 base, ZSOpts *options)
{
	host_a = new ZSHost(ZSChip::A, options->chanA_type,
		options->chanA_flags);
	host_b = new ZSHost(ZSChip::B, options->chanB_type,
		options->chanB_flags);
}

bool ZSChip::status_high(void) const {
	return writeregs[ZSRR_A_BASE + 9] & 0x10;
}

bool ZSChip::extended_read_enable(int32 chanbase) const {
	return writeregs[chanbase + ZSWR_7PRIME] & 0x40;
}

uint8
ZSChip::rr(uint8 regno, ZSChannel chan)
{
	uint8 rr2a = readregs[2];
	int32 chanbase;
	uint8 status; /* XXX FIXME see p 5-14 */

	chanbase = (chan == A ? ZSRR_A_BASE : ZSRR_B_BASE);
	switch (regno) {
		case 0: return readregs[chanbase + regno];
		case 1: return readregs[chanbase + regno];
		case 2:
			if (chan == A) {
				return rr2a;
			} else {
				/* check Status High/Status Low */
				if (status_high()) {
					return (rr2a & 0x8f) | (status << 4);
				} else {
					return (rr2a & 0xf1) | (status << 1);
				}
			}
		case 3:
			if (chan == A) {
				return readregs[3];
			} else {
				return 0;
			}
		case 4:
			if (extended_read_enable(chanbase)) {
				return writeregs[chanbase + 4];
			} else {
				return writeregs[chanbase + 0];
			}
		case 5:
			if (extended_read_enable(chanbase)) {
				return writeregs[chanbase + 5];
			} else {
				return writeregs[chanbase + 1];
			}
		/* XXX FIXME NOT DONE YET */
	}
	return 0xff;
}
