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
///////////////////////////////////////////////////////
//
//  File: gg_event.cpp
//
//  GameGUI Script Event class implementation
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


#include "gg_event.h"
#include <string.h>


static char logBuf[256] = "";   //  For quickndirty string builds


//  Keywords used in a script to add an event
//
static char g_eventStr[GG_MOVIE_NUM_EVENTS][GG_MAX_MOVIE_EVENT_STRLEN] =
                 { "PLAY", "SETSPACING",
                   "PAUSE", "MUTE", "HIDE",
                   "SETPLAYMODE", "SETTIMESCALE",
                   "SETPOS", "SETROT", "SETSCALE",
                   "SETCOLOR", "SETVOLUME" };


//  Datasize used for each event.. typos==bad
//
static uint32 g_eventDataSize[GG_MOVIE_NUM_EVENTS] =
                { sizeof(int), sizeof(GG_Vector2D),
                  sizeof(int), sizeof(int), sizeof(int),
                  sizeof(int32), sizeof(float),
                  sizeof(GG_Vector2D), sizeof(float), sizeof(GG_Vector2D),
                  sizeof(GG_ColorARGBf), sizeof(float) };



///////////////////////////////////////////////////////
//
//  A few base GG_Event member functions are defined..
//
GG_Rval GG_Event::writeHdr( GG_File *f )
{
  sprintf( logBuf, "%d ", t );
  f->writeText(logBuf);
  f->writeText( g_eventStr[id] );
  f->writeText( " " );
  return GG_OK;
}


int GG_Event::getEventID( char *buf )
{
  int id;
  for( id = 0; id < GG_MOVIE_NUM_EVENTS; id++ )
  {
    if( strcmp(buf, g_eventStr[id]) == 0 )
      return id;
  }

  return -1;
}

//  Get the size of the data associated with the event ID
//
uint32 GG_Event::getDataSize( uint32 eventId )
{
  return g_eventDataSize[eventId];
}


//  Convert a linear time scale (0.0 - 1.0) to
//  exponential.  Note how negative exponents are handled..
//
float GG_Event::linToExpT(float scale)
{
  float s;
  if( exp < 0 )
    s = 1.0f - powf((1.0f-scale), -exp);
  else
    s = powf(scale, exp);
  return s;
}


//  Search this string for the first interpolation tag
//  -- finds the first '^', reads the float val immed. after that
//  indicating the exponent to use.  Range must be:
//  -inf < n <= -1 or 1 <= n < inf, otherwise set to 1.0 (linear)
//  Returns the exponent the event interpolation will use.
//
float GG_Event::readInterp( char *buf )
{
  char  *p = buf;
  float e = 1.0;

  interpFlag = GG_EVENT_INTERP_LINEAR;  //  defaults
  exp = 1.0;

  while( *p != '\0' )
  {
    if( *p == '^' )
    {
      p++;  //  Skip to next char
      if( sscanf( p, "%f", &e ) < 1 )
        return 1.0;   //  Failed to read event, use default setting
      exp = e;
      if( exp > -1.0 && exp < 1.0 )   //  Legal value?
        exp = 1.0;                    //  No
      else                            //  Yes, use it.
        interpFlag = GG_EVENT_INTERP_EXP;
      return exp;
    }
    p++;
  }

  return 1.0;
}


///////////////////////////////////////////////////////
//
//  Derivations of GG_Event follow..
//

struct GG_ColorEvent : public GG_Event
{
  GG_ColorARGBf color;

  GG_ColorEvent()  { color.set( 0, 0, 0, 0 ); }
  ~GG_ColorEvent()  { }

  void interpolate( int32 time,     //  Time to interpolate at
                    GG_Event *ed,    //  Between this event instance and ed
                    void *result )   //  Interpolated color
  {
    float scale = (float)(time - t) / (float)(ed->t - t),
          s1;

    if( interpFlag == GG_EVENT_INTERP_EXP )   //  If non-linear interpolation requested
      scale = linToExpT(scale);               //  we must "curve" time with the exponent
    s1 = 1.0f - scale;

    ((GG_ColorARGBf*)result)->set( color.a * s1 + ((GG_ColorEvent*)ed)->color.a * scale,
                                   color.r * s1 + ((GG_ColorEvent*)ed)->color.r * scale,
                                   color.g * s1 + ((GG_ColorEvent*)ed)->color.g * scale,
                                   color.b * s1 + ((GG_ColorEvent*)ed)->color.b * scale );
  }

  GG_Rval read( char *buf )
  {
    if( sscanf( buf, "%f %f %f %f", &color.a, &color.r, &color.g, &color.b ) < 4 )
      return GG_ERR_FILEFORMAT;
    return GG_OK;
  }

