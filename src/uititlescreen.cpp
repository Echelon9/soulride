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
// UITitleScreen.cpp	-thatcher 1/6/1999 Copyright Slingshot

// Code for showing the title screen.


#include "gameloop.hpp"
#include "ui.hpp"
#include "psdread.hpp"
#include "render.hpp"
#include "sound.hpp"
#include "game.hpp"
#include "player.hpp"


class TitleScreen : public UI::ModeHandler {

//	bool	FirstTime = true;
	int	TitleEndTicks;
//	bool	NeedToShowTitle;
	
public:
	TitleScreen() {
		TitleEndTicks = 0;
//		NeedToShowTitle = false;
	}

	~TitleScreen() {
//		if (TitleImage) delete TitleImage;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering TITLESCREEN mode.
	{
		// Make sure the user is disabled.
		Game::SetUserActive(false);
		
//		UI::SetDontUpdateGame(true);
		
		TitleEndTicks = Ticks /* + 250 */;
//		NeedToShowTitle = true;

//		GameLoop::ShowLogo();
		
//		Sound::Play("intromusic.wav", Sound::Controls(1.0));
	}

	void	Close()
	// Called by UI when exiting TITLESCREEN mode.
	{
//		UI::SetDontUpdateGame(false);
	}

	void	Update(const UpdateState& u)
	// Check to see if we're done showing the title screen.
	{
		if (u.Ticks >= TitleEndTicks) {
			Done(u.Ticks);
		}
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// Early exit on button press.
		Done(Ticks);
	}

	void	Done(int Ticks)
	// Goes on to the introductory UI mode.
	{
		if (Player::GetSavedPlayersAvailable()) {
			// Let the user load a player or create a new one.
			UI::SetMode(UI::PLAYERQUERY, Ticks);
		} else {
			// No saved player files, so just go straight to player-name entry screen.
			UI::SetMode(UI::PLAYERNAME, Ticks);
		}
	}
} Instance;


static struct InitTitleScreen {
	InitTitleScreen() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	// Attach the title screen handler to the UI module.
	{
		UI::RegisterModeHandler(UI::TITLESCREEN, &Instance);
	}
	
} InitTitleScreen;

