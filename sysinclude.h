#ifndef __sysinclude_h__
#define __sysinclude_h__

/* First of all, pull in answers from autoconfiguration system. */
#include "config.h"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <iostream.h>
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
#include <signal.h>

/* We need this for mmap(2). */
#if !defined(MAP_FAILED)
# define MAP_FAILED ((caddr_t)-1L)
#endif

/* Random OS-specific wart-massaging. */
#if defined(__ultrix__)
extern "C" {
# include <sys/ioctl.h>
	extern void bzero(char *b1, int length);
	extern int getopt(int argc, char **argv, char *optstring);
	extern char *optarg;
	extern int optind, opterr, optopt;
	extern char *strdup(const char *s);
	extern int getpagesize(void);
	extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
	int ioctl(int d, int request, void *argp);
	extern caddr_t mmap(caddr_t addr, size_t len, int prot, int flags, int fd,
		off_t off);
	extern caddr_t munmap(caddr_t addr, size_t len);
	extern long random(void);
	extern int select(int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout);
	extern int strcasecmp(char *s1, char *s2);
}
#endif

/* Find me a 64-bit type (the only one that's not strictly necessary.....) */
#if SIZEOF_LONG_LONG == 8
typedef unsigned long long uint64;
typedef long long int64;
#define HAVE_LONG_LONG 1
#elif SIZEOF_LONG == 8
typedef unsigned long uint64;
typedef long int64;
#define HAVE_LONG_LONG 1
#elif SIZEOF_INT == 8
typedef unsigned int uint64;
typedef int int64;
#define HAVE_LONG_LONG 1
#endif

/* Normal-sized types. */
#if SIZEOF_LONG == 4
typedef unsigned long uint32;
typedef long int32;
#elif SIZEOF_INT == 4
typedef unsigned int uint32;
typedef int int32;
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

/* Define BYTESWAPPED if the MIPS target is byte-swapped with respect to
 * the host processor.
 */
#if defined(TARGET_BIG_ENDIAN)
#if !defined(WORDS_BIGENDIAN)
#define BYTESWAPPED
#endif
#elif defined(TARGET_LITTLE_ENDIAN)
#if defined(WORDS_BIGENDIAN)
#define BYTESWAPPED
#endif
#else
#error "MIPS cross tools are neither big nor little endian - unsupported"
#endif

#endif /* __sysinclude_h__ */
