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
// overlay.hpp	-thatcher 12/30/1999 Copyright Slingshot Game Technology

// Interface to a module which manages the blitting of overlay layers.


#ifndef OVERLAY_HPP
#define OVERLAY_HPP


#include "utility.hpp"
#include "gameloop.hpp"
#include "gamegui/gamegui.h"



namespace Overlay {
	void	Open();
	void	Close();
	void	Clear();

	void	Update(const UpdateState& u);
	void	Render();
	
	// Movie playback.
	int	PlayMovie(const char* file, int DepthKey, int ggplaymode);
	void	SetMovieActive(int id, bool act);
	void	StopMovie(int id);
	GG_Movie*	GetMovie(int id);
	GG_Player*	GetMoviePlayer(int id);


	//
	// Hooks for generic chunk rendering.
	//
	
	enum ChunkType {
		IMAGE = 0,
		TEXT,

		CHUNK_TYPE_COUNT
	};
	
	struct Chunk {
		int	Bytes;
		ChunkType	Type;
		// Derived chunk types can tack on extra data.
	};
	
	Chunk*	NewChunk(int bytes, ChunkType type);

	typedef void (*ChunkRendererFP)(Chunk*);
	void	RegisterChunkRenderer(ChunkType type, ChunkRendererFP callback);
};


#endif // OVERLAY_HPP

