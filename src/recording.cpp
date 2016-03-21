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
// recording.cpp	-thatcher 12/30/1999 Copyright Slingshot Game Technology

// Code for recording and playing back "movies" of the player's runs.


#ifdef LINUX

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#else // not LINUX

#include <io.h>
#include <direct.h>

#endif // not LINUX

#include <string.h>
#include "utility.hpp"
#include "recording.hpp"
#include "text.hpp"
#include "game.hpp"
#include "lua.hpp"
#include "console.hpp"
#include "timer.hpp"


namespace Recording {
;


// Some info about the recording.
const int	FORMAT_VERSION = 3;
const int	MTN_MAXLEN = 200;
const int	NAME_MAXLEN = 60;
struct HeaderInfo {
	bool	Valid;
	int	FormatVersion;
	char	Mountain[MTN_MAXLEN];
	int	Score;
	char	PlayerName[NAME_MAXLEN];
	int	RunIndex;
};


// Buffer structure for storing the recording data.
const int	BUFFER_MAX_BYTES = 300000;
struct RecBuffer {
	uint8	Data[BUFFER_MAX_BYTES];
	int	End;
	HeaderInfo	Header;

	RecBuffer() { End = 0; Header.Valid = false; }
	
} Buffer[CHANNEL_COUNT];	// 0 == demo channel, 1 == player channel.
ChannelID	CurrentChannel = DEMO_CHANNEL;
RecBuffer*	cbuf = &Buffer[DEMO_CHANNEL];	// Current buffer.


// Playback/recording state.
int	Cursor = 0;
int	StartTicks = 0;
int	CurrentTicks = 0;
Mode	CurrentMode = STOP;
int	LastFrameRecorded = 0;
int	LeftoverTicks = 0;
bool	FrameReady = false;
bool	RecordingAtEnd = false;


static void	GetMode_lua()
{
	lua_pushnumber(GetMode());
}


static void	SetMode_lua()
{
	SetMode((Mode) (int) lua_getnumber(lua_getparam(1)), GameLoop::GetCurrentTicks());
}


void	Open()
// Initialize recording module.
{
	lua_register("recording_get_mode", GetMode_lua);
	lua_register("recording_set_mode", SetMode_lua);
}


void	Close()
// Shut down recording module.
{
}


void	Clear()
// Clear out current recordings.
{
	int	i;
	for (i = 0; i < CHANNEL_COUNT; i++) {
		Buffer[i].End = 0;
		Buffer[i].Header.Valid = false;
	}
}


static void	ReadHeaderInfo(FILE* fp, HeaderInfo* h)
// Reads the header information from the given file stream.  If h->Valid == false, then
// there was some type of error reading the info.
{
	// Get the signature and version.
	uint32	sig = Read32(fp);
	h->FormatVersion = Read32(fp);

	if (sig != 0x00525253 /* "SRR" */ || h->FormatVersion < 1 || h->FormatVersion > 4) {
		// Bad format.
		h->Valid = false;
		return;
	}

	if (h->FormatVersion <= 1) {
		// Mountain name is not stored in the recording file, so just put a null-string
		// in the MountainName part of the header.
		h->Mountain[0] = 0;
	} else {
		// Read the mountain name from the file.
		int	i = 0;
		int	c;
		for (;;) {
			c = fgetc(fp);
			if (c == EOF || c == 0) {
				h->Mountain[i] = 0;	// Terminate the string.
				break;
			} else {
				h->Mountain[i++] = c;

				// Make sure buffer doesn't overflow.
				if (i >= MTN_MAXLEN) {
					h->Mountain[i] = 0;	// Terminate string.
					// Consume the rest of the string.
					for (;;) {
						c = fgetc(fp);
						if (c == EOF || c == 0) break;
					}
					break;
				}
			}
		}
	}
	
	if (h->FormatVersion >= 3) {
		// Load the user score.
		h->Score = Read32(fp);

		// Load the player name.
		fgets(h->PlayerName, NAME_MAXLEN, fp);
		if (h->PlayerName[strlen(h->PlayerName)-1] == '\n') h->PlayerName[strlen(h->PlayerName)-1] = 0;
		
	} else {
		// No score in the header.  Default to 0.
		h->Score = 0;

		// No player name in the header.  Default to null string.
		h->PlayerName[0] = 0;
	}

	if (h->FormatVersion >= 4) {
		// Load the run index.
		h->RunIndex = Read32(fp);
	} else {
		// No run index in the header.
		h->RunIndex = -1;
	}
	
	h->Valid = true;
}


void	Load(const char* filename)
// Load the recording of the specified name.
{
	char	buf[1000];
	strcpy(buf, ".." PATH_SEPARATOR "Recordings" PATH_SEPARATOR);
	strcat(buf, filename);
	strcat(buf, ".srr");
	
	FILE*	fp = fopen(buf, "rb");
	if (fp == NULL) {
		// Can't load this file.
		// xxx need to report an error somehow.
		Console::Printf("Can't open '%s' for input.\n", buf);
		return;
	}

	HeaderInfo	h;

	ReadHeaderInfo(fp, &h);

#ifdef NOT
	// Get the signature and version.
	uint32	sig = Read32(fp);
	h.FormatVersion = Read32(fp);

	if (h.Valid == false) {
		// Bad format.
		// xxxx need to report an error somehow.
		fclose(fp);
		return;
	}
#endif // NOT

	// Read the recording data.
	cbuf->End = Read32(fp);
	fread(cbuf->Data, 1, cbuf->End, fp);

	fclose(fp);

	cbuf->Header = h;
	if (h.FormatVersion < 2) {
		// Insert the name of the current mountain.
		strncpy(cbuf->Header.Mountain, Game::GetCurrentMountain(), MTN_MAXLEN);
		cbuf->Header.Mountain[MTN_MAXLEN-1] = 0;	// Ensure termination.
	}
	cbuf->Header.Valid = true;

	// xxx consistency-check the data?
	
	// Reset playback state.
	CurrentTicks = 0;
	Cursor = 0;
	CurrentMode = STOP;
	RecordingAtEnd = false;
}


void	Save(const char* filename)
// Save the current recording to the specified file.
{
	if (cbuf->End == 0) {
		// No data, don't save.
		return;
	}
	
	// Make sure Recordings subdirectory exists.
	chdir("..");
	if (chdir("Recordings") != 0) {
#ifdef LINUX
		mkdir("Recordings", 0660);
#else // not LINUX
		mkdir("Recordings");
#endif // not LINUX
	} else {
		chdir("..");
	}
	chdir("data");

	// Form filename.
	char	buf[1000];
	strcpy(buf, ".." PATH_SEPARATOR "Recordings" PATH_SEPARATOR);
	strcat(buf, filename);
	strcat(buf, ".srr");
	
	FILE*	fp = fopen(buf, "wb");
	if (fp == NULL) {
		// Can't save to this filename.
		// xxxx need to report an error somehow.
		Console::Printf("Can't open '%s' for output.\n", buf);
		return;
	}

	// Write signature and version.
	Write32(fp, 0x00525253);
	Write32(fp, FORMAT_VERSION);

	// Write the mountain name.
	const char*	p = cbuf->Header.Mountain;
	while (*p) {
		fputc(*p, fp);
		p++;
	}
	fputc(0, fp);	// String termination marker.

	// Write the score.
	Write32(fp, cbuf->Header.Score);

	// Write player name.
	fputs(cbuf->Header.PlayerName, fp);
	fputc('\n', fp);
	
	// Write data.
	Write32(fp, cbuf->End);
	fwrite(cbuf->Data, 1, cbuf->End, fp);

	fclose(fp);
}


bool	RecordingExists(const char* recname)
// Return true if the named recording already exists.
{
	char	buf[1000];
	strcpy(buf, ".." PATH_SEPARATOR "Recordings" PATH_SEPARATOR);
	strcat(buf, recname);
	strcat(buf, ".srr");
	
	FILE*	fp = fopen(buf, "rb");
	if (fp == NULL) {
		return false;
	}
	fclose(fp);
	return true;
}


static bool	MountainMatch(const char* MtnName, const char* RecName)
// Checks the RecName file, to see if it's a recording made at the given
// mountain.  Returns true on match, false otherwise.
{
	char	buf[1000];
	strcpy(buf, ".." PATH_SEPARATOR "Recordings" PATH_SEPARATOR);
	strcat(buf, RecName);	// xxxx possible buffer overflow.
	
	FILE*	fp = fopen(buf, "rb");
	if (fp == NULL) {
		// Can't load this file.
		return false;
	}

	HeaderInfo	h;
	ReadHeaderInfo(fp, &h);

	fclose(fp);
	
	if (h.FormatVersion == 0 || Utility::StringCompareIgnoreCase(h.Mountain, MtnName) != 0) {
		// Couldn't load the header, or mountain names don't match.
		return false;
	}

	return true;
}


void	EnumerateRecordings(const char* MtnName, void (*callback)(const char* RecName))
// Goes through each recording file, finds the ones which were made at
// the given mountain, and passes each qualifying file name to the given
// callback function.
{
#ifdef LINUX
	DIR*	dir = opendir("../Recordings");
	struct dirent*	ent;
	while (ent = readdir(dir)) {
		char*	filename = ent->d_name;
#else // not LINUX
	_finddata_t	d;
	long	handle = _findfirst(".." PATH_SEPARATOR "Recordings" PATH_SEPARATOR "*.srr", &d);
	int	result = handle;
	while (result != -1) {
		char*	filename = d.name;
#endif // not LINUX
		// Check to see if this recording matches the given mountain.
		if (MountainMatch(MtnName, filename)) {
			// Remove extension.
			char	buf[1000];
			strcpy(buf, filename);
			int	len = strlen(buf);
			if (len > 4) {
				buf[len - 4] = 0;
			}
			
			// Notify caller.
			(*callback)(buf);
		}
		
#ifdef LINUX
	}
	closedir(dir);
#else // not LINUX
		result = _findnext(handle, &d);
	}
	_findclose(handle);
#endif // not LINUX
}


void	SetScore(int score)
// Sets the score value in the current recording's header.
{
	cbuf->Header.Score = score;
}


int	GetScore()
// Returns the score stored in the current recording's header.
{
	return cbuf->Header.Score;
}


void	SetPlayerName(const char* playername)
// Sets the player name in the current recording's header.
{
	strncpy(cbuf->Header.PlayerName, playername, NAME_MAXLEN);
	cbuf->Header.PlayerName[NAME_MAXLEN-1] = 0;
}


const char*	GetPlayerName()
// Returns the player name stored in the current recording's header.
{
	return cbuf->Header.PlayerName;
}


int	GetRunIndex()
// Returns the run index this run was recorded at.
{
	return cbuf->Header.RunIndex;
}


void	SetRunIndex(int index)
// Sets the run index in the current recording's header.
{
	cbuf->Header.RunIndex = index;
}


void	SetChannel(ChannelID chan)
// Sets the currently active channel.
{
	if (chan < CHANNEL_COUNT /* && chan != CurrentChannel */) {
		CurrentChannel = chan;
		cbuf = &Buffer[CurrentChannel];

		// Reset to the start of the recording.
		Cursor = 0;
		CurrentTicks = 0;
		CurrentMode = STOP;
	}
}


ChannelID	GetChannel()
// Returns the current channel.
{
	return CurrentChannel;
}


bool	ChannelHasData(ChannelID chan)
// Returns true if the specified channel contains data.
{
	return Buffer[chan].End != 0;
}


Mode	GetMode()
// Returns the current recording/playback status.
{
	return CurrentMode;
}


int	GetCursorTicks()
// Returns the current recording time.
{
	return CurrentTicks;
}


bool	GetAtEnd()
// Returns true if we're at the end of the recording.
{
	return RecordingAtEnd;
}


void	SetMode(Mode m, int Ticks)
// Sets the record/playback mode.
{
	if (m == PLAY) {
		// Play, if we have a valid recording.
		if (cbuf->Header.Valid) {
			if (CurrentMode == PAUSE) {
				StartTicks = Ticks - CurrentTicks;
			} else {
				// Reset to the start of the recording.
				Cursor = 0;
				StartTicks = Ticks;
				CurrentTicks = 0;
			}
			CurrentMode = PLAY;
		} else {
			CurrentMode = STOP;
		}
		
	} else if (m == RECORD) {
		// Start recording.

		// Init the header.
		cbuf->Header.FormatVersion = 3;
		strncpy(cbuf->Header.Mountain, Game::GetCurrentMountain(), MTN_MAXLEN);
		cbuf->Header.Mountain[MTN_MAXLEN-1] = 0;
		cbuf->Header.Valid = false;

		if (CurrentMode == PAUSERECORD) {
		} else {
			// Clear the buffer, reset everything, and go.
			cbuf->End = 0;
			Cursor = 0;
			StartTicks = Ticks;
			LastFrameRecorded = 0;
			CurrentTicks = 0;
			LeftoverTicks = 0;
		}
		
		FrameReady = true;
		CurrentMode = RECORD;
		
		if (Cursor == 0) {
			// Insert the first chunk.
			InsertTimestamp(Ticks);
		}
		
	} else if (m == STOP) {
		// Stop the recording.
		
		if (CurrentMode == PLAY) {
		} else if (CurrentMode == RECORD || CurrentMode == PAUSERECORD) {
			// Close the recording.

			// Reset the cursor.
			CurrentTicks = 0;
			Cursor = 0;

			if (cbuf->End != 0) {
				cbuf->Header.Valid = true;
			}
		}

		CurrentMode = STOP;

	} else if (m == PAUSE || m == PAUSERECORD) {
		// Attempt to pause the recording.  Stop if we can't pause.

		if (CurrentMode == PLAY) {
			CurrentMode = PAUSE;
		} else if (CurrentMode == RECORD) {
			CurrentMode = PAUSERECORD;

			// Reset the cursor and validate the header, in case we don't resume recording later.
			CurrentTicks = 0;
			Cursor = 0;
			MoveCursor(10000000);	// Go to the end of the recording.
			if (cbuf->End != 0) {
				cbuf->Header.Valid = true;
			}
			
		} else {
			CurrentMode = STOP;
		}
	}
}


bool	IsReadyForStateData()
// Returns true if we're ready to accept state data for the current frame.
// Returns false if we don't want any state data for the current frame (event
// data is acceptable, though).
{
	return FrameReady;
}


void	InsertTimestamp(int Ticks)
// Add a timestamp chunk into the recording.  The given Ticks value is
// based on the engine timer; the function converts it to a ticks value
// relative to the start of the recording before storing it.
//
// This function does some filtering in an effort to record an average
// of 30 frames per second.  Data sources that insert state data into
// the recording buffer should call IsReadyForStateData() before
// inserting a frame's state.
{
	if (CurrentMode != RECORD) return;

	// Decide if we're ready for a new frame.
	const int	TARGET_INTERVAL = 50;	// 50 ms --> 20 Hz.
	int	dt = Ticks - LastFrameRecorded + LeftoverTicks;
	if (dt < TARGET_INTERVAL) {
		// Not enough time has passed since the last recorded frame.
		FrameReady = false;

	} else {
		FrameReady = true;
		LeftoverTicks = dt - TARGET_INTERVAL;
		LeftoverTicks = iclamp(0, LeftoverTicks, TARGET_INTERVAL - 1);
		LastFrameRecorded = Ticks;

		// Insert the timestamp into the recording.
		uint8*	data = NewChunk(TIMESTAMP, 4);
		if (data) {
			EncodeUInt32(data, 0, Ticks - StartTicks);
		}

	}
}


uint8*	NewChunk(ChunkType channel, int Bytes)
// Appends a chunk with the specified number of data bytes to the end of the
// current recording.  Returns a pointer to the data bytes, to be filled in by
// the caller.  Returns NULL if there isn't enough room for the new chunk.
{
//	if (CurrentMode != RECORD) return;

	if (cbuf->End + Bytes + 3 > BUFFER_MAX_BYTES) {
		// No room at the inn.
		
		return NULL;
	}

	// Insert chunk header.
	cbuf->Data[cbuf->End++] = Bytes + 1;
	cbuf->Data[cbuf->End++] = channel;
	
	// Reserve space for the chunk data.
	int	dataindex = cbuf->End;
	cbuf->End += Bytes;

	// Insert chunk trailer.
	cbuf->Data[cbuf->End++] = Bytes + 1;	// For backward traversal.

	return cbuf->Data + dataindex;
}


void	MoveCursor(int DeltaTicks)
// Moves the current play/record cursor by the specified amount of
// ticks.  Positive == forward, negative == backward.  Clamps to stay
// within the bounds of the recording.
{
	if (cbuf->End == 0) return;	// There's no recording data, so there's nothing to do.

	RecordingAtEnd = false;
	
	int	NewTicks = CurrentTicks + DeltaTicks;
	if (NewTicks < 0) NewTicks = 0;

	// Move the Cursor through the data, until it points to the
	// largest timestamp less than or equal to NewTicks.  That
	// should leave us correctly set up for extracting the current
	// chunks for the various channels.

	int32	cticks = 0;
	int	next = 0, prev = 0;
	for (;;) {
		// Extract ticks value at the current cursor's timestamp.
		DecodeUInt32(cbuf->Data, Cursor + 2, (uint32*) &cticks);

		if (NewTicks < cticks) {
			// If we're already at the start of the buffer, then we
			// can't rewind any more.
			if (Cursor <= 0) {
				Cursor = 0;
				CurrentTicks = 0;
				break;
			}
			
			// Probe backward for previous timestamp.
			prev = Cursor;
			for (;;) {
				prev -= cbuf->Data[prev-1] + 2;
				if (prev <= 0 || cbuf->Data[prev+1] == TIMESTAMP) {
					break;
				}
			}
			
			Cursor = prev;	// Rewind the cursor and continue.

		} else {
			// Probe forward for next timestamp.
			next = Cursor;
			for (;;) {
				next += cbuf->Data[next] + 2;
				if (next >= cbuf->End || cbuf->Data[next+1] == TIMESTAMP) {
					break;
				}
			}

			if (next >= cbuf->End) {
				// The cursor is already at the last timestamp.
				CurrentTicks = cticks;
				RecordingAtEnd = true;
				break;
			}
			
			int32	nticks;
			DecodeUInt32(cbuf->Data, next + 2, (uint32*) &nticks);
			if (nticks > NewTicks) {
				// The cursor is in the right spot.
				CurrentTicks = NewTicks;
				break;
			}

			Cursor = next;	// Advance the cursor and continue.
		}
	}
}


void	GetCurrentChunkPair(ChunkType channel, uint8** bufa, uint8** bufb, float* f)
// Finds the two relevant chunks in the specified channel, whose
// timestamps straddle the current cursor time.  Returns pointers to the
// chunks, and a floating-point interpolation value (in the range [0,1])
// which gives the relative position of the cursor between the two
// chunks.  Sets *bufb = *bufa if the second chunk can't be found.  Puts
// NULL in *bufa and *bufb if neither chunk can be found.
{
	uint32	TicksA = 0;
	
	// Probe forward.
	int	i = Cursor;

	DecodeUInt32(cbuf->Data, i+2, &TicksA);

	*bufa = NULL;
	for (;;) {
		i += cbuf->Data[i] + 2;
		if (i >= cbuf->End) {
			// Recording is over, and we haven't found the first chunk.
			break;
		} else if (cbuf->Data[i+1] == TIMESTAMP) {
			DecodeUInt32(cbuf->Data, i+2, &TicksA);
			if (TicksA > (uint32) CurrentTicks) {
				// No data for the desired time.
				return;
			}
		} else if (cbuf->Data[i+1] == channel) {
			// Found the chunk.
			*bufa = cbuf->Data + i + 2;
			break;
		}
	}

	*bufb = *bufa;
	*f = 0;
	if (*bufa == NULL) {
		// Couldn't find first chunk.
		return;
	}

	int	i0 = i;	//xxxxxx

	uint32	TicksB = 0;

	// Probe for timestamp and second chunk.
	for (;;) {
		i += cbuf->Data[i] + 2;

		if (i >= cbuf->End) {
			// End of recording.  Give up looking for second chunk.
			break;
			
		} else if (cbuf->Data[i+1] == TIMESTAMP) {
			// Found the start of the next frame.  Remember the time and continue looking for the second chunk.
			DecodeUInt32(cbuf->Data, i+2, &TicksB);
			
		} else if (cbuf->Data[i+1] == channel) {
			// Found the second chunk.
			if (TicksB == 0) {
				// Haven't found a timestamp yet, so just keep looking.
				continue;
			} else {
				// We have the two chunks and corresponding timestamps.
				// Compute *f and return.
				*bufb = cbuf->Data + i + 2;
				int	dt = TicksB - TicksA;
				if (dt > 0) {
					*f = (CurrentTicks - TicksA) / float(dt);
				}
//				if (*f < 0 || *f > 1) {
//					// Hm, no real data for the desired time -- the data
//					// we've found would be an extrapolation.
//					*f = 0;
//					*bufa = 0;
//					*bufb = 0;
//				}
				break;
			}
		} // else keep looking.
	}
}


void	SetResumePosition(int ResumeTicks, int Ticks)
// This function resets the recording cursor to the spot corresponding
// to simulation time ResumeTicks.  The recording start-time parameters
// are adjusted so that the given Ticks value is mapped to the new spot
// on the recording.  This function is used when restarting a recording
// after a pause, discarding information from the end of the recording
// (data that was recorded post-ResumeTicks).
{
	int	ResumeOffset = ResumeTicks - StartTicks;
	if (ResumeOffset < 0) ResumeOffset = 0;
	StartTicks = Ticks - ResumeOffset;	// Monkey with the recording start time so that subsequent data will be in sync.

	// Find the spot we want to resume at.  Look through the chunks
	// until we get to a timestamp which is later than our desired
	// resume position.
	int	i = 0;
	for (;;) {
		if (i >= cbuf->End) {
			// End of recording.
			break;
		} else if (cbuf->Data[i+1] == TIMESTAMP) {
			// See if this timestamp is >= the desired ResumeOffset.
			int	FrameTicks;
			DecodeUInt32(cbuf->Data, i+2, (uint32*) &FrameTicks);
			if (FrameTicks >= ResumeOffset) {
				// Resume recording here.
				cbuf->End = i;
				break;
			}
		}

		// Advance to the head of the next chunk.
		i += cbuf->Data[i] + 2;
	}
}


};	// namespace Recording

