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
// game.cpp	-thatcher 11/15/1998 Copyright Slingshot

// Game stuff.  Timers, levels, etc.


#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif

//#include <windows.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "boarder.hpp"
#include "main.hpp"
#include "game.hpp"
#include "text.hpp"
#include "timer.hpp"
#include "model.hpp"
#include "ui.hpp"
#include "user.hpp"
#include "sound.hpp"
#include "config.hpp"
#include "terrain.hpp"
#include "polygonregion.hpp"
#include "utility.hpp"
#include "player.hpp"
#include "music.hpp"
#include "overlay.hpp"
#include "imageoverlay.hpp"
#include "recording.hpp"
#include "usercamera.hpp"
#include "lua.hpp"
#include "weather.hpp"
#include "attractmode.hpp"
#include "console.hpp"
#include "highscore.hpp"
#include "../data/gui/guidefs.h"

#include "multiplayer.hpp"

#ifdef WIN32
#define snprintf _snprintf
#endif // WIN32


namespace Game {
;


// User.
MDynamic*	User = NULL;

// Course timer state.
int	TimerTicks = 0;
bool	TimerActive = false;

// Score.
  static int	Score[MultiPlayer::MAX_PLAYERS] = {0, 0, 0, 0};

static void	UpdateBonusMessagesAndFlyingPoints(int DeltaTicks);
static void	OpenBonusMessages();
static void	CloseBonusMessages();
static void	PostLoadRuns();

// Run, mtn state.
int	CurrentRun = 0;
int	ActiveRun = CurrentRun;	// ActiveRun is used to determine what par time shows up on the scoreboard.
char*	CurrentMountain = 0;



// Current player.
Player*	CurrentPlayer[] = {NULL, NULL, NULL, NULL};


// HUD.
GG_Player*	HUDPlayer = NULL;


int	FinishTime = 0;


const int	MAX_RUNS = 20;
struct RunInfo {
	int	RunID;	// Not the same as the run index.
	RunStarter*	Starter;
} RunData[MAX_RUNS];
int	RunCount = 0;


const int	MAX_END_TRIGGERS = 50;
struct EndTriggerInfo {
	int	RunID;
	RunEndTrigger*	Trigger;
} EndTriggerData[MAX_END_TRIGGERS];
int	EndTriggerCount = 0;


//
// Lua hooks.
//

static void	GetCurrentRun_lua() {
	lua_pushnumber(GetCurrentRun()+1);
}

static void	RunCompleted_lua() {
	RunCompleted(int(lua_getnumber(lua_getparam(1))-1));
}

static void
AddBonusMessage_lua() {
	AddBonusMessage(lua_getstring(lua_getparam(1)),
	                int(lua_getnumber(lua_getparam(2))),
	                lua_getstring(lua_getparam(3)),
	                Sound::Controls());
}

//static void	AddFlyingPoints_lua() {
//	AddFlyingPoints(lua_getnumber(lua_getparam(1)),
//			lua_getnumber(lua_getparam(2)),
//			lua_getnumber(lua_getparam(3)),
//			lua_getnumber(lua_getparam(4)));
//}


/**
 * Opens the game module. Basically just initializes some stuff.
 */
void
Open()
{
	int	i;
	for (i = 0; i < MAX_RUNS; i++) {
		RunData[i].Starter = NULL;
	}

	// Register some Lua hooks.
	lua_register("game_get_current_run", GetCurrentRun_lua);
	lua_register("game_run_completed", RunCompleted_lua);
	lua_register("game_add_bonus_message", AddBonusMessage_lua);
//	lua_register("game_add_flying_points", AddFlyingPoints_lua);

	HighScore::Open();

	GameLoop::CacheMovie("bonus.ggm");

	// Set up the HUD.
	HUDPlayer = GameLoop::LoadMovie("hud.ggm");
	if (HUDPlayer == NULL) {
		Error e; e << "Can't open movie 'hud.ggm'";
		throw e;
	}
	HUDPlayer->setPlayMode(GG_PLAYMODE_HANG);

	// Set up the bonus messages.
	OpenBonusMessages();
}


void	Close()
// Close the game module.
{
	CloseBonusMessages();
	
	// Make sure any player data has been saved.
	for (int player_index=0; player_index < MultiPlayer::NumberOfLocalPlayers(); player_index++)
	  if (CurrentPlayer[player_index]) 
	    CurrentPlayer[player_index]->Save();

	HighScore::Close();

	HUDPlayer->unRef();
	HUDPlayer = NULL;

	UI::MountainClear();
}


int	InitFunctionCount = 0;
const int	MAX_INIT_FUNCTIONS = 100;
void	(*InitFunctions[MAX_INIT_FUNCTIONS])();


int	ClearFunctionCount = 0;
const int	MAX_CLEAR_FUNCTIONS = 100;
void	(*ClearFunctions[MAX_CLEAR_FUNCTIONS])();


int	PostLoadFunctionCount = 0;
const int	MAX_POSTLOAD_FUNCTIONS = 100;
void	(*PostLoadFunctions[MAX_POSTLOAD_FUNCTIONS])();


void	AddInitFunction(void (*InitFct)())
// Adds a function to be called just after loading mountain data.
{
	if (InitFunctionCount >= MAX_INIT_FUNCTIONS) {
		Error e; e << "InitFunctionCount exceeded.";
		throw e;
	}

	// Add the given function to the list.
	InitFunctions[InitFunctionCount] = InitFct;
	InitFunctionCount++;
}


void	AddClearFunction(void (*ClearFct)())
// Adds a function to be called before clearing mountain data.
{
	if (ClearFunctionCount >= MAX_CLEAR_FUNCTIONS) {
		Error e; e << "ClearFunctionCount exceeded.";
		throw e;
	}

	// Add the given function to the list.
	ClearFunctions[ClearFunctionCount] = ClearFct;
	ClearFunctionCount++;
}


void	AddPostLoadFunction(void (*PostLoadFct)())
// Adds a function to be called after loading mountain data.
{
	if (PostLoadFunctionCount >= MAX_POSTLOAD_FUNCTIONS) {
		Error e; e << "PostLoadFunctionCount exceeded.";
		throw e;
	}

	// Add the given function to the list.
	PostLoadFunctions[PostLoadFunctionCount] = PostLoadFct;
	PostLoadFunctionCount++;
}


void	LoadPlayer(const char* PlayerName)
// Creates a player and loads it from the specified file.  Deletes the current player.
// Sets the run to the first run the new player hasn't yet completed.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();

	// Clean up the current player, if any.
	if (CurrentPlayer[player_index]) {
		CurrentPlayer[player_index]->Save();
		delete CurrentPlayer[player_index];
	}

	// Load the new player.
	CurrentPlayer[player_index] = Player::LoadPlayer(PlayerName);

	// xxx set the mountain to the last one used by this player?
	
	// Set the run to the next one this player hasn't completed yet.
	SetCurrentRun(CurrentPlayer[player_index]->GetHighestRunCompleted(GetCurrentMountain()) + 1);
}


Player*	GetCurrentPlayer()
// Returns a pointer to the current player.
{
  int player_index = MultiPlayer::CurrentPlayerIndex();
  return CurrentPlayer[player_index];
}


void	RunCompleted(int runid)
// Credits the player with having completed the specified run.
{
	Player*	p = GetCurrentPlayer();
	p->CompletedRun(GetCurrentRun());
}


const char*	GetCurrentMountain()
// Returns the name of the currently-loaded mountain.
{
	return CurrentMountain;
}


void	ClearMountain()
// Empties the terrain, the terrain mesh, the object database, the model
// resource database, and game info.
{
	// Make sure any player data has been saved.
	for (int player_index=0; player_index<MultiPlayer::NumberOfLocalPlayers(); player_index++)
	  if (CurrentPlayer[player_index]) 
	    CurrentPlayer[player_index]->Save();

	// Call clear functions.
	int	i;
	for (i = 0; i < ClearFunctionCount; i++) {
		(*ClearFunctions[i])();
	}

	// Notify UI handlers about the dataset clear.
	UI::MountainClear();

	Boarder::Clear();
	TerrainMesh::Clear();
	Surface::Clear();
	PolygonRegion::Clear();
	TerrainModel::Clear();
	Model::Clear();
	Overlay::Clear();
	// UI::Clear() ???;  actually, probably not.
	// Sound::Clear();

	CurrentRun = 0;
	ActiveRun = 0;
	User = NULL;
	
	TimerTicks = 0;
	TimerActive = false;
	
	RunCount = 0;
	EndTriggerCount = 0;

	if (CurrentMountain) {
		delete [] CurrentMountain;
		CurrentMountain = NULL;
	}
}


static GG_Player*	LoadingPlayer = NULL;


