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
// UIPlayback.cpp	-thatcher 1/3/2000 Copyright Slingshot Game Technology

// UI mode for playing back recorded runs.


#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#ifdef LINUX
#else // not LINUX
#include <io.h>
#endif // not LINUX
#include <stdio.h>
#include <list>

#include "ui.hpp"
#include "recording.hpp"
#include "gameloop.hpp"
#include "ui.hpp"
#include "psdread.hpp"
#include "render.hpp"
#include "sound.hpp"
#include "game.hpp"
#include "usercamera.hpp"
#include "terrain.hpp"
#include "utility.hpp"
#include "config.hpp"
#include "imageoverlay.hpp"
#include "player.hpp"
#include "text.hpp"
#include "attractmode.hpp"
#include "weather.hpp"
#include "../data/gui/guidefs.h"


//
// UI mode handler
//


const int	BUTTON_COUNT = 8;
struct ButtonInfo {
	const char*	Name;
	// key id
} ButtonData[BUTTON_COUNT] = {
	{ "vcr_load.ggm" },
	{ "vcr_save.ggm" },
	{ "vcr_restart.ggm" },
	{ "vcr_rew.ggm" },
	{ "vcr_pause.ggm" },
	{ "vcr_slow.ggm" },
	{ "vcr_play.ggm" },
	{ "vcr_ffwd.ggm" },
};


class Playback : public UI::ModeHandler {
	int	CameraChangeTimer;
	enum CameraMode {
		CLOSE_CHASE,
		FAR_CHASE,
		FIXED,

		MODE_COUNT
	} Mode;
	float	PlayRate;
	
	float	Angle, AngleSpeed;
	float	Distance, DistanceSpeed;
	float	Height, HeightSpeed;

	GG_Player*	VCRBackground;
	GG_Player*	VCRButton[BUTTON_COUNT];
	bool	ButtonActive[BUTTON_COUNT];

	bool	FirstUpdate;
	
public:
	Playback() {
		CameraChangeTimer = 0;
		Mode = CLOSE_CHASE;
		PlayRate = 1.0;
		
		Angle = AngleSpeed = 0;
		Distance = 4;
		DistanceSpeed = 0;
		Height = 2;
		HeightSpeed = 0;

		VCRBackground = NULL;
		for (int i = 0; i < BUTTON_COUNT; i++) {
			VCRButton[i] = NULL;
			ButtonActive[i] = false;
		}

	}

	virtual ~Playback() {
		if (VCRBackground) {
			VCRBackground->unRef();
		}
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		if (VCRBackground == NULL) {
			VCRBackground = GameLoop::LoadMovie("vcr.ggm");
//			VCRBackground->setPlayMode(GG_PLAYMODE_LOOP);
			for (int i = 0; i < BUTTON_COUNT; i++) {
				VCRButton[i] = GameLoop::LoadMovie(ButtonData[i].Name);
			}
		}
		
		AttractMode::Exit();
		
		UserCamera::SetSubject(Game::GetUser());
		UserCamera::SetMotionMode(UserCamera::CHASE);
		UserCamera::SetAimMode(UserCamera::LOOK_AT);

		Mode = CLOSE_CHASE;
		CameraChangeTimer = 0;

//		if (Recording::ChannelHasData(Recording::PLAYER_CHANNEL)) {
//			Recording::SetChannel(Recording::PLAYER_CHANNEL);
//		}
		
		Recording::SetMode(Recording::STOP, Ticks);
		Recording::SetMode(Recording::PLAY, Ticks);
		Recording::MoveCursor(-1000000);

		PlayRate = 1.0;
		GameLoop::SetSpeedScale(PlayRate);

		FirstUpdate = false;

//		char	buf[300];
		VCRBackground->getMovie()->setActorText(GUI_VCR_DISPLAY, (char*) Recording::GetPlayerName());

		Game::SetActiveRun(Recording::GetRunIndex());
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		Recording::SetMode(Recording::STOP, GameLoop::GetCurrentTicks());

//		GameLoop::SetSpeedScale(1.0);	// Restore normal speed of dynamic effects.

//		UserCamera::SetAutoCameraMode(UserCamera::AUTO_OFF);

		// xxxx try deleting VCR GameGUI assets...
		if (VCRBackground) {
			VCRBackground->unRef();
			VCRBackground = NULL;
			int	i;
			for (i = 0; i < BUTTON_COUNT; i++) {
				VCRButton[i]->unRef();
				VCRButton[i] = NULL;
			}
		}
	}

