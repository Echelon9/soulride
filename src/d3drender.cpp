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
// d3drender.cpp	-thatcher 1/28/1998 Copyright Thatcher Ulrich

// Direct3D implementation of Render interface.


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <search.h>
#include <ddraw.h>
#include <d3d.h>

#include "utility.hpp"
#include "render.hpp"
#include "main.hpp"
#include "winmain.hpp"
#include "error.hpp"
#include "psdread.hpp"
#include "timer.hpp"


using namespace Render;


// Internal variables.
namespace Render {
	bool	IsOpen = false;
	bool	Is3DOpen = false;

	PALETTEENTRY	OriginalPPE[256];
	PALETTEENTRY	PPE[256];

	const int	WINDOW_MINIMUM = 100;

	// Information about available display modes.
	ModeInfo	WindowsDisplay;

	// Interface pointers.
	LPDIRECTDRAW4	DD4Interface = NULL;
	LPDIRECT3D3	D3DInterface = NULL;	   /* D3D object */
	LPDIRECT3DDEVICE3	D3DDevice = NULL;   /* D3D device */
	LPDIRECT3DMATERIAL3	MaterialInterface = NULL;

	D3DMATERIALHANDLE	Background = 0;
	D3DMATERIALHANDLE	Foreground = 0;

	const int	NAME_MAXLEN = 30;
	const int	DESC_MAXLEN = 80;
	
	// Driver attributes.
	struct DriverInfo {
		char	Name[NAME_MAXLEN];	/* short name of the driver */
		char	About[DESC_MAXLEN];	/* short string about the driver */
		D3DDEVICEDESC	Desc;	/* D3DDEVICEDESC for complete information */
		GUID	Guid;		/* its GUID */
		bool	IsHardware;	/* does this driver represent a hardware device? */
		bool	DoesTextures;	/* does this driver do texture mapping? */
		bool	DoesZBuffer;	/* can this driver use a z-buffer? */
		bool	CanDoWindow;	/* can it render to Windows display depth? */
	};
	const int	MAX_DRIVERS = 10;

	const int	MAX_MODES = 30;
	
	// DD device attributes.
	struct DeviceInfo {
		char	Name[NAME_MAXLEN];
		char	Description[DESC_MAXLEN];
		bool	GuidValid;
		GUID	Guid;
		DDCAPS	DriverCaps;
		bool	Does3D;
		bool	IsPrimary;

		// Maintain a list of available drivers, and a currently selected driver.
		int	DriverCount;
		DriverInfo	Driver[MAX_DRIVERS];
		int	CurrentDriver;

		// Maintain a list of available modes, and a currently selected mode.
		int	ModeCount;
		int	CurrentMode;
		ModeInfo	Mode[MAX_MODES];
	};
	int	DeviceCount = 0;
	int	CurrentDevice = -1;
	const int	MAX_DEVICES = 10;
	DeviceInfo	Device[MAX_DEVICES];
	

	int	GetModeCount(int device) { return Device[device].ModeCount; }	// Returns the number of supported display modes under the specified device.
	int	GetCurrentMode(int device) { return Device[device].CurrentMode; }	// Returns the device's currently active mode.
	void	GetModeInfo(int device, ModeInfo* result, int index)
	// Returns information about the indexed mode.  Puts the data in
	// the structure pointed to by result.
	{
		*result = Device[device].Mode[index];
	}
	
	// Location of the client window, in screen coordinates.
	POINT	ClientOnPrimary;
	// Size of client window.
	SIZE	ClientSize;
	SIZE	LastClientSize;

//	// Values to offset input vertices by, so that a vertex at (0,0) is at the center of the viewport.
//	float	XOffset, YOffset;
	
	// Surface & buffer info.
	bool	Fullscreen = true;
	SIZE	BuffersSize;
	bool	BackBufferInVideo = false;
	bool	ZBufferInVideo = false;
	bool	OnlyEmulation = false;
	bool	OnlySystemMemory = false;
	LPDIRECTDRAWSURFACE4	FrontBuffer = NULL;	/* front buffer surface */
	LPDIRECTDRAWSURFACE4	BackBuffer = NULL;	/* back buffer surface */
	LPDIRECTDRAWSURFACE4	ZBuffer = NULL;	/* z-buffer surface */

	LPDIRECTDRAWCLIPPER	Clipper = NULL;
	LPDIRECT3DVIEWPORT3	Viewport = NULL;

	// Texture info.
	DDPIXELFORMAT	PreferredTextureFormat;
	DDPIXELFORMAT	BackBufferFormat;
	DDPIXELFORMAT	ZBufferFormat;
	
	// Private functions.
	void	EnumerateD3DOptions();
	void	OpenD3D(int device, int driver, int mode, bool Fullscreen);
	void	CloseD3D();
	bool	CreateBuffers(HWND hwnd, int w, int h, int bpp, bool Fullscreen, bool IsHardware);
	void	ClearBuffers();
	void	ClearBuffer(LPDIRECTDRAWSURFACE4 buffer);
	bool	CreateZBuffer(int w, int h, int driver);

	int	SelectDriver(bool Fullscreen, const ModeInfo& mode);
	void	SetWindowsMode(bool Fullscreen, int ModeIndex, HWND hWnd);
	void	SetD3DDriver(int driver);

	void	SetupViewport();
	
//	void	ConvertVertex(D3DTLVERTEX* result, const Vertex& v);
	
	char*	D3DErrorToString(HRESULT error);
	DWORD	BPPToDDBD(int bpp);

	int	CompareModes(const void* element1, const void* element2);
	BOOL FAR PASCAL	DDEnumCallback(GUID FAR* lpGUID, LPSTR lpDriverDesc, LPSTR lpDriverName, LPVOID lpContext);
	HRESULT CALLBACK	EnumDisplayModesCallback(LPDDSURFACEDESC2 pddsd, LPVOID lpContext);
	HRESULT WINAPI	EnumDeviceFunc(LPGUID lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC lpHWDesc, LPD3DDEVICEDESC lpHELDesc, LPVOID lpContext);
	HRESULT CALLBACK	EnumTextureCallback(DDPIXELFORMAT *DeviceFmt, LPVOID lParam);
	HRESULT CALLBACK	EnumZBufferCallback(DDPIXELFORMAT *DeviceFmt, LPVOID lParam);

	void	Copy32BitArrayToSurface(uint32* Data, int Width, int Height, DDSURFACEDESC2& Desc);
	
	// Texture stuff.
	
	// class Texture
	class D3DTexture : public Texture {
	public:
		D3DTexture(const char* FileOrResourceName);
		D3DTexture(bitmap32* b);
		virtual ~D3DTexture();

		void	ReplaceImage(bitmap32* b);
//		void	SetColorKey(int RGBColorKey);
		
		void	BuildSystemMemorySurface(const char* FileOrResourceName);
		void	BuildSystemMemorySurface(uint32* data);

		void	Invalidate();
		void	Restore();	// Copy the memory surface to the device surface.  Build device surface if needed.

		bool	Valid;
		int	Width, Height;
		LPDIRECTDRAWSURFACE4	MemorySurface;
		LPDIRECTDRAWSURFACE4	DeviceSurface;
		LPDIRECT3DTEXTURE2	DeviceTexture;
//		int	ColorKey;
//		bool	ColorKeyEnabled;
		
		// For managing a texture list.
		D3DTexture*	Next;
		D3DTexture*	Previous;
	};

	// 2D image stuff.
	class DDImage : public Image {
	public:
		DDImage(bitmap32* b);
		virtual	~DDImage();
		void	SetColorKey(int RGBColor);
		int	GetWidth() { return Width; }
		int	GetHeight() { return Height; }

		void	BuildSystemMemorySurface(uint32* data);
		// void	Invalidate();
		void	Restore();

		int	Width, Height;
		LPDIRECTDRAWSURFACE4	MemorySurface;
		LPDIRECTDRAWSURFACE4	DeviceSurface;
		int	ColorKey;
		bool	ColorKeyEnabled;

		// list stuff?
	};
	
	// Shadow D3D's render state vs. our logical render state, so we can minimize actual
	// state changes sent to the driver.
	struct RenderState {
		bool	AlphaTesting, ZBuffering, Texturing;
		bool	LightmapBlending, ColorKeying;
		D3DTexture*	Texture;
	};
	RenderState	ShadowState = {
		false, true, true, false, false, NULL
	};
	RenderState	DesiredState = {
		false, true, true, false, false, NULL
	};
	void	CommitRenderState();
	
	// Misc functions.
	bool	GetFullscreen() { return Fullscreen; }

	int	GetDeviceCount()
	// Returns the number of D3D devices available.
	{
		return DeviceCount;
	}

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
		if (device < 0 || device >= DeviceCount) return "!invalid";
		else return Device[device].Description;
	}

	int	GetCurrentDevice()
	// Returns the index of the currently selected device.
	{
		return CurrentDevice;
	}

	void	SetCurrentDevice(int device)
	// Sets the specified device as the current device.
	{
		//xxxxxxxxxxxxx
	}
	
	int	GetDriverCount(int device)
	// Returns the number of D3D drivers under the specified device.
	{
		if (device < 0 || device >= DeviceCount) return 0;
		return Device[device].DriverCount;
	}

	int	GetCurrentDriver(int device)
	// Returns the index of the driver currently selected under the specified device.
	{
		if (device < 0 || device >= DeviceCount) return 0;
		return Device[device].CurrentDriver;
	}

	char*	GetDriverDescription(int device, int index)
	// Returns a short string describing the indexed D3D device.  The purpose
	// is for letting the user pick the driver they want to use.
	{
		if (index < 0 || index >= Device[device].DriverCount) return "";
		else return Device[device].Driver[index].Name;
	}

//	Timer::Stopwatch	TextureDicking;	// xxxx for profiling.
};


void	Render::Open()
// Create the D3D interface.
{
	if (IsOpen == false) {
		EnumerateD3DOptions();

		CurrentDevice = 0;
		
		// Set driver and mode defaults.
		int	i;
		for (i = 0; i < DeviceCount; i++) {
			int	driver, mode;
			GetDefaultDriverAndMode(i, &driver, &mode);
			DeviceInfo&	d = Device[i];
			d.CurrentDriver = driver;
			d.CurrentMode = mode;
		}
	
		IsOpen = true;
	}
}


#define	RELEASE(x) if (x) { x->Release(); x = NULL; }


void	Render::Close()
// Close the D3D interface.
{
	if (IsOpen) {
		if (Is3DOpen) {
			Close3D();
		}
		
		IsOpen = false;
	}
}


void	Render::Open3D()
// Sets up for rendering.
{
	if (!Is3DOpen) {
		OpenD3D(CurrentDevice, Device[CurrentDevice].CurrentDriver, Device[CurrentDevice].CurrentMode, Fullscreen);
		Is3DOpen = true;
	}
}


void	Render::Close3D()
// Closes 3D stuff.
{
	if (Is3DOpen) {
		DeviceCount = 0;
		CloseD3D();
		Is3DOpen = false;
	}
}


void	Render::RefreshWindow()
// Resets our relationship to the screen, in case our window got moved or someting.
{
	if (Is3DOpen == false) return;

	ClientOnPrimary.x = ClientOnPrimary.y = 0;
	ClientToScreen(Main::GetMainWindow(), &ClientOnPrimary);
}


void	Render::MakeUIVisible()
// This function gets called when the app needs to allow access to the Windows UI.
{
//	DDSCAPS caps;
//	FrontBuffer->GetCaps(&caps);
//	
//	// if we are in ModeX go back to a windows mode
//	// so we can see the menu or dialog box.
//	
//	if (caps.dwCaps & DDSCAPS_MODEX)
//	{
//		DDSetMode(640,480,8);
//	}

	if (Is3DOpen == false) return;
	
	// Make sure the surface GDI writes to is visible.
	DD4Interface->FlipToGDISurface();
	
	// Redraw the window frame and whatnot.
	DrawMenuBar(Main::GetMainWindow());
	RedrawWindow(Main::GetMainWindow(), NULL, NULL, RDW_FRAME);
}


