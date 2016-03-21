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
// winsound.hpp	-thatcher 5/23/1998 Copyright Slingshot

// Interface code to Windows sound stuff.  Implements the interface defined by the Sound namespace.


#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dsound.h>
#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#include "utility.hpp"
#include "winmain.hpp"
#include "sound.hpp"
#include "config.hpp"
#include "console.hpp"


// Should maybe be an app-wide parameter, in Util:: or something.
#define RESOURCENAMESIZE	80


namespace Sound {


bool	IsOpen = false;


LPDIRECTSOUND	DirectSound = NULL;
unsigned long	OutputFrequency = 22050;	// Samples/sec of the output channel.


static char*	DSErrorToString(HRESULT error);


int	NextEventID = 1;	// Used to assign ids to specific sound events.


// To deal with sample allocation in a somewhat sane manner in the face
// of DirectSound's peculiar way of thinking, the Channel array is sort
// of an amalgamation of a sample table and an array of output channels.
// This is due to the way DS handles secondary buffers: every channel mixed
// into the output needs its own secondary buffer, and every secondary
// buffer must contain sample data, but they do provide a way for secondary
// buffers to share sample data, through DuplicateSoundBuffer.
const int	CHANNELCOUNT = 20;
struct ChannelInfo {
	int	EventID;
	char	ResourceName[RESOURCENAMESIZE];
	bool	Duplicate;	// true if this channel's data has been duplicated from another channel.  Used to aid the replacement policy.

	Controls	ControlParameters;

	LPDIRECTSOUNDBUFFER	DSBuffer;
	
