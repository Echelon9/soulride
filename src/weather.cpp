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
// weather.cpp	-thatcher 3/29/1998 Copyright Thatcher Ulrich

// Sky, fog, and falling snow code.


#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#include "ogl.hpp"
#include "weather.hpp"
#include "render.hpp"
#include "clip.hpp"
#include "utility.hpp"
#include "config.hpp"
#include "lua.hpp"
#include "psdread.hpp"
#include "model.hpp"



namespace Weather {
;


vec3	SunDirection(-2, -2, 1);


//
// skydome rendering
//


bool	OpenFlag = false;


float	FogDistance = 25000;


const int	POINTS_IN_CIRCLE = 12;	// Number of points around the dome, per row.
const int	ROWS = 15;
const float	DOME_RADIUS = 20000;
const float	DOME_FLATNESS = 0.3f;	// Factor to apply to vertical extent of dome, to make it flat-ish.
const float	DEGREES_PER_ROW = 5;


float	SkyColor[ROWS][4] = {
	{ 206 / 255.0f, 226 / 255.0f, 227 / 255.0f, 1.0 },
	{ 161 / 255.0f, 187 / 255.0f, 200 / 255.0f, 1.0 },
	{ 118 / 255.0f, 142 / 255.0f, 185 / 255.0f, 1.0 },
	{  72 / 255.0f, 116 / 255.0f, 161 / 255.0f, 1.0 },
	{  66 / 255.0f, 101 / 255.0f, 155 / 255.0f, 1.0 },
};

vec3	DomePoint[ROWS][POINTS_IN_CIRCLE];



static void	InitSkydomeGradient(const char* filename)
// Loads a small bitmap and pulls the skydome colors from it.
{
	bitmap32*	b = PSDRead::ReadImageData32(filename);
	if (b) {
		int	i;
		int	max = imin(ROWS, b->GetWidth());
		for (i = 0; i < max; i++) {
			unsigned char color[4];
			*(uint32 *)&color = b->GetData()[i];
			SkyColor[i][0] = color[0] / 255.0f;
			SkyColor[i][1] = color[1] / 255.0f;
			SkyColor[i][2] = color[2] / 255.0f;
			SkyColor[i][3] = 1.0;
		}
		// Repeat last value to fill up the SkyColor array, if there weren't enough colors in the bitmap.
		for ( ; i < ROWS; i++) {
			SkyColor[i][0] = SkyColor[max-1][0];
			SkyColor[i][1] = SkyColor[max-1][1];
			SkyColor[i][2] = SkyColor[max-1][2];
			SkyColor[i][3] = SkyColor[max-1][3];
		}

		delete b;
	}

	FogDistance = Config::GetFloatValue("FogDistance");
}


static void	InitFlakes();


/**
 * Gets the next word from Lua arg callstack, and uses it as a bitmap filename
 * for initializing the skydome gradient.
 */
static void
WeatherReset_lua()
{
	// Skydome.
	const char*	Filename = lua_getstring(lua_getparam(1));
	if (Filename == 0) Filename = "skydome.psd";
	
	InitSkydomeGradient(Filename);

	// Re-init snowflakes.
	InitFlakes();
}


void	RecalcSunDirection_lua()
{
	SunDirection = Geometry::Rotate(Config::GetFloat("SunPhi") * PI / 180, ZAxis, XAxis);
	SunDirection = Geometry::Rotate(Config::GetFloat("SunTheta") * PI / 180, YAxis, SunDirection);
	SunDirection.normalize();
	SunDirection = -SunDirection;
}


Render::Texture*	Cloud = NULL;


float*	GetFadeColor()
// Returns a pointer to an array of four floats, the rgba of the horizon fade color.
{
	return &SkyColor[0][0];
}


float	GetFadeDistance()
// Returns the distance, in meters, at which the fog should become a "wall of fog".
{
	return FogDistance;
}


/**
 * Init the data we need for skydome rendering.
 */
void
Open()
{
	int	i, r;

	lua_register("weather_reset", WeatherReset_lua);
	lua_register("weather_recalc_sun_direction", RecalcSunDirection_lua);

	RecalcSunDirection_lua();
//	SunDirection.normalize();
	
//	InitSkydomeGradient("skydome.psd);

	lua_dostring("weather_reset('skydome.psd')");
	
	for (r = 0; r < ROWS; r++) {
		float	Elevation = (PI / 180 * r) * DEGREES_PER_ROW;
		float	Y = sinf(Elevation) * DOME_RADIUS * DOME_FLATNESS;
		float	Radius = DOME_RADIUS * cosf(Elevation);

		for (i = 0; i < POINTS_IN_CIRCLE; i++) {
			float theta = (2 * PI / POINTS_IN_CIRCLE) * (i + r * 0.5f);
			DomePoint[r][i] = vec3(cosf(theta) * Radius, Y, -sinf(theta) * Radius);
		}
	}

	OpenFlag = true;
}


void	Close()
// Free anything that needs it.
{
	OpenFlag = false;
}


/**
 * Reload gradients and whatnot.
 */
void
Reset()
{
	lua_dostring("weather_reset('skydome.psd')");
}


#if 0


void	GetParameters(Parameters* p)
// Fills the given structure with the current weather parameters.
{
	int	i;
	if (ROWS != 15) {
		Error e; e << "internal error in Weather::GetParameters";
		throw e;
	}
	
	for (i = 0; i < ROWS; i++) {
		p->SkydomeGradient[i][0] = SkyColor[i][0] * 255.0;
		p->SkydomeGradient[i][1] = SkyColor[i][1] * 255.0;
		p->SkydomeGradient[i][2] = SkyColor[i][2] * 255.0;
	}

	p->Clouds = Config::GetBool("Clouds");
	p->Snowfall = Config::GetBool("Snowfall");
	p->SnowDensity = Config::GetFloat("SnowDensity");

	Surface::GetShadeTable(p->ShadeTable);
}


void	SetParameters(const Parameters& p)
// Sets the current weather according to the given parameters.
{
	int	i;
	if (ROWS != 15) {
		Error e; e << "internal error in Weather::SetParameters";
		throw e;
	}
	
	for (i = 0; i < ROWS; i++) {
		SkyColor[i][0] = p->SkydomeGradient[i][0] / 255.0;
		SkyColor[i][1] = p->SkydomeGradient[i][1] / 255.0;
		SkyColor[i][2] = p->SkydomeGradient[i][2] / 255.0;
		SkyColor[i][3] = 1.0;
	}

	Config::SetBool("Clouds", p->Clouds);
	Config::SetBool("Snowfall", p->Snowfall);
	Config::SetFloat("SnowDensity", p->SnowDensity);

	Surface::SetShadeTable(p->ShadeTable);
}


#endif // 0


const vec3&	GetSunDirection()
// Returns the direction from the sun towards the earth.
{
	return SunDirection;
}


static void	FlakesUpdate(const UpdateState& u);


float	CloudUVOffset[2][2] = { { 0, 0 }, { 0, 0 } };
float	CloudSpeed[2][2] = { { 0, 0 }, { 0, 0 } };	// Defaults get overridden by config vars.
float	CloudUVRepeat[2] = { 3, 4 };	// Defaults get overridden by config var.
const float	CLOUD_XZ = 50000;


void	Update(const UpdateState& u)
// Animate the clouds, and make the snow fall.
{
	// Snow fall.
	FlakesUpdate(u);

	// Get cloud parameters from config variables.
	CloudUVRepeat[0] = Config::GetFloatValue("Cloud0UVRepeat");
	CloudUVRepeat[1] = Config::GetFloatValue("Cloud1UVRepeat");
	CloudSpeed[0][0] = Config::GetFloatValue("Cloud0XSpeed") * CloudUVRepeat[0] / CLOUD_XZ;
	CloudSpeed[0][1] = Config::GetFloatValue("Cloud0ZSpeed") * CloudUVRepeat[0] / CLOUD_XZ;
	CloudSpeed[1][0] = Config::GetFloatValue("Cloud1XSpeed") * CloudUVRepeat[1] / CLOUD_XZ;
	CloudSpeed[1][1] = Config::GetFloatValue("Cloud1ZSpeed") * CloudUVRepeat[1] / CLOUD_XZ;
	
	// Move the clouds.  Wrap coords to keep within [0, 1].
	for (int pass = 0; pass < 2; pass++) {
		for (int i = 0; i < 2; i++) {
			CloudUVOffset[pass][i] += CloudSpeed[pass][i] * u.DeltaT * GameLoop::GetSpeedScale();
			if (CloudUVOffset[pass][i] < 0) CloudUVOffset[pass][i] += 1;
			if (CloudUVOffset[pass][i] > 1) CloudUVOffset[pass][i] -= 1;
		}
	}
}


const int	CLOUD_GRID = 10;
const float	CLOUD_Y_MAX[2] = { 4000, 2000 };


static void	CloudVert(int i, int j, float yoffset, int pass)
{
	vec3	v = vec3(float(i) / (CLOUD_GRID-1) * 2 - 1, 0, float(j) / (CLOUD_GRID-1) * 2 - 1);
	float	d = v * v;
	v.SetY(CLOUD_Y_MAX[pass] * (1 - d) + yoffset);
	v.SetX(v.X() * 0.5f * CLOUD_XZ);
	v.SetZ(v.Z() * 0.5f * CLOUD_XZ);

	glTexCoord2f(CloudUVRepeat[pass] * (float(i) / (CLOUD_GRID-1)) + CloudUVOffset[pass][0], CloudUVRepeat[pass] * (float(j) / (CLOUD_GRID-1)) + CloudUVOffset[pass][1]);
	glVertex3fv((float*) &v);
}


void	RenderBackdrop(const ViewState& s)
// Draw the skydome and clouds.
{
	if (!OpenFlag) Open();

	// Re-load the skydome color gradient on F4.
	if (Config::GetBoolValue("F4Pressed")) {
		InitSkydomeGradient("skydome.psd");
	}
	
	int	i, j, r;
	vec3	v;
	vec3	offset = s.Viewpoint;
	offset.SetY(offset.Y() * 0.5f);

	// Turn off the zbuffer.
	Render::DisableZBuffer();
	glDepthMask(GL_FALSE);	// No z-writes.
	
	// No texture.
	Render::SetTexture(NULL);
	Render::CommitRenderState();
	
	// Draw a fan covering the bottom of the dome.
	glColor3fv(SkyColor[0]);
	glBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < POINTS_IN_CIRCLE; i++) {
		v = DomePoint[0][i];	v += offset;
		glVertex3fv((float*) &v);
	}
	glEnd();

