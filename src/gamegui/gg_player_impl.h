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
//  File: gg_player_impl.h
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#ifndef _GG_PLAYER_IMPL_INCLUDED
#define _GG_PLAYER_IMPL_INCLUDED


#include "gamegui.h"
#include "gg_movie_impl.h"

// #include <vector>


///////////////////////////////////////////////////////
//
//  GG_Player_impl class
//
//
class GG_Player_impl : public GG_Player
{
  protected:
    int             m_refCount;
    vector<GG_Player_impl*> *m_ownerList;
    GG_Movie_impl   *m_movie;
    GG_PlayMode     m_playMode;
    int32           m_duration,         //  Total duration of movie
                    m_prevT;            //  Previous update time
    GG_Rect         m_rcView;           //  Viewport rect in screen coords
    GG_ActorState   m_state;


  public:
    GG_Player_impl(GG_Movie_impl *mov, const GG_Rect &rc)
    {
      m_refCount = 0;
      m_ownerList = null;
      m_playMode = GG_PLAYMODE_TOEND;
      m_movie = mov;
      m_movie->addRef();
      m_duration = m_movie->getDuration();
      m_prevT = 0;
      m_rcView = rc;
      m_state.bPlaying = true;
      m_state.scale.set( 1.0, 1.0 );
    }

    ~GG_Player_impl()
    {
      if( m_movie != null )
        m_movie->unRef();
    }

    void setOwnerList( vector<GG_Player_impl*> *ownerList );

    ///////////////////////////////////////////////////
    //
    //  Implementations of interface functions
    //
    int addRef(void);
    int unRef(void);
	GG_Movie*	getMovie();
    int32 getDuration(void);
    GG_PlayMode getPlayMode(void);
    GG_Rval setPlayMode( GG_PlayMode mode );
    GG_Rval play( int32 dt );
    void stop(void);
    int32 getTime(void);
    GG_Rval setTime(int32 t);
    GG_Rval setScale( float x, float y );
    GG_Rval getScale( GG_Vector2D *s );
    bool detectHitf( float x, float y, int id );
    bool detectHiti( int x, int y, int id );
    bool findHitf( float x, float y, int *id );
    bool findHiti( int x, int y, int *id );
};


#endif  // _GG_PLAYER_IMPL_INCLUDED
