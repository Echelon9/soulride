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
// clip.hpp	-thatcher 2/8/1998 Copyright Thatcher Ulrich

// Handles clipping and projection.


#ifndef CLIP_HPP
#define CLIP_HPP


#include "render.hpp"
#include "view.hpp"


namespace Clip {

//	enum Visibility { NO_CLIP, SOME_CLIP, NOT_VISIBLE };
	const int	NO_CLIP = 0x3F;	// Inside all six frustum planes.
	const int	NOT_VISIBLE = -1;	// Special code.
	
	int	ComputeSphereVisibility(const vec3& center, float radius, const ViewState& s, int ClipHint = 0);
	int	ComputeBoxVisibility(const vec3& min, const vec3& max, const ViewState& s, int ClipHint = 0);

	int	ComputeSphereVisibility(const vec3& center, float radius, Plane Frustum[6], int ClipHint = 0);
	int	ComputeSphereSquaredVisibility(const vec3& center, float radius_squared, Plane Frustum[6], int ClipHint = 0);
	int	ComputeBoxVisibility(const vec3& min, const vec3& max, Plane Frustum[6], int ClipHint = 0);
	
//	int	ComputeClipOutcode(const vec3& v, const ViewState& s);

	void	TransformFrustum(Plane Frustum[6], const matrix& obj_to_view);
};


#endif // CLIP_HPP
