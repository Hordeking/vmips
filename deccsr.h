/* Headers for Control/Status Register emulation.
   Copyright 2003 Brian R. Gaeke.

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

/* Memory-mapped device representing the Control/Status Register
 * in the DEC 5000/200 (KN02).
 */

#ifndef _DECCSR_H_
#define _DECCSR_H_

#include "sysinclude.h"
#include "devicemap.h"

class DECCSRDevice : public DeviceMap {
	uint32 robits;
	uint32 rwbits;
	uint32 ioint;
	uint32 leds;
	uint32 update_status_reg();
	void update_control_reg(uint32 data);
public:
	DECCSRDevice ();
	uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	void store_word(uint32 offset, uint32 data, DeviceExc *client);
	const char *descriptor_str() const { return "DECstation 5000/200 CSR"; }
};

#endif /* _DECCSR_H_ */
