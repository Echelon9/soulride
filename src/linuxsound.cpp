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
// linuxsound.hpp	-thatcher 1/28/2001 Copyright Thatcher Ulrich

// Interface code to Linux sound stuff.  Implements the interface defined by the Sound namespace.


#include <SDL.h>
#include <SDL_mixer.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <errno.h>
#include <math.h>

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif



#include "utility.hpp"
#include "sound.hpp"
#include "config.hpp"
#include "console.hpp"


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
	result = Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 1024);
	if (result < 0) {
		printf("Mix_OpenAudio returned %d, %s\n", result, SDL_GetError());
		return;
	}
	
	mixer_open();
	// cd::open();
	
	IsOpen = true;

	// Set initial volume levels if they're specified.
	if (Config::GetValue("MasterVolume")) SetMasterVolume(uint8(Config::GetFloat("MasterVolume") * 255.0f / 100.0f));
	if (Config::GetValue("SFXVolume")) SetSFXVolume(uint8(Config::GetFloat("SFXVolume") * 255.0f / 100.0f));
}


void	Close()
// Close the Sound:: interface.
{
	if (!IsOpen) return;
	
	// cd::stop();
	// cd::close();
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
//	alListenerf(AL_GAIN, 0.25 / 255.0 * vol);
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

//	//xxxxxxxx
//	float	newvol;
//	alGetSourcefv(EventID, AL_GAIN_LINEAR_LOKI, &newvol);
//	printf("new vol = %f\n", newvol);//xxxxxxxx
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
	// return cd::get_mode();
}


int	GetCDTrackCount()
// Returns the number of available audio tracks on the current cd.
{
	// return cd::track_count();
	return 0;
}

void	GetCDDriveName(char* result)
// Returns the drive name of the cd drive.
{
	// cd::get_drive_name(result);
}


void	PlayCDTrack(int TrackID)
// Plays the specified cd-audio track.  Track numbering starts with 1.
{
	// cd::play(TrackID);
}


void	SetMusicVolume(uint8 vol)
// Sets the volume control of the CD-Audio output.
{
	mixer_setcdvolume(vol);
}



#if 0


//
// some utility functions for loading .WAV files.  Stolen from MS DirectX samples.
//


int WaveOpenFile(
	char *pszFileName,                              // (IN)
	HMMIO *phmmioIn,                                // (OUT)
	WAVEFORMATEX **ppwfxInfo,                       // (OUT)
	MMCKINFO *pckInRIFF                             // (OUT)
			)