void	LoadMountain(const char* MountainName)
// Loads mountain info from the file formed by adding ".srt" to the given mountain name.
{
	GameLoop::AutoPauseInput	autoPause;	// Turn off the input thread inside this function.

// For Virtual Resorts and such, fix the name of the mountain file in the executable.
#ifdef STATIC_MOUNTAIN
	MountainName = STATIC_MOUNTAIN;
#endif
	
	// Start a movie to show loading messages.
	LoadingPlayer = GameLoop::LoadMovie("loading_message.ggm");
	if (LoadingPlayer) {
		LoadingPlayer->getMovie()->setActorText(GGID_LOADING_CAPTION, UI::String("loading_caption", "loading..."));
		LoadingTick(true);
	}

	LoadingTick();

	// This is the new current mountain.
	int	len = strlen(MountainName);
	CurrentMountain = new char[len+1];
	strcpy(CurrentMountain, MountainName);

	// Set the default path for data files.
	Utility::SetDefaultPath(MountainName);

	// Define a nested scope for config variable values specific to
	// this mountain.
	Config::SetLocalScope(MountainName);

	// Run pre-load script, if any.
	{
		const int	SCRIPTFILE_MAXLEN = 1000;
		char	scriptfile[SCRIPTFILE_MAXLEN];
		snprintf(scriptfile, SCRIPTFILE_MAXLEN, "../data/%s/preload.lua", MountainName);
		lua_dofile(scriptfile);
	}
	
	// Load surface bitmaps.
	LoadingMessage(UI::String("surface_textures_caption", "surface textures"));
	Surface::ResetSurfaceShader();
	
	// Prepare to load data file.
	const static int	BUF = 256;
	if (len + 5 > BUF) {
		Error e; e << "Game::LoadMountain: mountain name too long.";
		throw e;
	}
	char	filename[BUF];
	strcpy(filename, MountainName);
	char	cachefilename[BUF];
	strcpy(cachefilename, filename);
	strcat(cachefilename, ".dat");
	strcat(filename, ".srt");

	// Clear high score table.
	HighScore::Clear();
	
	// Clear any recordings hanging around.
	Recording::Clear();
	
	// Reload weather/sky gradients etc.
	Weather::Reset();

	// Reload shadetable.
	lua_dostring("shadetable_reset('shadetable.psd')");
	
	// Call init functions.
	int	i;
	for (i = 0; i < InitFunctionCount; i++) {
		(*InitFunctions[i])();
	}

	// Let UI handlers know we're loading a dataset.
	UI::MountainInit();

	// Get modification time of file and file size.
	struct stat	finfo;
	time_t	modtime = 0;
	int	filesize = 0;
	if (Utility::FileStat(filename, &finfo) == 0) {
		modtime = finfo.st_mtime;
		filesize = finfo.st_size;
	}

#ifdef MOUNTAIN_FILESIZE
	if (filesize != MOUNTAIN_FILESIZE) {
		Error e; e << "Mountain data file '" << filename << "' is corrupted.";
		throw e;
	}
#endif // MOUNTAIN_FILESIZE
	
	// Attempt to load.
	FILE*	fp = Utility::FileOpen(filename, "rb");
	if (fp == NULL) {
		Error e; e << "Can't open mountain data file '" << filename << "' for input.";
		throw e;
	} else {
		// Read the header.
		int	Marker, Version;

		Marker = Read32(fp);	// Read the '0x0FFFFFFFF' header marker.
		if (Marker != (int) 0x0FFFFFFFF) {
			Error e; e << "Input file '" << filename << "' is not a Soul Ride terrain.";
			throw e;
		}

		Version = Read32(fp);
		const int	VALID_VERSION = 12;
		if (Version < VALID_VERSION) {
			Error e; e << "Input file '" << filename << "' is an old version.  Resave from latest Shreditor.";
			throw e;
		} else if (Version > VALID_VERSION) {
			Error e; e << "Input file '" << filename << "' is in a new format.  You need an updated soulride.exe in order to load it.";
			throw e;
		}
		
		// Load the heightfield description.
		LoadingMessage(UI::String("topography_caption", "topography"));
		int	checkcode = TerrainMesh::Load(fp);	// Load quadtree height data.
		checkcode &= 0x0FFFF;

		if (Config::GetBool("ShowCheckCode")) {
			Console::Printf("checkcode = %d\n", checkcode);
		}

#ifdef MOUNTAIN_CHECKCODE
		if (checkcode != MOUNTAIN_CHECKCODE) {
			fclose(fp);
			Error e; e << "Mountain data file '" << filename << "' is corrupted.";
			throw e;
		}
#endif
		
		//
		// Load surface info.
		//
		
		// Figure out if there's valid cached terrain data on disk.
		bool	Write = true;

		// Figure out when the cache file was modified.  If it's newer than the .srt
		// file, then it's OK to read it.  Otherwise, write it.
		if (Utility::FileStat(cachefilename, &finfo) == 0) {
			time_t	cache_modtime = finfo.st_mtime;
			if (cache_modtime > modtime) {
				Write = false;
			}
		}

		static int	CACHE_FILE_VERSION = 0;
		FILE*	fp_cache;
		if (Write == false) {
			// Open cache file for reading.
			fp_cache = Utility::FileOpen(cachefilename, "rb");
			if (fp_cache) {
				int	ver = fgetc(fp_cache);
				if (ver != CACHE_FILE_VERSION) {
					// Cache file is an incompatible version.  Overwrite it.
					fclose(fp_cache);
					Write = true;
				}
			}
		}
		if (Write) {
			// Open cache file for writing.
			fp_cache = Utility::FileOpen(cachefilename, "w+b");
			if (fp_cache == NULL) {
				Write = false;
			} else {
				// Write the version number.
				fputc(CACHE_FILE_VERSION, fp_cache);
			}
		}
		// Do the actual load of surface info.
		LoadingMessage(UI::String("terrain_coverage_caption", "terrain coverage"));
		TerrainModel::Load(fp, fp_cache, Write);	// Surface types, lightmaps.
		if (fp_cache) fclose(fp_cache);

		// Load geometry.
		LoadingMessage(UI::String("models_caption", "models"));
		Model::LoadGModels(fp);

		// Load solids.
		Model::LoadSModels(fp);
		
		// Load objects.
		LoadingMessage(UI::String("objects_caption", "objects"));
		Model::LoadObjects(fp);

		// Load the terrain-region polygons.
		LoadingMessage(UI::String("regions_caption", "regions"));
		PolygonRegion::Load(fp);
	}
	fclose(fp);

	// Call post-load functions.
	for (i = 0; i < PostLoadFunctionCount; i++) {
		(*PostLoadFunctions[i])();
	}

	PostLoadRuns();

	// Set the player to the highest run they haven't completed on this mountain.
	for (int player_index=0; player_index < MultiPlayer::NumberOfLocalPlayers(); player_index++){
		if (CurrentPlayer[player_index]) {
			SetCurrentRun(CurrentPlayer[player_index]->GetHighestRunCompleted(GetCurrentMountain()) + 1);
		}
	}

	// Reset the user to the selected run, so they can see where it starts.
	Game::ResetRun(GetCurrentRun());

	// Initialize high scores.
	HighScore::Read();
	
	// Delete the loading movie.
	LoadingPlayer->unRef();
	LoadingPlayer = NULL;

	// Run post-load script, if any.
	{
		const int	SCRIPTFILE_MAXLEN = 1000;
		char	scriptfile[SCRIPTFILE_MAXLEN];
		snprintf(scriptfile, SCRIPTFILE_MAXLEN, "../data/%s/postload.lua", MountainName);
		lua_dofile(scriptfile);
	}

	// Ensure the weather is reset.
	lua_dofile("cloudy.lua");
}


void	LoadingMessage(const char* message)
// Displays a message in the loading movie.
{
	if (LoadingPlayer) {
		LoadingPlayer->getMovie()->setActorText(GGID_LOADINGMESSAGE, (char*) message);
		LoadingTick(true);
	}
}


void	LoadingTick(bool ForceUpdate)
// Advances the loading movie by 100 ms, so the user has an indication that
// something is happening.
{
	// Don't update more than 10 Hz.
	static int	LastFrameTicks = 0;
	int	ticks = Timer::GetTicks();
	if (ForceUpdate == false && ticks - LastFrameTicks < 100) {
		return;
	}
	int	dt = ticks - LastFrameTicks;
	if (dt > 200) dt = 200;
	LastFrameTicks = ticks;
	
	if (LoadingPlayer) {
		// Do a frame of the movie.
		glViewport(0, 0, Render::GetWindowWidth(), Render::GetWindowHeight());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GameLoop::GUIBegin();
		LoadingPlayer->play(dt);
		GameLoop::GUIEnd();
//		OGL::SwapBuffers(GetDC(Main::GetGameWindow()));
		Render::ShowFrame();
	}
}


void	Update(const UpdateState& u)
// Updates the game stuff.  Increments the timer if necessary.
{
	// Increment timer if necessary.
	if (TimerActive) {
		TimerTicks += u.DeltaTicks;
	}

	// Deal with bonus messages.
	UpdateBonusMessagesAndFlyingPoints(u.DeltaTicks);

	AttractMode::Update(u);
	UserCamera::AutoCameraUpdate(u);

	// Store game summary info in the recording if we're in record mode.
	if (Recording::GetMode() == Recording::RECORD && Recording::IsReadyForStateData()) {
		// Allocate and insert the data.
		uint8*	buf = Recording::NewChunk(Recording::GAME_STATE, 5);
		if (buf) {
			int	index = 0;
			index += EncodeUInt16(buf, index, GetTimer() / 10);	// TimerHundredths
			buf[index++] = Config::GetInt("RewindsAllowed");	// Rewinds
			index += EncodeUInt16(buf, index, GetScore());	// Score
		}
	}

	//
	// Game state playback.
	//
	UI::Mode	mode = UI::GetMode();
	if (mode == UI::PLAYBACK || mode == UI::SHOWREWIND) {
		uint8*	bufa;
		uint8*	bufb;
		float	f;
		Recording::GetCurrentChunkPair(Recording::GAME_STATE, &bufa, &bufb, &f);
		if (bufa) {
			int	index = 0;
			uint16	TimerHundredths;
			index += DecodeUInt16(bufa, index, &TimerHundredths);
			
			if (bufb) {
				// Interpolate.
				uint16	tb;
				DecodeUInt16(bufb, 0, &tb);
				TimerHundredths += uint16((tb - TimerHundredths) * f);
			}
			
			Game::SetTimer(((int)TimerHundredths) * 10);

			int	rew = (int8) bufa[index++];
			if (mode != UI::SHOWREWIND) {
				Config::SetIntValue("RewindsAllowed", rew);
			}
			
			int16	Points;
			index += DecodeUInt16(bufa, index, (uint16*) &Points);
			Game::SetScore(Points);
			// health = bufa[index++];
		}
	}
}


//
// Run bookkeeping.
//


void	PostLoadRuns()
// Make sure runs have a chance to run post-load stuff (mainly to grab
// translation overrides).
{
	for (int i = 0; i < RunCount; i++)
	{
		RunData[i].Starter->PostLoad(i);
	}
}