  GG_Rval write( GG_File *f )
  {
    writeHdr(f);
    sprintf( logBuf, "%f %f %f %f", color.a, color.r, color.g, color.b );
    f->writeText( logBuf );
    if( interpFlag == GG_EVENT_INTERP_EXP )
      sprintf( logBuf, " ^%f\n", exp );
    else
      strcpy( logBuf, "\n" );
    f->writeText( logBuf );
    return GG_OK;
  }

  uint32 dataSize(void)
  {
    return sizeof(color);
  }

  void getData(void *p)
  {
    *(GG_ColorARGBf*)p = color;
  }

  void setData(void *p)
  {
    color = *(GG_ColorARGBf*)p;
  }
};


struct GG_2dvEvent : public GG_Event
{
  GG_Vector2D   v;

  GG_2dvEvent()  { v.set( 0, 0 ); }
  ~GG_2dvEvent()  { }

  void interpolate( int32 time,
                    GG_Event *ed,
                    void *result )
  {
    float s1, scale = (float)(time - t) / (ed->t - t);
    if( interpFlag == GG_EVENT_INTERP_EXP )
      scale = linToExpT(scale);
    s1 = 1.0f - scale;
    ((GG_Vector2D*)result)->set( v.x * s1 + ((GG_2dvEvent*)ed)->v.x * scale,
                                 v.y * s1 + ((GG_2dvEvent*)ed)->v.y * scale );
  }

  GG_Rval read( char *buf )
  {
    if( sscanf( buf, "%f %f", &v.x, &v.y ) < 2 )
      return GG_ERR_FILEFORMAT;
    return GG_OK;
  }

  GG_Rval write( GG_File *f )
  {
    writeHdr(f);
    sprintf( logBuf, "%f %f", v.x, v.y );
    f->writeText( logBuf );
    if( interpFlag == GG_EVENT_INTERP_EXP )
      sprintf( logBuf, " ^%f\n", exp );
    else
      strcpy( logBuf, "\n" );
    f->writeText( logBuf );
    return GG_OK;
  }

  uint32 dataSize(void)
  {
    return sizeof(v);
  }

  void getData(void *p)
  {
    *(GG_Vector2D*)p = v;
  }

  void setData(void *p)
  {
    v = *(GG_Vector2D*)p;
  }
};


struct GG_bEvent : public GG_Event
{
  int            bVal;

  GG_bEvent()  { bVal = false; }
  ~GG_bEvent()  { }

  void interpolate( int32 time,
                    GG_Event *ed,
                    void *result )
  {
    //  Boolean events don't really interpolate.. the state
    //  remains as it was last set.. so the first of the bracketing
    //  event states ("this" event) is used..
    //
    *((int*)result) = bVal;
  }

  GG_Rval read( char *buf )
  {
    if( sscanf( buf, "%d", &bVal ) < 1 )
      return GG_ERR_FILEFORMAT;
    return GG_OK;
  }

  GG_Rval write( GG_File *f )
  {
    writeHdr(f);
    sprintf( logBuf, "%d", bVal );
    f->writeText( logBuf );
    if( interpFlag == GG_EVENT_INTERP_EXP )
      sprintf( logBuf, " ^%f\n", exp );
    else
      strcpy( logBuf, "\n" );
    f->writeText( logBuf );
    return GG_OK;
  }

  uint32 dataSize(void)
  {
    return sizeof(bVal);
  }

  void getData(void *p)
  {
    *(int*)p = bVal;
  }

  void setData(void *p)
  {
    bVal = *(int*)p;
  }
};


struct GG_iEvent : public GG_Event
{
  int32 iVal;

  GG_iEvent()  { iVal = 0; }
  ~GG_iEvent()  { }

  void interpolate( int32 time,
                    GG_Event *ed,
                    void *result )
  {
    float scale = (float)(time - t) / (ed->t - t);
    if( interpFlag == GG_EVENT_INTERP_EXP )
      scale = linToExpT(scale);
    *((int32*)result) = (int32)(iVal * (1.0f-scale) + ((GG_iEvent*)ed)->iVal * scale);
  }

  GG_Rval read( char *buf )
  {
    if( strcmp(buf, "TOEND") == 0 )       // Alternative strings to numeric digits
    {
      iVal = GG_PLAYMODE_TOEND;
      return GG_OK;
    }
    if( strcmp(buf, "LOOP") == 0 )
    {
      iVal = GG_PLAYMODE_LOOP;
      return GG_OK;
    }
    if( strcmp(buf, "HANG") == 0 )
    {
      iVal = GG_PLAYMODE_HANG;
      return GG_OK;
    }

    if( sscanf( buf, "%d", &iVal ) < 1 )  //  Ok, just read it as a number
      return GG_ERR_FILEFORMAT;
    return GG_OK;
  }

