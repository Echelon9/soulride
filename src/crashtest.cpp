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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <string.h>		// For strcpy

// Copyright © 1998 Bruce Dawson.

/*
This source file causes a number of different crashes in order to exercise
the exception handler.
*/

// Disable the optimizer, otherwise it might 'fix' some of the 'bugs'
// that I've placed in my code for test purposes.
#pragma optimize("g", off)

typedef void (*tBogusFunction)();

void __cdecl CrashTestFunction(int CrashCode)
{
	char *p = 0;	// Null pointer.
	char x = 0;
	int y = 0;
	switch (CrashCode)
	{
		case 0:
		default:
			*p = x;	// Illegal write.
			break;
		case 1:
			x = *p;	// Illegal read.
			break;
		case 2:
			strcpy(0, 0);	// Illegal read in C run time.
			break;
		case 3:
		{
			tBogusFunction	BadFunc = (tBogusFunction)0;
			BadFunc();	// Illegal code read - jump to address zero.
			break;
		}
		case 4:
			y = y / y;	// Divide by zero.
			break;
	}
}
