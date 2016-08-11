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
// collide.hpp	-thatcher 12/14/1998 Copyright Slingshot

// Interface to code for collision checking.


#ifndef COLLIDE_HPP
#define COLLIDE_HPP


#include "geometry.hpp"


namespace Collide {
	struct ContactInfo {
		float	EnterTime, ExitTime;
		vec3	Location;
		vec3	Normal;
	};

	struct SegmentInfo {
		vec3	Start;
		vec3	Ray;
		float	LimitTime;
	};

	struct CylinderInfo {
		float	Height;
		float	Radius;
	};

	struct BoxInfo {
		BoxInfo(const vec3& min = ZeroVector, const vec3& max = ZeroVector) : Min(min), Max(max) {}
		vec3	Min, Max;
	};

	bool	CheckForCylinderContact(ContactInfo* result, const SegmentInfo& seg, const CylinderInfo& cyl);
	bool	CheckForBoxContact(ContactInfo* result, const SegmentInfo& seg, const BoxInfo& box);
};


#endif // COLLIDE_HPP