void	RegisterRunEndTrigger(RunEndTrigger* t, int RunID)
// Registers a run end trigger for the specified run.  A run can have
// more than one end trigger.  The triggers for a run are activated when
// the run is selected, and all other triggers are deactivated.
{
	if (EndTriggerCount >= MAX_END_TRIGGERS) {
		Error e; e << "MAX_END_TRIGGERS exceeded";
		throw e;
	}

	EndTriggerData[EndTriggerCount].RunID = RunID;
	EndTriggerData[EndTriggerCount].Trigger = t;

	EndTriggerCount++;
}


void	RegisterRunStarter(RunStarter* s, int RunID)
// Registers the given starter for a run, identified with the given ID.  The
// starter will be used to reset the user at the start of that run.
{
	if (RunID < 0) {
		Error e; e << "RegisterRunStarter(): invalid run index " << RunID;
		throw e;
	}
	if (RunCount >= MAX_RUNS) {
		Error e; e << "RegisterRunStarter(): MAX_RUNS (" << MAX_RUNS << ") exceeded.";
		throw e;
	}

	RunData[RunCount].RunID = RunID;
	RunData[RunCount].Starter = s;
	RunCount++;

	// Sort the runs by RunID.
	int	i, j;
	for (j = 0; j < RunCount; j++) {
		for (i = 0; i < RunCount-1; i++) {
			if (i != j && RunData[i].RunID > RunData[i+1].RunID) {
				// Swap.
				RunInfo	temp = RunData[i];
				RunData[i] = RunData[i+1];
				RunData[i+1] = temp;
			}
		}
	}
	
//
//	if (RunCount < RunIndex + 1) RunCount = RunIndex + 1;
}


void	SetCurrentRun(int RunIndex)
// Sets the current run, i.e. course, level.
{
	if (RunIndex < -1 || RunIndex >= GetRunCount()) return;
	
	CurrentRun = RunIndex;
	ActiveRun = CurrentRun;

	// Activate/deactivate end triggers as appropriate.
	int	id = -1;
	if (RunIndex >= 0) id = RunData[CurrentRun].RunID;
	
	int	i;
	for (i = 0; i < EndTriggerCount; i++) {
		if (EndTriggerData[i].RunID == id) {
			EndTriggerData[i].Trigger->SetActive(true);
		} else {
			EndTriggerData[i].Trigger->SetActive(false);
		}
	}
}


int	GetCurrentRun()
// Returns the current run.
{
	return CurrentRun;
}


void	SetActiveRun(int run_index)
// Set this to show a certain run's par time in the scoreboard,
// without changing the user's current run (i.e. when playing back a
// recording, set this value to the run index of the recording.
{
	ActiveRun = run_index;
}


int	GetActiveRun()
// Returns the run whose time should be shown in the scoreboard.  This
// is not the same as the user's current run (i.e. the run they will
// start at when they select "Carve Now") in the case when we're playing
// back a recording.
{
	return ActiveRun;
}


int	GetRunCount()
// Returns the count of runs on this mountain.
{
	return RunCount;
}


const char*	GetRunName(int index)
// Returns the name of the specified run.
{
	const char*	name = "---";	// default.

	if (index == -1) {
		return UI::String("heli_drop_name", "heli-drop");
	}
	if (index >= 0 && index < RunCount) {
		RunStarter*	starter = RunData[index].Starter;
		if (starter) {
			name = starter->GetRunName();
		}
	}

	return name;
}


const char*	GetRunInfoText(int index)
// Returns descriptive text of the specified run, to show to the user before doing the run.
{
	const char*	text = "-- no info --";	// default.

	if (index == -1) {
		return UI::String("heli_drop_desc", "You're on your own.  Good luck...");
	}
	if (index >= 0 && index < RunCount) {
		RunStarter*	starter = RunData[index].Starter;
		if (starter) {
			text = starter->GetRunInfoText();
			if (text == NULL) text = "-- no info --";
		}
	}

	return text;
}


int	GetRunParTicks(int index)
// Returns the par time for finishing the specified run.
{
	if (index >= 0 && index < RunCount) {
		RunStarter*	starter = RunData[index].Starter;
		if (starter) {
			return starter->GetRunParTicks();
		}
	}

	return 0;
}


float	GetRunVerticalDrop(int index)
// Returns the vertical drop of the specified run.
{
	MObject*	start = NULL;
	MObject*	end = NULL;

	if (index >= 0 && index < RunCount) {
		// Find run starter.
		start = RunData[index].Starter;

		// Find highest end trigger.
		int	i;
		for (i = 0; i < EndTriggerCount; i++) {
			if (EndTriggerData[i].RunID == RunData[index].RunID &&
			    (end == NULL || EndTriggerData[i].Trigger->GetLocation().Y() > end->GetLocation().Y()))
			{
				end = EndTriggerData[i].Trigger;
			}
		}
	}

	if (start && end) {
		return start->GetLocation().Y() - end->GetLocation().Y();
	} else {
		return 0;
	}
}


vec3	HeliDropLocation = ZeroVector;
vec3	HeliDropDirection = XAxis;
vec3	HeliDropVelocity = ZeroVector;

	
void	ResetRun(int index)
// Resets the user to the start of the specified run.
{
	// Reset end triggers.
	SetCurrentRun(GetCurrentRun());

	// Reset the user.
	if (index == -1) {
		// Heli-drop parameters.
		MDynamic*	user = GetUser();
		if (user) {
			user->Reset(HeliDropLocation, HeliDropDirection, HeliDropVelocity);
		}
	}
	if (index >= 0 && index < RunCount) {
		RunStarter*	starter = RunData[index].Starter;
		MDynamic*	user = GetUser();
//		if (user) user->Reset()
		if (starter && user) {
			starter->ResetUser(user);
		}
	}
}


void	SetHeliDropInfo(const vec3& loc, const vec3& dir, const vec3& vel)
// Sets up reset info for a heli-drop run.
{
	HeliDropLocation = loc;
	HeliDropDirection = dir;
	HeliDropVelocity = vel;
}


//
// Timer
//


void	SetTimer(int ticks)
// Initializes the timer value to the given number of (millisecond) ticks.
{
	TimerTicks = ticks;
}


int	GetTimer()
// Returns the number of ticks recorded so far by the course timer.
{
	return TimerTicks;
}


void	SetTimerActive(bool NewActive)
// Turns the course timer on or off.
{
	TimerActive = NewActive;
}


//
// Score.
//


const int	FLYING_POINTS_CT = 20;
struct FlyingPointsInfo {
	float	x, y, dx, dy;
	int	Color;
	int	Timer;
	int	Points;

	FlyingPointsInfo() { Timer = 0; }
} FlyingPoints[FLYING_POINTS_CT][MultiPlayer::MAX_PLAYERS];


void	SetScore(int points)
// Sets the player's current score to the given value.  Cancels
// all pending "flying points".
{
	int player_index = MultiPlayer::CurrentPlayerIndex();

	Score[player_index] = points;

	int	i;
	for (i = 0; i < FLYING_POINTS_CT; i++) {
		FlyingPoints[i][player_index].Timer = 0;
	}
}


int	GetScore() { 
  int player_index = MultiPlayer::CurrentPlayerIndex();
  int total = Score[player_index];
  return Score[player_index]; 
}
void	AddScore(int delta) { 
  int player_index = MultiPlayer::CurrentPlayerIndex();
  Score[player_index] += delta; 
}


int	GetTotalScore()
// Returns the current player score, plus all pending flying points.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();  

	int	total = Score[player_index];

	int	i;
	for (i = 0; i < FLYING_POINTS_CT; i++) {
	  if (FlyingPoints[i][player_index].Timer) 
	    total += FlyingPoints[i][player_index].Points;
	}

	return total;
}


const int	BONUS_MSG_CT = 5;
const int	BONUS_MAXLEN = 80;

struct BonusMessageInfo {
	int	Points;
	char	SoundName[BONUS_MAXLEN];
	Sound::Controls	SoundControls;
	GG_Player*	Player;
	GG_Movie*	Movie;
	int	Timer;
	
	BonusMessageInfo() {
		Points = 0;
		SoundName[0] = 0;
		Player = NULL;
		Movie = NULL;
		Timer = 0;
	}

	BonusMessageInfo&	operator=(const BonusMessageInfo& b)
	{
		Points = b.Points;
		memcpy(SoundName, b.SoundName, BONUS_MAXLEN);
		SoundControls = b.SoundControls;
		Player = b.Player;
		Movie = b.Movie;
		Timer = b.Timer;

		return *this;
	}
} BonusMessage[BONUS_MSG_CT];


void	OpenBonusMessages()
// Preload our movies.
{
	for (int i = 0; i < BONUS_MSG_CT; i++)
	{
		BonusMessageInfo&	b = BonusMessage[i];
		GG_Rval	ret = GUI->loadMovie((char*) "bonus.ggm", &b.Movie);
		if (ret != GG_OK) {
			Error e; e << "Can't load movie file 'bonus.ggm'.";
			throw e;
		}
	}
}


void	CloseBonusMessages()
// Free resources opened by bonus messages.
{
	int	i;
	for (i = 0; i < BONUS_MSG_CT; i++) {
		BonusMessageInfo&	b = BonusMessage[i];
		if (b.Player) {
			b.Player->unRef();
			b.Player = NULL;
		}
		if (b.Movie) {
			b.Movie->unRef();
			b.Movie = NULL;
		}
	}
}


