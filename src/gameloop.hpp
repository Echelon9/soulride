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
// gameloop.hpp	-thatcher 2/2/1998

// Runs the main game loop.


#ifndef GAMELOOP_HPP
#define GAMELOOP_HPP


#include "input.hpp"
#include "gamegui/gamegui.h"


class MOriented;


struct UpdateState {
	int	Ticks, DeltaTicks;
	float	DeltaT;
	Input::InputState	Inputs;
};


// Global pointer to GameGUI instance.  Not very OO, but convenient, and
// I'm not sure why we'd ever need more than one.
extern GameGUI*	GUI;


namespace GameLoop {
	void	Open();
	void	Close();

	void	AddInitFunction(void (*InitFct)());	// For functions to be run once, before gameloop start-up.

	bool	GetIsOpen();
	void	SetViewer(MOriented* viewer);
  void	Update();
  void SetWindowCoordinates(int number_of_players, 
			    int *window_corner_x, int *window_corner_y, 
			    int *window_size_x, int *window_size_y);
  
  int	GetFrameNumber();
	int	GetCurrentTicks();

//	bool	GetIsInIntroMode();
//	void	PlayIntroMovie(const char* MovieFile);
	void	CueMovie(const char* MovieFile);
	void	PlayShortMovie(const char* MovieFile);
	void	PlayShortMovie(GG_Movie* MovieFile);
//	void	LoadMountainInBackground(const char* MountainName, const char* MovieName);

	void	CacheMovie(const char* filename);
	GG_Player*	LoadMovie(const char* filename);
	void	GUIBegin();	// Use these calls to bracket GG_Player::play() calls.
	void	GUIEnd();	//
	bool	PointQuery(GG_Player* p, float x, float y);	// return true if the given coords are inside the movie's bounding box.
	
//	void	ShowLogo();

	bool	UpdatesPending();
	void	GetNextUpdateState(UpdateState* u /*, int Ticks */);

	float	GetSpeedScale();	// For scaling the speed of dynamic effects during recording playback.
	void	SetSpeedScale(float scale);

	void	PauseInputs();
	void	UnpauseInputs();

	// Helper to turn off the input thread within a certain scope.
	// Use this to inhibit time-consuming input thread during
	// non-interactive stuff, like loading data or whatever.
	struct AutoPauseInput
	{
		AutoPauseInput()
		{
			PauseInputs();
		}

		~AutoPauseInput()
		{
			UnpauseInputs();
		}
	};
};


#endif // GAMELOOP_HPP
