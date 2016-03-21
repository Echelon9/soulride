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
// particle.hpp	8/12/1999 Copyright Slingshot Game Technology

// Module that deals with particle effects.


#ifndef PARTICLE_HPP
#define PARTICLE_HPP


#include "gameloop.hpp"
#include "view.hpp"


namespace Particle {
	void	Open();
	void	Close();
	void	Clear();
	void	Reset();
	
	void	Update(const UpdateState& u);
	void	Render(const ViewState& s);

	enum TypeID {
		SNOW_POWDER,
		SNOW_GRANULAR,
		SNOW_WET,
	};

	struct SourceInfo {
		vec3	Location;
		vec3	Velocity;
		float	LocationVariance, VelocityVariance;

		void	Lerp(const SourceInfo& s0, const SourceInfo& s1, float f)
		// Interpolates this structure's components between s0 and s1 according to
		// f, which should be in the range 0..1.
		{
			Location = s0.Location + (s1.Location - s0.Location) * f;
			Velocity = s0.Velocity + (s1.Velocity - s0.Velocity) * f;
			LocationVariance = s0.LocationVariance + (s1.LocationVariance - s0.LocationVariance) * f;
			VelocityVariance = s0.VelocityVariance + (s1.VelocityVariance - s0.VelocityVariance) * f;
		}
	};
	
	void	PointSource(TypeID Type, const SourceInfo& s0, const SourceInfo& s1, int Count, int DeltaTicks);
	void	LineSource(TypeID Type, const SourceInfo& a0, const SourceInfo& a1, const SourceInfo& b0, const SourceInfo& b1, int Count, int DeltaTicks);
};


#endif // PARTICLE_HPP