	// Draw strips connecting the rows.
	for (r = 0; r < ROWS - 1; r++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (i = 0; i < POINTS_IN_CIRCLE; i++) {
			glColor3fv(SkyColor[r]);
			v = DomePoint[r][i];	v += offset;
			glVertex3fv((float*) &v);

			glColor3fv(SkyColor[r+1]);
			v = DomePoint[r+1][i];	v += offset;
			glVertex3fv((float*) &v);
		}
		// Join the strip back up at the beginning.
		glColor3fv(SkyColor[r]);
		v = DomePoint[r][0];	v += offset;
		glVertex3fv((float*) &v);

		glColor3fv(SkyColor[r+1]);
		v = DomePoint[r+1][0];	v += offset;
		glVertex3fv((float*) &v);
				
		glEnd();
	}

	// Draw a fan covering the top of the dome.
	glColor3fv(SkyColor[ROWS-1]);
	glBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < POINTS_IN_CIRCLE; i++) {
		v = DomePoint[ROWS-1][i];	v += offset;
		glVertex3fv((float*) &v);
	}
	glEnd();

//	glDisable(GL_BLEND);
	
	// Draw some clouds.
	int	Clouds = Config::GetIntValue("Clouds");
	if (Clouds) {
		if (Cloud == NULL) {
			Cloud = Render::NewTexture("cloud0.psd", true, true, true);
		}
		
		// Set up blending mode.
		Render::SetTexture(Cloud);
		Render::DisableAlphaTest();
		Render::CommitRenderState();
		glColor3f(1, 1, 1);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		if (Config::GetBoolValue("Fog")) glEnable(GL_FOG);

#ifdef NOT
		glBegin(GL_TRIANGLE_FAN);
		
		static const float	CLOUD_Y_MAX = 4000;
		static const float	CLOUD_Y_MIN = 0;
		static const float	CLOUD_XZ = 25000;
		static const float	UV_MAX = 3;
		static const int	WEDGE_COUNT = 8;
//		offset.SetX(32767);
//		offset.SetZ(32767);
		offset.SetX(0);
		offset.SetZ(0);
		
		// A fan, with a vertex at the center.
		v = vec3(0, CLOUD_Y_MAX, 0) + offset;
		glTexCoord2f(UV_MAX / 2, UV_MAX / 2);
		glVertex3fv((float*) & v);
		
		for (i = 0; i <= WEDGE_COUNT; i++) {
			float	c = cosf(2 * PI * i / WEDGE_COUNT);
			float	s = -sinf(2 * PI * i / WEDGE_COUNT);
			v = vec3(CLOUD_XZ * c, CLOUD_Y_MIN, CLOUD_XZ * s) + offset;
			glTexCoord2f(UV_MAX / 2 * (c + 1), UV_MAX / 2 * (s + 1));
			glVertex3fv((float*) &v);
		}
		
		glEnd();
#else

		float	oy = offset.Y();

		// Cloud layer 1.
		for (j = 0; j < CLOUD_GRID-1; j++) {
			glBegin(GL_TRIANGLE_STRIP);

			CloudVert(0, j, oy, 0);
			for (i = 0; i < CLOUD_GRID-1; i++) {
				CloudVert(i, j+1, oy, 0);
				CloudVert(i+1, j, oy, 0);
			}
			CloudVert(i, j+1, oy, 0);
			
			glEnd();
		}

		// Cloud layer 2.
		if (Clouds > 1) {
			for (j = 0; j < CLOUD_GRID-1; j++) {
				glBegin(GL_TRIANGLE_STRIP);
				
				CloudVert(0, j, oy, 1);
				for (i = 0; i < CLOUD_GRID-1; i++) {
					CloudVert(i, j+1, oy, 1);
					CloudVert(i+1, j, oy, 1);
				}
				CloudVert(i, j+1, oy, 1);
				
				glEnd();
			}
		}
		
#endif //NOT
		
		glDisable(GL_BLEND);
	}
		
	// Turn the zbuffer back on.
	Render::EnableZBuffer();
	glDepthMask(GL_TRUE);	// Enable z-writes.
}


