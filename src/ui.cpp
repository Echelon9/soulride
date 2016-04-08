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
// ui.cpp	-thatcher 1/6/1999 Copyright Slingshot

// Code for UI module, which coordinates the game user interface.


#include "utility.hpp"
#include "ui.hpp"
#include "game.hpp"
#include "error.hpp"
#include "text.hpp"
#include "render.hpp"
#include "config.hpp"
#include "imageoverlay.hpp"
#include "lua.hpp"
#include "../data/gui/guidefs.h"

#include "multiplayer.hpp"

#include <iostream>

namespace UI {
;


bool	IsOpen = false;
bool	DontUpdateGame = false;
Mode	CurrentMode[MultiPlayer::MAX_PLAYERS] = {ATTRACT, ATTRACT, ATTRACT, ATTRACT};
ModeHandler*	Handler[MODECOUNT];
::Render::Texture*	Cursor = NULL;

Mode	DeferredMode = NONE;


::Render::Texture*	CursorIcon[CURSOR_TYPE_COUNT];

const char*
IconName[CURSOR_TYPE_COUNT] = {
	"arrow-cursor.psd",
	"cross-cursor.psd",
	"mapcursor.psd",
	"dropmarker.psd",
};


GG_Player*	MenuBackdrop[MultiPlayer::MAX_PLAYERS] = {NULL, NULL, NULL, NULL};
GG_Player*	StatusBackdrop[MultiPlayer::MAX_PLAYERS] = {NULL, NULL, NULL, NULL};

/**
 * Set current run as finished and progress to next run.
 */
static void
ShowFinish_lua() {
	// Ensure SHOWFINISH mode not set in menus prior to run
	if (Game::GetCurrentPlayer()) {
		SetMode(SHOWFINISH, GameLoop::GetCurrentTicks());
	}
}


void	Open()
// Initialize the UI module.
{
	DontUpdateGame = false;

	{for (int i=0; i<MultiPlayer::MAX_PLAYERS; i++)
	{
		CurrentMode[i] = NONE;
	}}

	{for (int i = 0; i < MODECOUNT; i++) {
		Handler[i] = 0;
	}}

	// Init the menu and status background movies.
	for (int i=0; i<MultiPlayer::MAX_PLAYERS; i++){
		MenuBackdrop[i] = GameLoop::LoadMovie("hud-menu.ggm");
		MenuBackdrop[i]->setPlayMode(GG_PLAYMODE_HANG);
		StatusBackdrop[i] = GameLoop::LoadMovie("player_status.ggm");
		StatusBackdrop[i]->setPlayMode(GG_PLAYMODE_HANG);
 	}

	IsOpen = true;

	{for (int i = 0; i < CURSOR_TYPE_COUNT; i++) {
		CursorIcon[i] = NULL;
	}}

	lua_register("ui_show_finish", ShowFinish_lua);
}


void	Close()
// Do any necessary destruction before exiting.
{
	// Get rid of the background movies.
	for (int i=0; i<MultiPlayer::MAX_PLAYERS; i++){
	  MenuBackdrop[i]->unRef();
	  StatusBackdrop[i]->unRef();
	}

	if (Cursor) {
		delete Cursor;
		Cursor = NULL;
	}
	IsOpen = false;
}


void	MountainInit()
// Called during load of dataset.
{
	int	i;
	for (i = 0; i < MODECOUNT; i++) {
		if (Handler[i]) Handler[i]->MountainInit();
	}

	for (i = 0; i < CURSOR_TYPE_COUNT; i++) {
		if (CursorIcon[i] == NULL) {
			CursorIcon[i] = ::Render::NewTexture(IconName[i], true, true, false);
		}
	}
}


void	MountainClear()
// Called during clearing out of dataset.
{
	int	i;
	for (i = 0; i < MODECOUNT; i++) {
		if (Handler[i]) Handler[i]->MountainClear();
	}
}


bool	GetDontUpdateGame()
// Return TRUE if we don't want the game loop to update game logic or do any rendering.
{
	return DontUpdateGame;
}


void	SetDontUpdateGame(bool b)
// Call with a value of true if you don't want the game loop to update logic
// or do rendering.
{
	DontUpdateGame = b;
}


void	DrawCursor(float x, float y, CursorType t)
// Paints the specified cursor icon at the given 2D x,y coordinates.
// Logical coordinates go from 0,0 at upper left to 640,480 at lower
// right (regardless of the actual display resolution).
{
	if (t < 0 || t >= CURSOR_TYPE_COUNT) t = ARROW;

	::Render::Texture*	b = CursorIcon[t];
	ImageOverlay::Draw((int) (x - b->GetWidth()/2), (int) (y - b->GetHeight()/2), b->GetWidth(), b->GetHeight(), b, 0, 0);
}


void	PreUpdate(const UpdateState& u)
// Called first thing by the game loop.
{
}

void	SetModeAll(Mode mode, int Ticks){
	for (int i=0; i<MultiPlayer::NumberOfLocalPlayers(); i++)
		SetMode(mode, Ticks, i);
}

void	SetMode(Mode mode, int Ticks)
{
  
	int player_index = MultiPlayer::CurrentPlayerIndex();

	// set playing mode for all players if the game is to start
	if (mode == UI::PLAYING || mode == UI::COUNTDOWN){
		SetModeAll(mode, Ticks);
		return;
	}
  
	/*
	 * Wait for all other players 
	 * When all players are waiting, start countdown
	 */
	if (mode == UI::WAITING_FOR_OTHERS){

		SetMode(UI::WAITING_FOR_OTHERS, Ticks, player_index);
    
		bool everone_is_waiting = true;
		for (int i=0; i<MultiPlayer::NumberOfLocalPlayers(); i++){
			if (CurrentMode[i] != WAITING_FOR_OTHERS)
				everone_is_waiting = false;
		}
		if (everone_is_waiting){
			SetModeAll(UI::COUNTDOWN, Ticks);
			return;
		}
	}

	/*
	 * When one player decides to start the game, 
	 * start the others too, if they are in the main menu. 
	 */
	if (mode == RUNFLYOVER){
		for (int i=0; i<MultiPlayer::NumberOfLocalPlayers(); i++){
			if (i != player_index && CurrentMode[i] == UI::MAINMENU){
				SetMode(UI::RUNFLYOVER, Ticks, i);
			}
		}
	}
  
	/* 
	 * If other players are waiting, 
	 * start Runflyover when going back to main menu 
	 */
	if (mode == UI::MAINMENU){
		for (int i=0; i<MultiPlayer::NumberOfLocalPlayers(); i++){
			if (i != player_index && 
				(CurrentMode[i] == WAITING_FOR_OTHERS
				 || CurrentMode[i] == RUNFLYOVER
				 || CurrentMode[i] == SHOWRUNINFO)){
				SetMode(RUNFLYOVER, Ticks, player_index);
//				printf("Player %d forced to start game\n", player_index + 1);
				return;
			}
		}
	}

	// wait for other players to finish if one player wants to exit from the game
	// TODO: make this player's view follow the other players like a helicopter (would look really nice)
	/*if (CurrentMode[player_index] == PLAYING && mode == MAINMENU){
	  for (int i=0; i<MultiPlayer::CurrentPlayerIndex(); i++){
      if (i != player_index && CurrentMode[i] == PLAYING){
	  // someone else is still playing
	  CurrentMode[player_index] = WAITING_FOR_OTHERS;
	  return;
      }
	  }
	  }*/
  
  
	// set mode for current player
	SetMode(mode, Ticks, player_index);
}

void	SetMode(Mode mode, int Ticks, int player_index)
// Changes the current UI mode.  Calls Close() for the handler of the current
// mode, and Open() for the handler of the new mode.
{
	if (mode == CurrentMode[player_index]) return;	// No need to change.

	// Clear out any stale mouse events when changing modes (not all
	// modes consume mouse events, so we could have stale events
	// sitting around).
	Input::ClearMouseEvents();
  
	// Bjorn debugging / understanding what happends
	char* mode_names[] = 
		{ "NONE","ATTRACT","PLAYERQUERY","PLAYERNAME","PLAYERSELECT","MAINMENU",
		  "SELECTMOUNTAIN","SELECTRUN","SHOWRUNINFO","RUNFLYOVER",
		  "COUNTDOWN","RESUME","PLAYING","SHOWCRASH","CRASHQUERY","SHOWREWIND","SHOWFINISH",
		  "FINISHTOTAL","FINISHQUERY","QUITQUERY","OPTIONS","DISPLAYOPTIONS","SOUNDOPTIONS",
		  "CONTROLOPTIONS","WEATHEROPTIONS","CREDITS","HELP","HELIDROP","RIDELIFT","CAMERA",
		  "PLAYBACK","RECORDINGSELECT","RECORDINGNAME","PERFTEST",
		  // multi-player stuff 
		  "MULTIPLAYERQUERY","WAITING_FOR_OTHERS", 
		  "MODECOUNT"};
  
//   // Bjorn: debugging
//   std::cout << "oldmode = " << mode_names[CurrentMode[player_index]] 
// 	    << ", newmode = " << mode_names[mode] 
// 	    << " for player_index " << player_index << std::endl;
  
  
	ModeHandler*	OldHandler = Handler[CurrentMode[player_index]];
	ModeHandler*	NewHandler = Handler[mode];

	/*
	 * Clean up the old mode. (if not used by another player)
	 * Some modes (RUNFLYOVER) need to be cleaned up after each player, 
	 * to stop movies from playing
	 */
	bool oldhandler_used = false;
	for (int i = 0; i<MultiPlayer::NumberOfLocalPlayers(); i++)
		if (CurrentMode[i] == CurrentMode[player_index] && i != player_index)
			oldhandler_used = true;
  
	if (OldHandler){
		if ((! oldhandler_used) || CurrentMode[player_index] == UI::RUNFLYOVER) {
//			printf("closing mode %s for player_index %d\n", mode_names[CurrentMode[player_index]], player_index);
			OldHandler->Close();
		}
	}

	// Change mode for player
	CurrentMode[player_index] = mode;
  
	// Clear the menu.
	ClearElements(player_index);
  
//	if (GameLoop::GetIsInIntroMode()) {
//		DeferredMode = mode;
//	} else {
	// Start up the new mode.
	if (NewHandler) NewHandler->Open(Ticks);
//	}
}


Mode	GetMode()
// Returns the current mode.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();
	return CurrentMode[player_index];
}