	void	Update(const UpdateState& u)
	// Deal with the camera and UI.
	{
		// Clear button-display flags.
		int	i;
		for (i = 0; i < BUTTON_COUNT; i++) {
			ButtonActive[i] = false;
		}
		
		//
		// Keystroke/mouse UI for VCR controls
		//

		// Collect key input.
		char	buf[80];
		Input::GetAlphaInput(buf, 40);

		// Collect mouse input.
		Input::MouseEvent	m;
		int	end_index = strlen(buf);
		while (Input::ConsumeMouseEvent(&m)) {
			if (m.type > 0) {	// button down.

				int	button = 0;
				if (VCRBackground->findHiti((int) m.x, (int) m.y, &button)) {
					char	cmd = 0;
					switch (button) {
					default:
					case 0:	cmd = 0; break;
					case GUI_VCR_LOAD:	cmd = (char) 145; break;
					case GUI_VCR_SAVE:	cmd = (char) 146; break;
					case GUI_VCR_RESTART:	cmd = (char) 147; break;
					case GUI_VCR_PAUSE:	cmd = (char) 149; break;
					case GUI_VCR_SLOW:	cmd = (char) 150; break;
					case GUI_VCR_PLAY:	cmd = (char) 151; break;
					}
					if (cmd != 0 && end_index < 40) {
						buf[end_index] = cmd;
						buf[end_index+1] = 0;
						end_index++;
//						char	str[2];
//						str[0] = cmd;
//						str[1] = 0;
//						strcat(buf, str);

						// Clear event queue.  We do this to be sure
						// not to process this mouse click as a
						// keypress event, since we're already
						// processing it as a mouse click.
						Input::ClearEvents();
						
						break;
					}
				}
			}
		}

		unsigned char*	p = (unsigned char*) buf;
		while (*p) {
			if (*p == 146) {
				// Save the recording to a file.
				UI::SetMode(UI::RECORDINGNAME, u.Ticks);
				ButtonActive[1] = true;
				return;
			}
			if (*p == 145) {
				// Load the recording.
				UI::SetMode(UI::RECORDINGSELECT, u.Ticks);
				ButtonActive[0] = true;
				return;
			}
			
			if (*p == 147) {
				// Rewind to beginning.
				Recording::MoveCursor(-1000000);
				ButtonActive[2] = true;
			}
//			if (*p == '}') {
//				// Forward to end.
//				Recording::MoveCursor(1000000);
//			}
			
			if (*p == 149 || *p == 151) {
				// Pause/unpause.
				if (Recording::GetMode() == Recording::PLAY) {
//					PlayRate = GameLoop::GetSpeedScale();
//					GameLoop::SetSpeedScale(0);
					Recording::SetMode(Recording::PAUSE, u.Ticks);
				} else {
//					GameLoop::SetSpeedScale(PlayRate);
					Recording::SetMode(Recording::PLAY, u.Ticks);
				}
			}
			if (*p == 150) {
				// Toggle slow.
				if (PlayRate > 0.9) {
					PlayRate = 0.25;
				} else {
					PlayRate = 1.0;
				}
//				if (Recording::GetMode() != Recording::PLAY) {
//					Recording::SetMode(Recording::PLAY, u.Ticks);
//				}
			}
//			if (*p == '\\') {
//				// Forward.
//				PlayRate = 1.0;
//				if (Recording::GetMode() != Recording::PLAY) {
//					Recording::SetMode(Recording::PLAY, u.Ticks);
//				}
//			}

			p++;
		}

		int	mb;
		float	mx, my;
		Input::GetMouseStatus(&mx, &my, &mb);
		
		// Move the recording: fast-forward, rewind, or ordinary playback.
		if (u.Inputs.Button[Input::LEFT1].State ||
		    u.Inputs.Button[Input::F8].State ||
		    (mb && VCRBackground->detectHiti((int) mx, (int) my, GUI_VCR_REW)))
		{
			// Rewind.
			GameLoop::SetSpeedScale(-3);
			Recording::MoveCursor(u.DeltaTicks * -3);
			ButtonActive[3] = true;
			
		} else if (u.Inputs.Button[Input::RIGHT1].State ||
			   u.Inputs.Button[Input::F12].State ||
			   (mb && VCRBackground->detectHiti((int) mx, (int) my, GUI_VCR_FFWD)))
		{
			// Fast-forward.
			GameLoop::SetSpeedScale(3);
			Recording::MoveCursor(u.DeltaTicks * 3);
			ButtonActive[7] = true;
			
		} else if (Recording::GetMode() == Recording::PLAY) {
			// Advance the recording.
			GameLoop::SetSpeedScale(PlayRate);
			Recording::MoveCursor((int) (u.DeltaTicks * GameLoop::GetSpeedScale()));
		} else {
			// Paused.
			GameLoop::SetSpeedScale(0);
		}

		if (FirstUpdate) {
			Recording::SetMode(Recording::PLAY, u.Ticks);
			Recording::MoveCursor(1);//xxxxxxx
			GameLoop::SetSpeedScale(PlayRate);
			FirstUpdate = false;
		}
		
		//
		// Camera.
		//
		CameraChangeTimer -= u.DeltaTicks;
		if ((CameraChangeTimer <= 0 && u.Inputs.Button[Input::BUTTON1].State == false)
		    || Input::CheckForEventDown(Input::BUTTON0)
			)
		{
			CameraChangeTimer = Utility::RandomInt(2000) + 3000;

			UserCamera::RandomAutoCameraMode();
		}
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Decide which VCR buttons to highlight, based on playback state.
		Recording::Mode	m = Recording::GetMode();
		if (m == Recording::PLAY) {
			ButtonActive[4] = false;
			ButtonActive[6] = true;
		} else if (m == Recording::PAUSE || m == Recording::STOP) {
			ButtonActive[4] = true;
			ButtonActive[6] = false;
		}

		if (PlayRate < 0.9) {
			ButtonActive[5] = true;
		} else {
			ButtonActive[5] = false;
		}
		
		// Show GUI elements.
		GameLoop::GUIBegin();

		VCRBackground->play(0);
		for (int i = 0; i < BUTTON_COUNT; i++) {
			if (ButtonActive[i]) VCRButton[i]->play(0);
		}
		
		GameLoop::GUIEnd();

		// Show the mouse cursor.
		int	mb;
		float	mx, my;
		Input::GetMouseStatus(&mx, &my, &mb);
		mx = mx / Render::GetWindowWidth() * 640.0f;
		my = my / Render::GetWindowHeight() * 480.0f;
		UI::DrawCursor(mx, my, UI::ARROW);

		// Show recording time, score & rewind count.
		Game::ShowRewinds();
		Game::ShowScore();
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			UI::SetMode(UI::MAINMENU, Ticks);
		}
	}
	
} PlaybackInstance;



