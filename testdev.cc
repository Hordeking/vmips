/* A device to test the memory-mappable device functionality. */

#include "sysinclude.h"
#include "devicemap.h"
#include "cpu.h"
#include "mapper.h"
#include "testdev.h"

TestDev::TestDev(void)
{
	extent = 1024; /* 1k */
	stores = (uint8 *) malloc(1024 * sizeof(uint8));
}

TestDev::~TestDev()
{
	free(stores);
}

uint32
TestDev::fetch_word(uint32 offset, int mode, DeviceExc *client)
{
/* 	return stores[offset - base] << 24 & stores[offset - base + 1] << 16 &
		stores[offset - base + 2] << 8 & stores[offset - base + 3]; */
	return offset;
}

uint16
TestDev::fetch_halfword(uint32 offset, DeviceExc *client)
{
	return stores[offset - base] << 8 & stores[offset - base + 1];
}

uint8
TestDev::fetch_byte(uint32 offset, DeviceExc *client)
{
	return stores[offset - base];
}

uint32
TestDev::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
	uint8 a, b, c, d;

	stores[offset - base]++;
	a = stores[offset - base];
	stores[offset - base + 1]++;
	b = stores[offset - base + 1];
	stores[offset - base + 2]++;
	c = stores[offset - base + 2];
	stores[offset - base + 3]++;
	d = stores[offset - base + 3];
	return (a << 24) | (b << 16) | (c << 8) | d;
}

uint16
TestDev::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	uint8 a, b;

	stores[offset - base]++;
	a = stores[offset - base];
	stores[offset - base + 1]++;
	b = stores[offset - base + 1];
	return (a << 8) | b;
}

uint8
TestDev::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	uint8 a;

	stores[offset - base]++;
	a = stores[offset - base];
	return a;
}
