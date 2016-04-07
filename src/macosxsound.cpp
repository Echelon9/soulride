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
// linuxsound.hpp	-thatcher 1/28/2001 Copyright Thatcher Ulrich
// macosxsound.cpp      Bjorn Leffler 8/28/2003

// Interface code to Mac OS X sound stuff.  Implements the interface defined by the Sound namespace.


#include <SDL.h>
#include <SDL_mixer.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>

#include "utility.hpp"
#include "sound.hpp"
#include "config.hpp"
#include "console.hpp"

// Playback of mp3/ogg files stuff
#include <vector>
#include <string>
#include <iostream>

namespace Sound {

bool	IsOpen = false;


unsigned long	OutputFrequency = 22050;	// Samples/sec of the output channel.

// cd interface.
namespace cd {
	void	open();
	void	play(int track);
	void	pause();
	void	stop();
	void	close();
	CDMode	get_mode();
	int	track_count();
	void	get_drive_name(char* result);
};


// mixer interface.
void	mixer_open();
void	mixer_close();
void	mixer_setcdvolume(uint8 vol);
void	mixer_setwavevolume(uint8 vol);
void	mixer_setmastervolume(uint8 vol);


BagOf<Mix_Chunk*>	Buffers;

	
void	Open()
// Open the Sound:: interface.
{
	if (Config::GetBoolValue("Sound") == false) return;

	int	result;
	//result = Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024);
	result = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
	if (result < 0) {
		printf("Mix_OpenAudio returned %d, %s\n", result, SDL_GetError());
		return;
	}
	
	mixer_open();
	cd::open();
	
	IsOpen = true;
	
	// Set initial volume levels if they're specified.
	if (Config::GetValue("MasterVolume")) SetMasterVolume(uint8(Config::GetFloat("MasterVolume") * 255.0f / 100.0f));
	if (Config::GetValue("SFXVolume")) SetSFXVolume(uint8(Config::GetFloat("SFXVolume") * 255.0f / 100.0f));
}


void	Close()
// Close the Sound:: interface.
{
	if (!IsOpen) return;
	
	cd::stop();
	cd::close();
	mixer_close();
	
	Mix_CloseAudio();

	IsOpen = false;
}


void	SetMasterVolume(uint8 vol)
// Sets the master volume level.
{
  mixer_setmastervolume(vol);
}


void	SetSFXVolume(uint8 vol)
// Sets the sound effects volume level.
{
  mixer_setwavevolume(vol);
  //Bjorn alListenerf(AL_GAIN, 0.25 / 255.0 * vol);
}


// Keep a list of sources which are currently active.  Periodically
// check for played-out sources, and delete them.  A played-out sound
// is a non-looped sound which has reached the end of its buffer.
const int LIVE_SOURCE_CT = 100;
int	LiveSources[LIVE_SOURCE_CT];
int	NextSourceIndex = 0;


// Natural base of logarithms.
#define	LN_BASE	2.718281828

int	sdl_volume(float vol)
// Given a volume level from 0 to 1, returns an SDL_mixer volume level
// from 0 to 127.  Un-does the odious logarithmic mapping.
{
  if (vol < 0.001) {
    return 0;
  } else {
    return int(((exp(vol) - 1) / (LN_BASE - 1)) * 127);
  }
}

int macosx_cd_status = CD_STOP;


/**
 * Plays the sound specified by the given resource name.
 *
 * @param ResourceName Filename
 * @param Parameters   Controls to specific parameters
 *
 * @return An "event id" for the sound event started by this call. This id can
 * be used later in calls to Sound::Adjust() or Sound::Release() to change the
 * sound's parameters.
 * @return 0 on failure.
 */
int
Play(const char* ResourceName, const Controls& Parameters)
{
	if (!IsOpen) return 0;

	Mix_Chunk*	chunk;
	bool	got_it = Buffers.GetObject(&chunk, ResourceName);
	if (!got_it) {
		// Need to load and initialize the resource.
		chunk = Mix_LoadWAV(ResourceName);
		if (chunk == NULL) return 0;

		// Remember this chunk.
		Buffers.Add(chunk, ResourceName);
	}
	int	channel = Mix_PlayChannel(-1, chunk, Parameters.Looped ? -1 : 0);
	if (channel >= 0) {
		Mix_Volume(channel, sdl_volume(Parameters.Volume));
		// pan
		// pitch
	}

	return channel + 1;
}


void	Adjust(int EventID, const Controls& NewParameters)
// Use this call to change the parameters of a sound that's currently playing.
{
	if (!IsOpen) return;
	if (EventID == 0) return;

	int	channel = EventID - 1;
	Mix_Volume(channel, sdl_volume(NewParameters.Volume));

	//xxxxxxxx
	float	newvol;
	// Bjorn alGetSourcefv(EventID, AL_GAIN_LINEAR_LOKI, &newvol);
	// printf("new vol = %f\n", newvol);//xxxxxxxx
}


void	Release(int EventID)
// Use this call to stop a playing sound, and release its channel.
{
	if (!IsOpen) return;
	if (EventID == 0) return;

	int	channel = EventID - 1;
	Mix_HaltChannel(channel);
}


int	GetStatus(int EventID)
// Returns status information about the specified sound event.
{
	if (!IsOpen) return 0;
	if (EventID == 0) return 0;

	int	channel = EventID - 1;
	if (Mix_Playing(channel)) return 1;
	return 0;
}


CDMode	GetCDMode()
// Returns the current mode of the cd player.
{

  // find out about music status
  if (Mix_PlayingMusic())
    return CD_PLAY;
  else
    return CD_STOP;
  
  //return cd::get_mode();
}


int	GetCDTrackCount()
// Returns the number of available audio tracks on the current cd.
{
  return MacOSX::GetMusicCount("../music");
  //return cd::track_count();
}

void	GetCDDriveName(char* result)
// Returns the drive name of the cd drive.
{
  //cd::get_drive_name(result);
}


void	PlayCDTrack(int TrackID)
// Plays the specified cd-audio track.  Track numbering starts with 1.
{
  //cd::play(TrackID);
  PlayMusicFile(TrackID);
}

Mix_Music *music = NULL;

void PlayMusicFile(int TrackID){

  char *music_directory = "../music/";

  std::vector<std::string> song = MacOSX::GetMusicFiles(music_directory);

  // chaning 1...n to 0...n-1 (C++ vector index)
  TrackID--;

  // This shouldn't happen, but just in case...
  if (TrackID >= song.size())
    TrackID = 0;

  std::cout << "Playing music file " 
	    <<  TrackID << ": " 
	    << song[TrackID] << std::endl;
  
  std::string filename = music_directory + song[TrackID];
  
  // load the song
  if(!(music=Mix_LoadMUS(filename.c_str())))
    std::cout << "Could not open music file " 
	      << filename << ": " 
	      << Mix_GetError() << std::endl;
  
  // Play the song
  if(Mix_PlayMusic(music, 1)==-1){
    std::cout << "Problems playing music" << std::endl;
    macosx_cd_status = CD_STOP;
  }
  else
    macosx_cd_status = CD_PLAY;

}

void	SetMusicVolume(uint8 vol)
// Sets the volume control of the CD-Audio output.
{
  mixer_setcdvolume(vol);
}


//
// cd stuff
//

namespace cd {

bool	CDIsOpen = false;
SDL_CD*	cdrom = NULL;


void	open()
// Tries to open the CD player.
{
// 	if (CDIsOpen) return;

// 	// Pick a cd device, and open it.
// 	int	cd_to_open = 0;
// 	const char*	devname = Config::GetValue("CDAudioDevice");
// 	if (devname) {
// 		int	cd_count = SDL_CDNumDrives();
// 		int	i;
// 		for (i = 0; i < cd_count; i++) {
// 			if (strcmp(SDL_CDName(i), devname) == 0) {
// 				cd_to_open = i;
// 				break;
// 			}
// 		}
// 	}
// 	cdrom = SDL_CDOpen(cd_to_open);

// 	if (cdrom == NULL) {
// 		// Failed to open.  Return without setting CDIsOpen.
// 		return;
// 	}

// 	CDIsOpen = true;
}


void	play(int track)
// Play the specified track.
{
// 	if (CDIsOpen == false) return;

// 	if (cdrom == NULL) return;
// 	if (CD_INDRIVE(SDL_CDStatus(cdrom))) {
// 		int	index = track-1;
// 		if (cdrom->numtracks && cdrom->track[0].type == SDL_DATA_TRACK) {
// 			index++;
// 		}
// 		SDL_CDPlayTracks(cdrom, index, 0, 1, 0);
// 	}

  macosx_cd_status = CD_PLAY;

}


void	pause()
// Pause the CD audio player.
{
	// if (CDIsOpen == false) return;

// 	if (cdrom == NULL) return;
// 	SDL_CDPause(cdrom);

  macosx_cd_status = CD_PAUSE;

}


void	stop()
// Stop playing CD audio.
{
// 	if (CDIsOpen == false) return;

// 	if (cdrom == NULL) return;

// 	SDL_CDStop(cdrom);

  macosx_cd_status = CD_STOP;

}


void	close()
// Close the CD audio interface.
{
// 	if (!CDIsOpen) return;

// 	if (cdrom) {
// 		SDL_CDClose(cdrom);
// 		cdrom = NULL;
// 	}

// 	CDIsOpen = false;

  macosx_cd_status = CD_STOP;

}


CDMode	get_mode()
// Returns the current mode (i.e. CD_NOT_READY, CD_PLAY, CD_STOP, CD_PAUSE, etc).
{
// 	if (CDIsOpen == false) return CD_NOT_READY;

// 	if (cdrom == NULL) return CD_NOT_READY;

// 	CDstatus	s = SDL_CDStatus(cdrom);
// 	if (s <= 0) return CD_STOP;

// 	if (s == CD_STOPPED) return CD_STOP;
// 	if (s == CD_PLAYING) return CD_PLAY;
// 	if (s == CD_PAUSED) return CD_PAUSE;

  return CD_NOT_READY;
}


int	track_count()
// Returns the number of available audio tracks on the cd.
{
// 	if (!CDIsOpen) return 0;

// 	if (cdrom) {
// 		int	count = cdrom->numtracks;
// 		if (cdrom->numtracks && cdrom->track[0].type == SDL_DATA_TRACK) {
// 			// If the first track is data, don't count it as a track.
// 			count--;
// 		}
// 		return count;
// 	} else {
// 		return 0;
// 	}
  return 0;
}


void	get_drive_name(char* result)
// Fills result[] with the name of the cd-audio drive (for reading
// data).  Sets to null string if name can't be obtained.
{
// 	result[0] = 0;

// 	if (!CDIsOpen) return;

// 	strcpy(result, Config::GetValue("CDAudioMountPoint"));

// 	// Make sure there's a trailing /.
// 	if (result[strlen(result)-1] != '/') strcat(result, "/");
// 
}


};	// end namespace cd


// #include <sys/soundcard.h>


bool	MixerIsOpen = false;
int	mixerfd = 0;

int	OriginalMaster = 0;
int	OriginalWave = 0;
int	OriginalCD = 0;


void	mixer_open()
// Open the mixer interface.
{
// 	if (MixerIsOpen) return;

// 	mixerfd = open("/dev/mixer", O_RDWR);
// 	if (mixerfd < 0) {
// 		// Failure.  Just give up without marking the mixer as open.
// 		return;
// 	}

// 	ioctl(mixerfd, SOUND_MIXER_READ_VOLUME, &OriginalMaster);
// 	ioctl(mixerfd, SOUND_MIXER_READ_PCM, &OriginalWave);
// 	ioctl(mixerfd, SOUND_MIXER_READ_CD, &OriginalCD);

// 	MixerIsOpen = true;
}


void	mixer_close()
{
// 	if (!MixerIsOpen) return;

// 	ioctl(mixerfd, SOUND_MIXER_WRITE_VOLUME, &OriginalMaster);
// 	ioctl(mixerfd, SOUND_MIXER_WRITE_PCM, &OriginalWave);
// 	ioctl(mixerfd, SOUND_MIXER_WRITE_CD, &OriginalCD);

// 	close(mixerfd);

// 	MixerIsOpen = false;
}


void	mixer_setcdvolume(uint8 vol)
// Sets the volume on CD-Audio mixer lines to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
  //if (!MixerIsOpen) return;

 	//int	percent = (vol * 100) / 255;
 	//int	arg = percent | (percent << 8);	// Left channel in low byte, right in high byte.
 	//ioctl(mixerfd, SOUND_MIXER_WRITE_CD, &arg);
}


void	mixer_setwavevolume(uint8 vol)
// Sets the volume on the WAVE audio lines to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
// 	if (!MixerIsOpen) return;

// 	int	percent = (vol * 100) / 255;
// 	int	arg = percent | (percent << 8);	// Left channel in low byte, right in high byte.
// 	ioctl(mixerfd, SOUND_MIXER_WRITE_PCM, &arg);
}


void	mixer_setmastervolume(uint8 vol)
// Sets the master speaker volume to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
  //	if (!MixerIsOpen) return;

// 	int	percent = (vol * 100) / 255;
// 	int	arg = percent | (percent << 8);	// Left channel in low byte, right in high byte.
//       	ioctl(mixerfd, SOUND_MIXER_WRITE_VOLUME, &arg);
}


};	// End namespace Sound

