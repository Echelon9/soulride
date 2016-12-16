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
// sound.hpp	-thatcher 5/23/1998 Copyright Slingshot

// Interface definition for basic sound facilities.


#ifndef SOUND_HPP
#define SOUND_HPP


#include "utility.hpp"
#include "error.hpp"


namespace Sound {

	// An exception class specific to the Sound:: module.
	class SoundError : public Error {};
	
	void	Open();
	void	Close();

	void	SetMasterVolume(uint8 vol);
	void	SetSFXVolume(uint8 vol);
	
	// A structure for specifying a sound's control parameters.
	struct Controls {
		float	Volume;	// 0.0 == silent, 1.0 == loudest.
		float	Pan;	// -1.0 is full left, 0.0 is center, 1.0 is full right.
		float	Pitch;	// scale factor, 1.0 == normal pitch.
		bool	Looped;	// true for continuous play, false for one-shot.

		Controls(float vol = 1.0, float pan = 0.0, float pitch = 1.0, bool looped = false) {
			Volume = vol;
			Pan = pan;
			Pitch = pitch;
			Looped = looped;	// This may not belong in ControlParameters.  Instead, a param to ::Play()...?
		}
	};
	
	int	Play(const char* ResourceName, const Controls& Parameters);
	void	Adjust(int EventID, const Controls& NewParameters);
	void	Release(int EventID);	// Use this to release a looped sound (or to stop short a discrete sound).
	int	GetStatus(int EventID);
	
	// Sample*	NewSample(filename?);

	// Interface for music.
	enum CDMode {
		CD_NOT_READY,
		CD_PAUSE,
		CD_PLAY,
		CD_STOP,
		CD_OPEN,
		CD_SEEK,
	};

	CDMode	GetCDMode();
	int	GetCDTrackCount();
	void	PlayCDTrack(int TrackID);
	void	SetMusicVolume(uint8 vol);
	void	GetCDDriveName(char* returnbuffer);

// macOS mp3/ogg playback instead of CD
#ifdef MACOSX
	void	PlayMusicFile(int TrackID);
#endif

//	// A callback from the OS, to notify the sound module to check
//	// the state of the CD player.  The point is to avoid having to
//	// poll the player all the time to see if something's changed,
//	// which is slow.
//	void	CDNotify();
	
};	// End namespace Sound


#endif // SOUND_HPP
