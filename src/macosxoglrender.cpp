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
// oglrender.cpp	-thatcher 3/1/1999 Copyright Slingshot Game Technology

// Rendering wrapper using OpenGL for rasterization.

// Bjorn Leffler 31/8/2003:
// There is a special file for OpenGL rendering for Mac OS X.
// This is because The interfaces are different compared to X windows (Linux)
// The windows part of the code has been removed.

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif

#include <SDL.h>
// #include <GL/glx.h>
// #include <X11/extensions/xf86vmode.h>
#include "macosxmain.hpp"

#include <stdio.h>
#include <math.h>

#include <sys/stat.h>
#include "ogl.hpp"


#include "utility.hpp"
#include "render.hpp"
#include "main.hpp"
#include "error.hpp"
#include "psdread.hpp"
#include "timer.hpp"
#include "config.hpp"
#include "console.hpp"


namespace Render {
;


bool	IsOpen = false;
bool	Is3DOpen = false;
bool	Fullscreen = true;
int	CurrentDriver = 0;
int	CurrentMode = 2;

int windowWidth = 1024, windowHeight = 768;
int windowBPP = 16;

class OGLTexture;


float	BlitUVOffset = 0;


// Misc functions.
bool	GetFullscreen() { return Fullscreen; }


void	OpenOGL();
void	CloseOGL();
void	FindDesiredPixelFormat();
void	ReadDriverList(const char* filename);
void	WriteDriverList(const char* filename);


const char*	DriverListFilename = "ogldrivers.txt";


void	Open()
// Create the OpenGL interface, and query what modes & drivers are available.
{
	if (IsOpen == false) {
		if (Config::GetValue("Fullscreen")) {
			Fullscreen = Config::GetBool("Fullscreen");
		}
		CurrentDriver = Config::GetInt("OGLDriverIndex");
		CurrentMode = Config::GetInt("OGLModeIndex");

		ReadDriverList(DriverListFilename);
		
		IsOpen = true;
	}
}


void	Close()
// Close the OpenGL interface.
{
	if (IsOpen) {

		IsOpen = false;
	}
}

void	Close3D()
// Close 3D stuff.
{
	if (Is3DOpen) {
		CloseOGL();
		Is3DOpen = false;
	}
}

	
void	RefreshWindow()
// Reset our relationship to the screen, in case our window got moved or something.
{
//	// Something involving glViewport()
//	POINT	ClientOnPrimary;
//	ClientToScreen(Main::GetMainWindow(), &ClientOnPrimary);
//
//	glViewport(0, 0, ClientOnPrimary.x, ClientOnPrimary.y);
}


void	MakeUIVisible()
// This function gets called when the app needs to allow access to the Windows UI.
{
	if (Is3DOpen == false) return;

	// xxxx some wgl function ???
	
#ifdef LINUX
#else // not LINUX
// windows specific code removed
#endif // not LINUX
}

bool	IsBlank(char* buf)
// Returns true if the given string is nothing but whitespace.
{
	char	c;
	while (c = *buf++) {
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return false;
	}
	return true;
}


void	ReadDriverList(const char* filename)
// Reads the list of drivers from the specified file.
{
// windows specific code removed
}


void	WriteDriverList(const char* filename)
// Writes the list of OpenGL driver DLLs to the specified filename.
{
// windows specific code removed
}


// *** Note that the MacOSX version will overide these ***
// *** modes through the graphical user interface      ***

// Maintain a list of available modes, and a currently selected mode.
const int	MAX_MODES = 15;
int	ModeCount = MAX_MODES;
ModeInfo	Mode[MAX_MODES] = {
//	{ 160, 120, 16, false },
	{ 320, 240, 16, false },
	{ 640, 480, 16, false },
	{ 800, 600, 16, false },
	{ 1024, 768, 16, false },
	{ 1280, 1024, 16, false },
	{ 320, 240, 32, false },
	{ 640, 480, 32, false },
	{ 800, 600, 32, false },
	{ 1024, 768, 32, false },
	{ 1280, 1024, 32, false },
	{ 1366, 768, 32, false },
	{ 1440, 900, 32, false },
	{ 1600, 900, 32, false },
	{ 1920, 1080, 32, false },
	{ 3840, 2160, 32, false },
};

#ifdef MACOSX

// MacOSX Set functions, changing window/screen options from GUI 

void SetWindowWidth(int width){
  windowWidth = width;
}

void SetWindowHeight(int height){
  windowHeight = height;
}

void SetWindowDepth(int depth){
  windowBPP = depth;
}

void SetWindowFullscreen(bool fullscreen){
  Fullscreen = fullscreen;
}

#endif

int	GetDeviceCount() { return 1; }

/**
 * Describe render device
 *
 * @param device Index of device
 *
 * @return String describing the specified device
 */
const char*
GetDeviceDescription(int device)
{
	return "---";
}

int	GetCurrentDevice() { return 0; }


int	GetDriverCount(int device)
// Returns the number of drivers under the given device.
{
  return 1;
}

/**
 * Get name of driver
 *
 * @param index Index of device
 *
 * @return String name of the specified driver.
 */
const char*
GetDriverName(int index)
{
  return "OpenGL";
}

/**
 * Returns extra information about the specified driver
 *
 * @param index Index of device
 *
 * @return String of extra information
 */
const char*
GetDriverComment(int index)
{
  return "---";
}

	
int	GetCurrentDriver(int device)
// Returns the index of the driver currently selected under the specified device.
{
	return CurrentDriver;
}


void	DeleteDriver(int index)
// Deletes the specified driver from our list.
{
// Windows specific code removed
}


int	AddNewDriver()
// Adds a new driver and returns its index.  Returns -1 if a new driver
// can't be added.
{
// Windows specific code removed
}


void	SetDriverInfo(int index, const char* Name, const char* Comment)
// Sets the info for the specified driver.
{
// Windows specific code removed
}

#ifndef MACOSX
int	GetWindowWidth() { return Mode[CurrentMode].Width; }
int	GetWindowHeight() { 
  return Mode[CurrentMode].Height; 
}
#else
int	GetWindowWidth() { return windowWidth; }
int	GetWindowHeight() { return windowHeight; }
int GetWindowDepth() { return windowBPP; }
#endif

int	GetModeCount(int device) { return ModeCount; }
int	GetCurrentMode(int device) { return CurrentMode; }
void	GetModeInfo(int device, ModeInfo* result, int index) { *result = Mode[index]; }


void	SetDisplayOptions(int device, int driver, int mode, bool fullscreen)
// Sets the given options as current.
{
	CurrentDriver = driver;
	CurrentMode = mode;
	Fullscreen = fullscreen;

	Config::SetInt("OGLDriverIndex", CurrentDriver);
	Config::ExportValue("OGLDriverIndex");
	Config::SetInt("OGLModeIndex", CurrentMode);
	Config::ExportValue("OGLModeIndex");
	Config::SetBool("Fullscreen", Fullscreen);
	Config::ExportValue("Fullscreen");
}


int	GLMagFilter = GL_LINEAR;
int	GLMinFilterMIPMap = GL_LINEAR_MIPMAP_LINEAR;
int	GLTextureEnvMode = GL_MODULATE;
int	GLClampMode = GL_CLAMP;


void	Open3D()
// Set video mode, etc, & get ready for rendering.
{
	if (!Is3DOpen) {
// windows specific code removed
		OpenOGL();
		
		Is3DOpen = true;

		if (Config::GetBool("SoftwareRenderingOptions")) {
			GLMagFilter = GL_NEAREST;
			GLMinFilterMIPMap = GL_NEAREST_MIPMAP_NEAREST;
			GLTextureEnvMode = GL_REPLACE;
		}

		if (OGL::GetEdgeClampEnabled()) {
			GLClampMode = GL_CLAMP_TO_EDGE_EXT;
		} else {
			GLClampMode = GL_CLAMP;
		}
	}
}


const int	DesiredZDepth = 16;
const int	DesiredColorDepth = 16;

