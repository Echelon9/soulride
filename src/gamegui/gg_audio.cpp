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
/////////////////////////////////////////////////
//
//  File:  gg_audio.cpp
//
//  GG_Audio Library:
//    an easy-to-use DirectSound wrapper
//
//  (c) 1998-2000  by Mike Linkovich
//
//  Start Date: ~ Spring 1998
//
/////////////////////////////////////////////////


#include "gg_audio.h"
#include "gg_string.h"

#include "gg_log.h"

#include <vector>
#include <algorithm>
using namespace std;


#ifndef LINUX

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>

#endif // not LINUX


//
//  IDs used for a .au or .snd (Next/Sun format) parsing
//
#define SND_FORMAT_UNSPECIFIED            0   //  unspecified format
#define SND_FORMAT_MULAW_8                1   //  8-bit mu-law samples
#define SND_FORMAT_LINEAR_8               2   //  8-bit linear samples
#define SND_FORMAT_LINEAR_16              3   //  16-bit linear samples
#define SND_FORMAT_LINEAR_24              4   //  24-bit linear samples
#define SND_FORMAT_LINEAR_32              5   //  32-bit linear samples
#define SND_FORMAT_FLOAT                  6   //  floating-point samples
#define SND_FORMAT_DOUBLE                 7   //  double-precision float samples
#define SND_FORMAT_INDIRECT               8   //  fragmented sampled data
#define SND_FORMAT_NESTED                 9   //  ??
#define SND_FORMAT_DSP_CORE               10  //  DSP program
#define SND_FORMAT_DSP_DATA_8             11  //  8-bit fixed-point samples
#define SND_FORMAT_DSP_DATA_16            12  //  16-bit fixed-point samples
#define SND_FORMAT_DSP_DATA_24            13  //  24-bit fixed-point samples
#define SND_FORMAT_DSP_DATA_32            14  //  32-bit fixed-point samples
                                              //  15 ?
#define SND_FORMAT_DISPLAY                16  //  non-audio display data
#define SND_FORMAT_MULAW_SQUELCH          17  //  ??
#define SND_FORMAT_EMPHASIZED             18  //  16-bit linear with emphasis
#define SND_FORMAT_COMPRESSED             19  //  16-bit linear with compression
#define SND_FORMAT_COMPRESSED_EMPHASIZED  20  //  A combination of the two above
#define SND_FORMAT_DSP_COMMANDS           21  //  Music Kit DSP commands
#define SND_FORMAT_DSP_COMMANDS_SAMPLES   22  //  ??

//
//  File header for .au/.snd
//
struct AUHeader
{
  uint32 magic;              //  magic number SND_MAGIC
  uint32 dataLocation;       //  offset or pointer to the data
  uint32 dataSize;           //  number of bytes of data
  uint32 dataFormat;         //  the data format code
  uint32 samplingRate;       //  the sampling rate
  uint32 channelCount;       //  the number of channels
  char info[4];              //  optional text information
};


static char msgBuf[256] = "";   //  For debug strings


//  Some error code defines for use with loading waves
//
#ifndef ER_MEM
#define ER_MEM              0xe000
#endif

#ifndef ER_CANNOTOPEN
#define ER_CANNOTOPEN       0xe100
#endif

#ifndef ER_NOTWAVEFILE
#define ER_NOTWAVEFILE      0xe101
#endif

#ifndef ER_CANNOTREAD
#define ER_CANNOTREAD       0xe102
#endif

#ifndef ER_CORRUPTWAVEFILE
#define ER_CORRUPTWAVEFILE  0xe103
#endif

#ifndef ER_CANNOTWRITE
#define ER_CANNOTWRITE      0xe104
#endif


/////////////////////////////////////////////////
//
//  Globals
//

//  GG_Audio is a singleton -- track instances

static int32 g_GG_AudioInstanceCount = 0;


/////////////////////////////////////////////////
//
//  Internal funcs -- used within this file
//


#ifndef LINUX


