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
// gstamp.cpp	-thatcher 12/2/1998 Copyright Slingshot

// Code for a GModel class that draws a vertically-oriented, scaled sprite, which always
// rotates to face the viewer.  I.e. a model type for displaying cheap trees.


#include <math.h>

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif



#include "ogl.hpp"
#include "gstamp.hpp"
#include "gameloop.hpp"
#include "clip.hpp"
#include "psdread.hpp"


struct UVPair {
	float	U, V;
	void	SetU(float u) { U = u; }
	void	SetV(float v) { V = v; }
	float*	GetUV() { return &U; }
};
static UVPair	vert[4] = {
	{ 0, 1 },
	{ 1, 1 },
	{ 1, 0 },
	{ 0, 0 },
};

static vec3	Right;
	

class GStampImp : public GStamp {
public:
	GStampImp(const char* filename)
	// Initialize the GStamp from the given file, which should be a Photoshop-format image file (.PSD).
	{
		LastFrameNumber = -1;
		
		// Load the image, create the texture, and set height/width.
		Width = 1;
		Height = 1;
		bitmap32*	b = PSDRead::ReadImageData32(filename, &Width, &Height);
		if (b == NULL) {
			Error e; e << "Can't load image '" << filename << "'.";
			throw e;
		}
		try {
			texture = Render::NewTextureFromBitmap(b, true, true, false);
		}
		catch (Render::NewTextureError& e) {
			// Add the filename to the error message, and re-throw.
			e << "  filename: " << filename;
			throw e;
		}
		delete b;

		// Update the radius.
		Radius = sqrtf(Width * Width * 0.25f + Height * Height);	//xxxxxxx
	}

	~GStampImp() {
		delete texture;
	}

	float	GetHeight() const { return Height; }
	float	GetWidth() const { return Width; }
	Render::Texture*	GetTexture() const { return texture; }
	
	void	Render(ViewState& s, int ClipHint)
	// Draw the GStamp.
	{
		// Once per frame, recompute the offset vec3s.
		if (s.FrameNumber != LastFrameNumber) {
//			vert[0].SetU(0);	vert[0].SetV(1);
//			vert[1].SetU(1);	vert[1].SetV(1);
//			vert[2].SetU(1);	vert[2].SetV(0);
//			vert[3].SetU(0);	vert[3].SetV(0);

			// The right vec3 is the cross product between the
			// view normal in obj. coordinates, and the object Y
			// axis.  Doing the math out results in the following
			// shortcut:
			Right = vec3(-s.ViewMatrix.GetColumn(2).Z(), 0, s.ViewMatrix.GetColumn(0).Z());
			Right.normalize();
			LastFrameNumber = s.FrameNumber;
		}

		// Draw.
		Render::SetTexture(texture);
		Render::EnableAlphaTest();

		Render::CommitRenderState();
		
		glBegin(GL_TRIANGLE_FAN);
		glColor3f(1, 1, 1);

		glTexCoord2fv(vert[0].GetUV());
		vec3	v(Right * (Width * -0.5f));
		glVertex3fv((const float*) v);
		
		glTexCoord2fv(vert[1].GetUV());
		v.SetX(-v.X());	v.SetZ(-v.Z());
		glVertex3fv((const float*) v);
		
		glTexCoord2fv(vert[2].GetUV());
		v.SetY(Height);
		glVertex3fv((const float*) v);
		
		glTexCoord2fv(vert[3].GetUV());
		v -= Right * Width;
		glVertex3fv((const float*) v);
		
		glEnd();

		Model::AddToTriangleCount(2);
		
		Render::DisableAlphaTest();
	}
	

private:
	Render::Texture*	texture;
	float	Height, Width;

	int	LastFrameNumber;
//	vec3	Corners[4];
//	vec3	Offset[4];
};


//int	GStamp::LastFrameNumber = -1;
//vec3	GStamp::Right(1, 0, 0);


// Initialization hooks.
static struct InitGStamp {
	InitGStamp() {
		GameLoop::AddInitFunction(Init);
	}
	static void	Init()
	{
		Model::AddGModelLoader("psd", LoadGStamp);
	}

	static GModel*	LoadGStamp(const char* filename)
	// Creates a GStamp using the given image file name, and returns it.
	{
		GStampImp*	g = new GStampImp(filename);

		return g;
	}
	
} InitGStamp;

