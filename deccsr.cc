/* DEC 5000/200 Control/Status Register emulation.
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

#include "sysinclude.h"
#include "deccsrreg.h"
#include "deccsr.h"

DECCSRDevice::DECCSRDevice ()
{
  extent = 0x80000;
}

uint32 DECCSRDevice::update_status_reg (void)
{
  return robits | rwbits | ioint;
}

void DECCSRDevice::update_control_reg (uint32 data)
{
  rwbits = data & CSR_RW_BITS;
  leds = data & CSR_LEDS;
}

uint32
DECCSRDevice::fetch_word (uint32 offset, int mode, DeviceExc *client)
{
  uint32 status_reg = update_status_reg ();
  fprintf (stderr, "CSR status reg read as 0x%x\n", status_reg);
  return status_reg;
}

void
DECCSRDevice::store_word (uint32 offset, uint32 data, DeviceExc *client)
{
  fprintf (stderr, "CSR control reg written with 0x%x\n", data);
  update_control_reg (data);
}