// RECORDINGSELECT

class RecordingSelect : public UI::ModeHandler {
	int	CancelIndex;
#if 0
	struct RecordingElem {
//		RecordingElem*	Next;
		char*	Name;

		RecordingElem() {
			Name = 0;
//			Next = 0;
		}
		RecordingElem(const RecordingElem& r) {
			Name = r.Name;
		}
		RecordingElem&	operator=(const RecordingElem& r) {
			Name = r.Name;
			return *this;
		}
		~RecordingElem()
		{
			delete [] Name;
		}

		bool	operator<(const RecordingElem& p) {
			char*	a = Name;
			char*	b = p.Name;
			while (*a) {
				if (*b == 0) return false;
				int	ca = tolower(*a);
				int	cb = tolower(*b);
				if (ca > cb) return false;
				else if (ca < cb) return true;
				a++;
				b++;
			}
			if (*b) return true;
			else return false;
		}
	};
//	RecordingElem*	List;
	std::list<RecordingElem>	List;
#endif // 0
	StringElem*	List;
	
public:
	RecordingSelect() {
		CancelIndex = 0;
		List = 0;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		Recording::EnumerateRecordings(Game::GetCurrentMountain(), AddRecordingName);
//		List.sort();
		List = Utility::SortStringList(List);
		
		ShowOptions();
	}

	static void	AddRecordingName(const char* recname);
	
