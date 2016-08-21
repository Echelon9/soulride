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
// UIPlayer.cpp	-thatcher 8/17/1999 Copyright Slingshot Game Technology

// Handle player selection, name input, etc.


#include <stdio.h>
#ifdef LINUX
#include <sys/types.h>
#include <dirent.h>
#else // not LINUX
#include <io.h>
#endif // not LINUX
#include <list>
#include "config.hpp"
#include "ui.hpp"
#include "game.hpp"
#include "text.hpp"
#include "player.hpp"
#include "utility.hpp"
#include "main.hpp"
#include "music.hpp"


class PlayerQuery : public UI::ModeHandler {
public:
	PlayerQuery() {
	}

	void	Open(int Ticks)
	// Set up the menu.
	{
		// Make sure the user is frozen.
		Game::SetUserActive(false);

//		Logo::ResetAnimation();

		// If there are no saved players, then don't allow Load Player as an option.
		bool	SavedPlayers = Player::GetSavedPlayersAvailable();
		
//		char	buf[80];
		UI::ClearElements();
		UI::SetMenuTitle("");
		Player*	p = Game::GetCurrentPlayer();
		if (p) {
			UI::SetPlayerName(p->GetName());
		} else {
			UI::SetPlayerName("---");
		}
		UI::SetMountainName(Game::GetCurrentMountain());
		
		UI::SetElement(0, UI::ElementData(UI::String("new_player", "New Player"), true));
		UI::SetElement(1, UI::ElementData(UI::String("load_player", "Load Player"),
						  SavedPlayers, SavedPlayers ? 0xFFFFFFFF : 0xFF808080));

		UI::SetCursor(SavedPlayers ? 1 : 0);

		// Play music in background.
		Music::FadeDown();
	}

	void	Close()
	// Called when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render frame.
	{
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// User has taken some action.
	{
		if (code == UI::ESCAPE) {
			// Return to player query menu or main menu.
			Player*	p = Game::GetCurrentPlayer();
			if (p) {
				// There is a current player selected, so go to main menu.
				UI::SetMode(UI::MAINMENU, Ticks);
			} else {
				// No player selected yet; apparently the user wants to exit the program.
				Main::SetQuit(true);
			}
			
		} else if (ElementIndex == 0) UI::SetMode(UI::PLAYERNAME, Ticks);
		else if (ElementIndex == 1) UI::SetMode(UI::PLAYERSELECT, Ticks);
	}
	
} PlayerQueryInstance;



class PlayerSelect : public UI::ModeHandler {
	int	CancelIndex;
#if 0
	struct PlayerElem {
//		PlayerElem*	Next;
		char*	Name;

		PlayerElem() {
			Name = 0;
//			Next = 0;
		}
		PlayerElem(const PlayerElem& p) {
			Name = p.Name;
		}
		PlayerElem& operator=(const PlayerElem& p) {
			Name = p.Name;
			return *this;
		}
		~PlayerElem()
		{
			delete [] Name;
		}

