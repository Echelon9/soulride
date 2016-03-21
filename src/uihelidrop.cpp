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
// UIHeliDrop.cpp	-thatcher 1/6/1999 Copyright Slingshot

// Code for showing the user an aerial view of the area and letting them pick
// their own starting spot using the mouse.


#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#include "config.hpp"
#include "gameloop.hpp"
#include "ui.hpp"
#include "psdread.hpp"
#include "render.hpp"
#include "sound.hpp"
#include "game.hpp"
#include "usercamera.hpp"
#include "terrain.hpp"
#include "utility.hpp"
#include "imageoverlay.hpp"
#include "attractmode.hpp"


//
// utility functions for mouse interface
//


static void	PlaceUser(const vec3& DropPoint, const vec3& CameraLocation)
// Places the user at the specified point on the terrain.  Samples the
// terrain to determine the fall line, and points the user downhill.
{
	vec3	norm(TerrainModel::GetHeight(DropPoint - vec3(0.5, 0, 0)) - TerrainModel::GetHeight(DropPoint + vec3(0.5, 0, 0)),
		     1,
		     TerrainModel::GetHeight(DropPoint - vec3(0, 0, 0.5)) - TerrainModel::GetHeight(DropPoint + vec3(0, 0, 0.5)));
	norm.normalize();

	vec3	dir;
	
	// If the slope is really shallow, then point the user towards the map view camera.
	if (norm.Y() > 0.999) {
		dir = (CameraLocation - DropPoint);
	} else {
		// Aim down the fall line.
		dir = YAxis.cross(norm.cross(YAxis));
	}

	dir.SetY(0);
	dir.normalize();

	MObject*	user = Game::GetUser();
	vec3	vel = dir * 2 + YAxis * 1;
	if (user) user->Reset(DropPoint + vec3(0, 1, 0), dir, vel);

	Game::SetHeliDropInfo(DropPoint + vec3(0, 1, 0), dir, vel);//xxxxx
}


//
// object to act as the camera in map-view/heli-drop mode.
//


class HeliCam : public MOriented {

	bool	Flying;
	vec3	TargetPoint;
	vec3	TargetDir;
	
public:
	HeliCam()
	{
		Flying = false;
		TargetPoint = ZeroVector;
		TargetDir = XAxis;
	}
	
	~HeliCam() {}

	void	Update(const UpdateState& u)
	// Fly towards the target point if Flying is enabled; otherwise just stay put.
	{
		if (Flying) {
			vec3	disp = TargetPoint - GetLocation();
			float	dist = disp.magnitude();

			// Limit speed.
			if (dist > 300.0f * u.DeltaT) {
				disp *= 300.0f * u.DeltaT / dist;
			}

			SetLocation(GetLocation() + disp);

			if (dist > 10.0) {
				disp.normalize();
				SetDirection(disp);
			} else {
				SetDirection(TargetDir);
			}
			vec3	up = (GetDirection().cross(YAxis)).cross(GetDirection());
			up.normalize();
			SetUp(up);
		}
	}

	void	SetFlying(bool fly)
	// Turns flying mode on or off.
	{
		Flying = fly;
	}

	void	SetTarget(const vec3& target)
	// Defines the target point to fly towards.
	{
		TargetPoint = target;
	}

	bool	GetReachedTarget()
	// Returns true if we're at the target point.
	{
		vec3	disp = TargetPoint - GetLocation();
		if (disp.magnitude() < 10.0) return true;
		else return false;
	}
};


//
// UI mode handler
//


class HeliDrop : public UI::ModeHandler {

	float	CursorX, CursorY;
	vec3	CursorRay;
	bool	DropValid;
	Render::Texture*	Cursor;
	Render::Texture*	DropMarker;
	HeliCam*	Camera;
	vec3	DropPoint;
	bool	FlyingMode;

	vec3	CamCenter;
	float	CamYOffsetMax;
	float	CamYOffsetMin;
	float	CamRadius;
	float	CamRadiusMax, CamRadiusMin;
	float	CamAngle;
	
public:
	HeliDrop() {
		CursorX = CursorY = 0;
		DropValid = false;
		Cursor = NULL;
		Camera = NULL;
		DropPoint = ZeroVector;
		FlyingMode = false;

//		CamCenter = vec3(-352, 850, 1804);
		CamCenter = vec3(0, 850, 0);
		CamYOffsetMax = 1800.0;
		CamYOffsetMin = 600.0;
		CamRadius = 2000;
		CamRadiusMax = 5000;
		CamRadiusMin = 1500;
		CamAngle = -75 * PI / 180.0f;
	}

	~HeliDrop() {
		ClearTextures();
	}


	void	ClearTextures()
	// Cleanup function.
	{
		if (Cursor) {
			delete Cursor;
			Cursor = NULL;
		}
		if (DropMarker) {
			delete DropMarker;
			DropMarker = NULL;
		}
	}


	void	MountainInit()
	// Called when loading a new mountain.
	{
		// Create the camera object.
		if (Camera == NULL) {
			Camera = new HeliCam;
			Model::AddDynamicObject(Camera);
		}

		// Set up the default camera angle.
		CamAngle = Config::GetFloat("HelicamDefaultAngle") * PI / 180.0f;
	}


	void	MountainClear()
	// Clear our camera pointer, since the object will be (or has already been) freed.
	{
		Camera = NULL;

		// Drop our textures, since we might be exiting the program.
		ClearTextures();
	}
	
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		if (Cursor == NULL) {
			Cursor = Render::NewTexture("mapcursor.psd", true, false, false);
		}

