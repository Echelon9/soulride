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
// UIOptions.cpp	-thatcher 2/16/2000 Copyright Slingshot Game Technology

// Various options screens.


#include <stdio.h>
#include <string.h>
#ifdef LINUX
#else // not LINUX
#include <io.h>
#endif // not LINUX
#include "config.hpp"
#include "ui.hpp"
#include "main.hpp"
#include "text.hpp"
#include "game.hpp"
#include "utility.hpp"
//#include "script.hpp"
#include "lua.hpp"
//#include "logo.hpp"
#include "player.hpp"
#include "sound.hpp"
#include "music.hpp"
#include "ogl.hpp"


// UI::OPTIONS handler
class Options : public UI::ModeHandler {
private:
public:
	Options() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		UI::SetMenuTitle(UI::String("options_menu", "OPTIONS"));
		
		UI::SetElement(2, UI::ElementData(UI::String("display", "Display"), true));
		UI::SetElement(3, UI::ElementData(UI::String("sound", "Sound"), true));
//		UI::SetElement(4, UI::ElementData(UI::String("controls", "Controls"), true));
		UI::SetElement(4, UI::ElementData(UI::String("weather", "Weather"), true));
		
		UI::SetCursor(2);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
//		// Dim the background.
//		Render::FullscreenOverlay(UI::BackgroundDim);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// Return to main menu on ESCAPE.
			UI::SetMode(UI::MAINMENU, Ticks);
			
		} else if (ElementIndex == 2) {
			UI::SetMode(UI::DISPLAYOPTIONS, Ticks);
			
		} else if (ElementIndex == 3) {
			UI::SetMode(UI::SOUNDOPTIONS, Ticks);
			
//		} else if (ElementIndex == 4) {
//			UI::SetMode(UI::CONTROLOPTIONS, Ticks);
			
		} else if (ElementIndex == 4) {
			UI::SetMode(UI::WEATHEROPTIONS, Ticks);
			
		}
	}

} OptionsInstance;


// UI::DISPLAYOPTIONS handler
class DisplayOptions : public UI::ModeHandler {
private:
public:
	DisplayOptions() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		ShowOptions();
		
		UI::SetMenuTitle(UI::String("display_menu_caption", "DISPLAY"));

		UI::SetCursor(2);