  // X windows specific code removed
  // See Linux code if you're interested...

  // windows specific code removed too

void	CheckOGLError(const char* FuncName);


HGLRC	RenderingContext = 0;


void	OpenOGL()
// Create a rendering context etc, that will let us start rendering.
{
	// First let's try linking in the DLL.
	const char*	lib = GetDriverName(CurrentDriver);
	const char*	override = Config::GetValue("OGLLibOverride");
	if (override) lib = override;
	
	if (OGL::Bind(lib) != 0) {
		RenderError e; e << "Couldn't bind OpenGL library '" << lib << "'";
		throw e;
	}


#ifdef LINUX

	const SDL_VideoInfo* info = NULL;
	info = SDL_GetVideoInfo();
	if (!info) {
		Error e; e << "SDL_GetVideoInfo() failed.";
		throw e;
	}

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	ModeInfo&	m = Mode[CurrentMode];
	int	bpp = info->vfmt->BitsPerPixel;
	int	flags = SDL_OPENGL | (Fullscreen ? SDL_FULLSCREEN : 0);

	// Set the video mode.
	if (SDL_SetVideoMode(m.Width, m.Height, bpp, flags) == 0) {
		Error e; e << "SDL_SetVideoMode() failed.";
		throw e;
	}


#else // not LINUX
// windows specific code removed
#endif // not LINUX

	//
	// End window-system stuff, start OpenGL standard stuff.
	//

	glDrawBuffer(GL_BACK);
	
	// If possible, don't do vertical sync.
	OGL::SwapIntervalEXT(0);
	
	// Make sure extensions are loaded.
	OGL::BindExtensions();
	
//	glEnable(GL_DITHER);
//	glDisable(GL_DITHER);//xxx
	glEnable(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (float) GL_NEAREST);	// Would enable MIP-mapping here.
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (float) GL_LINEAR);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (float) GL_NEAREST);
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (float) GL_DECAL);

