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
// psdread.hpp	-thatcher 8/19/1998 Copyright Slingshot

// Header for routines to handle Photoshop 2.5 .PSD files.


#ifndef PSDREAD_HPP
#define PSDREAD_HPP


#include "types.hpp"
#include "geometry.hpp"


namespace PSDRead {
	bitmap32*	ReadImageData32(const char* filename, float* Width = 0, float* Height = 0);
	void	WriteImageData32(const char* filename, uint32* RGBAData, int Width, int Height, bool IncludeAlpha);
};


#endif // PSDREAD_HPP

