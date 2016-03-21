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
// scylinder.cpp	-thatcher 12/21/1998 Copyright Slingshot

// Implementation of SCylinder, a class for cylinder solids, for collisions.


#include "scylinder.hpp"
#include "utility.hpp"


SCylinder::SCylinder(FILE* fp)
// Initialize the cylinder from the dimensions in the given stream.  Reads
// to the end of the cylinder data.
{
	c.Radius = ReadFloat(fp);
	c.Height = ReadFloat(fp);
}


bool	SCylinder::CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl)
// Checks for a contact between this cylinder, and the moving cylinder
// represented by the given cyl and the path encoded in the given line
// segment.  If there's a contact, returns true and puts contact info in
// *result.  Otherwise returns false.
{
	// Do collision by expanding our own cylinder, and comparing it
	// against the bare line segment.

	// Expand this cylinder.
	Collide::CylinderInfo	temp = c;
	temp.Height += cyl.Height;
	temp.Radius += cyl.Radius;

	// Compare against line segment.
	return Collide::CheckForCylinderContact(result, seg, temp);
}


static struct InitSCylinder {
	InitSCylinder() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	// Attach the loader for SCylinders.
	{
		Model::AddSModelLoader(1, LoadSCylinder);
	}

	static SModel*	LoadSCylinder(FILE* fp)
	// Creates a cylinder and initializes it from the given stream.
	{
		return new SCylinder(fp);
	}
	
} InitSCylinder;