void	Render::SetupViewport()
// Sets the viewport parameters.
{
	HRESULT	LastError;

	// Initialize the viewport based on the current window.
	D3DVIEWPORT2	ViewData;
	float  aspect = (float) ClientSize.cx / ClientSize.cy;    // aspect ratio of surface
	memset(&ViewData, 0, sizeof(ViewData));
	ViewData.dwSize = sizeof(ViewData);
	ViewData.dwX = 0;
	ViewData.dwY = 0;
	ViewData.dwWidth = ClientSize.cx;
	ViewData.dwHeight = ClientSize.cy;
	ViewData.dvClipX = 0;
	ViewData.dvClipY = 0;
	ViewData.dvClipWidth = ClientSize.cx;
	ViewData.dvClipHeight = ClientSize.cy;
	ViewData.dvMinZ = 0.0f;
	ViewData.dvMaxZ = 1.0f;

	LastError = Viewport->SetViewport2(&ViewData);
	if (LastError != D3D_OK) {
		RenderError e; e << "Can't SetViewport2().
" << D3DErrorToString(LastError);
		throw e;
	}
	
	LastError = D3DDevice->SetCurrentViewport(Viewport);
	if (LastError != D3D_OK) {
		RenderError e; e << "Can't SetCurrentViewport().
" << D3DErrorToString(LastError);
		throw e;
	}

//	// Set values for offsetting input vertex coordinates, for centering view volume.
//	XOffset = (ClientSize.cx + 1) * 0.5;
//	YOffset = (ClientSize.cy + 1) * 0.5;
}


void	Render::EnumerateD3DOptions()
// Fills the Device[] and subsidiary arrays with information about available
// D3D devices, drivers and modes.
{
	// Save the original palette for when we are paused.  Just in case we
	// start in a fullscreen mode, put them in ppe.
	HDC hdc = GetDC(NULL);
	GetSystemPaletteEntries(hdc, 0, (1 << 8), (LPPALETTEENTRY) (&OriginalPPE[0]));
	for (int i = 0; i < 256; i++) PPE[i] = OriginalPPE[i];
	ReleaseDC(NULL, hdc);
	
	// Enumerate the DirectDraw devices.
	HRESULT	LastError = DirectDrawEnumerate(DDEnumCallback, NULL);
	if (LastError != DD_OK) {
		RenderError e; e << "DirectDrawEnumerate failed.
" << D3DErrorToString(LastError);
		throw e;
	}
}


void	Render::GetDefaultDriverAndMode(int device, int* driver, int* mode)
// Given the device, returns the default driver and mode for that device.
{
	int	i;

//	CurrentDevice = device;	//xxxxx
	DeviceInfo&	d = Device[device];

	// Look for the best driver.
	*driver = 0;
	for (i = 0; i < d.DriverCount; i++) {
		DriverInfo&	dr = d.Driver[i];
		if (dr.IsHardware && dr.DoesTextures && dr.DoesZBuffer) {
			*driver = i;
		}
	}

	// Pick 640x480x16 if possible.
	*mode = 0;
	for (i = 0; i < d.ModeCount; i++) {
		if (d.Mode[i].Width == 640 && d.Mode[i].Height == 480 && d.Mode[i].BPP == 16) {
			*mode = i;
		}
	}
}


void	Render::SetDisplayOptions(int device, int driver, int mode, bool fullscreen)
// Re-initializes the rendering subsystem using the given hints.
{
	InvalidateTextures();
	if (Is3DOpen) {
		
		OpenD3D(device, driver, mode, fullscreen);
	} else {
		// Remember the info, but don't try to actually set the device & mode yet.
		CurrentDevice = device;
		DeviceInfo&	d = Device[device];
		d.CurrentDriver = driver;
		d.CurrentMode = mode;
		Fullscreen = fullscreen;
	}
}


void	Render::OpenD3D(int device, int driver, int mode, bool fullscreen)
// Opens everything needed for D3D rendering.  Uses the specified
// device, driver, mode and fullscreen hints.
{
	CloseD3D();	// Release everything.

	CurrentDevice = device;
	DeviceInfo&	d = Device[device];
	d.CurrentDriver = driver;
	d.CurrentMode = mode;
	Fullscreen = fullscreen;
	
	HWND	hWnd = Main::GetMainWindow();
	
	//
	// Create the DD4Interface.
	//
	
	LPDIRECTDRAW	DDInterface = NULL;
	
	// Create the DD interface.
	HRESULT	LastError = DirectDrawCreate(d.GuidValid ? &d.Guid : NULL, &DDInterface, NULL);
	if (LastError != DD_OK) {
		RenderError e; e << "DirectDrawCreate failed.
" << D3DErrorToString(LastError);
		throw e;
	}

	// Need to do this in order to do the subsequent call to QueryInterface() ?? (it was in the docs, but I don't understand the requirement)
	LastError = DDInterface->SetCooperativeLevel(hWnd, DDSCL_NORMAL); 
	if (LastError != DD_OK) {
		RenderError e; e << "DDInterface->SetCooperativeLevel() failed.
" << D3DErrorToString(LastError);
		throw e;
	}

	// Now create a IDirectDraw2 interface, using the IDirectDraw interface we just created.
	LastError = DDInterface->QueryInterface(IID_IDirectDraw4, (LPVOID *) &DD4Interface);
	if (LastError != DD_OK) {
		RenderError e; e << "DDInterface->QueryInterface() failed.
" << D3DErrorToString(LastError);
		throw e;
	}

	RELEASE(DDInterface);

	//
	// Create the D3D interface.
	//
	
	// Create Direct3D interface.
	LastError = DD4Interface->QueryInterface(IID_IDirect3D3, (LPVOID*) &D3DInterface);
	if (LastError != DD_OK) {
		RenderError e; e << "Creation of IDirect3D failed.
" << D3DErrorToString(LastError);
		throw e;
	}

	if (Fullscreen == false && !d.IsPrimary) {
		Fullscreen = true;
	}

	SetWindowsMode(Fullscreen, d.CurrentMode, hWnd);
	SetD3DDriver(d.CurrentDriver);
}


void	Render::CloseD3D()
// Release all D3D resources.
{
	// Release everything.
	RELEASE(D3DDevice);
	RELEASE(Viewport);
	RELEASE(ZBuffer);
	RELEASE(BackBuffer);
	RELEASE(FrontBuffer);
	RELEASE(Clipper);
	RELEASE(MaterialInterface);
	RELEASE(D3DInterface);
	RELEASE(DD4Interface);
}


int	Render::SelectDriver(bool Fullscreen, const ModeInfo& mode)
// Picks one of the enumerated drivers for 3D rendering.  Picks a driver
// that can do the specified mode.  Returns the index of the driver.
// Returns -1 if it can't find a suitable driver.
{
	DWORD	depths = BPPToDDBD(mode.BPP);
	int	PreferredDriver = -1;

	DeviceInfo&	d = Device[CurrentDevice];
	
	// Find a driver that does the bit depth we want.
	for (int i = 0; i < d.DriverCount; i++) {
		// xxxxx need to verify that the driver can do the display mode xxxxxxxx
		
		if (PreferredDriver == -1) {
			// See if this driver is acceptable.
			if (d.Driver[i].Desc.dwDeviceRenderBitDepth & depths) {
				PreferredDriver = i;
			}
		} else {
			// We already have an acceptable driver.  See if we can upgrade.
			// Give preference to hardware drivers that can do an RGB color model.
			if (d.Driver[i].Desc.dwDeviceRenderBitDepth & depths) {
				if (d.Driver[i].IsHardware && !d.Driver[PreferredDriver].IsHardware) {
					PreferredDriver = i;
				} else if (d.Driver[i].IsHardware == d.Driver[PreferredDriver].IsHardware &&
					   d.Driver[i].Desc.dcmColorModel & D3DCOLOR_RGB && !(d.Driver[PreferredDriver].Desc.dcmColorModel & D3DCOLOR_RGB))
				{
					PreferredDriver = i;
				}
			}
		}
	}

	return PreferredDriver;
}


void	Render::SetWindowsMode(bool Fullscreen, int ModeIndex, HWND hWnd)
// This function sets the display mode parameters specified by Mode[ModeIndex],
// with the specified full-screen attribute.
// Throws an exception on failure.
{
	HRESULT	LastError;

	DeviceInfo&	d = Device[CurrentDevice];
	
	// If we're exiting full-screen mode, then be sure to restore the
	// original video settings.
	if (Fullscreen == false && Render::Fullscreen == true) {
		LastError = DD4Interface->SetDisplayMode(WindowsDisplay.Width, WindowsDisplay.Height, WindowsDisplay.BPP, 0 /* refresh rate */, 0 /* add'l flags */);
		if (LastError != DD_OK) {
			RenderError e; e << "Can't restore original video mode";
			throw e;
		}
	}

	Render::Fullscreen = Fullscreen;
	
	if (!Fullscreen) {
		// Change the window size.
		RECT	r;
		GetWindowRect(hWnd, &r);
		MoveWindow(hWnd, r.left, r.top, d.Mode[ModeIndex].Width, d.Mode[ModeIndex].Height, true);
		
		// Get the client window size.
		ClientOnPrimary.x = ClientOnPrimary.y = 0;
		ClientToScreen(hWnd, &ClientOnPrimary);
		RECT rc;
		GetClientRect(hWnd, &rc);
		ClientSize.cx = rc.right;
		ClientSize.cy = rc.bottom;
	} else {
		// In the fullscreen case, we must be careful because if the window
		// frame has been drawn, the client size has shrunk and this can
		// cause problems, so it's best to report the entire screen.
		ClientOnPrimary.x = ClientOnPrimary.y = 0;
		ClientSize.cx = d.Mode[ModeIndex].Width;
		ClientSize.cy = d.Mode[ModeIndex].Height;
	}

	int w, h;

	if (Fullscreen) {
		LastClientSize = ClientSize;
		ClientSize.cx = d.Mode[ModeIndex].Width;
		ClientSize.cy = d.Mode[ModeIndex].Height;
	}
	w = ClientSize.cx;
	h = ClientSize.cy;

	if (Fullscreen) {
		LastError = DD4Interface->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	} else {
		LastError = DD4Interface->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
	}		
	if (LastError != DD_OK ) {
		RenderError e; e << "SetCooperativeLevel failed.
" << D3DErrorToString(LastError);
		throw e;
	}

	if (Fullscreen) {
		// Set the display mode.
//		bIgnoreWM_SIZE = TRUE;
		LastError = DD4Interface->SetDisplayMode(d.Mode[ModeIndex].Width, d.Mode[d.CurrentMode].Height, d.Mode[d.CurrentMode].BPP, 0 /* refresh rate */, 0 /* add'l flags */);
//		bIgnoreWM_SIZE = FALSE;
		if (LastError != DD_OK ) {
			RenderError e; e << "SetDisplayMode to " << d.Mode[d.CurrentMode].Width << "x" << d.Mode[d.CurrentMode].Height << "x" << d.Mode[d.CurrentMode].BPP << "failed
" <<
					D3DErrorToString(LastError);
			throw e;
		}
	}
		
	if (!CreateBuffers(hWnd, w, h, d.Mode[ModeIndex].BPP, Fullscreen, d.Driver[d.CurrentDriver].IsHardware)) {
		RenderError e; e << "Can't create buffers for mode.";
		throw e;
	}
}
	

