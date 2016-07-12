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
// oglrender.cpp	-thatcher 3/1/1999 Copyright Slingshot Game Technology

// Rendering wrapper using OpenGL for rasterization.


#ifdef MACOSX
#include "macosxworkaround.hpp"
#elif defined LINUX

#include <SDL.h>
#include <GL/glx.h>
//#include <X11/extensions/xf86vmode.h>
#include "linuxmain.hpp"

#else // not LINUX nor MACOSX

#include <windows.h>
#include "winmain.hpp"

#endif // LINUX

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
		
#ifndef LINUX
		Main::CloseGameWindow();
#endif // not LINUX
		
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
	// Redraw the window frame and whatnot.
	DrawMenuBar(Main::GetMainWindow());
	RedrawWindow(Main::GetMainWindow(), NULL, NULL, RDW_FRAME);
#endif // not LINUX
}


#ifndef LINUX

const int	MAX_DEVICES = 10;

const int	NAME_MAXLEN = 30;
const int	DESC_MAXLEN = 80;
	

// Maintain a list of available drivers, and a currently selected driver.
const int	MAX_DRIVERS = 20;
int	DriverCount = 0;
struct DriverInfo {
	char*	Name;
	char*	About;
} Driver[MAX_DRIVERS];

#endif // not LINUX


bool	IsBlank(char* buf)
// Returns true if the given string is nothing but whitespace.
{
	char	c;
	while ((c = *buf++)) {
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return false;
	}
	return true;
}


void	ReadDriverList(const char* filename)
// Reads the list of drivers from the specified file.
{
#ifndef LINUX

	DriverCount = 0;

	FILE*	fp = Utility::FileOpen(filename, "r");

	// Read the driver filenames and description.  The format is:
	// * any number of blank lines
	// * A line consisting of the driver description
	// * A line consisting of the driver filename (can include path)

	const int	MAX_LINE_LENGTH = 2000;
	char	buf[MAX_LINE_LENGTH];
	int	len;
	
	for (;;) {
		while (1) {
			if (fgets(buf, MAX_LINE_LENGTH, fp) == NULL) {
				// End of file, we're done here.
				goto Done;
			}

			// If this is a blank line, then just keep looping.
			// Otherwise, it's the description of the driver.
			if (IsBlank(buf) == false) break;
		}

		len = strlen(buf);
		
		// Blitz the trailing newline.
		if (len) {
			buf[len - 1] = 0;
			len--;
		}

		// Set the description.
		Driver[DriverCount].About = new char[len + 1];
		strcpy(Driver[DriverCount].About, buf);

		// Read the next line, it should be the driver filename.
		fgets(buf, MAX_LINE_LENGTH, fp);

		len = strlen(buf);
		// Blitz any trailing newline.
		if (len) {
			buf[len - 1] = 0;
			len--;
		}

		// Set the driver name.
		Driver[DriverCount].Name = new char[len + 1];
		strcpy(Driver[DriverCount].Name, buf);
		
		DriverCount++;
		if (DriverCount >= MAX_DRIVERS) break;
	}

Done:
	fclose(fp);

#endif // not LINUX
}


void	WriteDriverList(const char* filename)
// Writes the list of OpenGL driver DLLs to the specified filename.
{
#ifndef LINUX

	FILE*	fp = Utility::FileOpen(filename, "w");
	if (fp == NULL) return;

	int	i;
	for (i = 0; i < DriverCount; i++) {
		fputs(Driver[i].About, fp);
		fputs("\n", fp);
		fputs(Driver[i].Name, fp);
		fputs("\n\n", fp);
	}
	
	fclose(fp);

#endif // not LINUX
}


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
#ifdef LINUX
	return 1;
#else // not LINUX
	return DriverCount;
#endif // not LINUX
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
#ifdef LINUX
	return "OpenGL";
#else // not LINUX
	if (index < 0 || index >= DriverCount) return "";
	else return Driver[index].Name;
#endif // not LINUX
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
#ifdef LINUX
	return "---";
#else // not LINUX
	if (index < 0 || index >= DriverCount) return "";
	else return Driver[index].About;
#endif // not LINUX
}

	
int	GetCurrentDriver(int device)
// Returns the index of the driver currently selected under the specified device.
{
	return CurrentDriver;
}


