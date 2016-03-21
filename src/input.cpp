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
// input.cpp	-thatcher 2/6/1998 Copyright Thatcher Ulrich

// User-input code.


#ifdef LINUX

#include <SDL.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#else // not LINUX

#include <windows.h>
#define DIRECTINPUT_VERSION 0x0500
#include <dinput.h>
#include "winmain.hpp"

#endif // not LINUX

#include <math.h>

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif



#include "utility.hpp"
#include "timer.hpp"
#include "main.hpp"
#include "input.hpp"
#include "error.hpp"
#include "config.hpp"



namespace Input {
;
//


float	GetPitch();
float	GetYaw(float DeltaT);
float	GetTilt(float DeltaT);
float	GetThrottle();
bool	GetButtonStatus(int ButtonIndex);


#ifdef LINUX

#else // not LINUX

bool	EnableDirectInput = true;
LPDIRECTINPUT  DirectInput = NULL;

#endif // not LINUX


const int KEY_BUFFER_LEN = 20;
char	KeyInputBuffer[KEY_BUFFER_LEN];


struct KeyInfo {
	bool	Down;
	int	Clicks;
} KeyState[BUTTONCOUNT];


//
// Event buffer
//

#ifdef LINUX
pthread_mutex_t	EventBufferMutex = PTHREAD_MUTEX_INITIALIZER;
#else // not LINUX
HANDLE	EventBufferMutex = NULL;
#endif // not LINUX

const int	EVENT_BUFFER_LEN = 100;
EventInfo	EventBuffer[EVENT_BUFFER_LEN];
int	EventNext = 0;
int	EventLast = 0;
int	EventFrameMarker = 0;

static void	InsertEvent(EventInfo e);


// Joystick data.


#ifdef LINUX

SDL_Joystick*	joy = NULL;

struct {
	float	x_axis, y_axis, twist_axis, throttle_axis;
	uint32	buttons;
} joystick_status;

#else // not LINUX

//
// For DirectInput interface
//
const int	MAX_JOYSTICKS = 20;
int	JoystickCount = 0;
LPDIRECTINPUTDEVICE2	JoystickList[MAX_JOYSTICKS];
LPDIRECTINPUTDEVICE2	Joystick = NULL;
DIJOYSTATE	JoystickState;
BOOL CALLBACK	EnumJoysticks(LPCDIDEVICEINSTANCE pdinst, LPVOID DIPointer);
bool	ThrottleActive = false;

//
// For Windows Multimedia API interface
//
bool	JoystickAvailable = false;
int	JoystickPort = 0;
JOYCAPS	JoystickCaps;
JOYINFOEX	JoystickInfo;

#endif // not LINUX


bool	RudderActive = false;


void	Open()
// Initialize Input stuff.  Establish a connection to DirectInput.
{
#ifdef LINUX
#else // not LINUX
	// Create a mutex to serialize access to the event buffer.
	EventBufferMutex = CreateMutex(NULL, FALSE, "event-buffer-mutex");
#endif // not LINUX

	int	i;

#ifndef LINUX
	EnableDirectInput = Config::GetBoolValue("DirectInput");
#endif // not LINUX
	RudderActive = Config::GetBoolValue("JoystickRudder");

	// Clear the alphanumeric input buffer.
	KeyInputBuffer[0] = 0;

	// Clear the OS key-state info.
	for (i = 0; i < BUTTONCOUNT; i++) {
		KeyState[i].Down = false;
		KeyState[i].Clicks = 0;
	}

#ifdef LINUX

	joy = SDL_JoystickOpen(Config::GetInt("JoystickPort"));

	memset((void*) &joystick_status, 0, sizeof(joystick_status));

#else // not LINUX

	JoystickPort = Config::GetIntValue("JoystickPort");
	
	if (EnableDirectInput) {
		// Create the DirectInput object.
		HRESULT	err = DirectInputCreate(Main::GetAppInstance(), DIRECTINPUT_VERSION, &DirectInput, NULL);
		if (err != DI_OK) {
			Error e; e << "Error creating DirectInput object: " << err;
			throw e;
		}

		// Enumerate the joysticks.
		DirectInput->EnumDevices(DIDEVTYPE_JOYSTICK, EnumJoysticks, DirectInput, DIEDFL_ATTACHEDONLY);

		if (JoystickPort == 0) JoystickPort = 1;	// Should search for first valid port.
		
		if (JoystickCount > JoystickPort-1) {
			Joystick = JoystickList[JoystickPort-1];
			Joystick->Acquire();
		}
	} else {
		// Examine the joystick interface, using multimedia services (i.e. not via DirectInput).
		
		// First, decide which joystick port to use.
		if (JoystickPort == 0) {
			JoystickPort = 1;	// Default.
			
			JOYINFOEX joyinfo;
			int	NumDevs;
			bool	Dev1Attached, Dev2Attached;
			NumDevs = joyGetNumDevs();
			if (NumDevs > 0) {
				Dev1Attached = joyGetPosEx(JOYSTICKID1, &joyinfo) == JOYERR_NOERROR; 
				Dev2Attached = (NumDevs >= 2) && (joyGetPosEx(JOYSTICKID2, &joyinfo) == JOYERR_NOERROR);
				if (Dev1Attached || Dev2Attached) {
					JoystickPort = Dev1Attached ? 1 : 2;
				}
			}
		}
		
		MMRESULT	mmerr = joyGetDevCaps((JoystickPort - 1) + JOYSTICKID1, &JoystickCaps, sizeof(JOYCAPS));
		Config::SetFloatValue("JoyX", float(mmerr));
		if (mmerr != JOYERR_NOERROR) {
			// Joystick is unavailable, or something else is wrong.
			JoystickAvailable = false;
		} else {
			// Joystick exists, at least.  Caps data is in JoystickCaps.
			JoystickAvailable = true;
		}
	}
#endif // not LINUX	
}


#ifdef LINUX
#else // not LINUX

BOOL CALLBACK	EnumJoysticks(LPCDIDEVICEINSTANCE pdinst, LPVOID DIPointer)
// This function gets called during initialization with each joystick
// attached to the system.
{
	LPDIRECTINPUT	di = (LPDIRECTINPUT) DIPointer;
	LPDIRECTINPUTDEVICE pdev;
	DIPROPRANGE diprg;
	
	// create the DirectInput joystick device
	if (di->CreateDevice(pdinst->guidInstance, &pdev, NULL) != DI_OK) {
//		OutputDebugString("IDirectInput::CreateDevice FAILED\n");
		return DIENUM_CONTINUE;
	}
	
	// set joystick data format
	if (pdev->SetDataFormat(&c_dfDIJoystick) != DI_OK)
	{
		// Couldn't set the data format to match a joystick.
//		OutputDebugString("IDirectInputDevice::SetDataFormat FAILED\n");
		pdev->Release();
		return DIENUM_CONTINUE;
	}
	
	// set the cooperative level
	if (pdev->SetCooperativeLevel(Main::GetGameWindow(), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) != DI_OK)
	{
//		OutputDebugString("IDirectInputDevice::SetCooperativeLevel FAILED\n");
		pdev->Release();
		return DIENUM_CONTINUE;
	}

	// set X-axis range to (-1000 ... +1000)
	// This lets us test against 0 to see which way the stick is pointed.
	diprg.diph.dwSize       = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj        = DIJOFS_X;
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -1000;
	diprg.lMax              = +1000;

	if (pdev->SetProperty(DIPROP_RANGE, &diprg.diph) != DI_OK)
	{
//		OutputDebugString("IDirectInputDevice::SetProperty(DIPH_RANGE) FAILED\n");
		pdev->Release();
		return FALSE;	// ???
	}
	
	// And again for Y-axis range
	diprg.diph.dwObj        = DIJOFS_Y;
	if (pdev->SetProperty(DIPROP_RANGE, &diprg.diph) != DI_OK)
	{
//		OutputDebugString("IDirectInputDevice::SetProperty(DIPH_RANGE) FAILED\n");
		pdev->Release();
		return FALSE;
	}

	// Try to set up a rudder axis as well.
	diprg.diph.dwObj = DIJOFS_RZ;
	if (Config::GetBoolValue("JoystickRudder") == true && pdev->SetProperty(DIPROP_RANGE, &diprg.diph) != DI_OK) {
		// We couldn't set up the rudder axis.  No edging from the joystick.
		RudderActive = false;
	}

	// Try to set up a throttle axis.
	diprg.diph.dwObj = DIJOFS_RX;
	if (pdev->SetProperty(DIPROP_RANGE, &diprg.diph) != DI_OK) {
		// We couldn't set up the throttle axis.
	} else {
		ThrottleActive = true;
	}

	// set X axis dead zone to 10%
	// Units are ten thousandths, so 10% = 1000/10000.
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize       = sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwObj        = DIJOFS_X;
	dipdw.diph.dwHow        = DIPH_BYOFFSET;
	dipdw.dwData            = 1000;

	if (pdev->SetProperty(DIPROP_DEADZONE, &dipdw.diph) != DI_OK) {
//		OutputDebugString("IDirectInputDevice::SetProperty(DIPH_DEADZONE) FAILED\n");
		pdev->Release();
		return FALSE;
	}
	
	// set Y axis dead zone to 10%
	// Units are ten thousandths, so 10% = 1000/10000.
	dipdw.diph.dwObj = DIJOFS_Y;
	dipdw.dwData = 1000;
	if (pdev->SetProperty(DIPROP_DEADZONE, &dipdw.diph) != DI_OK) {
//		OutputDebugString("IDirectInputDevice::SetProperty(DIPH_DEADZONE) FAILED\n");
		pdev->Release();
		return FALSE;
	}

	if (RudderActive) {
		// Set rudder dead zone to 10%.
		dipdw.diph.dwObj = DIJOFS_RZ;
		dipdw.dwData = 1000;
		if (pdev->SetProperty(DIPROP_DEADZONE, &dipdw.diph) != DI_OK) {
//			OutputDebugString("IDirectInputDevice::SetProperty(DIPH_DEADZONE) FAILED\n");
			pdev->Release();
			return FALSE;
		}
	}

	if (ThrottleActive) {
		// Set throttle dead zone to 10%.
		dipdw.diph.dwObj = DIJOFS_RX;
		dipdw.dwData = 1000;
		if (pdev->SetProperty(DIPROP_DEADZONE, &dipdw.diph) != DI_OK) {
//			OutputDebugString("IDirectInputDevice::SetProperty(DIPH_DEADZONE) FAILED\n");
			pdev->Release();
			return FALSE;
		}
	}
	
	// Get a DEVICE2 interface.
	LPDIRECTINPUTDEVICE2	dev2;
	HRESULT err = pdev->QueryInterface(IID_IDirectInputDevice2, (LPVOID*) &dev2);
	if (err != DI_OK) {
		pdev->Release();
		return FALSE;
	}
	
	// Add it to our list of devices.
//	AddInputDevice(pdev, pdinst);
	if (JoystickCount < MAX_JOYSTICKS) {
		JoystickList[JoystickCount] = dev2;
		JoystickCount++;
	} else {
		pdev->Release();
	}
	
	return DIENUM_CONTINUE;
}

#endif // not LINUX


void	Close()
// Close the Input interface.
{
#ifdef LINUX

	if (joy) {
		SDL_JoystickClose(joy);
		joy = NULL;
	}

#else // not LINUX
	if (EnableDirectInput) {
		// Release the joysticks.
		while (JoystickCount > 0) {
			JoystickCount--;
			JoystickList[JoystickCount]->Release();
		}
	}
	
	if (DirectInput) {
		DirectInput->Release();
		DirectInput = NULL;
	}

	CloseHandle(EventBufferMutex);
#endif // not LINUX
}


bool	FirstTime = true;
InputState	LastState;


void	Update();


void	GetInputState(InputState* result, float DeltaT)
// Fills the given structure with the current state of the inputs.
{
	Update();

	// Report mouse speed, for faking joystick.
	float	MouseSpeedX, MouseSpeedY;
	int	MouseButtons;
	GetMouseStatus(&MouseSpeedX, &MouseSpeedY, &MouseButtons);
	GetMouseSpeed(&MouseSpeedX, &MouseSpeedY);
	result->MouseButtons = MouseButtons;
	result->MouseSpeedX = MouseSpeedX;
	result->MouseSpeedY = MouseSpeedY;

	result->Pitch = GetPitch();
	result->Yaw = GetYaw(DeltaT);
	result->Tilt = GetTilt(DeltaT);
	result->Throttle = GetThrottle();

	// Update the button data.
	int	i;
	for (i = 0; i < BUTTONCOUNT; i++) {
		InputState::ButtonData&	b = result->Button[i];
		
		b.State = GetButtonStatus(i);
		if (b.State == true && FirstTime == false && LastState.Button[i].State == false) {
			// This button just went down.  Insert an event into the event queue.
			InsertEvent(EventInfo((ButtonID) i, true));
		}

		// Check keyboard aliases for buttons.
		if (KeyState[i].Down) b.State = true;
	}

	LastState = *result;
	FirstTime = false;
}


void	Update()
// Checks input devices.
{
#ifdef LINUX

	if (joy) {
		SDL_JoystickUpdate();
		joystick_status.x_axis = SDL_JoystickGetAxis(joy, 0) / 32767.0;
		joystick_status.y_axis = SDL_JoystickGetAxis(joy, 1) / 32767.0;
		joystick_status.twist_axis = SDL_JoystickGetAxis(joy, 2) / 32767.0;
		joystick_status.throttle_axis = SDL_JoystickGetAxis(joy, 3) / 32767.0;
		joystick_status.buttons = 0;
		joystick_status.buttons |= (SDL_JoystickGetButton(joy, 0) ? 1 : 0);
		joystick_status.buttons |= (SDL_JoystickGetButton(joy, 1) ? 2 : 0);
		joystick_status.buttons |= (SDL_JoystickGetButton(joy, 2) ? 4 : 0);
		joystick_status.buttons |= (SDL_JoystickGetButton(joy, 3) ? 8 : 0);
	}

#else // not LINUX

	HRESULT err;

	if (EnableDirectInput == false) {
		// Poll the joystick.
		if (JoystickAvailable) {
			JoystickInfo.dwSize = sizeof(JOYINFOEX);
			JoystickInfo.dwFlags = JOY_RETURNBUTTONS
					       | JOY_RETURNCENTERED
					       | JOY_RETURNR
					       | JOY_RETURNX
					       | JOY_RETURNY
					       | JOY_RETURNZ
					       /* | JOY_USEDEADZONE */
					       ;
			MMRESULT	mmerr = joyGetPosEx((JoystickPort - 1) + JOYSTICKID1, &JoystickInfo);
			// xxx check error code, & do something if not OK.
		}
	} else {
		if (Joystick) {
			// poll the joystick to read the current state
			err = Joystick->Poll();
			if (err != DI_OK) {
				// Did we lose input for some reason? 
				// if so, then attempt to reacquire.
//				if (err == DIERR_INPUTLOST) {
					err = Joystick->Acquire();
//				}
			}
		
			// get data from the joystick 
			err = Joystick->GetDeviceState(sizeof(DIJOYSTATE), &JoystickState);
			if (err != DI_OK) {
				// did the read fail because we lost input for some reason? 
				// if so, then attempt to reacquire.
//				if (err == DIERR_INPUTLOST) {
					err = Joystick->Acquire();
//				}
			}
		}
	}

#endif // not LINUX
}


float	DeadZone(float in, float zone)
// Maps the input, which should be in the range -1 to 1, to
// the return value, also in the range -1 to 1, but with a dead zone
// in the middle from -zone to zone which maps to 0.
{
	if (in < -zone) {
		// Input is below the dead zone.
		return (in + zone) / (1 - zone);
	} else if (in < zone) {
		// Input is in the dead zone.
		return 0;
	} else {
		// Input is above the dead zone.
		return (in - zone) / (1 - zone);
	}
}


float	GetYaw(float DeltaT)
// Returns a value for the user's steering.  Ranges from -1.0 (turning
// full left) to 1.0 (turning full right).
{
	float	yaw = 0;

	if (RudderActive) {
#ifdef LINUX
		yaw += DeadZone(joystick_status.twist_axis, 0.1);
#else // not LINUX
		if (EnableDirectInput) {
			if (Joystick) {
				yaw += float(JoystickState.lRz) / 1000.0f;
			}
		} else {
			// Add joystick.
			if (JoystickAvailable && (JoystickInfo.dwFlags & JOY_RETURNR)) {
				float f = 2 * float(JoystickInfo.dwRpos - JoystickCaps.wRmin) / (JoystickCaps.wRmax - JoystickCaps.wRmin) - 1;
				yaw += DeadZone(f, 0.1f);
			}
		}
#endif // not LINUX
	}

	if (yaw > 1.0) yaw = 1.0;
	if (yaw < -1.0) yaw = -1.0;
	
	return yaw;
}


float	GetTilt(float DeltaT)
// Returns a value for the tilt of the board (for edging).  Ranges from
// -1.0 (left, i.e. heel-side for left-foot-forward) to 1.0 (right, i.e.
// toe-side).
{
	float	tilt = 0;

	// Add keyboard.
	{
		static float	KeyTilt = 0;
		static const float	DRIFT = 3.0;
		KeyTilt += fclamp(-DRIFT * DeltaT, -KeyTilt, DRIFT * DeltaT);

		if (KeyState[LEFT1].Down) KeyTilt -= 2 * DRIFT * DeltaT;
		if (KeyState[RIGHT1].Down) KeyTilt += 2 * DRIFT * DeltaT;
		KeyTilt = fclamp(-1, KeyTilt, 1);

		if (KeyTilt > 0) {
			tilt += KeyTilt;
		} else if (KeyTilt < 0) {
			tilt -= -KeyTilt;
		}
	}

#ifdef LINUX
	tilt += DeadZone(joystick_status.x_axis, 0.1);
#else // not LINUX
	if (EnableDirectInput) {
		if (Joystick) {
			tilt += float(JoystickState.lX) / 1000.0f;
		}
	} else {
		// Add joystick.
		if (JoystickAvailable && (JoystickInfo.dwFlags & JOY_RETURNX)) {
			float f = 2 * float(JoystickInfo.dwXpos - JoystickCaps.wXmin) / (JoystickCaps.wXmax - JoystickCaps.wXmin) - 1;
			tilt += DeadZone(f, 0.1f);
		}
	}
#endif // not LINUX

	// Use mouse button inputs?
	
	return fclamp(-1.0, tilt, 1.0);
}


float	GetPitch()
// Returns a value for the user's forward/back command.  Ranges from 1.0 (full forward)
// to -1.0 (full back).
{
	float	pitch = 0;

	// Check the keyboard.
	if (KeyState[UP1].Down) pitch += 1.0;
	if (KeyState[DOWN1].Down) pitch -= 1.0;

#ifdef LINUX
	pitch += DeadZone(-joystick_status.y_axis, 0.1);
#else // not LINUX
	if (EnableDirectInput) {
		if (Joystick) {
			pitch -= float(JoystickState.lY) / 1000.0f;
		}
	} else {
		// Check the joystick.
		if (JoystickAvailable /* && (JoystickCaps.wCaps & JOYCAPS_HASR) */) {
			float f = 2 * float(JoystickInfo.dwYpos - JoystickCaps.wYmin) / (JoystickCaps.wYmax - JoystickCaps.wYmin) - 1;
			pitch -= DeadZone(f, 0.1f);
		}
	}
#endif // not LINUX

	// Clamp the output.
	return fclamp(-1.0, pitch, 1.0);
}


float	GetThrottle()
// Returns a value for the user's throttle/speed setting.  Ranges from 1.0 (full forward)
// to -1.0 (full back).
{
	float	throttle = 0;

#ifdef LINUX
	throttle += DeadZone(joystick_status.throttle_axis, 0.1);
#else // not LINUX
	if (EnableDirectInput) {
		if (Joystick && ThrottleActive) {
			throttle -= float(JoystickState.lX) / 1000.0f;
		}
	} else {
		// Check the joystick.
		if (JoystickAvailable /* && (JoystickCaps.wCaps & JOYCAPS_HASR) */) {
			float f = 2 * float(JoystickInfo.dwZpos - JoystickCaps.wZmin) / (JoystickCaps.wZmax - JoystickCaps.wZmin) - 1;
			throttle -= DeadZone(f, 0.1f);
		}
	}
#endif // not LINUX

	// Clamp the output.
	return fclamp(-1.0, throttle, 1.0);
}


bool	GetButtonStatus(int ButtonIndex)
// Returns the status of the specified button.  TRUE if the button is down;
// false otherwise.
{
	switch (ButtonIndex) {
	default:
		break;
	case BUTTON0:
#ifdef LINUX
		return (joystick_status.buttons & 1) != 0;
#else // not LINUX
		if (EnableDirectInput) {
			if (JoystickState.rgbButtons[0] & 0x80) return true;
		} else {
			if (JoystickCaps.wMaxButtons > 0 && JoystickInfo.dwButtons & JOY_BUTTON1) return true;
		}
#endif // not LINUX
		break;
	case BUTTON1:
#ifdef LINUX
		return (joystick_status.buttons & 2) != 0;
#else // not LINUX
		if (EnableDirectInput) {
			if (JoystickState.rgbButtons[1] & 0x80) return true;
		} else {
			if (JoystickCaps.wMaxButtons > 1 && JoystickInfo.dwButtons & JOY_BUTTON2) return true;
		}
#endif // not LINUX
		break;
	case BUTTON2:
#ifdef LINUX
		return (joystick_status.buttons & 4) != 0;
#else // not LINUX
		if (EnableDirectInput) {
			if (JoystickState.rgbButtons[2] & 0x80) return true;
		} else {
			if (JoystickCaps.wMaxButtons > 2 && JoystickInfo.dwButtons & JOY_BUTTON3) return true;
		}
#endif // not LINUX
		break;
	case BUTTON3:
#ifdef LINUX
		return (joystick_status.buttons & 8) != 0;
#else // not LINUX
		if (EnableDirectInput) {
			if (JoystickState.rgbButtons[3] & 0x80) return true;
		} else {
			if (JoystickCaps.wMaxButtons > 3 && JoystickInfo.dwButtons & JOY_BUTTON4) return true;
		}
#endif // not LINUX
		break;

	case LEFT1:
		break;
	case RIGHT1:
		break;
	case UP1:
		break;
	case DOWN1:
		break;
	case ENTER:
		break;
	case ESCAPE:
		break;

	case F1:
		break;
	case F2:
		break;
	case F3:
		break;
	case F4:
		break;
	case F5:
		break;
	case F6:
		break;
	case F7:
		break;
	case F8:
		break;
	case F9:
		break;
	case F10:
		break;
	case F11:
		break;
	case F12:
		break;
	}

	return false;
}


//
// Mouse.
//


int	WinMouseX, WinMouseY, WinMouseButtons;
float	WinMouseSpeedX = 0, WinMouseSpeedY = 0;


void	NotifyMouseStatus(int x, int y, int buttons)
// Called by the window event-processing function when it gets mouse inputs.
// We store it away, to be returned on request by GetMouseStatus().
{
	WinMouseX = x;
	WinMouseY = y;
	WinMouseButtons = buttons;
}


void	GetMouseStatus(float* x, float* y, int* buttons)
// Returns the current status of the mouse.  Coordinates are in
// window pixel coords, (0,0) at upper left.
{
	*x = float(WinMouseX);
	*y = float(WinMouseY);
	*buttons = WinMouseButtons;
}


void	NotifyMouseSpeed(float x, float y)
// Called by the OS wrapper to tell us how fast the mouse is moving (in pixels/sec).
{
	WinMouseSpeedX = x;
	WinMouseSpeedY = y;
}


void	GetMouseSpeed(float* x, float* y)
// Returns the current speed of the mouse (in pixels/sec).
{
	*x = WinMouseSpeedX;
	*y = WinMouseSpeedY;
}


// Event queue for mouse clicks.
const int	MQ_SIZE = 8;
int	MQFront = 0, MQRear = 0;
MouseEvent	MQ[MQ_SIZE];


static int	MQEvents()
// returns the number of events stored in the mouse event queue.
{
	int	diff = MQFront - MQRear;
	if (diff < 0) diff += MQ_SIZE;
	return diff;
}


void	NotifyMouseEvent(MouseEvent m)
// Adds the given event to the end of the mouse-event queue.  Events can be
// retrieved later using ConsumeMouseEvent().
{
	// Make sure there's room in the queue.  If not, then make room by dropping the oldest event.
	if (MQEvents() >= MQ_SIZE-1) MQRear = (MQRear + 1) % MQ_SIZE;

	// Add the event.
	MQ[MQFront] = m;

	// Advance the front index.
	MQFront = (MQFront + 1) % MQ_SIZE;

	// Alias the left & right mouse buttons to the BUTTON0 and BUTTON1 buttons.
	switch ( m.type ) {
	case 1:
		NotifyKeyEvent( BUTTON0, true, true );
		break;
	case -1:
		NotifyKeyEvent( BUTTON0, false, false );
		break;
	case 2:
		NotifyKeyEvent( BUTTON1, true, true );
		break;
	case -2:
		NotifyKeyEvent( BUTTON1, false, false );
		break;
	case 3:
		NotifyKeyEvent( BUTTON2, true, true );
		break;
	case -3:
		NotifyKeyEvent( BUTTON2, false, false );
		break;
	}
}


bool	ConsumeMouseEvent(MouseEvent* m)
// If there are any mouse events in the queue, returns true and puts the oldest event in
// *m.  Removes the event from the queue.
{
	if (MQEvents() == 0) return false;

	*m = MQ[MQRear];

	MQRear = (MQRear + 1) % MQ_SIZE;

	return true;
}


void	ClearMouseEvents()
// Clear the mouse event queue.  Might want to do this in modes that
// do mouse processing, to clear out stale clicks.
{
	MQRear = MQFront;
}


//
// Keyboard notification.
//


void	NotifyAlphaKeyClick(char c)
// OS event-processing code calls this function when an ordinary keystroke
// is detected.  Store the event for later retrieval.
{
	int	len = strlen(KeyInputBuffer);

	if (len >= KEY_BUFFER_LEN-1) {
		// Buffer is full.  Shift the buffer down one, losing the oldest character.
		len = KEY_BUFFER_LEN - 2;
		for (int i = 0; i < len - 1; i++) {
			KeyInputBuffer[i] = KeyInputBuffer[i+1];
		}
	}

	KeyInputBuffer[len+1] = 0;
	KeyInputBuffer[len] = c;
}


int	GetAlphaInput(char* buf, int bufsize)
// Returns the contents of the keyboard-input buffer for the current
// frame.  The character codes go in buf[], and are null-terminated.  No
// more than bufsize elements of buf[] are touched.  The return value is
// the length of the string returned.
{
	int	i = 0;
	while (i < bufsize - 1) {
		char	c = KeyInputBuffer[i];
		if (c == 0) break;
		buf[i] = c;
		i++;
	}

	buf[i] = 0;

	return i;
}


void	NotifyKeyEvent(ButtonID id, bool Down, bool Clicked)
// Should be called by the OS interface layer when one of the keyboard
// alias keys changes state or generates a click.
// Records the event for later retrieval via GetInputState().
{
	if (id < 0 || id >= BUTTONCOUNT) return;

	KeyState[id].Down = Down;
	if (Clicked) KeyState[id].Clicks++;	//xxxx

	// Insert into the event queue.
	InsertEvent(EventInfo(id, Down));
}


void	InsertEvent(EventInfo e)
// Inserts the specified event into the event queue.
{
#ifdef LINUX
	if (pthread_mutex_trylock(&EventBufferMutex) == EBUSY) return;
#else // not LINUX
	if (WaitForSingleObject(EventBufferMutex, 0) == WAIT_FAILED) return;
#endif // not LINUX
	
	int	NextNext = (EventNext + 1) % EVENT_BUFFER_LEN;

	if (NextNext != EventLast) {
		EventBuffer[EventNext] = e;
		EventNext = NextNext;
	} else {
		// Buffer is full.  Just drop the event.
	}

	// Unlock.
#ifdef LINUX
	pthread_mutex_unlock(&EventBufferMutex);
#else // not LINUX
	ReleaseMutex(EventBufferMutex);
#endif // not LINUX
}


int	GetNextEvent(int SequenceCount, EventInfo* e)
// Puts the frame's SequenceCount'th event in the given structure.  If
// there are no frame events left, leaves the structure alone and
// returns 0, otherwise returns SequenceCount++.  Use this function to
// sequence through all of a frame's events in order, using a loop
// such as:
//
// seq = 0;
// while (seq = GetNextEvent(seq, &e)) ProcessEvent(e);
//
{
	// Compute the number of events in the current frame.
	int	events = (EventFrameMarker + EVENT_BUFFER_LEN - EventLast) % EVENT_BUFFER_LEN;
	if (SequenceCount >= events) return 0;

	int	index = (EventLast + SequenceCount) % EVENT_BUFFER_LEN;
	*e = EventBuffer[index];

	return SequenceCount + 1;
}


bool	CheckForEvent(EventInfo e)
// See if there's an event matching e in the frame's event buffer.
{
	int	index = EventLast;
	while (index != EventFrameMarker) {
		if (EventBuffer[index] == e) {
			// Found a matching event.
			return true;
		}

		index++;
		if (index >= EVENT_BUFFER_LEN) index = 0;
	}

	return false;
}


bool	CheckForEventDown(ButtonID b)
// Returns true if the specified button was pressed in the current frame.
{
	return CheckForEvent(EventInfo(b, true));
}


void	EndFrameNotify()
// Clear the current frame's event queue and buffers and set up for the next frame.
{
	// Clear the alpha input buffer.
	KeyInputBuffer[0] = 0;

	//
	// Deal with key and button events (which may be modified asynchronously from the
	// input thread).
	//
	
	// Lock.
#ifdef LINUX
	if (pthread_mutex_trylock(&EventBufferMutex) == EBUSY) return;
#else // not LINUX
	if (WaitForSingleObject(EventBufferMutex, 0) == WAIT_FAILED) return;
#endif // not LINUX
	
	EventLast = EventFrameMarker;
	EventFrameMarker = EventNext;

	// Unlock the buffer.
#ifdef LINUX
	pthread_mutex_unlock(&EventBufferMutex);
#else // not LINUX
	ReleaseMutex(EventBufferMutex);
#endif // not LINUX
}


void	ClearEvents()
// Clear the event queue completely.
{
	// Clear the alpha input buffer.
	KeyInputBuffer[0] = 0;

	//
	// Deal with key and button events (which may be modified asynchronously from the
	// input thread).
	//
	
	// Lock.
#ifdef LINUX
	if (pthread_mutex_trylock(&EventBufferMutex) == EBUSY) return;
#else // not LINUX
	if (WaitForSingleObject(EventBufferMutex, 0) == WAIT_FAILED) return;
#endif // not LINUX
	
	EventLast = EventNext;
	EventFrameMarker = EventNext;

	// Unlock the buffer.
#ifdef LINUX
	pthread_mutex_unlock(&EventBufferMutex);
#else // not LINUX
	ReleaseMutex(EventBufferMutex);
#endif // not LINUX
}


static bool	ControlKeyDown = false;


void	NotifyControlKey(bool Down)
// Called by OS wrapper when control key changes state.  Down == true for downstroke,
// false for upstroke.
{
	ControlKeyDown = Down;
}


bool	GetControlKeyState()
// Returns the state of the control key.  true == down.
{
	return ControlKeyDown;
}


};	// End of namespace Input
