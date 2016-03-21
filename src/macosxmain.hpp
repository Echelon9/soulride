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
// linuxmain.hpp	-thatcher 1/26/2001 Copyright Thatcher Ulrich

// Main:: declarations that involve Linux-specific stuff.  It's a back-door
// interface to Main for things like X that need certain Linux-related
// info.


#ifndef LINUXMAIN_HPP
#define LINUXMAIN_HPP

#include <Carbon/Carbon.h>

namespace Main {
	// Use of SDL has pretty much obviated any need for back-door OS info.
};

#ifdef MACOSX_CARBON
void getWindowOptions();
OSStatus SoulRideOptionWindowEventHandler(EventHandlerCallRef myHandler,
                                                 EventRef event,
                                                 void *userData);
int	main2(int argc, char** argv);
#endif // MACOSX_CARBON

#endif // LINUXMAIN_HPP