//
//  This function will open a wave input file and prepare it for reading,
//  so the data can be easily
//  read with WaveReadFile. Returns 0 if successful, the error code if not.
//
//  pszFileName - Input filename to load.
//  phmmioIn    - Pointer to handle which will be used
//                for further mmio routines.
//  ppwfxInfo   - Ptr to ptr to WaveFormatEx structure
//                with all info about the file.
//
//
static int waveOpenFile( char         *pszFileName,  // (IN)
                         HMMIO        *phmmioIn,     // (OUT)
                         WAVEFORMATEX **ppwfxInfo,   // (OUT)
                         MMCKINFO     *pckInRIFF )   // (OUT)
{
  HMMIO         hmmioIn;
  MMCKINFO      ckIn;           // chunk info. for general use.
  PCMWAVEFORMAT pcmWaveFormat;  // Temp PCM structure to load in.
  WORD          cbExtraAlloc;   // Extra bytes for waveformatex
  int           nError;         // Return value.


  // Initialization...
  //
  *ppwfxInfo = NULL;
  nError = 0;
  hmmioIn = NULL;

  if((hmmioIn = mmioOpen(pszFileName, NULL, MMIO_ALLOCBUF | MMIO_READ)) == NULL)
  {
    nError = ER_CANNOTOPEN;
    goto ERROR_READING_WAVE;
  }

  if((nError = (int)mmioDescend(hmmioIn, pckInRIFF, NULL, 0)) != 0)
    goto ERROR_READING_WAVE;


  if((pckInRIFF->ckid != FOURCC_RIFF) || (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')))
  {
    nError = ER_NOTWAVEFILE;
    goto ERROR_READING_WAVE;
  }

  //  Search the input file for for the 'fmt ' chunk.
  //
  ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');

  if( (nError = (int)mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
    goto ERROR_READING_WAVE;

  //  Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
  //  if there are extra parameters at the end, we'll ignore them
  //
  if (ckIn.cksize < (long) sizeof(PCMWAVEFORMAT))
  {
    nError = ER_NOTWAVEFILE;
    goto ERROR_READING_WAVE;
  }

  // Read the 'fmt ' chunk into <pcmWaveFormat>
  //
  if (mmioRead(hmmioIn, (HPSTR) &pcmWaveFormat, (long) sizeof(pcmWaveFormat)) != (long) sizeof(pcmWaveFormat))
  {
    nError = ER_CANNOTREAD;
    goto ERROR_READING_WAVE;
  }

  // Ok, allocate the waveformatex, but if its not pcm
  // format, read the next word, and thats how many extra
  // bytes to allocate.
  //
  if(pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
  {
    cbExtraAlloc = 0;
  }
  else
  {
    // Read in length of extra bytes.
    //
    if (mmioRead(hmmioIn, (LPSTR) &cbExtraAlloc,
        (long) sizeof(cbExtraAlloc)) != (long) sizeof(cbExtraAlloc))
    {
      nError = ER_CANNOTREAD;
      goto ERROR_READING_WAVE;
    }
  }

  // Ok, now allocate that waveformatex structure.
  //
//  *ppwfxInfo = (WAVEFORMATEX *)GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX)+cbExtraAlloc);
  *ppwfxInfo = (WAVEFORMATEX *)new char[sizeof(WAVEFORMATEX) + cbExtraAlloc];
  if( *ppwfxInfo == null )
  {
    nError = ER_MEM;
    goto ERROR_READING_WAVE;
  }

  // Copy the bytes from the pcm structure to the waveformatex structure
  //
  memcpy(*ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat));
  (*ppwfxInfo)->cbSize = cbExtraAlloc;

  // Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
  //
  if (cbExtraAlloc != 0)
  {
    if (mmioRead(hmmioIn, (LPSTR) (((BYTE*)&((*ppwfxInfo)->cbSize))+sizeof(cbExtraAlloc)),
        (long) (cbExtraAlloc)) != (long) (cbExtraAlloc))
    {
      nError = ER_NOTWAVEFILE;
      goto ERROR_READING_WAVE;
    }
  }

  //  Ascend the input file out of the 'fmt ' chunk.
  //
  if ((nError = mmioAscend(hmmioIn, &ckIn, 0)) != 0)
    goto ERROR_READING_WAVE;

  goto TEMPCLEANUP;

ERROR_READING_WAVE:
  if (*ppwfxInfo != null)
  {
//    GlobalFree(*ppwfxInfo);
    delete[] (*ppwfxInfo);
    *ppwfxInfo = null;
  }

  if (hmmioIn != null)
  {
    mmioClose(hmmioIn, 0);
    hmmioIn = null;
  }

TEMPCLEANUP:
  *phmmioIn = hmmioIn;
  return(nError);
}


//
//  This routine has to be called before WaveReadFile
//  as it searchs for the chunk to descend into for
//  reading, that is, the 'data' chunk.  For simplicity,
//  this used to be in the open routine, but was taken
//  out and moved to a separate routine so there was
//  more control on the chunks that are before the
//  data chunk, such as 'fact', etc...
//
//
static int waveStartDataRead( HMMIO *phmmioIn,
                              MMCKINFO *pckIn,
                              MMCKINFO *pckInRIFF )
{
  int nError = 0;

  // Do a nice little seek...
  //
  if( (nError = mmioSeek(*phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC), SEEK_SET)) == -1)
    GG_LOG( "waveStartDataRead() -- Failed mmioSeek()" );

  //  Search the input file for for the 'data' chunk.
  //
  pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
  if ((nError = mmioDescend(*phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
    goto ERROR_READING_WAVE;

  goto CLEANUP;

ERROR_READING_WAVE:

CLEANUP:
  return(nError);
}


//
//  This will read wave data from the wave file.
//  Make sure we're descended into the data chunk,
//  else this will fail bigtime!
//
//  hmmioIn         - Handle to mmio.
//  cbRead          - # of bytes to read.
//  pbDest          - Destination buffer to put bytes.
//  cbActualRead- # of bytes actually read.
//
//
static int waveReadFile( HMMIO hmmioIn,         // IN
                         UINT cbRead,           // IN
                         BYTE *pbDest,          // IN
                         MMCKINFO *pckIn,       // IN.
                         UINT *cbActualRead )   // OUT.
{
  MMIOINFO    mmioinfoIn;         // current status of <hmmioIn>
  int         nError;
  UINT        cT, cbDataIn;

  nError = 0;

  nError = mmioGetInfo(hmmioIn, &mmioinfoIn, 0);
  if( nError != 0 )
    goto ERROR_CANNOT_READ;

  cbDataIn = cbRead;
  if(cbDataIn > pckIn->cksize)
    cbDataIn = pckIn->cksize;

  pckIn->cksize -= cbDataIn;

  for(cT = 0; cT < cbDataIn; cT++)
  {
    //  Copy the bytes from the io to the buffer
    //
    if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
    {
      if ((nError = mmioAdvance(hmmioIn, &mmioinfoIn, MMIO_READ)) != 0)
        goto ERROR_CANNOT_READ;

      if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
      {
        nError = ER_CORRUPTWAVEFILE;
        goto ERROR_CANNOT_READ;
      }
    }

    // Actual copy.
    //
    *((BYTE*)pbDest+cT) = *((BYTE*)mmioinfoIn.pchNext);
    *mmioinfoIn.pchNext++;
  }

  if((nError = mmioSetInfo(hmmioIn, &mmioinfoIn, 0)) != 0)
    goto ERROR_CANNOT_READ;

  *cbActualRead = cbDataIn;
  goto FINISHED_READING;

ERROR_CANNOT_READ:
  *cbActualRead = 0;

FINISHED_READING:
  return(nError);
}


//
//  This routine loads a full wave file into memory.  Be careful, wave files can get
//  pretty big these days :).
//
//  szFileName      - sz Filename
//  cbSize          - Size of loaded wave (returned)
//  cSamples        - # of samples loaded.
//  ppwfxInfo       - Pointer to pointer to waveformatex structure.  The wfx structure
//                    IS ALLOCATED by this routine!  Make sure to free it!
//  ppbData         - Pointer to a byte pointer (globalalloc) which is allocated by this
//                    routine.  Make sure to free it!
//
//  Returns 0 if successful, else the error code.
//
static int loadWaveFile( char          *pszFileName,   // (IN)
                         UINT          *cbSize,        // (OUT)
//                         DWORD         *pcSamples,     // (OUT)
                         WAVEFORMATEX  **ppwfxInfo,    // (OUT)
                         BYTE          **ppbData )     // (OUT)
{
  HMMIO     hmmioIn;
  MMCKINFO  ckInRiff;
  MMCKINFO  ckIn;
  int       nError;
  UINT      cbActualRead;

  *ppbData = NULL;
  *ppwfxInfo = NULL;
  *cbSize = 0;

  if ((nError = waveOpenFile(pszFileName, &hmmioIn, ppwfxInfo, &ckInRiff)) != 0)
    goto ERROR_LOADING;

  if ((nError = waveStartDataRead(&hmmioIn, &ckIn, &ckInRiff)) != 0)
    goto ERROR_LOADING;

  //  Ok, size of wave data is in ckIn, allocate that buffer.
  //
//  if ((*ppbData = (BYTE *)GlobalAlloc(GMEM_FIXED, ckIn.cksize)) == NULL)
  if( (*ppbData = (BYTE *)new BYTE[ckIn.cksize]) == null)
  {
    nError = ER_MEM;
    goto ERROR_LOADING;
  }

  if ((nError = waveReadFile(hmmioIn, ckIn.cksize, *ppbData, &ckIn, &cbActualRead)) != 0)
    goto ERROR_LOADING;

  *cbSize = cbActualRead;
  goto DONE_LOADING;

ERROR_LOADING:
  if (*ppbData != null)
  {
//    GlobalFree(*ppbData);
    delete[] (BYTE*)(*ppbData);
    *ppbData = null;
  }
  if (*ppwfxInfo != null)
  {
    delete[] (char*)(*ppwfxInfo);
    *ppwfxInfo = null;
  }

DONE_LOADING:
  // Close the wave file.
  //
  if (hmmioIn != NULL)
  {
    mmioClose(hmmioIn, 0);
    hmmioIn = NULL;
  }

  return(nError);
}


#endif // not LINUX


/////////////////////////////////////////////////
//
//  Support for .au (.snd) files...
//
//  Little/Big endian conversions...
//
//
static unsigned short switchEndian16( uint16 n )
{
  uint16 r;

  r = ((n & 0xFF00) >> 8)  |
      ((n & 0x00FF) << 8);

//  *((char *)&r) = *(((char *)&n) + 1);
//  *(((char *)&r) + 1) = *((char *)&n);

  return r;
}


static unsigned long switchEndian32(long n)
{
  unsigned long r;

  r = ((n & 0xFF000000) >> 24) |
      ((n & 0x00FF0000) >> 8)  |
      ((n & 0x0000FF00) << 8)  |
      ((n & 0x000000FF) << 24);

//  ((char *)&r)[0] = ((char *)&n)[3];
//  ((char *)&r)[1] = ((char *)&n)[2];
//  ((char *)&r)[2] = ((char *)&n)[1];
//  ((char *)&r)[3] = ((char *)&n)[0];

  return r;
}


/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
#define SIGN_BIT    (0x80)    // Sign bit for a A-law byte
#define QUANT_MASK  (0xf)     // Quantization field mask
#define NSEGS       (8)       // Number of A-law segments
#define SEG_SHIFT   (4)       // Left shift for segment number
#define SEG_MASK    (0x70)    // Segment field mask

#define BIAS        (0x84)    // Bias for linear code

static short ulaw2linear( unsigned char u_val )
{
  short t;

  /* Complement to obtain normal u-law value. */
  u_val = ~u_val;

  /*
   * Extract and bias the quantization bits. Then
   * shift up by the segment number and subtract out the bias.
   */
  t = ((u_val & QUANT_MASK) << 3) + BIAS;
  t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

  return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}



#ifndef LINUX


//
//  loadAUFile()
//
//  - Load a .au (.snd) file
//
static byte *loadAUFile( GG_File *f, LPWAVEFORMATEX wfmt, uint32 *size )
{
  AUHeader  auHeader;
  char      sndStr[8] = ".snd";
  byte      *pData = null;
  int32     i, bps;
  uint16    *p16 = null;


  if( f == null )
  {
    GG_LOG("loadAUFile() -- got NULL file");
    return null;
  }

  if( f->read( &auHeader, sizeof(AUHeader), 1 ) < 1 )
  {
    GG_LOG("loadAUFile() -- failed to read header");
    return null;
  }

  //  Convert all from big to little endian
  //
//  auHeader.magic = switchEndian32( auHeader.magic );
  auHeader.dataLocation = switchEndian32( auHeader.dataLocation );
  auHeader.dataSize = switchEndian32( auHeader.dataSize );
  auHeader.dataFormat = switchEndian32( auHeader.dataFormat );
  auHeader.samplingRate = switchEndian32( auHeader.samplingRate );
  auHeader.channelCount = switchEndian32( auHeader.channelCount );

  //  Verify that this is in fact a .au/.snd file...
  //
//  if( auHeader.magic != 7564900 )
  if( memcmp( &auHeader.magic, sndStr, 4 ) != 0 )
  {
    sprintf(msgBuf, "loadAUFile() -- magic = %d -- not a .au or .snd file", auHeader.magic);
    GG_LOG(msgBuf);
    return null;
  }

  //  Check the format.. do we support it?
  //
  switch( auHeader.dataFormat )
  {
    case SND_FORMAT_LINEAR_8:
      bps = 8;
      break;

    case SND_FORMAT_MULAW_8:
      bps = 16;
      p16 = (unsigned short *)(new byte[auHeader.dataSize * 2]);   //  Need additional block for decompression
      if( p16 == null )
      {
        GG_LOG("loadAUFile() -- not enough memory to perform decompression:");
        sprintf(msgBuf, "requested %d bytes", auHeader.dataSize * 2);
        GG_LOG(msgBuf);
        return null;
      }
      break;

    case SND_FORMAT_LINEAR_16:
      bps = 16;
      break;

    default:
      GG_LOG("loadAUFile() -- unsupported .au file format");
      return null;
  }

  //  How far do we seek ahead past the header to reach the data?
  //  (if at all -- usually not)
  //
  i = (int)auHeader.dataLocation - (int)sizeof(AUHeader);
  if( i != 0 )
  {
    if( f->seek(i, GGFILE_SEEK_REL) != 0 )
    {
      GG_LOG("loadAUFile() -- failed to seek data");
      return null;
    }
  }

  //  Alloc the space to hold the sound data...
  //
  pData = new byte[auHeader.dataSize];
  if( pData == null )
  {
    GG_LOG("loadAUFile() -- not enough memory to load file");
    return null;
  }

  //  Read the actual sound data...
  //
  if( f->read( pData, sizeof(byte), auHeader.dataSize ) < auHeader.dataSize )
  {
    GG_LOG("loadAUFile() -- failed to read sound data");
    goto failExit;
  }

  *size = auHeader.dataSize;

  if( bps == 16 )
  {
    switch( auHeader.dataFormat )
    {
      case SND_FORMAT_LINEAR_16:
        p16 = (unsigned short *)pData;
        for( i = 0; i < auHeader.dataSize / 2; i++ )
          p16[i] = switchEndian16( p16[i] );
        break;

      case SND_FORMAT_MULAW_8:
        //
        //  Must de-compress from u-law 8 bit to 16 bit linear
        //
        for( i = 0; i < auHeader.dataSize; i++ )
          p16[i] = ulaw2linear( pData[i] );

        delete[] pData;         //  Dump the compressed data
        pData = (byte *)p16;  //  Swap pointers
        p16 = null;
        *size = auHeader.dataSize * 2;
        break;

      default:
        GG_LOG("loadAUFile() -- can't handle 16-bit case");
        goto failExit;
    }
  }

  //  Record the format data in the supplied Windows struct..
  //
  wfmt->wFormatTag = WAVE_FORMAT_PCM;
  wfmt->nChannels = (WORD)auHeader.channelCount;
  wfmt->wBitsPerSample = (WORD)bps;
  wfmt->nSamplesPerSec = auHeader.samplingRate;
  wfmt->nBlockAlign = (WORD)((auHeader.channelCount * bps) / 8);
  wfmt->nAvgBytesPerSec = wfmt->nBlockAlign * wfmt->nSamplesPerSec;
  wfmt->cbSize = 0;

  //  Everything turned out ok!
  //
  return pData;

failExit:

  if( pData != null )
    delete[] pData;

  return null;
}


#endif // not LINUX


///////////////////////////////////////////////////////////
//
//  Interface Implementation objects
//
//

class _GG_Sound : public GG_Sound
{

protected:
  int                 m_refCount;     //  Reference count
  bool                m_bLooping;     //  Is/not currently looping
  uint32              m_duration;     //  Duration of sound in ms
  float               m_volume;       //  0.0 = off, 1.0 = full
  int32               m_iFreqDefault; //  Sound buffer's default frequency
  float               m_fFreq;        //  Current altered frequency (1.0 == norm)
  vector<GG_Sound*>   *m_ownerList;   //  List to remove itself from on destroy

#ifndef LINUX
  LPDIRECTSOUNDBUFFER m_lpDSBuf;      //  DirectSound buffer
#endif // not LINUX

  friend class _GG_Audio;


public:

_GG_Sound()
  {
    m_refCount = 0;
    m_bLooping = false;
    m_volume = 1.0;
    m_duration = 0;
    m_ownerList = null;
#ifndef LINUX
    m_lpDSBuf = NULL;
#endif // not LINUX
  }

~_GG_Sound()
{
#ifndef LINUX
  if( m_lpDSBuf != NULL )
      m_lpDSBuf->Release();
#endif // not LINUX
}



bool play( bool bLoop )
{
#ifdef LINUX
	return true;
#else // not LINUX

  DWORD   sbStatus;
  DWORD   flags = 0;
  HRESULT rval;


  if( bLoop )
    flags = DSBPLAY_LOOPING;

  rval = m_lpDSBuf->GetStatus( &sbStatus );
  if( rval != DS_OK )
  {
    GG_LOG("GG_Sound::play() -- [DSound]Play() failed");
//    GG_LOG(DXErrorToString(rval));
    return FALSE;
  }

  if( sbStatus == DSBSTATUS_PLAYING )
  {
    rval = DS_OK;

    //  Was playing already... make some adjustments..
    //
    if( m_bLooping != bLoop )
    {
      //  Playing already, but not how was requested.  Rewind & play...
      //
      rval = m_lpDSBuf->Stop();
      rval = m_lpDSBuf->SetCurrentPosition( 0 );
      rval = m_lpDSBuf->Play( 0, 0, flags );
    }
    else
    {
      if( !bLoop )
        rval = m_lpDSBuf->SetCurrentPosition( 0 );
    }

    if( rval != DS_OK )
    {
      GG_LOG("GG_Sound::play() -- DSound error:");
//      GG_LOG(DXErrorToString(rval));
      return FALSE;
    }
  }
  else
  {
    //  Wasn't playing.. start er up
    //
    rval = m_lpDSBuf->Play( 0, 0, flags );
    if( rval != DS_OK )
    {
      GG_LOG("GG_Sound::play() -- [DSound]Play() failed");
//      GG_LOG(DXErrorToString(rval));
      return FALSE;
    }
  }

  m_bLooping = bLoop;

  return true;
#endif // not LINUX
}


bool stop(void)
{
#ifdef LINUX
	return true;
#else // not LINUX
  HRESULT rval;

  rval = m_lpDSBuf->Stop();
  if( rval != DS_OK )
  {
    GG_LOG("GG_Sound::stop() -- [DSound]Stop() failed:");
//        GG_LOG(DXErrorToString(rval));
    return false;
  }

  m_bLooping = FALSE;

  rval = m_lpDSBuf->SetCurrentPosition( 0 );
  if( rval != DS_OK )
  {
    GG_LOG("GG_Sound::stop() -- [DSound]SetCurrentPosition() failed:");
//        GG_LOG(DXErrorToString(rval));
    return false;
  }

  return true;
#endif // not LINUX
}


bool isPlaying(void)
{
#ifdef LINUX
	return false;
#else // not LINUX
  DWORD   sbStatus;
  HRESULT rval;


  rval = m_lpDSBuf->GetStatus( &sbStatus );
  if( rval != DS_OK )
  {
    GG_LOG("GG_Sound::isPlaying() -- [DSound]GetStatus() failed");
//        GG_LOG(DXErrorToString(rval));
    return false;
  }

  return (sbStatus == DSBSTATUS_PLAYING);
#endif // not LINUX
}


bool setVolume( float vol )
{
  if( vol < 0.0 )
    vol = 0.0;
  if( vol > 1.0 )
    vol = 1.0;

  m_volume = vol;

  vol = 1.0 - vol;
#ifndef LINUX
  m_lpDSBuf->SetVolume( -(long)( (vol * vol) * 4000.0) );
#endif // not LINUX

  return true;
}


float getVolume(void)
{
  return m_volume;
}


uint32 getDuration(void)
{
  return m_duration;
}


bool setFrequency( float f )
{
  if( f <= 0.0 )
  {
    GG_LOG("_GG_Sound::setFrequency() -- frequency must be > 0.0");
    return false;
  }

  if( f == m_fFreq )
    return true;

  int32   newFreq = (int32)(f * (float)m_iFreqDefault);

  if( newFreq < 100 )
    newFreq = 100;
  else if( newFreq > 100000 )
    newFreq = 100000;

#ifndef LINUX
  HRESULT rval;
  if( (rval = m_lpDSBuf->SetFrequency( newFreq )) != DS_OK )
  {
    sprintf(msgBuf, "_GG_Sound::setFrequency() -- DSoundBuf::SetFreqency failed; freq=%d", newFreq);
    GG_LOG(msgBuf);
    return false;
  }
#endif // not LINUX

  m_fFreq = (float)newFreq / (float)m_iFreqDefault;

  return true;
}


float getFrequency(void)
{
  return m_fFreq;
}


//
//  Reference management
//
int addRef(void)
{
  m_refCount++;
  return m_refCount;
}


int unRef(void)
{
  m_refCount--;

  if( m_refCount > 0 )
    return m_refCount;

  //  No more references .. remove from owner list..
  //
  vector<GG_Sound*>::iterator iter =
        find( m_ownerList->begin(), m_ownerList->end(), this );
  if( iter != m_ownerList->end() )
    m_ownerList->erase(iter);
  else
  {
    GG_LOG("GG_Sound::unRef() -- failed to find 'this' in owner list");
  }

  delete this;
  return 0;
}


void* getDSSoundBuffer1(void)
{
#ifdef LINUX
	return 0;
#else // not LINUX
  return (void*)m_lpDSBuf;
#endif // not LINUX
}


};    //  end class _GG_Sound


/////////////////////////////////////////////////
//
//  _GG_Audio class member functions
//

class _GG_Audio : public GG_Audio
{
protected:
    int                 m_refCount;         //  Reference count
    vector<GG_Sound*>   m_sounds;           //  List of all sound buffers

#ifndef LINUX
    LPDIRECTSOUND       m_lpDS;             //  DirectSound object
    LPDIRECTSOUNDBUFFER m_lpDSBufPrimary;   //  Primary sound buffer
#endif // not LINUX

//    GG_Sound* createAuSound( char *sndFile );
//    GG_Sound* createWavSound( char *sndFile );

public:

_GG_Audio()
{
  m_refCount = 0;
#ifndef LINUX
  m_lpDS = NULL;
  m_lpDSBufPrimary = NULL;
#endif // not LINUX
}

~_GG_Audio()
{
  //  Free all resources allocated by GG_Audio...
  for( vector<GG_Sound*>::iterator i = m_sounds.begin(); i < m_sounds.end(); i++ )
    if( *i != null )
      delete *i;

#ifndef LINUX
  //  Release DSound primary buffer
  if( m_lpDSBufPrimary != NULL )
    m_lpDSBufPrimary->Release();

  //  Release DSound object
  if( m_lpDS != NULL )
    m_lpDS->Release();
#endif // not LINUX
}


//
//  Reference management
//
int addRef(void)
{
  m_refCount++;

  return m_refCount;
}


int unRef(void)
{
  int r;

  m_refCount--;
  r = m_refCount;

  if( r < 1 )
  {
    delete this;
    g_GG_AudioInstanceCount--;
  }

  return r;
}


GG_Sound* createWavSound( char *sndFile )
{
#ifdef LINUX
	return NULL;
#else // not LINUX
  _GG_Sound    *sound = NULL;

  UINT            cbSize = 0;
  LPWAVEFORMATEX  lpWavFmt = NULL;
  LPBYTE          lpbData = NULL;
  LPBYTE          lpbData1 = NULL;
  LPBYTE          lpbData2 = NULL;
  DWORD           dwLength1;
  DWORD           dwLength2;
  DSBUFFERDESC    dsBufDesc;
  HRESULT         rval;


  if( loadWaveFile( sndFile, &cbSize, &lpWavFmt, &lpbData ) != 0 )
  {
    sprintf(msgBuf, "_GG_Audio::createSound(file) -- Failed to Load %s", sndFile);
    GG_LOG(msgBuf);
    return NULL;
  }

  memset( &dsBufDesc, 0, sizeof( DSBUFFERDESC ));
  dsBufDesc.dwSize = sizeof( DSBUFFERDESC );
  dsBufDesc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME;
  dsBufDesc.dwBufferBytes = cbSize;
  dsBufDesc.lpwfxFormat = lpWavFmt;

  sound = new _GG_Sound;
  sound->m_volume = 1.0;
  sound->m_ownerList = &m_sounds;
  sound->m_duration = 1000 * cbSize / lpWavFmt->nChannels / (lpWavFmt->wBitsPerSample / 8) /
                      lpWavFmt->nSamplesPerSec;
  sound->m_iFreqDefault = lpWavFmt->nSamplesPerSec;
  sound->m_fFreq = 1.0;

  rval = m_lpDS->CreateSoundBuffer( &dsBufDesc, &sound->m_lpDSBuf, NULL );
  if( rval != DS_OK )
  {
    GG_LOG( "_GG_Audio::createSound(file) -- [DSound]CreateSoundBuffer() failed");
    goto failExit;
  }

  rval = sound->m_lpDSBuf->Lock( 0, (DWORD)cbSize, (LPVOID*)&lpbData1, (LPDWORD)&dwLength1,
                                 (LPVOID*)&lpbData2, (LPDWORD)&dwLength2, 0 );
  if( rval != DS_OK )
  {
    GG_LOG("LoadWavIntoBuffer() -- [DSound]Lock() failed");
//        GG_LOG(DXErrorToString(rval));
    goto failExit;
  }

  memcpy( lpbData1, lpbData, cbSize );

  rval = sound->m_lpDSBuf->Unlock( lpbData1, cbSize, NULL, 0 );
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::createSound(file) -- [DSound]Unlock() failed");
//    GG_LOG(DXErrorToString(rval));
    goto failExit;
  }

//  GlobalFree( lpWavFmt );
//  GlobalFree( lpbData );
  delete[] (BYTE*)lpWavFmt;
  delete[] (BYTE*)lpbData;

  //  Add to our list
  //
  m_sounds.push_back(sound);

  return sound;

failExit:
//  GlobalFree( lpWavFmt );
//  GlobalFree( lpbData );
  delete[] (BYTE*)lpWavFmt;
  delete[] (BYTE*)lpbData;
  return null;
#endif // not LINUX
}


GG_Sound* createSound( GG_File *f )
{
#ifdef LINUX
	return NULL;
#else // not LINUX

  _GG_Sound       *sound = null;
  byte            *pData = null;

  uint32          size = 0;

  DSBUFFERDESC    dsBufDesc;
  WAVEFORMATEX    wavFmt;
  LPBYTE          lpbData1 = null;
  LPBYTE          lpbData2 = null;
  DWORD           dwLength1;
  DWORD           dwLength2;
  HRESULT         rval;


  if( (pData = loadAUFile( f, &wavFmt, &size )) == null )
  {
    GG_LOG("_GG_Audio::createSound(file) -- Failed to loadAUFile");
    return null;
  }

  memset( &dsBufDesc, 0, sizeof( DSBUFFERDESC ));
  dsBufDesc.dwSize = sizeof( DSBUFFERDESC );
  dsBufDesc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME;
  dsBufDesc.dwBufferBytes = size;
  dsBufDesc.lpwfxFormat = &wavFmt;

  sound = new _GG_Sound;
  sound->m_volume = 1.0;
  sound->m_ownerList = &m_sounds;
  sound->m_duration = 1000 * size / wavFmt.nChannels / (wavFmt.wBitsPerSample / 8) /
                      wavFmt.nSamplesPerSec;
  sound->m_iFreqDefault = wavFmt.nSamplesPerSec;
  sound->m_fFreq = 1.0;

  rval = m_lpDS->CreateSoundBuffer( &dsBufDesc, &sound->m_lpDSBuf, NULL );
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::createSound(file) -- [DSound]CreateSoundBuffer() failed");
    sound->m_lpDSBuf = null;
    goto failExit;
  }

  rval = sound->m_lpDSBuf->Lock( 0, (DWORD)size, (LPVOID*)&lpbData1, (LPDWORD)&dwLength1,
                                 (LPVOID*)&lpbData2, (LPDWORD)&dwLength2, 0 );
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::createSound(fp) -- DSound::Lock() failed");
//    GG_LOG( DXErrorToString(rval)) );
    goto failExit;
  }

  if( dwLength1 < size )
  {
    GG_LOG("_GG_Audio::createSound(fp) -- DSound::Lock() didn't supply enough space");
    rval = sound->m_lpDSBuf->Unlock( lpbData1, size, NULL, 0 );
    goto failExit;
  }

  memcpy( lpbData1, pData, size );

  rval = sound->m_lpDSBuf->Unlock( lpbData1, size, NULL, 0 );
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::createSound(file) -- DSoundBuf::Unlock() failed");
//    GG_LOG(DXErrorToString(rval));
    goto failExit;
  }

  delete[] pData;

//  if( (rval = sound->m_lpDSBuf->GetFrequency((LPDWORD)&sound->m_iFreqDefault)) != DS_OK )
//  {
//    BUGMSG( logErr( "_GG_Audio::createSound(file) -- DSoundBuf::GetFrequency() failed" ) );
//    BUGMSG( logErr( DXErrorToString(rval)) );
//    goto failExit;
//  }

  //  Add to our list
  //
  m_sounds.push_back(sound);

  return sound;

failExit:
  if( pData != null )
    delete[] pData;

  if( sound != null )
    delete sound;

  return null;
#endif // not LINUX
}


GG_Sound* createSound( FILE *fp )
{
  GG_File *f = GG_FileAliasFILE(fp);
  GG_Sound *snd = createSound(f);
  f->close();
  return snd;
}


GG_Sound* createAuSound( char *sndFile )
{
  GG_Sound *snd;
  FILE  *fp = fopen( sndFile, "rb" );
  if( fp == null )
    return null;

  snd = createSound( fp );
  fclose( fp );
  return snd;
}


/**
 * Create sound from path, handling format automatically
 *
 * @param sndFile Path to file
 */
GG_Sound*
createSound( char *sndFile )
{
  size_t   i = 0;
  char  ext[128] = "";


  while( i < strlen( sndFile ) )
  {
    if( sndFile[i] == '.' )
    {
      i++;
      break;
    }

    i++;
  }

  strcpy( ext, &sndFile[i] );
  GG_String::capitalize( ext );

  if( strcmp( ext, "WAV" ) == 0 )
    return createWavSound( sndFile );
  if( (strcmp( ext, "AU" ) == 0) || (strcmp( ext, "SND" ) == 0) )
    return createAuSound( sndFile );

  GG_LOG("_GG_Audio::createSound() -- got unknown extension from in filename:");
  GG_LOG(sndFile);

  return null;
}


bool playPrimary(void)
{
#ifndef LINUX
  HRESULT rval = m_lpDSBufPrimary->Play( 0, 0, DSBPLAY_LOOPING );
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::playPrimary() -- Play() failed...");
//    GG_LOG( DXErrorToString(rval)) );
    return false;
  }
#endif // not LINUX

  return true;
}


bool stopPrimary(void)
{
#ifndef LINUX
  HRESULT rval = m_lpDSBufPrimary->Stop();
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::stopPrimary() -- Stop() failed...");
//    GG_LOG( DXErrorToString(rval));
    return false;
  }
#endif // not LINUX

  return true;
}


//
//  Set up DirectSound & Primary sound buffer
//
//
GG_Rval setup( const GG_AudioSetup &osData )
{
#ifdef LINUX
	return GG_OK;

#else // not LINUX

  DSBUFFERDESC  dsbDesc;
  HRESULT       rval;
  GG_Rval       ggRval = GG_OK;

  m_refCount = 1;

  //  Create the DirectSound object
  //
  rval = DirectSoundCreate( NULL, &m_lpDS, NULL );
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::setup() -- DirectSoundCreate() failed:");
//    GG_LOG(DXErrorToString(rval));
    goto failExit;
  }

  //  Find the COM object..
  //
//  rval = CoCreateInstance( CLSID_DirectSound, NULL, CLSCTX_ALL,
//                           IID_IDirectSound, (LPVOID*)&m_lpDS );
//  if( rval != S_OK )
//  {
//    GG_LOG("_GG_Sound::setup() -- CoCreateInstance(DSound1) failed");
//    return GGA_ERR_NODIRECTSOUND;
//  }

//  rval = m_lpDS->Initialize( NULL );
//  if( rval != S_OK )
//  {
//    GG_LOG("_GG_Audio::setup() -- DS::Initialize() failed");
//    goto failExit;
//  }

  rval = m_lpDS->SetCooperativeLevel( (HWND)osData.hWnd, DSSCL_NORMAL );
  if( rval != DS_OK )
  {
    sprintf( msgBuf, "_GG_Audio::setup() -- DirectSound::SetCooperativeLevel() failed: HRESULT = %d", rval );
    GG_LOG(msgBuf);
    goto failExit;
  }

  //  Create the primary sound buffer
  //
  memset( &dsbDesc, 0, sizeof(DSBUFFERDESC));
  dsbDesc.dwSize = sizeof(DSBUFFERDESC);
  dsbDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

  rval = m_lpDS->CreateSoundBuffer(&dsbDesc, &m_lpDSBufPrimary, NULL);
  if( rval != DS_OK )
  {
    GG_LOG("_GG_Audio::setup() -- DirectSound::CreateSoundBuffer() failed for primary buffer");
    goto failExit;
  }

  return GG_OK;


failExit:
  if( m_lpDS != NULL )
  {
    m_lpDS->Release();
    m_lpDS = NULL;
  }

  if( ggRval == GG_OK )
    ggRval = GG_ERR;
  return ggRval;

#endif // not LINUX
}

};    //  end _GG_Sound class


/////////////////////////////////////////////////
//
//  Creator Function
//

//
//  GG_AudioCreate()
//
//  - Audio object creator function
//

GG_Rval GG_AudioCreate(const GG_AudioSetup &osData, GG_Audio **dstAudio)
{
  _GG_Audio *audio = NULL;       //  GG_Audio object that will be created
  GG_Rval   rval = GG_OK;


  //  GG_Audio is a singleton ...  don't create > 1 instance
  //
  if( g_GG_AudioInstanceCount > 0 )
  {
    GG_LOG("error: GG_AudioCreate() -- Attempted to create > 1 instance of GG_Audio");
    return GG_ERR_MAXINSTANCES;
  }

  //  Create & init the GG_Audio private data
  //
  audio = new _GG_Audio;
  if( (rval = audio->setup(osData)) != GG_OK )
  {
    delete audio;
    return rval;
  }

  g_GG_AudioInstanceCount++;
  *dstAudio = audio;

  //  Return the object!
  //
  return GG_OK;
}
