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
//  File: gg_player_impl.cpp
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#include "gg_player_impl.h"
#include "gg_log.h"

#include <algorithm>


///////////////////////////////////////////////////////
//
//  GG_Player_impl member functions
//
//

int GG_Player_impl::addRef(void)
{
  m_refCount++;
  return m_refCount;
}


int GG_Player_impl::unRef(void)
{
  m_refCount--;
  int r = m_refCount;
  if( r < 1 )
  {
    if( m_ownerList != null )
    {
      vector<GG_Player_impl*>::iterator i =
               find( m_ownerList->begin(), m_ownerList->end(), this );
      if( i != m_ownerList->end() )
        m_ownerList->erase(i);
    }
    delete this;
  }
  return r;
}


void GG_Player_impl::setOwnerList( vector<GG_Player_impl*> *ownerList )
{
  m_ownerList = ownerList;
}


GG_Movie*	GG_Player_impl::getMovie()
// Return our source movie.
{
	return m_movie;
}


int32 GG_Player_impl::getDuration(void)
{
  return m_duration;
}


GG_PlayMode GG_Player_impl::getPlayMode(void)
{
  return m_playMode;
}


GG_Rval GG_Player_impl::setPlayMode( GG_PlayMode mode )
{
  switch( mode )
  {
    case GG_PLAYMODE_TOEND:
    case GG_PLAYMODE_HANG:
    case GG_PLAYMODE_LOOP:
      m_playMode = mode;
      return GG_OK;
  }

  return GG_ERR_BADPARAM;
}


GG_Rval GG_Player_impl::getScale( GG_Vector2D *s )
{
  *s = m_state.scale;
  return GG_OK;
}


GG_Rval GG_Player_impl::setScale( float sx, float sy )
{
  m_state.scale.x = sx;
  m_state.scale.y = sy;

  return GG_OK;
}


//  Plays the movie this player is being used for
//
GG_Rval GG_Player_impl::play( int32 dt )
{
  if( m_movie == null )
    return GG_OK;                 //  No movie to play

  GG_Rval rval = GG_OK;
  int32   curT = dt + m_prevT;
  float   x, y;
  int32   dur = m_movie->getDuration();

  //  Check if we've gone past the end of the movie..
  //
  if( curT >= dur )
  {
    //  Yes, each playmode will handle this a little differently..
    switch( m_playMode )
    {
      case GG_PLAYMODE_TOEND:
        if( m_prevT >= dur )
          return GG_PLAYERFINISHED;           //  Don't play anything after end of movie reached
        //  no break, follow through..
      case GG_PLAYMODE_HANG:
        curT = dur;               //  "Hang" on last end T
        dt = curT - m_prevT;
        rval = GG_PLAYERFINISHED;
        break;
      case GG_PLAYMODE_LOOP:
        if( dur < 1 )             //  watch out for % 0
          curT = 0;
        else
          curT %= dur;            //  loop around
        if( dt > curT )
          dt = curT;
        break;
    }
    //  Adjust prevT now that dt/curT may have changed
    m_prevT = curT-dt;
  }

  //  Okay start doing the frame here.
  //  Setup all rendering states appropriate to rendering a movie frame..
  //

  glPushMatrix();
    int w = m_rcView.x2 - m_rcView.x1,    //  Figure out the aspect ratio
        h = m_rcView.y2 - m_rcView.y1;    //  then scale appropriately
    if( w > h )
    {
      x = (float)h / w;
      y = 1.0f;
    }
    else
    {
      x = 1.0f;
      y = (float)w / h;
    }
    glScalef( x * m_state.scale.x, y * m_state.scale.y, 1.0f );
    rval = m_movie->doFrame( m_state, m_prevT, dt );
  glPopMatrix();

  m_prevT += dt;

  return rval;
}


int32 GG_Player_impl::getTime(void)
{
  return m_prevT;
}


GG_Rval GG_Player_impl::setTime( int32 t )
{
  if( t < 0 )
    return GG_ERR_BADPARAM;

  if( m_movie == null )
  {
    m_prevT = t;
    return GG_OK;
  }

  stop();     //  Stop any async processes (like sound)

  int32 dur = m_movie->getDuration();

  if( t > dur )   //  "fix" the time to a real time..
  {
    switch( m_playMode )
    {
      case GG_PLAYMODE_TOEND:
      case GG_PLAYMODE_HANG:
        t = dur;
        break;
      case GG_PLAYMODE_LOOP:
        if( dur < 1 )             //  watch out for % 0
          t = 0;
        else
          t %= dur;            //  loop around
        break;
    }
  }

  m_prevT = t;
  return GG_OK;
}


//  Force stop it
//
void GG_Player_impl::stop(void)
{
  if( m_movie != null )
    m_movie->stop();
}


bool GG_Player_impl::detectHitf( float x, float y, int id )
{
  if( m_movie == null )
    return false;

  int32   t = m_prevT,
          dur = m_movie->getDuration();

  //  Check if we've gone past the end of the movie..
  if( t >= dur )
  {
    //  Yes, each playmode will handle this a little differently..
    switch( m_playMode )
    {
      case GG_PLAYMODE_TOEND:
        return false;           //  Past end of movie
        //  no break, follow through..
      case GG_PLAYMODE_HANG:
        t = dur;               //  "Hang" on end T
        break;
      case GG_PLAYMODE_LOOP:
        if( dur < 1 )          //  watch out for % 0
          t = 0;
        else
          t %= dur;            //  loop around
        break;
    }
  }

  GG_Matrix3f m;
  m.makeIdentity();
  return m_movie->detectHit( m, x, y, t, id );
}


bool GG_Player_impl::detectHiti( int ix, int iy, int id )
{
  if( m_movie == null )
    return false;

  float x, y, s;
  int   w = m_rcView.x2 - m_rcView.x1,      //  Transform to world coordinates
        h = m_rcView.y2 - m_rcView.y1;

  if( w < 1 || h < 1 )
    return false;         //  unusable viewrect

  if( w > h )
    s = 2.0f / h;
  else
    s = 2.0f / w;

  x = (ix - (w / 2)) * s;
  y = (h - iy - (h / 2)) * s;

  return detectHitf( x, y, id );
}


bool GG_Player_impl::findHitf( float x, float y, int *id )
{
  if( m_movie == null )
    return false;

  int32   t = m_prevT,
          dur = m_movie->getDuration();

  //  Check if we've gone past the end of the movie..
  if( t >= dur )
  {
    //  Yes, each playmode will handle this a little differently..
    switch( m_playMode )
    {
      case GG_PLAYMODE_TOEND:
        return false;           //  Past end of movie
        //  no break, follow through..
      case GG_PLAYMODE_HANG:
        t = dur;               //  "Hang" on end T
        break;
      case GG_PLAYMODE_LOOP:
        if( dur < 1 )          //  watch out for % 0
          t = 0;
        else
          t %= dur;            //  loop around
        break;
    }
  }

  GG_Matrix3f m;
  m.makeIdentity();
  return m_movie->findHit( m, x, y, t, id );
}


bool GG_Player_impl::findHiti( int ix, int iy, int *id )
{
  if( m_movie == null )
    return false;

  float x, y, s;
  int   w = m_rcView.x2 - m_rcView.x1,      //  Transform to world coordinates
        h = m_rcView.y2 - m_rcView.y1;

  if( w < 1 || h < 1 )
    return false;         //  unusable viewrect

  if( w > h )
    s = 2.0f / h;
  else
    s = 2.0f / w;

  x = (ix - (w / 2)) * s;
  y = (h - iy - (h / 2)) * s;

  return findHitf( x, y, id );
}