void	DeleteDriver(int index)
// Deletes the specified driver from our list.
{
#ifndef LINUX
	if (index <= 0 || index >= DriverCount) return;

	if (CurrentDriver == index) CurrentDriver--;

	// Delete the indexed driver's data.
	if (Driver[index].Name) delete [] Driver[index].Name;
	if (Driver[index].About) delete [] Driver[index].About;
	DriverCount--;

	// Shift the remainder of the list downwards, to close the gap.
	int	i;
	for (i = index; i < DriverCount; i++) {
		Driver[i].Name = Driver[i+1].Name;
		Driver[i].About = Driver[i+1].About;
	}
	Driver[i].Name = NULL;
	Driver[i].About = NULL;

	// Make sure the change gets saved.
	WriteDriverList(DriverListFilename);

#endif // not LINUX
}


int	AddNewDriver()
// Adds a new driver and returns its index.  Returns -1 if a new driver
// can't be added.
{
#ifndef LINUX

	if (DriverCount >= MAX_DRIVERS) return -1;

	int	d = DriverCount;
	DriverCount++;
	
	Driver[d].Name = new char[4];
	strcpy(Driver[d].Name, "---");
	Driver[d].About = new char[4];
	strcpy(Driver[d].About, "---");

	return d;
#else	// LINUX
	return -1;
#endif // LINUX
}


void	SetDriverInfo(int index, const char* Name, const char* Comment)
// Sets the info for the specified driver.
{
#ifndef LINUX

	if (index < 0 || index >= DriverCount) return;

	if (Driver[index].Name) delete [] Driver[index].Name;
	if (Driver[index].About) delete [] Driver[index].About;

	Driver[index].Name = new char[strlen(Name) + 1];
	strcpy(Driver[index].Name, Name);

	Driver[index].About = new char[strlen(Comment) + 1];
	strcpy(Driver[index].About, Comment);

	// Make sure the change gets saved.
	WriteDriverList(DriverListFilename);

#endif // not LINUX
}

	
int	GetWindowWidth() { return Mode[CurrentMode].Width; }
int	GetWindowHeight() { return Mode[CurrentMode].Height; }

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
#ifdef LINUX
#else // not LINUX
		Main::OpenGameWindow(Mode[CurrentMode].Width, Mode[CurrentMode].Height, Fullscreen);
#endif // not LINUX
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


#ifdef LINUX

Display*	display = NULL;
XVisualInfo	visual;
Colormap	colormap;
Window	window;
GLXContext	context;
//XF86VidModeModeInfo	original_mode;


bool	AcceptableVisual(Display* display, XVisualInfo* v)
// Returns true if the given X visual is acceptable for our purposes.
{
	int	value;

	// Check ability to use OpenGL.
	if (glXGetConfig(display, v, GLX_USE_GL, &value) || value == False) {
		return false;
	}

	// Make sure rendering is at default frame buffer level.
	if (glXGetConfig(display, v, GLX_LEVEL, &value) || value != 0) {
		return false;
	}

	// Require double buffering.
	if (glXGetConfig(display, v, GLX_DOUBLEBUFFER, &value) || value == False) {
		return false;
	}

	// Has the necessary attributes.
	return true;
}


int	CompareVisuals(Display* display, XVisualInfo* a, XVisualInfo* b)
// Compares visuals a and b with respect to our desired use.  If b is better than a,
// returns 1; if a is better than b, returns -1, else returns 0.
{
	// Assume they're both acceptable.  Go through and compare on basic criteria.

	int	aval, bval;

	// First compare RGBA vs. color index.
	if (glXGetConfig(display, a, GLX_RGBA, &aval)) return 0;
	if (glXGetConfig(display, b, GLX_RGBA, &bval)) return 0;
	if (aval != bval) {
		if (aval == True) return -1;
		else return 1;
	}

	// Try for the desired color depth.
	if (glXGetConfig(display, a, GLX_BUFFER_SIZE, &aval)) return 0;
	if (glXGetConfig(display, b, GLX_BUFFER_SIZE, &bval)) return 0;
	if (aval != bval) {
		if (aval == DesiredColorDepth) return -1;
		if (bval == DesiredColorDepth) return 1;
		if (aval > bval) return -1;
		else return 1;
	}

	// xxx try for desired alpha depth xxx
	// xxx try for desired stencil depth xxx
	// xxx try for desired accum depth xxx

	// Make sure we have at least DesiredZDepth bits of z-buffer.
	if (glXGetConfig(display, a, GLX_DEPTH_SIZE, &aval)) return 0;
	if (glXGetConfig(display, b, GLX_DEPTH_SIZE, &bval)) return 0;
	if (aval != bval) {
		if (aval == DesiredZDepth) return -1;
		if (bval == DesiredZDepth) return 1;
		if (aval > bval) return -1;
		else return 1;
	}
	
	// Avoid some oddball buffer types.

	// Avoid stereo.
	if (glXGetConfig(display, a, GLX_STEREO, &aval)) return 0;
	if (glXGetConfig(display, b, GLX_STEREO, &bval)) return 0;
	if (aval != bval) {
		if (aval == False) return -1;
		else return 1;
	}

	// Avoid aux buffers.
	if (glXGetConfig(display, a, GLX_AUX_BUFFERS, &aval)) return 0;
	if (glXGetConfig(display, b, GLX_AUX_BUFFERS, &bval)) return 0;
	if (aval != bval) {
		if (aval < bval) return -1;
		else return 1;
	}

	// Formats seem the same for our purposes.
	return 0;
}


