#include "clockdev.h"
#include "accesstypes.h"
#include "vmips.h"

ClockDev::ClockDev()
{
	extent = 20;
	gettimeofday(&real, NULL);
	sim = real;
	time_to_do_interrupt = false;
	did_interrupt = false;
	real_time_is_sim_time = machine->opt->option("realtime")->flag;
	time_ratio = machine->opt->option("timeratio")->num;
	clock_speed = machine->opt->option("clockspeed")->num;
	clock_interrupt_freq = machine->opt->option("clockintr")->num;
}

ClockDev::~ClockDev()
{
}

char *
ClockDev::descriptor_str(void)
{
	return "Clock device";
}

void 
ClockDev::update_times(void)
{
    if (real_time_is_sim_time) {
		gettimeofday(&real, NULL);
		sim_internal_ns += (timediff(&real, &sim) * 1000) / time_ratio;
	} else {
		sim_internal_ns += (1000000000/clock_speed);
	}
	if (sim_internal_ns > clock_interrupt_freq)  {
		time_to_do_interrupt = true;
		sim.tv_usec += (sim_internal_ns/1000);
		sim_internal_ns = (sim_internal_ns%1000);
	} else {
		time_to_do_interrupt = false;
	}
	if (sim.tv_usec>1000000) {
		sim.tv_usec -= 1000000;
		sim.tv_sec += 1;
	}
}

void 
ClockDev::periodic(void)
{
	update_times();
	if (control & CDC_INTERRUPTS_ENABLED) {
		if (did_interrupt) {
			deassertInt(IRQ7);
			did_interrupt = false;
		} else if (time_to_do_interrupt) {
			assertInt(IRQ7);
			did_interrupt = true;
		}
	} else {
		if (did_interrupt) {
			deassertInt(IRQ7);
			did_interrupt = false;
		}
	}
}

uint32 
ClockDev::fetch_word(uint32 offset, int mode, DeviceExc *client)
{
	uint32 rv = 0;

	switch(offset/4) {
		case 0:
			rv = sim.tv_sec;
			break;
		case 1:
			rv = sim.tv_usec;
			break;
		case 2:
			gettimeofday(&real, NULL);
			rv = real.tv_sec;
			break;
		case 3:
			gettimeofday(&real, NULL);
			rv = real.tv_usec;
			break;
		case 4:
			rv = control;
			break;
	}
#if defined(BYTESWAPPED)
    return Mapper::swap_word(rv);
#else
    return rv;
#endif
}

uint16
ClockDev::fetch_halfword(uint32 offset, DeviceExc *client)
{
	uint32 wd = fetch_word(offset & ~0x03, DATALOAD, client);
	return ((uint16 *) &wd)[(offset & 0x03) >> 1];
}

uint8
ClockDev::fetch_byte(uint32 offset, DeviceExc *client)
{
	uint32 wd = fetch_word(offset & ~0x03, DATALOAD, client);
	return ((uint8 *) &wd)[offset & 0x03];
}

uint32
ClockDev::store_word(uint32 offset, uint32 data, DeviceExc *client)
{
#if defined(BYTESWAPPED)
    data = Mapper::swap_word(data);
#endif
	switch(offset/4) {
		case 0:
		case 1:
		case 2:
		case 3:
			return 0;
		case 4:
			control = data;
			return control;
	}
	return 0;
}

uint16
ClockDev::store_halfword(uint32 offset, uint16 data, DeviceExc *client)
{
	const uint32 word_offset = offset & 0xfffffffc;
	const uint32 halfword_offset_in_word = (offset & 0x02) >> 1;
	uint32 word_data = 0;

	((uint16 *) &word_data)[halfword_offset_in_word] = data;
	return store_word(word_offset, word_data, client);
}

uint8
ClockDev::store_byte(uint32 offset, uint8 data, DeviceExc *client)
{
	const uint32 word_offset = offset & 0xfffffffc;
	const uint32 byte_offset_in_word = (offset & 0x03);
	uint32 word_data;

	((uint8 *) &word_data)[byte_offset_in_word] = data;
	return store_word(word_offset, word_data, client);
}