Mode	GetMode(int player_index){
	// Returns the current mode.
	return CurrentMode[player_index];
}


//
// Data for presenting menus.
//
const int	MAX_ELEMENTS = 100;
int	ElementCount[MultiPlayer::MAX_PLAYERS] = {0, 0, 0, 0};
struct ElementDataWithStorage {
	ElementDataWithStorage() { Text[0] = 0; CursorStop = false; }
	const ElementData&	operator=(const ElementData& e)
	// Copy the data from the given structure.
	{
		strncpy(Text, e.Text, 60);
		Text[60 - 1] = 0;
		CursorStop = e.CursorStop;
		Color = e.Color;
		return e;
	}
	char	Text[60];
	bool	CursorStop;
	uint32	Color;
} Element[MAX_ELEMENTS][MultiPlayer::MAX_PLAYERS];
int	CursorPosition[MultiPlayer::MAX_PLAYERS] = {0, 0, 0, 0};
int	MaxElementWidth = 0;

int	ListStartIndex[MultiPlayer::MAX_PLAYERS] = { -1, -1, -1, -1};
int	ListDisplayOffset = 0;
int	ListEntries = 0;
int	ListShowLines = 10;

bool	ShowMenuBackdrop[MultiPlayer::MAX_PLAYERS] = {false, false, false, false};
bool	ShowStatusBackdrop[MultiPlayer::MAX_PLAYERS] = {false, false, false, false};