void	AddBonusMessage(const char* desc, int points, const char* soundname, const Sound::Controls& soundparams)
// Show an explanatory message, accompanied by a points bonus, and a sound.
{
	// Rotate the message queue, and put the new message in the top slot.
	int	i;
	BonusMessageInfo	temp = BonusMessage[BONUS_MSG_CT-1];
	for (i = BONUS_MSG_CT-1; i > 0; i--) {
		BonusMessage[i] = BonusMessage[i-1];
	}
	BonusMessage[0] = temp;
	if (temp.Timer && temp.Points) AddScore(temp.Points);	// Make sure points got added, before we overwrite this slot.

	// Set up new message.
	BonusMessageInfo&	b = BonusMessage[0];
	
	b.Points = points;

	if (soundname) {
		strncpy(b.SoundName, soundname, BONUS_MAXLEN);
		b.SoundName[BONUS_MAXLEN-1] = 0;
	} else {
		b.SoundName[0] = 0;
	}

	b.SoundControls = soundparams;

	// If we haven't loaded & created the movie for this slot yet, then do it.
	assert(b.Movie);
	if (b.Player == NULL) {
		GUI->createPlayer(b.Movie, &b.Player);
//		b.Player = GameLoop::LoadMovie("bonus.ggm", &b.Movie);
		if (b.Player == NULL) {
			Error e; e << "Can't create movie 'bonus.ggm'";
			throw e;
		}
		b.Player->setPlayMode(GG_PLAYMODE_HANG);

//		Console::Printf("Loaded bonus.ggm, b.Player == x%X\n", b.Player);//xxxxx
	}

	b.Player->setTime(0);
	b.Movie->setActorText(GGID_BONUS_DESC, (char*) desc);
	char	buf[80];
	Text::FormatNumber(buf, float(b.Points), 4, 0);
	b.Movie->setActorText(GGID_BONUS_NUM, buf);
	
	b.Timer = b.Player->getDuration();
}


static int	BonusMessageDeltaTicks = 0;


void	UpdateBonusMessagesAndFlyingPoints(int DeltaTicks)
// Decrement bonus message timers, etc.
{
	BonusMessageDeltaTicks += DeltaTicks;
}


void	ShowScore()
// Display the score.
{
	int	dt = BonusMessageDeltaTicks;
	BonusMessageDeltaTicks = 0;
	
	char	buf[80];
	
	int player_index = MultiPlayer::CurrentPlayerIndex();

	// Current player score.
	Text::FormatNumber(buf, float(Score[player_index]), 5, 0);
	HUDPlayer->getMovie()->setActorText(GGID_SCORE, buf);

	// Timer.
	Text::FormatTime(buf, GetTimer());
	strcat(buf, " / ");
	Text::FormatTime(buf + strlen(buf), GetRunParTicks(GetActiveRun()));
	HUDPlayer->getMovie()->setActorText(GGID_CURTIME, buf);

	GameLoop::GUIBegin();

	HUDPlayer->play(dt);

	// Show the bonus messages.
	int	i = 0;

	// Don't start displaying messages until the older ones in the queue have advanced
	// beyond a certain point.
	for (i = 0; i < BONUS_MSG_CT-1; i++) {
		if (BonusMessage[i].Timer == 0) break;
		if (BonusMessage[i+1].Timer == 0) break;
		
		int	dur = BonusMessage[i+1].Player->getDuration();
		if (BonusMessage[i+1].Timer < dur - 500) {
			// OK, ready to display this message.
			break;
		}
	}
	
	for ( ; i < BONUS_MSG_CT; i++) {
		BonusMessageInfo&	b = BonusMessage[i];
		if (b.Timer) {
			// Advance the movie.
			b.Player->play(dt);

			int	dur = b.Player->getDuration();

			const int	SOUND_DELAY = 850;
			// Play the animation a bit, then play the sound associated with the anim.
			if (b.Timer > dur - SOUND_DELAY && b.Timer - dt <= dur - SOUND_DELAY) {
				// Play the associated sound, if any.
				if (b.SoundName[0]) {
					Sound::Play(b.SoundName, b.SoundControls);
				}
			}

			// After a short delay, add the bonus points associated with this message.
			if (b.Timer > dur - 2000 && b.Timer - dt <= dur - 2000) {
				HUDPlayer->setTime(0);	// Flash the HUD.
				AddScore(b.Points);
				b.Points = 0;
			}
				
			b.Timer -= dt;
			if (b.Timer < 0) {
				b.Timer = 0;
				if (b.Points) {
					HUDPlayer->setTime(0);	// Flash the HUD.
					AddScore(b.Points);	// Make sure the points get added...
				}
				b.Points = 0;
			}
		}
	}

	GameLoop::GUIEnd();
}




static Render::Texture*	RewindIcon = NULL;


void	ShowRewinds()
// Display the rewind icons.
{
	if (RewindIcon == NULL) {
		RewindIcon = Render::NewTexture("rewind-icon.psd", true, false, false);
	}

	if (RewindIcon) {
		int	ct = Config::GetInt("RewindsAllowed");
		int	w = int(RewindIcon->GetWidth() * 0.625);
		int	h = int(RewindIcon->GetHeight() * 0.625);
		for (int i = 0; i < ct; i++) {
// lower right			ImageOverlay::Draw(635 - w - (i*w*2)/3, 475 - h /* - (i*h)/5 */, w, h, RewindIcon, 0, 0, 0xFFFFFFFF, 0.625);
			ImageOverlay::Draw(639 - w - (i*w*2)/3, 67 - h /* - (i*h)/5 */, w, h, RewindIcon, 0, 0, 0xFFFFFFFF, 0.625);
		}
	}
}


//
// game stuff
//


void	RegisterUser(MDynamic* user)
// Sets the user pointer, for use by the Game module.
{
	User = user;
}


MDynamic*	GetUser()
// Returns a pointer to the user object.  If there is no user object registered,
// returns NULL.
{
	return User;
}


void	SetUserActive(bool active)
// Tries to activate/deactivate the user.
{
	UserType*	u = dynamic_cast<UserType*>(User);
	if (u) {
		u->SetActive(active);
	}

//	if (active) {
//		Music::FadeUp();
//	} else {
//		Music::FadeDown();
//	}
}


//CameraMode	PlayerCameraMode = CAM_THIRD_PERSON;


void	SetChaseCameraParams()
// Sets the default chase-camera parameters for in-play modes.
{
	UserCamera::SetAutoCameraMode(UserCamera::AUTO_OFF);

	CameraMode	mode = (CameraMode) Config::GetInt("PlayerCameraMode");
	if (mode == CAM_THIRD_PERSON) {
		UserCamera::SetSubject(Game::GetUser());
		UserCamera::SetMotionMode(UserCamera::CHASE);
		UserCamera::SetAimMode(UserCamera::LOOK_AT);
		
		UserCamera::SetChaseParams(ZeroVector, -4, 0, 0, 1.8f, 0);
//		UserCamera::SetChaseParams(ZeroVector, -1, 0, 3, 0);	// top view
//		UserCamera::SetChaseParams(ZeroVector, -2, 0, 0.5, PI/2);	// side view
	} else {
		// CAM_FIRST_PERSON

		UserCamera::SetSubject(Game::GetUser());
		UserCamera::SetMotionMode(UserCamera::CHASE);
		UserCamera::SetAimMode(UserCamera::FIRST_PERSON);
		
		UserCamera::SetChaseParams(vec3(0, 0.7f, 0), 0.0, 0.5f, 0, 0, 0);
	}
}


void	SetPlayerCameraMode(CameraMode m)
// Sets the in-play camera view from a set of discrete options.
{
	if (m >= 0 && m < CAMERA_MODES) {
		Config::SetInt("PlayerCameraMode", (int) m);
	}

	if (UI::GetMode() == UI::PLAYING ||
	    UI::GetMode() == UI::COUNTDOWN ||
	    UI::GetMode() == UI::RESUME ||
	    UI::GetMode() == UI::SHOWCRASH ||
	    UI::GetMode() == UI::SHOWFINISH
	   )
	{
		SetChaseCameraParams();
	}
}


CameraMode	GetPlayerCameraMode()
// Returns current camera mode.
{
	return (CameraMode) Config::GetInt("PlayerCameraMode");
}



//
// Mode handlers.
//


#ifdef NOT
static void	ShowStability(const ViewState& s)
// Show the stability gauge.
{
	return;	//xxxxxxxxxxxx
	
	static Render::Image*	StabilityGauge = NULL;
	static Render::Image*	StabilityBar = NULL;
	
	// Make sure we have the images we need for drawing gauges.
	if (StabilityGauge == NULL) {
		StabilityGauge = Render::NewImage("stability-gauge.psd");
	}
	if (StabilityBar == NULL) {
		StabilityBar = Render::NewImage("stability-gauge-bar.psd");
	}
		
	ImageOverlay::Draw(20, 260, StabilityGauge->GetWidth(), StabilityGauge->GetHeight(), StabilityGauge, 0, 0);
	int	BarTopY = 1 + 112 * (1 - Config::GetFloatValue("UserStability"));
	ImageOverlay::Draw(20, 260 + BarTopY, StabilityBar->GetWidth(), StabilityBar->GetHeight() - BarTopY, StabilityBar, 0, BarTopY);
}


static void	ShowFatigue(const ViewState& s)
// Show the fatigue gauge.
{
	return;	//xxxxxxxxxxxxx
	
	static Render::Image*	FatigueGauge = NULL;
	static Render::Image*	FatigueBar = NULL;
	
	// Make sure we have the images we need for drawing gauges.
	if (FatigueGauge == NULL) {
		FatigueGauge = Render::NewImage("fatigue-gauge.psd");
	}
	if (FatigueBar == NULL) {
		FatigueBar = Render::NewImage("fatigue-gauge-bar.psd");
	}
		
	ImageOverlay::Draw(567, 260, FatigueGauge->GetWidth(), FatigueGauge->GetHeight(), FatigueGauge, 0, 0);
	int	BarTopY = 1 + 112 * (1 - Config::GetFloatValue("UserFatigue"));
	ImageOverlay::Draw(567, 260 + BarTopY, FatigueBar->GetWidth(), FatigueBar->GetHeight() - BarTopY, FatigueBar, 0, BarTopY);
}
#endif // NOT


// UI::RUNFLYOVER
class RunFlyover : public UI::ModeHandler {
  int	IntroCountdown[MultiPlayer::MAX_PLAYERS];
  int	IntroID[MultiPlayer::MAX_PLAYERS];
  int	TextID[MultiPlayer::MAX_PLAYERS];
public:
	RunFlyover() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
	  int player_index = MultiPlayer::CurrentPlayerIndex();
//	  printf("RUNFLYOVER: Open player_index = %d\n", player_index);

