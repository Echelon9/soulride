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
// perftest.cpp	-thatcher 12/12/1998 Copyright Slingshot

// Object that follows a fixed path and measures framerate.


#include "main.hpp"
#include "text.hpp"
#include "config.hpp"
#include "model.hpp"
#include "path.hpp"
#include "game.hpp"
#include "usercamera.hpp"
#include "ui.hpp"


static void	ShowAverageFPS(float avg_fps);


class PerfTester : public MOriented {
public:
	PerfTester() : path(), PathParameter(0)
	// Constructor.
	{
		StartTicks = 0;
		StartFrame = 0;
		MaxParameter = float(path.GetPointCount() - 1);
		CalculatedStats = false;
	}

	/**
	 * Advance the PerfTester along the path.
	 *
	 * @param u State object.
	 */
	void
	Update(const UpdateState& u)
	{
		if (StartTicks == 0) {
			StartTicks = u.Ticks + 2000;
			PathParameter = 0;
		}

		if (PathParameter < MaxParameter) {
			if (PathParameter == 0 && u.Ticks > StartTicks) {
				PathParameter = 0.10f;
			}
			Path::Point	p;
			path.GetInterpolatedPoint(&p, PathParameter);
			SetLocation(p.Location);
			SetOrientation(p.Orientation);
		} else {
			if (!CalculatedStats) {
				// We've reached the end of the path.  Compute and display framerate stats.
				int	TotalTicks = u.Ticks - StartTicks;
				int	TotalFrames = GameLoop::GetFrameNumber() - StartFrame;
				float	AvgFrameRate = TotalFrames * 1000.0f / float(TotalTicks);

				ShowAverageFPS(AvgFrameRate);

				if (Config::GetBoolValue("PerfTestExit") == true) {
					Main::SetQuit(true);
				}
				
				CalculatedStats = true;
			}
		}
	}

	void	Advance()
	// Travel incrementally along the path.
	{
		if (PathParameter > 0) {
			if (StartFrame == 0) StartFrame = GameLoop::GetFrameNumber();
			PathParameter += 10.0f * 0.033f;
		}
	}


private:
	Path	path;
	float	PathParameter, MaxParameter;
	int	StartTicks, StartFrame;
	bool	CalculatedStats;
};


class PerfTestUI : public UI::ModeHandler
{
	PerfTester*	tester;
	bool	ShowFPS;
	float	AverageFPS;
public:
	PerfTestUI() {
		tester = NULL;
		ShowFPS = false;
		AverageFPS = 0;
	}

	~PerfTestUI() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	
	void	MountainInit()
	// Called when a new mountain is being loaded.
	// Create a PerfTester object and link it into the database if cvar "PerfTest" is true.
	{
		if (Config::GetBoolValue("PerfTest") == true) {
			tester = new PerfTester;
			MOriented*	pt = (MOriented*) tester;
			Model::AddDynamicObject(pt);
//			GameLoop::SetViewer(pt);

//			UserCamera::LookThrough(pt);
			UserCamera::SetSubject(pt);
			UserCamera::SetMotionMode(UserCamera::FOLLOW);
			UserCamera::SetAimMode(UserCamera::LOOK_THROUGH);
		}
		// Model::AddObjectLoader(......);
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		tester->Advance();

		if (ShowFPS) {
			// Show average FPS.
			char	buf[80];
			Text::FormatNumber(buf, AverageFPS, 3, 1);
			Text::DrawString(320, 250, Text::DEFAULT, Text::ALIGN_RIGHT, "AVG FPS ");
			Text::DrawString(320, 250, Text::DEFAULT, Text::ALIGN_LEFT, buf);
		}
	}
	
	void	Update(const UpdateState& u)
	// Check to see if we're done showing the title screen.
	{
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			Main::SetQuit(true);
		}
	}

	/**
	 * Tells this UI handler to start displaying the given FPS value as the
	 * average FPS, and print value to stdout.
	 *
	 * @param AvgFPS Pre-calculated average FPS for relevant period of time.
	 */
	void
	ShowAverageFPS(float AvgFPS)
	{
		ShowFPS = true;
		AverageFPS = AvgFPS;

		printf("Average FPS: %3.1f\n", AverageFPS);
	}
	
} PerfTestUIInstance;


void	ShowAverageFPS(float avg_fps) { PerfTestUIInstance.ShowAverageFPS(avg_fps); }


static struct InitPerfTester {
	InitPerfTester() {
		GameLoop::AddInitFunction(Init);
//		Game::AddInitFunction(AttachPerfTester);
	}

	static void	Init() {
		if (Config::GetBoolValue("PerfTest")) {
			UI::RegisterModeHandler(UI::PERFTEST, &PerfTestUIInstance);
		}
	}

} InitPerfTester;

