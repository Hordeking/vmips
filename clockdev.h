#include "sysinclude.h"
#include "devicemap.h"
#include "deviceint.h"
#include "periodic.h"
#include "intctrl.h"
#include "range.h"

#ifndef __clockdev_h__
#define __clockdev_h__

#define CDC_SIM_HZ (25*1000000)
#define CDC_INTERRUPTS_ENABLED 0x00000001

class ClockDev : public DeviceMap, public DeviceInt, public Periodic {
private:
	uint32 sim_internal_ns;
	uint32 control;
	uint32 clock_speed;
	uint32 clock_interrupt_freq;
	uint32 time_ratio;
	struct timeval sim, real;
	bool time_to_do_interrupt;
	bool did_interrupt;
	bool real_time_is_sim_time;
public:
	ClockDev();
	virtual ~ClockDev();
	virtual void update_times(void);
	virtual char *descriptor_str(void);
	virtual void periodic(void);
	virtual uint32 fetch_word(uint32 offset, int mode, DeviceExc *client);
	virtual uint16 fetch_halfword(uint32 offset, DeviceExc *client);
	virtual uint8 fetch_byte(uint32 offset, DeviceExc *client);
	virtual uint32 store_word(uint32 offset, uint32 data, DeviceExc *client);
	virtual uint16 store_halfword(uint32 offset, uint16 data, DeviceExc *client);
	virtual uint8 store_byte(uint32 offset, uint8 data, DeviceExc *client);
};

#endif /* __clockdev_h__ */
