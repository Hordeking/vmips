/* Implementation of SPIM compatible console device.
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

#include "spimconsole.h"
#include "spimconsreg.h"
#include "mapper.h"
#include <cassert>

SpimConsoleDevice::SpimConsoleDevice( Clock *clock ) throw( std::bad_alloc )
	: TerminalController( clock,
			      KEYBOARD_POLL_NS,
			      KEYBOARD_REPOLL_NS, 
			      DISPLAY_READY_DELAY_NS ),
	trigger(0), clock_interrupt(false), clock_state(UNREADY)
{
	// FIXME: hack until ranges are done right
	extent = 36;

	display_interrupt[0] = display_interrupt[1] = false;
	keyboard_interrupt[0] = keyboard_interrupt[1] = false;

	trigger = new ClockTrigger( this );
	clock->add_deferred_task( trigger, CLOCK_TRIGGER_NS );
}

SpimConsoleDevice::~SpimConsoleDevice() throw()
{
	assert( trigger );

	trigger->cancel();
}

void SpimConsoleDevice::unready_display( int line, char data )
	throw( std::bad_alloc )
{
	TerminalController::unready_display( line, data );
	deassertInt( line == 0 ? IRQ4 : IRQ6 );	
}

void SpimConsoleDevice::ready_display( int line ) throw()
{
	TerminalController::ready_display( line );
	if( display_interrupt[line] )
		assertInt( line == 0 ? IRQ4 : IRQ6 );
}

void SpimConsoleDevice::unready_keyboard( int line ) throw()
{
	TerminalController::unready_keyboard( line );
	deassertInt( line == 0 ? IRQ3 : IRQ5 );
}

void SpimConsoleDevice::ready_keyboard( int line ) throw()
{
	TerminalController::ready_keyboard( line );
	if( keyboard_interrupt[line] )
		assertInt( line == 0 ? IRQ3 : IRQ5 );
}

void SpimConsoleDevice::unready_clock() throw()
{
	clock_state = UNREADY;
	deassertInt( IRQ2 );
}

void SpimConsoleDevice::ready_clock() throw( std::bad_alloc )
{
	clock_state = READY;

	trigger = new ClockTrigger( this );
	clock->add_deferred_task( trigger, CLOCK_TRIGGER_NS );

	if( clock_interrupt )
		assertInt( IRQ2 );
}

uint32 SpimConsoleDevice::fetch_word( uint32 offset, int mode,
				      DeviceExc *client)
{
	uint32 word = 0;
	
	switch( offset / 4 ) {
	case 0:		// keyboard 1 control
		word = keyboard_interrupt[0] ? CTL_IE : 0;
		if( line_connected(0) )
			word |= lines[0].keyboard_state;
		break;
	case 1:		// keyboard 1 data
		if( line_connected(0) ) {
			unready_keyboard( 0 );
			word = lines[0].keyboard_char;
		}
		break;
	case 2:		// display 1 control
		word = display_interrupt[0] ? CTL_IE : 0;
		if( line_connected(0) )
			word |= lines[0].display_state;
		else
			word |= CTL_RDY;
		break;
	case 3:		// display 1 data
		break;
	case 4:		// keyboard 2 control
		word = keyboard_interrupt[1] ? CTL_IE : 0;
		if( line_connected(1) )
			word |= lines[1].keyboard_state;
		break;
	case 5:		// keyboard 2 data
		if( line_connected(1) ) {
			unready_keyboard( 1 );
			word = lines[1].keyboard_char;
		}
		break;
	case 6:		// display 2 control
		word = display_interrupt[1] ? CTL_IE : 0;
		if( line_connected(1) )
			word |= lines[1].display_state;
		else
			word |= CTL_RDY;
		break;
	case 7:		// display 2 data
		break;
	case 8:		// clock control
		word = clock_interrupt ? CTL_IE : 0;
		word |= clock_state;
		unready_clock();
		break;
	default:
		assert( ! "reached" );
	}

	// FIXME: this is the broken spim byte swaping thing
	return Mapper::mips_to_host_word(word);
}

void SpimConsoleDevice::store_word( uint32 offset, uint32 data,
				    DeviceExc *client )
{
	// FIXME: this is the broken spim byte swapping thing
	data = Mapper::host_to_mips_word(data);

	switch( offset / 4 ) {
	case 0:		// keyboard 1 control
		keyboard_interrupt[0] = data & CTL_IE;
		if( line_connected(0) && keyboard_interrupt[0]
		    && lines[0].display_state == READY )
			assertInt( IRQ3 );
		break;
	case 1:		// keyboard 1 data
		break;
	case 2:		// display 1 control
		display_interrupt[0] = data & CTL_IE;
		if( line_connected(0) && display_interrupt[0]
		    && lines[0].display_state == READY )
			assertInt( IRQ4 );
		break;
	case 3:		// display 1 data
		if( line_connected(0) )
			unready_display( 0, data );
		break;
	case 4:		// keyboard 2 control
		keyboard_interrupt[1] = data & CTL_IE;
		if( line_connected(1) && keyboard_interrupt[1] &&
		    lines[1].keyboard_state == READY )
			assertInt( IRQ5 );
		break;
	case 5:		// keyboard 2 data
		break;
	case 6:		// display 2 control
		display_interrupt[1] = data & CTL_IE;
		if( line_connected(1) && display_interrupt[1] &&
		    lines[1].display_state == READY )
			assertInt( IRQ6 );
		break;
	case 7:		// display 2 data
		if( line_connected(1) )
			unready_display( 1, data );
		break;
	case 8:		// clock control
		clock_interrupt = data & CTL_IE;
		if( clock_interrupt && clock_state == READY )
			assertInt( IRQ2 );
		break;
	default:
		assert( ! "reached" );
	}
}

char *SpimConsoleDevice::descriptor_str()
{
	return "SPIM console";
}


SpimConsoleDevice::ClockTrigger::ClockTrigger( SpimConsoleDevice *console )
	throw()
	: console( console )
{
	assert( console );
}

SpimConsoleDevice::ClockTrigger::~ClockTrigger() throw()
{
}

void SpimConsoleDevice::ClockTrigger::real_task()
{
	console->ready_clock();
}