	  // Put the user at the start of the run, and deactivate her.
	  ResetRun(GetCurrentRun());
	  SetUserActive(false);
	  
	  // Make sure attract mode is off.
	  AttractMode::Exit();
	  
	  // Fly towards the starting spot.
	  SetChaseCameraParams();
	  UserCamera::SetAimMode(UserCamera::FLY_AIM);
	  UserCamera::SetMotionMode(UserCamera::FLY);
	  
	  IntroCountdown[player_index] = 0;
	  
	  GameLoop::CacheMovie("runintro.ggm");
	  GameLoop::CacheMovie("runinfo_text.ggm");
	  IntroID[player_index] = Overlay::PlayMovie("runintro.ggm", 0, GG_PLAYMODE_HANG);
	  TextID[player_index] = 0;
	  
	  IntroCountdown[player_index] = Overlay::GetMoviePlayer(IntroID[player_index])->getDuration();
	  
	  GG_Movie*	m = Overlay::GetMovie(IntroID[player_index]);
	  if (m) {
	    m->setActorText(GGID_RUN_INFO_CAPTION, UI::String("run_info_caption", "Run Info:"));
	  }
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	  int player_index = MultiPlayer::CurrentPlayerIndex();

	  Overlay::StopMovie(IntroID[player_index]);
	  if (TextID[player_index]) Overlay::StopMovie(TextID[player_index]);
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	  for (int player_index = 0; 
	       player_index < MultiPlayer::MAX_PLAYERS; 
	       player_index++){
	    if (IntroCountdown[player_index]) {
	      IntroCountdown[player_index] -= u.DeltaTicks;
	      if (IntroCountdown[player_index] <= 0) {
		IntroCountdown[player_index] = 0;
		
		TextID[player_index] = Overlay::PlayMovie("runinfo_text.ggm", 1, GG_PLAYMODE_HANG);
	      }
	    }
	  }
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Show run name and run info.
		int player_index = MultiPlayer::CurrentPlayerIndex();
		GG_Movie*	m = Overlay::GetMovie(TextID[player_index]);
		if (m) {
			m->setActorText(GGID_RUNTITLE, GetRunName(GetCurrentRun()));
			m->setActorText(GGID_RUNDESCRIPTION, GetRunInfoText(GetCurrentRun()));

			m->setActorText(GGID_RUN_INFO_CAPTION, UI::String("run_info_caption", "Run Info:"));
			m->setActorText(GGID_RUNDROP_CAPTION, UI::String("run_drop_caption", "Vertical drop:"));
			m->setActorText(GGID_PARTIME_CAPTION, UI::String("par_time_caption", "Par Time:"));
			m->setActorText(GGID_YOURHIGHSCORE_CAPTION, UI::String("your_high_caption", "Your High"));
			m->setActorText(GGID_LOCALHIGHSCORE_CAPTION, UI::String("local_high_caption", "Local High:"));

			char	buf[80];

			Text::FormatTime(buf, GetRunParTicks(GetCurrentRun()));
			m->setActorText(GGID_PARTIME, buf);

			float	vert = GetRunVerticalDrop(GetCurrentRun());
			if (vert > 0) {
				Text::FormatNumber(buf, vert, 3, 0);
				strcat(buf, "m");
			} else {
				strcpy(buf, "???");
			}
			m->setActorText(GGID_RUNDROP, buf);

			// High scores...
			int	r = GetCurrentRun();

			// Personal best.
			Text::FormatNumber(buf, float(GetCurrentPlayer()->GetHighScore(r)), 5, 0);
			m->setActorText(GGID_YOURHIGHSCORE, buf);

			// Local high score.
			Text::FormatNumber(buf, float(HighScore::GetLocalHighScore(r)), 5, 0);
			m->setActorText(GGID_LOCALHIGHSCORE, buf);
			m->setActorText(GGID_LOCALHIGHPLAYER, (char*) HighScore::GetLocalHighPlayer(r));

//			// World high score.
//			Text::FormatNumber(buf, HighScore::GetWorldHighScore(r), 5, 0);
//			m->setActorText(GGID_WORLDHIGHSCORE, buf);
//			m->setActorText(GGID_WORLDHIGHPLAYER, (char*) HighScore::GetWorldHighPlayer(r));
		}
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// On input, skip to to SHOWRUNINFO.
		if (1 || code == UI::ESCAPE) {
			UI::SetMode(UI::SHOWRUNINFO, Ticks);
		}
	}
} RunFlyoverInstance;

// UI::WAITING_FOR_OTHERS
class WaitingForOthers : public UI::ModeHandler {
public:
	WaitingForOthers() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
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
	  const char*	prompt = UI::String("waiting_prompt", "Waiting for other players ...");
	  Text::DrawString(318, 102, Text::DEFAULT, Text::ALIGN_CENTER, prompt, 0x60202020);	// Drop shadow.
	  Text::DrawString(320, 100, Text::DEFAULT, Text::ALIGN_CENTER, prompt);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
	  // Do nothing unless players cancel.
	  if (code == UI::ESCAPE) {
	    UI::SetMode(UI::MAINMENU, Ticks);
	  }
	}
} WaitingForOthersInstance;
  

// UI::SHOWRUNINFO handler.
class ShowRunInfo : public UI::ModeHandler {
public:
	ShowRunInfo() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		SetTimer(0);
		SetScore(0);
		SetTimerActive(false);

		// Put the user at the start of the run, and deactivate her.
		ResetRun(GetCurrentRun());
		SetUserActive(false);

		// Make sure attract mode is off.
		AttractMode::Exit();
		
		// Ensure camera stuff is set up.
		SetChaseCameraParams();

		//xxxxxxx
		UserCamera::SetAimMode(UserCamera::FLY_AIM);
		UserCamera::SetMotionMode(UserCamera::FLY);
		//xxxxxxx

		// Reset the rewind count.
		Config::SetInt("RewindsAllowed", 3);

		// Bjorn, before: UI::SetMode(UI::COUNTDOWN, Ticks);
		UI::SetMode(UI::WAITING_FOR_OTHERS, Ticks);	//xxxxxxxx skip this mode.
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
		// Dim the background.
		Render::FullscreenOverlay(UI::BackgroundDim);
		
		// Show mountain name, run name, and run info.
		// xxxx mountain name
		
		Text::DrawString(320, 50, Text::DEFAULT, Text::ALIGN_CENTER, GetRunName(GetCurrentRun()));

		Text::DrawMultiLineString(60, 100, Text::DEFAULT, Text::ALIGN_LEFT, 520, GetRunInfoText(GetCurrentRun()));

		ShowRewinds();
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// on ESCAPE, quit to to MAINMENU.
		if (code == UI::ESCAPE) {
			SetTimerActive(false);
			SetTimer(0);
			SetScore(0);
			
			UI::SetMode(UI::MAINMENU, Ticks);
		} else {
			// Anything else starts the run.
			UI::SetMode(UI::COUNTDOWN, Ticks);
		}
	}
} ShowRunInfoInstance;


// UI::COUNTDOWN handler.
class Countdown : public UI::ModeHandler {
	int	CountdownTicks;
	const char*	RunName;
	enum { READY, SET, GO } Phase;

public:
	Countdown() {
		CountdownTicks = 0;
		RunName = NULL;
		Phase = READY;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		CountdownTicks = 3000;

		RunName = GetRunName(GetCurrentRun());
		Phase = READY;
		
		// beep?  click?  voice?

		SetTimer(0);
		SetScore(0);
		SetTimerActive(false);

		// Put the user at the start of the run, and deactivate her.
		ResetRun(GetCurrentRun());
		SetUserActive(false);

		GameLoop::SetSpeedScale(0);

		// Turn it up, man!
		Music::FadeUp();

		// Ensure camera stuff is set up.
		SetChaseCameraParams();

		// Make sure we're out of any playback modes.
		Recording::SetMode(Recording::STOP, Ticks);

		// Reset the rewind count.
		Config::SetInt("RewindsAllowed", 3);

		HUDPlayer->setTime(0);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
//		SetUserActive(true);
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		int	OldTicks = CountdownTicks;
		CountdownTicks -= u.DeltaTicks;

		if (OldTicks >= 0 && CountdownTicks < 0) {
			// Done counting down, exit to PLAYING mode.
			UI::SetMode(UI::PLAYING, u.Ticks);
			
		} else if (OldTicks >= 1000 && CountdownTicks < 1000) {
			// Show "GO!"
			Phase = GO;

			// Start the user going.
			ResetRun(GetCurrentRun());
			SetUserActive(true);
			SetTimerActive(true);

			GameLoop::SetSpeedScale(1);
			
			// Start recording.
			Recording::SetChannel(Recording::PLAYER_CHANNEL);
			Recording::SetMode(Recording::STOP, u.Ticks);
			Recording::SetMode(Recording::RECORD, u.Ticks);
			
			Recording::SetScore(0);
			Recording::SetPlayerName(GetCurrentPlayer()->GetName());
			Recording::SetRunIndex(GetCurrentRun());
//			Console::Printf("Rec::SPN('%s')", GetCurrentPlayer()->GetName());//xxxxxxxxx
			
		} else if (OldTicks >= 2000 && CountdownTicks < 2000) {
			// Show "Set"
			Phase = SET;
			
			// sound?
		}
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Show run name.
		Text::DrawString(318, 102, Text::DEFAULT, Text::ALIGN_CENTER, RunName, 0x60202020);	// Drop shadow.
		Text::DrawString(320, 100, Text::DEFAULT, Text::ALIGN_CENTER, RunName);

		// Show prompt.
		const char*	prompt;
		if (Phase == READY) prompt = UI::String("ready_prompt", "Ready");
		else if (Phase == SET) prompt = UI::String("set_prompt", "Set");
		else if (Phase == GO) prompt = UI::String("go_prompt", "Go!");
		Text::DrawString(318, 152, Text::DEFAULT, Text::ALIGN_CENTER, prompt, 0x60202020);	// Drop shadow.
		Text::DrawString(320, 150, Text::DEFAULT, Text::ALIGN_CENTER, prompt);

		ShowScore();
		ShowRewinds();
		
//		// Show gauges.
//		ShowStability(s);
//		ShowFatigue(s);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// on ESCAPE, quit to MAINMENU.
		if (code == UI::ESCAPE) {
			SetTimerActive(false);
			SetTimer(0);
			SetScore(0);

			// Stop the recording.
			Recording::SetMode(Recording::STOP, Ticks);
			// Recording::Clear(); or something???
			
			UI::SetMode(UI::MAINMENU, Ticks);
		}
	}
} CountdownInstance;