void	ClearElements(){
  int player_index = MultiPlayer::CurrentPlayerIndex();
  ClearElements(player_index);
}

void	ClearElements(int player_index)
// Resets the UI elements.  Follow this call with calls to SetElement()
// to set up a menu.  If no elements are set, then the UI module doesn't
// look for input.
{
	int	i;
	for (i = 0; i < MAX_ELEMENTS; i++) {
		Element[i][player_index].Text[0] = 0;
		Element[i][player_index].CursorStop = false;
		Element[i][player_index].Color = 0xFFFFFFFF;
	}
	ElementCount[player_index] = 0;
	CursorPosition[player_index] = 0;
	ListStartIndex[player_index] = -1;

	ShowMenuBackdrop[player_index] = false;
	ShowStatusBackdrop[player_index] = false;
}

void	SetElement(int index, const ElementData& data){
  int player_index = MultiPlayer::CurrentPlayerIndex();
  SetElement(index, data, player_index);
}

void	SetElement(int index, const ElementData& data, int player_index)
// Sets the contents of the index'th element to match the given data.
{
	if (index < 0 || index >= MAX_ELEMENTS) {
		// Throw an exception?  Use an assertion?
		return;
	}

	// Copy the given data into the element slot.
	Element[index][player_index] = data;

	// Update the ElementCount if necessary.
	if (index + 1 > ElementCount[player_index]) {
		ElementCount[player_index] = index + 1;
	}

	// Compute the maximum width (in pixels) of the widest element,
	// for centering the menu on the screen.
	MaxElementWidth = 0;
	int i;
	for (i = 0; i < ElementCount[player_index]; i++) {
		int	w = Text::GetWidth(Text::DEFAULT, Element[i][player_index].Text);
		if (w > MaxElementWidth) MaxElementWidth = w;
	}
}