//	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);  /* Superimpose smaller Z values over larger ones */
	glAlphaFunc(GL_GEQUAL, 0.5);
//	glShadeModel(GL_SMOOTH); /* Smooth shading */
	glPixelStoref(GL_UNPACK_ALIGNMENT, (float) 4);
	glPixelStoref(GL_PACK_ALIGNMENT, (float) 4);
//	glAlphaFunc(GL_GEQUAL, 0.07f);
//	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST); /* Z buffering is go... */

//	// Back-face culling.
//	glFrontFace(GL_CCW);
//	glEnable(GL_CULL_FACE);
	
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST|GL_FASTEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glViewport(0, 0, GetWindowWidth(), GetWindowHeight()); // Bjorn
	
	CheckOGLError("OpenOGL()");
}


void	CloseOGL()
// Close the OpenGL rendering context, and do other cleanup.
{
// windows specific code removed
}


HGLRC	GetMainContext()
// Return the main rendering context.
{
	return RenderingContext;
}


void	ShowFrame()
// Show what's in the rendering frame.
{
#ifdef LINUX
	SDL_GL_SwapBuffers( );
#else // not LINUX
// windows specific code removed
#endif // not LINUX

	CheckOGLError("ShowFrame()");

	// Update the value of BlitUVOffset.
	BlitUVOffset = Config::GetFloatValue("BlitUVOffset");
}


void	ClearFrame()
// Clear what's in the rendering frame.
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CheckOGLError("ClearFrame()");
}


void	ClearZBuffer()
// Clear the z buffer.
{
	glClear(GL_DEPTH_BUFFER_BIT);
}


//
// texture stuff
//


uint32*	MIPScratchA = NULL;
uint32*	MIPScratchB = NULL;
int	MIPScratchTexels = 0;


struct OGLTexture;
//OGLTexture*	TextureList = NULL;
int	NextOGLTextureName = 1;


struct OGLTexture : public Texture {
	GLuint	OGLTextureName;

	bool	MIPMapped;
	
//	// For managing a texture list.
//	OGLTexture*	Next;
//	OGLTexture*	Previous;
	

// methods:	
	OGLTexture(const char* Filename, bool NeedAlpha, bool MakeMIPMaps, bool Tile, bool AlphaFadedMIPMaps)
	// Constructor.  Loads the image from the file and initializes the texture with it.
	{
		OGLTextureName = 0;
		
		// Try to load the bitmap.
		bitmap32*	b = PSDRead::ReadImageData32(Filename);
		if (b == NULL) {
			NewTextureError e; e << "Can't load bitmap named '" << Filename << "'.";
			throw e;
		}

		Init(b, NeedAlpha, MakeMIPMaps, Tile, AlphaFadedMIPMaps, Filename);
		
		delete b;
	}
	