		bool	operator<(const PlayerElem& p) {
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

	std::list<PlayerElem>	List;
#endif // 0
	
	int	PlayerCount;
	StringElem*	List;
	
public:
	PlayerSelect() {
		CancelIndex = 0;
		PlayerCount = 0;

		List = NULL;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Build a list of player names, by looking for .srp files.
		//PlayerCount = 0;
	  
	  // only load the list one time to avoid doubles
	  if (PlayerCount == 0){

#ifdef LINUX

		DIR*	dir = opendir(".." PATH_SEPARATOR "PlayerData");     

		struct dirent*	ent;
		while (ent = readdir(dir)) {
			char*	filename = ent->d_name;
			if (Utility::StringCompareIgnoreCase(Utility::GetExtension(filename), "srp")) {
				// Not an ".srp" file.
				continue;
			}
#else // not LINUX
		_finddata_t	d;
		long	handle = _findfirst(".." PATH_SEPARATOR
					    "PlayerData" PATH_SEPARATOR
					    "*.srp",
					    &d);
		int	result = handle;
		while (result != -1) {
			char*	filename = d.name;
#endif // not LINUX


			// Add name to list.
			PlayerCount++;
//			PlayerElem	pe;
			StringElem*	pe = new StringElem;
			const char*	p = Utility::GetExtension(filename);
			int	len = p - filename;
			pe->Name = new char[len];
			strncpy(pe->Name, filename, len);
			pe->Name[len-1] = 0;

//			List.push_front(pe);
			pe->Next = List;
			List = pe;
			
#ifdef LINUX
		}
		closedir(dir);
#else // not LINUX
			result = _findnext(handle, &d);
		}
		_findclose(handle);
#endif // not LINUX

//		List.sort();
		List = Utility::SortStringList(List);
	  
	  }
	  
	  ShowOptions();
	  
	  // Play music in background.
	  Music::FadeDown();
	}

	void	ShowOptions()
	// Draw the menu with the current options displayed.
	{
		Player*	p = Game::GetCurrentPlayer();
		
		UI::ClearElements();
		
		UI::SetMenuTitle(UI::String("choose_player_menu", "CHOOSE PLAYER"));
		if (p) {
			UI::SetPlayerName(p->GetName());
		} else {
			UI::SetPlayerName("---");
		}
		UI::SetMountainName(Game::GetCurrentMountain());

		int	CurrentIndex = 2;
		StringElem*	pe;
		int	i;
		UI::SetListRange(CurrentIndex, PlayerCount, 8);
		for (pe = List, i = 0; pe != NULL; pe = pe->Next, i++) {
			int	Color = 0xFFFFFFFF;
			if (p && (strcmp(p->GetName(), pe->Name) == 0)) {
				// This player is the one already selected, so highlight it.
				Color = 0xFFC05050;
				// And default the cursor to it.
				CurrentIndex = i + 2;
			}
			UI::SetElement(i + 2, UI::ElementData(pe->Name, true, Color));
		}

		i++;
		CancelIndex = i + 2;
		UI::SetElement(CancelIndex, UI::ElementData(UI::String("cancel", "    Cancel    "), true));
		
		UI::SetCursor(CurrentIndex);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
//		List.clear();
		if (List) {
			delete List;
			List = NULL;
		}

		PlayerCount = 0;
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render frame.
	{
//		// Dim the background.
//		Render::FullscreenOverlay(UI::BackgroundDim);
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// User has taken some action.
	{
		if (code == UI::ESCAPE || ElementIndex == CancelIndex) {
//			// Return to player query menu or main menu.
//			Player*	p = Game::GetCurrentPlayer();
//			if (p) {
//				// There is a current player selected, so go to main menu.
//				UI::SetMode(UI::MAINMENU, Ticks);
//			} else {
//				// No player selected yet, so go back to player query menu.
				UI::SetMode(UI::PLAYERQUERY, Ticks);
//			}
		} else {
			// Select and load the player at the cursor.
			int	SelectedPlayer = ElementIndex - 2;

			// Find the indexed item.
//			std::list<PlayerElem>::iterator	it;
			StringElem*	pe;
			int	i;
//			for (it = List.begin(), i = 0; i < SelectedPlayer; it++, i++) {
			for (pe = List, i = 0; i < SelectedPlayer; pe = pe->Next, i++) {
				// No-op.
			}

//			Game::LoadPlayer((*it).Name);
			Game::LoadPlayer(pe->Name);

			UI::SetMode(UI::MAINMENU, Ticks);
		}
	}
	
} PlayerSelectInstance;


class PlayerName : public UI::ModeHandler {
	char	Name[PLAYERNAME_MAXLEN];
	bool	ReturnPressed;
public:
	PlayerName() {
		Name[0] = 0;
		ReturnPressed = false;
	}

	void	Open(int Ticks)
	// Set up the menu.
	{
		Name[0] = '_';
		Name[1] = 0;

		UI::ClearElements();
		
		UI::SetMenuTitle("");
		Player*	p = Game::GetCurrentPlayer();
		if (p) {
			UI::SetPlayerName(p->GetName());
		} else {
			UI::SetPlayerName("---");
		}
		UI::SetMountainName(Game::GetCurrentMountain());
		
		UI::SetElement(0, UI::ElementData(UI::String("enter_player_name", "  Enter Player Name  "), false));
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
					if (len < PLAYERNAME_MAXLEN-1) {
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
//		// Dim the background.
//		Render::FullscreenOverlay(UI::BackgroundDim);
		
//		Logo::Render(s);
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// User has taken some action.
	{
		if (code == UI::ESCAPE) {
			// Return to player query menu or main menu.
//			Player*	p = Game::GetCurrentPlayer();
//			if (p) {
//				// There is a current player selected, so go to main menu.
//				UI::SetMode(UI::MAINMENU, Ticks);
//			} else {
//				// No player selected yet, so go back to player query menu.
				UI::SetMode(UI::PLAYERQUERY, Ticks);
//			}
		} else if (ReturnPressed /* code == UI::ENTER */) {
			// Only accept the name if it's not empty.
			int	len = strlen(Name);
			if (len > 1) {
				Name[len - 1] = 0;	// Lop off the trailing '_' (i.e. the cursor).

				// If a matching file exists, we should ask the user whether we should overwrite it or not.
				
				Game::LoadPlayer(Name);
				
				UI::SetMode(UI::MAINMENU, Ticks);
			}

			ReturnPressed = false;
		}
	}
	
} PlayerNameInstance;



// For registering the mode handlers.
static struct InitUIPlayer {
	InitUIPlayer() {
		GameLoop::AddInitFunction(Init);
	}
	static void	Init()
	// Attach the mode handlers to the UI module.
	{
		UI::RegisterModeHandler(UI::PLAYERQUERY, &PlayerQueryInstance);
		UI::RegisterModeHandler(UI::PLAYERNAME, &PlayerNameInstance);
		UI::RegisterModeHandler(UI::PLAYERSELECT, &PlayerSelectInstance);
	}
} InitUIPlayer;