void	Render::SetD3DDriver(int driver)
// Enables the driver specified by the given index, and sets up the basic
// required stuff.
{
	DeviceInfo&	d = Device[CurrentDevice];
	
	HRESULT	LastError;
	
	// Figure out what our preferred z-buffer format is.
	memset(&ZBufferFormat, 0, sizeof(ZBufferFormat));
	ZBufferFormat.dwSize = 0;
	LastError = D3DInterface->EnumZBufferFormats(d.Driver[driver].Guid, EnumZBufferCallback, (LPVOID) &ZBufferFormat);
	if (LastError != DD_OK) {
		RenderError e; e << "Can't enumerate z-buffer formats.
" << D3DErrorToString(LastError);
		throw e;
	}
	if (ZBufferFormat.dwSize == 0) {
		// Couldn't find a suitable z-buffer format.
		RenderError e; e << "Can't find a suitable z-buffer format.";
		throw e;
	}
	
	// Create the Z-buffer.
	if (CreateZBuffer(ClientSize.cx, ClientSize.cy, driver) == false) {
		RenderError e; e << "Can't create z buffer.";
		throw e;
	}
	
	/*
	 * Create the D3D device, load the textures, call the device create
	 * callback and set a default render state
	 */

	// Create the D3D device and enumerate the texture formats.

	RELEASE(D3DDevice);
	
	if (d.Driver[driver].IsHardware && !BackBufferInVideo) {
		RenderError e; e << "Could not fit the rendering surfaces in video memory for this hardware device.
";
		throw e;
	}

	LastError = D3DInterface->CreateDevice(d.Driver[driver].Guid, BackBuffer, &D3DDevice, NULL);
	if (LastError != DD_OK) {
		RenderError e; e << "Create D3D device failed.
" << D3DErrorToString(LastError);
		throw e;
	}

	// Figure out what our preferred texture format is.
	memset(&PreferredTextureFormat, 0, sizeof(PreferredTextureFormat));
	PreferredTextureFormat.dwSize = 0;	// Signal that we're not initialized.
	LastError = D3DDevice->EnumTextureFormats(EnumTextureCallback, (LPVOID) &PreferredTextureFormat);
	if (LastError != DD_OK) {
		RenderError e; e << "Can't enumerate texture formats.
" << D3DErrorToString(LastError);
		throw e;
	}
	if (PreferredTextureFormat.dwSize == 0) {
		// EnumTextureCallback didn't find a suitable format.
		RenderError e; e << "Can't find a suitable texture format.";
		throw e;
	}

	// Create the material interface.
	RELEASE(MaterialInterface);
	LastError = D3DInterface->CreateMaterial(&MaterialInterface, NULL);
	if (LastError != DD_OK) {
		RenderError e; e << "Create D3D material interface.
" << D3DErrorToString(LastError);
		throw e;
	}

#ifdef NOT
	// Create a material.
	D3DMATERIAL mat;
	memset(&mat, 0, sizeof(D3DMATERIAL));
	mat.dwSize = sizeof(D3DMATERIAL);
	mat.diffuse.r = (D3DVALUE)0.0;
	mat.diffuse.g = (D3DVALUE)0.0;
	mat.diffuse.b = (D3DVALUE)0.0;
	mat.ambient.r = (D3DVALUE)0.0;
	mat.ambient.g = (D3DVALUE)0.0;
	mat.ambient.b = (D3DVALUE)0.0;
	mat.specular.r = (D3DVALUE)0.0;
	mat.specular.g = (D3DVALUE)0.0;
	mat.specular.b = (D3DVALUE)0.0;
	mat.power = (float)40.0;
	mat.hTexture = NULL;
	mat.dwRampSize = 1;
	MaterialInterface->SetMaterial(&mat);
	MaterialInterface->GetHandle(D3DDevice, &Background);

	mat.diffuse.r = (D3DVALUE)0.5;
	mat.diffuse.g = (D3DVALUE)0.5;
	mat.diffuse.b = (D3DVALUE)0.5;
	mat.ambient.r = (D3DVALUE)0.0;
	mat.ambient.g = (D3DVALUE)0.0;
	mat.ambient.b = (D3DVALUE)0.0;
	mat.specular.r = (D3DVALUE)0.0;
	mat.specular.g = (D3DVALUE)0.0;
	mat.specular.b = (D3DVALUE)0.0;
	MaterialInterface->SetMaterial(&mat);
	MaterialInterface->GetHandle(D3DDevice, &Foreground);
#endif // NOT

	// Create a viewport, and attach it to our D3D device.
	RELEASE(Viewport);
	LastError = D3DInterface->CreateViewport(&Viewport, NULL);
	if (LastError  != D3D_OK) {
		RenderError e; e << "Can't create viewport.
" << D3DErrorToString(LastError);
		throw e;
	}
	LastError = D3DDevice->AddViewport(Viewport);
	if (LastError != D3D_OK) {
		RenderError e; e << "Can't add viewport to device.
" << D3DErrorToString(LastError);
		throw e;
	}

	// Set the proper stats for our viewport, based on the current ClientSize info.
	SetupViewport();

	// Set the default renderstate.

	// Enable the z-buffer.
	D3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_USEW);
	D3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
	D3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
	ShadowState.ZBuffering = true;
	EnableZBuffer();
	
	// Enable gouraud shading.
	D3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);

	// Turn off (backface) culling within D3D.
	D3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
	
	// Turn on dithering.
	D3DDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, TRUE);	// Not sure what effect this will have on various hardware.
	
	// We want subpixel precision for rendering.
	D3DDevice->SetRenderState(D3DRENDERSTATE_SUBPIXEL, TRUE);

//	// Texture takes precedence over vertex colors.
	D3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, /* D3DTBLEND_ADD */ /* D3DTBLEND_MODULATE */ D3DTBLEND_DECAL);	// ????
//xxx	D3DDevice->SetTextureStateState(0, ..., ....);

	// Bilinear filtering for magnifying textures.
	D3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_LINEAR);	// xxx mip-map options here...

	// We want perspective correction, always.
	D3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
	
// D3DRENDERSTATE_MIPMAPLODBIAS Interesting.
	
	// Disable alpha testing by default.
	ShadowState.AlphaTesting = false;
	DisableAlphaTest();
	
	// Set the Foreground color as the default.
	D3DDevice->SetLightState(D3DLIGHTSTATE_MATERIAL, Foreground);

//	D3DDevice->SetLightState(D3DLIGHTSTATE_AMBIENT, 0x00FFFFFF);
}

	
BOOL FAR PASCAL	Render::DDEnumCallback(GUID FAR* lpGUID, LPSTR lpDriverDesc, LPSTR lpDriverName, LPVOID lpContext)
// Callback function used during enumeration of DirectDraw drivers.
// The attributes of 3D-capable devices are stored in the Device[] array.
{
	LPDIRECTDRAW lpDD;
	DDCAPS	HELCaps;

	if (DeviceCount >= MAX_DEVICES) {
		return DDENUMRET_OK;
	}
	
	DeviceInfo&	d = Device[DeviceCount];
	
	// Record some basic info about this device.
	strncpy(d.Name, lpDriverName, NAME_MAXLEN);
	d.Name[NAME_MAXLEN-1] = 0;
	strncpy(d.Description, lpDriverDesc, DESC_MAXLEN);
	d.Description[DESC_MAXLEN-1] = 0;
	if (lpGUID) {
		d.IsPrimary = false;
		d.Guid = *lpGUID;
		d.GuidValid = true;
	} else {
		// A NULL lpGUID indicates that this is the primary display device.
		d.IsPrimary = true;
		d.GuidValid = false;
	}

	// Initialize device struct members.
	d.DriverCount = 0;
	d.CurrentDriver = 0;
	d.ModeCount = 0;
	d.CurrentMode = 0;
	
	/*
	 * Create the DirectDraw device using this driver.  If it fails,
	 * just move on to the next driver.
	 */
	if (DirectDrawCreate(d.GuidValid ? &d.Guid : NULL, &lpDD, NULL) != DD_OK) {
		// No go.  Don't increment DeviceCount.
		return DDENUMRET_OK;
	}

	// Need to do this in order to do the subsequent call to QueryInterface() ?? (it was in the docs, but I don't fully understand the requirement)
	HWND	hWnd = Main::GetMainWindow();
	HRESULT	LastError = lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL); 
	if (LastError != DD_OK) {
		RenderError e; e << "lpDD->SetCooperativeLevel() failed.
" << D3DErrorToString(LastError);
		throw e;
	}
		
	// Get the capabilities of this DirectDraw driver.  If it fails,
	// just move on to the next driver.
	memset(&d.DriverCaps, 0, sizeof(DDCAPS));
	d.DriverCaps.dwSize = sizeof(DDCAPS);
	
	memset(&HELCaps, 0, sizeof(DDCAPS));
	HELCaps.dwSize = sizeof(DDCAPS);
	
	if (lpDD->GetCaps(&d.DriverCaps, &HELCaps) != DD_OK) {
		// Can't get caps.  Don't increment DeviceCount.
		lpDD->Release();
		return DDENUMRET_OK;
	}
	
	if (d.DriverCaps.dwCaps & DDCAPS_3D) {
		/*
		 * We have found a 3d hardware device.  Return the DD object
		 * and stop enumeration.
		 */
		d.Does3D = true;
		
		// Add this device's info to our list.
		DeviceCount++;
	}	

	LPDIRECTDRAW4	DD4 = NULL;
	
	// Now create a IDirectDraw4 interface, using the IDirectDraw interface we just created.
	LastError = lpDD->QueryInterface(IID_IDirectDraw4, (LPVOID *) &DD4);
	if (LastError != DD_OK) {
		RenderError e; e << "lpDD->QueryInterface() failed.
" << D3DErrorToString(LastError);
		throw e;
	}
	lpDD->Release();

	// If this is the primary device, then record the current display settings.
	if (d.IsPrimary) {
		// Record the current display mode in Render::WindowsDisplay
		DDSURFACEDESC2 ddsd;
		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		LastError = DD4->GetDisplayMode(&ddsd);
		if (LastError != DD_OK) {
			RenderError e; e << "Getting the current display mode failed.
" << D3DErrorToString(LastError);
			throw e;	
		}
		WindowsDisplay.Width = ddsd.dwWidth;
		WindowsDisplay.Height = ddsd.dwHeight;
		WindowsDisplay.BPP = ddsd.ddpfPixelFormat.dwRGBBitCount;
	}
	
	// Generate the list of available display modes.
	d.ModeCount = 0;
	LastError = DD4->EnumDisplayModes(0, NULL, &d, EnumDisplayModesCallback);
	if (LastError != DD_OK ) {
		RenderError e; e << "EnumDisplayModes failed.
" << D3DErrorToString(LastError);
		throw e;
	}
	
	// Sort the list of display modes
	qsort((void*) &d.Mode[0], (size_t) d.ModeCount, sizeof(ModeInfo), CompareModes);

	LPDIRECT3D3	D3D = NULL;
	
	// Create Direct3D interface.
	LastError = DD4->QueryInterface(IID_IDirect3D3, (LPVOID*) &D3D);
	if (LastError != DD_OK) {
		RenderError e; e << "Creation of IDirect3D failed.
" << D3DErrorToString(LastError);
		throw e;
	}
	
	// Get the available drivers from Direct3D by enumeration.
	d.DriverCount = 0;
	LastError = D3D->EnumDevices(EnumDeviceFunc, &d);
	if (LastError != DD_OK) {
		RenderError e; e << "Enumeration of drivers failed.
" << D3DErrorToString(LastError);
		throw e;
	}

	D3D->Release();
	DD4->Release();

	return DDENUMRET_OK;
}


HRESULT CALLBACK	Render::EnumDisplayModesCallback(LPDDSURFACEDESC2 pddsd, LPVOID lpContext)
// Callback to save the display mode information.
{
	// lpContext points to the DeviceInfo structure we're currently working on.
	DeviceInfo&	d = * (DeviceInfo*) lpContext;
	
	/*
	 * Very large resolutions cause problems on some hardware.  They are also
	 * not very useful for real-time rendering.  We have chosen to disable
	 * them by not reporting them as available.
	 */
	// Also ignore modes below 640x480.
	if (pddsd->dwWidth > 1024 || pddsd->dwHeight > 768 ||
	    pddsd->dwWidth < 640 || pddsd->dwHeight < 480)
	{
		return DDENUMRET_OK;
	}

	// Need at least 15-bit color.
	if (pddsd->ddpfPixelFormat.dwRGBBitCount < 15) {
		return DDENUMRET_OK;
	}
	
	/*
	 * Save this mode at the end of the mode array and increment mode count
	 */
	d.Mode[d.ModeCount].Width = pddsd->dwWidth;
	d.Mode[d.ModeCount].Height = pddsd->dwHeight;
	d.Mode[d.ModeCount].BPP = pddsd->ddpfPixelFormat.dwRGBBitCount;
	d.Mode[d.ModeCount].CurrentDriverCanDo = FALSE;
	d.ModeCount++;
	
	if (d.ModeCount >= MAX_MODES) return DDENUMRET_CANCEL;
	else return DDENUMRET_OK;
}


