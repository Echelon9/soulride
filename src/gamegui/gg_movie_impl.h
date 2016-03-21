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
//  File: gg_movie_impl.h
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#ifndef _GG_MOVIE_IMPL_INCLUDED
#define _GG_MOVIE_IMPL_INCLUDED


#include "gamegui.h"
#include "gg_script.h"
#include "gg_file.h"
#include "gg_string.h"

#include <vector>


///////////////////////////////////////////////////////
//
//  GG_Movie_impl class
//
//
class GG_Movie_impl : public GG_Movie
{
  protected:
    GameGUI                 *m_gg;          //  Owner GameGUI object
    GG_Callbacks            m_callbacks;    //  Loader callbacks
    int                     m_refCount;
    vector<GG_Movie_impl*>  *m_ownerList;   //  List this object removes itself from on unRef=0
    GG_PlayMode             m_playMode;     //  TOEND, HANG, LOOP
    GG_Script               *m_script;      //  Script tree
    vector<GG_Actor*>       m_actors;       //  All actors in movie
    vector<GG_AppID*>       m_appIDs;       //  List of all keyword IDs
    int32                   m_duration;     //  Total duration of movie
    GG_ActorState           m_state;        //  "Identity" state

    //  Internal funcs to help read & parse files

    GG_Rval readPlayMode( char *buf );
    int parseLine( char *buf, GG_File *f, vector<GG_Resource*> *res );
    GG_Rval readIncludeFile( char *fileName, vector<GG_AppID*> *vIDs );

  public:
    GG_Movie_impl(GameGUI *gg, const GG_Callbacks &cb)
    {
      m_gg = gg;
      m_callbacks = cb;
      m_refCount = 0;
      m_ownerList = null;
      m_playMode = GG_PLAYMODE_TOEND;
      m_script = null;
      m_duration = 0;
      m_state.bPlaying = true;
    }

    ~GG_Movie_impl()
    {
      if( m_script != null )
        delete m_script;        //  Script destructor deletes all children

      //  Delete all actor resources
      for( vector<GG_Actor*>::iterator i = m_actors.begin();
           i < m_actors.end(); i++ )
        if( *i != null )
          delete *i;
      for( vector<GG_AppID*>::iterator k = m_appIDs.begin();
           k < m_appIDs.end(); k++ )
        if( *k != null )
          delete *k;
    }

    ///////////////////////////////////////////////////
    //
    //  Functions only used by the implementation
    //
    void setOwnerList( vector<GG_Movie_impl*> *ownerList );

    GG_Rval read( GG_File *f );
    GG_Rval read( GG_File *f, vector<GG_Resource*> *res );
          //  Reads movie data from a file, using the callback
          //  to first allow caller (GameGUI_impl::loadMovie)
          //  to filter the line for commands it must parse.
          //  Returns appropriate error code.
          //  A movie should be "safe" if it does't read ok,
          //  but will likely not play as intended.

    GG_Rval write( GG_File *f );
          //  Write out a movie file using current data.
          //  Should be a usable movie file.

    GG_Rval doFrame( const GG_ActorState &state, int32 t0, int32 dt );
          //  This plays one frame of the movie.
          //
          //  t0 - start time of play frame
          //  dt - amount of time (ms) to play through

    void stop(void);
          //  Stops movie -- handles anything that needs "stopping"

    void logStats(void);
          //  Writes some stats to a logfile

    vector<GG_Script*> *getChildScripts(void)
    {
      if( m_script == null )
        return null;

      return m_script->getChildScripts();
    }

    vector<GG_Actor*> *getActors(void)
    {
      return &m_actors;
    }

    bool detectHit( const GG_Matrix3f &m, float x, float y, int32 t, int id );
    bool findHit( const GG_Matrix3f &m, float x, float y, int32 t, int *pId );

    ///////////////////////////////////////////////////
    //
    //  Implementations of interface functions
    //
    int addRef(void);
    int unRef(void);
    int32 getDuration(void);
    GG_PlayMode getPlayMode(void);
    GG_Rval setPlayMode( GG_PlayMode mode );
    GG_Rval setActorText( int actId, const char *txt );
};



//
//  GG_Actor_Movie
//
//  This is a special GG_Actor class being defined:
//  an Actor that is a Movie.  This way, whole movies
//  can become actors and manipulated by a script.
//

class GG_Actor_Movie : public GG_Actor
{
  protected:
    GameGUI       *m_gg;
    GG_Movie_impl *m_movie;
    char          m_fileName[256];

  public:
    GG_Actor_Movie(GameGUI *gg, GG_Movie_impl *movie)
    {
      m_gg = gg;
      m_movie = movie;
      if( m_movie != null )
      {
        m_movie->addRef();
        m_duration = m_movie->getDuration();
      }
      m_fileName[0] = '\0';
      m_bHasChildren = true;
    }
    ~GG_Actor_Movie()
    {
      if( m_movie != null )
        m_movie->unRef();
    }
    GG_Rval read( GG_File *f );
    GG_Rval write( GG_File *f );
    GG_Rval doFrame( const GG_ActorState &state, int32 t0, int32 dt );
    bool detectHit( const GG_Matrix3f &m, float x, float y, int32 t, int id );
    bool findHit( const GG_Matrix3f &m, float x, float y, int32 t, int *pId );
    GG_Actor* findChild( int id );
};


#endif  // _GG_MOVIE_IMPL_INCLUDED
