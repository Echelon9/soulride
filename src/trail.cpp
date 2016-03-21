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
// trail.cpp	-thatcher 3/7/2000 Copyright Slingshot Game Technology

// Some code for maintaining and drawing a trail in the snow behind the boarder.


#include "trail.hpp"


namespace Trail {
;


struct TrailPoint {
	vec3	vec;
	float	u, v;
	uint32	color;
} TrailVerts[TRAIL_SECTIONS * 2];
int	Tickstamp[TRAIL_SECTIONS];

int	TrailFront = 0;
int	TrailRear = 0;

bool	LastChunkWasTrail = false;


void	Open()
// Initialize trail data structure.
{
	
}


void	Close()
// Clean up prior to exit.
{
}


void	Clear()
// Clears the current trail.
{
	TrailFront = 0;
	TrailRear = 0;
	LastChunkWasTrail = false;
}


void	TrailAddTrack(int Ticks, float AWeight, const vector& a, float BWeight, const vector& b)
// Add a segment to the trail.  a and b are on opposite edges of the trail.
// The associated weights indicate how dark each side should be.
{
	if (Recording::IsReadyForStateData() == false) return;
	
//	if (recording) {
//		insert chunk (Ticks, AWeight, a, BWeight, b);
//	}

	// Add a piece of trail.
	TrailVerts[..] ...;

	Tickstamp[..] = Ticks;
	
	LastChunkWasTrail = true;
}


void	TrailNoTrack(int Ticks);
{
	if (LastChunkWasTrail) {
		// Insert break.
		Tickstamp[..++] = -Ticks;
	}
	
	if (Recording::IsReadyForStateData() == false) return;
	if (recording) {
		if (last chunk inserted was a trail) {
			insert chunk (Ticks, no trail);
		}
	}

	if (last chunk inserted was a trail) {
		insert break;
		last chunk inserted was a trail = false;
	}
}


void	Render(const ViewSTate& s)
// Draw the trail.
{
	if (playing back) {
		compute delta ticks;
		if (delta ticks < 0) {
			remove trail from the front;
			add trail to the back;
		} else {
			add trail to the front;
			remove trail from the back;
		}
	}

	gl setup stuff;

	for (sections) {
		find beginning of strip;

		fast-forward to end of strip;

		setup client state;
		draw arrays or whatever;
	}

	gl unsetup stuff;
}



};	// end namespace Trail

