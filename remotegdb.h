/* remotegdb2.cc - gdb glue for vmips debugging interface
 * 
 * This file consists almost entirely of code taken from the GNU debugger
 * (gdb), in file gdb-4.17/gdb/serial.h, where it had the copyright notice
 * reproduced below. Additions by me (brg) have been tagged with my initials.
 */

/* Remote serial support interface definitions for GDB, the GNU Debugger.
   Copyright 1992, 1993 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef __remotegdb_h__
#define __remotegdb_h__

/* brg - added glue code */
#include "sysinclude.h"
#define ULONGEST uint64
#if defined(PARAMS)
# undef PARAMS
#endif
#define PARAMS(x) x
typedef int serial_t;
/* brg - end added glue code */

/* brg - added prototypes */
int fromhex (int a);
int tohex (int nib);
int hexnumlen (ULONGEST num);
int readchar (int timeout);
void remote_send (char *buf);
int putpkt (char *buf);
int read_frame (char *buf);
void getpkt (char *buf, int forever);
/* brg - end added prototypes */

/* ... */

/* Read one char from the serial device with TIMEOUT seconds to wait
   or -1 to wait forever.  Use timeout of 0 to effect a poll. Returns
   char if ok, else one of the following codes.  Note that all error
   codes are guaranteed to be < 0.  */

#define SERIAL_ERROR -1		/* General error, see errno for details */
#define SERIAL_TIMEOUT -2
#define SERIAL_EOF -3

extern int serial_readchar PARAMS ((serial_t scb, int timeout));

#define SERIAL_READCHAR(SERIAL_T, TIMEOUT)  serial_readchar (SERIAL_T, TIMEOUT)

/* ... */

/* Write LEN chars from STRING to the port SERIAL_T.  Returns 0 for
   success, non-zero for failure.  */

extern int serial_write PARAMS ((serial_t scb, const char *str, int len));

#define SERIAL_WRITE(SERIAL_T, STRING,LEN)  serial_write (SERIAL_T, STRING, LEN)

#endif /* __remotegdb_h__ */
