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
// waveread.hpp	-thatcher 5/24/1998 Copyright Slingshot

// Declaration of some functions for reading .WAV files.  Code is adapted from MS DirectSound samples.


#ifndef WAVEREAD_HPP
#define WAVEREAD_HPP


#include <mmreg.h>


namespace WaveRead {
	int	OpenFile(char* Filename, HMMIO* In, WAVEFORMATEX** FormatInfo, MMCKINFO* InRIFF);
	
};


#endif // WAVEREAD_HPP

