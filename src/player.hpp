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
// player.hpp	-thatcher 8/16/1999 Copyright Slingshot Game Technology

// Interface to player information.


#ifndef PLAYER_HPP
#define PLAYER_HPP


#include "game.hpp"


const int	PLAYERNAME_MAXLEN = 50;


class Player {
public:
	virtual	~Player() {}

	virtual void	Save() = 0;
	virtual void	CompletedRun(int run /* assume current mtn. */ /* time? */ /* run rating? */) = 0;
	virtual void	RegisterScore(int run, int score) = 0;
	
	virtual const char*	GetName() = 0;
	virtual int	GetHighestRunCompleted(const char* MountainFilename) = 0;
	virtual const char*	GetLastMountainUsed() = 0;
	// get rating...?
	// etc.

	virtual int	GetHighScore(int run) = 0;

	static bool	GetSavedPlayersAvailable();
	
private:
	static Player*	LoadPlayer(const char* name);
	friend	void Game::LoadPlayer(const char* name);
};



#endif // PLAYER_HPP
