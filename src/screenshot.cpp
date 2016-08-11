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
// screenshot.cpp	-thatcher 7/5/1999 Copyright Slingshot Game Technology

// Code for a screenshot-capture interface.


#include "config.hpp"
#include "model.hpp"
#include "gameloop.hpp"
#include "render.hpp"
#include "psdread.hpp"
#include "game.hpp"
#include "text.hpp"


class Screenshot : public MObject {
	float	x;	//xxxxxxxx
public:
	Screenshot()
	// Constructor.  Initialize the members.
	{
	}

	const vec3&	GetLocation() const { return ZeroVector; }
	void	SetLocation(const vec3& v) {}
	
	void	Render(ViewState& s, int ClipHint)
	// Show some debug info.
	{
	}
	
	void	Update(const UpdateState& u)
	// Runs physics/behavior for the screenshot object.
	{
		if (Input::GetControlKeyState() && Input::CheckForEventDown(Input::F10)) {
			Render::WriteScreenshotFile();
		}

		if (Input::GetControlKeyState() && Input::CheckForEventDown(Input::F9)) {
			Config::Toggle("Wireframe");
		}

		x = u.Inputs.Tilt * 1000;
	}

private:
};


class DummyGModel : public GModel {
public:
	DummyGModel() { Radius = 1; }
	void	Render(ViewState& s, int ClipHint) {}
};


static struct InitScreenshot {
	InitScreenshot() {
		Game::AddInitFunction(Init);
	}

	static void	Init()
	// Create a screenshot instance and make sure it gets updated.
	{
		// Set up object.
		Screenshot* c = new Screenshot;

		c->SetVisual(new DummyGModel);
		
		// Attach to database.
		c->SetLocation(ZeroVector);
		Model::AddDynamicObject(c);
	}
} InitScreenshot;
