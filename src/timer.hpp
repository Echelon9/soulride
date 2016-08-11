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
// timer.hpp	-thatcher 2/6/1998 Copyright Thatcher Ulrich

// Interface to millisecond timer.


#ifndef TIMER_HPP
#define TIMER_HPP


#include "types.hpp"


namespace Timer {
	uint32	GetTicks();
	void	Sleep(uint32 milliseconds);

	uint64	GetFastTicksFrequency();
	uint64	GetFastTicks();

	class Stopwatch {
	public:
		Stopwatch() { Reset(); }
		void	Reset()
		{
			total = 0;
			Start();
		}

		void	Start()
		{
			last = GetFastTicks();
		}

		void	Stop()
		{
			total += GetFastTicks() - last;
		}

		float	GetMilliseconds()
		{
			return int64(total) * 1000.0f / int64(GetFastTicksFrequency());
		}
		
	private:
		uint64	total, last;
	};
};


// Use this to stopwatch the execution of a function.  Put it at the entry to the function.
#define FunctionTimer(stopwatch) \
	struct FT {		\
		FT() { stopwatch.Start(); }	\
		~FT() { stopwatch.Stop(); }	\
	} FT;
	

#endif // TIMER_HPP
