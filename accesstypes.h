#ifndef __accesstypes_h__
#define __accesstypes_h__

/* Three kinds of memory accesses are possible.
 *
 * "ANY" is a catch-all used in exception prioritizing which
 * implies that none of the kinds of memory accesses applies,
 * or that the type of memory access otherwise doesn't matter.
 */
#define INSTFETCH 0
#define DATALOAD 1
#define DATASTORE 2
#define ANY 3

/* add_core_mapping and friends maintain a set of protection
 * bits which define allowable access to memory:
 */
#define MEM_READ       0x01
#define MEM_WRITE      0x02
#define MEM_READ_WRITE 0x03

#endif /* __accesstypes_h__ */
