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
// types.hpp	-thatcher 8/13/1998 Copyright Thatcher Ulrich

// Some standard types.


#ifndef TYPES_HPP
#define TYPES_HPP




typedef signed char	int8;
typedef unsigned char	uint8;
typedef uint8	byte;
typedef short int	int16;
typedef unsigned short int	uint16;
typedef int	int32;
typedef unsigned int	uint32;
#ifndef LINUX
  typedef unsigned __int64	uint64;
  typedef __int64	int64;
#else // not LINUX
  typedef unsigned long long	uint64;
  typedef long long	int64;
#endif // not LINUX


#endif // TYPES_HPP
