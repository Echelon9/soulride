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
// mdynamic.cpp	-thatcher 12/14/1998 Copyright Slingshot

// Member functions for MDynamic mix-in to MObject.


#include "model.hpp"
#include "collide.hpp"
#include "utility.hpp"


Collide::CylinderInfo	cylinder = { 2, 1 };	// xxxxxxxx


MDynamic::MDynamic()
// Default constructor.  Initialize members.
{
	Velocity = ZeroVector;
//	Omega = ZeroVector;
	L = ZeroVector;
}


void	MDynamic::RunLogic(const UpdateState& u)
// Run any non-dynamical logic updates the object needs.
// Default implementation does nothing.
{
}


void	MDynamic::RunLogicPostDynamics(const UpdateState& u)
// Run any non-dynamical logic updates the object needs to do
// *after* the dynamics step has finished.
// Default implementation does nothing.
{
}


void	MDynamic::CollisionNotify(const UpdateState& u, const vec3& loc, const vec3& normal, const vec3& impulse)
// Called by the MDynamic::Update() function when it detects a collision.
// Overload this to give the object an opportunity to play a sound or do
// other logic.  The physics of the collision response is handled by
// MDynamic::Update().
// Default implementation does nothing.
{
}


void	MDynamic::Update(const UpdateState& in)
// Runs the physics/logic of the object.  Calls RunLogic() first, then runs dynamics and
// checks for collisions until everything is cool, then calls RunLogicPostDynamics().
{
	// Do the logic update.
	RunLogic(in);
	
	UpdateState	u = in;

	static int	HitCount = 0;	//xxxxxxxxxxx
	
	// Run updates and check for/respond to collisions until everything is consistent.
	Collide::SegmentInfo	seg;
	for (int count = 0; count < 5; count++) {
		vec3	OldLoc = GetLocation();
		RunDynamics(u);
		vec3	NewLoc = GetLocation();
		
		seg.Start = OldLoc;
		seg.Ray = NewLoc - OldLoc;
		float	Distance = seg.Ray.magnitude();
		if (Distance > 0.000001f) {
			seg.Ray *= 1 / Distance;
			seg.LimitTime =  /* u.DeltaT */ Distance;
		} else {
			seg.Ray = XAxis;
			seg.LimitTime = Distance;
		}

		Collide::ContactInfo	c;
		if (Model::CheckForContact(&c, seg, cylinder)) {
			HitCount++;	//xxxxxxx
			
			// Process collision.

			// Advance to the point of collision.
			OldLoc += seg.Ray * c.EnterTime;
			SetLocation(OldLoc);

			// Compute velocity reflection.
			float	Restitution = 0.3f;
			float	ContraryVelocity = -(GetVelocity() * c.Normal);
			vec3	impulse;
			if (ContraryVelocity > 0) {
				impulse = c.Normal * ContraryVelocity * (1 + Restitution);
			} else {
				// Give the object a tiny boost, to keep it from penetrating again.
				impulse = c.Normal * 0.001f;
			}
			SetVelocity(GetVelocity() + impulse);

			// Callback to object, to tell it about
			// collision, in case it wants to cause damage,
			// make a sound, or whatever.
			CollisionNotify(u, c.Location, c.Normal, impulse);

			// Subtract off the time before the collision, so we can re-start the simulation after the collision response.
			if (Distance > 1e-6f) {
				u.DeltaT -= (c.EnterTime / Distance) * u.DeltaT;
			}

			if (u.DeltaT < 0.00001) break;	// Safety cut-off.

		} else {
			// No collisions; we're done updating.
			break;
		}
	}

	// Do any necessary post-dynamics updates.
	RunLogicPostDynamics(in);
}

