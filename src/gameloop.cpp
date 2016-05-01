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
// gameloop.cpp	-thatcher 2/2/1998

// Runs the main game loop.


#include <math.h>
#include <assert.h>

#ifdef LINUX
#include <pthread.h>
#include <errno.h>
#include "linuxmain.hpp"
#else // not LINUX
#include <windows.h>	// For input thread stuff.
#include <winbase.h>
#include <process.h>
#include "winmain.hpp"
#endif // not LINUX

#include "ogl.hpp"
#include "timer.hpp"

#include "utility.hpp"
#include "main.hpp"

#include "gameloop.hpp"
#include "render.hpp"
#include "sound.hpp"
#include "input.hpp"
#include "model.hpp"
#include "game.hpp"
#include "text.hpp"
#include "console.hpp"

#include "terrain.hpp"
#include "surface.hpp"
#include "polygonregion.hpp"
#include "weather.hpp"
#include "config.hpp"
#include "ui.hpp"
#include "particle.hpp"
#include "music.hpp"
#include "overlay.hpp"
#include "imageoverlay.hpp"
#include "recording.hpp"
#include "boarder.hpp"
#include "gamegui/gg_audio.h"
#include "avi_tools.hpp"

#include "multiplayer.hpp"

//
// GameGUI stuff.
//


GameGUI*	GUI = NULL;	// Global pointer to GameGUI instance.
static GG_AudioSetup	GUIAudioSetup;
static GG_Audio*	GUIAudio = null;
static GG_Callbacks	GUICallbacks;

//  Sound loader callback, supplied to GameGUI
//
static GG_Rval loadSndCallback( void *appObj, char *fileName, void **dsSnd )
{
	GG_Audio *audio = (GG_Audio *)appObj;
	GG_Sound *snd = null;
	if( (snd = audio->createSound(fileName)) == null )
		return GG_ERR;
	*dsSnd = snd->getDSSoundBuffer1();
	
	//  Note that we didn't destroy the GG_Sound object we created here!
	//  Gonna let it slide, and allow the destruction of audio to take
	//  care of it in the main loop.
	
	return GG_OK;
}


//
// GameLoop stuff.
//