  GG_Rval write( GG_File *f )
  {
    writeHdr(f);
    sprintf( logBuf, "%d", iVal );
    f->writeText( logBuf );
    if( interpFlag == GG_EVENT_INTERP_EXP )
      sprintf( logBuf, " ^%f\n", exp );
    else
      strcpy( logBuf, "\n" );
    f->writeText( logBuf );
    return GG_OK;
  }

  uint32 dataSize(void)
  {
    return sizeof(iVal);
  }

  void getData(void *p)
  {
    *(int32*)p = iVal;
  }

  void setData(void *p)
  {
    iVal = *(int32*)p;
  }
};


struct GG_uEvent : public GG_Event
{
  uint32 uVal;

  GG_uEvent()  { uVal = 0; }
  ~GG_uEvent()  { }

  void interpolate( int32 time,
                    GG_Event *ed,
                    void *result )
  {
    float scale = (float)(time - t) / (ed->t - t);
    if( interpFlag == GG_EVENT_INTERP_EXP )
      scale = linToExpT(scale);
    *((uint32*)result) = (uint32)(uVal * (1.0f-scale) + ((GG_uEvent*)ed)->uVal * scale);
  }

  GG_Rval read( char *buf )
  {
    if( sscanf( buf, "%d", &uVal ) < 1 )
      return GG_ERR_FILEFORMAT;
    return GG_OK;
  }

  GG_Rval write( GG_File *f )
  {
    writeHdr(f);
    sprintf( logBuf, "%d", uVal );
    f->writeText( logBuf );
    if( interpFlag == GG_EVENT_INTERP_EXP )
      sprintf( logBuf, " ^%f\n", exp );
    else
      strcpy( logBuf, "\n" );
    f->writeText( logBuf );
    return GG_OK;
  }

  uint32 dataSize(void)
  {
    return sizeof(uVal);
  }

  void getData(void *p)
  {
    *(uint32*)p = uVal;
  }

  void setData(void *p)
  {
    uVal = *(uint32*)p;
  }
};


struct GG_fEvent : public GG_Event
{
  float fVal;

  GG_fEvent()  { fVal = 0; }
  ~GG_fEvent()  { }

  void interpolate( int32 time,
                    GG_Event *ed,
                    void *result )
  {
    float scale = (float)(time - t) / (float)(ed->t - t);
    if( interpFlag == GG_EVENT_INTERP_EXP )
      scale = linToExpT(scale);
    *((float*)result) = fVal * (1.0f-scale) + ((GG_fEvent*)ed)->fVal * scale;
  }

  GG_Rval read( char *buf )
  {
    if( sscanf( buf, "%f", &fVal ) < 1 )
      return GG_ERR_FILEFORMAT;
    return GG_OK;
  }

  GG_Rval write( GG_File *f )
  {
    writeHdr(f);
    sprintf( logBuf, "%f", fVal );
    f->writeText( logBuf );
    if( interpFlag == GG_EVENT_INTERP_EXP )
      sprintf( logBuf, " ^%f\n", exp );
    else
      strcpy( logBuf, "\n" );
    f->writeText( logBuf );
    return GG_OK;
  }

  uint32 dataSize(void)
  {
    return sizeof(fVal);
  }

  void getData(void *p)
  {
    *(float*)p = fVal;
  }

  void setData(void *p)
  {
    fVal = *(float*)p;
  }
};


///////////////////////////////////////////////////////
//
//  Factory function.  Takes an event ID, returns
//  the correct object
//
GG_Event *GG_EventCreate( int id )
{
  GG_Event *ev = null;
  switch( id )
  {
    case GG_MOVIE_EVENT_PLAY:
    case GG_MOVIE_EVENT_PAUSE:
    case GG_MOVIE_EVENT_MUTE:
    case GG_MOVIE_EVENT_HIDE:
      ev = new GG_bEvent;
      ev->id = id;
      return ev;
    case GG_MOVIE_EVENT_SETPLAYMODE:
      ev = new GG_iEvent;
      ev->id = id;
      return ev;
    case GG_MOVIE_EVENT_SETTIMESCALE:
    case GG_MOVIE_EVENT_SETROT:
    case GG_MOVIE_EVENT_SETVOLUME:
      ev = new GG_fEvent;
      ev->id = id;
      return ev;
    case GG_MOVIE_EVENT_SETPOS:
    case GG_MOVIE_EVENT_SETSCALE:
    case GG_MOVIE_EVENT_SETSPACING:
      ev = new GG_2dvEvent;
      ev->id = id;
      return ev;
    case GG_MOVIE_EVENT_SETCOLOR:
      ev = new GG_ColorEvent;
      ev->id = id;
      return ev;
  }
  return null;
}
