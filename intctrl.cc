#include "sysinclude.h"
#include "deviceint.h"
#include "intctrl.h"

IntCtrl::IntCtrl()
{
	devs = NULL;
}

uint32 IntCtrl::calculateIP(void)
{
	uint32 IP = 0;
	for (DeviceInt *d = devs; ; d = d->next) {
		IP = IP | d->lines_asserted;
		if (d == d->next) break;
	}
	return IP;
}

void IntCtrl::connectLine(uint32 line, DeviceInt *dev)
{
	dev->lines_connected |= line;
	link(dev);
}

void IntCtrl::link(DeviceInt *dev)
{
	if (dev->next == NULL) {
		dev->next = devs;
		devs = dev;
	} /* Otherwise, we assume it's already linked in. */
}