void	SetMenuTitle(const char* title)
// Enables the display of the menu backdrop, and also
// sets the title above it.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();
	ShowMenuBackdrop[player_index] = true;
	MenuBackdrop[player_index]->getMovie()->setActorText(GGID_MENU_TITLE, title);
	MenuBackdrop[player_index]->setTime(0);
}


void	SetPlayerName(const char* playername)
// Enables the display of the status backdrop, and sets the player
// name in the backdrop.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();
  
	ShowStatusBackdrop[player_index] = true;
	StatusBackdrop[player_index]->getMovie()->setActorText(GGID_PLAYER_NAME, playername);
	StatusBackdrop[player_index]->setTime(0);
  
	if (MultiPlayer::NumberOfLocalPlayers() == 1)
	{
		StatusBackdrop[0]->getMovie()->setActorText(GGID_PLAYER_CAPTION, UI::String("player_caption", "Player:"));
	}
	else
	{
		StatusBackdrop[0]->getMovie()->setActorText(GGID_PLAYER_CAPTION, "Player 1:");
	}
  
	switch(player_index){
	case 1:
		StatusBackdrop[1]->getMovie()->setActorText(GGID_PLAYER_CAPTION, "Player 2:");
		break;
	case 2:
		StatusBackdrop[2]->getMovie()->setActorText(GGID_PLAYER_CAPTION, "Player 3:");
		break;
	case 3:
		StatusBackdrop[3]->getMovie()->setActorText(GGID_PLAYER_CAPTION, "Player 4:");
		break;
	default:
		// Leave the default.
		break;
	}
}


void	SetMountainName(const char* mountainname)
// Enables the display of the status backdrop, and sets the mountain
// name in the backdrop.
{
	for (int i=0; i<MultiPlayer::MAX_PLAYERS; i++){
		ShowStatusBackdrop[i] = true;
		StatusBackdrop[i]->getMovie()->setActorText(GGID_MOUNTAIN_NAME, mountainname);
		StatusBackdrop[i]->setTime(0);

		StatusBackdrop[i]->getMovie()->setActorText(GGID_MOUNTAIN_CAPTION, UI::String("mountain_caption", "Mountain:"));
	}
}


static void	CursorBack(int player_index)
// Moves the cursor to the previous cursor stop, if there is one.  If there
// isn't one, then leaves the cursor at its current position.
{
	CursorPosition[player_index] = iclamp(0, CursorPosition[player_index], ElementCount[player_index] - 1);
	if (ElementCount[player_index] <= 0) return;
	int	i = CursorPosition[player_index];
	do {
		i--;
		if (i < 0) i = ElementCount[player_index] - 1;
		if (Element[i][player_index].CursorStop) {
			CursorPosition[player_index] = i;
			return;
		}
	} while (i != CursorPosition[player_index]);
}


static void	CursorForward(int player_index)
// Moves the cursor to the next cursor stop, if there is one.  If there
// isn't one, then leaves the cursor at its current position.
{
	CursorPosition[player_index] = iclamp(0, CursorPosition[player_index], ElementCount[player_index] - 1);
	if (ElementCount[player_index] <= 0) return;
	int	i = CursorPosition[player_index];
	do {
		i++;
		if (i >= ElementCount[player_index]) i = 0;
		if (Element[i][player_index].CursorStop) {
			CursorPosition[player_index] = i;
			return;
		}
	} while (i != CursorPosition[player_index]);
}