		Music::FadeDown();
	}

	void	ShowOptions()
	// Draw the menu with the current options displayed.
	{
		ShowValue("TerrainMeshSlider", UI::String("terrain_mesh_detail", "Terrain mesh detail: "), 2);
		ShowValue("TextureDetailSlider", UI::String("texture_resolution", "Texture resolution: "), 3);
		ShowValue("SurfaceNodeBuildLimit", UI::String("texture_build_limit", "Texture build limit: "), 4);
		ShowValue("DetailMapping", UI::String("detail_texturing", "Detail texturing: "), 5);
		ShowValue("Clouds", UI::String("clouds", "Clouds: "), 6);
		ShowValue("Fog", UI::String("distance_haze", "Distance haze: "), 7);
		ShowValue("ShowFrameRate", UI::String("display_frame_rate", "Display frame rate: "), 8);
	}

	void	ShowValue(const char* var, const char* label, int ElementIndex)
	// Displays the name and value of a Config:: variable on the specified UI element.
	{
		char	buf[80];
		
		strcpy(buf, label);
		const char*	value = Config::GetValue(var);
		if (value) strcat(buf, value);
		else strcat(buf, "<null>");
		UI::SetElement(ElementIndex, UI::ElementData(buf, true));
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		Config::ExportValue("TerrainMeshSlider");
		Config::ExportValue("TextureDetailSlider");
		Config::ExportValue("SurfaceNodeBuildLimit");
		Config::ExportValue("DetailMapping");
		Config::ExportValue("Clouds");
		Config::ExportValue("Fog");
		Config::ExportValue("ShowFrameRate");
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
//		// Dim the background.
//		Render::FullscreenOverlay(UI::BackgroundDim);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// Return to parent Options menu on ESCAPE.
			UI::SetMode(UI::OPTIONS, Ticks);
		} else if (ElementIndex == 2) {
			// Adjust the terrain mesh slider.
			float	f = Config::GetFloatValue("TerrainMeshSlider");
			if (code == UI::LEFT) {
				f -= 1;
			} else {
				f += 1;
				if (code == UI::ENTER && f > 10) {
					// Wrap around.
					f = 1;
				}
			}
			Config::SetFloatValue("TerrainMeshSlider", fclamp(1, f, 10));
			
			// Redraw menu.
			ShowOptions();
		} else if (ElementIndex == 3) {
			// Adjust the terrain mesh slider.
			int	i = Config::GetIntValue("TextureDetailSlider");
			if (code == UI::LEFT) {
				i -= 1;
			} else {
				i += 1;
				if (code == UI::ENTER && i > 10) {
					// Wrap around.
					i = 1;
				}
			}
			Config::SetIntValue("TextureDetailSlider", iclamp(1, i, 10));
			
			// Redraw menu.
			ShowOptions();
		} else if (ElementIndex == 4) {
			// Adjust the build count limit.
			int	i = Config::GetInt("SurfaceNodeBuildLimit");
			if (code == UI::LEFT) {
				i -= 1;
			} else {
				i += 1;
			}
			Config::SetInt("SurfaceNodeBuildLimit", iclamp(3, i, 16));

			// Redraw.
			ShowOptions();
		} else if (ElementIndex == 5) {
			// Toggle Detail texturing.
			Config::Toggle("DetailMapping");

			// Redraw menu.
			ShowOptions();
		} else if (ElementIndex == 6) {
			// Clouds.
			Config::Toggle("Clouds");

			// Redraw menu.
			ShowOptions();
		} else if (ElementIndex == 7) {
			// Fog.
			Config::Toggle("Fog");

			// Redraw menu.
			ShowOptions();
		} else if (ElementIndex == 8) {
			// Frame rate display.
			Config::Toggle("ShowFrameRate");

			// Redraw menu.
			ShowOptions();
#if 0
		} else if (ElementIndex == 9) {
			// MIP-map LOD bias.
			float	f = Config::GetFloatValue("MIPMapLODBias");
			if (code == UI::LEFT) {
				f -= 0.05;
			} else {
				f += 0.05;
			}

			// Set the modified bias.
			if (OGL::GetLODBiasEnabled()) {
				Config::SetFloatValue("MIPMapLODBias", f);
//				glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, f);
//				glTexParameterf(GL_TEXTURE_2D, , );
			}

			// Refresh menu.
			ShowOptions();
#endif // 0
		}
	}

} DisplayOptionsInstance;


// UI::SOUNDOPTIONS handler
class SoundOptions : public UI::ModeHandler {
private:
public:
	SoundOptions() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		ShowOptions();
		
		UI::SetMenuTitle(UI::String("sound_menu", "SOUND"));

		UI::SetCursor(2);

