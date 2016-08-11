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
// input.hpp	-thatcher 2/6/1998 Copyright Thatcher Ulrich

// User-input interface.


#ifndef INPUT_HPP
#define INPUT_HPP


#include "types.hpp"


namespace Input {
	void	Open();
	void	Close();

	// Button IDs.
	enum ButtonID {
		BUTTON0 = 0,
		BUTTON1,
		BUTTON2,
		BUTTON3,

		UP1,
		DOWN1,
		RIGHT1,
		LEFT1,

		UP2,
		DOWN2,
		RIGHT2,
		LEFT2,

		UP3,
		DOWN3,
		RIGHT3,
		LEFT3,

		UP4,
		DOWN4,
		RIGHT4,
		LEFT4,

		ENTER,
		ESCAPE,

		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		
		BUTTONCOUNT
	};

//	enum KeyID {
//	};
	
	// For polling input state.
	struct InputState {
		float	Pitch, Yaw, Tilt, Throttle;
		float	MouseSpeedX, MouseSpeedY;
		int	MouseButtons;
		struct ButtonData {
			bool	State;
//			bool	Clicked, Released;
		} Button[BUTTONCOUNT];

		InputState()
		// Constructor.  Clear everything.
		{
			Pitch = Yaw = Tilt = Throttle = 0;
			for (int i = 0; i < BUTTONCOUNT; i++) {
				Button[i].State = /* Button[i].Clicked = Button[i].Released = */ false;
			}
		}
	};
	void	GetInputState(InputState* i, float DeltaT);

	struct EventInfo {
		ButtonID	ID;
		bool	Down;	// true --> clicked, false --> released.

		EventInfo() {}
		EventInfo(ButtonID i, bool d = true) : ID(i), Down(d) {}
		bool	operator==(const EventInfo& e) const { return (ID == e.ID) && (Down == e.Down); }
	};
	int	GetNextEvent(int SequenceCount, EventInfo* e);
	bool	CheckForEvent(EventInfo e);
	bool	CheckForEventDown(ButtonID b);	// Wrapper for CheckForEvent().
	void	EndFrameNotify();	// For organizing queues.
	void	ClearEvents();		// For special situations.

	void	NotifyMouseStatus(int x, int y, int buttons);
	void	GetMouseStatus(float* px, float* py, int* pbuttons);
	void	NotifyMouseSpeed(float x, float y);
	void	GetMouseSpeed(float* px, float* py);

	struct MouseEvent {
		float	x, y;
		int8	type;	// 1,2,3 for buttons down; -1,-2,-3 for buttons up.
	};
	void	NotifyMouseEvent(MouseEvent m);
	bool	ConsumeMouseEvent(MouseEvent* m);
	void	ClearMouseEvents();
	
	void	NotifyAlphaKeyClick(char CharCode /* , bool Repeat */);	// For alphanumeric keystrokes.
	int	GetAlphaInput(char* buf, int bufsize);

	void	NotifyKeyEvent(ButtonID id, bool Down, bool Clicked);	// For keys that act like buttons.

	void	NotifyControlKey(bool Down);
	bool	GetControlKeyState();
};


#endif // INPUT_HPP