void	SetCursor(int index)
// Positions the cursor at the given element.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();
	SetCursor(index, player_index);
}

void	SetCursor(int index, int player_index)
// Positions the cursor at the given element.
{
	CursorPosition[player_index] = iclamp(0, index, ElementCount[player_index] - 1);

	if (Element[CursorPosition[player_index]][player_index].CursorStop == false) {
		// Go to next cursor stop.
		CursorForward(player_index);
	}
}


void	SetListRange(int start_index, int entries, int show_lines)
// Defines a scrollable list within the list of UI elements.
// start_index and entries define the bounds of the scrolling data
// elements in the UI elements, and show_lines defines how many lines
// the list display should take up.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();

	// Bjorn TODO: index the following elements

	ListStartIndex[player_index] = start_index;
	ListDisplayOffset = 0;
	ListEntries = entries;
	ListShowLines = show_lines;
}

float	GetRowY(int element_index){
	int player_index = MultiPlayer::CurrentPlayerIndex();
	return GetRowY(element_index, player_index);
}

float	GetRowY(int element_index, int player_index)
// Returns the y value of the given element's baseline for
// rendering text.
// XXX doesn't take list into account yet XXX
{
	int	ystep = Text::GetFontHeight(Text::DEFAULT) + 3;
	int	y = (480 - ElementCount[player_index] * ystep) / 2;

	return float(y + ystep * element_index);
}


static int	DeltaTicks[MultiPlayer::MAX_PLAYERS] = {0, 0, 0, 0};