int	Render::CompareModes(const void* element1, const void* element2)
// Comparison function for qsort(), for ordering display modes.
{
	ModeInfo*	Mode1 = (ModeInfo*) element1;
	ModeInfo*	Mode2 = (ModeInfo*) element2;
	
	if (Mode1->Width > Mode2->Width) return -1;
	else if (Mode2->Width > Mode1->Width) return 1;
	else if (Mode1->Height > Mode2->Height) return -1;
	else if (Mode2->Height > Mode1->Height) return 1;
	else if (Mode1->BPP > Mode2->BPP) return -1;
	else if (Mode2->BPP > Mode1->BPP) return 1;
	else return 0;
}


HRESULT WINAPI	Render::EnumDeviceFunc(LPGUID lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC lpHWDesc, LPD3DDEVICEDESC lpHELDesc, LPVOID lpContext)
// Device enumeration callback.  Record information about the D3D device
// reported by D3D in the DeviceInfo structure pointed to by lpContext.
{
	DeviceInfo&	d = * (DeviceInfo*) lpContext;
	
	/*
	 * Don't accept any hardware D3D devices if emulation-only option is set
	 */
	if (lpHWDesc->dcmColorModel && OnlyEmulation) return D3DENUMRET_OK;
	
	/*
	 * Record the D3D driver's inforamation
	 */
	DriverInfo&	dr = d.Driver[d.DriverCount];
	memcpy(&dr.Guid, lpGuid, sizeof(GUID));
	strncpy(dr.About, lpDeviceDescription, DESC_MAXLEN);
	dr.About[DESC_MAXLEN-1] = 0;
	strncpy(dr.Name, lpDeviceName, NAME_MAXLEN);
	dr.Name[NAME_MAXLEN-1] = 0;
	
	/*
	 * Is this a hardware device or software emulation?  Checking the color
	 * model for a valid model works.
	 */
	// This seems slightly dubious...
	if (lpHWDesc->dcmColorModel) {
		dr.IsHardware = TRUE;
		memcpy(&dr.Desc, lpHWDesc, sizeof(D3DDEVICEDESC));
	} else {
		dr.IsHardware = FALSE;
		memcpy(&dr.Desc, lpHELDesc, sizeof(D3DDEVICEDESC));
	}
	
	/*
	 * Does this driver do texture mapping?
	 */
	dr.DoesTextures = (dr.Desc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) ? TRUE : FALSE;

	/*
	 * Can this driver use a z-buffer?
	 */
	dr.DoesZBuffer = dr.Desc.dwDeviceZBufferBitDepth ? TRUE : FALSE;
	
	/*
	 * Can this driver render to the Windows display depth?
	 */
	dr.CanDoWindow = (dr.Desc.dwDeviceRenderBitDepth & BPPToDDBD(WindowsDisplay.BPP)) ? TRUE : FALSE;

	if (!d.IsPrimary) dr.CanDoWindow = FALSE;

	d.DriverCount++;

	if (d.DriverCount >= MAX_DRIVERS) return (D3DENUMRET_CANCEL);

	return (D3DENUMRET_OK);
}


HRESULT CALLBACK	Render::EnumTextureCallback(DDPIXELFORMAT *PixelFormat, LPVOID Param)
// Gets called with all the various texture formats.  If we see one we like,
// then copy the data into *(DDPIXELFORMAT*) Param.
{
	DDPIXELFORMAT& ddpf = *PixelFormat;
	DDPIXELFORMAT*	Result = static_cast<DDPIXELFORMAT*>(Param);
	
//xxxx	// we use GetDC/BitBlt to init textures so we only
//	// want to use formats that GetDC will support.
//	if (ddpf.dwFlags & (DDPF_ALPHA | DDPF_ALPHAPIXELS)) {
//		return D3DENUMRET_OK;
//	}

//	if (ddpf.dwRGBBitCount <= 8 && !(ddpf.dwFlags & (DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4))) {
//		return DDENUMRET_OK;
//	}

	// We're only really interested in RGB formats. xxx
	if (ddpf.dwFlags & DDPF_RGB == 0 || ddpf.dwRGBBitCount < 16) {
		return D3DENUMRET_OK;
	}
	
//    if (ddpf.dwRGBBitCount > 8 && !(ddpf.dwFlags & DDPF_RGB))
//        return DDENUMRET_OK;

//	// BUGBUG GetDC does not work for 1 or 4bpp YET!
//	if (ddpf.dwRGBBitCount < 8) return D3DENUMRET_OK;

//    // keep the texture format that is nearest to the bitmap we have
//    //
//    if (FindData->ddpf.dwRGBBitCount == 0 ||
//       (ddpf.dwRGBBitCount >= FindData->bpp &&
//       (UINT)(ddpf.dwRGBBitCount - FindData->bpp) < (UINT)(FindData->ddpf.dwRGBBitCount - FindData->bpp)))
//    {
//        FindData->ddpf = ddpf;
//    }

	// We'll take any RGB format, but we prefer 16 bit.
	if (Result->dwSize == 0) {
		// This is the first acceptable format.  Put it in the result structure.
		*Result = ddpf;
	} else if (Result->dwRGBBitCount != 16 && ddpf.dwRGBBitCount == 16) {
		// This format is preferable to the one already in the result structure.
		*Result = ddpf;
	} else if (ddpf.dwRGBAlphaBitMask == 0x8000 && ddpf.dwRBitMask == 0x7C00 && ddpf.dwGBitMask == 0x03E0 && ddpf.dwBBitMask == 0x001F) {
		// What we really want is ARRR RRGG GGGB BBBB.
		*Result = ddpf;
	}
	
	return D3DENUMRET_OK;
}


HRESULT CALLBACK	Render::EnumZBufferCallback(DDPIXELFORMAT *PixelFormat, LPVOID Param)
// Gets called with all the various zbuffer formats.  If we see one we like,
// then copy the data into *(DDPIXELFORMAT*) Param.
{
	DDPIXELFORMAT& ddpf = *PixelFormat;
	DDPIXELFORMAT*	Result = static_cast<DDPIXELFORMAT*>(Param);

	// We want at least 16 bits of z-buffer.
	if ((ddpf.dwFlags & DDPF_ZBUFFER) == 0) {
		// No good.
		return D3DENUMRET_OK;
	}
	if (ddpf.dwZBufferBitDepth < 16) {
		// No good.
		return D3DENUMRET_OK;
	}

	// OK.  Copy into *Result.
	*Result = ddpf;
	
	return D3DENUMRET_OK;
}