	// Initializing constructor.
	ChannelInfo() {
		EventID = 0;
		ResourceName[0] = 0;
		Duplicate = false;

		DSBuffer = NULL;
		// etc.
	}
} Channel[CHANNELCOUNT];



// interface for reading .WAV files.
int	WaveOpenFile(char *pszFileName, HMMIO *phmmioIn, WAVEFORMATEX **ppwfxInfo, MMCKINFO *pckInRIFF);
int	WaveStartDataRead(HMMIO *phmmioIn, MMCKINFO *pckIn, MMCKINFO *pckInRIFF);
int	WaveReadFile(HMMIO hmmioIn, UINT cbRead, BYTE *pbDest, MMCKINFO *pckIn, UINT *cbActualRead);
int	WaveCloseReadFile(HMMIO *phmmio, WAVEFORMATEX **ppwfxSrc);


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


void	Open()
// Open the Sound:: interface.
{
	if (Config::GetBoolValue("Sound") == false) return;
	
	// Open an interface to DirectSound.
	HRESULT err;
	err = DirectSoundCreate(NULL, &DirectSound, NULL);
	if (err != DS_OK) {
		// Give up, no sound.
		Config::SetBoolValue("Sound", false);
		return;
		
//		SoundError e;
//		e << DSErrorToString(err);
//		throw e;
	}

	// Set the normal cooperative level.
	err = DirectSound->SetCooperativeLevel(Main::GetGameWindow(), DSSCL_NORMAL);
	if (err != DS_OK) {
		// Give up, no sound.
		Config::SetBoolValue("Sound", false);
		DirectSound->Release();
		DirectSound = NULL;
		return;
		
//		SoundError e; e << DSErrorToString(err);
//		throw e;
	}

	// Reset all the sound channels.
	for (int i = 0; i < CHANNELCOUNT; i++) {
		Channel[i].EventID = 0;
	}

	// Query the primary buffer to see what the output frequency is...

	// Set up DSBUFFERDESC structure.
	DSBUFFERDESC	dsbdesc;
	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out. 
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	// Buffer size is determined by sound hardware. 
	dsbdesc.dwBufferBytes = 0; 
	dsbdesc.lpwfxFormat = NULL; // Must be NULL for primary buffers. 

	LPDIRECTSOUNDBUFFER	PrimaryBuffer = NULL;
	err = DirectSound->CreateSoundBuffer(&dsbdesc, &PrimaryBuffer, NULL); 
        if (DS_OK == err) { 
		// Succeeded.  Query the primary buffer.
		PrimaryBuffer->GetFrequency(&OutputFrequency);

		// Make sure the mixer goes continuously.
		PrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

		PrimaryBuffer->Release();	//xxxxxxxxxxx
		
//		//xxxxxxxxxxx
//		char	buf[80];
//		sprintf(buf, "out freq = %d\n", OutputFrequency);
//		Utility::Log(4, buf);
//		//xxxxxxxxxxx
	}

	mixer_open();
	cd::open();
//	CDNotify();	// Initialize our cached CD state.
	
	IsOpen = true;

	// Set initial volume levels if they're specified.
	if (Config::GetValue("MasterVolume")) SetMasterVolume(int(Config::GetFloat("MasterVolume") * 255.0f / 100.0f));
	if (Config::GetValue("SFXVolume")) SetSFXVolume(int(Config::GetFloat("SFXVolume") * 255.0 / 100.0));
}


void	Close()
// Close the Sound:: interface.
{
	if (!IsOpen) return;
	
	if (DirectSound) {
		DirectSound->Release();
		DirectSound = NULL;
	}

	cd::stop();
	cd::close();
	mixer_close();
	
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
}


int	Play(char* ResourceName, const Controls& Parameters)
// Plays the sound specified by the given resource name.  The return value
// is an "event id" for the sound event started by this call.  This id can be
// used later in calls to Sound::Adjust() or Sound::Release() to change the
// sound's parameters.
{
	if (!IsOpen) return -1;
	
	int	i;
	HRESULT err;

	int	AvailableIndex = -1;
	int	MatchingResource = -1;
	
	// Find an available channel.
	for (i = 0; i < CHANNELCOUNT; i++) {
		// Check to see if this channel has the buffer data we want.
		if (MatchingResource == -1 &&
		    Channel[i].Duplicate == false &&
		    strcmp(Channel[i].ResourceName, ResourceName) == 0 &&
		    Channel[i].DSBuffer != NULL)
		{
			MatchingResource = i;

			// If it's not currently playing, then we'll just use this channel.
			unsigned long	Status;
			Channel[i].DSBuffer->GetStatus(&Status);
			if ((Status & DSBSTATUS_PLAYING) == 0) {
				AvailableIndex = i;
				break;
			}
		}
		
		if (Channel[i].EventID == 0 || Channel[i].DSBuffer == NULL) {
			// Channel is currently unused.
			AvailableIndex = i;
			break;
		}
		
		if (Channel[i].Duplicate == true && Channel[i].DSBuffer != NULL) {
			unsigned long	ChannelStatus;
			Channel[i].DSBuffer->GetStatus(&ChannelStatus);
			if ((ChannelStatus & DSBSTATUS_PLAYING) == 0) {
				// We can commandeer this channel.
				AvailableIndex = i;
				
				// Release the DS interface.
				Channel[i].DSBuffer->Release();
				Channel[i].DSBuffer = NULL;
				
				break;
			}
		}
	}

	if (AvailableIndex == -1) {
		// xxxx bump an active sound?  Or fail...
		return 0;
	}

	// Continue scanning for a matching resource, if we haven't found one already.
	if (MatchingResource == -1) {
		for ( ; i < CHANNELCOUNT; i++) {
			// Check to see if this channel has the buffer data we want.
			if (Channel[i].Duplicate == false && strcmp(Channel[i].ResourceName, ResourceName) == 0 && Channel[i].DSBuffer != NULL)
			{
				MatchingResource = i;
			}
		}
	}

	ChannelInfo& c = Channel[AvailableIndex];
	
	// Assign an event id to this sound.
	c.EventID = NextEventID++;

	if (MatchingResource != -1) {
		if (MatchingResource != AvailableIndex) {
			ChannelInfo&	match = Channel[MatchingResource];
			err = DirectSound->DuplicateSoundBuffer(match.DSBuffer, &c.DSBuffer);
			if (err != DS_OK) {
				// Couldn't duplicate the buffer for some reason.  Fail.
				c.DSBuffer = NULL;
				c.EventID = 0;
				return 0;
			}
			c.Duplicate = true;
		} else {
			// Just play this channel with the new parameters.
		}
	} else {

		DSBUFFERDESC dsbdesc; 

		// Notify Utility:: that we're opening a file.  This is because at the moment,
		// WaveOpenFile doesn't use Utility::FileOpen(), so it doesn't get logged automatically.
		Utility::FileOpenNotify(ResourceName);
		
		// Read the header of the .WAV file.
		HMMIO	IOIn;
		WAVEFORMATEX*	WaveInfo;
		MMCKINFO	CKIn, CKRIFFIn;
		err = WaveOpenFile(ResourceName, &IOIn, &WaveInfo, &CKRIFFIn);
		if (err) {
			c.EventID = 0;
			SoundError e; e << "Sound::Play(): couldn't open resource '" << ResourceName << "'.";
			throw e;
		}
		WaveStartDataRead(&IOIn, &CKIn, &CKRIFFIn);
		
//		// Set up wave format structure. 
//		memset(&wf, 0, sizeof(WAVEFORMATEX));
//		wf.wf.wFormatTag = WAVE_FORMAT_PCM;
//		wf.wf.nChannels = 1;
//		wf.wf.nSamplesPerSec = 22050;
//		wf.wf.nBlockAlign = 1;
//		wf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
//		wf.wBitsPerSample = 8;
		
		// Set up DSBUFFERDESC structure.
		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
		dsbdesc.dwSize = sizeof(DSBUFFERDESC);
		// Need default controls (pan, volume, frequency).
		dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC;

		dsbdesc.dwBufferBytes = CKIn.cksize;  /* 3 * WaveInfo.nAvgBytesPerSec 	// 3-second buffer. */
		dsbdesc.lpwfxFormat = WaveInfo;
		
		err = DirectSound->CreateSoundBuffer(&dsbdesc, &c.DSBuffer, NULL);
		if (err != DS_OK) {
			c.DSBuffer = NULL;
			c.EventID = 0;
			return 0;
		}

		// Load resource into locked buffer ...
		void*	Buffer = NULL;
		unsigned long	BufferBytes = 0;
		c.DSBuffer->Lock(0, 0, &Buffer, &BufferBytes, NULL, NULL, DSBLOCK_ENTIREBUFFER);;

		unsigned	ActuallyRead = 0;
		WaveReadFile(IOIn, BufferBytes, (BYTE*) Buffer, &CKIn, &ActuallyRead);

		WaveCloseReadFile(&IOIn, &WaveInfo);
		GlobalFree(WaveInfo);
		c.DSBuffer->Unlock(Buffer, BufferBytes, NULL, 0);
		
		c.Duplicate = false;
	}
	
	strcpy(c.ResourceName, ResourceName);

	c.ControlParameters = Parameters;
	
	c.DSBuffer->SetVolume(int(DSBVOLUME_MIN + (DSBVOLUME_MAX - DSBVOLUME_MIN) * c.ControlParameters.Volume));
	c.DSBuffer->SetPan(int((DSBPAN_RIGHT - DSBPAN_CENTER) * c.ControlParameters.Pan));
	c.DSBuffer->SetFrequency(int(OutputFrequency * c.ControlParameters.Pitch));
	c.DSBuffer->SetCurrentPosition(0);
	c.DSBuffer->Play(0, 0, c.ControlParameters.Looped ? DSBPLAY_LOOPING : 0);

	// Return the event id for later reference, if the caller wants to Release() or Adjust() this sound.
	return c.EventID;
}


// Natural base of logarithms.
#define	LN_BASE	2.718281828f


void	Adjust(int EventID, const Controls& NewParameters)
// Use this call to change the parameters of a sound that's currently playing.
{
	if (!IsOpen) return;
	
	if (EventID <= 0) return;
	
	// Find the specified sound event.
	for (int i = 0; i < CHANNELCOUNT; i++) {
		if (Channel[i].EventID == EventID) {
			// This is the one.
			break;
		}
	}

	if (i >= CHANNELCOUNT) {
		// No matching event.
		return;
	}

	ChannelInfo&	c = Channel[i];

	// Make sure the channel is active.
	if (c.DSBuffer == NULL) {
		return;
	}
	unsigned long	Status;
	c.DSBuffer->GetStatus(&Status);
	if ((Status & DSBSTATUS_PLAYING) == 0) {
		return;
	}

	// Adjust any changed parameters.
	if (c.ControlParameters.Volume != NewParameters.Volume) {
		c.ControlParameters.Volume = NewParameters.Volume;
		// -50dB is about as quiet as we care about.  The extra
		// 50dBs down to DirectSound's limit of -100dB is pretty
		// much beyond the usable range for us, so we'll map
		// almost the entire volume range between -50dB and 0dB.
		//
		// We'll also compensate to make a linear volume scale, instead of log.
		if (c.ControlParameters.Volume < 0.0001) {
			c.DSBuffer->SetVolume(DSBVOLUME_MIN);
		} else {
			c.DSBuffer->SetVolume(int(-5000 + 5000 * logf(1 + (LN_BASE - 1) * c.ControlParameters.Volume)));
		}
	}
	if (c.ControlParameters.Pan != NewParameters.Pan) {
		c.ControlParameters.Pan = NewParameters.Pan;
		c.DSBuffer->SetPan(int((DSBPAN_RIGHT - DSBPAN_CENTER) * c.ControlParameters.Pan));
	}
	if (c.ControlParameters.Pitch != NewParameters.Pitch) {
		c.ControlParameters.Pitch = NewParameters.Pitch;
		c.DSBuffer->SetFrequency(int(OutputFrequency * c.ControlParameters.Pitch));
	}
	
//	c.DSBuffer->SetCurrentPosition(0);
}


void	Release(int EventID)
// Use this call to stop a playing sound, and release its channel.
{
	if (!IsOpen) return;
	
	if (EventID <= 0) return;
	
	// Find the specified sound event.
	for (int i = 0; i < CHANNELCOUNT; i++) {
		if (Channel[i].EventID == EventID) {
			// This is the one.
			break;
		}
	}

	if (i >= CHANNELCOUNT) {
		// No matching event.
		return;
	}

	ChannelInfo&	c = Channel[i];

	// Make sure the channel is active.
	if (c.DSBuffer == NULL) {
		return;
	}
	unsigned long	Status;
	c.DSBuffer->GetStatus(&Status);
	if ((Status & DSBSTATUS_PLAYING) == 0) {
		return;
	}

	// Turn this channel off.
	// xxx could possibly just stop the sound from playing, and let it hang around to be re-used at some point...
	c.DSBuffer->Release();
	c.DSBuffer = NULL;
}


int	GetStatus(int EventID)
// Returns status information about the specified sound event.
{
	if (!IsOpen) return 0;
	
	if (EventID <= 0) return 0;
	
	int	i;

	// Find the event.
	for (i = 0; i < CHANNELCOUNT; i++) {
		if (Channel[i].EventID == EventID) {
			break;
		}
	}

	if (i >= CHANNELCOUNT || Channel[i].DSBuffer == NULL) {
		// Couldn't find the requested event, so it's definitely not playing.
		return 0;
	}

	unsigned long	DSStatus;
	Channel[i].DSBuffer->GetStatus(&DSStatus);

//	// Print to the log window.
//	char	buf[80];
//	sprintf(buf, "dsstatus = %d\n", DSStatus);
//	Utility::Log(4, buf);

	return 1;
}


//CDMode	CurrentCDMode = CD_NOT_READY;
//int	CurrentCDTrackCount = 0;


CDMode	GetCDMode()
// Returns the current mode of the cd player.
{
//	return CurrentCDMode;
	return cd::get_mode();
}


int	GetCDTrackCount()
// Returns the number of available audio tracks on the current cd.
{
//	return CurrentCDTrackCount;
	return cd::track_count();
}

void	GetCDDriveName(char* result)
// Returns the drive name of the cd drive.
{
//	return CurrentCDTrackCount;
	cd::get_drive_name(result);
}


void	PlayCDTrack(int TrackID)
// Plays the specified cd-audio track.  Track numbering starts with 1.
{
	cd::play(TrackID);

//	// For the moment assume that the command succeeded and the CD
//	// player is playing.  We should hopefully get a notification if
//	// something goes wrong.
//	CurrentCDMode = CD_PLAY;
}


//void	CDNotify()
//// Called by the OS when it thinks something has changed with the CD player.
//// Re-poll our track count and CD status.
//{
//	CurrentCDMode = cd::get_mode();
//	CurrentCDTrackCount = cd::track_count();
//}


void	SetMusicVolume(uint8 vol)
// Sets the volume control of the CD-Audio output.
{
	mixer_setcdvolume(vol);
}


#ifdef NOT


// Some pseudo-code for a custom mixer.  I'm not clear on all the
// details of the DirectSound notification facility, so I'm not going to
// tackle this yet.  It looks as though it will require a separate thread,
// sitting and waiting for notify messages, to build the output data.


play()
{
	find a free channel;
	assign event id;

	copy parameters;

	activate;

	// Could process channel, w/ estimate of buffer's current play position.
}

primary buffer notify()
{
	identify the next buffer to write into;

	clear buffer;

	for (channels) {
		if (not channel active) continue;

		process channel(channel, buffer, buffer length);
	}
}


process channel(source, buffer, buffer length)
{
	buffer left = buffer length;
	
	while (buffer left > 0) {
		to write = (source length - source cursor) * output rate / source rate;
		
		if (to write > buffer length) {
			to write = buffer length;
		}
		
		add to buffer(buffer, to write, source + source cursor, lvol, rvol, output rate, source rate);
		buffer left -= to write;
		buffer += to write;

		if (not source looped) {
			// Done with this sound event.
			source active = false;
			break;
		} else {
			// Loop to the beginning.
			source cursor = 0;
		}
	}
}


#endif // NOT



char*	DSErrorToString(HRESULT error)
// Looks up a DirectSound error message based on the given error code.
{
	switch (error) {
	default:
		return "DSerr: Unknown error code.\0";
        case DS_OK:
		return "No error.\0";
        case DSERR_ALLOCATED:
		return "DSerr: This resource has already been allocated.\0";
	case DSERR_INVALIDPARAM:
		return "DSerr: Invalid parameter.\0";
	case DSERR_NOAGGREGATION:
		return "DSerr: This object does not support aggregation.\0";
	case DSERR_NODRIVER:
		return "DSerr: No sound driver is available for use.\0";
	case DSERR_OUTOFMEMORY:
		return "DSerr: DirectSound could not allocate sufficient memory for this request.\0";
	case DSERR_INVALIDCALL:
		return "DSerr: This function is not valid for the current state of this object.\0";
	}
}


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


//
// cd stuff
//


namespace cd {
;

bool	CDIsOpen = false;
UINT	wCDDeviceID = 0;
int	LastTrackCount = 1;
bool	FirstTrackIsData = false;


void	open()
// Tries to open the CD player.
{
	DWORD	dwReturn;
	MCI_OPEN_PARMS	mciOpenParms;
	MCI_SET_PARMS	mciSetParms;

	if (Config::GetValue("CDAudioDrive") == NULL) {
		// User hasn't specified a specific CD drive to use for audio,
		// so go ahead and scan for a CDROM drive.

		char	drivename[4];
		strcpy(drivename, "A:\\");
		
		uint32	driveflags = GetLogicalDrives();
		int	driveindex;
		for (driveindex = 0; driveindex < 26 && driveflags != 0; driveindex++, driveflags >>= 1) {
			if (driveflags & 1) {
				drivename[0] = 'A' + driveindex;
				if (GetDriveType(drivename) == DRIVE_CDROM) {
					Config::SetValue("CDAudioDrive", drivename);
					break;
				}
			}
		}
	}

	if (Config::GetValue("CDAudioDrive") == NULL) return;
	
	mciOpenParms.lpstrElementName = (char*) Config::GetValue("CDAudioDrive");
	mciOpenParms.lpstrDeviceType = (LPCSTR) MCI_DEVTYPE_CD_AUDIO;
	dwReturn = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_ELEMENT | MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE, (DWORD)(LPVOID) &mciOpenParms);
	if (dwReturn) {
		// Couldn't open the desired device.
		return;
	}
	