#else // not LINUX


int	ComparePFD(const PIXELFORMATDESCRIPTOR* a, const PIXELFORMATDESCRIPTOR* b)
// Compares the two pixel formats, w/r/t their desirability as our rendering mode.
// Returns 1 if a is more desirable than b; 0 if they're equal; or -1 if b is more
// desirable than a.
{
	int	atype, btype;

	// Require ability to double-buffer, draw to a window and support OpenGL.
	int	aok, bok;
	if ((a->dwFlags & PFD_DRAW_TO_WINDOW) && (a->dwFlags & PFD_SUPPORT_OPENGL) && (a->dwFlags & PFD_DOUBLEBUFFER)) aok = 1;
	else aok = 0;

	if ((b->dwFlags & PFD_DRAW_TO_WINDOW) && (b->dwFlags & PFD_SUPPORT_OPENGL) && (b->dwFlags & PFD_DOUBLEBUFFER)) bok = 1;
	else bok = 0;

	if (aok > bok) return 1;
	else if (aok < bok) return -1;
	else if (aok == 0 && bok == 0) return 0;	// Neither format is OK, so don't bother doing any more checking.
	
	//
	// First criterion is whether it uses an ICD, an MCD, or the generic software renderer.
	//
	
	// See what type of renderer a uses.
	if (a->dwFlags & PFD_GENERIC_FORMAT) {
		if (a->dwFlags & PFD_GENERIC_ACCELERATED) {
			atype = 1;	// MCD.
		} else {
			atype = 0;	// Software renderer.
		}
	} else {
		atype = 2;	// ICD.
	}

	// and b...
	if (b->dwFlags & PFD_GENERIC_FORMAT) {
		if (b->dwFlags & PFD_GENERIC_ACCELERATED) {
			btype = 1;	// MCD.
		} else {
			btype = 0;	// Software renderer.
		}
	} else {
		btype = 2;	// ICD.
	}

	if (Config::GetBoolValue("ForceSoftwareRenderer")) {
		if (atype < btype) return 1;
		else if (atype > btype) return -1;
	} else {
		if (atype > btype) return 1;
		else if (atype < btype) return -1;
	}

	// Next criterion is support of the desired pixel depth.
	// In absense of the exact support, more bits are presumably better.
	int	acolor, bcolor;
	if (a->cColorBits == DesiredColorDepth) acolor = 1000;
	else acolor = a->cColorBits - DesiredColorDepth;

	if (b->cColorBits == DesiredColorDepth) bcolor = 1000;
	else bcolor = b->cColorBits - DesiredColorDepth;

	if (acolor > bcolor) return 1;
	else if (acolor < bcolor) return -1;
	
	// Next criterion is a suitable z-buffer depth.
	// Like color depth, if the exact depth isn't supported then more is better.
	int	az, bz;
	if (a->cDepthBits == DesiredZDepth) az = 1000;
	else az = a->cDepthBits - DesiredZDepth;

	if (b->cDepthBits == DesiredZDepth) bz = 1000;
	else bz = b->cDepthBits - DesiredZDepth;

	if (az > bz) return 1;
	else if (az < bz) return -1;

	// Well, the formats look pretty much the same for our purposes.
	return 0;
}


PIXELFORMATDESCRIPTOR	DesiredPFD;
int	DesiredPFDIndex = -1;