// UI::RESUME handler.
class Resume : public UI::ModeHandler {
	int	CountdownTicks;
//	const char*	RunName;
	enum { READY, GO } Phase;

	int	ResumeTicks;
	
public:
	Resume() {
		CountdownTicks = 0;
//		RunName = NULL;
		Phase = READY;
		ResumeTicks = -1;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		CountdownTicks = 2000;

//		RunName = GetRunName(GetCurrentRun());
		Phase = READY;
		
		// beep?  click?  voice?

//		// Make sure the recording is not active.
//		Recording::SetMode(Recording::PAUSERECORD, Ticks);
		
		// Deactivate the user and put her back at a previous valid spot.
		SetUserActive(false);
		int	LostTicks = 0;
		UserType*	u = dynamic_cast<UserType*>(GetUser());
		if (u) {
			ResumeTicks = u->ResumeFromCheckpoint();
			LostTicks = Ticks - ResumeTicks;
		} else {
			ResumeTicks = -1;
		}

//		SetTimer(GetTimer() + 3000);
		SetTimerActive(false);

		Music::FadeDown();

		GameLoop::SetSpeedScale(0);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
//		// Resume recording.
//		Recording::MoveCursor(Ticks - PauseTicks);
//		Recording::SetMode(Recording::RECORD, GameLoop::GetCurrentTicks());
		
		SetUserActive(true);
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		int	OldTicks = CountdownTicks;
		CountdownTicks -= u.DeltaTicks;

		if (OldTicks >= 0 && CountdownTicks < 0) {
			// Done counting down, exit to PLAYING mode.
			UI::SetMode(UI::PLAYING, u.Ticks);
			
		} else if (OldTicks >= 1000 && CountdownTicks < 1000) {
			// Show "GO!"
			Phase = GO;

			// Start the user going.
//			ResetRun(GetCurrentRun());
			SetUserActive(true);
			SetTimerActive(true);

			GameLoop::SetSpeedScale(1);
			
			// Resume the recording right at the checkpoint.
			UserType*	user = dynamic_cast<UserType*>(GetUser());
			if (user && ResumeTicks >= 0) {
				user->ResumeFromCheckpoint();//xxxxxx
				user->TimestampCheckpoints(u.Ticks);
				Recording::SetResumePosition(ResumeTicks, u.Ticks);
				Recording::SetMode(Recording::RECORD, u.Ticks);
			}
		}
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
//		// Show run name.
//		Text::DrawString(320, 100, Text::DEFAULT, Text::ALIGN_CENTER, RunName);

		// Show prompt.
		const char*	prompt;
		if (Phase == READY) prompt = UI::String("get_ready_prompt", "Get Ready...");
		else if (Phase == GO) prompt = UI::String("go_prompt", "Go!");
		Text::DrawString(318, 152, Text::DEFAULT, Text::ALIGN_CENTER, prompt, 0x60202020);	// Drop shadow.
		Text::DrawString(320, 150, Text::DEFAULT, Text::ALIGN_CENTER, prompt);

		ShowScore();
		ShowRewinds();
//		// Show gauges.
//		ShowStability(s);
//		ShowFatigue(s);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// on ESCAPE, quit to MAINMENU.
		if (code == UI::ESCAPE) {
			SetTimerActive(false);
			SetTimer(0);
			SetScore(0);

			// Recording::Clear(); or something???
			
			UI::SetMode(UI::MAINMENU, Ticks);
		}
	}
} ResumeInstance;


// UI::PLAYING handler
class Playing : public UI::ModeHandler {
	int	ShowMouseTimer;
	float	MouseSpeedX, MouseSpeedY;
public:
	Playing() {
		ShowMouseTimer = 0;
		MouseSpeedX = MouseSpeedY = 0;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		ShowMouseTimer = 0;
		MouseSpeedX = MouseSpeedY = 0;

		SetUserActive(true);	// Just to be sure.

		GameLoop::SetSpeedScale(1);
		
		// Ensure camera stuff is set up.
		SetChaseCameraParams();

		Music::FadeUp();
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		if (ShowMouseTimer) {
			ShowMouseTimer -= u.DeltaTicks;
			if (ShowMouseTimer < 0) ShowMouseTimer = 0;
		}

		if (Config::GetBool("MouseSteering") 
			&& Config::GetBool("Fullscreen")
			&& (fabs(u.Inputs.MouseSpeedX) > 5.0 || fabs(u.Inputs.MouseSpeedY) > 5.0))
		{
			ShowMouseTimer = 1000;
		}
		
		MouseSpeedX = u.Inputs.MouseSpeedX;
		MouseSpeedY = u.Inputs.MouseSpeedY;

		if (Config::GetBool("MouseSteering")) {
			// Prevent the mouse pointer from escaping our window.
			Main::CenterMouse();
		}
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Timer.
		ShowScore();
		ShowRewinds();

//		ShowStability(s);
//		ShowFatigue(s);

		// Speedometer.
		if (Config::GetBool("Speedometer")) {
			MDynamic*	u = dynamic_cast<MDynamic*>(GetUser());
			if (u) {
				float	speed = u->GetVelocity().magnitude();
				char	buf[80];
				Text::FormatNumber(buf, speed * 2.216f, 3, 1);	// Show speed in MPH.
				Text::DrawString(600, 400, Text::DEFAULT, Text::ALIGN_RIGHT, buf);
			}
		}

		// Show Mouse motion.
		if (ShowMouseTimer) {
			float	MouseDivisor = 5 * expf((10 - fclamp(1, Config::GetFloat("MouseSteeringSensitivity"), 10)) * (5.298f / 9.0f));

			vec3	verts[9];
			verts[0] = vec3(290/320.0, -210/240.0, 0);
			vec3	v = verts[0] + vec3(fclamp(-1, MouseSpeedX/MouseDivisor, 1)/320.0f, fclamp(-1, -MouseSpeedY/MouseDivisor, 1)/240.0f, 0) * 15.0f;
			verts[1] = v + vec3(3/320.0f, 3/240.0f, 0);
			verts[2] = v + vec3(3/320.0f, -3/240.0f, 0);
			verts[3] = v + vec3(-3/320.0f, -3/240.0f, 0);
			verts[4] = v + vec3(-3/320.0f, 3/240.0f, 0);
			
			verts[5] = verts[0] + vec3(18/320.0f, 18/240.0f, 0);
			verts[6] = verts[0] + vec3(18/320.0f, -18/240.0f, 0);
			verts[7] = verts[0] + vec3(-18/320.0f, -18/240.0f, 0);
			verts[8] = verts[0] + vec3(-18/320.0f, 18/240.0f, 0);

			GameLoop::GUIBegin();

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);

			glBegin(GL_TRIANGLES);

			glColor4f(0, 0, 0, 0.3f);
			
			// Shaded background.
			glVertex3fv((const float*) verts[5]);
			glVertex3fv((const float*) verts[6]);
			glVertex3fv((const float*) verts[7]);

			glVertex3fv((const float*) verts[5]);
			glVertex3fv((const float*) verts[7]);
			glVertex3fv((const float*) verts[8]);

			glColor4f(0.1f, 0.3f, 0.1f, 1.0f);

			// Stick graphic.
			glVertex3fv((const float*) verts[0]);
			glVertex3fv((const float*) verts[2]);
			glVertex3fv((const float*) verts[1]);

			glVertex3fv((const float*) verts[0]);
			glVertex3fv((const float*) verts[2]);
			glVertex3fv((const float*) verts[3]);

			glVertex3fv((const float*) verts[0]);
			glVertex3fv((const float*) verts[3]);
			glVertex3fv((const float*) verts[4]);

			glVertex3fv((const float*) verts[0]);
			glVertex3fv((const float*) verts[1]);
			glVertex3fv((const float*) verts[4]);

			glColor3f(0.2f, 0.6f, 0.2f);

			glVertex3fv((const float*) verts[1]);
			glVertex3fv((const float*) verts[2]);
			glVertex3fv((const float*) verts[3]);

			glVertex3fv((const float*) verts[1]);
			glVertex3fv((const float*) verts[3]);
			glVertex3fv((const float*) verts[4]);

			glEnd();

			glDisable(GL_BLEND);

			GameLoop::GUIEnd();
		}
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// Go to the resume menu.
			Recording::SetScore(GetScore());
			Recording::SetMode(Recording::PAUSERECORD, Ticks);
//			Recording::MoveCursor(1000000);	// Go to the end of the recording, so we show the most recent state of the boarder.
			
			UI::SetMode(UI::CRASHQUERY, Ticks);
		}
	}
} PlayingInstance;


// UI::SHOWCRASH handler
class ShowCrash : public UI::ModeHandler {

public:
	ShowCrash() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		Music::FadeDown();

		SetTimerActive(false);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		Recording::SetScore(GetScore());
		Recording::SetMode(Recording::PAUSERECORD, GameLoop::GetCurrentTicks());
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Show crash message
		const char*	crash_msg = UI::String("crash_message", "CRASH");
		Text::DrawString(318, 102, Text::DEFAULT, Text::ALIGN_CENTER, crash_msg, 0x60202020);	// Drop shadow.
		Text::DrawString(320, 100, Text::DEFAULT, Text::ALIGN_CENTER, crash_msg);

