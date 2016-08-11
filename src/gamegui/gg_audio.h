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
//  File:  gg_audio.h
//
//  GG_Audio Library:
//    an easy-to-use DirectSound wrapper
//
//  (c) 1998-2000  by Mike Linkovich
//
/////////////////////////////////////////////////


#ifndef _GG_AUDIO_INCLUDED
#define _GG_AUDIO_INCLUDED


#include "gamegui.h"
#include "gg_file.h"
#include <stdio.h>


#define GGA_ERR_NODIRECTSOUND  -100   //  In addition to standard return codes in gamegui.h


struct GG_AudioSetup
{
  void *hWnd;             //  This allows you to not include <windows.h> at all
  void *hInst;            //  Change voids to HWND and HINSTANCE if you want type-safety
};


//  GG_Sound
//
class GG_Sound
{
  public:
    GG_Sound()  {}
    virtual ~GG_Sound()  {}
          //  Use GG_Audio::createSound() and unRef to create/destroy
    virtual int addRef(void) = 0;
    virtual int unRef(void) = 0;
    virtual bool play( bool bLoop ) = 0;
    virtual bool stop(void) = 0;
    virtual bool isPlaying(void) = 0;
          //  Latency unknown..?
    virtual bool setVolume( float vol ) = 0;
    virtual float getVolume(void) = 0;
          //  1.0 == full volume
    virtual uint32 getDuration(void) = 0;
    virtual bool setFrequency( float f ) = 0;
    virtual float getFrequency(void) = 0;
          //  1.0 == normal frequency
    virtual void* getDSSoundBuffer1(void) = 0;
          //  Returns LPDIRECTSOUNDBUFFER associated with the sound
          //  using void* keeps this from including <windows.h>/<dsound.h>
          //  can be changed later with ifdefs.
};


//  GG_Audio
//
class GG_Audio
{
  public:
    GG_Audio()  {}
    virtual ~GG_Audio()  {}
    virtual int addRef(void) = 0;
    virtual int unRef(void) = 0;
    virtual GG_Sound *createSound( char *soundFile ) = 0;
    virtual GG_Sound *createSound( FILE *fp ) = 0;
    virtual GG_Sound *createSound( GG_File *f ) = 0;
    virtual bool playPrimary(void) = 0;
          //  Call this once you are starting/stopping lots of short sounds
    virtual bool stopPrimary(void) = 0;
          //  Call this once you stop using sounds
};


//  For static link usage
//
GG_Rval GG_AudioCreate(const GG_AudioSetup &pOSData, GG_Audio **audio);

//
//  For DLL usage
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//EZA_Rval GG_EXPORT GG_AudioCreate1(EZOSData *pOSData, GG_Audio **audio);
//
//#ifdef __cplusplus
//}
//#endif
//
//  Typedef for GG_AudioCreate function pointer
//
//typedef GG_Rval (* EZ_AUDIO_CREATE)( GG_OSData *pOSData, GG_Audio **audio );
//
//  Version (Search DLL for this function name)
//
//#define GG_AUDIO_CREATE_DLL_STR "_GGAudioCreate1@4"
//


#endif    //  _GGAUDIO_INCLUDED