	OGLTexture(bitmap32* b, bool NeedAlpha, bool MakeMIPMaps, bool Tile, bool AlphaFadedMIPMaps)
	// Constructor.  Init using given bitmap image.
	{
		OGLTextureName = 0;

		Init(b, NeedAlpha, MakeMIPMaps, Tile, AlphaFadedMIPMaps, "");
	}

	void	Init(bitmap32* b, bool NeedAlpha, bool MakeMIPMaps, bool Tile, bool AlphaFadedMIPMaps, const char* Filename)
	// Finish constructing, using the given bitmap as the image data.
	// Set NeedAlpha to false if the texture doesn't need an alpha channel.
	// Set MakeMIPMaps true if you want the texture to be MIP-mapped.
	// The Tile parameter controls whether the texture repeats (tiles) or clamps at the edges.
	//
	// The Filename parameter is strictly for debugging purposes.
	// Pass in NULL or "" if you don't have a filename handy.
	{
		if (Config::GetBoolValue("MIPMapping") == false) MakeMIPMaps = false;
		
		MIPMapped = MakeMIPMaps;

		// Don't create any textures with dimensions > 256.  Otherwise
		// we'll crash on old Voodoo cards.
		while (b->GetHeight() >= 512 || b->GetWidth() > 256) {
			int	sw = b->GetWidth();
			int	sh = b->GetHeight();

			Console::Printf("OGLTexture::Init(): shrinking bitmap '%s' from (%d x %d)\n", Filename ? Filename : "", sw, sh);

			int	size = (sw >> 1) * (sh >> 1);
			if (size) CreateMIPScratch(size);

			Geometry::HalfScaleFilterBox(sw, sh, b->GetData(), MIPScratchA);

			// HACK: overwrite b's data.  Potentially dangerous, if 
			memcpy(b->GetData(), MIPScratchA, size * sizeof(uint32));
			b->SetWidth(sw >> 1);
			b->SetHeight(sh >> 1);
		}

		// Compute log2(height) here to see if bitmap includes pre-built mip-maps, and how many.
		// If there are no mip-maps, the height will be a power-of-two.  If there are mip-maps,
		// they'll be stacked below the main image, and the height value will indicate how many
		// there are.

		int	HeightBits = 0;
		int	i = b->GetHeight();
		for (;;) {
			i >>= 1;
			if (i == 0) break;
			HeightBits++;
		}

		Height = 1 << HeightBits;

		int	PremadeMIPMapCount = 0;
		i = b->GetHeight() << 1;
		while (Height & i) {
			PremadeMIPMapCount++;
			i <<= 1;
		}
		
		Width = b->GetWidth();
//		Height = b->GetHeight();
		
		TexelCount = Width * Height;

		glGenTextures(1, &OGLTextureName);
		glBindTexture(GL_TEXTURE_2D, OGLTextureName);

		if (Tile) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     (float) GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     (float) GL_REPEAT);
		} else {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     (float) GLClampMode);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     (float) GLClampMode);
		}
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (float) GLMagFilter /* GL_LINEAR */);
		if (MakeMIPMaps) {
//			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (float) GL_LINEAR_MIPMAP_LINEAR);	// GL_NEAREST_MIPMAP_LINEAR is lerp across nearest texel in each mip; GL_LINEAR_MIPMAP_NEAREST is bilerp within the nearest mip.
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (float) GLMinFilterMIPMap /* GL_LINEAR_MIPMAP_LINEAR */);	// GL_NEAREST_MIPMAP_LINEAR is lerp across nearest texel in each mip; GL_LINEAR_MIPMAP_NEAREST is bilerp within the nearest mip.
		} else {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (float) GL_NEAREST);
		}
