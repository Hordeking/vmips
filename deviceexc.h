#ifndef __deviceexc_h__
#define __deviceexc_h__

#include "sysinclude.h"
#include "accesstypes.h"

/* An abstract class which describes a device that can handle exceptions. */

class DeviceExc {
public:
    virtual void exception(uint16 excCode, int mode = ANY,
		int coprocno = -1) = 0;
};

#endif /* __deviceexc_h__ */