	// Get the device ID.
	wCDDeviceID = mciOpenParms.wDeviceID;
		
	CDIsOpen = true;
	
	// Set the time format to track/minute/second/frame (TMSF).
	mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	dwReturn = mciSendCommand(wCDDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms);
	if (dwReturn) {
		// Error.
		mciSendCommand(wCDDeviceID, MCI_CLOSE, 0, NULL);
		return;
	}

//	// Get the number of tracks; 
//	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
//	dwReturn = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)(LPVOID) &mciStatusParms);
//	if (dwReturn) {
//		mciSendCommand(wCDDeviceID, MCI_CLOSE, 0, NULL);
//		return (dwReturn);
//	}
//	iNumTracks = mciStatusParms.dwReturn;
}


void	play(int track)
// Play the specified track.
{
	if (!CDIsOpen) return;

	int	FinalAudioTrack = LastTrackCount;
	if (FirstTrackIsData) {
		track++;
		FinalAudioTrack++;
	}
	
	MCI_PLAY_PARMS	mciPlayParms;
	mciPlayParms.dwFrom = 0;
	mciPlayParms.dwTo = 0;
	mciPlayParms.dwFrom = MCI_MAKE_TMSF(track, 0, 0, 0);
	mciPlayParms.dwTo = MCI_MAKE_TMSF(track + 1, 0, 0, 0);
	mciPlayParms.dwCallback = (DWORD)(LPVOID) Main::GetGameWindow();
	DWORD	dwReturn;
	if (track < FinalAudioTrack) {
		dwReturn = mciSendCommand(wCDDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM | MCI_TO, (DWORD)(LPVOID) &mciPlayParms);
	} else {
		// Omit MCI_TO parameter when playing the last track.
		dwReturn = mciSendCommand(wCDDeviceID, MCI_PLAY, MCI_NOTIFY | MCI_FROM, (DWORD)(LPVOID) &mciPlayParms);
	}
	if (dwReturn) {
		// mciSendCommand(wCDDeviceID, MCI_CLOSE, 0, NULL);
		return;
	}
}


void	pause()
{
	if (!CDIsOpen) return;
	
	MCI_GENERIC_PARMS p;
	p.dwCallback = (DWORD)(LPVOID) Main::GetGameWindow();
	DWORD	dwReturn = mciSendCommand(wCDDeviceID, MCI_PAUSE, MCI_NOTIFY, (DWORD)(LPVOID) &p);
}


void	stop()
{
	if (!CDIsOpen) return;
	
	MCI_GENERIC_PARMS p;
	p.dwCallback = (DWORD)(LPVOID) Main::GetGameWindow();
	DWORD	dwReturn = mciSendCommand(wCDDeviceID, MCI_STOP, MCI_NOTIFY, (DWORD)(LPVOID) &p);
}


void	close()
{
	if (!CDIsOpen) return;
	
	mciSendCommand(wCDDeviceID, MCI_CLOSE, 0, NULL);

	CDIsOpen = false;
}


CDMode	get_mode()
// Returns the current mode (i.e. CD_NOT_READY, CD_PLAY, CD_STOP, CD_PAUSE, etc).
{
	if (!CDIsOpen) return CD_NOT_READY;
	
	MCI_STATUS_PARMS	stat;
	DWORD	dwReturn;

	// Check for media.
	stat.dwCallback = 0;
	stat.dwItem = MCI_STATUS_MEDIA_PRESENT;
	dwReturn = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD)(LPVOID) &stat);
	if (dwReturn == 0) {
		if (stat.dwReturn == FALSE) return CD_NOT_READY;
	}
	
	// Check mode of device.
	stat.dwCallback = 0;
	stat.dwItem = MCI_STATUS_MODE;
	dwReturn = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD)(LPVOID) &stat);

	if (dwReturn == 0) {
		switch (stat.dwReturn) {
		default:
		case MCI_MODE_NOT_READY:
			return CD_NOT_READY;
		case MCI_MODE_PAUSE:
			return CD_PAUSE;
		case MCI_MODE_PLAY:
		case MCI_MODE_RECORD:
			return CD_PLAY;
		case MCI_MODE_STOP:
			return CD_STOP;
		case MCI_MODE_OPEN:
			return CD_OPEN;
		case MCI_MODE_SEEK:
			return CD_SEEK;
		}
	}
	
	return CD_NOT_READY;
}