		if (DropMarker == NULL) {
			DropMarker = Render::NewTexture("dropmarker.psd", true, false, false);
		}

		CamCenter.SetX(Config::GetFloat("HelicamCenterX") - 32767.0f);
		CamCenter.SetZ(Config::GetFloat("HelicamCenterZ") - 32767.0f);
		CamCenter.SetY(TerrainModel::GetHeight(CamCenter));
		
		// Make sure attract-mode is turned off.
		AttractMode::Exit();

		// Reset run stats.
		Game::SetTimer(0);
		Game::SetScore(0);
		Game::SetTimerActive(false);
		
		// Reset the rewind count.
		Config::SetInt("RewindsAllowed", 3);

//		ResetRun(GetCurrentRun());
		
		// Make sure the user is disabled.
		Game::SetUserActive(false);

		// Set the location and orientation of the camera.
		SetCameraLocation();

		UserCamera::SetSubject(Camera);
		UserCamera::SetMotionMode(UserCamera::FOLLOW);
		UserCamera::SetAimMode(UserCamera::LOOK_THROUGH);
		
		// Use the current user location to initialize the drop marker.
		MObject*	user = Game::GetUser();
		if (user) DropPoint = user->GetLocation();
		DropValid = true;

		FlyingMode = false;
		Camera->SetFlying(false);
	}


	void	SetCameraLocation()
	// Set the camera location and orientation according to the current view parameters.
	{
		vec3	v;
		v.SetX(CamCenter.X() + CamRadius * cosf(CamAngle));
		v.SetY(CamCenter.Y() + CamYOffsetMin + (CamYOffsetMax - CamYOffsetMin) * (CamRadius - CamRadiusMin) / (CamRadiusMax - CamRadiusMin));
		v.SetZ(CamCenter.Z() + CamRadius * sinf(CamAngle));

		// Ensure that the camera is always at least 50m above the ground surface.
		float	Height = TerrainModel::GetHeight(v);
		if (v.Y() < Height + 50.0f) v.SetY(Height + 50.0f);
		
		Camera->SetLocation(v);

		vec3	dir = CamCenter - v;
		dir.normalize();
		Camera->SetDirection(dir);
		
		vec3	up = dir.cross(YAxis).cross(dir);
		up.normalize();
		Camera->SetUp(up);
	}
	
	void	Close()
	// Called by UI when exiting mode.
	{
//		UserCamera::LookAt(Game::GetUser());
//		UserCamera::SetSubject(Game::GetUser());
	}

	void	Update(const UpdateState& u)
	// Move the mouse.
	{
		if (FlyingMode) {
			// Wait til the camera has reached its target.
			if (Camera->GetReachedTarget()) {
				// Start boarding...
				UI::SetMode(UI::PLAYING, u.Ticks);
			}
		} else {
			// Move the heli view according to the directional control inputs.
			CamRadius = fclamp(CamRadiusMin, CamRadius - u.Inputs.Pitch * 650.0f * u.DeltaT, CamRadiusMax);
			CamAngle += u.Inputs.Tilt * -0.35f * u.DeltaT;
			while (CamAngle < 0) CamAngle += 2 * PI;
			while (CamAngle > 2 * PI) CamAngle -= 2 * PI;
			SetCameraLocation();
			
			// Check mouse input, for placing the cursor and drop point.
			float	x, y;
			int	buttons;
			Input::GetMouseStatus(&x, &y, &buttons);
			
			CursorX = x;
			CursorY = y;
			
			// If user left-clicks, then locate the intersection of the cursor with the terrain.
			if (buttons & 1) {
				bool	Found = TerrainModel::FindIntersectionCoarse(&DropPoint, Camera->GetLocation(), CursorRay);
				if (Found == false) {
					// Low beep.
				} else {
					// Hi beep.
				}
			}
		}
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		if (FlyingMode == false) {
			// Show the drop point, if we've got one.
			if (DropValid) {
				vec3	v;
				View::Project(&v, s, DropPoint);
				v.SetX(v.X() / Render::GetWindowWidth() * 640.0f);
				v.SetY(v.Y() / Render::GetWindowHeight() * 480.0f);
				ImageOverlay::Draw(
					(int) (v.X() - DropMarker->GetWidth() / 2),
					(int) (v.Y() - DropMarker->GetHeight() / 2),
					DropMarker->GetWidth(),
					DropMarker->GetHeight(),
					DropMarker, 0, 0);
			}
			
			// Show the cursor.
			ImageOverlay::Draw((int) (CursorX), (int) (CursorY), Cursor->GetWidth(), Cursor->GetHeight(), Cursor, 0, 0);
			View::Unproject(
				&CursorRay,
				s,
				CursorX / 640.0f * Render::GetWindowWidth(),
				CursorY / 480.0f * Render::GetWindowHeight());
		}
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			UI::SetMode(UI::MAINMENU, Ticks);
			return;
		}

		if (code == UI::ENTER && FlyingMode == false) {
			if (DropValid) {
				PlaceUser(DropPoint, Camera->GetLocation());
				Camera->SetTarget(DropPoint);
				Camera->SetFlying(true);
//				FlyingMode = true;

				Game::SetCurrentRun(-1);
				UI::SetMode(UI::RUNFLYOVER, Ticks);
			}
		}
	}
	
} Instance;


static struct InitHeliDrop {
	InitHeliDrop() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	// Attach the title screen handler to the UI module.
	{
		UI::RegisterModeHandler(UI::HELIDROP, &Instance);
	}
	
} InitHeliDrop;