//		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (float) GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (float) GLTextureEnvMode);

		GLenum	InternalFormat = GL_RGB;
		if (NeedAlpha) {
			InternalFormat = GL_RGBA4;	// or RGB5_A1, for cutouts...
		}

		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*) b->GetData());
		
		if (MakeMIPMaps) {
			int	sw = Width;
			int	sh = Height;

			int	size = (sw >> 1) * (sh >> 1);
			if (size) CreateMIPScratch(size);
			
			int	i;
			uint32*	PremadeData = b->GetData() + Width * Height;
			uint32*	dest = MIPScratchA;
			uint32*	src = b->GetData();
			for (i = 0; ; i++) {
				int	dw = sw >> 1;
				int	dh = sh >> 1;
				if (dw == 0) dw = 1;
				if (dh == 0) dh = 1;
				
				TexelCount += dw * dh;

				if (i & 1) {
					dest = MIPScratchB;
				} else {
					dest = MIPScratchA;
				}

				if (PremadeMIPMapCount) {
					// Copy from bitmap source into dest[], rather than filtering.
					for (int row = 0; row < dh; row++) {
						memcpy(&dest[row * dw], PremadeData, dw * sizeof(uint32));
						PremadeData += Width;
					}
					PremadeMIPMapCount--;
					
				} else if (AlphaFadedMIPMaps) {
					Geometry::HalfScaleFilterSincScaleAlpha(sw, sh, src, dest, fclamp(0, 1 - (i / 3.0), 1));
				} else {
//x					if (i < 2) {
//						Geometry::HalfScaleFilterSinc(sw, sh, src, dest);
//					} else {
						Geometry::HalfScaleFilterBox(sw, sh, src, dest);
//					}
				}
				glTexImage2D(GL_TEXTURE_2D, i+1, InternalFormat, dw, dh, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*) dest);

				// Set up for next iteration.
				sw = dw;
				sh = dh;
				src = dest;

				if (dw == 1 && dh == 1) break;
			}
		}
		
		// Add to the texture list.
//		Next = TextureList;
//		Previous = NULL;
//		TextureList = this;
	}
	
	virtual ~OGLTexture()
	// Destructor.
	{
		glDeleteTextures(1, &OGLTextureName);
		
//		// Unlink from the texture list.
//		if (Previous) {
//			Previous->Next = Next;
//		} else {
//			TextureList = Next;
//		}
//		if (Next) {
//			Next->Previous = Previous;
//		}
	}
	
	void	ReplaceImage(bitmap32* b)
	// Change the image data of this texture to match the data in the given bitmap.
	{
		glBindTexture(GL_TEXTURE_2D, OGLTextureName);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, b->GetWidth(), b->GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, (void*) b->GetData());

		if (MIPMapped) {
			int	sw = Width;
			int	sh = Height;

			int	size = (sw >> 1) * (sh >> 1);
			if (size) CreateMIPScratch(size);
			
			int	i;
			uint32*	dest = MIPScratchA;
			uint32*	src = b->GetData();
			for (i = 0; ; i++) {
				int	dw = sw >> 1;
				int	dh = sh >> 1;
				if (dw == 0) dw = 1;
				if (dh == 0) dh = 1;

				if (i & 1) {
					dest = MIPScratchB;
				} else {
					dest = MIPScratchA;
				}

//				Geometry::HalfScaleFilterSinc(sw, sh, src, dest);
				Geometry::HalfScaleFilterBox(sw, sh, src, dest);
				glTexSubImage2D(GL_TEXTURE_2D, i+1, 0, 0, dw, dh, GL_RGBA, GL_UNSIGNED_BYTE, (void*) dest);

				// Set up for next iteration.
				sw = dw;
				sh = dh;
				src = dest;

				if (dw == 1 && dh == 1) break;
			}
		}
	}

	void	CreateMIPScratch(int texels)
	// Makes sure that MIPScratchA[texels] is a valid scratch array,
	// to be used for building MIP-maps.
	{
		if (texels > MIPScratchTexels || MIPScratchA == NULL) {
			if (MIPScratchA) {
				delete [] MIPScratchA;
				delete [] MIPScratchB;
			}			
			
			MIPScratchB = new uint32[texels >> 2];
			MIPScratchA = new uint32[texels];
			MIPScratchTexels = texels;
		}
	}


	void	MakeAlphaFadedMIPMaps(bitmap32* b)
	// Makes MIP-maps for this texture, but fades up the alpha so that the smaller
	// maps become increasingly transparent.
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (float) GL_LINEAR_MIPMAP_LINEAR);
		
		MIPMapped = true;
		
		int	sw = Width;
		int	sh = Height;
		
		int	size = (sw >> 1) * (sh >> 1);
		if (size) CreateMIPScratch(size);
		
		int	i;
		uint32*	dest = MIPScratchA;
		uint32*	src = b->GetData();
		for (i = 0; ; i++) {
			int	dw = sw >> 1;
			int	dh = sh >> 1;
			if (dw == 0) dw = 1;
			if (dh == 0) dh = 1;
			
			TexelCount += dw * dh;
			
			if (i & 1) {
				dest = MIPScratchB;
			} else {
				dest = MIPScratchA;
			}
			
			Geometry::HalfScaleFilterSinc(sw, sh, src, dest);
			GLenum	InternalFormat = GL_RGBA4;
			glTexImage2D(GL_TEXTURE_2D, i+1, InternalFormat, dw, dh, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*) dest);
			
			// Set up for next iteration.
			sw = dw;
			sh = dh;
			src = dest;
			
			if (dw == 1 && dh == 1) break;
		}
	}
};

	
Texture*	NewTexture(const char* Filename, bool NeedAlpha, bool MakeMIPMaps, bool Tile, bool AlphaFadedMIPMaps)
// Creates a new texture from the given bitmap file.
{
	OGLTexture*	t = new OGLTexture(Filename, NeedAlpha, MakeMIPMaps, Tile, AlphaFadedMIPMaps);
	return t;
}


