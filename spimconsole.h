/* Prototypes for: a SPIM-compatible console device. */

#ifndef __spimconsole_h__
#define __spimconsole_h__

#include "serialhost.h"
#include "deviceint.h"

/* kinds of register to access */
#define DATA                 0
#define CONTROL              1
#define UNKNOWN              2

/* controls how fast things are */
#define IOSPEED				40000   /* usec */
#define CLOCK_GRANULARITY	1000000 /* usec */

class SPIMConsoleKeyboard;
class SPIMConsoleDisplay;
class SPIMConsoleClock;
class SPIMConsoleDevice;

class SPIMConsole : public DeviceMap, public DeviceInt, public Periodic {
private:
	SPIMConsoleKeyboard *keyboard[2];
	SPIMConsoleDisplay *display[2];
	SPIMConsoleClock *clockdev;
public:
	SerialHost *host[2];

	SPIMConsole(SerialHost *h1 = NULL, SerialHost *h2 = NULL);
	~SPIMConsole();
	char *descriptor_str(void);
	void attach(SerialHost *h1, SerialHost *h2);
	uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	uint8 fetch_byte(uint32 offset, DeviceExc *client);
	uint32 store_word(uint32 offset, uint32 data, DeviceExc *client);
	uint16 store_halfword(uint32 offset, uint16 data, DeviceExc
		*client);
	uint8 store_byte(uint32 offset, uint8 data, DeviceExc *client);
	void get_type(uint32 offset, int *type, SPIMConsoleDevice **device);
	void periodic(void);
};

class SPIMConsoleDevice {
protected:
	bool ie;
	struct timeval timer;
	int lineno;
	SPIMConsole *parent;
public:
	SPIMConsoleDevice(int l, SPIMConsole *p);
	virtual ~SPIMConsoleDevice();

	virtual bool ready(void) = 0;
	virtual uint8 data(void) = 0;
	virtual void setData(uint8 newData) = 0;

	virtual bool intEnable(void);
	virtual void setIntEnable(bool newIntEnable);
};

class SPIMConsoleKeyboard : public SPIMConsoleDevice {
private:
	bool rdy;
	uint8 databyte;
public:
	virtual bool ready(void);
	virtual uint8 data(void);

	virtual void check(void);
	SPIMConsoleKeyboard(int l, SPIMConsole *p);
	virtual void setData(uint8 newData);
};

class SPIMConsoleDisplay : public SPIMConsoleDevice {
private:
	bool rdy;
public:
	virtual bool ready(void);
	virtual void setData(uint8 newData);

	SPIMConsoleDisplay(int l, SPIMConsole *p);
	virtual uint8 data(void);
};

class SPIMConsoleClock : public SPIMConsoleDevice {
private:
	bool rdy;
public:
	virtual bool ready(void);

	virtual void check(void);
	SPIMConsoleClock(int l, SPIMConsole *p);
	virtual void setData(uint8 newData);
	virtual uint8 data(void);
	virtual void read(void);
};

#endif /* __spimconsole_h__ */