		ShowScore();
		ShowRewinds();
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// On escape, skip to the crash query.
		if (code == UI::ESCAPE) {
			UI::SetMode(UI::CRASHQUERY, Ticks);
		}
	}
} ShowCrashInstance;


int	CrashPauseTicks = 0;
const float	RewindSpeed = 3.0;

// UI::SHOWREWIND handler
class ShowRewind : public UI::ModeHandler {
	int	RewindTicks;
	int	SumUpdateTicks;

	GG_Player*	VideoStreaks;
	
public:
	ShowRewind() {
		RewindTicks = 0;
		SumUpdateTicks = 0;

		VideoStreaks = NULL;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Decrement the rewinds allowed counter.
		Config::SetInt("RewindsAllowed", Config::GetInt("RewindsAllowed") - 1);

		Music::FadeDown();

		// Initialize the VideoStreaks movie if necessary.
		if (VideoStreaks == NULL) {
			GameLoop::AutoPauseInput	autoPause;
			GG_Movie*	m;
			GG_Rval	ret;
			ret = GUI->loadMovie("vcr_static.ggm", &m);
			if (ret != GG_OK) {
				Error e; e << "Can't load movie file 'vcr_static.ggm'.";
				throw e;
			}
			m->setPlayMode(GG_PLAYMODE_LOOP);
			GUI->createPlayer(m, &VideoStreaks);
			if (VideoStreaks == NULL) {
				Error e; e << "Couldn't create movie player from 'vcr_static.ggm'.";
				throw e;
			}
			
			VideoStreaks->setPlayMode(GG_PLAYMODE_LOOP);
			m->unRef();
		}
		
		// Set playback speed, figure out rewind ticks.
		UserType*	u = dynamic_cast<UserType*>(GetUser());
		if (u) {
			RewindTicks = CrashPauseTicks - u->GetResumeCheckpointTicks();
		} else {
			RewindTicks = 0;
		}
		RewindTicks = int(iclamp(0, RewindTicks, imin(12000, Recording::GetCursorTicks())) / RewindSpeed);
		
		SumUpdateTicks = 0;

		GameLoop::SetSpeedScale(-RewindSpeed);
	}

	void	Close()
	// Called by UI when exiting mode.
	{
//		Recording::SetMode(Recording::PAUSERECORD, GameLoop::GetCurrentTicks());

		GameLoop::SetSpeedScale(1.0);
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		// Run the recording backwards a bit...
		RewindTicks -= u.DeltaTicks;
		if (RewindTicks <= 0) {
			UI::SetMode(UI::RESUME, u.Ticks);

			Recording::MoveCursor(int(-(u.DeltaTicks + RewindTicks) * RewindSpeed));
		} else {
			Recording::MoveCursor(int(-u.DeltaTicks * RewindSpeed));
		}

		// Remember how many accumulated ticks have passed, in order to properly advance the video static overlay
		// during the Render() call.
		SumUpdateTicks += u.DeltaTicks;
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Show video streaks.
		GameLoop::GUIBegin();
		VideoStreaks->play(SumUpdateTicks);
		GameLoop::GUIEnd();
		SumUpdateTicks = 0;

		// Show rewinds & score.
		ShowRewinds();
		ShowScore();
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// On escape, skip to the crash query.
		if (code == UI::ESCAPE) {
			UI::SetMode(UI::RESUME, Ticks);
		}
	}
} ShowRewindInstance;


// UI::SHOWFINISH handler
class ShowFinish : public UI::ModeHandler {
	int	ShowFinishTicks;
	int	FinishTime;
public:
	ShowFinish() {
		ShowFinishTicks = 0;
		FinishTime = 0;
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		SetTimerActive(false);
		FinishTime = GetTimer();
		ShowFinishTicks = 2500;

		// Check for record time & acknowledge & register as appropriate...
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		Recording::SetScore(GetScore());
		Recording::SetMode(Recording::PAUSERECORD, GameLoop::GetCurrentTicks());	//xxx recscore
	}

	void	Update(const UpdateState& u)
	// Called every update.
	{
		// Decrement show-finish timer.
		ShowFinishTicks -= u.DeltaTicks;
		if (ShowFinishTicks <= 0) {
			ShowFinishTicks = 0;

			UI::SetMode(UI::FINISHTOTAL, u.Ticks);
		}
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Show finished message, and finish time.
		const char*	finished_message = UI::String("finished_message", "FINISHED");
		Text::DrawString(318, 102, Text::DEFAULT, Text::ALIGN_CENTER, finished_message, 0x60202020);	// Drop shadow.
		Text::DrawString(320, 100, Text::DEFAULT, Text::ALIGN_CENTER, finished_message);

		char	buf[80];
		Text::FormatTime(buf, FinishTime);
		Text::DrawString(318, 152, Text::DEFAULT, Text::ALIGN_CENTER, buf, 0x60202020);	// Drop shadow.
		Text::DrawString(320, 150, Text::DEFAULT, Text::ALIGN_CENTER, buf);

		ShowScore();
		ShowRewinds();
		
//		// Show stability gauge.
//		ShowStability(s);
//		ShowFatigue(s);
	}
	
	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		// On escape, skip to the finish query.
		if (code == UI::ESCAPE) {
			UI::SetMode(UI::FINISHTOTAL, Ticks);
		}
	}
} ShowFinishInstance;


// UI::FINISHTOTAL handler.  Add time bonus/penalty to score.
class FinishTotal : public UI::ModeHandler {
	enum PhaseType {
		SHOW_SCORESHEET,
		SHOW_PAR_TIME,
		SHOW_PLAYER_TIME,
		SHOW_DIFFERENCE,
		COUNTDOWN_DIFFERENCE,
		COUNTDOWN_REWINDS,
		PAUSE,
		WAIT_FOR_INPUT,

		PHASE_COUNT
	};
	
	int	CountdownTicks;
	PhaseType	Phase;
	int	DifferenceHundredths;
	bool	Penalty;
	int	TicksRemainder;

	int	ScoresheetID;
	int	ScoretextID;

	int	ResumeTicks;

	float	DingPitch;
	int	DingTimer;
	
public:
	FinishTotal() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Make sure the user is frozen.
		SetUserActive(false);

		GameLoop::SetSpeedScale(1);
		
		Music::FadeDown();

		Phase = SHOW_SCORESHEET;
		CountdownTicks = 2000;
		DifferenceHundredths = (GetRunParTicks(GetCurrentRun()) - GetTimer()) / 10;
		if (DifferenceHundredths < 0) {
			Penalty = true;
			DifferenceHundredths = -DifferenceHundredths;
		} else {
			Penalty = false;
		}
		TicksRemainder = 0;

		GameLoop::CacheMovie("scoresheet.ggm");
		GameLoop::CacheMovie("scoresheet_text.ggm");
		ScoresheetID = Overlay::PlayMovie("scoresheet.ggm", 0, GG_PLAYMODE_HANG);
		ScoretextID = 0;

		CountdownTicks = Overlay::GetMoviePlayer(ScoresheetID)->getDuration();

		ResumeTicks = Ticks;

		DingPitch = 1;
		DingTimer = 0;

		GG_Movie*	m = Overlay::GetMovie(ScoresheetID);
		if (m)
		{
			m->setActorText(GGID_SCORESHEET_CAPTION, UI::String("scoresheet_caption", "Scoresheet"));
		}
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		Overlay::StopMovie(ScoresheetID);
		if (ScoretextID) Overlay::StopMovie(ScoretextID);

