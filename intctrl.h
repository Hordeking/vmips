#ifndef __intctrl_h__
#define __intctrl_h__

#include "sysinclude.h"

#define HW_INT_6 0x00008000
#define HW_INT_5 0x00004000
#define HW_INT_4 0x00002000
#define HW_INT_3 0x00001000
#define HW_INT_2 0x00000800
#define HW_INT_1 0x00000400

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