bool	Render::CreateBuffers(HWND hwnd, int w, int h, int BPP, bool Fullscreen, bool IsHardware)
// Creates the front and back buffers for the window or fullscreen case
// depending on the Fullscreen flag.  In the window case, bpp is ignored.
{
	DDSURFACEDESC2	ddsd;
	DDSCAPS2	ddscaps;
	HRESULT	LastError = DD_OK;

	// Release any existing objects.
	RELEASE(Clipper);
	RELEASE(BackBuffer);
	RELEASE(FrontBuffer);

	// The sizes of the buffers are going to be w x h, so record it now.
	if (w < WINDOW_MINIMUM) w = WINDOW_MINIMUM;
	if (h < WINDOW_MINIMUM) h = WINDOW_MINIMUM;
	BuffersSize.cx = w;
	BuffersSize.cy = h;
	
	if (Fullscreen) {
		/*
		 * Create a complex flipping surface for fullscreen mode with one
		 * back buffer.
		 */
		memset(&ddsd,0,sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_3DDEVICE | DDSCAPS_COMPLEX;
		ddsd.dwBackBufferCount = 1;

		if (IsHardware) ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		if (OnlySystemMemory) ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		LastError = DD4Interface->CreateSurface(&ddsd, &FrontBuffer, NULL);
		if (LastError != DD_OK) {
			RenderError e;
			if (LastError == DDERR_OUTOFMEMORY || LastError == DDERR_OUTOFVIDEOMEMORY) {
				e << "There was not enough video memory to create the rendering surface.";
			} else {
				e << "CreateSurface for fullscreen flipping surface failed.
" << D3DErrorToString(LastError);
			}
			throw e;
		}

		/* 
		 * Obtain a pointer to the back buffer surface created above so we
		 * can use it later.  For now, just check to see if it ended up in
		 * video memory (FYI).
		 */
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		LastError = FrontBuffer->GetAttachedSurface(&ddscaps, &BackBuffer);
		if (LastError != DD_OK) {
			RenderError e; e << "GetAttachedSurface failed to get back buffer.
" << D3DErrorToString(LastError);
			throw e;
		}

//		memset(&ddsd, 0, sizeof(DDSURFACEDESC));
//		ddsd.dwSize = sizeof(DDSURFACEDESC);
//		LastError = BackBuffer->GetSurfaceDesc(&ddsd);
//		if (LastError != DD_OK) {
//			RenderError e; e << "Failed to get surface description of back buffer.
" << D3DErrorToString(LastError);
//			throw e;
//		}
//
//		BackBufferInVideo = (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ? TRUE : FALSE;
		
	} else {
		/*
		 * In the window case, create a front buffer which is the primary
		 * surface and a back buffer which is an offscreen plane surface.
		 */
		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		/*
		 * If we specify system memory when creating a primary surface, we
		 * won't get the actual primary surface in video memory.  So, don't
		 * use D3DAppICreateSurface().
		 */
		LastError = DD4Interface->CreateSurface(&ddsd, &FrontBuffer, NULL);
		if (LastError != DD_OK ) {
			RenderError e;
			if (LastError == DDERR_OUTOFMEMORY || LastError == DDERR_OUTOFVIDEOMEMORY) {
				e << "There was not enough video memory to create the rendering surface.
";
			} else {
				e << "CreateSurface for window front buffer failed.
" << D3DErrorToString(LastError);
			}
			throw e;
		}
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
		ddsd.dwWidth = w;
		ddsd.dwHeight = h;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		if (IsHardware) {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		} else {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		}
		if (OnlySystemMemory) ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		LastError = DD4Interface->CreateSurface(&ddsd, &BackBuffer, NULL);
		if (LastError != DD_OK) {
			RenderError e;
			if (LastError == DDERR_OUTOFMEMORY || LastError == DDERR_OUTOFVIDEOMEMORY) {
				e << "There was not enough video memory to create the rendering surface.";
			} else {
				e << "CreateSurface for fullscreen flipping surface failed.
" << D3DErrorToString(LastError);
			}
			throw e;
		}

		/*
		 * Create the DirectDraw Clipper object and attach it to the window
		 * and front buffer.
		 */
		LastError = DD4Interface->CreateClipper(0, &Clipper, NULL);
		if (LastError != DD_OK) {
			RenderError e; e << "CreateClipper failed.
" << D3DErrorToString(LastError);
			throw e;
		}
		
		LastError = Clipper->SetHWnd(0, hwnd);
		if (LastError != DD_OK) {
			RenderError e; e << "Attaching clipper to window failed.
" << D3DErrorToString(LastError);
			throw e;
		}
		LastError = FrontBuffer->SetClipper(Clipper);
		if (LastError != DD_OK) {
			RenderError e; e << "Attaching clipper to front buffer failed.
" << D3DErrorToString(LastError);
			throw e;
		}
	}

	/*
	 * Check to see if the back buffer is in video memory (FYI).
	 */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(ddsd);
	LastError = BackBuffer->GetSurfaceDesc(&ddsd);
	if (LastError != DD_OK) {
		RenderError e; e << "Failed to get surface description for back buffer.
" << D3DErrorToString(LastError);
		throw e;
	}
	BackBufferInVideo = (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ? TRUE : FALSE;

	// Get the pixel format of the back buffer.
	BackBufferFormat = ddsd.ddpfPixelFormat;
	
	ClearBuffers();

	return true;
//exit_with_error:
//    RELEASE(d3dappi.lpFrontBuffer);
//    RELEASE(d3dappi.lpBackBuffer);
//    RELEASE(lpClipper);
//    return FALSE;
}


void	Render::ClearBuffers()
// Clear the front and back buffers to black.
{
	ClearBuffer(FrontBuffer);
	ClearBuffer(BackBuffer);
}


void	Render::ClearBuffer(LPDIRECTDRAWSURFACE4 buffer)
// Clears the given surface.
{
	DDSURFACEDESC2	ddsd;
	RECT	dst;
	DDBLTFX	ddbltfx;
	HRESULT	LastError = DD_OK;

	if (buffer) {
		// Find the width and height of the buffer by getting its
		// DDSURFACEDESC
		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(ddsd);
		LastError = buffer->GetSurfaceDesc(&ddsd);
		if (LastError != DD_OK) {
			RenderError e; e << "Failure getting the surface description of the buffer before clearing.
" << D3DErrorToString(LastError);
			throw e;
		}

		// Clear the buffer to black.
		memset(&ddbltfx, 0, sizeof(ddbltfx));
		ddbltfx.dwSize = sizeof(DDBLTFX);
		SetRect(&dst, 0, 0, ddsd.dwWidth, ddsd.dwHeight);
		LastError = buffer->Blt(&dst, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
		if (LastError != DD_OK) {
			RenderError e; e << "Clearing the buffer failed.
" << D3DErrorToString(LastError);
			throw e;
		}
	}
}


bool	Render::CreateZBuffer(int w, int h, int driver)
// Create a Z-Buffer of the appropriate depth and attach it to the back
// buffer.
{
	DDSURFACEDESC2 ddsd;
	DWORD devDepth;
	HRESULT	LastError = DD_OK;

//	// Release any existing Z-Buffer.
//	RELEASE(ZBuffer);

	DeviceInfo&	d = Device[CurrentDevice];
	
	try {
		// If this driver does not do z-buffering, don't create a z-buffer
		if (!d.Driver[driver].DoesZBuffer) return true;
		
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
		ddsd.dwHeight = h;
		ddsd.dwWidth = w;
		
		// If this is a hardware D3D driver, the Z-Buffer MUST end up in video
		// memory.  Otherwise, it MUST end up in system memory.
		if (d.Driver[driver].IsHardware) {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		} else {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		}
		
//		// Get the Z buffer bit depth from this driver's D3D device description
//		devDepth = d.Driver[driver].Desc.dwDeviceZBufferBitDepth;
//		if (devDepth & DDBD_32) {
//			ddsd.dwZBufferBitDepth = 32;
//		} else if (devDepth & DDBD_24) {
//			ddsd.dwZBufferBitDepth = 24;
//		} else if (devDepth & DDBD_16) {
//			ddsd.dwZBufferBitDepth = 16;
//		} else if (devDepth & DDBD_8) {
//			ddsd.dwZBufferBitDepth = 8;
//		} else {
//			RenderError e; e << "Unsupported Z-buffer depth requested by device.";
//			throw e;
//		}
		ddsd.ddpfPixelFormat = ZBufferFormat;
		
		LastError = DD4Interface->CreateSurface(&ddsd, &ZBuffer, NULL);
		if (LastError != DD_OK) {
			RenderError e;
			if (LastError == DDERR_OUTOFMEMORY || LastError == DDERR_OUTOFVIDEOMEMORY) {
				if (Fullscreen) {
					e << "There was not enough video memory to create the Z-buffer surface.
Please restart the program and try another fullscreen mode with less resolution or lower bit depth.";
				} else {
					e << "There was not enough video memory to create the Z-buffer surface.
To run this program in a window of this size, please adjust your display settings for a smaller desktop area or a lower palette size and restart the program.";
				}
			} else {
				e << "CreateSurface for Z-buffer failed.
" << D3DErrorToString(LastError);
			}
			throw e;
		}
		
		// Attach the Z-buffer to the back buffer so D3D will find it
		LastError = BackBuffer->AddAttachedSurface(ZBuffer);
		if (LastError != DD_OK) {
			RenderError e; e << "AddAttachedBuffer failed for Z-Buffer.
" << D3DErrorToString(LastError);
			throw e;
		}
		
		// Find out if it ended up in video memory.
		LastError = ZBuffer->GetSurfaceDesc(&ddsd);
		if (LastError != DD_OK) {
			RenderError e; e << "Failed to get surface description of Z buffer.
" << D3DErrorToString(LastError);
			throw e;
		}
		ZBufferInVideo = (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ? TRUE : FALSE;
		if (d.Driver[driver].IsHardware && !ZBufferInVideo) {
			RenderError e; e << "Could not fit the Z-buffer in video memory for this hardware device.
";
			throw e;
		}
		
	}
	catch (Error& e) {
		if (ZBuffer) {
			ZBuffer->Release();
			ZBuffer = NULL;
		}
		throw e;
	}

	return true;
}


void	Render::ShowFrame()
// Show the back buffer.
{
//	//xxxxx
//	char	buf[80];
//	sprintf(buf, "d3d tex = %.2g             ", TextureDicking.GetMilliseconds());
//	Utility::Log(1, buf);
//	TextureDicking.Reset();
//	//xxxx
	
	HRESULT	LastError;
	
	if (Fullscreen) {
		/*
		 * Flip the back and front buffers
		 */
		LastError = FrontBuffer->Flip(NULL /*BackBuffer*/, 1);

//		RECT front;
//		RECT buffer;
//		// Set rectangles to blt from the back to front buffer.
//		SetRect(&buffer, 0, 0, ClientSize.cx, ClientSize.cy);
//		SetRect(&front, 0, 0, ClientSize.cx, ClientSize.cy);
//		LastError = FrontBuffer->Blt(&front, BackBuffer, &buffer, DDBLT_WAIT, NULL);
		
	} else {
		RECT front;
		RECT buffer;
		// Set rectangles to blt from the back to front buffer.
		SetRect(&buffer, 0, 0, ClientSize.cx, ClientSize.cy);
		SetRect(&front, ClientOnPrimary.x, ClientOnPrimary.y,
			ClientSize.cx + ClientOnPrimary.x, ClientSize.cy + ClientOnPrimary.y);
		LastError = FrontBuffer->Blt(&front, BackBuffer, &buffer, DDBLT_WAIT, NULL);
	}

	if (LastError == DDERR_SURFACELOST) {
		// ???
		FrontBuffer->Restore();
		BackBuffer->Restore();
		ClearBuffers();
	} else if (LastError != DD_OK) {
//		D3DAppISetErrorString("Flipping complex display surface failed.
%s", D3DAppErrorToString(LastError));
//		return FALSE;
		return;
	}
}


void	Render::ClearFrame()
// Erase the back buffer.
{
	ClearBuffer(BackBuffer);
}


void	Render::ClearZBuffer()
// Erase the z buffer.
{
	// ????
	/*
	 * Clear the dirty rectangles on the Z buffer
	 */
	D3DRECT rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = ClientSize.cx;
	rect.y2 = ClientSize.cy;
	
	HRESULT r = Viewport->Clear(1, &rect, D3DCLEAR_ZBUFFER);
	if (r != D3D_OK) {
		RenderError e; e << "Viewport clear of Z buffer failed.
" << D3DErrorToString(r);
		throw e;
	}
}


int	Render::GetWindowWidth()
// Returns the width of the rendering window, in pixels.
{
	return ClientSize.cx;
}


int	Render::GetWindowHeight()
// Returns the height of the rendering window, in pixels.
{
	return ClientSize.cy;
}


void	Render::BeginFrame()
// Call this before starting to draw a frame.
{
	if (FrontBuffer->IsLost() == DDERR_SURFACELOST)
	{
		FrontBuffer->Restore();
//		Texture.Restore();
	}

	D3DDevice->BeginScene();
}


void	Render::EndFrame()
// Call this after you're done drawing the frame, before showing it.
{
	D3DDevice->EndScene();
}


void	Render::EnableZBuffer()
// Enables the zbuffer.  You could turn it off to do the backdrop, for example.
{
	DesiredState.ZBuffering = true;
}


void	Render::DisableZBuffer()
// Disables the zbuffer.  Handy for drawing a backdrop, for example.
{
	DesiredState.ZBuffering = false;
}


void	Render::EnableAlphaTest()
// Enables alpha-test transparency.  Well, it's not really transparency; it might be better
// called "knockouts" or something.  Anyway, it prevents any pixel with an alpha value below 0.5
// from being rendered.
{
	DesiredState.AlphaTesting = true;
}


void	Render::DisableAlphaTest()
// Turns off alpha-test "transparency".
{
	DesiredState.AlphaTesting = false;
}


void	Render::EnableLightmapBlend()
// Enables the lightmap-blending mode.
{
	DesiredState.LightmapBlending = true;
}


void	Render::DisableLightmapBlend()
// Disables the lightmap-blending mode.
{
	DesiredState.LightmapBlending = false;
}


void	Render::CommitRenderState()
// Communicates any rendering state changes to the D3D driver.  Compares
// our internal shadow of D3D's rendering state with our own desired
// rendering state to avoid sending changes that aren't needed.
{
	if (D3DDevice == 0) return;

	// Set the z-buffer to the proper state.
	if (DesiredState.ZBuffering != ShadowState.ZBuffering) {
		if (DesiredState.ZBuffering) {
			// Enable the z-buffer.
			D3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
			D3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
			D3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
		} else {
			// Disable the z-buffer.
			D3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
			D3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
//			D3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
		}
		ShadowState.ZBuffering = DesiredState.ZBuffering;
	}

	// Set alpha-testing to the proper state.
	if (DesiredState.AlphaTesting != ShadowState.AlphaTesting) {
		if (DesiredState.AlphaTesting) {
//			D3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);

//			D3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
			D3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER/*EQUAL*/);
			D3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0x08001);
			D3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
//			D3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
//			D3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCALPHA);
			
		} else {
//			D3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);

			D3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
		}
		ShadowState.AlphaTesting = DesiredState.AlphaTesting;
	}

	// Enable/disable lightmap blending.
	if (DesiredState.LightmapBlending != ShadowState.LightmapBlending) {
		if (DesiredState.LightmapBlending) {
			D3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
			D3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);		// ???
			D3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);	// ???
		} else {
			D3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
			D3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE /* ZERO /* INVSRCCOLOR */);		// ???
			D3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);	// ???
		}
		ShadowState.LightmapBlending = DesiredState.LightmapBlending;
	}

	// Set color keying to the proper state.
	if (DesiredState.ColorKeying != ShadowState.ColorKeying) {
		if (DesiredState.ColorKeying) {
			D3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
		} else {
			D3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
		}
		ShadowState.ColorKeying = DesiredState.ColorKeying;
	}
		
	// Set the texture.
	if (DesiredState.Texture != ShadowState.Texture) {
		if (DesiredState.Texture) {
			if (DesiredState.Texture->Valid == false) {
				DesiredState.Texture->Restore();
			}
			
			if (DesiredState.Texture->DeviceSurface->IsLost() != DD_OK) {
				DesiredState.Texture->Restore();
			}
			
			if (DesiredState.Texture) {
//				D3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, DesiredState.Texture->Handle);
				D3DDevice->SetTexture(0, DesiredState.Texture->DeviceTexture);
			} else {
//				D3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, NULL);
				D3DDevice->SetTexture(0, NULL);
			}
		} else {
			// No texturing.
//			D3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, NULL);
			D3DDevice->SetTexture(0, NULL);
		}
		
		ShadowState.Texture = DesiredState.Texture;
	}
}

	
// D3DRENDERSTATE_COLORKEYENABLE Enables chroma-key, I think.

// "This render state was introduced in DirectX 5. Applications should
// check the D3DDEVCAPS_DRAWPRIMTLVERTEX flag in the D3DDEVICEDESC
// structure to find out whether this render state is supported."


// Here's some info from the DirectX help files concerning color-keys.

//The IDirectDrawSurface3::SetColorKey method sets the color key value for the DirectDrawSurface object if the hardware supports color keys on a per surface basis.
//
//HRESULT SetColorKey(
//  DWORD dwFlags,             
//  LPDDCOLORKEY lpDDColorKey  
//);
//DDCKEY_SRCBLT
//Set if the structure specifies a color key or color space to be used as a source color key for blit operations. 
//DDCKEY_SRCOVERLAY
//Set if the structure specifies a color key or color space to be used as a source color key for overlay operations. 
//lpDDColorKey
//
//Address of the DDCOLORKEY structure that contains the new color key values for the DirectDrawSurface object.
//
//typedef struct _DDCOLORKEY{ 
//    DWORD dwColorSpaceLowValue; 
//    DWORD dwColorSpaceHighValue; 
//} DDCOLORKEY,FAR* LPDDCOLORKEY; 


void	Render::SetTexture(Texture* t)
// Makes the given texture the current texture for drawing.
{
	DesiredState.Texture = static_cast<D3DTexture*>(t);
}


