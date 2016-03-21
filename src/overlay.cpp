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
// overlay.cpp	-thatcher 12/30/1999 Copyright Slingshot Game Technology

// Module that builds a sort of display-list for drawing 2D overlay stuff
// at the end of a frame.  E.g. text and images.  By deferring rendering to
// the end of the frame, we can avoid z-buffer issues and use alpha
// blending.


#include "ogl.hpp"
#include "render.hpp"
#include "error.hpp"
#include "overlay.hpp"
#include "console.hpp"
#include "lua.hpp"

#include "multiplayer.hpp"

namespace Overlay {
;


const int	MAX_CHUNK_BYTES = 8000;
char	ChunkBuffer[MAX_CHUNK_BYTES];
int	NextChunkByte = 0;


ChunkRendererFP	ChunkRendererTable[CHUNK_TYPE_COUNT];


// List of active movies.
const int	MAX_MOVIES = 20;
struct MovieInfo {
	GG_Player*	Player;
	int	DepthKey;	// Arbitrary integer, used for depth sorting.  Higher number == closer to the viewer.
	int	ID;
	bool	Active;

	int	player_index;

	MovieInfo() {
		Player = NULL;
		DepthKey = 0;
		ID = 0;
		Active = true;
		player_index = 0;
	}
} Movie[MAX_MOVIES];

int	NextMovieID = 1;


int	DeltaTicks = 0;	// Accumulate ticks from Update() calls, use to advance movies during Render().


// Lua hooks.
static void	PlayMovie_lua()
{
	int	id = PlayMovie(
		lua_getstring(lua_getparam(1)),
		(int) lua_getnumber(lua_getparam(2)),
		(int) lua_getnumber(lua_getparam(3)));
	lua_pushnumber(id);
}

static void	StopMovie_lua()
{
	StopMovie((int) lua_getnumber(lua_getparam(1)));
}


void	Open()
// Open the overlay module.  Empty the chunk list.
{
	NextChunkByte = 0;

	// Clear the renderer callback table.
	int	i;
	for (i = 0; i < CHUNK_TYPE_COUNT; i++) ChunkRendererTable[i] = 0;

	// Register Lua hooks.
	lua_register("overlay_play_movie", PlayMovie_lua);
	lua_register("overlay_stop_movie", StopMovie_lua);
}


void	Close()
// Close the overlay module.
{
	Clear();
}


void	Clear()
// Empty the chunk list, unreference movies.
{
	NextChunkByte = 0;

	int	i;
	for (i = 0; i < MAX_MOVIES; i++) {
		if (Movie[i].Player) {
			Movie[i].Player->unRef();
			Movie[i].Player = NULL;
		}
	}

	DeltaTicks = 0;
}


void	Update(const UpdateState& u)
// Update.  Collects delta ticks, and applies the total to the currently
// playing movies during Render().
{
	DeltaTicks += u.DeltaTicks;
}


int	PlayMovie(const char* file, int DepthKey, int ggplaymode)
// Loads the specified movie and starts playing it.  The DepthKey is
// used to sort all the active movies.  Larger number == closer to
// viewer.  Movies with the same depth key are ordered according to
// their respective calls to PlayMovie(); later calls == closer to
// viewer.
// Returns an ID number which can be used later to refer to the
// movie in StopMovie() or GetMoviePlayer() calls.
{
	int	slot = 0;
	int	i;

	for (i = 0; i < MAX_MOVIES; i++) {
		if (Movie[i].Player == NULL) {
			slot = i;
			break;
		}
	}

	// If we're bumping a movie out of the way, then unreference it.
	// Also log a warning.
	if (Movie[slot].Player) {
		Console::Printf("PlayMovie(\"%s\") caused movie to be bumped.", file);

		Movie[slot].Player->unRef();
		Movie[slot].Player = NULL;
	}

	// Create the new movie and reference it.
	GameLoop::AutoPauseInput	autoPause;	// Input thread seems to slow down file loads drastically on Win32.
	Movie[slot].Player = GameLoop::LoadMovie((char*) file);

// 	GG_Rval	ret;
// 	ret = GameLoop::LoadMovie((char*) file);	// GUI->loadMovie((char*) file, &(Movie[slot].movie));
// 	if (ret != GG_OK) {
// 		Error e; e << "Can't load movie file '" << file << "'.";
// 		throw e;
// 	}
// //	Movie[slot].movie->setPlayMode(ggplaymode);
// 	GUI->createPlayer(Movie[slot].movie, &(Movie[slot].Player));
	if (Movie[slot].Player == NULL) {
		Error e; e << "Couldn't create movie player from '" << file << "'.";
		throw e;
	}
	
	Movie[slot].Player->setPlayMode(ggplaymode);

	Movie[slot].DepthKey = DepthKey;
	Movie[slot].ID = NextMovieID;

	Movie[slot].player_index = MultiPlayer::CurrentPlayerIndex();

	NextMovieID++;	// Advance the ID counter.

	return Movie[slot].ID;
}


void	StopMovie(int id)
// Stops the movie using the given ID.
{
	int	i;
	for (i = 0; i < MAX_MOVIES; i++) {
		if (Movie[i].ID == id && Movie[i].Player != NULL) {
			Movie[i].Player->unRef();
			Movie[i].Player = NULL;
		}
	}
}


void	SetMovieActive(int id, bool active)
// Enables/disables a movie, without discarding it.
{
	int	i;
	for (i = 0; i < MAX_MOVIES; i++) {
		if (Movie[i].ID == id && Movie[i].Player != NULL) {
			Movie[i].Active = active;
		}
	}
}

GG_Movie*	GetMovie(int id)
// Returns a pointer to the movie for the id'd movie.
// See GetMoviePlayer() for caveats.
{
	int	i;
	for (i = 0; i < MAX_MOVIES; i++) {
		if (Movie[i].ID == id && Movie[i].Player != NULL) {
			return Movie[i].Player->getMovie();
		}
	}

	return NULL;
}


GG_Player*	GetMoviePlayer(int id)
// Returns a pointer to the player for the id'd movie.  The caller can
// then manipulate the player using the GameGUI API.  However, the
// returned pointer should be considered temporary.  Continuing access
// (i.e. across frames) should be done by repeated calls to
// GetMoviePlayer() using the original ID.
{
	int	i;
	for (i = 0; i < MAX_MOVIES; i++) {
		if (Movie[i].ID == id && Movie[i].Player != NULL) {
			return Movie[i].Player;
		}
	}

	return NULL;
}


static void	RenderOverlayChunks();


void	Render()
// Draw the movies, then stored chunks, and then clear the chunk list.
{
	//
	// GameGUI movies.
	//

	// First thing, sort active movies by DepthKey.  Bubble sort is good, because the list
	// won't change frequently.
	int	i, j;
	for (j = 0; j < MAX_MOVIES; j++) {
		for (i = 0; i < MAX_MOVIES - j - 1; i++) {
			if (Movie[i+1].Player == NULL) continue;
			if (Movie[i].Player == NULL || Movie[i].DepthKey > Movie[i+1].DepthKey) {
				// Swap.
				MovieInfo	temp;
				temp = Movie[i];
				Movie[i] = Movie[i+1];
				Movie[i+1] = temp;
			}
		}
	}
	
	int player_index = MultiPlayer::CurrentPlayerIndex();

	// Now, render.
	GameLoop::GUIBegin();
	for (i = 0; i < MAX_MOVIES; i++) {
		MovieInfo&	m = Movie[i];
		if (m.Player && m.Active && m.player_index == player_index) {
			// Advance.
			GG_Rval	rval = m.Player->play(DeltaTicks);
			
			if (rval == GG_PLAYERFINISHED && m.Player->getPlayMode() == GG_PLAYMODE_TOEND) {
				// Movie is done; unreference it so we don't play it anymore.
				m.Player->unRef();
				m.Player = NULL;
			}
		}
	}
	GameLoop::GUIEnd();

	DeltaTicks = 0;
	
	//
	// Set up the OpenGL rendering state for 2D overlay.  Establish
	// matrices so that the output window is mapped to a 640x480
	// isometric coordinate system with 0,0 in the upper left, x
	// increasing to the right and y increasing downwards.
	//
	// Turn off the z-buffer, disable alpha testing, and set a basic
	// alpha-blending mode.
	//

	::Render::DisableZBuffer();
	::Render::DisableAlphaTest();
	::Render::DisableLightmapBlend();
	::Render::CommitRenderState();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glScalef(2.0f / 640, -2.0f / 480.0f, 1);
	glTranslatef(-640 / 2.0f, -480 / 2.0f, 0);

	glMatrixMode(GL_PROJECTION);
//	glPushMatrix();
	glLoadIdentity();

#ifdef NOT
	// xxxx Do something funky and random to the modelview matrix,
	// then draw the overlay, then restore the correct modelview
	// before drawing it correctly.
	if (rand() < 5000) {
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		float	xshift = (rand() / float(RAND_MAX) - 0.5f) * 20;
		float	yshift = (rand() / float(RAND_MAX) - 0.5f) * 20;
		glTranslatef(xshift, yshift, 0);
//		float	xscale = (rand() / float(RAND_MAX) - 0.5) * 0.1 + 1.0;
//		float	yscale = (rand() / float(RAND_MAX) - 0.5) * 0.1 + 1.0;
//		glScalef(xscale, yscale, 1);

		RenderOverlayChunks();	// Really needs to be in a modulated color...

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
#endif // NOT
	
	// Render the overlay chunks.
	RenderOverlayChunks();
	
	// Clear the chunk buffer to get ready for the next frame.
	NextChunkByte = 0;

	// Restore OpenGL state.
	glMatrixMode(GL_PROJECTION);
//	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


void	RenderOverlayChunks()
// Go through the chunk list and call the appropriate handler for each chunk.
{
	int	index = 0;

	// Walk the chunk buffer and render each chunk.
	while (index < NextChunkByte) {
		// Get the next chunk.
		Chunk*	c = reinterpret_cast<Chunk*>(ChunkBuffer + index);
		index += c->Bytes;	// Increment for following chunk.
		index = (index + 3) & ~3;	// Force 4-byte alignment.

		// Look up and call the chunk renderer function to draw this chunk.
		if (c->Type >= 0 && c->Type < CHUNK_TYPE_COUNT) {
			void (*handler)(Chunk*) = ChunkRendererTable[c->Type];
			if (handler) {
				(*handler)(c);
			}
		}
	}
}


Chunk*	NewChunk(int bytes, ChunkType type)
// Allocate a new chunk and insert it into the list.  Initializes the header
// info.  Returns a pointer to the new chunk, so the caller can fill in the
// rest of the data.  A handler callback function based on the specified type
// will be called later, with a pointer to this chunk as an argument, to do
// the actual rendering.
// Returns NULL if there isn't enough storage in the chunk buffer to create
// the new chunk.
{
	if (NextChunkByte + bytes >= MAX_CHUNK_BYTES) return NULL;

	Chunk*	c = reinterpret_cast<Chunk*>(ChunkBuffer + NextChunkByte);
	
	NextChunkByte += bytes;
	NextChunkByte = (NextChunkByte + 3) & ~3;	// Force 4-byte alignment.

	c->Bytes = bytes;
	c->Type = type;

	return c;
}


void	RegisterChunkRenderer(ChunkType type, ChunkRendererFP callback)
// Associates the specified chunk type with the given callback function.
// This callback will be called with pointers to the corresponding chunk
// types during the overlay rendering pass, to do the actual rendering.
{
	if (type < 0 || type >= CHUNK_TYPE_COUNT) {
		Error e; e << "Overlay::RegisterChunkRenderer() - illegal chunk type " << type;
		throw e;
	}

	if (ChunkRendererTable[type] != 0) {
		Error e; e << "Overlay::RegisterChunkRenderer() - type " << type << " already registered.";
		throw e;
	}

	// Register.
	ChunkRendererTable[type] = callback;
}


};	// namespace Overlay

