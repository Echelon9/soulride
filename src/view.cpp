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
// view.cpp	-thatcher 2/6/1998 Copyright Thatcher Ulrich

// Utility code dealing with view transformations.


#include "render.hpp"
#include "view.hpp"


namespace View {
;


void	Project(vec3* result, const ViewState& s, const vec3& point)
// Transforms and projects the given point according to the given view
// state, and returns a screen-coordinate result.  The z coordinate of
// the result is not significant.
{
	vec3	v;
	s.ViewMatrix.Apply(&v, point);	// Transform.
	float	rz = 1.0f / v.Z();
	result->SetX(v.X() * rz * -s.XProjectionFactor + s.XOffset);
	result->SetY(v.Y() * rz * -s.YProjectionFactor + s.YOffset);
	result->SetZ(rz);	//xxxx
}


void	Unproject(vec3* result, const ViewState& s, float ScreenX, float ScreenY)
// Given the screen coordinates and the view state, forms a world-coordinate vec3
// which represents the line along which the given coordinates have been projected.
{
	float	x = -(ScreenX - s.XOffset) / s.XProjectionFactor;
	float	y = -(ScreenY - s.YOffset) / s.YProjectionFactor;
	vec3	v(x, y, 1);
	s.ViewMatrix.ApplyInverseRotation(result, v);
}

	
};