/* This function will open a wave input file and prepare it for reading,
 * so the data can be easily
 * read with WaveReadFile. Returns 0 if successful, the error code if not.
 *      pszFileName - Input filename to load.
 *      phmmioIn    - Pointer to handle which will be used
 *          for further mmio routines.
 *      ppwfxInfo   - Ptr to ptr to WaveFormatEx structure
 *          with all info about the file.                        
 *      
*/
{
	HMMIO           hmmioIn;
	MMCKINFO        ckIn;           // chunk info. for general use.
	PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       
	WORD            cbExtraAlloc;   // Extra bytes for waveformatex 
	int             nError;         // Return value.


	// Initialization...
	*ppwfxInfo = NULL;
	nError = 0;
	hmmioIn = NULL;
	
	if ((hmmioIn = mmioOpen(pszFileName, NULL, MMIO_ALLOCBUF | MMIO_READ)) == NULL)
		{
		nError = -1 /* ER_CANNOTOPEN */;
		goto ERROR_READING_WAVE;
		}

	if ((nError = (int)mmioDescend(hmmioIn, pckInRIFF, NULL, 0)) != 0)
		{
		goto ERROR_READING_WAVE;
		}


	if ((pckInRIFF->ckid != FOURCC_RIFF) || (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')))
		{
		nError = -1 /* ER_NOTWAVEFILE */;
		goto ERROR_READING_WAVE;
		}
			
	/* Search the input file for for the 'fmt ' chunk.     */
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if ((nError = (int)mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
		{
		goto ERROR_READING_WAVE;                
		}
					
	/* Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    * if there are extra parameters at the end, we'll ignore them */
    
    if (ckIn.cksize < (long) sizeof(PCMWAVEFORMAT))
		{
		nError = -1 /* ER_NOTWAVEFILE */;
		goto ERROR_READING_WAVE;
		}
															
	/* Read the 'fmt ' chunk into <pcmWaveFormat>.*/     
    if (mmioRead(hmmioIn, (HPSTR) &pcmWaveFormat, (long) sizeof(pcmWaveFormat)) != (long) sizeof(pcmWaveFormat))
		{
		nError = -1 /* ER_CANNOTREAD */;
		goto ERROR_READING_WAVE;
		}
							

	// Ok, allocate the waveformatex, but if its not pcm
	// format, read the next word, and thats how many extra
	// bytes to allocate.
	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
		cbExtraAlloc = 0;                               
							
	else
		{
		// Read in length of extra bytes.
		if (mmioRead(hmmioIn, (LPSTR) &cbExtraAlloc,
			(long) sizeof(cbExtraAlloc)) != (long) sizeof(cbExtraAlloc))
			{
			nError = -1 /* ER_CANNOTREAD */;
			goto ERROR_READING_WAVE;
			}

		}
							
	// Ok, now allocate that waveformatex structure.
	if ((*ppwfxInfo = (struct tWAVEFORMATEX*) GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX)+cbExtraAlloc)) == NULL)
		{
		nError = -1 /* ER_MEM */;
		goto ERROR_READING_WAVE;
		}

	// Copy the bytes from the pcm structure to the waveformatex structure
	memcpy(*ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat));
	(*ppwfxInfo)->cbSize = cbExtraAlloc;

	// Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
	if (cbExtraAlloc != 0)
		{
		if (mmioRead(hmmioIn, (LPSTR) (((BYTE*)&((*ppwfxInfo)->cbSize))+sizeof(cbExtraAlloc)),
			(long) (cbExtraAlloc)) != (long) (cbExtraAlloc))
			{
			nError = -1 /* ER_NOTWAVEFILE */;
			goto ERROR_READING_WAVE;
			}
		}

	/* Ascend the input file out of the 'fmt ' chunk. */                                                            
	if ((nError = mmioAscend(hmmioIn, &ckIn, 0)) != 0)
		{
		goto ERROR_READING_WAVE;

		}
	

	goto TEMPCLEANUP;               

ERROR_READING_WAVE:
	if (*ppwfxInfo != NULL)
		{
		GlobalFree(*ppwfxInfo);
		*ppwfxInfo = NULL;
		}               

	if (hmmioIn != NULL)
	{
	mmioClose(hmmioIn, 0);
		hmmioIn = NULL;
		}
	
TEMPCLEANUP:
	*phmmioIn = hmmioIn;

	return(nError);

}

/*      This routine has to be called before WaveReadFile as it searchs for the chunk to descend into for
	reading, that is, the 'data' chunk.  For simplicity, this used to be in the open routine, but was
	taken out and moved to a separate routine so there was more control on the chunks that are before
	the data chunk, such as 'fact', etc... */

int WaveStartDataRead(
					HMMIO *phmmioIn,
					MMCKINFO *pckIn,
					MMCKINFO *pckInRIFF
					)
{
	int                     nError;

	nError = 0;
	
	// Do a nice little seek...
	if ((nError = mmioSeek(*phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC), SEEK_SET)) == -1)
	{
//		Assert(FALSE);
	}

	nError = 0;
	//      Search the input file for for the 'data' chunk.
	pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
	if ((nError = mmioDescend(*phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
		{
		goto ERROR_READING_WAVE;
		}

	goto CLEANUP;

ERROR_READING_WAVE:

CLEANUP:        
	return(nError);
}


/*      This will read wave data from the wave file.  Makre sure we're descended into
	the data chunk, else this will fail bigtime!
	hmmioIn         - Handle to mmio.
	cbRead          - # of bytes to read.   
	pbDest          - Destination buffer to put bytes.
	cbActualRead- # of bytes actually read.

		

*/


int WaveReadFile(
		HMMIO hmmioIn,                          // IN
		UINT cbRead,                            // IN           
		BYTE *pbDest,                           // IN
		MMCKINFO *pckIn,                        // IN.
		UINT *cbActualRead                      // OUT.
		
		)
{

	MMIOINFO    mmioinfoIn;         // current status of <hmmioIn>
	int                     nError;
	UINT                    cT, cbDataIn, uCopyLength;

	nError = 0;

	if (nError = mmioGetInfo(hmmioIn, &mmioinfoIn, 0) != 0)
		{
		goto ERROR_CANNOT_READ;
		}
				
	cbDataIn = cbRead;
	if (cbDataIn > pckIn->cksize) 
		cbDataIn = pckIn->cksize;       

	pckIn->cksize -= cbDataIn;
	
	for (cT = 0; cT < cbDataIn; )
		{
		/* Copy the bytes from the io to the buffer. */
		if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
			{
	    if ((nError = mmioAdvance(hmmioIn, &mmioinfoIn, MMIO_READ)) != 0)
				{
		goto ERROR_CANNOT_READ;
				} 
	    if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
				{
				nError = -1 /* ER_CORRUPTWAVEFILE */;
		goto ERROR_CANNOT_READ;
				}
			}
			
		// Actual copy.
		uCopyLength = (UINT)(mmioinfoIn.pchEndRead - mmioinfoIn.pchNext);
		if((cbDataIn - cT) < uCopyLength )
			uCopyLength = cbDataIn - cT;
		memcpy( (BYTE*)(pbDest+cT), (BYTE*)mmioinfoIn.pchNext, uCopyLength );
		cT += uCopyLength;
		mmioinfoIn.pchNext += uCopyLength;
		}

	if ((nError = mmioSetInfo(hmmioIn, &mmioinfoIn, 0)) != 0)
		{
		goto ERROR_CANNOT_READ;
		}

	*cbActualRead = cbDataIn;
	goto FINISHED_READING;

ERROR_CANNOT_READ:
	*cbActualRead = 0;

FINISHED_READING:
	return(nError);

}

/*      This will close the wave file openned with WaveOpenFile.  
	phmmioIn - Pointer to the handle to input MMIO.
	ppwfxSrc - Pointer to pointer to WaveFormatEx structure.

	Returns 0 if successful, non-zero if there was a warning.

*/
int WaveCloseReadFile(
			HMMIO *phmmio,                                  // IN
			WAVEFORMATEX **ppwfxSrc                 // IN
			)
{

	if (*ppwfxSrc != NULL)
		{
		GlobalFree(*ppwfxSrc);
		*ppwfxSrc = NULL;
		}

	if (*phmmio != NULL)
		{
		mmioClose(*phmmio, 0);
		*phmmio = NULL;
		}

	return(0);

}


#endif // 0


//
// cd stuff
//

/**
namespace cd {
;

bool	CDIsOpen = false;
SDL_CD*	cdrom = NULL;


void	open()
// Tries to open the CD player.
{
	if (CDIsOpen) return;

	// Pick a cd device, and open it.
	int	cd_to_open = 0;
	const char*	devname = Config::GetValue("CDAudioDevice");
	if (devname) {
		int	cd_count = SDL_CDNumDrives();
		int	i;
		for (i = 0; i < cd_count; i++) {
			if (strcmp(SDL_CDName(i), devname) == 0) {
				cd_to_open = i;
				break;
			}
		}
	}
	cdrom = SDL_CDOpen(cd_to_open);

	if (cdrom == NULL) {
		// Failed to open.  Return without setting CDIsOpen.
		return;
	}

	CDIsOpen = true;
}


void	play(int track)
// Play the specified track.
{
	if (CDIsOpen == false) return;

	if (cdrom == NULL) return;
	if (CD_INDRIVE(SDL_CDStatus(cdrom))) {
		int	index = track-1;
		if (cdrom->numtracks && cdrom->track[0].type == SDL_DATA_TRACK) {
			index++;
		}
		SDL_CDPlayTracks(cdrom, index, 0, 1, 0);
	}
}


void	pause()
// Pause the CD audio player.
{
	if (CDIsOpen == false) return;

	if (cdrom == NULL) return;
	SDL_CDPause(cdrom);
}


void	stop()
// Stop playing CD audio.
{
	if (CDIsOpen == false) return;

	if (cdrom == NULL) return;

	SDL_CDStop(cdrom);
}


void	close()
// Close the CD audio interface.
{
	if (!CDIsOpen) return;

	if (cdrom) {
		SDL_CDClose(cdrom);
		cdrom = NULL;
	}

	CDIsOpen = false;
}


CDMode	get_mode()
// Returns the current mode (i.e. CD_NOT_READY, CD_PLAY, CD_STOP, CD_PAUSE, etc).
{
	if (CDIsOpen == false) return CD_NOT_READY;

	if (cdrom == NULL) return CD_NOT_READY;

	CDstatus	s = SDL_CDStatus(cdrom);
	if (s <= 0) return CD_STOP;

	if (s == CD_STOPPED) return CD_STOP;
	if (s == CD_PLAYING) return CD_PLAY;
	if (s == CD_PAUSED) return CD_PAUSE;

	return CD_NOT_READY;
}


int	track_count()
// Returns the number of available audio tracks on the cd.
{
	if (!CDIsOpen) return 0;

	if (cdrom) {
		int	count = cdrom->numtracks;
		if (cdrom->numtracks && cdrom->track[0].type == SDL_DATA_TRACK) {
			// If the first track is data, don't count it as a track.
			count--;
		}
		return count;
	} else {
		return 0;
	}
}


void	get_drive_name(char* result)
// Fills result[] with the name of the cd-audio drive (for reading
// data).  Sets to null string if name can't be obtained.
{
	result[0] = 0;

	if (!CDIsOpen) return;

	strcpy(result, Config::GetValue("CDAudioMountPoint"));

	// Make sure there's a trailing /.
	if (result[strlen(result)-1] != '/') strcat(result, "/");
}


};	// end namespace cd
*/


#ifdef NOT

MCI_STATUS
   MCI_TRACK | (MCI_STATUS_POSITION | MCI_STATUS_LENGTH),
   dwTrack = track number;


MCI_STATUS
   MCI_STATUS_ITEM | MCI_TRACK
   dwTrack = track number;
   dwItem = MCI_CDA_STATUS_TYPE_TRACK,
   mci sets dwReturn to (MCI_CDA_TRACK_AUDIO | MCI_CDA_TRACK_OTHER);

   
   MCI_STATUS_MEDIA_PRESENT
   dwReturn <-- TRUE | FALSE


   MCI_STATUS_NUMBER_OF_TRACKS 
   dwReturn <-- total number of playable tracks


   MCI_STATUS_MODE 
   dwReturn <--
	   MCI_MODE_NOT_READY
	   MCI_MODE_PAUSE
	   MCI_MODE_PLAY
	   MCI_MODE_STOP
	   MCI_MODE_OPEN
	   MCI_MODE_RECORD
	   MCI_MODE_SEEK 


BOOL mciGetErrorString( DWORD fdwError, 
LPTSTR lpszErrorText, 
UINT cchErrorText 
); 
 

BYTE MCI_TMSF_FRAME(DWORD dwTMSF); // _MINUTE, _SECOND, _TRACK
 
DWORD MCI_MAKE_TMSF(BYTE tracks, BYTE minutes, BYTE seconds, BYTE frames); 
 


#endif // NOT


#include <sys/soundcard.h>


bool	MixerIsOpen = false;
int	mixerfd = 0;

int	OriginalMaster = 0;
int	OriginalWave = 0;
int	OriginalCD = 0;


void	mixer_open()
// Open the mixer interface.
{
	if (MixerIsOpen) return;

	mixerfd = open("/dev/mixer", O_RDWR);
	if (mixerfd < 0) {
		// Failure.  Just give up without marking the mixer as open.
		return;
	}

	ioctl(mixerfd, SOUND_MIXER_READ_VOLUME, &OriginalMaster);
	ioctl(mixerfd, SOUND_MIXER_READ_PCM, &OriginalWave);
	ioctl(mixerfd, SOUND_MIXER_READ_CD, &OriginalCD);

	MixerIsOpen = true;
}


void	mixer_close()
{
	if (!MixerIsOpen) return;

	ioctl(mixerfd, SOUND_MIXER_WRITE_VOLUME, &OriginalMaster);
	ioctl(mixerfd, SOUND_MIXER_WRITE_PCM, &OriginalWave);
	ioctl(mixerfd, SOUND_MIXER_WRITE_CD, &OriginalCD);

	close(mixerfd);

	MixerIsOpen = false;
}


void	mixer_setcdvolume(uint8 vol)
// Sets the volume on CD-Audio mixer lines to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
	if (!MixerIsOpen) return;

	int	percent = (vol * 100) / 255;
	int	arg = percent | (percent << 8);	// Left channel in low byte, right in high byte.
	ioctl(mixerfd, SOUND_MIXER_WRITE_CD, &arg);
}


void	mixer_setwavevolume(uint8 vol)
// Sets the volume on the WAVE audio lines to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
	if (!MixerIsOpen) return;

	int	percent = (vol * 100) / 255;
	int	arg = percent | (percent << 8);	// Left channel in low byte, right in high byte.
	ioctl(mixerfd, SOUND_MIXER_WRITE_PCM, &arg);
}


void	mixer_setmastervolume(uint8 vol)
// Sets the master speaker volume to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
	if (!MixerIsOpen) return;

	int	percent = (vol * 100) / 255;
	int	arg = percent | (percent << 8);	// Left channel in low byte, right in high byte.
	ioctl(mixerfd, SOUND_MIXER_WRITE_VOLUME, &arg);
}


};	// End namespace Sound

