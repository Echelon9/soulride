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
// camera.cpp	-thatcher 2/7/1998 Copyright Thatcher Ulrich

// Code for a camera object.


#include "utility.hpp"
#include "model.hpp"
#include "main.hpp"
#include "gameloop.hpp"
#include "input.hpp"
#include "model.hpp"
#include "config.hpp"
#include "usercamera.hpp"
#include "ui.hpp"
#include "text.hpp"
#include "game.hpp"
#include "imageoverlay.hpp"
#include "attractmode.hpp"
#include "terrain.hpp"
#include "console.hpp"
#include "weather.hpp"


class Camera : public MOriented {
private:
	unsigned int	LastTicks;
	float	LastMouseX, LastMouseY;
	Render::Texture*	Cursor;
	int	WarpFactor;

	vec3	CursorRay;

	uint32	CursorColor;
	
	// State for capturing a set of Optikos images.
	int	OptikosMode;	// 0 == normal, 1 == rotate, 2 == translate
	int	ScreenshotNumber;
	vec3	Focus;
	vec3	Dir;
	float	Distance;
	quaternion	OriginalOrientation;
	vec3	OriginalLocation;

	//xxxxx
	vec3	ProbeOrg, ProbeEnd;
	
public:
	Camera()
	// Constructor.  Initialize the members.
	{
		LastTicks = 0;
		LastMouseX = LastMouseY = 0;
		Cursor = NULL;
		WarpFactor = 4;

		OptikosMode = 0;
	}

	~Camera()
	{
		if (Cursor) delete Cursor;
	}

	void	Render(ViewState& s, int ClipHint)
	// Show some debug info.
	{
		View::Unproject(
			&CursorRay,
			s,
			LastMouseX / 640.0f * Render::GetWindowWidth(),
			LastMouseY / 480.0f * Render::GetWindowHeight());
		CursorRay.normalize();

		// Forward to the base-class.
		MOriented::Render(s, ClipHint);


		// Special Optikos screenshot-sequence capture.
		if (OptikosMode) {
			Console::Printf("OptikosMode = %d\n", OptikosMode);//xxxxxx

			if (Config::GetValue("SaveFramePPM") == NULL) {
				// Write a screenshot.
				if (OptikosMode == 1 && ScreenshotNumber > 0) {
					char	fn[200];
					sprintf(fn, "%s" PATH_SEPARATOR "r%02d.ppm", Config::GetValue("OptikosPath"), ScreenshotNumber-1);
					
//					Render::WriteScreenshotFilePPM(fn);
					Config::SetValue("SaveFramePPM", fn);
				}
				
				// Move the camera.
				float	angle = PI/180 * 30.0f * ((ScreenshotNumber - 0.5f) / 30.0f - 0.5f);
				SetOptikosViewpoint(angle);

				Console::Printf("%d, angle = %g\n", ScreenshotNumber, angle);//xxxx
				
				ScreenshotNumber++;
				if (ScreenshotNumber >= 31) {
					OptikosMode = 0;
					SetOrientation(OriginalOrientation);
					SetLocation(OriginalLocation);
				}
			}
			
			return;	// don't draw cursor.
		}
		
		
		// Show a cursor.
		if (UI::GetMode() == UI::CAMERA) {
			float	mx, my;
			int	mb;
			Input::GetMouseStatus(&mx, &my, &mb);
			if (Cursor == NULL) {
				// Make a cursor.
				bitmap32*	b = new bitmap32(16, 16);
				int	j, i;
				uint32*	p = b->GetData();
				for (j = 0; j < 16; j++) {
					for (i = 0; i < 16; i++) {
						if (i == j || i == 15-j) {
							*p = 0xFFFFFFFF;
						} else if (i == j+1 || i == j-1 || i == 14-j || i == 16-j) {
							*p = 0xFF000000;
						} else {
							*p = 0x00000000;
						}
						p++;
					}
				}
				
				Cursor = Render::NewTextureFromBitmap(b, true, false, false);
				
				delete b;
			}
			ImageOverlay::Draw(
				(int) (mx - Cursor->GetWidth()/2),
				(int) (my - Cursor->GetHeight()/2),
				Cursor->GetWidth(), Cursor->GetHeight(), Cursor, 0, 0, CursorColor);

#if 0
			//xxxx
			// Show probe ray.
			Render::SetTexture(NULL);
			Render::CommitRenderState();
			glColor3f(1, 0, 0);
			glBegin(GL_LINES);
			glVertex3fv(ProbeOrg);
			glVertex3fv(ProbeEnd);
			glEnd();
			//xxxx
#endif // 0
		}

	}
	
