#ifndef __devicemap_h__
#define __devicemap_h__

#include "sysinclude.h"
#include "range.h"

/* Physical memory map for a device supporting memory-mapped I/O. */
class DeviceMap : public Range {
protected:
	DeviceMap(Range *r = NULL); /* abstract class */
public:
	virtual char *descriptor_str(void);
	virtual uint32 fetch_word(uint32 offset, int mode, DeviceExc *client) = 0;
	virtual uint16 fetch_halfword(uint32 offset, DeviceExc *client) = 0;
	virtual uint8 fetch_byte(uint32 offset, DeviceExc *client) = 0;
	virtual uint32 store_word(uint32 offset, uint32 data, DeviceExc *client)
		= 0;
	virtual uint16 store_halfword(uint32 offset, uint16 data, DeviceExc *client)
		= 0;
	virtual uint8 store_byte(uint32 offset, uint8 data, DeviceExc *client) = 0;
	virtual bool canRead(uint32 offset);
	virtual bool canWrite(uint32 offset);
};

#endif /* __devicemap_h__ */
