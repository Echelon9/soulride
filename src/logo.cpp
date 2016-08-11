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
// logo.cpp	-thatcher 8/11/1999 Copyright Slingshot Game Technology

// Some code to draw an animated logo.


#include "logo.hpp"
#include "render.hpp"
#include "ImageOverlay.hpp"


namespace Logo {
;


Render::Texture*	LogoImage = 0;
ModeID	Mode = MELLOW;


struct ColorComps {
	int	r, g, b;

	ColorComps() { r = g = b = 0; }
	ColorComps(int red, int green, int blue) { r = red; g = green; b = blue; }
	ColorComps&	operator=(uint32 RGB) {
		r = (RGB >> 16) & 255;
		g = (RGB >> 8) & 255;
		b = (RGB) & 255;
		return *this;
	}
	operator uint32() const { return 0xFF000000 | ((r & 255) << 16) | ((g & 255) << 8) | (b & 255); }
	ColorComps	operator+(const ColorComps& c) {
		return ColorComps(r + c.r, g + c.g, b + c.b);
	}
	ColorComps	operator-(const ColorComps& c) {
		return ColorComps(r - c.r, g - c.g, b - c.b);
	}
	ColorComps	operator*(float f) {
		return ColorComps(r * f, g * f, b * f);
	}
};


ColorComps	Current;
ColorComps	Next, Previous;


const int MELLOW_COLOR_COUNT = 8;
uint32 MellowColors[MELLOW_COLOR_COUNT] = {
	0x00C2751D,
	0x004A4A4A,
	0x004DC456,
	0x004A4A4A,
	0x004D77C4,
	0x004A4A4A,
	0x00934DC4,
	0x004A4A4A,

// Rasta colors
//	0x00DB2020,
//	0x004A4A4A,
//	0x00D9Db20,
//	0x004A4A4A,
//	0x0020DB25,
//	0x004A4A4A,
//	0x00C5C5C5,
//	0x004A4A4A,
};
int	MellowChangeTicks = 1100;


int	ColorTimer = MellowChangeTicks;
int	NextColor = 0;


void	Update(const UpdateState& u)
// Advances the animation.
{
	ColorTimer += u.DeltaTicks;
	
	switch (Mode) {
	default:
	case MELLOW:
		// Smoothly interpolate between mellow colors.
		if (ColorTimer >= MellowChangeTicks) {
			ColorTimer -= MellowChangeTicks;
			
			// Change to the next color.
			Previous = MellowColors[NextColor];
			NextColor++;
			if (NextColor >= MELLOW_COLOR_COUNT) NextColor = 0;
			Next = MellowColors[NextColor];
		}

		Current = (Next - Previous) * (ColorTimer / float(MellowChangeTicks)) + Previous;
		break;

	case THROBBING:
		// Pulsate.
		
		break;
	}
}


void	Render(const ViewState& s)
// Draws the logo.
{
	if (LogoImage == 0) {
		LogoImage = Render::NewTexture("srlogo-flat.psd", true, false, false);
	}

	if (LogoImage) {
		ImageOverlay::Draw(320 - LogoImage->GetWidth() / 2, 25, LogoImage->GetWidth(), LogoImage->GetHeight(), LogoImage, 0, 0, Current);
	}
}


void	ResetAnimation()
// Restarts the animation cycle.
{
}


void	SetMode(ModeID m)
// Changes the animation mode of the logo.
{
	Mode = m;
}


};	// end namespace Logo