	void	Update(const UpdateState& u)
	// Runs physics/behavior for the Camera.
	{
		// Switch into/out-of camera mode on F12.
		if (Input::GetControlKeyState() && Input::CheckForEventDown(Input::F12)) {
			if (UI::GetMode() == UI::CAMERA) {
				// Switch out of camera mode.
				UI::SetMode(UI::MAINMENU, u.Ticks);
			} else {
				// Switch into camera mode.
				UI::SetMode(UI::CAMERA, u.Ticks);
			}
		}
		
		// If we're in camera mode, check user input for number keys, to change warp factor.
		if (UI::GetMode() == UI::CAMERA) {
			char	buf[80];
			Input::GetAlphaInput(buf, 80);
			char*	p = buf;
			while (*p) {
				int	c = *p - '0';
				if (c >= 1 && c <= 9) {
					WarpFactor = c;
				}
				p++;
			}
		
			// Get control inputs.
			float	pitch = u.Inputs.Pitch;
			float	yaw = u.Inputs.Tilt;
			float	roll = u.Inputs.Yaw;
			float	throttle = u.Inputs.Throttle;
			
			int mode = (u.Inputs.Button[0].State ? 1 : 0) |
				   (u.Inputs.Button[1].State ? 2 : 0);
			
			// Add in mouse inputs.
			float	mx, my;
			int	mb;
			Input::GetMouseStatus(&mx, &my, &mb);
			float	dmx, dmy;
			dmx = mx - LastMouseX;
			dmy = my - LastMouseY;
			LastMouseX = mx;
			LastMouseY = my;
			
			if (mb) {
				if (mode == 1 || mode == 3) dmy = -dmy;
				
				pitch += dmy / u.DeltaT / 200.0f;
				yaw += dmx / u.DeltaT / 200.0f;
			}
			
			// Move the camera based on inputs.
			vec3	Right = GetRight();
			
			float	SlideSpeed = expf((WarpFactor - 4.0f) / 1.0f) * 100.0f;
			if (u.Inputs.Button[2].State) SlideSpeed *= 5.0f;
			
			switch (mode) {
			default:
			case 0: // Pitch on pitch; yaw on yaw, roll on roll.
				SetDirection((GetDirection() + Right * yaw * u.DeltaT * 1.2f + GetUp() * -pitch * u.DeltaT * 1.2f).normalize());
				Right -= GetUp() * roll * u.DeltaT * 1.2f;
				SetUp((Right.cross(GetDirection())).normalize());
				break;
				
			case 1:	// Translate forward/back on pitch input, and yaw on yaw input.
				SetLocation(GetLocation() + GetDirection() * pitch * u.DeltaT * SlideSpeed);
				SetDirection((GetDirection() + Right * yaw * u.DeltaT * 1.2f).normalize());
				Right -= GetUp() * roll * u.DeltaT * 1.2f;
				SetUp((Right.cross(GetDirection())).normalize());
				break;
				
			case 2:	// Roll on yaw input.  Pitch on pitch.
				SetDirection((GetDirection() + GetUp() * -pitch * u.DeltaT * 1.2f).normalize());
				Right -= GetUp() * yaw * u.DeltaT * 1.2f;
				SetUp((Right.cross(GetDirection())).normalize());
				break;
				
			case 3:	// Slide on pitch & yaw.
				SetLocation(GetLocation() + (GetUp() * pitch + Right * yaw) * u.DeltaT * SlideSpeed);
				break;
			}
			
			//
			// On F1/F2, capture a sequence of PPM images for use in making 3D Optikos displays.
			//
			if (OptikosMode == 0 && Config::GetValue("OptikosPath") != NULL) {
				if (Input::GetControlKeyState() &&
				    (Input::CheckForEventDown(Input::F1) || Input::CheckForEventDown(Input::F2)))
				{
					OptikosMode = 1;
					if (Input::CheckForEventDown(Input::F2)) OptikosMode = 2;	// dry run.
					ScreenshotNumber = 0;
					
					bool	Found = TerrainModel::FindIntersectionCoarse(&Focus, GetLocation(), CursorRay);
					if (!Found) Focus = GetLocation() + GetDirection() * 0.5;
					
					vec3	up = GetDirection().cross(YAxis).cross(GetDirection());
					up.normalize();
					
					vec3	disp = (Focus - GetLocation());
					disp -= up * (disp * up);
					Focus = GetLocation() + disp;
					Distance = disp.magnitude();
					
					Dir = CursorRay;
					Dir -= up * (Dir * up);
					Dir.normalize();
					
					OriginalOrientation = GetOrientation();
					OriginalLocation = GetLocation();
				}
			}
			
//			if (Input::GetControlKeyState() && u.Inputs.Button[Input::F3].State) {
				// Shoot ray from camera to terrain and see if it hits.
			float	dist = TerrainMesh::CheckForRayHit(GetLocation(), /*CursorRay*/ -Weather::GetSunDirection());
			Config::SetFloat("RayDistXX", dist);//xxxx
			if (dist >= 0) {
				CursorColor = 0xFF00FF00;
			} else {
				CursorColor = 0xFFFF0000;
			}
			//xxxxxxxx
			if (Input::GetControlKeyState() && u.Inputs.Button[Input::F3].State) {
				ProbeOrg = GetLocation();
				ProbeEnd = ProbeOrg + CursorRay * dist;
				ProbeEnd = ProbeOrg + -Weather::GetSunDirection() * 5000;
			}
//			} else {
//				CursorColor = 0xFFFFFFFF;
//			}
		}
	}

