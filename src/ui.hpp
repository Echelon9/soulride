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
// ui.hpp	-thatcher 1/6/1999 Copyright Slingshot

// Header for UI module, which coordinates the user interface of the game.


#ifndef UI_HPP
#define UI_HPP


#include "gameloop.hpp"
#include "view.hpp"

namespace UI {

	// Functions called by GameLoop.
	void	Open();
	void	Close();
	void	MountainInit();
	void	MountainClear();
	
	void	PreUpdate(const UpdateState& u);
	void	Update(const UpdateState& u);
  void	Render(const ViewState& s, int player_index);
	bool	GetDontUpdateGame();
	void	SetDontUpdateGame(bool b);

	enum CursorType {
		ARROW = 0,
		CROSS,
		HELI_ICON,
		DROP_MARKER,

		CURSOR_TYPE_COUNT
	};
	void	DrawCursor(float x, float y, CursorType t = ARROW);
	
	enum Mode {
		NONE = 0,
		ATTRACT,
		PLAYERQUERY,
		PLAYERNAME,
		PLAYERSELECT,
		MAINMENU,
		SELECTMOUNTAIN,
		SELECTRUN,
		SHOWRUNINFO, // 8
		RUNFLYOVER,
		COUNTDOWN,
		RESUME,
		PLAYING, //12
		SHOWCRASH,
		CRASHQUERY,
		SHOWREWIND,
		SHOWFINISH,
		FINISHTOTAL,
		FINISHQUERY,
		QUITQUERY,
		OPTIONS,
		DISPLAYOPTIONS,
		SOUNDOPTIONS,
		CONTROLOPTIONS,
		WEATHEROPTIONS,
		CREDITS,
		HELP,
		HELIDROP,
		RIDELIFT,
		CAMERA,
		PLAYBACK,
		RECORDINGSELECT,
		RECORDINGNAME,
		PERFTEST,

		// multi-player stuff 
		MULTIPLAYERQUERY,
 		WAITING_FOR_OTHERS, 

		MODECOUNT
	};

	void	SetModeAll(Mode mode, int Ticks);
	void	SetMode(Mode mode, int Ticks);
	void	SetMode(Mode mode, int Ticks, int player_index);
	Mode	GetMode();
	Mode	GetMode(int player_index);

	// Looks up a string in the localization config.
	const char*	String(const char* varname, const char* DefaultValue);


	//
	// Stuff for implementing modes.
	//
	
	// UI elements.
	struct ElementData {
		ElementData(const char* t = "", bool cs = false, uint32 ARGBColor = 0xFFFFFFFF) : Text(t), CursorStop(cs), Color(ARGBColor) {}
		
		const char*	Text;
		bool	CursorStop;
		uint32	Color, DropShadow;
		// positioning, alignment, font?
	};

	void	ClearElements();
	void	ClearElements(int player_index);

	void	SetElement(int index, const ElementData& data);
	void	SetElement(int index, const ElementData& data, int player_index);
	void	SetCursor(int index);
	void	SetCursor(int index, int player_index);
	void	SetListRange(int start_index, int entries, int show_lines);

	void	SetMenuTitle(const char* title);
	void	SetPlayerName(const char* playername);
	void	SetMountainName(const char* mountainname);
	  
	float	GetRowY(int element_index);
	float	GetRowY(int element_index, int player_index);

	enum ActionCode {
		ENTER,
		LEFT,
		RIGHT,
		ESCAPE,
	};
	
	class ModeHandler {
	public:
		virtual ~ModeHandler() {}

		virtual void	Open(int Ticks) = 0;
		virtual void	Close() = 0;
		virtual void	MountainInit();	// Called when loading a new mountain.
		virtual void	MountainClear();	// Called before clearing the storage used by a mountain.
		virtual void	Update(const UpdateState& u) = 0;
		virtual void	Render(const ViewState& s);
		virtual void	Action(int ElementIndex, ActionCode code, int Ticks) = 0;
	};
	void	RegisterModeHandler(Mode mode, ModeHandler* handler);

//	const uint32	BackgroundDim = 0x80303D48; // 5068A4;
	const uint32	BackgroundDim = 0x50303030;
};


#endif // UI_HPP

