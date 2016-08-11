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
// collide.cpp	-thatcher 12/14/1998 Copyright Slingshot

// Some code for collision checking.


#include <math.h>

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif


#include "collide.hpp"


namespace Collide {
;


const float	EPSILON = 0.005f;


static bool	CheckForCircleContact(ContactInfo* result, const SegmentInfo& seg, const CylinderInfo& cyl)
// Computes contact information, considering the x,z plane only, between
// the given line segment and the cylinder (just a circle in 2D).
{
	// Use quadratic formula to find where the distance from the line to the circle is 0.
	// roots = (-b +- sqrt(b^2 - 4ac)) / 2a

	float	t0 = 0;
	float	t1 = 0;

	float	a = seg.Ray.X() * seg.Ray.X() + seg.Ray.Z() * seg.Ray.Z();
	float	b = 2 * (seg.Ray.X() * seg.Start.X() + seg.Ray.Z() * seg.Start.Z());
	float	c = seg.Start.X() * seg.Start.X() + seg.Start.Z() * seg.Start.Z() - cyl.Radius * cyl.Radius;
	
	float	d = b * b - 4 * a * c;
	if (d < 0) {
		// No contact.
		return false;
	}
		
	if (a > EPSILON) {
		d = sqrtf(d);
		t0 = (-b + d) / (2 * a);
		t1 = (-b - d) / (2 * a);
		
		if (t0 > t1) {
			// swap.
			float	temp = t0;
			t0 = t1;
			t1 = temp;
		}

	} else {
		// Zero-length segment.  If it passed the d >= 0 test, then
		// it's in contact.
		t0 = 0;
		t1 = 1;
	}

	result->EnterTime = t0;
	result->ExitTime = t1;
	// Compute normal...
	float	x, z;
	x = seg.Start.X() + seg.Ray.X() * t0;
	z = seg.Start.Z() + seg.Ray.Z() * t0;
	result->Normal.SetX(x);
	result->Normal.SetY(0);
	result->Normal.SetZ(z);
	result->Normal.normalize();

	return true;
}


static bool	CheckForPlaneContact(ContactInfo* result, const SegmentInfo& seg, int axis, float Min, float Max)
// Checks for contacts of the given segment against the volume between the planes whose axis'th
// coordinates are Min and Max.
{
	float	dc = seg.Ray.Get(axis);
	float	sc = seg.Start.Get(axis);
	static const vec3*	AxisTable[3] = { &XAxis, &YAxis, &ZAxis };
	const vec3*	Axis = AxisTable[axis];

	if (dc > -0.0000001 && dc < 0.0000001) {
		// Segment is effectively parallel to the planes.
		if (sc >= Min && sc <= Max) {
			result->EnterTime = -1000000;
			result->ExitTime = +1000000;
			//xxxxx arbitrary.
			if (sc < 0) {
				result->Normal = - *Axis;
			} else {
				result->Normal = *Axis;
			}

			return true;
		} else {
			// No contact.
			return false;
		}
	}

	// Compute when segment enters/leaves the region between -y & +y.
	float	t0 = (Max - sc) / dc;
	float	t1 = (Min - sc) / dc;

	if (t0 <= t1) {
		result->EnterTime = t0;
		result->ExitTime = t1;
		result->Normal = *Axis;
	} else {
		result->EnterTime = t1;
		result->ExitTime = t0;
		result->Normal = - *Axis;
	}

	return true;
}


static bool	TakeIntersection(ContactInfo* result, const ContactInfo& a, const ContactInfo& b)
// Takes the intersection of two contact periods.  The Normal member of *result is
// taken from the contact with the later enter time.
// If the contacts don't overlap, then returns false, otherwise returns true.
{
	if (a.EnterTime > b.ExitTime || a.ExitTime < b.EnterTime) {
		// No overlap.
		return false;
	}

	// Take the later enter time.
	if (a.EnterTime > b.EnterTime) {
		result->EnterTime = a.EnterTime;
		result->Normal = a.Normal;
	} else {
		result->EnterTime = b.EnterTime;
		result->Normal = b.Normal;
	}

	// Take the earlier exit time.
	if (a.ExitTime < b.ExitTime) {
		result->ExitTime = a.ExitTime;
	} else {
		result->ExitTime = b.ExitTime;
	}

	return true;
}


bool	CheckForCylinderContact(ContactInfo* result, const SegmentInfo& seg, const CylinderInfo& cyl)
// Checks for contact between the given line segment and the given
// cylinder.  Returns true and fills in *result if it is able to compute
// contact information; returns false and leaves *result alone if there
// are no contacts.
{
	ContactInfo	PlaneResults, CircleResults;
	
	// Compute contact with circle in x/z plane.
	if (CheckForCircleContact(&CircleResults, seg, cyl) == false) {
		return false;
	}

	// Compute contact with +y/-y.
	if (CheckForPlaneContact(&PlaneResults, seg, 1, -cyl.Height / 2, cyl.Height / 2) == false) {
		return false;
	}

	// Intersect the results.
	ContactInfo	inter;
	if (TakeIntersection(&inter, PlaneResults, CircleResults) == false) {
		return false;
	}

	// If segment exited later than time 0 + epsilon, then no contact.
	if (inter.ExitTime < 0 + EPSILON) return false;

	// If segment entered later than limit time, then no contact.
	if (inter.EnterTime > seg.LimitTime) return false;

	*result = inter;
	if (result->EnterTime < 0) {
		result->EnterTime = 0;
	}

	// Compute collision location.
	result->Location = seg.Start + seg.Ray * result->EnterTime;

	return true;
}


bool	CheckForBoxContact(ContactInfo* result, const SegmentInfo& seg, const BoxInfo& box)
// Checks for contact between the given line segment and the given cylinder.
// Returns true and fills in *result if it is able to find a contact; returns
// false and leaves *result alone if there are no contacts.
{
	ContactInfo	c[3], temp, inter;

	for (int i = 0; i < 3; i++) {
		if (CheckForPlaneContact(&c[i], seg, i, box.Min.Get(i), box.Max.Get(i)) == false) {
			return false;
		}
	}

	if (TakeIntersection(&temp, c[0], c[1]) == false) {
		return false;
	}
	if (TakeIntersection(&inter, temp, c[2]) == false) {
		return false;
	}
	
	// If segment exited later than time 0, then no contact.
	if (inter.ExitTime < 0 + EPSILON) return false;

	// If segment entered later than limit time, then no contact.
	if (inter.EnterTime > seg.LimitTime) return false;

	*result = inter;
	if (result->EnterTime < 0) result->EnterTime = 0;

	// Compute collision location.
	result->Location = seg.Start + seg.Ray * result->EnterTime;

	return true;
}


};