void	Update(const UpdateState& u)
// Check for button presses and whatnot, and notify the current mode's
// handler of any user actions.
{
	if (!IsOpen) return;

	for (int i=0; i<MultiPlayer::MAX_PLAYERS; i++)
		DeltaTicks[i] += u.DeltaTicks;
	
	// See if there's a deferred mode we need to open now.
	if (DeferredMode != NONE) {
		// Open the deferred mode.
		ModeHandler*	h = Handler[DeferredMode];
		h->Open(u.Ticks);
		DeferredMode = NONE;
	}
	
	// Depending on whether "PerfTest" or "Camera" are defined, start in different modes.
	for (int i=0; i<MultiPlayer::MAX_PLAYERS; i++){
		if (CurrentMode[i] == NONE) {
			if (Config::GetBoolValue("PerfTest")) {
				// Performance test.
				SetMode(PERFTEST, u.Ticks, i);
			} else if (Config::GetBoolValue("Camera")) {
				// 6DOF camera for looking at the world.
				SetMode(CAMERA, u.Ticks, i);
			} else {
				// Ordinary game.
				SetMode(ATTRACT, u.Ticks, i);
			}
		}
	}


	// Call the mode handler's update function.
	for (int i=0; i<MultiPlayer::NumberOfLocalPlayers(); i++)
		if (Handler[CurrentMode[i]]) 
			Handler[CurrentMode[i]]->Update(u);
	
//	const Input::InputState&	i = u.Inputs;

	// Process input events.
	int	seq = 0;
	Input::EventInfo	e;
	while ((seq = Input::GetNextEvent(seq, &e))) {
		if (e.Down == false) {
			// Ignore key-up events.
			continue;
		}
		
		// Menu navigation inputs.

		// player 1
		if (e.ID == Input::UP1 || e.ID == Input::BUTTON2) {
			if (ElementCount[0] != 0) {
				CursorBack(0);
			}
		} else if (e.ID == Input::DOWN1 || e.ID == Input::BUTTON1) {
			if (ElementCount[0] != 0) {
				CursorForward(0);
			}
		}
		// player 2
		if (e.ID == Input::UP2 || e.ID == Input::BUTTON2) {
			if (ElementCount[1] != 0) {
				CursorBack(1);
			}
		} else if (e.ID == Input::DOWN2 || e.ID == Input::BUTTON1) {
			if (ElementCount[1] != 0) {
				CursorForward(1);
			}
		}
		// player 3
		if (e.ID == Input::UP3 || e.ID == Input::BUTTON2) {
			if (ElementCount[2] != 0) {
				CursorBack(2);
			}
		} else if (e.ID == Input::DOWN4 || e.ID == Input::BUTTON1) {
			if (ElementCount[2] != 0) {
				CursorForward(2);
			}
		}
		// player 4
		if (e.ID == Input::UP4 || e.ID == Input::BUTTON2) {
			if (ElementCount[3] != 0) {
				CursorBack(3);
			}
		} else if (e.ID == Input::DOWN4 || e.ID == Input::BUTTON1) {
			if (ElementCount[3] != 0) {
				CursorForward(3);
			}
		}

		// Forward user actions to the handler.

		// player 1
		int player_index = 0;
		MultiPlayer::SetCurrentPlayerIndex(player_index);
		ModeHandler*	h = Handler[CurrentMode[player_index]];
		if (h){
			if (e.ID == Input::LEFT1) {
				h->Action(CursorPosition[player_index], LEFT, u.Ticks);
			}
			if (e.ID == Input::RIGHT1) {
				h->Action(CursorPosition[player_index], RIGHT, u.Ticks);
			}
			if (e.ID == Input::ENTER || e.ID == Input::BUTTON0) {
				h->Action(CursorPosition[player_index], ENTER, u.Ticks);
			}
			if (e.ID == Input::ESCAPE || e.ID == Input::BUTTON3) {
				h->Action(CursorPosition[player_index], ESCAPE, u.Ticks);
			}
		}

		// player 2
		player_index = 1;
		MultiPlayer::SetCurrentPlayerIndex(player_index);
		h = Handler[CurrentMode[player_index]];
		if (h){
			if (e.ID == Input::LEFT2) {
				h->Action(CursorPosition[player_index], LEFT, u.Ticks);
			}
			if (e.ID == Input::RIGHT2) {
				h->Action(CursorPosition[player_index], RIGHT, u.Ticks);
			}
		}

		// player 3
		player_index = 2;
		MultiPlayer::SetCurrentPlayerIndex(player_index);
		h = Handler[CurrentMode[player_index]];
		if (h){
			if (e.ID == Input::LEFT3) {
				h->Action(CursorPosition[player_index], LEFT, u.Ticks);
			}
			if (e.ID == Input::RIGHT3) {
				h->Action(CursorPosition[player_index], RIGHT, u.Ticks);
			}
		}
		
		// player 4
		player_index = 3;
		MultiPlayer::SetCurrentPlayerIndex(player_index);
		h = Handler[CurrentMode[player_index]];
		if (h){
			if (e.ID == Input::LEFT4) {
				h->Action(CursorPosition[player_index], LEFT, u.Ticks);
			}
			if (e.ID == Input::RIGHT4) {
				h->Action(CursorPosition[player_index], RIGHT, u.Ticks);
			}
		}
	}
}