Texture*	NewTextureFromBitmap(bitmap32* b, bool NeedAlpha, bool MakeMIPMaps, bool Tile, bool AlphaFadedMIPMaps)
// Creates a new texture from the pre-loaded bitmap.
{
	OGLTexture*	t = new OGLTexture(b, NeedAlpha, MakeMIPMaps, Tile, AlphaFadedMIPMaps);
	return t;
}


void	DeleteTexture(Texture* t)
// Deletes the texture from active use.
{
	delete t;
}


void	ClearTextures()
// Deletes all textures.
{
}


void	InvalidateTextures()
// Force all textures to be reloaded/rebuilt.
{
}


//
// Rendering state.
//


// Shadow OpenGL's render state vs. our logical render state, so we can minimize actual
// state changes sent to the driver.
struct RenderState {
	bool	AlphaTesting, ZBuffering;
	bool	LightmapBlending;
	OGLTexture*	Texture;
};
RenderState	ShadowState = {
	false, true, true, NULL
};
RenderState	DesiredState = {
	false, true, true, NULL
};


void	CommitRenderState()
// Makes sure that the desired render state has been communicated to the
// card.  Call this before drawing polygons.
{
	// Alpha testing.
	if (ShadowState.AlphaTesting != DesiredState.AlphaTesting) {
		if (DesiredState.AlphaTesting) {
			glEnable(GL_ALPHA_TEST);
		} else {
			glDisable(GL_ALPHA_TEST);
		}
		ShadowState.AlphaTesting = DesiredState.AlphaTesting;
	}

	// Z Buffer.
	if (ShadowState.ZBuffering != DesiredState.ZBuffering) {
		if (DesiredState.ZBuffering) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}
		ShadowState.ZBuffering = DesiredState.ZBuffering;
	}

	// Lightmap blending.
	if (ShadowState.LightmapBlending != DesiredState.LightmapBlending) {
		if (DesiredState.LightmapBlending) {
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
		ShadowState.LightmapBlending = DesiredState.LightmapBlending;
	}

	// Texture map.
	if (ShadowState.Texture != DesiredState.Texture) {
		OGLTexture*	t = DesiredState.Texture;	// Shorthand.
		if (t) {
			glBindTexture(GL_TEXTURE_2D, t->OGLTextureName);
			glEnable(GL_TEXTURE_2D);
		} else {
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		}
		ShadowState.Texture = DesiredState.Texture;
	}
}


//
// Drawing commands.
//


void	BeginFrame()
// Call before starting a new frame.
// Leaves the matrix mode in GL_MODELVIEW by default until EndFrame().
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	if (Config::GetBoolValue("Wireframe")) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


void	EndFrame()
// Call when finished with a frame.
{
  glFlush();
  
  CheckOGLError("EndFrame()");
}


