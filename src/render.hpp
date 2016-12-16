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
// render.hpp	-thatcher 1/28/1998 Copyright Thatcher Ulrich

// Wrapper for 3D rendering library.


#ifndef RENDER_HPP
#define RENDER_HPP


#include "types.hpp"
#include "geometry.hpp"
#include "error.hpp"
#include "ogl.hpp"


namespace Render {
	class RenderError : public Error {};
	
	void	Open();
	void	Close();

	void	Open3D();
	void	Close3D();

	void	SwapBuffers();

	void	RefreshWindow();	// ?? good name? It's for resetting the viewport when window moves...
	void	MakeUIVisible();

// macOS Set functions, changing window/screen options from GUI
#ifdef MACOSX
  void SetWindowWidth(int width);
  void SetWindowHeight(int height);
  void SetWindowDepth(int depth);
  void SetWindowFullscreen(bool fullscreen);
#endif


	// Driver/device control.
	int	GetDeviceCount();
	const char*	GetDeviceDescription(int DeviceIndex);
	int	GetCurrentDevice();
	
	int	GetDriverCount(int device);
	const char*	GetDriverName(int DriverIndex);
	const char*	GetDriverComment(int DriverIndex);
	int	GetCurrentDriver(int device);
	int	AddNewDriver();
	void	SetDriverInfo(int index, const char* Name, const char* Comment);
	void	DeleteDriver(int index);
	
	bool	GetFullscreen();
	int	GetWindowWidth();
	int	GetWindowHeight();

	HGLRC	GetMainContext();
	
	// Mode control.
	struct ModeInfo {
		int	Width, Height;
		int	BPP;
		bool	CurrentDriverCanDo;
	};

	int	GetModeCount(int device);
	int	GetCurrentMode(int device);
	void	GetModeInfo(int device, ModeInfo* result, int index);
	void	GetDefaultDriverAndMode(int device, int* driver, int* mode);

	void	SetDisplayOptions(int device, int driver, int mode, bool fullscreen);
	
	// Assume RGB mode.
	// Assume 16-bit z-buffer or equivalent.
	
	void	ShowFrame();
	void	ClearFrame();
	void	ClearZBuffer();
	
	// Textures.
	class	Texture {
	public:
		virtual ~Texture() {}
		virtual void	ReplaceImage(bitmap32* b) = 0;
		int	GetHeight() const { return Height; }
		int	GetWidth() const { return Width; }
		int	GetTexelCount() const { return TexelCount; }
	protected:
		int	Height, Width, TexelCount;
	};
	class	NewTextureError : public RenderError {};
	Texture*	NewTexture(const char* Filename, bool NeedAlpha, bool MakeMIPMaps, bool Tile, bool AlphaFadedMIPMaps = false);
	Texture*	NewTextureFromBitmap(bitmap32* b, bool NeedAlpha, bool MakeMIPMaps, bool Tile, bool AlphaFadedMIPMaps = false);
	void	DeleteTexture(Texture* t);
	void	ClearTextures();	// For deleting all the textures.
	void	InvalidateTextures();	// To force all textures to be reloaded/rebuilt.

	// Drawing commands.
	void	BeginFrame();
	void	EndFrame();

	void	MultMatrix(const matrix& m);	// converts m to OpenGL format and calls glMultMatrix()
	
	void	SetTexture(Texture* t);

	void	EnableZBuffer();
	void	DisableZBuffer();

	void	EnableAlphaTest();
	void	DisableAlphaTest();

	void	EnableLightmapBlend();
	void	DisableLightmapBlend();

	void	CommitRenderState();

	void	BlitImage(int x, int y, int width, int height, Texture* im, int u, int v, uint32 ARGBColor = 0xFFFFFFFF, float scale = 1);

	void	FullscreenOverlay(uint32 ARGB);
	
	void	ShowImageImmediate(bitmap32* b);

	uint32*	GetScreenPixels();
	void	WriteScreenshotFile();
	void	WriteScreenshotFilePPM(const char* filename);
};


#endif // RENDER_HPP
