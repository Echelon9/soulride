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
// MainMenu.cpp	-thatcher 1/7/1999 Copyright Slingshot

// Code for handling the game's main menu.


#include <stdio.h>
#include <string.h>
#ifdef LINUX
#include <dirent.h>
#else // not LINUX
#include <io.h>
#endif // not LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <list>

#include "ogl.hpp"
#include "config.hpp"
#include "ui.hpp"
#include "main.hpp"
#include "text.hpp"
#include "game.hpp"
#include "utility.hpp"
#include "music.hpp"
#include "player.hpp"
#include "sound.hpp"
#include "attractmode.hpp"
#include "recording.hpp"
#include "highscore.hpp"
#include "weather.hpp"
#include "../data/gui/guidefs.h"

#include "multiplayer.hpp"

const int	ATTRACT_MODE_TIMEOUT = 60000;
static int	InactivityTimer = 0;


#ifndef CREDITS_MOVIE
#define CREDITS_MOVIE "credits.ggm"
#endif

#ifndef OUTRO_MOVIE
#define OUTRO_MOVIE "outro.ggm"
#endif


class MainMenu : public UI::ModeHandler {
	GG_Player*	player;
	uint32	DeltaTicks;

public:

	MainMenu()
	{
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Make sure we're in attract mode.
		AttractMode::Enter();
		
		// Make sure the user is frozen.
		Game::SetUserActive(false);

		GameLoop::SetSpeedScale(1);
		
		// Set up the menu.
		char	buf[80];
		Player*	p = Game::GetCurrentPlayer();
		UI::SetElement(0, UI::ElementData(UI::String("change_player", "Change Player"), true));

#ifndef STATIC_MOUNTAIN
		UI::SetElement(1, UI::ElementData(UI::String("change_mountain", "Change Mountain"), true));
#endif
		
		sprintf(buf, "%s %d: %s", UI::String("run", "Run"), Game::GetCurrentRun()+1, Game::GetRunName(Game::GetCurrentRun()));
		UI::SetElement(2, UI::ElementData(buf, true));
		UI::SetElement(3, UI::ElementData(UI::String("carve_now", "Carve now!"), true));

		if (Config::GetBool("MultiplayerEnabled"))
		{
			UI::SetElement(5, UI::ElementData(UI::String("multi_player", "Multi Player"), true));
		}
 
 		UI::SetElement(7, UI::ElementData(UI::String("vcr", "VCR"), true));
 		UI::SetElement(8, UI::ElementData(UI::String("heli_drop", "Heli-Drop"), true));
 		UI::SetElement(9, UI::ElementData(UI::String("options", "Options"), true));
 		UI::SetElement(10, UI::ElementData(UI::String("credits", "Credits"), true));
 		UI::SetElement(11, UI::ElementData(UI::String("quit", "Quit"), true));

		UI::SetCursor(3);

		InactivityTimer = 0;

		UI::SetMenuTitle(UI::String("main_menu", "MAIN MENU"));
		UI::SetPlayerName(p->GetName());
		UI::SetMountainName(Game::GetCurrentMountain());
		
		// Play music in background.
		Music::FadeDown();
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		DeltaTicks += u.DeltaTicks;

		InactivityTimer += u.DeltaTicks;
		if (InactivityTimer >= ATTRACT_MODE_TIMEOUT) {
			UI::SetMode(UI::ATTRACT, u.Ticks);
		}
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		InactivityTimer = 0;
		
		if (code == UI::ESCAPE) {
			UI::SetMode(UI::QUITQUERY, Ticks);
		}
		else if (ElementIndex == 0) {
			Game::GetCurrentPlayer()->Save();	// Save the current player first.
			UI::SetMode(UI::PLAYERQUERY, Ticks);
		}
#ifndef STATIC_MOUNTAIN
		else if (ElementIndex == 1) UI::SetMode(UI::SELECTMOUNTAIN, Ticks);
#endif
		else if (ElementIndex == 2) UI::SetMode(UI::SELECTRUN, Ticks);
		else if (ElementIndex == 3) UI::SetMode(UI::RUNFLYOVER, Ticks);
		else if (ElementIndex == 5) UI::SetMode(UI::MULTIPLAYERQUERY, Ticks);
 		else if (ElementIndex == 7) {
			if (Recording::ChannelHasData(Recording::PLAYER_CHANNEL)) {
				Recording::SetChannel(Recording::PLAYER_CHANNEL);
			}

			UI::SetMode(UI::PLAYBACK, Ticks);
		}
		else if (ElementIndex == 8) UI::SetMode(UI::HELIDROP, Ticks);
		else if (ElementIndex == 9) UI::SetMode(UI::OPTIONS, Ticks);
//		else if (ElementIndex == 10) UI::SetMode(UI::HELP, Ticks);
		else if (ElementIndex == 10) UI::SetMode(UI::CREDITS, Ticks);
		else if (ElementIndex == 11) UI::SetMode(UI::QUITQUERY, Ticks);
	}

} MainMenuInstance;


