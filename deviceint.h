#ifndef __deviceint_h__
#define __deviceint_h__

#include "sysinclude.h"
#include "periodic.h"
#include "intctrl.h"

/* Interrupt lines that DeviceInts can use. */
#define IRQ7 0x00008000
#define IRQ6 0x00004000
#define IRQ5 0x00002000
#define IRQ4 0x00001000
#define IRQ3 0x00000800
#define IRQ2 0x00000400
/* These are for use by software only, NOT hardware!
 * #define IRQ1 0x00000200
 * #define IRQ0 0x00000100
 */

/* An abstract class (because Periodic::periodic() is pure-virtual)
 * which describes a device that can trigger hardware interrupts in the 
 * processor. DeviceInts can be connected to an IntCtrl (interrupt
 * controller).
 */

class DeviceInt {
private:
	uint32 lines_connected;
	uint32 lines_asserted;
	DeviceInt *next;
	friend class IntCtrl;
protected:
	void assertInt(uint32 line);
	void deassertInt(uint32 line);
	bool isAsserted(uint32 line);
	char *strlineno(uint32 line);
	virtual char *descriptor_str(void) = 0;
	void reportAssert(uint32 line);
	void reportAssertDisconnected(uint32 line);
	void reportDeassert(uint32 line);
	void reportDeassertDisconnected(uint32 line);
	DeviceInt();
};

#endif /* __deviceint_h__ */