void	MultMatrix(const matrix& m)
// Multiplies the current matrix stack top by m.
{
	// Copy to the 4x4 layout for OpenGL.
	float	mat[16];
	for (int col = 0; col < 4; col++) {
		for (int row = 0; row < 3; row++) {
			mat[col * 4 + row] = m.GetColumn(col).Get(row);
		}
		if (col < 3) {
			mat[col * 4 + 3] = 0;
		} else {
			mat[col * 4 + 3] = 1;
		}
	}
	// Apply to the current OpenGL matrix.
	glMultMatrixf(mat);
}


void	SetTexture(Texture* t)
// Sets the given texture to be used for texturing of subsequent triangles.
{
	OGLTexture*	o = static_cast<OGLTexture*>(t);
	DesiredState.Texture = o;
}


void	EnableZBuffer()
// Turns on the z-buffer.
{
	DesiredState.ZBuffering = true;
}


void	DisableZBuffer()
// Turns the z-buffer off.
{
	DesiredState.ZBuffering = false;
}


void	EnableAlphaTest()
// Enables alpha testing, for cut-out effects.
{
	DesiredState.AlphaTesting = true;
}


void	DisableAlphaTest()
// Turns off alpha testing.
{
	DesiredState.AlphaTesting = false;
}


void	EnableLightmapBlend()
// Turns on blending mode appropriate for lightmapping.
{
	DesiredState.LightmapBlending = true;
}


void	DisableLightmapBlend()
// Turns off blending.
{
	DesiredState.LightmapBlending = false;
}


void	BlitImage(int x, int y, int width, int height, Texture* im, int u, int v, uint32 ARGBColor, float scale)
// Blits the given image to the screen.  No rotation allowed.
{
	OGLTexture*	t = static_cast<OGLTexture*>(im);
	if (t == NULL) return;
	Render::SetTexture(t);
	Render::CommitRenderState();

#ifdef NOT
	// Matrix stuff, & disabling zbuffer, & enabling alpha test, should go in EndFrame().
	EnableAlphaTest();
	CommitRenderState();
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glScalef(2.0 / 640, -2.0 / 480.0, 1);
	glTranslatef(-640 / 2.0, -480 / 2.0, 0);
//	glTranslatef(-Render::GetWindowWidth() / 2.0, -Render::GetWindowHeight() / 2.0, 0);
#endif // NOT

	float	u0, v0, u1, v1;
	float	x0, y0, x1, y1;

	u0 = (u + BlitUVOffset) / float(t->GetWidth());
	v0 = (v + BlitUVOffset) / float(t->GetHeight());
	u1 = (u + width / scale + BlitUVOffset) / float(t->GetWidth());
	v1 = (v + height / scale + BlitUVOffset) / float(t->GetHeight());

	x0 = x;
	y0 = y;
	x1 = x0 + width;
	y1 = y0 + height;

	glColor4ub((ARGBColor >> 16) & 255, (ARGBColor >> 8) & 255, ARGBColor & 255, (ARGBColor >> 24) & 255);
	
	glBegin(GL_QUADS);

	glTexCoord2f(u0, v1);
	glVertex2f(x0, y1);
	
	glTexCoord2f(u1, v1);
	glVertex2f(x1, y1);
	
	glTexCoord2f(u1, v0);
	glVertex2f(x1, y0);
	
	glTexCoord2f(u0, v0);
	glVertex2f(x0, y0);

	glEnd();

#ifdef NOT
//	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#endif // NOT
}


void	FullscreenOverlay(uint32 ARGBColor)
// Covers the screen with the specified color.  Alpha blending turned on.
{
	SetTexture(NULL);
	DisableAlphaTest();
	DisableZBuffer();
	CommitRenderState();
	
	// Matrix stuff, & enabling alpha blend.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glScalef(2.0 / 640, -2.0 / 480.0, 1);
	glTranslatef(-640 / 2.0, -480 / 2.0, 0);

	float	x0, y0, x1, y1;

	x0 = 0;
	y0 = 0;
	x1 = 640;
	y1 = 480;

	glColor4ub((ARGBColor >> 16) & 255, (ARGBColor >> 8) & 255, ARGBColor & 255, (ARGBColor >> 24) & 255);
	
	glBegin(GL_QUADS);
	glVertex2f(x0, y1);
	glVertex2f(x1, y1);
	glVertex2f(x1, y0);
	glVertex2f(x0, y0);
	glEnd();

	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glDisable(GL_BLEND);
}