void	Render(const ViewState& s, int player_index)
// Draw the current UI elements.
{
	if (!IsOpen) return;

	// Show backdrops, if enabled.
	if (ShowMenuBackdrop[player_index]) {
		GameLoop::GUIBegin();
		MenuBackdrop[player_index]->play(DeltaTicks[player_index]);
		GameLoop::GUIEnd();
	}
	if (ShowStatusBackdrop[player_index]) {
		GameLoop::GUIBegin();
		StatusBackdrop[player_index]->play(DeltaTicks[player_index]);
		GameLoop::GUIEnd();
	}
	DeltaTicks[player_index] = 0;
	
	// Call the handler, to give it a chance to render.
	ModeHandler*	h = Handler[CurrentMode[player_index]];
	if (h) {
		h->Render(s);
	}

	if (ElementCount[player_index] <= 0) {
		// No menu elements, so we're done.
		return;
	}
	
	// Make sure we have a cursor image.
	if (Cursor == NULL) {
		Cursor = ::Render::NewTexture("cursor.psd", true, false, false);
	}
	
	// Update the list display offset if necessary, based on where the cursor is, to
	// make sure the cursor is visible.
	if (ListStartIndex[player_index] > -1 && ListEntries > ListShowLines 
	    && CursorPosition[player_index] >= ListStartIndex[player_index] 
	    && CursorPosition[player_index] < ListStartIndex[player_index] + ListEntries) {
		int	offset = CursorPosition[player_index] - ListStartIndex[player_index];
		if (ListEntries > ListShowLines && offset >= ListDisplayOffset + ListShowLines - 1) {
			// Scroll down.
			ListDisplayOffset = offset - (ListShowLines - 1);
			if (offset == ListDisplayOffset + ListShowLines - 1 && offset < ListEntries-1) {
				// "<more>" line needed.
				ListDisplayOffset += 1;
			}
		} else if (ListDisplayOffset > 0 && offset <= ListDisplayOffset) {
			// Scroll up.
			ListDisplayOffset = offset - 1;
			if (ListDisplayOffset < 0) ListDisplayOffset = 0;
		}
	}
	
	// Draw the UI elements.
	int	ystep = Text::GetFontHeight(Text::DEFAULT) + 6;
	int	y = (480 - ElementCount[player_index] * ystep) / 2;
	int	x = (640 - MaxElementWidth) / 2;

	if (ShowMenuBackdrop[player_index]) {
		y = 75;
		x = 358;
	}
	
	int	i;
	for (i = 0; i < ElementCount[player_index]; i++, y += ystep) {
		if (i == ListStartIndex[player_index]) {
			if (ListDisplayOffset) {
				// Show <more> on the top line of the list area.
				Text::DrawString(x, y, Text::DEFAULT, Text::ALIGN_LEFT, "<more>", 0xFF808080);
				y += ystep;
				i += ListDisplayOffset + 1;
			}
		} else if (ListStartIndex[player_index] >= 0 
			   && i == ListStartIndex[player_index] + ListDisplayOffset + ListShowLines - 1) {
			if (i < ListStartIndex[player_index] + ListEntries - 1) {
				// Show <more> on the bottom line of the list area.
				Text::DrawString(x, y, Text::DEFAULT, Text::ALIGN_LEFT, "<more>", 0xFF808080);
				i = ListStartIndex[player_index] + ListEntries - 1;
				continue;
			}
		}
		
		if (Cursor != NULL && i == CursorPosition[player_index] && Element[i][player_index].CursorStop) {
			// Draw the cursor.
			ImageOverlay::Draw(x - 2 - Cursor->GetWidth(), y - Text::GetFontBaseline(Text::DEFAULT) / 2 - Cursor->GetHeight() / 2, Cursor->GetWidth(), Cursor->GetHeight(), Cursor, 0, 0);
		}
		Text::DrawString(x-2, y+2, Text::DEFAULT, Text::ALIGN_LEFT, Element[i][player_index].Text, 0x60202020);	// Drop shadow.
		Text::DrawString(x, y, Text::DEFAULT, Text::ALIGN_LEFT, Element[i][player_index].Text, Element[i][player_index].Color);
	}
}

void	RegisterModeHandler(Mode mode, ModeHandler* handler)
// Associates the given handler with the specified mode.  The various
// methods of the handler will be called by UI at appropriate times when
// the mode is active.
// NOTE: throws an exception if an attempt is made to register a second,
// different, handler for the same mode.
{
	if (Handler[mode] == 0) {
		// Register the handler.
		Handler[mode] = handler;
	} else if (Handler[mode] != handler) {
		Error e; e << "UI::RegisterModeHandler(): attempt made to register more than one handler for mode '" << int(mode) << "'.";
		throw e;
	}
}


void	ModeHandler::MountainInit()
// Called when loading a new mountain dataset.  Typically a mode handler
// would take this opportunity to create or link to any database objects it
// needs to function.
{
}


void	ModeHandler::MountainClear()
// Called when clearing the current mountain dataset.  Typically a mode handler
// would clear any pointers it has to database objects here.
{
}


void	ModeHandler::Render(const ViewState& s)
// Called on the active mode handler by UI, to allow the mode to do
// any additional 2D rendering it wants to do.  Rendering appears behind
// the UI menu, if any.
// Default implementation does nothing.
{
}


const char*	String(const char* VarName, const char* DefaultValue)
// Look up a string in the localization table.  Return DefaultValue if
// there's nothing in the table under the specified varname.
{
	const int	BUFLEN = 1000;
	char	buf[BUFLEN];

	snprintf(buf, BUFLEN, "text.%s.ui.%s", Config::Get("Language"), VarName);

	const char*	result = Config::Get(buf);
	if (result) return result;
	else return DefaultValue;
}


};	// end namespace UI

