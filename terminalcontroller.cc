/* Definitions to support an abstract terminal controller.
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

#include "error.h"
#include "terminalcontroller.h"

#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

TerminalController::TerminalController( Clock *clock, long keyboard_poll_ns,
					long keyboard_repoll_ns,
					long display_ready_delay_ns )
	throw( std::bad_alloc )
	: keyboard_poll_ns( keyboard_poll_ns ),
	  keyboard_repoll_ns( keyboard_repoll_ns ),
	  display_ready_delay_ns( display_ready_delay_ns ),
	  keyboard_poll(0), clock( clock ), max_fd( -1 )
{
	assert( clock );
	assert( keyboard_poll_ns > 0 );
	assert( keyboard_repoll_ns > 0 );
	assert( display_ready_delay_ns > 0 );

	FD_ZERO( &unready_keyboards );

	for( int i = 0; i < MAX_TERMINALS; i++ ) {
		lines[i].tty_fd = -1;
		lines[i].keyboard_char = 0;
		lines[i].keyboard_state = UNREADY;
		lines[i].display_state = READY;
		lines[i].keyboard_repoll = NULL;
		lines[i].display_delay = NULL;
	}

	keyboard_poll = new KeyboardPoll( this );
	clock->add_deferred_task( keyboard_poll, keyboard_poll_ns );
}

TerminalController::~TerminalController() throw()
{
	for( int i = 0; i < MAX_TERMINALS; i++ ) {
		remove_terminal( i );
	}

	if( keyboard_poll )
		keyboard_poll->cancel();
}

bool TerminalController::connect_terminal( int tty_fd, int line ) throw()
{
	// there is already a terminal connected to that line
	if( line_connected( line ) ) {
		while( close( tty_fd ) == -1 && errno == EINTR );
		return false;
	}
	assert( !lines[line].keyboard_repoll );
	assert( !lines[line].display_delay );

	if( !isatty( tty_fd ) ) {
		error( "attempt to connect non tty to line %d", line );
		while( close( tty_fd ) == -1 && errno == EINTR );
		return false;
	}

	// save the old terminal state
	if( tcgetattr( tty_fd, &lines[line].tty_state ) == -1 ) {
		error( "cannot get terminal state on line %d: %s",
		       line, strerror(errno) );
		while( close( tty_fd ) == -1 && errno == EINTR );
		return false;
	}

	// prepare to call prepare_tty()
	lines[line].tty_fd = tty_fd;

	if( !prepare_tty( line ) ) {
		lines[line].tty_fd = -1;
		while( close( tty_fd ) == -1 && errno == EINTR );
		return false;
	}

	lines[line].keyboard_char = 0;
	lines[line].keyboard_state = UNREADY;
	lines[line].display_state = READY;

	// the keyboard starts with no data
	FD_SET( tty_fd, &unready_keyboards );

	if( tty_fd + 1 > max_fd )
		max_fd = tty_fd + 1;

	assert( line_connected( line ) );
	return true;
}

void TerminalController::remove_terminal( int line ) throw()
{
	assert( line >= 0 && line < MAX_TERMINALS );

	int tty_fd = lines[line].tty_fd;
	if( tty_fd == -1 ) {
		assert( !lines[line].keyboard_repoll );
		assert( !lines[line].display_delay );
		return;
	}
	assert( isatty( lines[line].tty_fd ) );

	// invalidate this line
	lines[line].tty_fd = -1;
	FD_CLR( tty_fd, &unready_keyboards );

	// reset the original terminal settings
	if( tcsetattr( tty_fd, TCSAFLUSH, &lines[line].tty_state ) == -1 )
		error( "cannot restore state for terminal on line %d", line );
	
	// make sure to cancel tasks which refer to this line
	if( lines[line].keyboard_repoll ) {
		lines[line].keyboard_repoll->cancel();
		lines[line].keyboard_repoll = NULL;
	}

	if( lines[line].display_delay ) {
		lines[line].display_delay->cancel();
		lines[line].display_delay = NULL;
	}

	// recompute max_fd
	max_fd = -1;
	for( int i = 0; i < MAX_TERMINALS; i++ ) {
		if( lines[i].tty_fd > max_fd )
			max_fd = lines[i].tty_fd;
	}
	if( max_fd != -1 )
		max_fd++;
	
	assert( !line_connected( line ) );
}

inline bool TerminalController::line_connected( int line ) throw()
{
	return line >= 0 && line < MAX_TERMINALS && lines[line].tty_fd != -1;
}

void TerminalController::reinitialize_terminals() throw()
{
	for( int i = 0; i < MAX_TERMINALS; i++ ) {
		if( !line_connected(i) )
			continue;
		
		if( !prepare_tty( lines[i].tty_fd ) ) {
			error("tty on line %d cannot be restored, removing",i);
			remove_terminal( i );
		}
	}
}

void TerminalController::ready_display( int line ) throw()
{
	assert( line_connected( line ) );
	assert( lines[line].display_state == UNREADY );
	assert( lines[line].display_delay );

	lines[line].display_delay = NULL;
	lines[line].display_state = READY;
}

void TerminalController::unready_display( int line, char data )
	throw( std::bad_alloc )
{
	assert( line_connected( line ) );

	if( lines[line].display_state == UNREADY )
		return;

	int result = write( lines[line].tty_fd, &data, sizeof(char) );
	assert( result == sizeof(char) );

	assert( !lines[line].display_delay );
	lines[line].display_state = UNREADY;
	lines[line].display_delay = new DisplayDelay( this, line ); 
	clock->add_deferred_task( lines[line].display_delay,
				  display_ready_delay_ns );
}

void TerminalController::unready_keyboard( int line ) throw()
{
	assert( line_connected( line ) );
	assert( lines[line].keyboard_state == READY );
	assert( lines[line].keyboard_repoll );	

	FD_SET( lines[line].tty_fd, &unready_keyboards );
	lines[line].keyboard_state = UNREADY;
	lines[line].keyboard_repoll->cancel();
	lines[line].keyboard_repoll = NULL;
}

void TerminalController::ready_keyboard( int line ) throw()
{
	assert( line_connected( line ) );

	lines[line].keyboard_state = READY;
	FD_CLR( lines[line].tty_fd, &unready_keyboards );
	read( lines[line].tty_fd, &lines[line].keyboard_char, sizeof(char) );
}

void TerminalController::repoll_keyboard( int line ) throw( std::bad_alloc )
{
	assert( line_connected( line ) );
	assert( lines[line].keyboard_state == READY );

	ready_keyboard( line );
	lines[line].keyboard_repoll = new KeyboardRepoll( this, line ); 
	clock->add_deferred_task( lines[line].keyboard_repoll,
				  keyboard_repoll_ns );
}

void TerminalController::poll_keyboards() throw( std::bad_alloc )
{
	// FIXME: setup new poll only when there is an UNREADY keyboard
	keyboard_poll = new KeyboardPoll( this );
	clock->add_deferred_task( keyboard_poll, keyboard_poll_ns );
	
	if( max_fd == -1 )
		return;

	fd_set to_read;
	copy_unready_keyboards( &to_read );

	timeval zero = { 0, 0 };
	int read_count = select( max_fd, &to_read, NULL, NULL, &zero );
	
	if( read_count == 0 )
		return;

	for( int i = 0; i < MAX_TERMINALS; i++ ) {
		if(lines[i].tty_fd !=-1 && FD_ISSET(lines[i].tty_fd,&to_read)){
			assert( !lines[i].keyboard_repoll );
			assert( lines[i].keyboard_state == UNREADY );
			
			// do not repoll keyboard until it is unready again
			lines[i].keyboard_repoll = new KeyboardRepoll( this,i);
			clock->add_deferred_task( lines[i].keyboard_repoll,
						   keyboard_repoll_ns );
			ready_keyboard( i );
		}
	}
}

bool TerminalController::prepare_tty( int line ) throw()
{
	assert( line_connected( line ) );
	assert( isatty( lines[line].tty_fd ) );

	int tty_fd = lines[line].tty_fd;

	// use old state as a foundation
	termios new_tty_state = lines[line].tty_state;

	// FIXME: add configurable escape code handling
	// change to the new state, disable echoing and line mode editing
	new_tty_state.c_lflag &= ~(ICANON | ECHO);
	new_tty_state.c_cc[VMIN] = 0;
	new_tty_state.c_cc[VTIME] = 0;

	if( tcsetattr( tty_fd, TCSAFLUSH, &new_tty_state ) == -1 ) {
		error( "cannot set terminal state on line %d: %s",
		       line, strerror(errno) );
		
		// attempt to restore the old state
		tcsetattr( tty_fd, TCSAFLUSH, &lines[line].tty_state );
		return false;
	}

	return true;
}

inline void TerminalController::copy_unready_keyboards( fd_set *set ) throw()
{
	assert( set );

#ifdef	FD_COPY
	FD_COPY( &unready_keyboards, set );
#else
	FD_ZERO(set);

	for( int i = 0; i < MAX_TERMINALS; i++ ) {
		if( line_connected( i ) && lines[i].keyboard_state == UNREADY )
			FD_SET( lines[i].tty_fd, set );
	}
#endif	/* FD_COPY */
}


TerminalController::DisplayDelay::DisplayDelay( TerminalController *controller,
						int line ) throw()
	: controller( controller ), line( line )
{
	assert( controller );
	assert( controller->line_connected( line ) );
}

TerminalController::DisplayDelay::~DisplayDelay() throw()
{
}

void TerminalController::DisplayDelay::real_task()
{
	controller->ready_display( line );
}


TerminalController::KeyboardPoll::KeyboardPoll(TerminalController *controller) 
	throw()
	: controller( controller )
{
	assert( controller );
}

TerminalController::KeyboardPoll::~KeyboardPoll() throw()
{
}

void TerminalController::KeyboardPoll::real_task()
{
	controller->poll_keyboards();
}


TerminalController::KeyboardRepoll::KeyboardRepoll(
	TerminalController *controller, int line )
	throw()
	: controller( controller ), line( line )
{
	assert( controller );
	assert( controller->line_connected( line ) );
}

TerminalController::KeyboardRepoll::~KeyboardRepoll() throw()
{
}

void TerminalController::KeyboardRepoll::real_task()
{
	controller->repoll_keyboard( line );
}
