/* System include files and glue.
   Copyright 2001, 2003 Brian R. Gaeke.

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

#ifndef _SYSINCLUDE_H_
#define _SYSINCLUDE_H_

/* First of all, pull in answers from autoconfiguration system. */
#include "config.h"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <stdarg.h>
#include <cassert>
#include <vector>
#include <set>
#include <new>
#include <exception>
#include <algorithm>
#include <signal.h>

/* We need this for mmap(2). */
#if !defined(MAP_FAILED)
# define MAP_FAILED ((caddr_t)-1L)
#endif

/* Check whether we can use __attribute__. */
#if HAVE_ATTRIBUTE_FORMAT
# define __ATTRIBUTE_FORMAT__(archetype, string_index, first_to_check) \
    __attribute__((format(archetype, string_index, first_to_check)))
#else
# define __ATTRIBUTE_FORMAT__(archetype, string_index, first_to_check)
#endif
#if HAVE_ATTRIBUTE_NORETURN
# define __ATTRIBUTE_NORETURN__ __attribute__((noreturn))
#else
# define __ATTRIBUTE_NORETURN__
#endif

/* Find me a 64-bit type (the only one that's not strictly necessary.....) */
#if SIZEOF_INT == 8
typedef unsigned int uint64;
typedef int int64;
#define HAVE_LONG_LONG 1
#elif SIZEOF_LONG == 8
typedef unsigned long uint64;
typedef long int64;
#define HAVE_LONG_LONG 1
#elif SIZEOF_LONG_LONG == 8
typedef unsigned long long uint64;
typedef long long int64;
#define HAVE_LONG_LONG 1
#endif

/* Normal-sized types. */
#if SIZEOF_INT == 4
typedef unsigned int uint32;
typedef int int32;
#elif SIZEOF_LONG == 4
typedef unsigned long uint32;
typedef long int32;
#else
#error "Can't find a 32-bit type, and I need one."
#endif

#if SIZEOF_SHORT == 2
typedef unsigned short uint16;
typedef short int16;
#else
#error "Can't find a 16-bit type, and I need one."
#endif

#if SIZEOF_CHAR == 1
typedef unsigned char uint8;
typedef char int8;
#else
#error "Can't find an 8-bit type, and I need one."
#endif

#endif /* _SYSINCLUDE_H_ */
