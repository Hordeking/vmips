#ifndef __intctrl_h__
#define __intctrl_h__

#include "sysinclude.h"

class DeviceInt;

class IntCtrl {
private:
	DeviceInt *devs;
public:
	IntCtrl();
	uint32 calculateIP(void);
	void connectLine(uint32 line, DeviceInt *dev);
	void link(DeviceInt *dev);
};

#endif /* __intctrl_h__ */
