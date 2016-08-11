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
// game.hpp	-thatcher 11/15/1998 Copyright Slingshot

// Interface to game stuff.


#ifndef GAME_HPP
#define GAME_HPP


#include "view.hpp"
#include "gameloop.hpp"
#include "model.hpp"
#include "sound.hpp"


class Player;


class RunStarter : virtual public MOriented {
public:
	virtual void	ResetUser(MObject* User) = 0;
	virtual const char*	GetRunName() = 0;
	virtual const char*	GetRunInfoText() = 0;
	virtual int	GetRunParTicks() = 0;
	virtual void	PostLoad(int runIndex) = 0;
};


class RunEndTrigger : virtual public MOriented {
public:
	virtual void	SetActive(bool active) = 0;
};


namespace Game {
	void	Open();
	void	Close();
	void	Clear();

	void	Update(const UpdateState& u);

	void	ShowRewinds();
	void	ShowScore();
	
	void	AddInitFunction(void (*InitFct)());	// For functions to be run at the beginning of the mountain-loading process.
	void	AddPostLoadFunction(void (*PostFct)());	// For functions to be run at the end of mountain loading.
	void	AddClearFunction(void (*ClearFct)());	// For functions to be run when the mountain dataset is being cleared.

	// Player info.
	void	LoadPlayer(const char* PlayerFile);
	Player*	GetCurrentPlayer();
	
	// Mountains.
	void	LoadMountain(const char* filename);
	void	ClearMountain();
	const char*	GetCurrentMountain();

	void	LoadingMessage(const char* message);
	void	LoadingTick(bool ForceUpdate = false);	// Nominally 100ms in the loading movie.
	
	// Runs.
	void	SetCurrentRun(int run);
	int	GetCurrentRun();
	void	SetActiveRun(int run);
	int	GetActiveRun();
	int	GetRunCount();
	const char*	GetRunName(int run);
	const char*	GetRunInfoText(int run);
	int	GetRunParTicks(int run);
	float	GetRunVerticalDrop(int run);
	
	void	ResetRun(int index);

	void	SetHeliDropInfo(const vec3& loc, const vec3& dir, const vec3& vel);
	
	// mountain
	// GetMountainName()...

	// For course timing.
	void	SetTimer(int ticks);
	int	GetTimer();
	void	SetTimerActive(bool NewActive);

	// For scoring.
	void	SetScore(int points);
	void	AddScore(int delta);
	int	GetScore();
	int	GetTotalScore();	// Include flying points not yet awarded.

	void	AddBonusMessage(const char* msg, int points, const char* soundname, const Sound::Controls& soundparams);
	
	
	// User object.
	void	RegisterUser(MDynamic* user);
	MDynamic*	GetUser();
	void	SetUserActive(bool active);

	// Run beginnings and endings.
	void	RegisterRunEndTrigger(RunEndTrigger* t, int RunIndex);
	void	RegisterRunStarter(RunStarter* s, int RunIndex);

	// Player view.
	enum CameraMode {
		CAM_THIRD_PERSON,
		CAM_FIRST_PERSON,

		CAMERA_MODES
	};

	void	SetPlayerCameraMode(CameraMode m);
	CameraMode	GetPlayerCameraMode();

	void	SetChaseCameraParams();

	void	RunCompleted(int RunID);	// Credit current player with a run completion.
};


#endif // GAME_HPP