#ifdef NOT

void	Render::ConvertVertex(D3DTLVERTEX* result, const Vertex& v)
// Convert the information from the given Vertex to the equivalent info
// in the given D3DTLVERTEX.
{
	result->sx = /* XOffset - */ v.GetVector().X();
	result->sy = /* YOffset - */ v.GetVector().Y();
	result->sz = v.GetVector().Z();
	result->rhw = v.GetW();
	result->color = v.GetColor();
	result->specular = 0;
	result->tu = v.GetU();
	result->tv = v.GetV();
}

#endif // NOT


void	Render::DrawTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
// Draw a triangle.
{
	// Make sure all rendering states are committed.
	CommitRenderState();

	// Wish we could eliminate these copies.  Can do it by using
	// Begin(), Vertex(), End(), but it ends up being slower anyway.
	// Stripping and fanning may solve it eventually.
	Render::Vertex	array[3];
	array[0] = a;
	array[1] = b;
	array[2] = c;

	HRESULT r = D3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,
					     D3DFVF_XYZRHW |
					     D3DFVF_DIFFUSE |
					     D3DFVF_TEX1,
					     array, 3 /* vert. count */,
					     D3DDP_DONOTCLIP |
					     D3DDP_DONOTUPDATEEXTENTS |
					     D3DDP_DONOTLIGHT
					     /* | D3DDP_WAIT */
					    );

//	if (r != DD_OK) {
//		RenderError e; e << "DrawPrim error
" << D3DErrorToString(r);
//		throw e;
//	}
}


void	Render::DrawQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d)
// Draw a quadrangle.  Should be planar.
{
	// Make sure all rendering states are committed.
	CommitRenderState();

	// Wish we could eliminate these copies.  Can do it by using
	// Begin(), Vertex(), End(), but it ends up being slower anyway.
	// Stripping and fanning may solve it eventually.
	Render::Vertex	array[4];
	array[0] = a;
	array[1] = b;
	array[2] = c;
	array[3] = d;

	HRESULT r = D3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN,
					     D3DFVF_XYZRHW |
					     D3DFVF_DIFFUSE |
					     D3DFVF_TEX1,
					     array, 4 /* vert. count */,
					     D3DDP_DONOTCLIP |
					     D3DDP_DONOTUPDATEEXTENTS |
					     D3DDP_DONOTLIGHT
					     /* | D3DDP_WAIT */
					    );

//	if (r != DD_OK) {
//		RenderError e; e << "DrawPrim error
" << D3DErrorToString(r);
//		throw e;
//	}
}


void	Render::DrawLine(const Vertex& a, const Vertex& b)
// Draw a line.
{
	// Make sure all rendering states are committed.
	CommitRenderState();

	Render::Vertex	array[2];
	array[0] = a;
	array[1] = b;

	HRESULT	r = D3DDevice->DrawPrimitive(D3DPT_LINELIST,
					     D3DFVF_XYZRHW |
					     D3DFVF_DIFFUSE |
					     D3DFVF_TEX1,
					     array, 2 /* vert. count */,
					     D3DDP_DONOTCLIP |
					     D3DDP_DONOTUPDATEEXTENTS |
					     D3DDP_DONOTLIGHT
					     /* | D3DDP_WAIT */
					    );
}


//
// Texture stuff
//


// Keep a list of textures, so we can invalidate them when changing modes, etc.
D3DTexture*	TextureList = 0;


Texture*	Render::NewTexture(const char* Filename)
// Creates a new texture and registers it.
{
	return new D3DTexture(Filename);
}


Texture*	Render::NewTextureFromBitmap(bitmap32* b)
// Creates a new texture from the given bitmap pixel data and registers it.
{
	return new D3DTexture(b);
}


void	Render::InvalidateTextures()
// Marks all textures as invalid.  Forces textures to be rebuilt.
{
	D3DTexture*	e;
	for (e = TextureList; e; e = e->Next) {
		e->Invalidate();
	}
	
	ShadowState.Texture = 0;
}


Render::D3DTexture::D3DTexture(const char* FileOrResourceName)
// Initializes components.
{
	Valid = false;
	Width = Height = 0;
	MemorySurface = NULL;
	DeviceSurface = NULL;
//	Handle = NULL;
	DeviceTexture = NULL;
//	ColorKey = 0;
//	ColorKeyEnabled = false;
	Next = NULL;
	Previous = NULL;

	BuildSystemMemorySurface(FileOrResourceName);
	Restore();

	// Add to the texture list.
	Next = TextureList;
	Previous = NULL;
	TextureList = this;
}


Render::D3DTexture::D3DTexture(bitmap32* b)
// Initializes components.
{
	Valid = false;
	Width = b->GetWidth();
	Height = b->GetHeight();
	MemorySurface = NULL;
	DeviceSurface = NULL;
//	Handle = NULL;
	DeviceTexture = NULL;
//	ColorKey = 0;
//	ColorKeyEnabled = false;
	Next = NULL;
	Previous = NULL;

//	TextureDicking.Start();	//xxxxxxxx
	BuildSystemMemorySurface(b->GetData());
	Restore();
//	TextureDicking.Stop();	//xxxxxxx

	// Add to the texture list.
	// Add to the texture list.
	Next = TextureList;
	Previous = NULL;
	TextureList = this;
}


void	Render::D3DTexture::Restore()
// Restores the device surface and copies the image from the memory surface
// to the device surface.
{
	HRESULT             err;
	
	if (MemorySurface == NULL) {
		RenderError e; e << "Render::D3DTexture::Restore(): MemorySurface == NULL!";
		throw e;
	}

	//
	// we dont need to do this step for system memory surfaces.
	//
	if (DeviceSurface == MemorySurface) {
		return /* TRUE */;
	}

	// Create the video-memory surface if necessary.
	if (DeviceSurface == NULL) {
		DDSURFACEDESC2	d;
		memset(&d, 0, sizeof(d));
		d.dwSize = sizeof(d);
		d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
		
		d.dwHeight = Height;
		d.dwWidth = Width;
//		d.dwMipMapCount = possibly interesting;
		
		d.ddpfPixelFormat = PreferredTextureFormat;

		// Create a video-memory surface.
		d.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD;
		err = DD4Interface->CreateSurface(&d, &DeviceSurface, NULL);
		if (err != DD_OK) {
			NewTextureError e; e << "D3DTexture::Restore(); CreateSurface(&DeviceSurface) failed.
" << D3DErrorToString(err);
			throw e;
		}
	} else {
		//
		// restore the video memory texture.
		//
		if (DeviceSurface->Restore() != DD_OK) {
//			return /* FALSE */;
		}
	}

	//
	// call IDirect3DTexture::Load() to copy the texture to the device.
	//
	IDirect3DTexture2  *MemoryTexture = NULL;
//	IDirect3DTexture2  *DeviceTexture = NULL;
	DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&DeviceTexture);
	MemorySurface->QueryInterface(IID_IDirect3DTexture2, (void**)&MemoryTexture);
	
	err = DeviceTexture->Load(MemoryTexture);
	if (err != DD_OK) {
		RenderError e; e << "->Load() failed.
" << D3DErrorToString(err);
		throw e;
	}
	
//	// Attach a color key to this texture, if needed.
//	if (ColorKeyEnabled) {
//		DDCOLORKEY	Key;
//		Key.dwColorSpaceLowValue = ColorKey;
//		Key.dwColorSpaceHighValue = Key.dwColorSpaceLowValue;
//		err = DeviceSurface->SetColorKey(DDCKEY_SRCBLT, &Key);
//		if (err != DD_OK) {
//			NewTextureError e; e << "Can't SetColorKey.
" << D3DErrorToString(err);
//			throw e;
//		}
//	}
	
//	// Get a pointer to the texture.
//	err = DeviceTexture->GetHandle(D3DDevice, &Handle);
//	if (err != DD_OK) {
//		RenderError e; e << "Can't get handle from surface.
" << D3DErrorToString(err);
//		throw e;
//	}
//    
//	DeviceTexture->Release();
	MemoryTexture->Release();

	Valid = true;
	
	return;
}


Render::D3DTexture::~D3DTexture()
// Free the components and unlink from the texture list.
{
	if (DesiredState.Texture == this) {
		DesiredState.Texture = NULL;
	}

//	TextureDicking.Start();	//xxxx
	Invalidate();
	if (MemorySurface) {
		MemorySurface->Release();
		MemorySurface = NULL;
	}
//	TextureDicking.Stop();	//xxxxx

	// Unlink from the texture list.
	if (Previous) {
		Previous->Next = Next;
	} else {
		TextureList = Next;
	}
	if (Next) {
		Next->Previous = Previous;
	}
}


void	Render::D3DTexture::Invalidate()
// Marks this texture as invalid.
// Releases any device-memory surface associated with the texture.
{
	if (Valid) {
		Valid = false;
		DeviceSurface->Release();
		DeviceSurface = NULL;
	}
}


//void	Render::D3DTexture::SetColorKey(int RGBColor)
//// Set the color key of this texture.  The color key will create chromakey-style
//// transparency between calls to Render::EnableColorKey()/Render::DisableColorKey().
//{
//	// Set flags and color key value.
//	ColorKeyEnabled = true;
//	ColorKey = RGBColor;
//
//	// Update the DD device surface.
//	Restore();
//}


