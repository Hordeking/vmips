/* Definitions to support the interrupt controller.
   Copyright 2001, 2002 Brian R. Gaeke.
   Copyright 2002 Paul Twohey.

This file is part of VMIPS.

VMIPS is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

VMIPS is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with VMIPS; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _INTCTRL_H_
#define _INTCTRL_H_

#include "sysinclude.h"

class DeviceInt;

class IntCtrl {
private:
	typedef std::vector<DeviceInt *> Devices;
	Devices devs;

public:
	IntCtrl() throw();
	virtual ~IntCtrl() throw();

	/* Return the mask of all asserted interrupts for connected devices. */
	virtual uint32 calculateIP() throw();

	/* Connect interrupt line LINE (see deviceint.h) to device DEV. */
	virtual void connectLine(uint32 line, DeviceInt *dev);
};

#endif /* _INTCTRL_H_ */
