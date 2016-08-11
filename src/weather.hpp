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
// weather.hpp	-thatcher 3/29/1998 Copyright Thatcher Ulrich

// Take care of backdrop/sky, depth-cueing parameters, snow particle parameters, etc.


#ifndef WEATHER_HPP
#define WEATHER_HPP


#include "view.hpp"
#include "gameloop.hpp"


namespace Weather {
	void	Open();
	void	Close();
	void	Reset();
	
	// Load(...);
	// GenerateRandomWeather(...);

	void	Update(const UpdateState& u);

  void	RenderBackdrop(const ViewState& s);
  void	RenderOverlay(const ViewState& s, bool flakes_have_moved);

	float*	GetFadeColor();	// Returns * to array of four floats, rgba.
	float	GetFadeDistance();

	// SetSnowVelocity(const vector& vel);
	// SetSnowfallDensity([0,1]);

	const vec3&	GetSunDirection();

	struct Parameters {
		uint8	SkydomeGradient[15][3];
		bool	Clouds;
		bool	Snowfall;
		float	SnowDensity;
		uint8	ShadeTable[256][3];

		// sun direction...
		// ambient factor... etc.
	};

	void	GetParameters(Parameters* p);
	void	SetParameters(const Parameters& p);
};


#endif // WEATHER_HPP
