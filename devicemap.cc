/* Routines implementing memory mappings for devices which support
 * memory-mapped I/O. */

#include "sysinclude.h"
#include "range.h"
#include "devicemap.h"

/* Debugging - return a static string describing this mapping in memory. */
char *
DeviceMap::descriptor_str(void)
{
	static char buff[80];
	sprintf(buff,"(%lx)[b=%lx e=%lx t=dev n=%lx]\n->",(unsigned long)this,
		getBase(), getExtent(),(unsigned long)getNext());
	return buff;
}

/* Initialization with an optional range list;
   insert ourselves into the given list. */
DeviceMap::DeviceMap(Range *n = NULL) :
	Range(0, 0, NULL, DEVICE, 0, n)
{
}

bool
DeviceMap::canRead(uint32 offset)
{
	return true;
}

bool
DeviceMap::canWrite(uint32 offset)
{
	return true;
}
