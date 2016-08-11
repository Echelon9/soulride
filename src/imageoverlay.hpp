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
// ImageOverlay.hpp	12/30/1999 Copyright Slingshot Game Technology

// Interface for 2D image overlays.


#ifndef IMAGEOVERLAY_HPP
#define IMAGEOVERLAY_HPP


#include "render.hpp"


namespace ImageOverlay {
	void	Open();
	void	Close();

	void	Draw(int x, int y, int width, int height, Render::Texture* im, int u, int v, uint32 ARGBColor = 0xFFFFFFFF, float Scale = 1);
};


#endif // IMAGEOVERLAY_HPP

