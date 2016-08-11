/*
    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.

    This file is part of The Soul Ride Engine, see http://soulride.com

    The Soul Ride Engine is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    The Soul Ride Engine is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
// timer.cpp	-thatcher 2/6/1998 Copyright Thatcher Ulrich

// Code for accessing the millisecond timer.


#include "timer.hpp"

#ifndef LINUX


#include <windows.h>
#include <mmsystem.h>
#include <winbase.h>


namespace Timer {


uint32	GetTicks()
// Returns the current tick count in milliseconds.
{
//	return timeGetTime();
	return (uint32) ((GetFastTicks() * 1000) / GetFastTicksFrequency());
}


void	Sleep(uint32 milliseconds)
// Put the current thread to sleep for the specified number of
// milliseconds.  Actual sleeping time may not be precise, especially on
// Win9x.
{
	::Sleep(milliseconds);
}


uint64	GetFastTicksFrequency()
// Returns the frequency of the fast timer, in ticks/second.  Returns 0
// if the fast timer is not supported.
{
	uint64	freq;

	QueryPerformanceFrequency((LARGE_INTEGER*) &freq);

	return freq;
}


uint64	GetFastTicks()
// Returns the ticks count of the fast timer.
{
	uint64	ticks;

	QueryPerformanceCounter((LARGE_INTEGER*) &ticks);

	return ticks;
}


};	// end namespace Timer


#else // LINUX


#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


namespace Timer {
;


static bool	FirstTime = true;
static struct timeval	OpenTime;


uint32	GetTicks()
// Returns the millisecond counter.
{
	// If this is the first time we've been called since program start-up, then
	// initialize the OpenTime value.
	if (FirstTime) {
		FirstTime = false;
		gettimeofday(&OpenTime, NULL);
	}

	struct timeval	now;

	gettimeofday(&now, NULL);

	long	dsec;
	long	dus;

	dsec = now.tv_sec - OpenTime.tv_sec;
	dus = now.tv_usec - OpenTime.tv_usec;

	return dsec * 1000 + int(dus / 1000);
}


void	Sleep(uint32 milliseconds)
// Sleeps for the specified number of milliseconds.
{
	struct timeval	tv;
	tv.tv_sec = int(milliseconds / 1000);
	tv.tv_usec = (milliseconds - tv.tv_sec * 1000) * 1000;
	select(0, NULL, NULL, NULL, &tv);
}


uint64	GetFastTicksFrequency()
// Returns the frequency of the fast timer.
{
	return 1000000;
}


uint64	GetFastTicks()
// Returns the fast tick counter.
{
	// If this is the first time we've been called since program start-up, then
	// initialize the OpenTime value.
	if (FirstTime) {
		FirstTime = false;
		gettimeofday(&OpenTime, NULL);
	}

	struct timeval	now;

	gettimeofday(&now, NULL);

	long	dsec;
	long	dus;

	dsec = now.tv_sec - OpenTime.tv_sec;
	dus = now.tv_usec - OpenTime.tv_usec;

	return dsec * 1000000 + dus;
}


};	// end namespace Timer


#endif // LINUX


