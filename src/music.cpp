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
// music.cpp	-thatcher 9/24/1999 Copyright Slingshot Game Technology

// Code for managing background music in the game.

// Uses CD audio currently.


#include "sound.hpp"
#include "model.hpp"
#include "music.hpp"
#include "ui.hpp"
#include "config.hpp"
#include "console.hpp"
#include "lua.hpp"
#include "text.hpp"

#include <iostream>

namespace Music {
;


bool	IsOpen = false;
int	TimeMusicLastPlayed = 0;
int	LastCheckedCD = 0;
int	CurrentCDTrack = -1;

//
// Fader stuff.
//
enum FaderInfo {
	FADER_OUT = 0,
	FADER_DOWN,
	FADER_UP,
} FaderState = FADER_OUT;

const int	TotalFadeTime = 1500;
int	FaderTimer = 0;
uint8	FadeUpVolume = 255;
const float	FADE_DOWN_FACTOR = 0.82f;
uint8	FadeDownVolume = uint8(FadeUpVolume * FADE_DOWN_FACTOR);
uint8	FadeStartVolume, FadeEndVolume;
uint8	CurrentVolume = 0;


//
// CD sequencing stuff.
//

const int	MAX_TRACKS = 30;
const int	SONG_MAXLEN = 50;
const int	ARTIST_MAXLEN = 50;
const int	COMMENT_MAXLEN = 80;
struct TrackData {
	char	Song[SONG_MAXLEN];
	char	Artist[ARTIST_MAXLEN];
	char	Comment[2][COMMENT_MAXLEN];
} TrackInfo[MAX_TRACKS];

int8	Sequence[MAX_TRACKS];
int	CDTrackCount = 0;

int	CaptionTimer = 0;

const int	SHOW_CAPTION_TICKS = 8000;


Sound::CDMode	CachedCDMode = Sound::CD_STOP;


void	GetDiscInfo()
// Examine the CD and see what kind of track info we can get out of it.
{
	if (!IsOpen) return;
	
	CDTrackCount = Sound::GetCDTrackCount();
	if (CDTrackCount) {
		// Truncate track memory if necessary.
		if (CDTrackCount > MAX_TRACKS) CDTrackCount = MAX_TRACKS;
		
		// Generate a track sequence.
		int	i;
		for (i = 0; i < CDTrackCount; i++) {
			Sequence[i] = i;
		}

		// Randomly swap items.
		for (i = 0; i < CDTrackCount * 2; i++) {
			int	a, b;
			a = Utility::RandomInt(CDTrackCount);
			b = Utility::RandomInt(CDTrackCount);

			int8	temp = Sequence[a];
			Sequence[a] = Sequence[b];
			Sequence[b] = temp;
		}

		//
		// Get track info.
		//

		// Clear old info.
		for (i = 0; i < CDTrackCount; i++) {
			TrackInfo[i].Song[0] = 0;
			TrackInfo[i].Artist[0] = 0;
			TrackInfo[i].Comment[0][0] = 0;
			TrackInfo[i].Comment[0][1] = 0;
		}
		
		// Look for cdaindex.txt
		char	filename[80];
		Sound::GetCDDriveName(filename);
		strcat(filename, "cdaindex.txt");

#ifdef MACOSX
		// macosx workaround FIX ME LATER
		strcpy(filename, "/tmp/cdaindex.txt");
#endif

//		Console::Printf("cd index filename = '%s'\n", filename);//xxxxxxxxx

		// Load index file from elsewhere during debugging.
		if (Config::GetValue("TestCDAIndex")) {
			strcpy(filename, Config::GetValue("TestCDAIndex"));
		}

		FILE*	fp = fopen(filename, "r");
		if (fp) {
			// Glean what info we can.
			int	index = 0;
			for (index = 0; index < MAX_TRACKS; index++) {
//				int	eof = fscanf(fp, "%d", &track);

				TrackData	t;
				t.Song[0] = 0;
				t.Artist[0] = 0;
				t.Comment[0][0] = 0;
				t.Comment[1][0] = 0;

				// Consume any blank lines.
				int	c;
				do {
					c = fgetc(fp);
				} while (c == '\n' || c == '\r');
				if (c == EOF) break;
				ungetc(c, fp);
				
				char*	eof = fgets(t.Song, SONG_MAXLEN, fp);
				if (eof == NULL) break;
				
				if (t.Song[strlen(t.Song)-1] == '\n') t.Song[strlen(t.Song)-1] = 0;
				if (t.Song[0]) {
					fgets(t.Artist, ARTIST_MAXLEN, fp);
					if (t.Artist[strlen(t.Artist)-1] == '\n') t.Artist[strlen(t.Artist)-1] = 0;
					if (t.Artist[0]) {
						fgets(t.Comment[0], COMMENT_MAXLEN, fp);
						if (t.Comment[0][strlen(t.Comment[0])-1] == '\n') t.Comment[0][strlen(t.Comment[0])-1] = 0;
						if (t.Comment[0][0]) {
							fgets(t.Comment[1], COMMENT_MAXLEN, fp);
							if (t.Comment[1][strlen(t.Comment[1])-1] == '\n') t.Comment[1][strlen(t.Comment[1])-1] = 0;
						}
					}
				}
				
				TrackInfo[index] = t;
			}

			fclose(fp);
		}

		// xxxx lookup data in CDDB, etc...
		
	} else {
		// No audio tracks.  Don't bother with the CD drive until the disc changes.
		CachedCDMode = Sound::CD_NOT_READY;
	}
}


void	PlayNext()
// Play the next CD audio track.
{
	if (!IsOpen) return;
	
//	int	tracks = Sound::GetCDTrackCount();
//	if (tracks) {
//		CurrentCDTrack = (CurrentCDTrack + 1) % tracks;
//		Sound::PlayCDTrack((CurrentCDTrack % tracks) + 1);
//	}
	if (CDTrackCount) {
		CurrentCDTrack = (CurrentCDTrack + 1) % CDTrackCount;
		Sound::PlayCDTrack(Sequence[CurrentCDTrack] + 1);

		// Show the information about this track for some seconds.
		CaptionTimer = SHOW_CAPTION_TICKS;
	}
}


void	Update(const UpdateState& u)
// Change the mixer volume, play the next track, or whatever else needs
// doing w/r/t background music.
{
	if (!IsOpen) return;
	
	// Volume fades.
	if (FaderTimer) {
		FaderTimer -= u.DeltaTicks;
		if (FaderTimer < 0) FaderTimer = 0;

		float	f = 1 - float(FaderTimer) / TotalFadeTime;

		CurrentVolume = uint8(FadeStartVolume + (FadeEndVolume - FadeStartVolume) * f);
		Sound::SetMusicVolume(CurrentVolume);
	}

	// Timer for caption fade-in and fade-out.
	if (CaptionTimer) {
		CaptionTimer -= u.DeltaTicks;
		if (CaptionTimer < 0) CaptionTimer = 0;
	}
	
	// Monitor the music-playing status, periodically.
	if (u.Ticks - LastCheckedCD > 1000) {
#ifdef LINUX
		LastCheckedCD = u.Ticks;

		if (CachedCDMode == Sound::CD_NOT_READY) {
			CachedCDMode = Sound::GetCDMode();

			if (CachedCDMode != Sound::CD_NOT_READY) {
				// New disc inserted.  Examine it for track info.
				GetDiscInfo();
			}
		}
		if (CachedCDMode != Sound::CD_NOT_READY && CDTrackCount > 0) {
#else // not LINUX
		if (CachedCDMode != Sound::CD_NOT_READY && CachedCDMode != Sound::CD_PLAY && CDTrackCount > 0) {
#endif // not LINUX
			LastCheckedCD = u.Ticks;
			
			Sound::CDMode	cdm = Sound::GetCDMode();
			CachedCDMode = cdm;
			if (cdm == Sound::CD_PLAY) {
				TimeMusicLastPlayed = u.Ticks;
			}
			
			int	QuietTime = u.Ticks - TimeMusicLastPlayed;
			
			if (cdm != Sound::CD_NOT_READY && FaderState != FADER_OUT) {
				// Keep the music playing, if it's available and audible.
				if (QuietTime > 1000) {
				  // Play next track.
				  PlayNext();
				}
			}
		}
	}

	if (CachedCDMode != Sound::CD_NOT_READY && CDTrackCount > 0) {
		if (Input::CheckForEventDown(Input::F3)) {
			// Play previous track.
			CurrentCDTrack -= 2;
			while (CurrentCDTrack < 0) CurrentCDTrack += CDTrackCount;
			PlayNext();
		} else if (Input::CheckForEventDown(Input::F4)) {
			// Play next track.
			PlayNext();
		} else if (Input::CheckForEventDown(Input::F2)) {
			// Show track info.
			CaptionTimer = SHOW_CAPTION_TICKS;
		}
	}
}


void	Render(const ViewState& s)
// Draw a track caption overlay after a new track starts, with the artist,
// song title, and comments.
{
	if (CaptionTimer) {
		uint32	alpha = 0xFF000000;	// xxx Fade in/out using CaptionTimer...
		if (CaptionTimer > SHOW_CAPTION_TICKS - 1000) {
			alpha = int((SHOW_CAPTION_TICKS - float(CaptionTimer)) / 1000.0 * 255.0) << 24;
		} else if (CaptionTimer < 1000) {
			alpha = int(float(CaptionTimer) / 1000.0 * 255) << 24;
		}
		uint32	shadow_alpha = uint32(((alpha >> 24) / 255.0) * 96.0) << 24;

		float	dy = float(Text::GetFontHeight(Text::DEFAULT) + 3);
		float	y = 470 - dy * 3;
		float	x = 13;

		int	track = Sequence[CurrentCDTrack];
		TrackData&	info = TrackInfo[track];
		
		Text::DrawString(int(x-2), int(y+2), Text::DEFAULT, Text::ALIGN_LEFT, info.Song, shadow_alpha | 0x202020);
		Text::DrawString(int(x), int(y), Text::DEFAULT, Text::ALIGN_LEFT, info.Song, alpha | 0xB9D2FF);
		y += dy;

		Text::DrawString(int(x-2), int(y+2), Text::DEFAULT, Text::ALIGN_LEFT, info.Artist, shadow_alpha | 0x202020);
		Text::DrawString(int(x), int(y), Text::DEFAULT, Text::ALIGN_LEFT, info.Artist, alpha | 0xB9D2FF);
		y += dy;

		Text::DrawString(int(x-2), int(y+2), Text::DEFAULT, Text::ALIGN_LEFT, info.Comment[0], shadow_alpha | 0x202020);
		Text::DrawString(int(x), int(y), Text::DEFAULT, Text::ALIGN_LEFT, info.Comment[0], alpha | 0xB9D2FF);
		y += dy;

		Text::DrawString(int(x-2), int(y+2), Text::DEFAULT, Text::ALIGN_LEFT, info.Comment[1], shadow_alpha | 0x202020);
		Text::DrawString(int(x), int(y), Text::DEFAULT, Text::ALIGN_LEFT, info.Comment[1], alpha | 0xB9D2FF);
		y += dy;
	}
}


void	CDCheckEntry()
// Called by OS wrapper when CD may have been inserted.
{
	if (!IsOpen) return;
	
	if (CachedCDMode == Sound::CD_NOT_READY) {
		CachedCDMode = Sound::GetCDMode();

		if (CachedCDMode != Sound::CD_NOT_READY) {
			// New disc inserted.  Examine it for track info.
			GetDiscInfo();
		}
	}
}


void	CDNotify()
// Called by OS wrapper when our track may have stopped playing.
{
	if (!IsOpen) return;
	
	if (CachedCDMode == Sound::CD_PLAY) {
		CachedCDMode = Sound::GetCDMode();
	}
}


void	Open()
// Initialize music module.
{
	if (Config::GetBoolValue("Music") == false) return;
	
	Sound::SetMusicVolume(0/*FadeDownVolume*/);

	FadeUpVolume = uint8(Config::GetFloat("CDAudioVolume") * 255.0 / 100.0);
	FadeDownVolume = uint8(FadeUpVolume * FADE_DOWN_FACTOR);

	lua_register("music_play_next", PlayNext);

	IsOpen = true;

	// Get initial disc track info.
	GetDiscInfo();
}


void	Close()
// Shut down the music module.
{
	IsOpen = false;
}


void	FadeUp()
// Signal to fade the music volume up.
{
	if (!IsOpen) return;
	
	if (FaderState != FADER_UP) {
		FadeStartVolume = FaderState == FADER_OUT ? 0 : FadeDownVolume;
		FadeEndVolume = FadeUpVolume;
		FaderTimer = TotalFadeTime;
		FaderState = FADER_UP;
	}
}


void	FadeDown()
// Signal to fade the music volume down to a quieter volume.
{
	if (!IsOpen) return;
	
	if (FaderState != FADER_DOWN) {
		FadeStartVolume = FaderState == FADER_OUT ? 0 : FadeUpVolume;
		FadeEndVolume = FadeDownVolume;
		FaderTimer = TotalFadeTime;
		FaderState = FADER_DOWN;
	}
}


void	FadeOut()
// Signal to fade the music to silence.
{
	if (!IsOpen) return;
	
	if (FaderState != FADER_OUT) {
		FadeStartVolume = FaderState == FADER_DOWN ? FadeDownVolume : FadeUpVolume;
		FadeEndVolume = 0;
		FaderTimer = TotalFadeTime;
		FaderState = FADER_OUT;
	}
}


void	SetMaxVolume(uint8 vol)
// Sets the maximum CD-Audio volume, which corresponds to the
// Fade-Up volume.  This also scales the Fade Down volume.
{
	FadeUpVolume = vol;
	FadeDownVolume = uint8(FadeUpVolume * FADE_DOWN_FACTOR);

	FadeStartVolume = CurrentVolume;
	if (FaderState == FADER_OUT) {
		FadeEndVolume = 0;
	} else if (FaderState == FADER_DOWN) {
		FadeEndVolume = FadeDownVolume;
	} else {
		FadeEndVolume = FadeUpVolume;
	}
	FaderTimer = 50;
}


};	// end namespace Music

