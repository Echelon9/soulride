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
// attractmode.cpp	-thatcher 4/18/2000 Copyright Slingshot

// Code for attract mode.


#include "attractmode.hpp"
#include "utility.hpp"
#include "recording.hpp"
#include "gameloop.hpp"
#include "game.hpp"
#include "usercamera.hpp"
#include "ui.hpp"
#include "player.hpp"
#include "text.hpp"
#include "music.hpp"
#include "gamegui/gamegui.h"
#include "../data/gui/guidefs.h"
#include "config.hpp"


#ifndef ATTRACT_MOVIE
#define ATTRACT_MOVIE "attract.ggm"
#endif


namespace AttractMode {
;


bool	Active = false;
int	ChangeViewTimer = 0;


static void	ChooseRecording();


void	Enter()
// Called when app wants to activate the attract mode.  Take inventory
// of the available recordings for the current mountain, and start playing
// one of them.
{
	if (Active == false) {
		Active = true;

		// Pick a recording to play back.
		ChooseRecording();

		// Setup camera.
		UserCamera::SetSubject(Game::GetUser());
		UserCamera::SetAimMode(UserCamera::LOOK_AT);
		UserCamera::SetMotionMode(UserCamera::FIXED);
		ChangeViewTimer = 0;
	}
}


void	Exit()
// Called when the app wants to deactivate the attract mode.
{
	Active = false;

	UserCamera::SetAutoCameraMode(UserCamera::AUTO_OFF);
	Recording::SetMode(Recording::STOP, GameLoop::GetCurrentTicks());
}


void	Update(const UpdateState& u)
// Update the attract mode, if we're active.
{
	if (Active) {
		Recording::MoveCursor(u.DeltaTicks);

		if (Recording::GetAtEnd()) {
			// Choose a recording.
			ChooseRecording();
		}

		// Deal with camera.
		ChangeViewTimer -= u.DeltaTicks;
		if (ChangeViewTimer <= 0) {
			ChangeViewTimer = Utility::RandomInt(2000) + 3000;

			UserCamera::RandomAutoCameraMode();
		}
	}
}


struct DemoName {
	char*	Name;
	DemoName*	Next;
};
DemoName*	NameList = NULL;
int	NameCount = 0;


static void	AddNameToList(const char* name)
// Adds the given name to our list of demos.
{
	int	len = strlen(name);
	DemoName*	d = new DemoName;
	d->Name = new char[len+1];
	strcpy(d->Name, name);
	d->Next = NameList;
	NameList = d;
	NameCount++;
}


static void	ClearNameList()
// Delete the list of names.
{
	DemoName*	d = NameList;
	while (d) {
		DemoName*	n = d->Next;
		delete [] d->Name;
		delete d;
		d = n;
	}
	
	NameList = NULL;
	NameCount = 0;
}

   
static void	ChooseRecording()
// Pick a recording to play back.
{
	// Enumerate all recordings which correspond to the current mountain.
	Recording::EnumerateRecordings(Game::GetCurrentMountain(), AddNameToList);

	// Pick a recording.
	
	int	count = NameCount;
	if (Recording::ChannelHasData(Recording::PLAYER_CHANNEL)) {
		count++;
	}

	int	chosen = Utility::RandomInt(count);

	if (chosen >= NameCount) {
		// Use PLAYER_CHANNEL; i.e. the most recent user recording.
		Recording::SetChannel(Recording::PLAYER_CHANNEL);
	} else {
		Recording::SetChannel(Recording::DEMO_CHANNEL);

		// Find the name of the chosen recording, and load it.
		DemoName*	d = NameList;
		while (chosen) {
			d = d->Next;
			chosen--;
		}
		Recording::Load(d->Name);
	}

	ClearNameList();

	Recording::SetMode(Recording::STOP, GameLoop::GetCurrentTicks());
	Recording::SetMode(Recording::PLAY, GameLoop::GetCurrentTicks());

//	Recording::MoveCursor(-1000000);	// xxx make sure movie is rewound.
}


//
// UI mode handler for dedicated attract mode.  It's just a simple shell
// which turns on the attract mode, plays a GameGUI movie over the top,
// and enters the main menu structure on receiving input.
//


class Attract : public UI::ModeHandler {
	GG_Player*	player;
	uint32	DeltaTicks;
	
public:
	Attract() {
		player = NULL;
		DeltaTicks = 0;
	}