		// Done recording, for sure.
		Recording::SetMode(Recording::STOP, GameLoop::GetCurrentTicks());
	}

	void	Render(const ViewState& s)
	{
		// Dim the background.
		Render::FullscreenOverlay(UI::BackgroundDim);

		ShowScore();
		ShowRewinds();

		GG_Movie*	m = Overlay::GetMovie(ScoretextID);

		char	buf[80];
		if (Phase >= SHOW_PAR_TIME && Phase <= SHOW_DIFFERENCE) {
			// Draw par time.
			Text::FormatTime(buf, GetRunParTicks(GetCurrentRun()));
			if (m) m->setActorText(GGID_PARTIME, buf);
		} else {
			if (m) m->setActorText(GGID_PARTIME, "");
		}
		if (Phase >= SHOW_PLAYER_TIME && Phase <= SHOW_DIFFERENCE) {
			// Draw player time.
			Text::FormatTime(buf, GetTimer());
			if (m) m->setActorText(GGID_YOURTIME, buf);
		} else {
			if (m) m->setActorText(GGID_YOURTIME, "");
		}
		if (Phase >= SHOW_DIFFERENCE && Phase <= COUNTDOWN_DIFFERENCE) {
			// Draw difference.
			Text::FormatTime(buf, DifferenceHundredths * 10);
			if (m) m->setActorText(GGID_DIFTIME, buf);
		} else {
			if (m) m->setActorText(GGID_DIFTIME, "");
		}

		if (m) {
			int	r = GetCurrentRun();

			// Personal best.
			Text::FormatNumber(buf, float(GetCurrentPlayer()->GetHighScore(r)), 5, 0);
			m->setActorText(GGID_YOURHIGHSCORE, buf);

			// Local high score.
			Text::FormatNumber(buf, float(HighScore::GetLocalHighScore(r)), 5, 0);
			m->setActorText(GGID_LOCALHIGHSCORE, buf);
			m->setActorText(GGID_LOCALHIGHPLAYER, (char*) HighScore::GetLocalHighPlayer(r));

//			// World high score.
//			Text::FormatNumber(buf, HighScore::GetWorldHighScore(r), 5, 0);
//			m->setActorText(GGID_WORLDHIGHSCORE, buf);
//			m->setActorText(GGID_WORLDHIGHPLAYER, (char*) HighScore::GetWorldHighPlayer(r));
		}
	}
		
	void	Update(const UpdateState& u)
	// Called every update.
	{
		if (Phase == SHOW_PAR_TIME && ScoretextID == 0) {
			// Show the score text movie.
			ScoretextID = Overlay::PlayMovie("scoresheet_text.ggm", 1, GG_PLAYMODE_HANG);

			GG_Movie*	m = Overlay::GetMovie(ScoretextID);
			m->setActorText(GGID_PARTIME_CAPTION, UI::String("par_time_caption", "Par Time:"));
			m->setActorText(GGID_YOURTIME_CAPTION, UI::String("your_time_caption", "Your Time:"));
			m->setActorText(GGID_DIFFERENCE_CAPTION, UI::String("difference_caption", "Difference"));
			m->setActorText(GGID_YOURHIGHSCORE_CAPTION, UI::String("your_high_caption", "Your High"));
			m->setActorText(GGID_LOCALHIGHSCORE_CAPTION, UI::String("local_high_caption", "Local High"));
		}
		
		if (Phase == COUNTDOWN_DIFFERENCE) {
			// Decrement the difference, while inc/decrementing the player's score.
			const int	POINTS_PER_FIFTH = 10;
			TicksRemainder += u.DeltaTicks;
			int	fifths = TicksRemainder / 25;
			TicksRemainder -= fifths * 25;
			
//			if (DifferenceHundredths < 0) inc = -inc;
//			else if (DifferenceHundredths == 0) inc = 0;

			if (fifths >= DifferenceHundredths / 20) {
				fifths = DifferenceHundredths / 20;
				DifferenceHundredths = 0;
				Phase = (PhaseType) (int(Phase) + 1);
			} else {
				DifferenceHundredths -= fifths * 20;
			}

			if (fifths) {
				HUDPlayer->setTime(0);	// Flash the HUD.
				AddScore((Penalty ? -fifths : fifths) * POINTS_PER_FIFTH);
			}

			// Some sfx to go with the timer countdown.
			DingTimer -= u.DeltaTicks;
			if (DingTimer <= 0) {
				while (DingTimer <= 0) DingTimer += 125;
				
				Sound::Play("ding1.wav", Sound::Controls(1, 0, DingPitch, false));
				if (Penalty) {
					// Drop one octave over 25 dings.
					DingPitch *= 0.972654947f;
				} else {
					// Rise one octave over 25 dings.  Very "Eastern Music".
					DingPitch *= 1.028113827f;
				}
				DingPitch = fclamp(0.5, DingPitch, 2);
			}
			
		} else if (Phase == COUNTDOWN_REWINDS) {
			int	rw = Config::GetInt("RewindsAllowed");
			if (rw <= 0) {
				// Done counting rewinds.
				Phase = (PhaseType) (int(Phase) + 1);

				// Set the total score in the recording.
				Recording::SetScore(GetScore());
				
				// Register the total score with the player and high-score modules.
				GetCurrentPlayer()->RegisterScore(GetCurrentRun(), GetScore());
				HighScore::RegisterScore(GetCurrentPlayer()->GetName(), GetCurrentRun(), GetScore());

				CountdownTicks = 150;	// Short PAUSE phase.
				
			} else {
				TicksRemainder += u.DeltaTicks;
				if (TicksRemainder > 500) {
					// Deduct a rewind and add points.
					TicksRemainder -= 500;
					HUDPlayer->setTime(0);	// Flash the HUD.
					AddScore(200);
					Config::SetInt("RewindsAllowed", rw - 1);
					// Make an "earcon" sound for rewind.
					Sound::Play("rewind.wav", Sound::Controls(1, 0, 1, false));
				}
			}
		} else if (Phase != WAIT_FOR_INPUT) {
			// Countdown the timer.  If the timer expires, reset it and advance to the next phase.
			CountdownTicks -= u.DeltaTicks;
			if (CountdownTicks <= 0) {
				CountdownTicks += 1000;
				Phase = (PhaseType) (int(Phase) + 1);
				if (Phase == COUNTDOWN_DIFFERENCE) {
					// Record the points increases/decreases.
					Recording::SetResumePosition(ResumeTicks, u.Ticks);
					Recording::SetMode(Recording::RECORD, u.Ticks);
				}
				if (Phase == WAIT_FOR_INPUT) {
					// Done recording, for sure.
					Recording::SetMode(Recording::STOP, GameLoop::GetCurrentTicks());
				}
				if (Phase == PHASE_COUNT) {
					UI::SetMode(UI::FINISHQUERY, u.Ticks);
				}
			}
		}
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			// xxx add the bonus to the score in a lump.
			UI::SetMode(UI::FINISHQUERY, Ticks);
		} else {
			if (Phase == WAIT_FOR_INPUT) {
				// We're outta here.
				UI::SetMode(UI::FINISHQUERY, Ticks);
				
			} else if (Phase > SHOW_SCORESHEET && Phase < COUNTDOWN_DIFFERENCE) {
				// Skip to countdown.
				Phase = COUNTDOWN_DIFFERENCE;
				CountdownTicks = 1000;
			}
		}
	}
} FinishTotalInstance;


// UI::FINISHQUERY handler.
class FinishQuery : public UI::ModeHandler {
	
public:
	FinishQuery() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Make sure the user is frozen.
		SetUserActive(false);

		GameLoop::SetSpeedScale(1);
		
		Music::FadeDown();
		
		// Set up menu.
		// "SAVE MOVIE" xxxx
		UI::SetElement(0, UI::ElementData(UI::String("restart_run", "Restart Run"), true));
		bool	AnotherRun = GetCurrentRun() < GetRunCount() - 1;
		if (AnotherRun) {
			UI::SetElement(1, UI::ElementData(UI::String("next_run", "Next Run"), true));
		} // else next mtn?
		UI::SetElement(2, UI::ElementData(UI::String("exit", "Exit"), true));

		if (AnotherRun) {
			UI::SetCursor(1);
		} else {
			UI::SetCursor(0);
		}
	}

	void	Close()
	// Called by UI when exiting mode.
	{
	}

	void	Render(const ViewState& s)
	{
		// Dim the background.
		Render::FullscreenOverlay(UI::BackgroundDim);

		ShowScore();
		ShowRewinds();
	}
		
	void	Update(const UpdateState& u)
	// Called every update.
	{
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE || ElementIndex == 2) {
			// User selected "Exit".  Back to main menu.
			UI::SetMode(UI::MAINMENU, Ticks);
		} else if (ElementIndex == 0) {
			// User selected "Restart Run".  Restart the level.
//			UI::SetMode(UI::COUNTDOWN, Ticks);
			UI::SetMode(UI::RUNFLYOVER, Ticks);
		} else if (ElementIndex == 1) {
			// User selected "Next Run".  Bump the current run index, and start.
			SetCurrentRun(GetCurrentRun() + 1);
			UI::SetMode(UI::RUNFLYOVER, Ticks);
		}
	}
	
} FinishQueryInstance;


// UI::CRASHQUERY handler.
class CrashQuery : public UI::ModeHandler {
public:
	CrashQuery() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering CRASHQUERY mode.
	{
		// Make sure the user is frozen.
		SetUserActive(false);
		SetTimerActive(false);
		CrashPauseTicks = Ticks;
		
		GameLoop::SetSpeedScale(0);

		// Set up the menu.
		bool	AllowRewind = Config::GetInt("RewindsAllowed") > 0;
		UI::SetElement(0, UI::ElementData(UI::String("rewind", "Rewind & Try Again"),
			       AllowRewind, AllowRewind ? 0xFFFFFFFF : 0xFF808080));
		UI::SetElement(1, UI::ElementData(UI::String("restart_run", "Restart Run"), true));
		UI::SetElement(2, UI::ElementData(UI::String("exit", "Exit"), true));
		UI::SetCursor((Config::GetInt("RewindsAllowed") > 0) ? 0 : 1);

		Music::FadeDown();
	}

	void	Close()
	// Called by UI when exiting CRASHQUERY mode.
	{
	}

	void	Render(const ViewState& s)
	{
		// Dim the background.
		Render::FullscreenOverlay(UI::BackgroundDim);

		ShowScore();
		ShowRewinds();
	}
		
	void	Update(const UpdateState& u)
	// Called every update.  No need to do anything.
	{
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE || ElementIndex == 2) {
			// User selected "Exit".  Go back to main menu.
			UI::SetMode(UI::MAINMENU, Ticks);
			
		} else if (ElementIndex == 0) {
			// User selected "Rewind & Try Again".  Restart the player from a prior valid spot.
			UI::SetMode(UI::SHOWREWIND, Ticks);
			
		} else if (ElementIndex == 1) {
			// User selected "Restart Run".  Restart the level.
//			UI::SetMode(UI::COUNTDOWN, Ticks);
			UI::SetMode(UI::RUNFLYOVER, Ticks);
		}
	}
	
} CrashQueryInstance;



// For static init.
static struct InitUIModes {
	InitUIModes() {
		GameLoop::AddInitFunction(Init);
	}
	static void	Init()
	// Attach the mode handlers to the UI module.
	{
		UI::RegisterModeHandler(UI::SHOWRUNINFO, &ShowRunInfoInstance);
		UI::RegisterModeHandler(UI::RUNFLYOVER, &RunFlyoverInstance);
		UI::RegisterModeHandler(UI::COUNTDOWN, &CountdownInstance);
		UI::RegisterModeHandler(UI::RESUME, &ResumeInstance);
		UI::RegisterModeHandler(UI::PLAYING, &PlayingInstance);
		UI::RegisterModeHandler(UI::SHOWCRASH, &ShowCrashInstance);
		UI::RegisterModeHandler(UI::SHOWREWIND, &ShowRewindInstance);
		UI::RegisterModeHandler(UI::SHOWFINISH, &ShowFinishInstance);
		UI::RegisterModeHandler(UI::FINISHTOTAL, &FinishTotalInstance);
		UI::RegisterModeHandler(UI::FINISHQUERY, &FinishQueryInstance);
		UI::RegisterModeHandler(UI::CRASHQUERY, &CrashQueryInstance);
		UI::RegisterModeHandler(UI::WAITING_FOR_OTHERS, &WaitingForOthersInstance);
	}
} InitUIModes;


};	// end namespace Game.

