/* Definitions to support the main driver program.
   Copyright 2001 Brian R. Gaeke.
   Copyright 2002 Paul Twohey.

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

#ifndef _VMIPS_H_
#define _VMIPS_H_

#include "sysinclude.h"
#include "intctrl.h"
#include "cpzero.h"
#include "mapper.h"
#include "clock.h"
#include "clockdev.h"
#include "cpu.h"
#include "haltdev.h"
#include "memorymodule.h"
#include "options.h"
#include "spimconsole.h"
#include "terminalcontroller.h"

long timediff(struct timeval *after, struct timeval *before);

class vmips
{
public:
	Mapper		*physmem;
	CPZero		*cpzero;
	CPU		*cpu;
	IntCtrl		*intc;
	Options		*opt;
	MemoryModule	*memmod;
	bool		host_bigendian;
	bool		halted;

protected:
	Clock		*clock;
	ClockDevice	*clock_device;
	HaltDevice	*halt_device;

	/* Cached versions of options: */
	bool		opt_bootmsg;
	bool		opt_clockdevice;
	bool		opt_debug;
	bool		opt_dumpcpu;
	bool		opt_dumpcp0;
	bool		opt_haltdevice;
	bool		opt_haltdumpcpu;
	bool		opt_haltdumpcp0;
	bool		opt_instcounts;
	bool		opt_memdump;
	bool		opt_realtime;
	uint32		opt_clockspeed;
	uint32		clock_nanos;
	uint32		opt_clockintr;
	uint32		opt_clockdeviceirq;
	uint32		opt_loadaddr;
	uint32		opt_memsize;
	uint32		opt_timeratio;
	char		*opt_image;
	char		*opt_memdumpfile;

#if TTY
	SpimConsoleDevice	*spim_console;
	bool			opt_usetty;
	char			*opt_ttydev;
	char			*opt_ttydev2;
#endif

private:
	bool dumpcpu, dumpcp0;

	Debug	*dbgr;
	uint32	num_instrs;

protected:
	/* If boot messages are enabled with opt_bootmsg, print MSG as a
	   printf(3) style format string for the remaing arguments. */
	virtual void boot_msg( const char *msg, ... ) throw();

	/* Initialize any configured terminal devices and connect them to
	   configured terminal lines. */
	virtual void setup_terminals() throw( std::bad_alloc );

	/* Initialize the clock device if it is configured. Return true if
	   there are no initialization problems, otherwise return false. */
	virtual bool setup_clockdevice() throw( std::bad_alloc );

	/* Connect the file or device named NAME to line number L of
	   console device C, or do nothing if NAME is "off".  */
	virtual void setup_console_line(int l, char *name,
		SpimConsoleDevice *c) throw();

	/* Initialize the halt device if it is configured. */
	virtual void setup_haltdevice() throw( std::bad_alloc );

public:
	void refresh_options(void);
	vmips(int argc, char **argv);

	/* Cleanup after we are done. */
	virtual ~vmips() throw();
	
	void setup_machine(void);
	void randomize(void);

	/* Halt the simulation. */
	void halt(void) throw();

	/* Return the size of the file ROM, in target-processor words. */
	uint32 auto_size_rom(FILE *rom) throw();
	int host_endian_selftest(void);
	int run_self_tests(void);
	void step(void);
	int run(void);
};

extern vmips *machine;

#endif /* _VMIPS_H_ */