//
// Falling snow.
//


bool	FlakesInited = false;
float	SnowDensity = 0.6f;	// Particles per cubic meter.  Exaggeratedly small.
vec3	SnowVelocity(0, -1.0, 0);


//const float	SNOWFLAKE_SIZE = 0.04;	// Exaggeratedly large.
const float	WEATHER_BLOCK_SIZE = 14;
vec3	PreviousViewpoint(0, 0, 0);
Render::Texture*	SnowflakeTexture = NULL;
float	SnowflakeWidth = 0.04f, SnowflakeHeight = 0.04f;


struct FlakeInfo {
	uint32	x, y, z;
} *Flake = NULL;
int	FlakeCount;

int	DeltaTicks = 0;


void	InitFlakes()
// Set up a set of random flakes within the weather block.
{
	SnowDensity = Config::GetFloatValue("SnowDensity");
	
	FlakeCount = (int) (SnowDensity * WEATHER_BLOCK_SIZE * WEATHER_BLOCK_SIZE * WEATHER_BLOCK_SIZE);
	if (FlakeCount > 10000) FlakeCount = 10000;	// Sanity limit.
	
	if (Flake) delete [] Flake;
	if (FlakeCount > 0) {
		Flake = new FlakeInfo[FlakeCount];
	} else {
		Flake = NULL;
		return;
	}
	
	int	i;
	// Initialize flakes.
	for (i = 0; i < FlakeCount; i++) {
		Flake[i].x = rand() << 17;
		Flake[i].y = rand() << 17;
		Flake[i].z = rand() << 17;
	}

	// Load flake texture.
	bitmap32*	b = PSDRead::ReadImageData32("snowflake.psd", &SnowflakeWidth, &SnowflakeHeight);
	if (b == NULL) {
		Error e; e << "Can't open snowflake.psd";
		throw e;
	}
	
	SnowflakeWidth *= 0.01f;
	SnowflakeHeight *= 0.01f;
	if (SnowflakeTexture) {
		SnowflakeTexture->ReplaceImage(b);
	} else {
		SnowflakeTexture = Render::NewTextureFromBitmap(b, true, true, false);
	}
	delete b;
}