namespace GameLoop {
;


bool	IsOpen = false;

// "MovieMode" means we're showing an intro animation or cut-scene.
bool	MovieMode = false;

//bool	ChildThreadError = false;
//char	ChildThreadErrorMessage[300];

GG_Player*	MoviePlayer = NULL;
//GG_Player*	LoadingMoviePlayer = NULL;
//HGLRC	BackgroundOGLContext = 0;	// Rendering context used for background animation thread.
int	MovieDuration = 0;	// Length of intro movie, in milliseconds.


int	StartTicks = 0;
int	LastTicks = 0;
int	CurrentTicks = 0;

int	FrameNumber = 0;

MOriented*	Viewer = NULL;

float	SpeedScale = 1.0;	// For scaling dynamic effects like particle motion during recording playback.


bool	StopInputsThread = false;
int		PauseInputsThread = 0;
bool	InputsThreadActive = false;
unsigned long	ServiceInputsThread = (unsigned long) -1;

#ifdef LINUX
void*	ServiceInputsThreadFunction(void* dummy);
#else // not LINUX
void _cdecl	ServiceInputsThreadFunction(void* dummy);
#endif // not LINUX



const float	Z_MIN = 0.3f;
const float	Z_MAX = 30000.0f;
const float	TWO_PASS_CUTOFF = 500.0f;


int	InitFunctionCount = 0;
const int	MAX_INIT_FUNCTIONS = 100;
void	(*InitFunctions[MAX_INIT_FUNCTIONS])();


void	FinishOpening(void* dummy);
void	LoadInBackground(void* MountainName);
//void _cdecl	PlayIntroMovie(const char* MovieFile);


avi_tools::avi_stream*	RecordAVI = NULL;


void	Open()
// Initialize any internal stuff we need in order to run the game loop.
{
	if (IsOpen) return;

	// Simulation subsystems.
	Console::Open();
	Render::Open3D();
	Sound::Open();
	Music::Open();
	Input::Open();
	
	// Start timing.
	StartTicks = Timer::GetTicks();
	LastTicks = 0;
	
	//
	// Initialize GUI.
	//
#ifndef LINUX
	GUIAudioSetup.hWnd = Main::GetGameWindow();
	GUIAudioSetup.hInst = Main::GetAppInstance();

	if (Config::GetBoolValue("Sound") == true) {
		if (GG_AudioCreate(GUIAudioSetup, &GUIAudio) != GG_OK) {
			Error e; e << "Failed to create GG_Audio, hwnd = " << (int) GUIAudioSetup.hWnd << ", hinst = " << (int) GUIAudioSetup.hInst;
			throw e;
		}

		GUICallbacks.loadSound = loadSndCallback;
		GUICallbacks.loadSoundObj = GUIAudio;
	} else {
		memset(&GUICallbacks, 0, sizeof(GUICallbacks));
	}
#else // LINUX
	memset(&GUICallbacks, 0, sizeof(GUICallbacks));
#endif // LINUX
	
	
	// Create a GameGUI instance.
	GG_Rect	r;
	r.set(0, 0, Render::GetWindowWidth(), Render::GetWindowHeight());
	GameGUICreate(&r, &GUICallbacks, 0 /* | GG_FLAGS_GRAPHICS | GG_FLAGS_SOUND */, &GUI);
	
	GG_File::setCurrentPath("gui" PATH_SEPARATOR);

	// Show a quick pre-intro animation.
	PlayShortMovie("preintro.ggm");
	
	// Start input thread.
	StopInputsThread = false;
	AutoPauseInput	autoPause;	// Pause inputs through the remainder of this function.
#ifdef LINUX
	pthread_attr_t	Attr;
	pthread_attr_init(&Attr);
	pthread_t	InputThreadInfo;
	ServiceInputsThread = pthread_create(&InputThreadInfo, &Attr, ServiceInputsThreadFunction, NULL);
	if (ServiceInputsThread != 0) {
		Error e; e << "Can't create thread to poll input.";
		throw e;
	}
#else // not LINUX
	ServiceInputsThread = _beginthread(ServiceInputsThreadFunction, 0, 0);
	if (ServiceInputsThread == -1) {
		Error e; e << "Can't create thread to poll input.";
		throw e;
	}
#endif // no LINUX

	UI::Open();
	Model::Open();
	Game::Open();
	Overlay::Open();
	ImageOverlay::Open();
	Text::Open();
	
	TerrainModel::Open();
	TerrainMesh::Open();
	PolygonRegion::Open();
	Surface::Open();
	
	Particle::Open();
	Weather::Open();

	Recording::Open();
	
	// Call init functions.
	int	i;
	for (i = 0; i < InitFunctionCount; i++) {
		(*InitFunctions[i])();
	}
	
	// Load a test mountain.
	const char*	mtn = Config::GetValue("DefaultMountain");
	Game::LoadMountain(mtn);

	IsOpen = true;
}


void	PauseInputs()
// Pause the input thread.  Remember to un-pause the thread when
// you're done!  See AutoPauseInput for a helper.
{
	PauseInputsThread++;
}


void	UnpauseInputs()
// Counterpart to PauseInputs().
{
	PauseInputsThread--;
	assert(PauseInputsThread >= 0);
}


void	CueMovie(const char* MovieFile)
// Enter movie mode, load the named movie, and cue it for playing by
// the main loop.
{
//		// Load a spinning-disk movie or something, to play while levels are loading.
//		if (LoadingMoviePlayer == NULL) {
//			GG_Movie*	movie;
//			GUI->loadMovie("diskicon.ggm", &movie);
//			movie->setPlayMode(GG_PLAYMODE_LOOP);
//			GUI->createPlayer(movie, &LoadingMoviePlayer);
//			LoadingMoviePlayer->setPlayMode(GG_PLAYMODE_LOOP);
//			movie->unRef();
//		}

	// Load and set up the movie.
	GG_Movie*	movie;
	GG_Rval	ret;
	ret = GUI->loadMovie((char*) MovieFile, &movie);
	if (ret != GG_OK) {
		Error e; e << "Can't load movie file '" << MovieFile << "'.";
		throw e;
	}
	movie->setPlayMode(GG_PLAYMODE_HANG);
	GUI->createPlayer(movie, &MoviePlayer);
	if (MoviePlayer == NULL) {
		Error e; e << "Couldn't create movie player from '" << MovieFile << "'.";
		throw e;
	}
	
	MoviePlayer->setPlayMode(GG_PLAYMODE_HANG);
	movie->unRef();
	
	MovieDuration = MoviePlayer->getDuration();
	MovieMode = true;
}


static BagOf<GG_Movie*>	s_cachedMovies;


void	CacheMovie(const char* file)
// Load the movie and put it in a cache, so it doesn't need to be
// loaded from disk later.  Cached movies can be used by
// GameLoop::LoadMovie().
{
	GG_Movie*	movie = NULL;

	// Is this movie already in cache?
	if (s_cachedMovies.GetObject(&movie, file))
	{
		// No need to cache it again.
		return;
	}

	// Not in cache.

	// Load it.
	GG_Rval	ret = GUI->loadMovie((char*) file, &movie);
	if (ret != GG_OK) {
		Error e; e << "Can't load movie file '" << file << "'.";
		throw e;
	}

	// Cache it.
	s_cachedMovies.Add(movie, file);
}


GG_Player*	LoadMovie(const char* file)
// Loads a movie from the specified file and returns a player for
// playing it.  Returns the movie in *movie if it's not NULL; otherwise
// creates a temporary movie and discards it before returning.  If movie
// is not NULL, the caller is responsible for calling *movie->unRef() to
// release the movie.
{
	GameLoop::AutoPauseInput	autoPause;

	GG_Movie*	m;
	GG_Player*	p = NULL;
	GG_Rval	ret;

	// Check to see if this movie is cached.
	bool	cached = s_cachedMovies.GetObject(&m, file);
	if (cached == false)
	{
		// Movie is not cached; need to load it.
		ret = GUI->loadMovie((char*) file, &m);
		if (ret != GG_OK) {
			Error e; e << "Can't load movie file '" << file << "'.";
			throw e;
		}
	}

	GUI->createPlayer(m, &p);
	p->setPlayMode(GG_PLAYMODE_LOOP);

	if (cached == false)
	{
		// We don't hang on to this movie.
		m->unRef();
	}

	return p;
}


void	PlayShortMovie(const char* MovieFile)
// Play the specified animation right now, block until it finishes, and then
// return.  Don't use this to play anything long, because it blocks event
// processing until the animation is through.
{
	AutoPauseInput	autoPause;

	// Load and set up the movie.
	GG_Movie*	movie;
	GG_Rval	ret;
	ret = GUI->loadMovie((char*) MovieFile, &movie);
	if (ret != GG_OK) {
		Error e; e << "Can't load movie file '" << MovieFile << "'.";
		throw e;
	}

	PlayShortMovie(movie);
	movie->unRef();
}


void	PlayShortMovie(GG_Movie* movie)
// Play the specified animation, blocking until it finishes.
{
	GG_Player*	player;
	movie->setPlayMode(GG_PLAYMODE_HANG);
	GUI->createPlayer(movie, &player);
	player->setPlayMode(GG_PLAYMODE_HANG);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	// Play through the movie.
	int	duration = player->getDuration();
	int	last = Timer::GetTicks();
	while (duration > 0) {
		int	ticks = Timer::GetTicks();
		int	dt = ticks - last;
		last = ticks;

		// Do a frame of the movie.
		glViewport(0, 0, Render::GetWindowWidth(), Render::GetWindowHeight());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		player->play(dt);
//		OGL::SwapBuffers(GetDC(Main::GetGameWindow()));
		Render::ShowFrame();

		duration -= dt;
		
		// Sleep approximately until the next frame is due.
		int	SleepTime = 13 - dt;
		if (SleepTime > 0) {
			Timer::Sleep(SleepTime);
		}
	}

	// Delete the movie.
	player->unRef();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


void	GUIBegin()
// Pushes OpenGL state required for rendering GUI movies.  You should
// bracket GG_Player::play() calls with GUIBegin()/GUIEnd() calls to
// make sure the GUI stuff renders as intended.
{
//	glDisable(GL_DEPTH_TEST);
//	glDepthMask(GL_TRUE);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	Render::DisableZBuffer();
	Render::SetTexture(NULL);
	Render::CommitRenderState();
}


void	GUIEnd()
// Pops the state pushed by GUIBegin().
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


#ifdef NOT


void	LoadMountainInBackground(const char* MountainName, const char* MovieName)
// Clears the current mountain and starts a separate thread to load the
// specified mountain.  Meanwhile, uses the main thread to show the
// specified movie at the same time.  Returns immediately; the movie is
// updated via GameLoop::Update().
{
	// Start playing the movie.
	PlayIntroMovie(MovieName);

	// Initiate thread to clear the current mountain and load the new one.
	unsigned long	thread = _beginthread(LoadInBackground, 0, (void*) MountainName);
	if (thread == -1) {
		Error e; e << "Can't create thread to load mountain.";
		throw e;
	}
}


void _cdecl	LoadInBackground(void* MountainName)
// Thread function which clears the current mountain database and loads
// the specified mountain to replace it.
{
	try {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
		
		// Make sure main OGL context is current.
//		OGL::MakeCurrent(GetDC(Main::GetGameWindow()), Render::GetMainContext());
		HGLRC	glrc = OGL::CreateContext(GetDC(Main::GetGameWindow()));
		bool	ret = OGL::ShareLists(Render::GetMainContext(), glrc);
		if (ret == false) {
			Error e; e << "OGL::ShareLists() failed.";
			throw e;
		}
		OGL::MakeCurrent(GetDC(Main::GetGameWindow()), glrc);
		
		Game::ClearMountain();
		Game::LoadMountain((const char*) MountainName);
		
		OGL::MakeCurrent(NULL, NULL);	// Release main rendering context.
		OGL::DeleteContext(glrc);
		
		// Signal to main thread that we're done.
		IntroExitOK = true;
	}
	catch (Error& e) {
		strcpy(ChildThreadErrorMessage, e.GetMessage());
		ChildThreadError = true;
	}
	catch (...) {
		strcpy(ChildThreadErrorMessage, "Unknown exception caught in FinishOpening().");
		ChildThreadError = true;
	}
}
#endif // NOT

	
void	AddInitFunction(void (*InitFct)())
// Adds a function to be called once, after initializing the game subsystems.
{
	if (InitFunctionCount >= MAX_INIT_FUNCTIONS) {
		Error e; e << "InitFunctionCount exceeded.";
		throw e;
	}

	// Add the given function to the list.
	InitFunctions[InitFunctionCount] = InitFct;
	InitFunctionCount++;
}


void	Close()
// Do any needed cleanup.
{
	if (IsOpen) {
		// Close the AVI if we're writing one.
		if (RecordAVI) {
			RecordAVI->close();
			fclose(RecordAVI->fp);
			delete RecordAVI;
			RecordAVI = NULL;
		}
		
		// Stop input thread.
		StopInputsThread = true;

		// Wait for inputs thread to end itself.
		int	Timeout = 20;
		while (InputsThreadActive) {
			// Wait a bit.
			Timer::Sleep(50);
			if (Timeout-- <= 0) {
				// We can't wait forever.
				break;
			}
		}

		if (MoviePlayer) {
			MoviePlayer->unRef();
			MoviePlayer = NULL;
		}
		
		// Delete the background OpenGL context.
//		OGL::DeleteContext(IntroOGLContext);
//		if (BackgroundOGLContext) {
//			OGL::DeleteContext(BackgroundOGLContext);
//		}
		
		// Close subsystems.

		Recording::Close();
		
		Weather::Close();
		Particle::Close();

		Surface::Close();
		PolygonRegion::Close();
		TerrainMesh::Close();
		TerrainModel::Close();
		
		Text::Close();
		ImageOverlay::Close();
		Overlay::Close();
		Game::Close();
		Model::Close();
		UI::Close();
		
		Input::Close();
		Music::Close();
		Sound::Close();
		Render::Close3D();

		Console::Close();
		
		InitFunctionCount = 0;

		// Unreference the GUI instance.
		GUI->unRef();
		GUI = NULL;

		// Unreference the GUI audio instance.
		if (GUIAudio != null) {
			GUIAudio->unRef();
			GUIAudio = null;
		}

		IsOpen = false;
	}
}


bool	GetIsOpen()
// Returns true if this module is open.
{
	return IsOpen;
}


bool	GetIsInMovieMode()
// Returns true if we're in movie mode.
{
	return MovieMode;
}


void	SetViewer(MOriented* NewViewer)
// Sets the object to use as the viewer for rendering.
{
	Viewer = NewViewer;
}


float	GetSpeedScale()
// Returns a scale factor to apply to delta-time for rendering dynamic
// effects (e.g. snow falling).  For the benefit of recordings.
{
	return SpeedScale;
}


void	SetSpeedScale(float scale)
// For changing the speed at which dynamic effects are updated.  Used
// during playback of recordings to scale snow falling etc properly
// during e.g. slow-motion.
{
	SpeedScale = scale;
}


bool	RecordPath = false;	// To enable/disable path recording.


static void	ViewpointPathUpdate(const UpdateState& u, MOriented* o)
// Records a viewpoint path to disk.
{
	// PerfTest path recording.
	if (RecordPath) {
		// For demo path recording.
		static int	RecordPathPointCount = 0;
		static int	NextPointTicks = 0;
		static FILE*	PathFile = NULL;
		
		const int	PathPointCount = 450;
		if (NextPointTicks == 0) {
			// Initialize path recording.
			NextPointTicks = u.Ticks + 1000;
			PathFile = Utility::FileOpen("PerfTestPath.dat", "wb");
			if (PathFile == NULL) {
				Error e; e << "Can't open PerfTestPath.dat for output";
				throw e;
			}
			int	temp = PathPointCount;
			fwrite(&temp, sizeof(int), 1, PathFile);	// point count.
		}
		if (u.Ticks >= NextPointTicks) {
			// Record a point.
			vec3	v = o->GetLocation();
			fwrite(&v, sizeof(vec3), 1, PathFile);
			quaternion	q = o->GetOrientation();
			fwrite(&q, sizeof(quaternion), 1, PathFile);
			
			NextPointTicks += 100;	// 10 Hz.
			RecordPathPointCount++;
			if (RecordPathPointCount >= PathPointCount) {
				RecordPath = false;
				fclose(PathFile);
			}
		}
	}
}		


static void	DoUpdates(const UpdateState& u)
// Runs the update functions using the given update state.
{

	if (GetIsInMovieMode()) return;

	// Command-line console.
	Console::Update(u);

	if (Console::IsActive() == false) {
		if (Recording::GetMode() == Recording::RECORD) {
			// If we're in record mode, insert a timestamp for this frame.
			Recording::InsertTimestamp(u.Ticks);
		}
		
		// Deal with background music.
		Music::Update(u);
		
		// Run UI update.
		UI::Update(u);
		
		// If we switched to intro mode then don't continue doing updates.
		if (GetIsInMovieMode()) return;
	
		// Run game logic (course timers, etc.)
		Game::Update(u);
		
		// Run object updates (physics & behavior).
		Model::Update(u);
		
		// Update ballistic particles -- snow kicked up by the board, etc.
		Particle::Update(u);
		
		// Update weather -- make the snow fall, etc.
		Weather::Update(u);
		
		// Surface module.
		Surface::Update(u);
		
		// Deal with network?

		// Overlay module.
		Overlay::Update(u);
		
		// Deal with viewpoint path recording.
		ViewpointPathUpdate(u, Viewer);
		
		//xxxxxxx
		if (Input::GetControlKeyState() && Input::CheckForEventDown(Input::F4)) Config::SetBoolValue("F4Pressed", true);
		//xxxxxxxxxx
		
		//xxxxxxx
		if (Input::GetControlKeyState() && Input::CheckForEventDown(Input::F5)) Config::Toggle("Snowfall");
		//xxxxxxxxxx

		//xxxxxxx
		if (Input::CheckForEventDown(Input::F5)) Config::Toggle("RecordMoviePause");
		//xxxxxxx
	}
	
	// Notify the input module that we're done with this frame's events.
	Input::EndFrameNotify();
}


int	GetFrameNumber()
// Returns the current frame number.
{
	return FrameNumber;
}


int	GetCurrentTicks()
// Returns the current ticks value.
{
	return CurrentTicks;
}


void	OGLFrustum(float width, float height, float nearz, float farz)
// Multiplies the current matrix by Soulride's idea of a projection
// matrix.  Soulride's eye coordinates assume a right handed system with
// z going *into* the screen, which is like OpenGL except rotated 180
// degrees around the y axis, or with x and z coords negated.
{
	float	m[16];

	int	i;
	for (i = 0; i < 16; i++) m[i] = 0;

	m[0] = - 2 * nearz / width;
	m[5] = 2 * nearz / height;
	m[10] = (farz + nearz) / (farz - nearz);
	m[11] = 1;
	m[14] = - 2 * farz * nearz / (farz - nearz);
	
	glMultMatrixf(m);
}


void	OGLViewMatrix(const vec3& dir, const vec3& up, const vec3& loc)
// Multiplies the current OpenGL matrix by the view matrix corresponding to the given parameters.
{
	matrix m;
	vec3 Z = dir.cross(up);
	m.SetColumn(0, -Z);
	m.SetColumn(1, up);
	m.SetColumn(2, dir);
	m.SetColumn(3, loc);
	m.Invert();
	float	mat[16];

	// Copy to the 4x4 layout.
	for (int col = 0; col < 4; col++) {
		for (int row = 0; row < 3; row++) {
			mat[col * 4 + row] = m.GetColumn(col).Get(row);
		}
		if (col < 3) {
			mat[col * 4 + 3] = 0;
		} else {
			mat[col * 4 + 3] = 1;
		}
	}

	// Apply to the current OpenGL matrix.
	glMultMatrixf(mat);
}


// A queue for recording recent frame durations, for computing min/max/average
// frame rates.
const int	FT_SAMPLES = 200;
struct FrameTimeSample {
	int	Timestamp;
	int	FrameTime;
	FrameTimeSample() { Timestamp = 0; FrameTime = 0; }
} FrameTimeQueue[FT_SAMPLES];
int	NextFTSample = 0;


int	MovieFrameNumber = 0;	// For recording a series of screen shots to make a movie.


void	SaveMovieFrame()
// Save a frame to a movie file.
{
	if (Config::GetBool("RecordMoviePause")) return;
	
	int	w = Render::GetWindowWidth();
	int	h = Render::GetWindowHeight();
	
	if (RecordAVI == NULL) {
		// Open the movie file and create our avi_stream object.
		FILE*	fp = fopen(Config::GetValue("RecordMoviePath"), "wb");
		if (fp) {
			RecordAVI = new avi_tools::avi_stream();
			RecordAVI->open_video(fp, w, h, 33333);
		} else {
			Error e;
			e << "Couldn't open movie file " << Config::GetValue("RecordMoviePath");
			throw e;
		}
	}

	if (RecordAVI) {
		uint32*	pixels = Render::GetScreenPixels();
		RecordAVI->write_video_frame(pixels, w, h);
	}
	
	MovieFrameNumber++;
}


void	Update()
// Runs one iteration of the game loop.
{
	CurrentTicks = Timer::GetTicks() - StartTicks;
	int	DeltaTicks = CurrentTicks - LastTicks;

	// Force DeltaTicks to a fixed value if we're making a movie.
	if (Config::GetValue("RecordMoviePath") /*&& Config::GetBool("RecordMoviePause") == false*/ ) {
		DeltaTicks = 33;
		if (MovieFrameNumber % 3 == 0) DeltaTicks++;	// Exactly 30 frames/sec.

		// Monkey with the time base so that CurrentTicks
		// increments at 30 frames/sec in sync with DeltaTicks.
		// This is so code which samples CurrentTicks will behave
		// properly during movie recording.
		CurrentTicks = LastTicks + DeltaTicks;
//		StartTicks = (Timer::GetTicks() - CurrentTicks);
	}
	
	if (MovieMode == true) {
		LastTicks = CurrentTicks;

		if (DeltaTicks > 200) DeltaTicks = 200;
		
		//
		// Movie mode.
		//
		// Don't do world update and rendering.  Just show the movie, until
		// it's done, or until the user escapes past the movie.

		// Check for user escape.
		int	UpdateCount = 0;	// For protection against update functions taking longer than the sampling period.
		UpdateState	u;
		while (UpdatesPending() && UpdateCount++ < 30) {
			GetNextUpdateState(&u);
		}
		if (Input::CheckForEventDown(Input::BUTTON0) ||
		    Input::CheckForEventDown(Input::ENTER) ||
		    Input::CheckForEventDown(Input::ESCAPE) ||
		    Input::CheckForEventDown(Input::BUTTON2) ||
		    Config::GetBoolValue("SkipIntro"))
		{
			// Cut the movie short.
			MovieDuration = 0;
		}
		Input::EndFrameNotify();
		
		// Show a new frame.
		glViewport(0, 0, Render::GetWindowWidth(), Render::GetWindowHeight());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (MoviePlayer) MoviePlayer->play(DeltaTicks);

		if (Config::GetValue("RecordMoviePath")) SaveMovieFrame();
		
		Render::ShowFrame();

		MovieDuration -= DeltaTicks;

		if (MovieDuration <= 0) {
			// Done with the movie, now exit movie mode.
			MovieDuration = 0;

			if (MoviePlayer) {
				MoviePlayer->unRef();
				MoviePlayer = NULL;
			}

			MovieMode = false;
		}

		// Sleep a little.
		int	SleepTime = 13 - DeltaTicks;
		if (SleepTime > 0) {
			Timer::Sleep(SleepTime);
		}
		
		return;
	}

	//
	// Update the game logic & physics.
	//

	UpdateState	u;
	
	if (Config::GetValue("RecordMoviePath") != NULL) {
		// Special case -- simulate a fixed update rate.

		// Consume input.
		do {
			GetNextUpdateState(&u);
		} while (UpdatesPending());

		u.DeltaTicks = DeltaTicks;	// Force frame time.
		u.Ticks = CurrentTicks;
		DoUpdates(u);
			
	} else {
		// Normal case -- consume input info and update in real-time.
		int	UpdateCount = 0;	// For protection against update functions taking longer than the sampling period.
		while (UpdatesPending() && UpdateCount++ < 30) {
			GetNextUpdateState(&u);
			DoUpdates(u);
		}
		if (!Config::GetBoolValue("LimitUpdateRate")) {
			UpdateCount++;
			GetNextUpdateState(&u);
			
			//xxxxxx
			Config::SetBoolValue("LastUpdateBeforeRender", true);
			//xxxxx
			
			DoUpdates(u);
		} else {
			if (UpdateCount) {
				Config::SetBoolValue("LastUpdateBeforeRender", true);
			}
		}
		
		if (UpdateCount == 0) {
			return;
		}
	}
	
	LastTicks = CurrentTicks;

	// Don't try to render if we've entered movie mode during the updates.
	if (MovieMode) return;

	// Don't render if we're about to quit.
	if (Main::GetQuit()) return;
	
	// Render visuals.

	int number_of_players = MultiPlayer::NumberOfLocalPlayers();
	
	int window_corner_x[4];
	int window_corner_y[4];
	int window_size_x[4];
	int window_size_y[4];
	
	SetWindowCoordinates(number_of_players, window_corner_x, window_corner_y, window_size_x, window_size_y);

	// For each player, draw graphics
	for (int player_index = 0; player_index < number_of_players; player_index++){

	  MultiPlayer::SetCurrentPlayerIndex(player_index);
	  
	  ViewState	s;


	  // Increment the frame number.
	  //if (player_index == 0)
	  FrameNumber++;
	  s.FrameNumber = FrameNumber;
	  
	  // Set the time value.
	  s.Ticks = u.Ticks;
	
	float	ViewAngle = Config::GetFloatValue("ViewAngle") * (PI / 180);

	// Get viewer orientation.
	vec3	ViewerDir = XAxis;
	vec3	ViewerUp = YAxis;
	vec3	ViewerLoc = ZeroVector;
	if (Viewer) {
		ViewerDir = Viewer->GetDirection();
		ViewerUp = Viewer->GetUp();
		ViewerLoc = Viewer->GetLocation();

		if (ViewerDir.checknan() || ViewerUp.checknan()) {
			// Trouble.  Fall back to a default orientation.
			ViewerDir = XAxis;
			ViewerUp = YAxis;
		}
		if (ViewerLoc.checknan()) {
			// Trouble.  Fall back to a default location.
			ViewerLoc = ZeroVector;
		}
	}

	// Establish transformation from world coordinates to view coordinates.
	s.CameraMatrix.View(ViewerDir, ViewerUp, ViewerLoc);

	s.ViewMatrix = s.CameraMatrix;
	s.Viewpoint = ViewerLoc;
	s.MinZ = 1;
	s.MaxZ = Z_MAX;
	s.OneOverMaxZMinusMinZ = 1.0f / (s.MaxZ - s.MinZ);

	// Set the clipping planes.
//	s.ClipPlaneCount = 0;

	// Near.
	s.ClipPlane[0].Normal = ZAxis;
	s.ClipPlane[0].D = s.MinZ;
//	s.ClipPlaneCount++;

	// Left.
	s.ClipPlane[1].Normal = vec3(-cosf(ViewAngle/2), 0, sinf(ViewAngle/2));
	s.ClipPlane[1].D = 0;
//	s.ClipPlaneCount++;
	
	// Right.
	s.ClipPlane[2].Normal = vec3(cosf(ViewAngle/2), 0, sinf(ViewAngle/2));
	s.ClipPlane[2].D = 0;
//	s.ClipPlaneCount++;

//	float	AspectRatio = float(Render::GetWindowHeight()) / float(Render::GetWindowWidth() * 2.0); // Bjorn
//	float	VerticalAngle2 = atanf(tanf(ViewAngle/2) * AspectRatio);
	float	AspectRatio = float(window_size_y[player_index]) / float(window_size_x[player_index]); // Bjorn
	float	VerticalAngle2 = atanf(tanf(ViewAngle/2) * AspectRatio);

	// Top.
	s.ClipPlane[3].Normal = vec3(0, -cosf(VerticalAngle2), sinf(VerticalAngle2));
	s.ClipPlane[3].D = 0;
//	s.ClipPlaneCount++;
	
	// Bottom.
	s.ClipPlane[4].Normal = vec3(0, cosf(VerticalAngle2), sinf(VerticalAngle2));
	s.ClipPlane[4].D = 0;
//	s.ClipPlaneCount++;
	
	// Far.
	s.ClipPlane[5].Normal = -ZAxis;
	s.ClipPlane[5].D = -s.MaxZ;
//	s.ClipPlaneCount++;

	// Set the projection factors so the view volume fills the screen.
	s.XProjectionFactor = (Render::GetWindowWidth() - 0.6f) / 2 / tanf(ViewAngle/2);
	s.YProjectionFactor = s.XProjectionFactor;

	// Set the x/y offsets so that 0,0 appears in the middle of the screen.
	s.XOffset = (Render::GetWindowWidth() + 1) * 0.5f;
	s.YOffset = (Render::GetWindowHeight() + 1) * 0.5f;

	// Only clear the screen the first time each frame
	if (player_index == 0){
	  Render::BeginFrame();
	  Render::ClearFrame();
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	

	// Set the viewport (split the screen)
	glViewport(window_corner_x[player_index], 
		   window_corner_y[player_index], 
		   window_size_x[player_index], 
		   window_size_y[player_index]);
	
	// Set up OpenGL matrices.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float	w = 2 * s.MinZ * tanf(ViewAngle/2);
	float	h = w * AspectRatio;
	OGLFrustum(w, h, s.MinZ, s.MaxZ);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	OGLViewMatrix(ViewerDir, ViewerUp, ViewerLoc);

	// Update the terrain.
	TerrainMesh::Update(s);

	// Set up fog parameters.
	if (Config::GetBoolValue("Fog")) {
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_LINEAR /* GL_EXP */);
		glFogf(GL_FOG_START, 0);
		glFogf(GL_FOG_END, Weather::GetFadeDistance());	// For GL_LINEAR mode.
//		glFogf(GL_FOG_DENSITY, 4.852 / Weather::GetFadeDistance());		// For GL_EXP mode; meaningless for GL_LINEAR mode.
		glHint(GL_FOG_HINT, GL_NICEST);
		glFogfv(GL_FOG_COLOR, Weather::GetFadeColor());
	}

	// Determine far clipping plane, and decide whether or not to do two passes.
	float	MaxZ = fmin(Z_MAX, Weather::GetFadeDistance());
	bool	TwoPass = true;	// Should set to false if we have a 24- or 32-bit z-buffer...
	if (MaxZ < TWO_PASS_CUTOFF) TwoPass = false;
	if (TwoPass && Config::GetBoolValue("ZSinglePass")) TwoPass = false;	// Manual override, force single pass.
	
	float	MidZ = MaxZ;

	if (TwoPass) {
		//
		// Draw the distant part of the world.
		//

		// Pick a z value to separate the near and far passes, in order to optimize z-buffer usage.
		MidZ = sqrtf(Z_MIN * Z_MAX);

		s.MaxZ = MaxZ;
		s.MinZ = MidZ * 0.9f;
		s.OneOverMaxZMinusMinZ = 1.0f / (s.MaxZ - s.MinZ);
		// Near.
		s.ClipPlane[0].Normal = ZAxis;
		s.ClipPlane[0].D = s.MinZ;
		
		// Far.
		s.ClipPlane[5].Normal = -ZAxis;
		s.ClipPlane[5].D = -s.MaxZ;
		
		// Set up OpenGL matrices.
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		w = 2 * s.MinZ * tanf(ViewAngle/2);
		h = w * AspectRatio;
		OGLFrustum(w, h, s.MinZ, s.MaxZ);
		
		glMatrixMode(GL_MODELVIEW);
//		glLoadIdentity();
//		OGLViewMatrix(Viewer->GetDirection(), Viewer->GetUp(), Viewer->GetLocation());
		
		// Bjorn : moved this for sky to work when having two players
		// Draw a backdrop.
		glDisable(GL_FOG);
		Weather::RenderBackdrop(s);
		
		TerrainMesh::Render(s);
		
		Model::Render(s);
	}

	//
	// Draw closer part of terrain.
	//
	
	Render::ClearZBuffer();

	s.MaxZ = MidZ;
	s.MinZ = Z_MIN;
	s.OneOverMaxZMinusMinZ = 1.0f / (s.MaxZ - s.MinZ);
	// Near.
	s.ClipPlane[0].Normal = ZAxis;
	s.ClipPlane[0].D = s.MinZ;

	// Far.
	s.ClipPlane[5].Normal = -ZAxis;
	s.ClipPlane[5].D = -s.MaxZ;
	
	// Set up OpenGL matrices.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	w = 2 * s.MinZ * tanf(ViewAngle/2);
	h = w * AspectRatio;
	OGLFrustum(w, h, s.MinZ, s.MaxZ);
	
	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	OGLViewMatrix(Viewer->GetDirection(), Viewer->GetUp(), Viewer->GetLocation());
	
	TerrainMesh::Render(s);

	// Draw the objects.
	Model::Render(s);

	//
	// Overlay effects.
	//
	
	// Turn off the fog for overlay stuff.
	glDisable(GL_FOG);
	
	if (Config::GetBoolValue("BoarderShadow")) {
		// Draw boarder shadow.
		Boarder::RenderShadow(s);
	}
	
	// Draw a trail in the snow.
//	Trail::Render(s);

	// Draw ballistic particles.
	Particle::Render(s);

	// Draw weather overlays (falling snow, etc).
	if (player_index == 0)
	  Weather::RenderOverlay(s, true);
	else
	  Weather::RenderOverlay(s, false);
	
	// End of 3D drawing.
	Render::EndFrame();

	// Draw any UI stuff.
	UI::Render(s, player_index);

	// Music captions.
	Music::Render(s);
	
	// Console.
	Console::Render(s);

	// Remember the frame time, for computing frame rate stats.
	FrameTimeQueue[NextFTSample].Timestamp = CurrentTicks;
	FrameTimeQueue[NextFTSample].FrameTime = DeltaTicks;
	NextFTSample += 1;
	if (NextFTSample >= FT_SAMPLES) NextFTSample = 0;
	
	// Show the frame rate.
	if (Config::GetBoolValue("ShowFrameRate")) {
		// Compute min, max, average frame times over the past one second.
		int	min = 1000, max = 1, sum = 0, SampleCount = 0;
		int	i;
		for (i = 0; i < FT_SAMPLES; i++) {
			FrameTimeSample&	s = FrameTimeQueue[i];
			if (CurrentTicks - s.Timestamp < 1000) {
				// Sample is within the last second.
				if (s.FrameTime < min) min = s.FrameTime;
				if (s.FrameTime > max) max = s.FrameTime;
				sum += s.FrameTime;
				SampleCount++;
			}
		}

		// Compute corresponding frame rates.
		float	MinFR, MaxFR, AvgFR;
		MinFR = 1000.0f / max;
		MaxFR = 1000.0f / min;
		if (SampleCount) {
			AvgFR = (SampleCount * 1000.0f) / sum;
		}
		
		// Show the stats.
		char	buf[80];
		strcpy(buf, "fps: ");
		Text::FormatNumber(buf + strlen(buf), 1000.0f / DeltaTicks, 2, 1);	// last
		strcat(buf, "/");
		Text::FormatNumber(buf + strlen(buf), MinFR, 2, 1);	// min
		strcat(buf, "/");
		Text::FormatNumber(buf + strlen(buf), MaxFR, 2, 1);	// max
		strcat(buf, "/");
		Text::FormatNumber(buf + strlen(buf), AvgFR, 2, 1);	// avg
		
		Text::DrawString(5, 12, Text::FIXEDSYS, Text::ALIGN_LEFT, buf);

		// Show a scrolling bar graph of frame rate.
		GUIBegin();//xxxx
		for (i = 0; i < FT_SAMPLES; i++) {
			FrameTimeSample&	s = FrameTimeQueue[(NextFTSample + i) % FT_SAMPLES];
			float	height = 0;
			if (s.FrameTime) height = (1000.0f / s.FrameTime) / 240.0f;
			glColor3f(1, 0.25f, 0.25f);
			glBegin(GL_QUADS);
			glVertex2f((i-320)/320.0f, 180/240.0f + 0);
			glVertex2f((i-320+1)/320.0f, 180/240.0f + 0);
			glVertex2f((i-320+1)/320.0f, 180/240.0f + height);
			glVertex2f((i-320)/320.0f, 180/240.0f + height);
			glEnd();
		}
		GUIEnd();//xxxx
		
	}

	// Show some other miscellaneous info, if desired.
	if (Config::GetBoolValue("ShowRenderStats")) {
		char	buf[1000];
		sprintf(buf, "Model:\n texels: %dK\n tris: %d\n"
			"Terrain:\n tris: %d\n active nodes: %d\n total nodes: %d\n nudge: %g\n"
			" cache:\n  texels: %dK\n  active nodes = %d\n  nodes built = %d\n  thrash ct = %d"
			,
			Model::GetTexelCount() / 1024,
			Model::GetRenderedTriangleCount(),
			TerrainMesh::GetRenderedTriangleCount(),
			TerrainMesh::GetNodesActiveCount(),
			TerrainMesh::GetNodesTotalCount(),
			TerrainMesh::GetDetailNudge(),
			Surface::GetTexelCount() / 1024,
			Surface::GetActiveNodeCount(),
			Surface::GetNodesBuilt(),
			Surface::GetThrashCount()
		       );
		Text::DrawMultiLineString(5, 24, Text::FIXEDSYS, Text::ALIGN_LEFT, 640, buf);
	}

	// Show viewer location.
	if (Config::GetBoolValue("ShowViewerLocation") && Viewer) {
		char	buf[80];
		Text::FontID	f = Text::FIXEDSYS;

		vec3	v = ViewerLoc;
		
		int	y = 400;
		int	dy = Text::GetFontHeight(f);
		Text::FormatNumber(buf, v.X() + 32768, 4, 1);
		Text::DrawString(20, y, f, Text::ALIGN_LEFT, buf, 0xFF000000);
		y += dy;
		Text::FormatNumber(buf, v.Y(), 4, 1);
		Text::DrawString(20, y, f, Text::ALIGN_LEFT, buf, 0xFF000000);
		y += dy;
		Text::FormatNumber(buf, v.Z() + 32768, 4, 1);
		Text::DrawString(20, y, f, Text::ALIGN_LEFT, buf, 0xFF000000);
		y += dy;
	}
		
	
//	// Log user speed xxxxxx
//	{
//		char	buf[80];
//		float	speed = 0;
//		MDynamic*	d = Game::GetUser();
//		if (d) speed = d->GetVelocity().magnitude();
//		Text::FormatNumber(buf, speed * 2.2369, 3, 1);
//		Text::DrawString(40, 400, Text::DEFAULT, Text::ALIGN_LEFT, buf);
//	}
//	// xxxxxxxx

	Overlay::Render();
	
	if (Config::GetValue("RecordMoviePath")) SaveMovieFrame();

	const char*	fn = Config::GetValue("SaveFramePPM");
	if (fn) {
		Render::WriteScreenshotFilePPM(fn);
		Config::SetValue("SaveFramePPM", NULL);
	}
	
	} // end of two player display
	
	Render::ShowFrame();

	//xxxxxxx
	Config::SetBoolValue("F4Pressed", false);
}

void SetWindowCoordinates(int number_of_players, 
			  int *window_corner_x, int *window_corner_y, 
			  int *window_size_x, int *window_size_y){
  
  int x_size = Render::GetWindowWidth();
  int y_size = Render::GetWindowHeight();
  
  if (number_of_players == 1){
    window_corner_x[0] = window_corner_y[0] = 0;
    window_size_x[0] = x_size;
    window_size_y[0] = y_size;
  }
  if (number_of_players == 2){
    window_corner_x[0] = window_corner_x[1] = 0;
    window_corner_y[0] = y_size / 2;
    window_corner_y[1] = 0;
    window_size_x[0] = window_size_x[1] = x_size;
    window_size_y[0] = window_size_y[1] = y_size / 2;
  }
  if (number_of_players == 3){
    window_corner_x[0] = window_corner_x[1] = 0;
    window_corner_x[2] = x_size / 2;
    window_corner_y[0] = y_size / 2;
    window_corner_y[1] = window_corner_y[2] = 0;
    window_size_x[0] = x_size;
    window_size_x[1] = window_size_x[2] = x_size / 2;
    window_size_y[0] = window_size_y[1] = window_size_y[2] = y_size / 2;
  }
  if (number_of_players == 4){
    window_corner_x[0] = window_corner_x[2] = 0;
    window_corner_x[1] = window_corner_x[3] = x_size / 2;
    window_corner_y[0] = window_corner_y[1] = y_size / 2;
    window_corner_y[2] = window_corner_y[3] = 0;
    window_size_x[0] = window_size_x[1] = window_size_x[2] = window_size_x[3] = x_size / 2;
    window_size_y[0] = window_size_y[1] = window_size_y[2] = window_size_y[3] = y_size / 2;
  }    
}

#ifdef NOT

	
void	ShowLogo()
// Shows a logo image.
{
	Render::Texture*	TitleImage = Render::NewTexture("slingshot-logo.psd", false, false, false);
		
	Render::ClearFrame();
	Render::ClearZBuffer();
	int	x0 = (640 - TitleImage->GetWidth()) / 2;
	int	y0 = (480 - TitleImage->GetHeight()) / 2;
	ImageOverlay::Draw(x0, y0, TitleImage->GetWidth(), TitleImage->GetHeight(), TitleImage, 0, 0);
	Overlay::Render();
	Render::ShowFrame();

	delete TitleImage;
}


#endif // NOT


//
// Input polling thread.
//


const int	MIN_SAMPLE_PERIOD = 16;	// ~60 Hz.  Sample at least this often (in ms).


#ifdef LINUX
pthread_mutex_t	BufferMutex = PTHREAD_MUTEX_INITIALIZER;
#else // not LINUX
HANDLE	BufferMutex = NULL;
#endif // not LINUX
int	NextSampleTime = 0;
int	LastSampleTime = 0;

const int	SAMPLE_BUFFER_BITS = 4;	// 16-sample buffer.
const int	MAX_SAMPLES = 1 << SAMPLE_BUFFER_BITS;
const int	BUFFER_MASK = MAX_SAMPLES - 1;
UpdateState	Buffer[MAX_SAMPLES];
int	Front = 0;
int	Rear = 0;
int	BufferCount() { return (Front - Rear) & BUFFER_MASK; }


bool	SampleInputsAndAddToBuffer()
// Locks the input buffer, samples the control inputs, and inserts a new UpdateState
// into the buffer.
// Returns false if the buffer is full or it can't acquire a lock on the input buffer.
{
	if (BufferCount() >= 15
#ifdef LINUX
	    || pthread_mutex_trylock(&BufferMutex) == EBUSY
#else // not LINUX
	    || WaitForSingleObject(BufferMutex, 0) == WAIT_FAILED
#endif // not LINUX
		)
	{
		return false;
	}

	UpdateState*	u = &Buffer[Front];

	// Fill timer values.
	u->Ticks = Timer::GetTicks();
	u->DeltaTicks = u->Ticks - LastSampleTime;
	if (u->DeltaTicks <= 0) u->DeltaTicks = 1;	// Don't allow a 0 time delta.
	if (u->DeltaTicks > 100) u->DeltaTicks = 100;	// Limit max time delta.
	u->DeltaT = u->DeltaTicks / 1000.0f;

	// Poll user input.
	Input::GetInputState(&u->Inputs, u->DeltaT);

	// Remember when we sampled.
	LastSampleTime = u->Ticks;

	// Schedule next sample.
	NextSampleTime = u->Ticks + MIN_SAMPLE_PERIOD;

	// Commit the sample.
	Front = (Front + 1) & BUFFER_MASK;
	
	// Unlock the buffer.
#ifdef LINUX
	pthread_mutex_unlock(&BufferMutex);
#else // not LINUX
	ReleaseMutex(BufferMutex);
#endif // not LINUX

	return true;
}


bool	UpdatesPending()
// Returns true if there are update samples in the queue waiting to be processed.
{
	// Check buffer.
	return BufferCount() > 0;
}


void	GetNextUpdateState(UpdateState* u)
// Returns the next update to be processed.  If the update buffer is empty, then
// polls the control inputs and returns their current values.
{
	// Timeout on this, to be safe?
	while (BufferCount() == 0) {
		SampleInputsAndAddToBuffer();
	}

	*u = Buffer[Rear];	// Copy sampled data.

	Rear = (Rear + 1) & BUFFER_MASK;
}


#ifdef LINUX
void*	ServiceInputsThreadFunction(void* dummy)
#else // not LINUX
void _cdecl	ServiceInputsThreadFunction(void* dummy)
#endif // not LINUX
// Thread procedure which periodically polls the inputs and the time and puts
// the results in a buffer for retrieval by the main game loop.
{
	InputsThreadActive = true;
	
#ifndef LINUX
	BufferMutex = CreateMutex(NULL, FALSE, "input-buffer-mutex");
#endif // not LINUX

	while (!StopInputsThread) {
		if (PauseInputsThread > 0) {
			// Sleep for a while.
			Timer::Sleep(100);
		} else {

			// Sleep while waiting until it's time to collect input.
			int	Ticks = Timer::GetTicks();
			int	SleepTime = NextSampleTime - Ticks;
			if (SleepTime > 0) {
				if (SleepTime > 100) SleepTime = 100;	// Keep it within reason.
				// Zzz.
				Timer::Sleep(SleepTime);
			}
			
			// If it's time to collect input, then go for it.
			Ticks = Timer::GetTicks();
			if (Ticks >= NextSampleTime) {
				SampleInputsAndAddToBuffer();	// get inputs, & timestamp, & add to buffer;
			}
		}
	}

	InputsThreadActive = false;

#ifdef LINUX
	return NULL;
#endif // LINUX
}


};	// namespace GameLoop;