int	track_count()
// Returns the number of available audio tracks on the cd.
{
	if (!CDIsOpen) return 0;
	
	MCI_STATUS_PARMS	stat;
	stat.dwCallback = 0;
	stat.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	
	DWORD	dwReturn = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)(LPVOID) &stat);

	if (dwReturn == 0) {
		LastTrackCount = stat.dwReturn;

		// Check track 1 to see if it's data.  If so, then reduce the track count by 1.
		stat.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
		stat.dwTrack = 1;
		dwReturn = mciSendCommand(wCDDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)(LPVOID) &stat);
		if (dwReturn == 0 && stat.dwReturn != MCI_CDA_TRACK_AUDIO) {
			FirstTrackIsData = true;
			LastTrackCount--;
		} else {
			FirstTrackIsData = false;
		}
				
		return LastTrackCount;	// Return the number of tracks on the cd.
	}

	LastTrackCount = 0;
	return 0;
}


void	get_drive_name(char* result)
// Fills result[] with the name of the cd-audio drive.  Sets to
// null string if name can't be obtained.
{
	result[0] = 0;

	if (!CDIsOpen) return;

	strcpy(result, Config::GetValue("CDAudioDrive"));

	// Make sure there's a trailing \.
	if (result[strlen(result)-1] != '\\') strcat(result, "\\");
}


};	// end namespace cd



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


