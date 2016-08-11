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
// finish.cpp	-thatcher 11/17/1998 Copyright Slingshot

// Implementation of finish-line trigger object.


#include "main.hpp"
#include "model.hpp"
#include "clip.hpp"
#include "gameloop.hpp"
#include "game.hpp"
#include "ui.hpp"
#include "player.hpp"


class FinishLine : virtual public RunEndTrigger
{
private:
	bool	Active;
	
public:
	FinishLine()
	// Constructor.
	{
		Active = false;
	}

	void	SetActive(bool a)
	// Enables/disables the trigger.  Called by Game:: to set up a run.
	{
		Active = a;
	}
	
	void	Update(const UpdateState& u)
	// Look for the user, and see if s/he crossed the finish line.  If so, then notify
	// the Game module.
	{
		if (Active == false) return;
		
		MObject*	User = Game::GetUser();
		if (User) {
			if (UI::GetMode() == UI::PLAYING && (User->GetLocation() - GetLocation()).magnitude() < 10) {
				UI::SetMode(UI::SHOWFINISH, u.Ticks);

				// Credit the player with finishing this run.
				Game::RunCompleted(Game::GetCurrentRun());
				
//				Player*	p = Game::GetCurrentPlayer();
//				p->CompletedRun(Game::GetCurrentRun());
			}
		}
	}
};


static struct InitFinishLine {
	InitFinishLine() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	{
		Model::AddObjectLoader(3, FinishLineLoader);
	}

	static void	FinishLineLoader(FILE* fp)
	// Loads information from the given file and uses it to initialize a FinishLine object.
	{
		FinishLine*	o = new FinishLine();

		// Load the standard object information.
		o->LoadLocation(fp);

		// Load the run id.
		int	RunID = Read32(fp);
		RunID--;

		// Load orientation.
		o->LoadOrientation(fp);
		
		// Link to the database.
		Model::AddDynamicObject(o);	// Dynamic, so that Update() gets called.

		// Register with ::Game.
		Game::RegisterRunEndTrigger(o, RunID);
	}

} InitFinishLine;

