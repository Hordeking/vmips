/* A SPIM-compatible console device. */

#include "sysinclude.h"
#include "devicemap.h"
#include "cpu.h"
#include "mapper.h"
#include "spimconsole.h"
#include "spimconsreg.h"

char *
SPIMConsole::descriptor_str(void)
{
	static char buff[60];
	sprintf(buff, "SPIMConsole(0x%lx,0x%lx)@0x%lx", (long)host[0],
		(long)host[1], (long)this);
	return buff;
}

SPIMConsoleDevice::~SPIMConsoleDevice ()
{				/* nothing to do here */
}

bool SPIMConsoleDevice::intEnable (void)
{
  return ie;
}

void SPIMConsoleDevice::setIntEnable (bool newIntEnable)
{
  ie = newIntEnable;
}

SPIMConsoleKeyboard::SPIMConsoleKeyboard (int l, SPIMConsole *p)
	: SPIMConsoleDevice(l,p)
{
	rdy = false;
	databyte = 0;
}

void SPIMConsoleKeyboard::setData (uint8 newData)
{				/* ignores writes */
}

SPIMConsoleDisplay::SPIMConsoleDisplay (int l, SPIMConsole *p) :
	SPIMConsoleDevice (l, p)
{
	rdy = false;
}

uint8 SPIMConsoleDisplay::data (void)
{				/* reads as zero */
  return 0;
}

SPIMConsoleClock::SPIMConsoleClock (int l, SPIMConsole *p) :
	SPIMConsoleDevice (l, p)
{
	rdy = false;
}

void SPIMConsoleClock::setData (uint8 newData)
{				/* ignores writes */
}

uint8 SPIMConsoleClock::data (void)
{				/* reads as zero */
  return 0;
}

void SPIMConsoleClock::read (void)
{
	rdy = false;
}

void SPIMConsoleClock::check(void)
{
	struct timeval now_time;

	gettimeofday(&now_time, NULL);
/*	fprintf(stderr, "now %lu,%lu  then %lu,%lu  diff %lu  gran %lu\n",
		now_time.tv_sec, now_time.tv_usec,
		timer.tv_sec, timer.tv_usec,
		timediff(&now_time, &timer), CLOCK_GRANULARITY); */
	if ((timer.tv_sec == 0) ||
        (timediff(&now_time, &timer) > CLOCK_GRANULARITY)) {
		rdy = true;
		gettimeofday(&timer, NULL);
/*		fprintf(stderr, "*** RESETTING THE TIMER to %lu,%lu\n",
	        timer.tv_sec, timer.tv_usec); */
	}
}

bool SPIMConsoleClock::ready(void)
{
	check();
	return rdy;
}

void SPIMConsoleKeyboard::check(void)
{
	bool new_rdy;
	struct timeval now_time;

	if (parent->host[lineno]->dataAvailable()) {
		new_rdy = true;
		gettimeofday(&now_time, NULL);
		if ((new_rdy != rdy) ||
		    (timediff(&now_time, &timer) >= IOSPEED)) {
			parent->host[lineno]->read_byte(1, &databyte);
			gettimeofday(&timer, NULL);
		}
		rdy = new_rdy;
	}
}

bool SPIMConsoleKeyboard::ready(void)
{
	check();
	return rdy;
}

uint8 SPIMConsoleKeyboard::data(void)
{
	rdy = false;
	return databyte;
}

void SPIMConsole::periodic(void)
{
	if (host[0]) {
		if (keyboard[0]->ready() && keyboard[0]->intEnable()) {
			/* keyboard #1 interrupt is wired to interrupt line 3 */
			assertInt(IRQ3);
		} else {
			deassertInt(IRQ3);
		}
		if (display[0]->ready() && display[0]->intEnable()) {	
			/* display #1 interrupt is wired to interrupt line 4 */
			assertInt(IRQ4);
		} else {
			deassertInt(IRQ4);
		}
	}
	if (host[1]) {
		if (keyboard[1]->ready() && keyboard[1]->intEnable()) {
			/* keyboard #2 interrupt is wired to interrupt line 5 */
			assertInt(IRQ5);
		} else {
			deassertInt(IRQ5);
		}
		if (display[1]->ready() && display[1]->intEnable()) {
			/* display #2 interrupt is wired to interrupt line 6 */
			assertInt(IRQ6);
		} else {
			deassertInt(IRQ6);
		}
	}
	if (clockdev->ready() && clockdev->intEnable()) {
		/* clock interrupt is wired to interrupt line 2 */
/*		fprintf(stderr, "*** Clock is ready and interrupt is enabled\n"); */
		assertInt(IRQ2);
	} else {
		deassertInt(IRQ2);
	}
}

long 
SPIMConsoleDevice::timediff(struct timeval *after, struct timeval *before)
{
	return (after->tv_sec * 1000000 + after->tv_usec) -
		(before->tv_sec * 1000000 + before->tv_usec);
}

SPIMConsoleDevice::SPIMConsoleDevice(int l, SPIMConsole *p)
{
	lineno = l;
	parent = p;
    ie = false;
    timer.tv_sec = timer.tv_usec = 0;
}