		Music::FadeUp();	// Fade up to the true CD-Audio volume during adjustment.
	}

	void	ShowOptions()
	// Draw the menu with the current options displayed.
	{
//		UI::SetElement(0, UI::ElementData("     Options    ", false));

		ShowValue("MasterVolume", UI::String("master_volume", "Master Volume: "), 2);
		ShowValue("SFXVolume", UI::String("sfx_volume", "SFX Volume: "), 3);
		ShowValue("CDAudioVolume", UI::String("cd_audio_volume", "CD-Audio Volume: "), 4);

		// Re-shuffle CD audio tracks...
	}

	void	ShowValue(const char* var, const char* label, int ElementIndex)
	// Displays the name and value of a Config:: variable on the specified UI element.
	{
		char	buf[80];
		
		strcpy(buf, label);
		const char*	value = Config::GetValue(var);
		if (value) strcat(buf, value);
		else strcat(buf, "---");
		UI::SetElement(ElementIndex, UI::ElementData(buf, true));
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		if (Config::GetValue("MasterVolume")) Config::ExportValue("MasterVolume");
		Config::ExportValue("SFXVolume");
		Config::ExportValue("CDAudioVolume");
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
//		// Dim the background.
//		Render::FullscreenOverlay(UI::BackgroundDim);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// Return to parent Options menu on ESCAPE.
			UI::SetMode(UI::OPTIONS, Ticks);
		} else if (ElementIndex == 2) {
			// Adjust the master volume slider.
			if (Config::GetValue("MasterVolume") == NULL) {
				// Default to 50% if the master volume is not yet defined.
				Config::SetFloat("MasterVolume", 50);
			}
			float	f = Config::GetFloat("MasterVolume");
			if (code == UI::LEFT) {
				f -= 5;
			} else {
				f += 5;
			}
			Config::SetFloat("MasterVolume", fclamp(0, f, 100));

			// Effect the volume change.
			Sound::SetMasterVolume((unsigned char) (Config::GetFloat("MasterVolume") * 255.0f / 100.0f));
			
			// Play a reference sound.
			Sound::Play("ding.wav", Sound::Controls(1, 0, 1, false));
			
			// Redraw menu.
			ShowOptions();
			
		} else if (ElementIndex == 3) {
			// Adjust the sound-effects volume level.
			if (Config::GetValue("SFXVolume") == NULL) {
				// Default to 50% if the volume is not yet defined.
				Config::SetFloat("SFXVolume", 50);
			}
			float	f = Config::GetFloat("SFXVolume");
			if (code == UI::LEFT) {
				f -= 5;
			} else {
				f += 5;
			}
			Config::SetFloat("SFXVolume", fclamp(0, f, 100));

			// Effect the volume change.
			Sound::SetSFXVolume((unsigned char) (Config::GetFloat("SFXVolume") * 255.0f / 100.0f));

			// Play a reference sound.
			Sound::Play("ding.wav", Sound::Controls(1, 0, 1, false));
			
			// Redraw menu.
			ShowOptions();
			
		} else if (ElementIndex == 4) {
			// Adjust the CD-audio volume level.
			if (Config::GetValue("CDAudioVolume") == NULL) {
				// Default to 50% if the volume is not yet defined.
				Config::SetFloat("CDAudioVolume", 50);
			}
			float	f = Config::GetFloat("CDAudioVolume");
			if (code == UI::LEFT) {
				f -= 5;
			} else {
				f += 5;
			}
			Config::SetFloat("CDAudioVolume", fclamp(0, f, 100));

//			// Effect the volume change.
//			Sound::SetSFXVolume(Config::GetFloat("SFXVolume"));
			// xxxx ?????
			Music::SetMaxVolume((unsigned char) (Config::GetFloat("CDAudioVolume") * 255.0f / 100.0f));
			
			// Play a reference sound.
			Sound::Play("ding.wav", Sound::Controls(1, 0, 1, false));
			
			// Redraw menu.
			ShowOptions();
		}
	}

} SoundOptionsInstance;


// UI::WEATHEROPTIONS handler
class WeatherOptions : public UI::ModeHandler {
private:
public:
	WeatherOptions() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
//		UI::SetElement(0, UI::ElementData("     Weather    ", false));

		UI::SetMenuTitle(UI::String("weather_menu", "WEATHER"));

		UI::SetElement(2, UI::ElementData(UI::String("clear", "Clear"), true));
		UI::SetElement(3, UI::ElementData(UI::String("partly_cloudy", "Partly cloudy"), true));
		UI::SetElement(4, UI::ElementData(UI::String("sunset", "Sunset"), true));
		UI::SetElement(5, UI::ElementData(UI::String("snowing", "Snowing"), true));
		UI::SetElement(6, UI::ElementData(UI::String("whiteout", "Whiteout"), true));
		
		UI::SetCursor(2);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// Return to parent Options menu on ESCAPE.
			UI::SetMode(UI::OPTIONS, Ticks);
		} else if (ElementIndex == 2) {
			lua_dofile("clear.lua");
		} else if (ElementIndex == 3) {
			lua_dofile("cloudy.lua");
		} else if (ElementIndex == 4) {
			lua_dofile("sunset.lua");
		} else if (ElementIndex == 5) {
			lua_dofile("snowing.lua");
		} else if (ElementIndex == 6) {
			lua_dofile("whiteout.lua");
		}
	}

} WeatherOptionsInstance;


// For registering the mode handlers.
static struct InitOptionModes {
	InitOptionModes() {
		GameLoop::AddInitFunction(Init);
	}
	static void	Init()
	// Attach the mode handler to the UI module.
	{
		UI::RegisterModeHandler(UI::OPTIONS, &OptionsInstance);
		UI::RegisterModeHandler(UI::DISPLAYOPTIONS, &DisplayOptionsInstance);
		UI::RegisterModeHandler(UI::SOUNDOPTIONS, &SoundOptionsInstance);
		UI::RegisterModeHandler(UI::WEATHEROPTIONS, &WeatherOptionsInstance);
	}
} InitOptionModes;