bool	MixerIsOpen = false;


enum MixType {
	MIXTYPE_MASTER,
	MIXTYPE_WAVE,
	MIXTYPE_CD,
};


const int	MAX_MIXERS = 5;
int	MixerCount = 0;
HMIXER	Mixers[MAX_MIXERS];


const int	MAX_CONTROLS = 5;
int	ControlCount = 0;
struct MixerControl {
	HMIXER	Handle;
	DWORD	ControlID;
	DWORD	Min, Range;
	DWORD	OriginalVolume;
	MixType	Type;
} Controls[MAX_CONTROLS];


bool	AddLine(HMIXER mixerhandle, DWORD type, MixType mixtype)
// Tries to add a mixer line to our list, of the specified type, and using the indexed mixer.
// Tags any added controls with the given mixtype.
// Returns true if the line has been added; false otherwise.
{
	HMIXEROBJ	handle = (HMIXEROBJ) mixerhandle;
	MIXERLINE	line;
	line.cbStruct = sizeof(line);
	line.dwComponentType = type;
	MMRESULT	result = mixerGetLineInfo(handle, &line, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
	
	if (result) {
		// Couldn't get line info of the desired type.  Give up.
		return false;
	}

	Controls[ControlCount].Handle = (HMIXER) handle;
	line.dwLineID;

	// Find the volume control on this line.
	MIXERLINECONTROLS	control;
	MIXERCONTROL	mixctl;
	control.cbStruct = sizeof(control);
	control.dwLineID = line.dwLineID;
	control.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	control.cControls = 1;
	control.cbmxctrl = sizeof(mixctl);
	control.pamxctrl = &mixctl;
	result = mixerGetLineControls(handle, &control, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
	if (result) {
		// Error.
		return false;
	}
	
	Controls[ControlCount].ControlID = mixctl.dwControlID;
	Controls[ControlCount].Min = mixctl.Bounds.dwMinimum;
	Controls[ControlCount].Range = mixctl.Bounds.dwMaximum - mixctl.Bounds.dwMinimum;
	Controls[ControlCount].Type = mixtype;

	// Measure the original volume setting of this line.
	MIXERCONTROLDETAILS	m;
	m.cbStruct = sizeof(m);
	m.dwControlID = mixctl.dwControlID;
	m.cChannels = 1;
	m.cMultipleItems = 0;
	MIXERCONTROLDETAILS_UNSIGNED	volume;
	m.cbDetails = sizeof(volume);
	m.paDetails = &volume;
	result = mixerGetControlDetails((HMIXEROBJ) Controls[ControlCount].Handle, &m, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
	if (result) {
		// Default original volume to half volume.
		Controls[ControlCount].OriginalVolume = Controls[ControlCount].Min + (Controls[ControlCount].Range >> 1);
	} else {
		Controls[ControlCount].OriginalVolume = volume.dwValue;
	}

	ControlCount++;

	return true;
}


void	mixer_open()
{
	MMRESULT	result;
	
	int	NumberOfMixers = mixerGetNumDevs();
	if (NumberOfMixers < 1) {
		return;
	}

	if (NumberOfMixers > MAX_MIXERS) NumberOfMixers = MAX_MIXERS;	// Don't overflow our handle array.

	// Look for the mixers we want, i.e. CD (analog) and Digital Audio (could be CD input).
	int	i;
	for (i = 0; i < NumberOfMixers; i++) {

		HMIXER	handle;
		result = mixerOpen(&handle, i, 0, 0, MIXER_OBJECTF_MIXER);
		if (result) {
			// Can't open this mixer.  Skip it.
			continue;
		}

//		MMRESULT	mixerGetID(HMIXEROBJ mixerindex, UINT* puMxId, MIXER_OBJECTF_MIXER);
	
//		typedef struct {
//			WORD    wMid;	// manufacturer id
//			WORD    wPid;	// product id
//			MMVERSION vDriverVersion;
//			CHAR    szPname[MAXPNAMELEN];
//			DWORD   fdwSupport;
//			DWORD   cDestinations;
//		} MIXERCAPS;
//		MMRESULT	mixerGetDevCaps(UINT uMxId, LPMIXERCAPS pmxcaps, UINT cbmxcaps);

		bool	Using = false;
		Using = AddLine(handle, MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC, MIXTYPE_CD) || Using;
		Using = AddLine(handle, MIXERLINE_COMPONENTTYPE_SRC_DIGITAL, MIXTYPE_CD) || Using;

		Using = AddLine(handle, MIXERLINE_COMPONENTTYPE_DST_SPEAKERS, MIXTYPE_MASTER) || Using;

		Using = AddLine(handle, MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT, MIXTYPE_WAVE) || Using;
   
		if (Using) {
			// Add this mixer handle to our list, so we know to close it later.
			Mixers[MixerCount] = handle;
			MixerCount++;
		} else {
			// Close this mixer; we won't be using it.
			mixerClose(handle);
		}
	}

	MixerIsOpen = true;
}


void	mixer_close()
{
	if (!MixerIsOpen) return;

	int	i;

	// Set all the line volumes back to their original values.
	MIXERCONTROLDETAILS	m;
	m.cbStruct = sizeof(m);
	m.dwControlID = 0;
	m.cChannels = 1;
	m.cMultipleItems = 0;
	MIXERCONTROLDETAILS_UNSIGNED	volume;
	m.cbDetails = sizeof(volume);
	m.paDetails = &volume;

	MMRESULT	result;
	for (i = 0; i < ControlCount; i++) {
		m.dwControlID = Controls[i].ControlID;
		volume.dwValue = Controls[i].OriginalVolume;
		result = mixerSetControlDetails((HMIXEROBJ) Controls[i].Handle, &m, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
	}
	
	
	// Close all the mixers.
	for (i = 0; i < MixerCount; i++) {
		mixerClose(Mixers[i]);
	}
	MixerCount = 0;
	ControlCount = 0;

	MixerIsOpen = false;
}


static void	MixerSetVolume(uint8 vol, MixType mixtype)
// Sets the mixer controls of the given mixtype to the specified volume.
// The volume goes from 0 (softest) to 255 (loudest).
{
	if (!MixerIsOpen) return;
	
	MIXERCONTROLDETAILS	m;
	m.cbStruct = sizeof(m);
	m.dwControlID = 0;
	m.cChannels = 1;
	m.cMultipleItems = 0;
	MIXERCONTROLDETAILS_UNSIGNED	volume;
	m.cbDetails = sizeof(volume);
	m.paDetails = &volume;

	MMRESULT	result;
	int	i;
	for (i = 0; i < ControlCount; i++) {
		if (Controls[i].Type != mixtype) continue;
		
		m.dwControlID = Controls[i].ControlID;
		volume.dwValue = Controls[i].Min + (Controls[i].Range * vol / 255);
		result = mixerSetControlDetails((HMIXEROBJ) Controls[i].Handle, &m, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
	}
}


void	mixer_setcdvolume(uint8 vol)
// Sets the volume on CD-Audio mixer lines to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
	MixerSetVolume(vol, MIXTYPE_CD);
}


void	mixer_setwavevolume(uint8 vol)
// Sets the volume on the WAVE audio lines to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
	MixerSetVolume(vol, MIXTYPE_WAVE);
}


void	mixer_setmastervolume(uint8 vol)
// Sets the master speaker volume to the specified value.  The value
// goes from 0 (softest) to 255 (loudest).
{
	MixerSetVolume(vol, MIXTYPE_MASTER);
}


};	// End namespace Sound

