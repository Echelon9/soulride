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
// main.cpp	-thatcher 1/28/1998 Copyright Thatcher Ulrich

// Generic Main:: implementation.


#ifdef LINUX
#include <unistd.h>
#else // not LINUX
#include <direct.h>
#endif // not LINUX
#include <time.h>
#include "main.hpp"
#include "render.hpp"
#include "gameloop.hpp"
#include "input.hpp"
#include "config.hpp"
#include "utility.hpp"
#include "model.hpp"
#include "sound.hpp"
#include "terrain.hpp"
#include "surface.hpp"
#include "psdread.hpp"
#include "text.hpp"
#include "lua.hpp"


namespace Main {
;


#ifndef VERSION_STRING
#define VERSION_STRING "<unknown version -- check config.mak>"
#endif

const char*	VersionString = VERSION_STRING;


bool	Paused = false;
bool	Quit = false;


void	Open(char* CommandLine)
// Set everything up.
{
	// Randomize rand().
	srand(time(NULL));
	
	// Switch to "data" subdirectory, before trying to read game info.
	if (chdir("data") != 0) {
		Error e; e << "Couldn't change to subdirectory 'data'.\n";
		throw e;
	}
	
	// Open subsystems.
	Config::Open();

	// Set some default config variable values, now that Config:: is open.
	// Graphical options are set using the Carbon GUI version for MacOSX
#ifndef MACOSX_CARBON
	Config::SetBool("Fullscreen", true);
	Config::SetInt("OGLDriverIndex", 0);
	Config::SetInt("OGLModeIndex", 1);
#endif // MACOSX_CARBON
	Config::SetFloat("ViewAngle", 97);
	Config::SetFloat("TerrainMeshSlider", 6);
	Config::SetInt("TextureDetailSlider", 6);
	Config::SetFloat("MIPMapLODBias", 0);
	Config::SetInt("OGLCheckErrorLevel", 1);

// DefaultMountain is set using the Carbon GUI version for MacOSX
#ifndef MACOSX_CARBON
#ifdef STATIC_MOUNTAIN
	Config::Set("DefaultMountain", STATIC_MOUNTAIN);
#else
	Config::Set("DefaultMountain", "Mammoth");
#endif // STATIC_MOUNTAIN
#endif // MACOSX_CARBON

	Config::Set("Language", "en");
	Config::SetBool("DetailMapping", true);
	Config::SetBool("Fog", true);
	Config::SetBool("MIPMapping", true);
	Config::SetBool("KeyRepeat", true);
	Config::SetBool("Wireframe", false);
	Config::SetBool("ShowFrameRate", false);
	Config::SetBool("ShowRenderStats", false);
	Config::SetInt("SurfaceNodeBuildLimit", 4);

	Config::SetBool("DirectInput", true);
	Config::SetBool("JoystickRudder", true);

//	Config::SetValue("JoystickDevice", "/dev/js0");	// Linux default joystick input device.
//	Config::SetValue("CDAudioDevice", "/dev/cdrom");	// Linux default audio device.
	Config::SetValue("CDAudioMountPoint", "/mnt/cdrom");	// Path to read "cdaindex.txt" from.
	
	Config::SetBool("MouseSteering", true);
	Config::SetFloat("MouseSteeringSensitivity", 5);

	Config::SetBool("Sound", true);
	Config::SetBool("Music", true);
	
	Config::SetFloat("FogDistance", 25000);
	Config::SetFloat("SnowDensity", 0.6f);
	Config::SetInt("Clouds", 1);
	Config::SetFloat("Cloud0UVRepeat", 3);
	Config::SetFloat("Cloud0XSpeed", 0);
	Config::SetFloat("Cloud0ZSpeed", 0);
	Config::SetFloat("Cloud1UVRepeat", 4);
	Config::SetFloat("Cloud1XSpeed", 0);
	Config::SetFloat("Cloud1ZSpeed", 0);

	Config::SetBool("Particles", true);

	Config::SetFloat("SFXVolume", 80);
	Config::SetFloat("CDAudioVolume", 70);
	
	Config::SetBool("TerrainShadow", false);
	Config::SetFloat("SunTheta", 243);
	Config::SetFloat("SunPhi", 81);
	Config::SetFloat("DiffuseLightingFactor", 1.2f);
	Config::SetFloat("ShadowLightingFactor", 0.5f);
	Config::SetBool("BoarderShadow", 1);
	Config::SetBool("Speedometer", 0);
	Config::SetInt("PlayerCameraMode", 0);

	Config::SetFloat("HelicamCenterX", 32767);
	Config::SetFloat("HelicamCenterZ", 32767);
	Config::SetFloat("HelicamDefaultAngle", -75);

	Config::SetBool("RecordMoviePause", true);
	
	// Read global preload script, if any.
	lua_dofile("preload.lua");

	// Read values from config.txt.
	FILE*	fp = fopen(".." PATH_SEPARATOR "config.txt", "r");
	if (fp) {
		char	line[1000];
		for (;;) {
			char*	res = fgets(line, 1000, fp);
			if (res == NULL) {
				break;
			}
			Config::ProcessOption(line);
		}
		fclose(fp);
	}
	
	// Run start-up script.
	lua_dofile(".." PATH_SEPARATOR "startup.lua");
	//Script::Load(".." PATH_SEPARATOR "startup.srs");
	
	// Read user-specified variable settings from the command-line.
	Config::ProcessCommandLine(CommandLine);

	// For backwards-compatibility...
	if (Config::GetBoolValue("DisableDetailMapping")) Config::SetBoolValue("DetailMapping", false);

	Utility::Open();
	Render::Open();
}


void	Close()
// Shut down subsystems.
{
	// Switch back to original directory.
	chdir("..");
	
	// Close subsystems.
	Render::Close();
	Utility::Close();
	Config::Close();
}


const char*	GetVersionString()
// Return the app version string.
{
	return VersionString;
}


bool	GetQuit()
// Returns true if the quit flag is set.
{
	return Quit;
}


void	SetQuit(bool q)
// Call with true to quit the program.
{
	Quit = q;
}


bool	GetPaused()
// Returns true if the app is paused.
{
	return Paused;
}


void	SetPaused(bool NewPausedState)
// Call with true to pause the app, false to un-pause it.
{
	if (NewPausedState != Paused) {
		Paused = NewPausedState;
		// Make sure the user can see the windows menus, etc.
		if (Paused) Render::MakeUIVisible();
	}
}


};
