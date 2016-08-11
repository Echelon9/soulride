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
// comport.cpp	-thatcher 2/9/1998 Copyright Thatcher Ulrich

// Code for accessing the serial ports via Windows.


#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include <io.h>
#include <memory.h>
#include <process.h>

#include "comport.hpp"
#include "error.hpp"


namespace ComPort {


bool	IsOpen = false;
unsigned long	InputThread = -1;
const int	IN_BUFFER_SIZE = 100;
char	InBuffer[IN_BUFFER_SIZE];
int	InBufferNextIn = 0;
int	InBufferNextOut = 0;


void _cdecl	InputThreadFunction(void* param);


void	Open(int port)
// Opens the specified com port.  Port indices start at 1, not 0.
{
	// Make sure buffers are set up.

	// Create the thread which will watch for input.
	IsOpen = true;	// The input thread uses IsOpen as a flag to know when to exit.
	InputThread = _beginthread(InputThreadFunction, 0, (void*) port);
	if (InputThread == -1) {
		Error e; e << "Can't create thread to watch com port.";
		throw e;
	}
}


void	Close()
// Closes the com port.
{
	if (IsOpen) {
		IsOpen = false;	// Flags the input thread to exit.
	}
}


int	GetBytesAvailable()
// Returns the number of received bytes that are available by GetByte().
{
	if (!IsOpen) return 0;

	if (InBufferNextIn >= InBufferNextOut) {
		return InBufferNextIn - InBufferNextOut;
	} else {
		return InBufferNextIn + (IN_BUFFER_SIZE - InBufferNextOut);
	}
}


char	GetByte()
// Returns the next byte in the input buffer of the com port.
{
	if (!IsOpen) return 0;

	// Wait for data.
	while (InBufferNextIn == InBufferNextOut) ;

	// Retrieve a character and remove from the FIFO.
	char	c = InBuffer[InBufferNextOut];
	InBufferNextOut++;
	if (InBufferNextOut >= IN_BUFFER_SIZE) InBufferNextOut = 0;

	return c;
}


void _cdecl	InputThreadFunction(void* PortNumber)
// This is the thread function for servicing input from the serial port.
// Takes the index of the serial port to service; ** indices start at 1, not 0 **.
// It opens the port, watches for input, and puts incoming characters in
// the input FIFO.  If the FIFO fills up, it will drop characters.
{
	HANDLE	ComPortHandle;
	
	int port = int(PortNumber);
	
	DCB dcb;
	DWORD dwError;
	BOOL fSuccess;

	char filename[20];
	strcpy(filename, "COMx");
	filename[3] = '0' + port;
	
	ComPortHandle = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
				   0, /* comm devices must be opened w/exclusive-access */
				   NULL, /* no security attrs */
				   OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
				   FILE_FLAG_OVERLAPPED,    /* overlapped I/O */
				   NULL  /* hTemplate must be NULL for comm devices */
				  );
	if (ComPortHandle == INVALID_HANDLE_VALUE) {
//		Error e; e << "Error in CreateFile(): " << GetLastError();
//		throw e;
		IsOpen = false;	//xxxx
		return;
	}

	/* * Omit the call to SetupComm to use the default queue sizes.
	  * Get the current configuration. */
	fSuccess = GetCommState(ComPortHandle, &dcb);
	if (!fSuccess) {
//		Error e; e << "Error in GetCommState()";
//		throw e;
		CloseHandle(ComPortHandle);
		IsOpen = false;//xxxx
		return;
	}
	
	/* Fill in the DCB */
	dcb.BaudRate = 1200;
	dcb.ByteSize = 7;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	fSuccess = SetCommState(ComPortHandle, &dcb);
	if (!fSuccess) {
//		Error e; e << "Error in SetCommState()";
//		throw e;
		CloseHandle(ComPortHandle);
		IsOpen = false;
		return;
	}

	// Set the timeouts.
	COMMTIMEOUTS	t;
	t.ReadIntervalTimeout = 0;
	t.ReadTotalTimeoutMultiplier = 0;
	t.ReadTotalTimeoutConstant = 1000;	// Time out after one second of waiting.
	t.WriteTotalTimeoutMultiplier = 0;
	t.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(ComPortHandle, &t);
	
//	SetupComm(ComPortHandle, 100, 100);	// Reset the device and allocate new buffers.

	// May need to call ClearCommError() after an error.

	while (IsOpen) {
		char c;
		DWORD	ReadCount = 0;
		
		// Wait for a character.
		BOOL result = ReadFile(ComPortHandle, &c, 1, &ReadCount, NULL);
		if (result == 0 || ReadCount < 1) {
			// Failed for some reason.

			// Maybe call ClearCommError().
			DWORD	ErrorCode;
			ClearCommError(ComPortHandle, &ErrorCode, NULL);
			
		} else {
			// We got a character.

			// See if there's room for it.
			int	NextIndex = InBufferNextIn + 1;
			if (NextIndex >= IN_BUFFER_SIZE) NextIndex = 0;

			if (NextIndex != InBufferNextOut) {
				// Put the new character in the FIFO.
				InBuffer[InBufferNextIn] = c;
				InBufferNextIn = NextIndex;
			} // else there's no room, so we have to discard.
		}
	}

	// Clean up and exit.
	CloseHandle(ComPortHandle);
}


};	// End of namespace ComPort.
