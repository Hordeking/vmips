/* DZ11-based DEC 5000/200 Serial chip emulation.
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

/* DZ11 serial device as implemented the DECstation 5000/200
 * This version does not support interrupts (CSR's RIE and TIE bits). In
 * order for Linux to fully boot, we will need to support interrupts.
 * This version does not support multiple serial lines. An actual DZ11
 * supports 4 lines (on the 5000/200, these are the keyboard, mouse, modem,
 * and printer ports.)
 *
 * More To-Do items:
 * CSR<TRDY> should depend on TCR<LNENBX>. 
 * CSR<TLINEB:TLINEA> should be set when CSR<TRDY> is set.
 * CSR<MAINT> (loopback) should be implemented.
 * 
 */

#include "sysinclude.h"
#include "deviceexc.h"
#include "deviceint.h"
#include "cpu.h"
#include "decserial.h"
#include "mapper.h"
#include "vmips.h"

DECSerialDevice::DECSerialDevice (Clock *clock) throw()
  : TerminalController (clock, KEYBOARD_POLL_NS, KEYBOARD_REPOLL_NS,
                        DISPLAY_READY_DELAY_NS)
{
  extent = 0x80000;
  master_clear ();
}

DECSerialDevice::~DECSerialDevice() throw ()
{
}

void
DECSerialDevice::master_clear ()
{
  fprintf (stderr, "DZ11 Master clear!\n");
  csr = 0;
  rbuf &= ~DZ_RBUF_DVAL;
  lpr = 0;
  tcr = 0;
}

static bool
TCR_LINE_ENABLED (uint32 Tcr, unsigned int Line)
{
  return ((bool) ((Tcr) & (DZ_TCR_LNENB0 << (Line))));
}

static unsigned int
CSR_TLINE (unsigned int Line)
{
  return (((Line) & 0x03) << 8);
}

static unsigned int
GET_CURRENT_CSR_TLINE (uint32 Csr)
{
  return (((Csr) >> 8) & 0x03);
}

static unsigned int
RBUF_RLINE (unsigned int Line)
{
  return (((Line) & 0x03) << 8);
}

uint32
DECSerialDevice::fetch_word (uint32 offset, int mode, DeviceExc *client)
{
  uint32 rv = 0;
  switch (offset & 0x18) {
  case DZ_CSR:
    csr &= ~(DZ_CSR_RDONE | DZ_CSR_TRDY | DZ_CSR_TLINE);
    for (int line = 0; line < 4; ++line) {
      if (line_connected (line)) {
        if ((csr & DZ_CSR_MSE)
            && lines[line].keyboard_state == READY)
          csr |= DZ_CSR_RDONE;
        if ((csr & DZ_CSR_MSE)
            && TCR_LINE_ENABLED(tcr, line)
            && (lines[line].display_state == READY)) {
          csr |= DZ_CSR_TRDY;
          csr |= CSR_TLINE(line);
        }
      }
    }
	rv = csr;
    break;
  case DZ_RBUF:
    rbuf &= ~DZ_RBUF_DVAL;
    for (int line = 0; line < 4; ++line) {
      if (line_connected (line) && lines[line].keyboard_state == READY) {
        unready_keyboard (line);
        rbuf = lines[line].keyboard_char | DZ_RBUF_DVAL;
        rbuf |= RBUF_RLINE(line);
        break;
      }
    }
    rv = rbuf;
    break;
  case DZ_TCR:
    rv = tcr;
    fprintf (stderr, "DZ11 TCR read as 0x%x\n", rv);
    break;
  case DZ_MSR:
    rv = msr;
    fprintf (stderr, "DZ11 MSR read as 0x%x\n", rv);
    break;
  }
  return rv;
}

void
DECSerialDevice::store_word (uint32 offset, uint32 data, DeviceExc *client)
{
  uint16 data16 = data & 0x0ffff;
  bool any_enable_is_on = false, any_source_is_ready = false; // see FIXME
  switch (offset & 0x18) {
    case DZ_CSR:
      fprintf (stderr, "DZ11 write CSR as %x\n", data16);
      csr = data16;
      if (csr & DZ_CSR_CLR)
        master_clear ();
      display_interrupt_enable = (csr & DZ_CSR_TIE);
      keyboard_interrupt_enable = (csr & DZ_CSR_RIE);
      fprintf (stderr, "DZ11 Keyboard IE is now %s, Display IE now %s, "
               "selected tx line now %d\n",
               keyboard_interrupt_enable ? "on" : "off",
               display_interrupt_enable ? "on" : "off",
               GET_CURRENT_CSR_TLINE (csr));
      any_enable_is_on = (csr & (DZ_CSR_RIE | DZ_CSR_TIE));
      for (int line = 0; line < 4; ++line) {
        any_source_is_ready |= (line_connected (line)
                                && display_interrupt_enable
                                && lines[line].display_state == READY
                                && TCR_LINE_ENABLED (tcr, line));
        any_source_is_ready |= (line_connected (line)
                                && keyboard_interrupt_enable
                                && lines[line].keyboard_state == READY);
      }
      if (!any_enable_is_on)
        deassertInt (IRQ2);
      else if (any_enable_is_on && any_source_is_ready)
        assertInt (IRQ2);
      break;
    case DZ_LPR:
      fprintf (stderr, "DZ11 write LPR as %x\n", data16);
      lpr = data16;
      break;
    case DZ_TCR:
      fprintf (stderr, "DZ11 write TCR as %x\n", data16);
      tcr = data16;
      break;
    case DZ_TDR: {
      int line = 3; // GET_CURRENT_CSR_TLINE (csr);
      if (line_connected (line))
        unready_display (line, data16 & 0xff);
      break;
    }
  }
}

void
DECSerialDevice::unready_display (int line, char data) throw(std::bad_alloc)
{
  TerminalController::unready_display (line, data);
  deassertInt (IRQ2);
}

void
DECSerialDevice::ready_display (int line) throw()
{
  TerminalController::ready_display (line);
  if (display_interrupt_enable)
    assertInt (IRQ2);
}

void
DECSerialDevice::unready_keyboard (int line) throw()
{
  TerminalController::unready_keyboard (line);
  deassertInt (IRQ2);
}

void
DECSerialDevice::ready_keyboard (int line) throw()
{
  TerminalController::ready_keyboard (line);
  if (keyboard_interrupt_enable)
    assertInt (IRQ2);
}

