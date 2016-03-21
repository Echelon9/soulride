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
#ifndef	EXCEPTIONHANDLER_H
#define	EXCEPTIONHANDLER_H

// Copyright © 1998 Bruce Dawson.

// We forward declare PEXCEPTION_POINTERS so that the function
// prototype doesn't needlessly require windows.h.
typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
int __cdecl RecordExceptionInfo(PEXCEPTION_POINTERS data, const char *Message);

// Call this function if you want to test the exception handler by crashing.
// Pass any integer to specify a particular way of crashing.
void __cdecl CrashTestFunction(int CrashCode);

/*
// Sample usage - put the code that used to be in main into HandledMain.
// To hook it in to an MFC app add exceptionattacher.cpp from the mfctest
// application into your project.
int main(int argc, char *argv[])
{
	int Result = -1;
	__try
	{
		Result = HandledMain(argc, argv);
	}
	__except(RecordExceptionInfo(GetExceptionInformation(), "main thread"))
	{
		// Do nothing here - RecordExceptionInfo() has already done
		// everything that is needed. Actually this code won't even
		// get called unless you return EXCEPTION_EXECUTE_HANDLER from
		// the __except clause.
	}
	return Result;
}
*/

#endif
