#ifndef __testdev_h__
#define __testdev_h__

/* A device to test the memory-mappable device functionality. */

#include "sysinclude.h"
#include "devicemap.h"
#include "mapper.h"

class DeviceExc;

class TestDev : public DeviceMap {
private:
	uint8 *stores;
public:
	TestDev(void);
	~TestDev();
	uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	uint8 fetch_byte(uint32 offset, DeviceExc *client);
	uint32 store_word(uint32 offset, uint32 data, DeviceExc *client);
	uint16 store_halfword(uint32 offset, uint16 data, DeviceExc *client);
	uint8 store_byte(uint32 offset, uint8 data, DeviceExc *client);
};

#endif /* __testdev_h__ */ 

