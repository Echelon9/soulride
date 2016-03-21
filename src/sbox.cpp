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
// sbox.cpp	-thatcher 1/4/1999 Copyright Slingshot

// Implementation of SBox, a class for box solids, for collisions.


#include "sbox.hpp"
#include "utility.hpp"


SBox::SBox(FILE* fp)
// Initialize the cylinder from the dimensions in the given stream.  Reads
// to the end of the cylinder data.
{
	float	dim[3];
	int	i;
	
	for (i = 0; i < 3; i++) {
		dim[i] = ReadFloat(fp);
	}

	b.Max = vec3(dim[0] / 2, dim[1] / 2, dim[2] / 2);
	b.Min = -b.Max;
}


bool	SBox::CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl)
// Checks for a contact between this box, and the moving cylinder
// represented by the given cyl and the path encoded in the given line
// segment.  If there's a contact, returns true and puts contact info in
// *result.  Otherwise returns false.
{
	// Do collision by expanding the box, and comparing it
	// against the bare line segment.
	// Expand the box my modeling it as four cylinders (at the corners) and
	// two boxes (in the middle).  Check against all these parts, and return
	// the closest contact, if any.

	// Expand this cylinder.
//	Collide::CylinderInfo	temp = c;
//	temp.Height += cyl.Height;
//	temp.Radius += cyl.Radius;

//	// Compare against line segment.
//	return Collide::CheckForCylinderContact(result, seg, temp);

	int	i;
	
	Collide::ContactInfo	temp;
	bool	ContactFound = false;

	// Check for contact with cylinders placed at the corners of the original box.
	Collide::CylinderInfo	c;
	c.Height = b.Max.Y() * 2 + cyl.Height;
	float	YOffset = (b.Max.Y() + b.Min.Y()) / 2;	// Compensate for box not centered on origin.
	c.Radius = cyl.Radius;
	for (i = 0; i < 4; i++) {
		// Transform the segment to position the cylinder at the corners of the box.
		Collide::SegmentInfo	seg2(seg);
		vec3	offset(i & 1 ? b.Max.X() : b.Min.X(),
			       YOffset,
			       i & 2 ? b.Max.Z() : b.Min.Z());
		seg2.Start -= offset;
		
		if (Collide::CheckForCylinderContact(&temp, seg2, c)) {
			if (ContactFound == false || temp.EnterTime < result->EnterTime) {
				*result = temp;
				result->Location += offset;
				ContactFound = true;
			}
		}
	}

	// Check for contact with boxes.
	for (i = 0; i < 2; i++) {
		vec3	Delta;
		Delta.SetY(cyl.Height / 2);
		if (i == 0) {
			Delta.SetX(cyl.Radius);
			Delta.SetZ(0);
		} else {
			Delta.SetX(0);
			Delta.SetZ(cyl.Radius);
		}
		Collide::BoxInfo	box(b.Min - Delta, b.Max + Delta);
		if (Collide::CheckForBoxContact(&temp, seg, box)) {
			if (ContactFound == false || temp.EnterTime < result->EnterTime) {
				*result = temp;
				ContactFound = true;
			}
		}
	}

	return ContactFound;	
}


static struct InitSBox {
	InitSBox() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	// Attach the loader for SBoxes.
	{
		Model::AddSModelLoader(0, LoadSBox);
	}

	static SModel*	LoadSBox(FILE* fp)
	// Creates a box and initializes it from the given stream.
	{
		return new SBox(fp);
	}
	
} InitSBox;

