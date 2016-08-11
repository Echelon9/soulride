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
/////////////////////////////////////////////////
//
//  File gameguitypes.h
//
//  Start Date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////


#ifndef _GAMEGUITYPES_INCLUDED
#define _GAMEGUITYPES_INCLUDED


//  Types used in the absence of compiler/app defined ones
//  Make edits to suit your compiler

//  Windows-independent boolean/true/false/null stuff
//
#if 0
#ifndef bool
typedef int bool;
#endif
#endif // 0

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef null
#define null 0
#endif

//  Sized integer types
//
#ifndef int16
typedef short int int16;
#endif

#ifndef uint16
typedef unsigned short int uint16;
#endif

#ifndef int32
typedef int int32;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

#ifndef uint
typedef unsigned int uint;
#endif

#ifndef byte
typedef unsigned char byte;
#endif


#endif  // _GAMEGUITYPES_INCLUDED