class MultiPlayerQuery : public UI::ModeHandler {

	bool player_initialized[4];

public:
	MultiPlayerQuery() {
	  player_initialized[0] = true;
	  player_initialized[1] = false;
	  player_initialized[2] = false;
	  player_initialized[3] = false;
        }
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		UI::SetMenuTitle(UI::String("multi_player_menu", "MULTI PLAYER"));

		UI::SetElement(2, UI::ElementData(UI::String("one_player", "One Player"), true));
		UI::SetElement(3, UI::ElementData(UI::String("two_players", "Two Players"), true));
		UI::SetElement(4, UI::ElementData(UI::String("three_players", "Three Players"), true));
		UI::SetElement(5, UI::ElementData(UI::String("four_players", "Four Players"), true));

		// set cursor to the current number of players
		UI::SetCursor(1 + MultiPlayer::NumberOfLocalPlayers());
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
	  if (code != UI::ESCAPE && code != UI::LEFT) {
	    switch(ElementIndex){
	    case 2:
	      SetNumberOfPlayers(Ticks, 1);		    
	      break;
	    case 3:
	      SetNumberOfPlayers(Ticks, 2);
	      break;
	    case 4:
	      SetNumberOfPlayers(Ticks, 3);
	      break;
	    case 5:
	      SetNumberOfPlayers(Ticks, 4);
	    }
	  }
	  UI::SetMode(UI::MAINMENU, Ticks);
	}
  
  void SetNumberOfPlayers(int Ticks, int number_of_players){
    
    MultiPlayer::SetNumberOfLocalPlayers(number_of_players);
    
    int old_player_index = MultiPlayer::CurrentPlayerIndex();
    
    for (int player_index=0; player_index < number_of_players; player_index++){
      
      // make sure player is initialized
      if (! player_initialized[player_index]){
 	MultiPlayer::SetCurrentPlayerIndex(player_index);
 	UI::SetMode(UI::PLAYERQUERY, Ticks);
	player_initialized[player_index] = true;
      }
      
    }
    
    MultiPlayer::SetCurrentPlayerIndex(old_player_index);
    
  }
  
} MultiPlayerQueryInstance;


class QuitQuery : public UI::ModeHandler {

public:
	QuitQuery() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		UI::SetMenuTitle(UI::String("quit_menu", "QUIT?"));
		
		UI::SetElement(2, UI::ElementData(UI::String("quit_yes", "Yes"), true));
		UI::SetElement(3, UI::ElementData(UI::String("quit_no", "No"), true));

		UI::SetCursor(2);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
//		// Dim the background.
//		Render::FullscreenOverlay(UI::BackgroundDim);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code != UI::ESCAPE && ElementIndex == 3) {
			UI::SetMode(UI::MAINMENU, Ticks);
		} else if (code == UI::ESCAPE || ElementIndex == 2) {
			// Quit.
			Music::FadeOut();

			GG_Movie*	movie = NULL;
			if (GUI->loadMovie(const_cast<char*>(OUTRO_MOVIE), &movie) == GG_OK) {
				// Localize it.
				movie->setActorText(GGID_OUTRO1, UI::String("outro1",
															"For hints, upgrades, high scores and information on \n"
															"the revolutionary Catapult snowboard controller, visit"));
				movie->setActorText(GGID_OUTRO2, UI::String("outro2", "www.soulride.com "));
			
				// Play it.
				GameLoop::PlayShortMovie(movie);
				
				movie->unRef();
			}

			Main::SetQuit(true);
		}
	}
} QuitQueryInstance;