void	FindDesiredPixelFormat()
// Enumerates the various OpenGL options, and updates our info arrays.
{
	// Query for OpenGL info.
	HDC	hdc = GetDC(Main::GetGameWindow());
	PIXELFORMATDESCRIPTOR	pfd;

	int	PixelFormatCount = OGL::DescribePixelFormat(hdc, 0, 0, NULL);

	for (int i = 0; i < PixelFormatCount; i++) {
		OGL::DescribePixelFormat(hdc, i + 1, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

		if (DesiredPFDIndex < 0) {
			// First pfd, so by definition it's the best so far.
			DesiredPFD = pfd;
			DesiredPFDIndex = i + 1;
		} else {
			// Compare this pfd to our previous best, and replace if it's better.
			if (ComparePFD(&pfd, &DesiredPFD) > 0) {
				DesiredPFD = pfd;
				DesiredPFDIndex = i + 1;
			}
		}
	}
}


#endif // not LINUX


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
	
	HDC	hdc = GetDC(Main::GetGameWindow());	//xxxxxxx?????

	PIXELFORMATDESCRIPTOR pfd = { 
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd 
		1,                                // version number 
		PFD_DRAW_TO_WINDOW |              // support window 
		PFD_SUPPORT_OPENGL |              // support OpenGL
		PFD_DOUBLEBUFFER,                 // double buffered
		
		PFD_TYPE_RGBA,                    // RGBA type
		16,                               // 16-bit color depth
		0, 0, 0, 0, 0, 0,                 // color bits ignored
		0,                                // no alpha buffer
		0,                                // shift bit ignored
		0,                                // no accumulation buffer
		0, 0, 0, 0,                       // accum bits ignored
		16,                               // 16-bit z-buffer
		0,                                // no stencil buffer
		0,                                // no auxiliary buffer
		PFD_MAIN_PLANE,                   // main layer
		0,                                // reserved
		0, 0, 0                           // layer masks ignored
	};

	FindDesiredPixelFormat();
	
	int	PixelFormat = DesiredPFDIndex;
	PIXELFORMATDESCRIPTOR*	p = &DesiredPFD;

	if (PixelFormat < 0) {
		// Ask the OS to choose something for us.
		p = &pfd;
		PixelFormat = OGL::ChoosePixelFormat(hdc, &pfd);
	}
	
	// Now, set the pixel format.
	bool	Result = OGL::SetPixelFormat(hdc, PixelFormat, p) ? true : false;
	if (Result == false) {
		RenderError e; e << "SetPixelFormat() failed.";
		throw e;
	}
	
	RenderingContext = OGL::CreateContext(hdc);
	Result = OGL::MakeCurrent(hdc, RenderingContext);
	if (Result == false) {
		RenderError e; e << "OGL::MakeCurrent() failed.";
		throw e;
	}

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

	glViewport(0, 0, GetWindowWidth(), GetWindowHeight());
	
	CheckOGLError("OpenOGL()");
}


void	CloseOGL()
// Close the OpenGL rendering context, and do other cleanup.
{
#ifdef LINUX
#else // not LINUX
	bool	Result = OGL::MakeCurrent(NULL, NULL);
	if (Result == false) {
		RenderError e; e << "CloseOGL(): OGL::MakeCurrent failed.";
		throw e;
	}
#endif // not LINUX

#ifdef LINUX
#else // not LINUX
	Result = OGL::DeleteContext(RenderingContext);
	if (Result == false) {
		RenderError e; e << "CloseOGL(): OGL::DeleteContext failed.";
		throw e;
	}
#endif // not LINUX
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
	bool	Result = OGL::SwapBuffers(GetDC(Main::GetGameWindow()));
	if (Result == false) {
		RenderError e; e << "SwapBuffers() failed.";
		throw e;
	}
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


class OGLTexture;
//OGLTexture*	TextureList = NULL;
int	NextOGLTextureName = 1;


class OGLTexture : public Texture {
public:
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
					Geometry::HalfScaleFilterSincScaleAlpha(sw, sh, src, dest, fclamp(0, 1 - (i / 3.0f), 1));
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
	
	glViewport(0, 0, GetWindowWidth(), GetWindowHeight());

	if (Config::GetBoolValue("Wireframe")) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


void	EndFrame()
// Call when finished with a frame.
{
//	glFlush();
//
//	CheckOGLError("EndFrame()");
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

	x0 = float(x);
	y0 = float(y);
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
	glScalef(2.0f / 640, -2.0f / 480.0f, 1);
	glTranslatef(-640 / 2.0f, -480 / 2.0f, 0);

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