	void	ShowOptions()
	// Draw the menu with the current options displayed.
	{
//		Player*	p = Game::GetCurrentPlayer();
		
		UI::ClearElements();
		
		UI::SetElement(0, UI::ElementData(UI::String("choose_a_recording", "     Choose a recording     "), false));

		int	CurrentIndex = 2;
		UI::SetListRange(CurrentIndex, List->CountElements(), 8);
		StringElem*	m = List;
		int	i;
		for (m = List, i = 0; m; i++, m = m->Next) {
			int	Color = 0xFFFFFFFF;
			UI::SetElement(i + 2, UI::ElementData(m->Name, true, Color));
		}

		i++;
		CancelIndex = i + 2;
		UI::SetElement(CancelIndex, UI::ElementData(UI::String("cancel", "    Cancel    "), true));
		
		UI::SetCursor(CurrentIndex);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		if (List) {
			delete List;
			List = NULL;
		}
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render frame.
	{
		// Dim the background.
		Render::FullscreenOverlay(UI::BackgroundDim);
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// User has taken some action.
	{
		if (code == UI::ESCAPE || ElementIndex == CancelIndex) {
			UI::SetMode(UI::PLAYBACK, Ticks);
		}
		else if (code == UI::ENTER)
		{
			// Select and load the recording at the cursor.
			int	SelectedRecording = ElementIndex - 2;

			// Find the indexed item.
			StringElem*	m;
			int	i;
			for (m = List, i = 0; i < SelectedRecording; m = m->Next, i++) {
				// No-op.
			}

			Recording::SetChannel(Recording::DEMO_CHANNEL);
			
			Recording::Load(m->Name);

			UI::SetMode(UI::PLAYBACK, Ticks);
		}
	}
	
} RecordingSelectInstance;


void	RecordingSelect::AddRecordingName(const char* recname)
// Adds a recording to our instance's list.
{
	StringElem*	se = new StringElem;
	
	int	len = strlen(recname) + 1;
	se->Name = new char[len];
	strcpy(se->Name, recname);

	se->Next = RecordingSelectInstance.List;
	RecordingSelectInstance.List = se;
}


#ifdef WIN32
#define snprintf _snprintf
#endif // WIN32	

const int	NAMELEN = 60;


// UI::RECORDINGNAME -- for entering the filename to save a recording under.

class RecordingName : public UI::ModeHandler {
	char	Name[NAMELEN];
	bool	ReturnPressed;
public:
	RecordingName() {
		Name[0] = 0;
		ReturnPressed = false;
	}

	void	Open(int Ticks)
	// Set up the menu.
	{
		// Default name.  Combine player name and score.
		snprintf(Name, NAMELEN, "%s-%d", Recording::GetPlayerName(), Recording::GetScore());
		Name[NAMELEN-1] = 0;	// Ensure termination.  The MS docs are ambiguous about whether snprintf
					// always terminates the string.

		// Tack on a sequence count if the generated name already exists.
		if (Recording::RecordingExists(Name)) {
			char	test[1000];
			int	seq;
			for (seq = 1; seq < 100; seq++) {
				sprintf(test, "%s-%d", Name, seq);
				if (Recording::RecordingExists(test) == false) {
					strncpy(Name, test, NAMELEN);
					Name[NAMELEN-1] = 0;
					break;
				}
			}
		}
		
		strcat(Name, "_");
		
		UI::SetElement(0, UI::ElementData(UI::String("enter_recording_name", "  Enter Recording Name  "), false));
		UI::SetElement(1, UI::ElementData(Name, true));

		UI::SetCursor(1);

		// Empty the key-input buffer.
		char	buf[80];
		Input::GetAlphaInput(buf, 80);

		ReturnPressed = false;
	}

	void	Close()
	// Called when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
//		Logo::Update(u);

		// Check for keyboard input, and update Name accordingly.
		char	buf[80];
		if (Input::GetAlphaInput(buf, 80) > 0) {
			char*	p = buf;
			for (;;) {
				char	c = *p++;
				if (c == 0) break;

				int	len = strlen(Name);

				// Allow upper & lower-case letters.  Allow spaces, numbers, and a few other symbols after the first character.
				if ((len > 1 && ((c >= '0' && c <= '9') || c == ' ' || c == '-')) ||
				    (c >= 'A' && c <= 'Z') ||
				    (c >= 'a' && c <= 'z'))
				{
					// Add the character to Name.
					if (len < NAMELEN-1) {
						Name[len - 1] = c;
						Name[len] = '_';
						Name[len + 1] = 0;
					}
				} else if (c == 8) {
					// Backspace.  Delete the last character.
					int	len = strlen(Name);
					if (len > 1) {
						Name[len - 2] = '_';
						Name[len - 1] = 0;
					}
				} else if (c == 13) {
					// User pressed return.  Flag it.
					ReturnPressed = true;
				}
			}
		}

		UI::SetElement(1, UI::ElementData(Name, true));
	}

	void	Render(const ViewState& s)
	// Called every render frame.
	{
		// Dim the background.
		Render::FullscreenOverlay(UI::BackgroundDim);
		
//		Logo::Render(s);
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// User has taken some action.
	{
		if (code == UI::ESCAPE) {
			// Return to player query menu or main menu.
			UI::SetMode(UI::PLAYBACK, Ticks);
		} else if (ReturnPressed /* code == UI::ENTER */) {
			// Only accept the name if it's not empty.
			int	len = strlen(Name);
			if (len > 1) {
				Name[len - 1] = 0;	// Lop off the trailing '_' (i.e. the cursor).

				Recording::Save(Name);
				
				UI::SetMode(UI::PLAYBACK, Ticks);
			}

			ReturnPressed = false;
		}
	}
	
} RecordingNameInstance;



static struct InitPlayback {
	InitPlayback() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	// Attach the handler to the UI module.
	{
		UI::RegisterModeHandler(UI::PLAYBACK, &PlaybackInstance);
		UI::RegisterModeHandler(UI::RECORDINGSELECT, &RecordingSelectInstance);
		UI::RegisterModeHandler(UI::RECORDINGNAME, &RecordingNameInstance);
	}
	
} InitPlayback;