// UI::CREDITS handler
class Credits : public UI::ModeHandler {
	float	ScrollY;
	int	YSize;
	float	SpeedScale;

	GG_Player*	Player;
	int	DeltaTicks;
public:
	Credits() {
		ScrollY = 0;
		YSize = 0;
		Player = NULL;
		DeltaTicks = 0;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		ScrollY = 500;
		SpeedScale = 0;

		Player = GameLoop::LoadMovie(CREDITS_MOVIE);
		Player->setPlayMode(GG_PLAYMODE_LOOP);
		Player->getMovie()->setActorText(
			GGID_CREDITS_COPYRIGHT,
			UI::String("credits_rights_reserved", "Copyright 2003\nAll rights reserved"));
		
		DeltaTicks = 0;
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		Player->unRef();
		Player = NULL;
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		// Check inputs and adjust the scrolling speed.
		if (u.Inputs.Button[Input::DOWN1].State) {
			SpeedScale = 6;
		} else if (u.Inputs.Button[Input::UP1].State) {
			SpeedScale = -6;
		} else {
			SpeedScale = 1.0;
		}
		
		DeltaTicks += (int) (u.DeltaTicks * SpeedScale);
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		GameLoop::GUIBegin();
		Player->play(DeltaTicks);
		GameLoop::GUIEnd();

		DeltaTicks = 0;
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// Return to main menu on any input.
		UI::SetMode(UI::MAINMENU, Ticks);
	}
} CreditsInstance;



#ifndef STATIC_MOUNTAIN


// UI::SELECTMOUNTAIN handler
class SelectMountain : public UI::ModeHandler {
private:
	int	DoneIndex;
	int	CurrentIndex;

	StringElem*	List;
	
public:
	SelectMountain() {
		DoneIndex = 0;
		CurrentIndex = 0;
		List = NULL;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Build a list of mountain names, by looking for .srt files.
#ifdef LINUX
		DIR*	dir = opendir(".");
		struct dirent*	ent;
		while ((ent = readdir(dir))) {
			if (strcmp(Utility::GetExtension(ent->d_name), ".srt")) {
				// Not a mountain file.
				continue;
			}
			char*	filename = ent->d_name;
#else // not LINUX
		_finddata_t	d;
		long	handle = _findfirst("*.srt", &d);
		int	result = handle;
		while (result != -1) {
			char*	filename = d.name;
#endif // not LINUX
			// Add name to list.
			StringElem*	se = new StringElem;
			const char*	p = Utility::GetExtension(filename);
			int	len = p - filename;
			se->Name = new char[len];
			strncpy(se->Name, filename, len);
			se->Name[len-1] = 0;

			se->Next = List;
			List = se;
			
#ifdef LINUX
		}
		closedir(dir);
#else // not LINUX
			result = _findnext(handle, &d);
		}
		_findclose(handle);
#endif // not LINUX

		// Examine subdirectories.  Include those which contain an .srt file of the same name.
#ifdef LINUX
		dir = opendir(".");
		while ((ent = readdir(dir))) {
			char*	dirname = ent->d_name;
			// Check to see if it's actually a directory.
			struct	stat	s;
			if (stat(dirname, &s)==0 && S_ISDIR(s.st_mode)) {
				// Yup, it's a directory.
#else // not LINUX		
		handle = _findfirst("*", &d);
		result = handle;
		while (result != -1) {
			char*	dirname;
			if (d.attrib & _A_SUBDIR) {
				dirname = d.name;
#endif // not LINUX
				// Check to see if "dir/dir.srt" exists.
				char	buf[1000];
				strcpy(buf, dirname);
				strcat(buf, PATH_SEPARATOR);
				strcat(buf, dirname);
				strcat(buf, ".srt");

				struct stat	s;
				if (stat(buf, &s) == 0) {
					// Add the mountain to the list.
					StringElem*	se = new StringElem;
					int	len = strlen(dirname) + 1;
					se->Name = new char[len];
					strcpy(se->Name, dirname);
					se->Next = List;
					List = se;
				}
			}

#ifdef LINUX
		}
		closedir(dir);
#else // not LINUX
			result = _findnext(handle, &d);
		}
		_findclose(handle);
#endif // not LINUX
		
		List = Utility::SortStringList(List);
		
		ShowOptions();
	}

	void	ShowOptions()
	// Draw the menu with the current options displayed.
	{
		UI::ClearElements();

		UI::SetMenuTitle(UI::String("select_mountain", "SELECT MOUNTAIN"));
		UI::SetMountainName(Game::GetCurrentMountain());
		
		int	CurrentIndex = 0;
		UI::SetListRange(2, List->CountElements(), 8);
		char	buf[1000];
		StringElem*	m;
		int	i;		
		for (m = List, i = 0; m; m = m->Next, i++) {
			buf[0] = 0;
			uint32	Color = 0xFFFFFFFF;	// White.
			if (Utility::StringCompareIgnoreCase(m->Name, Game::GetCurrentMountain()) == 0) {
				CurrentIndex = i + 2;
				Color = 0xFFC05050;	// Red.
			}
			strcat(buf, m->Name);
			UI::SetElement(i + 2, UI::ElementData(buf, true, Color));
		}

		i++;
		DoneIndex = i + 2;
		UI::SetElement(DoneIndex, UI::ElementData(UI::String("cancel", "    Cancel    "), true));
		
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
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// Return to main menu on ESCAPE.
			UI::SetMode(UI::MAINMENU, Ticks);
			
		} else if (ElementIndex == DoneIndex) {
			// Return to main menu.
			UI::SetMode(UI::MAINMENU, Ticks);
		} else {
			// Select and load the mountain at the cursor.
			int	SelectedRun = ElementIndex - 2;

			// Find the indexed item.
			StringElem*	m;
			int	i;
			for (m = List, i = 0; i < SelectedRun; m = m->Next, i++) {
				// No-op.
			}

			AttractMode::Exit();
			GameLoop::PlayShortMovie("loading.ggm");
			Game::ClearMountain();
			Game::LoadMountain(m->Name);
			
			UI::SetMode(UI::MAINMENU, Ticks);
		}
	}
} SelectMountainInstance;