	~Attract() {
//		// Release the movie player.
//		if (player) {
//			player->unRef();
//			player = NULL;
//		}
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Make sure we're in attract mode.
		AttractMode::Enter();
		
		// Make sure the user is frozen.
		Game::SetUserActive(false);

		// Get the name of the movie to use.
		const char*	AttractMovieName = Config::Get("AttractMovie");
		if (AttractMovieName == NULL) {
			// Use the hard-coded default.
			AttractMovieName = ATTRACT_MOVIE;
		}

		// Set up a background movie.
		if (player == NULL) {
			// Load the movie.
			GameLoop::AutoPauseInput	autoPause;
			GG_Movie*	movie = NULL;
			if (GUI->loadMovie(const_cast<char*>(AttractMovieName), &movie) == GG_OK) {
				movie->setPlayMode(GG_PLAYMODE_LOOP);
				GUI->createPlayer(movie, &player);
				player->setPlayMode(GG_PLAYMODE_LOOP);

				// Localize.
				movie->setActorText(GGID_FEATURE1, UI::String("attract_feature1", "no visibility limits"));
				movie->setActorText(GGID_FEATURE2, UI::String("attract_feature2", "physics-based gameplay"));
				movie->setActorText(GGID_FEATURE3, UI::String("attract_feature3", "real world locations"));
				movie->setActorText(GGID_FEATURE4, UI::String("attract_feature4", "find your own route"));

				movie->unRef();
			}
		} else {
			// Rewind the movie.
			player->setTime(0);
		}
		DeltaTicks = 0;

		// Turn off cd-audio background music.
//		Music::FadeOut();
		Music::FadeDown();
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		if (player) player->stop();
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		DeltaTicks += u.DeltaTicks;
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Play the movie.
		if (DeltaTicks > 0 && player != NULL) {
//			glMatrixMode(GL_PROJECTION);
//			glPushMatrix();
//			glLoadIdentity();
//			glMatrixMode(GL_MODELVIEW);
//			glPushMatrix();
//			glLoadIdentity();
//			Render::DisableZBuffer();
//			Render::SetTexture(NULL);
//			Render::CommitRenderState();
			
			GameLoop::GUIBegin();
			player->play(DeltaTicks);
			DeltaTicks = 0;
			GameLoop::GUIEnd();
			
//			glMatrixMode(GL_MODELVIEW);
//			glPopMatrix();
//			glMatrixMode(GL_PROJECTION);
//			glPopMatrix();
		}
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// Go to either main menu or player menu.
		if (Game::GetCurrentPlayer()) {
			// There's a loaded player.  Go to main menu.
			UI::SetMode(UI::MAINMENU, Ticks);
			
		} else if (Player::GetSavedPlayersAvailable()) {
			// No player loaded, but some on disk: let the user load a player or create a new one.
			UI::SetMode(UI::PLAYERQUERY, Ticks);
			
		} else {
			// No loaded player, none on disk: just go straight to player-name entry screen.
			UI::SetMode(UI::PLAYERNAME, Ticks);
		}
	}
} AttractInstance;


// For registering the mode handler.
static struct InitUIMode {
	InitUIMode() {
		GameLoop::AddInitFunction(Init);
	}
	static void	Init()
	// Attach the mode handler to the UI module.
	{
		UI::RegisterModeHandler(UI::ATTRACT, &AttractInstance);
	}
} InitUIMode;


};	// end namespace AttractMode