void	ShowImageImmediate(bitmap32* im)
// Copies the given image straight to the screen.
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	glRasterPos2f(-1, -1);
	glDrawPixels(im->GetWidth(), im->GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, im->GetData());

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	ShowFrame();
}


// Pointer to temp buffer for screen-shots.
static uint32*	data = 0;


uint32*	GetScreenPixels()
// Returns a pointer to a buffer filled with the screen's pixel data.
// The format is in default OpenGL format (i.e. left-to-right,
// bottom-to-top).  Each pixel is 32 bits, RGBA.
{
//	glFinish();

	int	w = GetWindowWidth();
	int	h = GetWindowHeight();

	// Allocate temp buffer once.  No way to free it, but this is a specialized function anyway.
	if (data == 0) data = new uint32[w * h];

	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return data;
}


void	WriteScreenshotFile()
// Takes a screenshot of the current frame, and writes it to a new
// bitmap file.  Generates the filename of the form "SCREENxx.PSD",
// where xx is replaced by a number.
{
	// Pick an unused file name.
	char	filename[20];
	int	i;
	struct stat	dummy;
	for (i = 0; i < 100; i++) {
		sprintf(filename, "screen%02d.psd", i);
		if (stat(filename, &dummy) != 0) break;
	}

#if 0
	// Grab the framebuffer data, and write it out.
	glFinish();

	int	w = GetWindowWidth();
	int	h = GetWindowHeight();
	
	if (data == 0) data = new uint32[w * h];

	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
#else
	int	w = GetWindowWidth();
	int	h = GetWindowHeight();
	
	GetScreenPixels();
#endif

	PSDRead::WriteImageData32(filename, data, w, h, false);
}


void	WriteScreenshotFilePPM(const char* filename)
// Writes a .ppm format file to the given filename.
{
#if 0
	// Grab the framebuffer data, and write it out.
	glFinish();

	int	w = GetWindowWidth();
	int	h = GetWindowHeight();

	// Allocate temp buffer once.  No way to free it, but this is a specialized function anyway.
	if (data == 0) data = new uint32[w * h];

	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
#else
	int	w = GetWindowWidth();
	int	h = GetWindowHeight();
	
	GetScreenPixels();
#endif

	// Write the data as .ppm .
	FILE*	fp = fopen(filename, "wb");
	if (fp == NULL) return;

	fprintf(fp, "P6\n%d %d\n255\n", w, h);	// Header.

	// Pixel data, rgbrgbrgb...
	int pixels = w * h;
	for (int y = 0; y < h; y++) {
		uint8*	p = (uint8*) (data + ((h - y - 1) * w));	// Write the rows in reverse order of the screenshot data.
		for (int x = 0; x < w; x++) {
			fwrite(p, 1, 3, fp);	// Write rgb bytes.
			p += 4;
		}
	}

	fclose(fp);
}


/**
 * Checks for OpenGL error codes.
 *
 * If there's an error, throws an exception with an error message which includes
 * the given function name.
 *
 * @param FuncName   Provide callee function for logging purposes
 */
void
CheckOGLError(const char* FuncName)
{
	GLenum	error = glGetError();
	if (error == GL_NO_ERROR) return;

	int	ErrorLevel = Config::GetInt("OGLCheckErrorLevel");
	if (ErrorLevel == 0) {
		return;	// Ignore errors.
	}
	
	RenderError	e;
	e << FuncName;
	
	switch (error) {
	default:
		e << ": unknown GL error " << int(error);
		break;
	case GL_INVALID_ENUM:
		e << ": Invalid enum";
		break;
	case GL_INVALID_VALUE:
		e << ": Invalid value";
		break;
	case GL_INVALID_OPERATION:
		e << ": Invalid operation";
		break;
	case GL_STACK_OVERFLOW:
		e << ": Stack overflow";
		break;
	case GL_STACK_UNDERFLOW:
		e << ": Stack underflow";
		break;
	case GL_OUT_OF_MEMORY:
		e << ": Out of memory";
		break;
	}

	if (ErrorLevel == 1) {
		// Log errors.
		Console::Printf("%s\n", e.GetMessage());
	} else {
		// Throw exception on error.
		throw e;
	}
}


}; // end namespace Render