	void	SetOptikosViewpoint(float angle)
	// Points the camera at the focus, using Distance, Dir, and angle to determine
	// parameters.
	{
		vec3	up = Dir.cross(YAxis).cross(Dir);
		up.normalize();
		
		vec3	dir = Geometry::Rotate(angle, up, Dir);

		SetLocation(Focus - dir * Distance);
		SetDirection(dir);
		SetUp(up);
	}

};


class DummyVisual : public GModel {
public:
	DummyVisual() {
		Radius = 100;
	}
	void	Render(ViewState& s, int ClipHint) {
	}
};


class CameraUI : public UI::ModeHandler {
	Camera*	CameraInstance;
public:
	CameraUI() {
		CameraInstance = NULL;
	}

	~CameraUI() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		AttractMode::Exit();
		
		if (CameraInstance) {
			UserCamera::SetSubject(CameraInstance);
			UserCamera::SetMotionMode(UserCamera::FOLLOW);
			UserCamera::SetAimMode(UserCamera::LOOK_THROUGH);
		}
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	MountainInit()
	// Create a camera instance and make it the viewer.
	{
		// Set up camera object.
		CameraInstance = new Camera;
		Camera*	c = CameraInstance;
		// Attach to database xxxxx
		
//		c->SetLocation(vec3(0, 1500, 0));//xxxxxxxx
		c->SetLocation(vec3(281, 896, -3028));//xxxxxxxx
		c->SetDirection(vec3(-1, 0, 1).normalize() /* ZAxis */);
		c->SetUp(YAxis);
		c->SetVisual(new DummyVisual);//xxxxxxx
		
		Model::AddDynamicObject(c);
	}

	void	MountainClear()
	{
		CameraInstance = NULL;
	}
	
	void	Update(const UpdateState& u)
	// Nothing.
	{
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			Main::SetQuit(true);
		}
	}
} CameraUIInstance;


static struct InitCamera {
	InitCamera() {
		GameLoop::AddInitFunction(Init);
//		Game::AddInitFunction(AttachCamera);	// Called after loading a new mountain.
	}

	static void	Init()
	{
		// Attach mode handler.
		UI::RegisterModeHandler(UI::CAMERA, &CameraUIInstance);
	}

} InitCamera;
