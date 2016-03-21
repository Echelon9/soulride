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
///////////////////////////////////////////////////////
//
//  GameGUI Script Event class
//
//  This class helps do a lot of dirty work, and
//  thus gets a *little* hairy here and there.
//  A variety of derivations of MovieEvent
//  (eg: Vector2D events, ColorARGBf events, etc.)
//  are used and encapsulate operations on those
//  datatypes.
//
//  Since each datatype will interpolate
//  differently, virtual functions are used for
//  interpolation tasks.
//
//  Start Date: Feb 14 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////

#ifndef _GG_MOVIE_EVENT_INCLUDED
#define _GG_MOVIE_EVENT_INCLUDED


#include "gamegui.h"
#include <math.h>

#ifdef MACOSX
#include "../macosxworkaround.hpp"
#endif




#define GG_MOVIE_EVENT_PLAY          0
#define GG_MOVIE_EVENT_SETSPACING    1      //  Spacing between elements (type spacing, leading) [objects?]
#define GG_MOVIE_EVENT_PAUSE         2
#define GG_MOVIE_EVENT_MUTE          3
#define GG_MOVIE_EVENT_HIDE          4
#define GG_MOVIE_EVENT_SETPLAYMODE   5
#define GG_MOVIE_EVENT_SETTIMESCALE  6
#define GG_MOVIE_EVENT_SETPOS        7
#define GG_MOVIE_EVENT_SETROT        8
#define GG_MOVIE_EVENT_SETSCALE      9
#define GG_MOVIE_EVENT_SETCOLOR      10
#define GG_MOVIE_EVENT_SETVOLUME     11

#define GG_MOVIE_NUM_EVENTS          12

#define GG_EVENT_INTERP_LINEAR        0     //  Linear interpolation
#define GG_EVENT_INTERP_EXP           1     //  Exponential interpolation


#define GG_MAX_MOVIE_EVENT_STRLEN    32    //  Max chars used for event type keyword



///////////////////////////////////////////////////////
//
//  GG_Event is the abstract base class for
//  all types of script events
//
//
struct GG_Event
{
  int32   id;           //  Type of event ID
  int32   t;            //  Time of event (millisec)
  float   exp;          //  Time scale exponent -- allows for "curved" time
  uint32  interpFlag;   //  See GG_EVENT_INTERP_... flags


  GG_Event()
  {
    id = 0;
    t = 0;
    exp = 1.0f;
    interpFlag = GG_EVENT_INTERP_LINEAR;
  }

  virtual ~GG_Event()  { }

  virtual GG_Rval read( char *buf ) = 0;
        //  Reads data by parsing a null-terminated, "stripped" string

  GG_Rval writeHdr( GG_File *f );

  virtual GG_Rval write( GG_File *f ) = 0;
        //  Writes event description to a file

  virtual uint32 dataSize(void) = 0;
        //  Size of data portion only

  virtual void getData(void *pDest) = 0;
        //  Copies data to destination pointer.  Dest ptr must have room..

  virtual void setData(void *pSrc) = 0;
        //  Copies from supplied source.
        //  Be sure to know what data size & format this event requires!

  float linToExpT(float t);
        //  Using this event's interpFlag and exp value,
        //  we re-calculate a time scale value (range 0.0 - 1.0)
        //  as an exponent.  Negative exponents reverse the curve.

  virtual void interpolate( int32 t,                  //  Time at which to calc interpolation
                            GG_Event *ed,             //  end event (*this* is start event)
                            void *result ) = 0;       //  Ptr to result value (size dependent on event type, get it straight!)
        //  interpolate() is not type safe, it must be carefully used.
        //  Caller MUST ensure e1 and e2 are the same type, and that
        //  'result' points to enough space to hold the interpolated value.
        //  Use dataSize() to find out how much space a GG_Event's
        //  data takes.

  float readInterp( char *buf );
        //  Search this string for the first interpolation tag.
        //  Function finds the first '^', reads the float after that
        //  as the exponent.  Range must be:
        //  -inf < n <= -1 or 1 <= n < inf, otherwise rejected and
        //  exp is set to 1.0 and interpFlag set to GG_EVENT_INTERP_LINEAR.
        //  Returns the exponent the event interpolation will use.
        //  note: use this function to scan a line before or after calling
        //  Event::read()

  static int getEventID( char *buf );
        //  Given a string, match it to an event ID
        //  Returns -1 on no match

  static uint32 getDataSize( uint32 eventId );
        //  Get the size of the data associated with the event ID
};


///////////////////////////////////////////////////////
//
//  Factory function.  Takes an event ID, returns
//  the correct object
//
GG_Event *GG_EventCreate( int id );


#endif  // _GG_MOVIE_EVENT_INCLUDED