#endif	// not STATIC_MOUNTAIN


// UI::SELECTRUN handler
class SelectRun : public UI::ModeHandler {
private:
	int	DoneIndex;
	int	SelectedRun;
	int	AvailableCount;
public:
	SelectRun() {
		DoneIndex = 0;
		SelectedRun = 0;
		AvailableCount = 0;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		SelectedRun = Game::GetCurrentRun();
		Player*	p = Game::GetCurrentPlayer();
		AvailableCount = p->GetHighestRunCompleted(Game::GetCurrentMountain()) + 2;
		ShowOptions();
	}

	void	ShowOptions()
	// Draw the menu with the current options displayed.
	{
		UI::ClearElements();

		UI::SetMountainName(Game::GetCurrentMountain());
		
		UI::SetElement(1, UI::ElementData(
			UI::String("choose_a_run", "     Choose a run:                            "), false));

		char	buf[80];
		int	i;
		for (i = 0; i < Game::GetRunCount(); i++) {
			int	Color = 0xFFFFFFFF;	// White.
			buf[0] = 0;
			if (i == SelectedRun) {
				Color = 0xFFC05050;	// Red.
			} else {
			}
			strcat(buf, Game::GetRunName(i));
			bool	avail = i < AvailableCount || Config::GetBoolValue("AllowAllRuns");
			if (!avail) Color = 0xFFB0B0B0;	// Gray, if the choice isn't available.
			UI::SetElement(i + 2, UI::ElementData(buf, avail, Color));
		}

		i++;
		DoneIndex = i + 2;
		UI::SetElement(DoneIndex, UI::ElementData(UI::String("cancel", "    Cancel    "), true));
		
		UI::SetCursor(SelectedRun + 2);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		// Make sure user & camera are positioned at the start of the run that
		// was eventually selected.
		Game::ResetRun(Game::GetCurrentRun());
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		char	buf[80];
		
		// Dim the background.
		Render::FullscreenOverlay(UI::BackgroundDim);

		// Show a high-score table.
		const char*	high_scores_caption = UI::String("high_scores_caption", "High Scores");
		const char*	yours_caption = UI::String("yours_caption", "Yours");
		const char*	local_caption = UI::String("local_caption", "Local");

		float	y = UI::GetRowY(0);
		Text::DrawString(498, (int) (y+2), Text::DEFAULT, Text::ALIGN_CENTER, high_scores_caption, 0x60202020);
		Text::DrawString(500, (int) (y), Text::DEFAULT, Text::ALIGN_CENTER, high_scores_caption, 0xFF99B2FF);
		y = UI::GetRowY(1);
		Text::DrawString(478, (int) (y+2), Text::DEFAULT, Text::ALIGN_RIGHT, yours_caption, 0x60202020);
		Text::DrawString(480, (int) (y), Text::DEFAULT, Text::ALIGN_RIGHT, yours_caption, 0xFF99B2FF);
		Text::DrawString(558, (int) (y+2), Text::DEFAULT, Text::ALIGN_RIGHT, local_caption, 0x60202020);
		Text::DrawString(560, (int) (y), Text::DEFAULT, Text::ALIGN_RIGHT, local_caption, 0xFF99B2FF);

		int	i;
		for (i = 0; i < Game::GetRunCount(); i++) {
			int	Color = 0xFFFFFFFF;	// White.
			if (i == SelectedRun) {
				Color = 0xFFC05050;	// Red.
			}
			bool	avail = i < AvailableCount || Config::GetBoolValue("AllowAllRuns");
			if (!avail) Color = 0xFFB0B0B0;	// Gray, if the choice isn't available.

			y = UI::GetRowY(i + 2);
			Text::FormatNumber(buf, (float) Game::GetCurrentPlayer()->GetHighScore(i), 5, 0);
			Text::DrawString(478, (int) (y+2), Text::DEFAULT, Text::ALIGN_RIGHT, buf, 0x60202020);
			Text::DrawString(480, (int) (y), Text::DEFAULT, Text::ALIGN_RIGHT, buf, Color);

			Text::FormatNumber(buf, (float) HighScore::GetLocalHighScore(i), 5, 0);
			Text::DrawString(558, (int) (y+2), Text::DEFAULT, Text::ALIGN_RIGHT, buf, 0x60202020);
			Text::DrawString(560, (int) (y), Text::DEFAULT, Text::ALIGN_RIGHT, buf, Color);
		}
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// Return to main menu on ESCAPE.  Don't set selected run.
			UI::SetMode(UI::MAINMENU, Ticks);
			
		} else if (ElementIndex == DoneIndex) {
//			// Set the selected run, and go back to main menu.
//			Game::SetCurrentRun(SelectedRun);
			UI::SetMode(UI::MAINMENU, Ticks);
		} else {
			// Select the run at the cursor.
			SelectedRun = ElementIndex - 2;

			// Reset the user to the selected run, so they can see where it starts.
			Game::ResetRun(SelectedRun);

//			// Redraw menu.
//			ShowOptions();

			// Make the selected run current.
			Game::SetCurrentRun(SelectedRun);
			
			// Go back to the main menu.
			UI::SetMode(UI::MAINMENU, Ticks);
		}
	}
} SelectRunInstance;


// For registering the mode handlers.
static struct InitUIModes {
	InitUIModes() {
		GameLoop::AddInitFunction(Init);
	}
	static void	Init()
	// Attach the mode handler to the UI module.
	{
		UI::RegisterModeHandler(UI::MAINMENU, &MainMenuInstance);
		UI::RegisterModeHandler(UI::MULTIPLAYERQUERY, &MultiPlayerQueryInstance);
		UI::RegisterModeHandler(UI::QUITQUERY, &QuitQueryInstance);
		UI::RegisterModeHandler(UI::CREDITS, &CreditsInstance);
//		UI::RegisterModeHandler(UI::HELP, &HelpInstance);
		UI::RegisterModeHandler(UI::SELECTRUN, &SelectRunInstance);
#ifndef STATIC_MOUNTAIN
		UI::RegisterModeHandler(UI::SELECTMOUNTAIN, &SelectMountainInstance);
#endif
	}
} InitUIModes;

