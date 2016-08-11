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
//  File: gg_script.h
//
//  GameGUI Script class
//
//  Organizes, and searches events.
//
//  Start Date: Feb 14 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////

#ifndef _GG_MOVIESCRIPT_INCLUDED
#define _GG_MOVIESCRIPT_INCLUDED


#include "gamegui.h"
#include "gg_event.h"
#include "gg_actor.h"
#include <vector>


//  This struct maps out the actor's events in a quickly
//  searchable format.  Events are stored sequentially,
//  categorized by event type.
//
class GG_Script
{
  protected:
    int                m_id;                           //  Identifier (for app queries)
    int32              m_startTime;                    //  Start time (in ms) of actor
    int32              m_duration;                     //  Length (in ms) of actor Script
    bool               m_bSelected;                    //  Selected state (editor)
    GG_Actor           *m_actor;                       //  Actor this script's events affect
    vector<GG_Event*>  m_events[GG_MOVIE_NUM_EVENTS];  //  Array of each kind of event list
    vector<GG_Script*> m_childScripts;                 //  All scripts that are children of this
                                                       //   (which may in turn have children..)
//    vector<GG_Matrix3f> m_matrixStack;                 //  Used for hit-tests

    //  Internally used util funcs..
    //
    void addEvent( GG_Event *ev );
        //  This sorts a new event into place, however it isn't really optimal.
        //  Ideally, events would probably be a binary tree.  If the sequential
        //  list is too slow at runtime, binary tree searches may provide
        //  speedups.  Overhead shouldn't be a problem until you're dealing with
        //  a LOT of actors with a lot of keframes.

    GG_Rval readEvent( char *buf, int32 t );
        //  An event is read by supplying a string, stripped
        //  of all comments.  This does NOT accept blank lines as valid,
        //  nor does it consider END a valid command.  Events are
        //  single-line defintions.

    static void findEvents( int32 time, vector<GG_Event*> *v,
                            GG_Event **e1, GG_Event **e2 );
        //  Given the list of events provided, find the 2
        //  events bracketing this time value.  If the time value
        //  falls outside event time bounds, corresponding returned
        //  value will be null.  e1 or e2 may be null if not found
        //
        //  findEvents may benefit from a binary search..

  public:
    GG_Script()
    {
//      GG_Matrix3f mtx;
      m_id = 0;
      m_startTime = 0;
      m_duration = 0;
      m_bSelected = false;
      m_actor = null;
//      mtx.makeIdentity();
//      m_matrixStack.push_back(mtx);
    }

    GG_Script( GG_Actor *actor, int id )
    {
//      GG_Matrix3f mtx;
      m_id = id;
      m_actor = actor;
      m_startTime = 0;
      if( m_actor != null )
        m_duration = m_actor->getDuration();
      else
        m_duration = 0;
      m_bSelected = false;
//      mtx.makeIdentity();
//      m_matrixStack.push_back(mtx);
    }

    ~GG_Script();

    void select( bool s )
    {
      m_bSelected = s;
    }

    bool isSelected(void)
    {
      return m_bSelected;
    }

    GG_Rval read( GG_File *f, vector<GG_Actor*> *actors, vector<GG_AppID*> *appIds );
        //  Reads a Script from supplied file stream.
        //  Script events apply to script's actor, UNLESS the
        //  script specifies a specific actor, in which case
        //  the <vector> of actors supplied is used to find it.

    GG_Rval write( GG_File *f );
        //  Write this object to a script file.  Uses writeIt() to do children

    bool getActorState( int32 t, GG_ActorState *state );
        //  Get all interpolated values of actorstate for this time.
        //  Booleans are interpolated by using the last event change
        //
        //  In the case where there are NO events for that particular
        //  state value, the supplied struct member will not be changed.
        //  (i.e., it may contain default values that will remain)
        //
        //  This uses some funny pointer trickery to synchronously iterate
        //  through the events[] array and assigning properly sized the
        //  members of state.  There must be a better way to design this,
        //  but I can't think of it precisely.  I couldn't entirely
        //  polymorph Events; they need IDs.  There's probably some nifty
        //  way of doing this with <map> but the notation could get hairy..
        //
        //  Returns false if the given time falls entirely outside of
        //  the Script's timespan.

    GG_Rval doFrame( const GG_ActorState &parentState, int32 t0, int32 dt );
        //  Handle all events, interpolations across the specified time slice

    bool detectHit( const GG_Matrix3f &m, float x, float y, int32 t, int id );
    bool findHit( const GG_Matrix3f &m, float x, float y, int32 t, int *pId );

    void stop(void);
        //  Stop actor, child scripts

    int getID(void)
    {
      return m_id;
    }

    void setID(int id)
    {
      m_id = id;
    }

    int32 getDuration(void)
    {
      return m_duration;
    }

    void setDuration(int32 t)
    {
      m_duration = t;
    }

    int32 getStartTime(void)
    {
      return m_startTime;
    }

    void setStartTime(int32 t)
    {
      m_startTime = t;
    }

    GG_Actor *getActor(void)
    {
      return m_actor;
    }

    void setActor( GG_Actor *actor )
    {
      m_actor = actor;
    }

/*
    GG_Rval pushMatrix(void);
    GG_Rval popMatrix(void);
    GG_Rval multMatrix( const GG_Matrix3f &m );
    GG_Rval rotate( float r );
    GG_Rval scale( float sx, float sy );
    GG_Rval translate( float dx, float dy );
*/

    vector<GG_Event*> *getEvents( uint32 typeId )
    {
      if( typeId >= GG_MOVIE_NUM_EVENTS )
        return null;
      return &m_events[typeId];
    }

    vector<GG_Script*> *getChildScripts(void)
    {
      return &m_childScripts;
    }
};


#endif  // _GG_SCRIPT_INCLUDED
