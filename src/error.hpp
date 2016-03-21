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
// error.hpp	-thatcher 1/31/1998 Copyright Thatcher Ulrich

// Error class for throwing exceptions.


#ifndef ERROR_HPP
#define ERROR_HPP


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// XXX need to add safety checks against the length of the string.


class Error {
public:
	Error() { Message[0] = 0; }
	char*	GetMessage() { return Message; }
	
	Error&	operator<<(const char* s) { strcat(Message, s); return *this; }
	Error&	operator<<(int d) { char temp[20]; sprintf(temp, "%d", d); return *this << temp; }
	Error&	operator<<(float f) { char temp[20]; sprintf(temp, "%f", f); return *this << temp; }

private:
	char	Message[300];
};


#endif // ERROR_HPP