bool
SPIMConsoleDisplay::ready(void)
{
	struct timeval now;
	struct timezone zone;

	if (rdy) return true;
	gettimeofday(&now, &zone);
	if ((timer.tv_sec == 0) || (timediff(&now, &timer) >= 40000)) { 
		rdy = true;
		return true;
	}
	return false;
}

void
SPIMConsoleDisplay::setData(uint8 newData)
{
	struct timezone zone;

	if (!rdy) return;
	rdy = false;
	gettimeofday(&timer, &zone);
	parent->host[lineno]->write_byte(newData);
}

SPIMConsole::SPIMConsole(SerialHost *h0 = NULL, SerialHost *h1 = NULL)
{
	extent = 36; /* 9 words */
	if (h0 || h1) attach(h0, h1);
}

void
SPIMConsole::attach(SerialHost *h0, SerialHost *h1)
{
	if (h0) host[0] = h0;
	if (h1) host[1] = h1;
	if (!keyboard[0]) { keyboard[0] = new SPIMConsoleKeyboard(0, this); }
	if (!keyboard[1]) { keyboard[1] = new SPIMConsoleKeyboard(1, this); }
	if (!display[0]) { display[0] = new SPIMConsoleDisplay(0, this); }
	if (!display[1]) { display[1] = new SPIMConsoleDisplay(1, this); }
	if (!clockdev) { clockdev = new SPIMConsoleClock(0, this); }
}

SPIMConsole::~SPIMConsole()
{
	if (keyboard[0]) { delete keyboard[0]; }
	if (keyboard[1]) { delete keyboard[1]; }
	if (display[0]) { delete display[0]; }
	if (display[1]) { delete display[1]; }
	if (clockdev) { delete clockdev; }
}

uint32
SPIMConsole::fetch_word(uint32 offset, int mode, DeviceExc *client)
{
	int type = UNKNOWN;
	SPIMConsoleDevice *device = NULL;

	get_type(offset, &type, &device);
	if (type == CONTROL) {
		uint32 rv = 0;
		rv = (device->intEnable() ? CTL_IE : 0) |
			   (device->ready() ? CTL_RDY : 0);
		if (device == clockdev) {
			clockdev->read();
		}
		return rv;
	} else if (type == DATA)  {
		return device->data();
	}
	fprintf(stderr, "Impossible SPIM console register access!\n"
		"offset=0x%lx mode=0x%lx client=0x%lx\n",offset,mode,client);
	abort();
}

uint16
SPIMConsole::fetch_halfword(uint32 offset, DeviceExc *client)
{
	uint32 wd = fetch_word(offset & ~0x03, DATALOAD, client);
	return ((uint16 *) &wd)[(offset & 0x03) >> 1];
}

uint8
SPIMConsole::fetch_byte(uint32 offset, DeviceExc *client)
{
	uint32 wd = fetch_word(offset & ~0x03, DATALOAD, client);
	return ((uint8 *) &wd)[offset & 0x03];
}

void
SPIMConsole::get_type(uint32 offset, int *type, SPIMConsoleDevice **device)
{
	switch(offset) {
		case KEYBOARD_1_CONTROL:
			*type = CONTROL; *device = keyboard[0]; break;
		case KEYBOARD_1_DATA:
			*type = DATA; *device = keyboard[0]; break;
		case DISPLAY_1_CONTROL:
			*type = CONTROL; *device = display[0]; break;
		case DISPLAY_1_DATA:
			*type = DATA; *device = display[0]; break;
		case KEYBOARD_2_CONTROL:
			*type = CONTROL; *device = keyboard[1]; break;
		case KEYBOARD_2_DATA:
			*type = DATA; *device = keyboard[1]; break;
		case DISPLAY_2_CONTROL:
			*type = CONTROL; *device = display[1]; break;
		case DISPLAY_2_DATA:
			*type = DATA; *device = display[1]; break;
		case CLOCK_CONTROL:
			*type = CONTROL; *device = clockdev; break;
		default:
			*type = UNKNOWN; *device = NULL; break;
	}
}

uint32
SPIMConsole::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
	int type = UNKNOWN;
	SPIMConsoleDevice *device = NULL;

	get_type(offset, &type, &device);
	if (type == CONTROL) {
		device->setIntEnable(data & CTL_IE);
		/* Ready is read-only. */
		return 0;
	} else if (type == DATA) {
		device->setData(data & 0x0ff);
		return 0;
	}
	fprintf(stderr, "Impossible SPIM console register write!\n"
		"offset=0x%lx data=0x%lx client=0x%lx\n",offset,data,client);
	abort();
}

uint16
SPIMConsole::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	const uint32 word_offset = offset & 0xfffffffc;
	const uint32 halfword_offset_in_word = (offset & 0x02) >> 1;
	uint32 word_data = 0;

	((uint16 *) &word_data)[halfword_offset_in_word] = data;
	return store_word(word_offset, word_data, client);
}

uint8
SPIMConsole::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	const uint32 word_offset = offset & 0xfffffffc;
	const uint32 byte_offset_in_word = (offset & 0x03);
	uint32 word_data;

	((uint8 *) &word_data)[byte_offset_in_word] = data;
	return store_word(word_offset, word_data, client);
}