void	Render::D3DTexture::BuildSystemMemorySurface(const char* FileName)
// Builds the system memory texture by loading our bitmap file.
{
	// Load .PSD file.
	
	// Try to load the bitmap.
	bitmap32*	b = PSDRead::ReadImageData32(FileName);
	if (b == NULL) {
		NewTextureError e; e << "Can't load bitmap named '" << FileName << "'.";
		throw e;
	}

	Width = b->GetWidth();
	Height = b->GetHeight();
	
	// Get ready to create DD surfaces.
	DDSURFACEDESC2	d;
	memset(&d, 0, sizeof(d));
	d.dwSize = sizeof(d);
	d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
	
	d.dwHeight = Height;
	d.dwWidth = Width;
	
	d.ddpfPixelFormat = PreferredTextureFormat;
	
	// Create the system memory surface.
	d.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE;
	HRESULT	err = DD4Interface->CreateSurface(&d, &MemorySurface, NULL);
	if (err != DD_OK) {
		NewTextureError e; e << "D3DTexture::BSMS(); CreateSurface() failed.
" << D3DErrorToString(err);
		throw e;
	}
	
	// Copy the bitmap from the image in memory to the system memory surface.
	DDSURFACEDESC2	Desc;
	Desc.dwSize = sizeof(Desc);
	err = MemorySurface->Lock(NULL, &Desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY /**/, NULL);
	if (err != DD_OK) {
		NewTextureError e; e << "D3DTexture::BuildSystemMemorySurface(): Lock() failed.
" << D3DErrorToString(err);
		throw e;
	}
	
	Copy32BitArrayToSurface(b->GetData(), Width, Height, Desc);
	
	MemorySurface->Unlock(NULL);
	
	// Free the image.
	delete b;
}

	
void	Render::Copy32BitArrayToSurface(uint32* Data, int Width, int Height, DDSURFACEDESC2& Desc)
// Copies the pixel data from the 32-bit ARGB pixel array pointed to by Data
// to the directdraw surface memory pointed to by ptr and described by
// Desc.
{
	int	Pitch;
	if (Desc.dwFlags & DDSD_PITCH) Pitch = Desc.lPitch;
	else Pitch = Width;

	if (Desc.dwFlags & DDSD_PIXELFORMAT == 0) return;
	if (Desc.dwFlags & DDSD_LPSURFACE == 0) return;

	DDPIXELFORMAT&	pf = Desc.ddpfPixelFormat;
	if (pf.dwFlags & DDPF_RGB == 0) return;

	if (pf.dwRGBBitCount != 16) {
		// Handle >16 cases.
		return;
	}

	uint16	rmask = pf.dwRBitMask;
	uint16	gmask = pf.dwGBitMask;
	uint16	bmask = pf.dwBBitMask;
	uint16	amask = pf.dwRGBAlphaBitMask;

	uint16*	destrow = static_cast<uint16*>(Desc.lpSurface);

	if (rmask == 0xF800 && gmask == 0x07E0 && bmask == 0x001F) {
		// 565: RRRR RGGG GGGB BBBB (e.g. Voodoo, without alpha bit).
		for (int j = 0; j < Height; j++) {
			uint16*	dest = destrow;
			for (int i = 0; i < Width; i++) {
				uint32	src = *Data++;
				*dest++ = /* ((src & 0x80000000) >> 16) | */
					  ((src & 0x00F80000) >> 8) |
					  ((src & 0x0000FC00) >> 5) |
					  ((src & 0x000000F8) >> 3);
			}
			destrow = (uint16*) (((byte*)(destrow)) + Pitch);
		}
	} else if (rmask == 0x7C00 && gmask == 0x03E0 && bmask == 0x001F) {
		// 1555: RGB data is ARRR RRGG GGGB BBBB
		for (int j = 0; j < Height; j++) {
			uint16*	dest = destrow;
			for (int i = 0; i < Width; i++) {
				uint32	src = *Data++;
				*dest++ = ((src & 0x80000000) >> 16) |
					  ((src & 0x00F80000) >> 9) |
					  ((src & 0x0000F800) >> 6) |
					  ((src & 0x000000F8) >> 3);
			}
			destrow = (uint16*) (((byte*)(destrow)) + Pitch);
		}
	} else {
		// Do it the slow way...
//		????;
	}
}

	
void	Render::D3DTexture::BuildSystemMemorySurface(uint32* data)
// Builds the system memory texture from the given array of 32-bit ARGB pixels.
{
//	HBITMAP	hbm;
//	BITMAP	bm;
	DDSURFACEDESC2	ddsd;
 
	// Get ready to create DD surfaces.
	DDSURFACEDESC2	d;
	memset(&d, 0, sizeof(d));
	d.dwSize = sizeof(d);
	d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;

	d.dwWidth = Width;
	d.dwHeight = Height;

	d.ddpfPixelFormat = PreferredTextureFormat;

	// Create the system memory surface.
	d.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE;
	HRESULT	err = DD4Interface->CreateSurface(&d, &MemorySurface, NULL);
	if (err != DD_OK) {
		NewTextureError e; e << "D3DArrayTexture::BSMS(): CreateSurface() failed.
" << D3DErrorToString(err);
		throw e;
	}

	DDSURFACEDESC2	Desc;
	Desc.dwSize = sizeof(Desc);
	err = MemorySurface->Lock(NULL, &Desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
	if (err != DD_OK) {
		NewTextureError e; e << "D3DArrayTexture::BSMS(): Lock failed.
" << D3DErrorToString(err);
		throw e;
	}

	Copy32BitArrayToSurface(data, Width, Height, Desc);
	
	MemorySurface->Unlock(NULL);
}


void	D3DTexture::ReplaceImage(bitmap32* b)
// Replaces the current image data in this texture with the given image data.
{
//	FunctionTimer(TextureDicking);	//xxxxxxx
	
	DDSURFACEDESC2	Desc;
	Desc.dwSize = sizeof(Desc);
	HRESULT	err = MemorySurface->Lock(NULL, &Desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
	if (err != DD_OK) {
		NewTextureError e; e << "D3DArrayTexture::ReplaceImage(): Lock failed.
" << D3DErrorToString(err);
		throw e;
	}

	Copy32BitArrayToSurface(b->GetData(), Width, Height, Desc);
	
	MemorySurface->Unlock(NULL);

	Restore();
}


//
// DDImage
//


Image*	Render::NewImage(char* Filename)
// Creates and returns a new Image from the given bitmap file.
{
	bitmap32*	b = PSDRead::ReadImageData32(Filename);
	if (b == NULL) {
		Error e; e << "Can't load image named '" << Filename << "'.";
		throw e;
	}
	
	// Turn the alpha channel into color 0, and perturb any non-transparent color 0 in the bitmap so it
	// doesn't get dropped out.
//	b->ProcessForColorKeyZero();
	
	Image*	i = NewImageFromBitmap(b);
	delete b;

//	// We want color 0 to be transparent.
//	i->SetColorKey(0);
	
	return i;
}


Image*	Render::NewImageFromBitmap(bitmap32* b)
// Creates a new image from the given bitmap pixel data and registers it.
{
	return new DDImage(b);
}


Render::DDImage::DDImage(bitmap32* b)
// Create the DD image and fill it with the given image data.
{
	Width = b->GetWidth();
	Height = b->GetHeight();
	MemorySurface = NULL;
	DeviceSurface = NULL;
	ColorKey = 0;
	ColorKeyEnabled = false;

	// Turn the alpha channel into color 0, and perturb any non-transparent color 0 in the bitmap so it
	// doesn't get dropped out.
	b->ProcessForColorKeyZero();
	
	BuildSystemMemorySurface(b->GetData());
	Restore();

	// Add to the image list?

	// We want color 0 to be transparent.
	SetColorKey(0);
}


Render::DDImage::~DDImage()
// Destructor.  Free resources.
{
	// unlink?
	
	if (MemorySurface) {
		MemorySurface->Release();
		MemorySurface = NULL;
	}
	if (DeviceSurface) {
		DeviceSurface->Release();
		DeviceSurface = NULL;
	}
}


void	Render::DDImage::SetColorKey(int RGBColor)
// Establishes the color key for this image.
{
	ColorKey = RGBColor;
	ColorKeyEnabled = true;
	Restore();
}


void	Render::DDImage::BuildSystemMemorySurface(uint32* data)
// Creates a system memory surface, and copies the given pixel
// data into it.
{
	// Get ready to create DD surfaces.
	DDSURFACEDESC2	d;
	memset(&d, 0, sizeof(d));
	d.dwSize = sizeof(d);
	d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;

	d.dwWidth = Width;
	d.dwHeight = Height;

	d.ddpfPixelFormat = BackBufferFormat;

	// Create the system memory surface.
	d.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
	HRESULT	err = DD4Interface->CreateSurface(&d, &MemorySurface, NULL);
	if (err != DD_OK) {
		ImageError e; e << "DDImage::BSMS(): CreateSurface() failed.
" << D3DErrorToString(err);
		throw e;
	}

	DDSURFACEDESC2	Desc;
	Desc.dwSize = sizeof(Desc);
	err = MemorySurface->Lock(NULL, &Desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
	if (err != DD_OK) {
		ImageError e; e << "DDImage::BSMS(): Lock failed.
" << D3DErrorToString(err);
		throw e;
	}

	Copy32BitArrayToSurface(data, Width, Height, Desc);
	
	MemorySurface->Unlock(NULL);
}


void	Render::DDImage::Restore()
// Creates and copies the data from the system-memory surface to the device
// surface, as needed.
{
	HRESULT             err;
	
	if (MemorySurface == NULL) {
		ImageError e; e << "Render::DDImage::Restore(): MemorySurface == NULL!";
		throw e;
	}

	//
	// we dont need to do this step for system memory surfaces.
	//
	if (DeviceSurface == MemorySurface) {
		return /* TRUE */;
	}

	// Create the video-memory surface if necessary.
	if (DeviceSurface == NULL) {
		DDSURFACEDESC2	d;
		memset(&d, 0, sizeof(d));
		d.dwSize = sizeof(d);
		d.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
		
		d.dwHeight = Height;
		d.dwWidth = Width;
		
		d.ddpfPixelFormat = BackBufferFormat;

		// Create a video-memory surface.
		d.ddsCaps.dwCaps = /* DDSCAPS_VIDEOMEMORY |*/ DDSCAPS_OFFSCREENPLAIN /* | DDSCAPS_ALLOCONLOAD */;
		err = DD4Interface->CreateSurface(&d, &DeviceSurface, NULL);
		if (err != DD_OK) {
			ImageError e; e << "DDImage::Restore(); CreateSurface(&DeviceSurface) failed.
" << D3DErrorToString(err);
			throw e;
		}
	} else {
		//
		// restore the video memory texture.
		//
		if (DeviceSurface->Restore() != DD_OK) {
//			return /* FALSE */;
		}
	}

	// Do a Blt to copy the memory surface to the device surface.
	RECT	rect;
	SetRect(&rect, 0, 0, Width, Height);
	DDBLTFX	ddfx;
	memset(&ddfx, 0, sizeof(ddfx));
	ddfx.dwSize = sizeof(ddfx);
	ddfx.dwROP = SRCCOPY;
	err = DeviceSurface->Blt(&rect, MemorySurface, &rect, DDBLT_WAIT | DDBLT_ROP, &ddfx);
	if (err != DD_OK) {
		ImageError e; e << "DDImage::Restore(): Can't Blt.
" << D3DErrorToString(err);
		throw e;
	}
	
	// Attach a color key to this image, if needed.
	if (ColorKeyEnabled) {
		DDCOLORKEY	Key;
		Key.dwColorSpaceLowValue = ColorKey;
		Key.dwColorSpaceHighValue = Key.dwColorSpaceLowValue;
		err = DeviceSurface->SetColorKey(DDCKEY_SRCBLT, &Key);
		if (err != DD_OK) {
			ImageError e; e << "Can't SetColorKey.
" << D3DErrorToString(err);
			throw e;
		}
	}
	
//	Valid = true;
	
	return;
}


// misc utility functions.


DWORD	Render::BPPToDDBD(int bpp)
// Convert an integer bit per pixel number to a DirectDraw bit depth flag.
{
	switch(bpp) {
	case 1:
		return DDBD_1;
	case 2:
		return DDBD_2;
	case 4:
		return DDBD_4;
	case 8:
		return DDBD_8;
	case 16:
		return DDBD_16;
	case 24:
		return DDBD_24;
	case 32:
		return DDBD_32;
	default:
		return (DWORD)-1;
	}
}


char*	Render::D3DErrorToString(HRESULT error)
// Looks up an error message based on the given error code.
{
    switch(error) {
        case DD_OK:
            return "No error.\0";
        case DDERR_ALREADYINITIALIZED:
            return "This object is already initialized.\0";
        case DDERR_BLTFASTCANTCLIP:
            return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
        case DDERR_CANNOTATTACHSURFACE:
            return "This surface can not be attached to the requested surface.\0";
        case DDERR_CANNOTDETACHSURFACE:
            return "This surface can not be detached from the requested surface.\0";
        case DDERR_CANTCREATEDC:
            return "Windows can not create any more DCs.\0";
        case DDERR_CANTDUPLICATE:
            return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
        case DDERR_CLIPPERISUSINGHWND:
            return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
        case DDERR_COLORKEYNOTSET:
            return "No src color key specified for this operation.\0";
        case DDERR_CURRENTLYNOTAVAIL:
            return "Support is currently not available.\0";
        case DDERR_DIRECTDRAWALREADYCREATED:
            return "A DirectDraw object representing this driver has already been created for this process.\0";
        case DDERR_EXCEPTION:
            return "An exception was encountered while performing the requested operation.\0";
        case DDERR_EXCLUSIVEMODEALREADYSET:
            return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
        case DDERR_GENERIC:
            return "Generic failure.\0";
        case DDERR_HEIGHTALIGN:
            return "Height of rectangle provided is not a multiple of reqd alignment.\0";
        case DDERR_HWNDALREADYSET:
            return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
        case DDERR_HWNDSUBCLASSED:
            return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
        case DDERR_IMPLICITLYCREATED:
            return "This surface can not be restored because it is an implicitly created surface.\0";
        case DDERR_INCOMPATIBLEPRIMARY:
            return "Unable to match primary surface creation request with existing primary surface.\0";
        case DDERR_INVALIDCAPS:
            return "One or more of the caps bits passed to the callback are incorrect.\0";
        case DDERR_INVALIDCLIPLIST:
            return "DirectDraw does not support the provided cliplist.\0";
        case DDERR_INVALIDDIRECTDRAWGUID:
            return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
        case DDERR_INVALIDMODE:
            return "DirectDraw does not support the requested mode.\0";
        case DDERR_INVALIDOBJECT:
            return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
        case DDERR_INVALIDPARAMS:
            return "One or more of the parameters passed to the function are incorrect.\0";
        case DDERR_INVALIDPIXELFORMAT:
            return "The pixel format was invalid as specified.\0";
        case DDERR_INVALIDPOSITION:
            return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
        case DDERR_INVALIDRECT:
            return "Rectangle provided was invalid.\0";
        case DDERR_LOCKEDSURFACES:
            return "Operation could not be carried out because one or more surfaces are locked.\0";
        case DDERR_NO3D:
            return "There is no 3D present.\0";
        case DDERR_NOALPHAHW:
            return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
        case DDERR_NOBLTHW:
            return "No blitter hardware present.\0";
        case DDERR_NOCLIPLIST:
            return "No cliplist available.\0";
        case DDERR_NOCLIPPERATTACHED:
            return "No clipper object attached to surface object.\0";
        case DDERR_NOCOLORCONVHW:
            return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
        case DDERR_NOCOLORKEY:
            return "Surface doesn't currently have a color key\0";
        case DDERR_NOCOLORKEYHW:
            return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
        case DDERR_NOCOOPERATIVELEVELSET:
            return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
        case DDERR_NODC:
            return "No DC was ever created for this surface.\0";
        case DDERR_NODDROPSHW:
            return "No DirectDraw ROP hardware.\0";
        case DDERR_NODIRECTDRAWHW:
            return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
        case DDERR_NOEMULATION:
            return "Software emulation not available.\0";
        case DDERR_NOEXCLUSIVEMODE:
            return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
        case DDERR_NOFLIPHW:
            return "Flipping visible surfaces is not supported.\0";
        case DDERR_NOGDI:
            return "There is no GDI present.\0";
        case DDERR_NOHWND:
            return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
        case DDERR_NOMIRRORHW:
            return "Operation could not be carried out because there is no hardware present or available.\0";
        case DDERR_NOOVERLAYDEST:
            return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
        case DDERR_NOOVERLAYHW:
            return "Operation could not be carried out because there is no overlay hardware present or available.\0";
        case DDERR_NOPALETTEATTACHED:
            return "No palette object attached to this surface.\0";
        case DDERR_NOPALETTEHW:
            return "No hardware support for 16 or 256 color palettes.\0";
        case DDERR_NORASTEROPHW:
            return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
        case DDERR_NOROTATIONHW:
            return "Operation could not be carried out because there is no rotation hardware present or available.\0";
        case DDERR_NOSTRETCHHW:
            return "Operation could not be carried out because there is no hardware support for stretching.\0";
        case DDERR_NOT4BITCOLOR:
            return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
        case DDERR_NOT4BITCOLORINDEX:
            return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
        case DDERR_NOT8BITCOLOR:
            return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
        case DDERR_NOTAOVERLAYSURFACE:
            return "Returned when an overlay member is called for a non-overlay surface.\0";
        case DDERR_NOTEXTUREHW:
            return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
        case DDERR_NOTFLIPPABLE:
            return "An attempt has been made to flip a surface that is not flippable.\0";
        case DDERR_NOTFOUND:
            return "Requested item was not found.\0";
        case DDERR_NOTLOCKED:
            return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
        case DDERR_NOTPALETTIZED:
            return "The surface being used is not a palette-based surface.\0";
        case DDERR_NOVSYNCHW:
            return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
        case DDERR_NOZBUFFERHW:
            return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
        case DDERR_NOZOVERLAYHW:
            return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
        case DDERR_OUTOFCAPS:
            return "The hardware needed for the requested operation has already been allocated.\0";
        case DDERR_OUTOFMEMORY:
            return "DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OUTOFVIDEOMEMORY:
            return "DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OVERLAYCANTCLIP:
            return "The hardware does not support clipped overlays.\0";
        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
            return "Can only have ony color key active at one time for overlays.\0";
        case DDERR_OVERLAYNOTVISIBLE:
            return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
        case DDERR_PALETTEBUSY:
            return "Access to this palette is being refused because the palette is already locked by another thread.\0";
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            return "This process already has created a primary surface.\0";
        case DDERR_REGIONTOOSMALL:
            return "Region passed to Clipper::GetClipList is too small.\0";
        case DDERR_SURFACEALREADYATTACHED:
            return "This surface is already attached to the surface it is being attached to.\0";
        case DDERR_SURFACEALREADYDEPENDENT:
            return "This surface is already a dependency of the surface it is being made a dependency of.\0";
        case DDERR_SURFACEBUSY:
            return "Access to this surface is being refused because the surface is already locked by another thread.\0";
        case DDERR_SURFACEISOBSCURED:
            return "Access to surface refused because the surface is obscured.\0";
        case DDERR_SURFACELOST:
            return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
        case DDERR_SURFACENOTATTACHED:
            return "The requested surface is not attached.\0";
        case DDERR_TOOBIGHEIGHT:
            return "Height requested by DirectDraw is too large.\0";
        case DDERR_TOOBIGSIZE:
            return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
        case DDERR_TOOBIGWIDTH:
            return "Width requested by DirectDraw is too large.\0";
        case DDERR_UNSUPPORTED:
            return "Action not supported.\0";
        case DDERR_UNSUPPORTEDFORMAT:
            return "FOURCC format requested is unsupported by DirectDraw.\0";
        case DDERR_UNSUPPORTEDMASK:
            return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
        case DDERR_VERTICALBLANKINPROGRESS:
            return "Vertical blank is in progress.\0";
        case DDERR_WASSTILLDRAWING:
            return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
        case DDERR_WRONGMODE:
            return "This surface can not be restored because it was created in a different mode.\0";
        case DDERR_XALIGN:
            return "Rectangle provided was not horizontally aligned on required boundary.\0";
        case D3DERR_BADMAJORVERSION:
            return "D3DERR_BADMAJORVERSION\0";
        case D3DERR_BADMINORVERSION:
            return "D3DERR_BADMINORVERSION\0";
        case D3DERR_EXECUTE_LOCKED:
            return "D3DERR_EXECUTE_LOCKED\0";
        case D3DERR_EXECUTE_NOT_LOCKED:
            return "D3DERR_EXECUTE_NOT_LOCKED\0";
        case D3DERR_EXECUTE_CREATE_FAILED:
            return "D3DERR_EXECUTE_CREATE_FAILED\0";
        case D3DERR_EXECUTE_DESTROY_FAILED:
            return "D3DERR_EXECUTE_DESTROY_FAILED\0";
        case D3DERR_EXECUTE_LOCK_FAILED:
            return "D3DERR_EXECUTE_LOCK_FAILED\0";
        case D3DERR_EXECUTE_UNLOCK_FAILED:
            return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
        case D3DERR_EXECUTE_FAILED:
            return "D3DERR_EXECUTE_FAILED\0";
        case D3DERR_EXECUTE_CLIPPED_FAILED:
            return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
        case D3DERR_TEXTURE_NO_SUPPORT:
            return "D3DERR_TEXTURE_NO_SUPPORT\0";
        case D3DERR_TEXTURE_NOT_LOCKED:
            return "D3DERR_TEXTURE_NOT_LOCKED\0";
        case D3DERR_TEXTURE_LOCKED:
            return "D3DERR_TEXTURELOCKED\0";
        case D3DERR_TEXTURE_CREATE_FAILED:
            return "D3DERR_TEXTURE_CREATE_FAILED\0";
        case D3DERR_TEXTURE_DESTROY_FAILED:
            return "D3DERR_TEXTURE_DESTROY_FAILED\0";
        case D3DERR_TEXTURE_LOCK_FAILED:
            return "D3DERR_TEXTURE_LOCK_FAILED\0";
        case D3DERR_TEXTURE_UNLOCK_FAILED:
            return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
        case D3DERR_TEXTURE_LOAD_FAILED:
            return "D3DERR_TEXTURE_LOAD_FAILED\0";
        case D3DERR_MATRIX_CREATE_FAILED:
            return "D3DERR_MATRIX_CREATE_FAILED\0";
        case D3DERR_MATRIX_DESTROY_FAILED:
            return "D3DERR_MATRIX_DESTROY_FAILED\0";
        case D3DERR_MATRIX_SETDATA_FAILED:
            return "D3DERR_MATRIX_SETDATA_FAILED\0";
        case D3DERR_SETVIEWPORTDATA_FAILED:
            return "D3DERR_SETVIEWPORTDATA_FAILED\0";
        case D3DERR_MATERIAL_CREATE_FAILED:
            return "D3DERR_MATERIAL_CREATE_FAILED\0";
        case D3DERR_MATERIAL_DESTROY_FAILED:
            return "D3DERR_MATERIAL_DESTROY_FAILED\0";
        case D3DERR_MATERIAL_SETDATA_FAILED:
            return "D3DERR_MATERIAL_SETDATA_FAILED\0";
        case D3DERR_LIGHT_SET_FAILED:
            return "D3DERR_LIGHT_SET_FAILED\0";
        default:
            return "Unrecognized error value.\0";
    }
}


#ifdef NOT
HRESULT	Render::DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int x, int y, int dx, int dy)
/*
 *  DDCopyBitmap
 *
 *  draw a bitmap into a DirectDrawSurface
 *
 */
{
    HDC                 hdcImage;
    HDC                 hdc;
    BITMAP              bm;
    DDSURFACEDESC2      ddsd;
    HRESULT             hr = DD_OK;

    if (hbm == NULL || pdds == NULL)
	return E_FAIL;

    //
    // make sure this surface is restored.
    //
    pdds->Restore();

    //
    //  select bitmap into a memoryDC so we can use it.
    //
    hdc = NULL;
    pdds->GetDC(&hdc);
    if (hdc) {
	    hdcImage = CreateCompatibleDC(hdc);
	    if (!hdcImage) {
		    RenderError e; e << "CreateCompatibleDC() failed.";
		    throw e;
//		OutputDebugString("createcompatible dc failed
");
	    }

	    SelectObject(hdcImage, hbm);

	    //
	    // get size of the bitmap
	    //
	    GetObject(hbm, sizeof(bm), &bm);    // get size of bitmap
	    dx = dx == 0 ? bm.bmWidth  : dx;    // use the passed size, unless zero
	    dy = dy == 0 ? bm.bmHeight : dy;
	    
	    //
	    // get size of surface.
	    //
	    ddsd.dwSize = sizeof(ddsd);
	    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
	    pdds->GetSurfaceDesc(&ddsd);

	    if (dx == ddsd.dwWidth && dy == ddsd.dwHeight) {
		    BitBlt(hdc, 0, 0, dx, dy, hdcImage, x, y, SRCCOPY);
	    } else {
		    StretchBlt(hdc, 0, 0, ddsd.dwWidth, ddsd.dwHeight, hdcImage, x, y, dx, dy, SRCCOPY);
	    }
	    
	    DeleteDC(hdcImage);
	    pdds->ReleaseDC(hdc);
    }

    return hr;
}


#endif // NOT


void	Render::ShowImageImmediate(Image* i)
// Shows the given image in the application window.
{
	BlitImage(0, 0, i->GetWidth(), i->GetHeight(), i, 0, 0);
	ShowFrame();
}


void	Render::BlitImage(int x, int y, int width, int height, Image* src, int u, int v)
// Blits a section of the source image onto the rendering buffer.
{
	DDImage*	t = static_cast<DDImage*>(src);
	if (t->DeviceSurface->IsLost() != DD_OK) {
		t->Restore();
	}

	// Ensure that the blit touches the screen somewhere.
	if (x >= ClientSize.cx) return;
	if (y >= ClientSize.cy) return;
	if (x + width < 0) return;
	if (y + height < 0) return;
	
	// Clip against back buffer edges...
	if (x < 0) {
		u += -x;
		width -= -x;
		x = 0;
	}
	if (y < 0) {
		v += -y;
		height -= -y;
		y = 0;
	}
	if (x + width > ClientSize.cx) {
		width = ClientSize.cx - x;
	}
	if (y + height > ClientSize.cy) {
		height = ClientSize.cy - y;
	}
	if (width <= 0 || height <= 0) return;
	
	// Set rectangles to blt from the source surface to the back buffer.
	RECT	source;
	SetRect(&source, u, v, u + width, v + height);

	LPDIRECTDRAWSURFACE4	surf = t->DeviceSurface;
	// Do the Blt.
	HRESULT	err = BackBuffer->BltFast(x, y, surf, &source,
					  (t->ColorKeyEnabled ? DDBLTFAST_SRCCOLORKEY : DDBLTFAST_NOCOLORKEY) |
					  DDBLTFAST_WAIT);

	if (err != DD_OK) {
		RenderError e; e << "BlitImage(): Can't Blt.
" << D3DErrorToString(err);
		throw e;
	}
}

