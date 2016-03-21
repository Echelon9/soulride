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
// view.hpp	-thatcher 2/6/1998 Copyright Thatcher Ulrich

// Headers for use by code that transforms geometry for rendering.


#ifndef VIEW_HPP
#define VIEW_HPP


#include "geometry.hpp"


struct Plane {
	vec3	Normal;
	float	D;
};


struct	ViewState {
	int	FrameNumber;
	int	Ticks;
	matrix	ViewMatrix;
	matrix	CameraMatrix;
	vec3	Viewpoint;
	float	MinZ, MaxZ, OneOverMaxZMinusMinZ;

	// Clipping info.
	Plane	ClipPlane[6];
	
	float	XProjectionFactor, YProjectionFactor;
	float	XOffset, YOffset;	// For origin offset.
};


namespace View {
	void	Project(vec3* ScreenCoordResult, const ViewState& s, const vec3& WorldCoordPoint);
	void	Unproject(vec3* WorldCoordRay, const ViewState& s, float ScreenX, float ScreenY);
};


#endif // VIEW_HPP