void	FlakesUpdate(const UpdateState& u)
// Defer the actual update until the Render() call.  But, keep track of the
// amount of time which has passed.
{
	DeltaTicks += u.DeltaTicks;
}


  void	RenderOverlay(const ViewState& s, bool flakes_have_moved)
// Update and then draw snowflakes.
{
	if (FlakesInited == false || Config::GetBoolValue("F4Pressed")) {
		InitFlakes();
		FlakesInited = true;
	}

	if (Flake == NULL) return;
	if (Config::GetBoolValue("Snowfall") == false) return;	
	
	// Bjorn added this
	const vec3&	Viewpoint = s.Viewpoint;

	if (flakes_have_moved){
	  
	  float	dt = DeltaTicks / 1000.0f * GameLoop::GetSpeedScale();
	  DeltaTicks = 0;
	  
	  // Calculate the viewpoint shift.  The flakes shift by an opposite amount.
	  //const vec3&	Viewpoint = s.Viewpoint;
	  vec3	Shift = -(Viewpoint - PreviousViewpoint);
	  PreviousViewpoint = Viewpoint;
	  
	  // Calculate the shift due to the motion of the snowflakes.
	  Shift += SnowVelocity * dt;
	  
	  // Calculate the shift in terms of weather-block fixed-point coords.
	  Shift *= 1.0f / WEATHER_BLOCK_SIZE;
	  
	  uint32	dx, dy, dz;
	  dx = int((Shift.X() - floor(Shift.X())) * 65536.0) << 16;
	  dy = int((Shift.Y() - floor(Shift.Y())) * 65536.0) << 16;
	  dz = int((Shift.Z() - floor(Shift.Z())) * 65536.0) << 16;
	  
	  // Move the flakes.  Rely on integer wrap-around arithmetic to keep the flakes within the weather block.
	  int	i;
	  for (i = 0; i < FlakeCount; i++) {
	    Flake[i].x += dx;	// Could throw in some turbulence or something?  From a table, indexed on middle bits of coordinates?  Scaling to dt is an issue.
	    Flake[i].y += dy;
	    Flake[i].z += dz;
	  }
	} // end of flakes moved 
	
	// Now draw 'em.
	
	// Put the viewpoint right in the middle of the weather block.
	glPushMatrix();
	glTranslatef(
		     Viewpoint.X() - WEATHER_BLOCK_SIZE * 0.5f,
		     Viewpoint.Y() - WEATHER_BLOCK_SIZE * 0.5f,
		     Viewpoint.Z() - WEATHER_BLOCK_SIZE * 0.5f);
	glColor3f(1, 1, 1);
	
	Render::SetTexture(SnowflakeTexture);
	Render::DisableAlphaTest();
	Render::EnableZBuffer();
	Render::CommitRenderState();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	vec3	Right, Up;
	s.ViewMatrix.ApplyInverseRotation(&Right, vec3(-SnowflakeWidth, 0, 0));
	s.ViewMatrix.ApplyInverseRotation(&Up, vec3(0, SnowflakeHeight, 0));
	
	// Draw each flake.
	glBegin(GL_QUADS);
	vec3	v;
	for (int i = 0; i < FlakeCount; i++) {
//		four verts of the billboard;	// (x >> 16) * (WEATHER_BLOCK_SIZE / 65536.0);
		v.SetX((Flake[i].x >> 16) * (WEATHER_BLOCK_SIZE / 65536.0f));
		v.SetY((Flake[i].y >> 16) * (WEATHER_BLOCK_SIZE / 65536.0f));
		v.SetZ((Flake[i].z >> 16) * (WEATHER_BLOCK_SIZE / 65536.0f));
		glTexCoord2f(0, 1);
		glVertex3fv((const float*) v);
		v += Right;
		glTexCoord2f(1, 1);
		glVertex3fv((const float*) v);
		v += Up;
		glTexCoord2f(1, 0);
		glVertex3fv((const float*) v);
		v -= Right;
		glTexCoord2f(0, 0);
		glVertex3fv((const float*) v);
	}
	glEnd();

	Model::AddToTriangleCount(FlakeCount);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	
	// Restore viewpoint.
	glPopMatrix();
}



};	// end namespace Weather

