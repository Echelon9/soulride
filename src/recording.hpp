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
// recording.hpp	-thatcher 12/29/1999 Copyright Slingshot Game Technology

// Interface to module responsible for recording engine events & state,
// for later playback.


#ifndef RECORDING_HPP
#define RECORDING_HPP


#include "types.hpp"


namespace Recording {
	enum ChunkType {
		TIMESTAMP = 0,
		
		BOARDER_STATE,
		
		PARTICLE_SOURCE,

		GAME_STATE,

		BOARDER_PARTICLES,
		
		// camera hint
		// camera command
		
//		DISCRETE_SOUND,
//		CONTINUOUS_SOUND,
//		SOUND_MODIFIER,
//		CD_COMMAND,
//
//		TEXT,
//		ICON,
//		ICON_MODIFIER,

		CHUNK_TYPE_COUNT
	};

	enum Mode {
		STOP = 0,
		PAUSE,
		PAUSERECORD,
		PLAY,
		RECORD,

		MODE_COUNT
	};

	void	Open();
	void	Close();
	void	Clear();

	void	Load(const char* filename);
	void	Save(const char* filename);
	void	EnumerateRecordings(const char* MtnName, void (*callback)(const char* RecName));
	bool	RecordingExists(const char* recname);

	enum ChannelID {
		DEMO_CHANNEL = 0,
		PLAYER_CHANNEL,

		CHANNEL_COUNT
	};
	void	SetChannel(ChannelID chan);
	ChannelID	GetChannel();
	bool	ChannelHasData(ChannelID chan);
	
	Mode	GetMode();
	void	SetMode(Mode m, int Ticks);

	int	GetCursorTicks();
	bool	GetAtEnd();
	
	// Recording.
	bool	IsReadyForStateData();
	void	InsertTimestamp(int Ticks);
	uint8*	NewChunk(ChunkType channel, int Bytes);
	void	SetResumePosition(int ResumeTicks, int Ticks);

	// Recording header data.
	void	SetScore(int score);
	int	GetScore();
	void	SetPlayerName(const char* playername);
	const char*	GetPlayerName();
	void	SetRunIndex(int index);
	int	GetRunIndex();
	
	// Playback.
	void	MoveCursor(int DeltaTicks);
	void	GetCurrentChunkPair(ChunkType channel, uint8** bufa, uint8** bufb, float* f);
};


#endif // RECORDING_HPP
