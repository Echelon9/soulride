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
//  File: gg_script.cpp
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


#include "gg_script.h"
#include "gg_string.h"
#include "gg_log.h"

#ifndef LINUX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // not LINUX

#ifndef MACOSX
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif


static char msgBuf[256] = "";


///////////////////////////////////////////////////////
//
//  GG_Script member functions
//
//

GG_Script::~GG_Script()
{
  //  Dump all event lists for all types
  for( int id = 0; id < GG_MOVIE_NUM_EVENTS; id++ )
  {
    for( vector<GG_Event*>::iterator i = m_events[id].begin();
         i < m_events[id].end(); i++ )
    {
      if( *i != null )
        delete *i;
    }
  }

  //  Dump all child scripts
  for( vector<GG_Script*>::iterator j = m_childScripts.begin();
       j < m_childScripts.end(); j++ )
  {
    if( *j != null )
      delete *j;
  }
}


void GG_Script::addEvent( GG_Event *ev )
{
  vector<GG_Event*>  *v = &m_events[ev->id]; //  Use the event ID to pick the right list
  vector<GG_Event*>::iterator i;

  for( i = v->begin(); i < v->end(); i++ )
  {
    if( ev->t < (*i)->t )     //  iterate through the vector til we find the right spot
    {
      v->insert( i, ev );
      return;
    }
  }
  v->insert( i, ev );     //  or add it to the end.
}


//  An event is read by supplying a string, stripped
//  of all comments.  This does NOT accept blank lines as valid,
//  nor does it consider END a valid command.  Events are
//  single-line defintions.
//
GG_Rval GG_Script::readEvent( char *buf, int32 t )
{
  char    wordBuf[256], *p;
  int     id;
  GG_Rval rval = GG_OK;

  //  Get the event keyword
  GG_String::getWord(buf, wordBuf);
  GG_String::capitalize(wordBuf);

  //  Validate and turn into ID #
  id = GG_Event::getEventID( wordBuf );
  if( id < 0 )
    return GG_ERR_FILEFORMAT;

  //  Create new event object and set up what we know..
  //
  GG_Event *ev = GG_EventCreate(id);    //  Use the ID to create proper object
  if( ev == null )
    return GG_ERR;

  ev->id = id;
  ev->t = t;            //  Set the time supplied

  //  Now read the parameters into the event object
  p = GG_String::skipNextWord(buf); //  Advance p to first parameter
  if( (rval = ev->read(p)) != GG_OK )
  {
    delete ev;
    return rval;
  }

  //  Finally, scan this string for an interpolation setting (eg: "^2.0")
  ev->readInterp(p);

  //  Event data is filled in, add it to the appropriate list, sorted
  addEvent(ev);

  return GG_OK;
}


//  Reads a Script from supplied file stream.
//  Uses the list of actors supplied (acts) to search through
//  when a script is referring to actor by keyword.
//
GG_Rval GG_Script::read( GG_File *f, vector<GG_Actor*> *acts, vector<GG_AppID*> *appIDs )
{
  GG_Script       *script = null;
  GG_Event        *ev = null;
  GG_Rval         rval;
  char            buf[256], wordBuf[256], *p, *p1;
  int32           t = 0,
                  test = 0;
  int             appId = 0;
  vector<GG_Actor*>::iterator i;


  while( f->readLine(buf, 255) )
  {
    p = buf;

    if( GG_String::stripComment(p) )
      continue;   //  comment only

    //  Try to read a time value otherwise use last event time
    //
    if( sscanf( p, "%d", &test ) > 0 )
    {
      if( test < 0 )
      {
        GG_LOG( "error: GG_Script::read -- cannot use negative time index" );
        return GG_ERR_FILEFORMAT;
      }
      t = test;                           //  Update our time counter in this script
      p = GG_String::skipNextWord(p);     //  .. and move string ptr past the time value
    }

    if( t > m_duration )                  //  Check if duration has increased..
      m_duration = t;

    if( GG_String::getWord(p, wordBuf) == null ) //  Only a time index, no command.
      continue;

    GG_String::capitalize(wordBuf);

    if( strcmp(wordBuf, "END") == 0 )
    {
      //  No errors were encountered, reaching END signals
      //  all keyframe events (if any) were read successfully,
      //
      //  Note that an END within a SCRIPT may have a time index.
      //  This will force the end time.
      //
      return GG_OK;
    }

    //  Check for an existing actor keyword..
    //
    bool bFound = false;
    for( i = acts->begin(); i < acts->end(); i++ )
    {
      if( (*i)->cmpKeyWord(wordBuf) )
      {
        //  We DID find a matching actor.  That means
        //  the start of  a new child script, applying
        //  to the actor object we found.
        //
        //  Now check if this script has an app ID for app references..

        bFound = true;
        p = GG_String::skipNextWord(p);       //  Read data from after the actor keyword

        appId = 0;
        if( (p = GG_String::getWord(p, wordBuf)) != null &&
            (strcmp(wordBuf, "id=") == 0 || strcmp(wordBuf, "ID=") == 0 ))
        {
          if( sscanf(p, "%d", &appId) < 1 )
          {
            sscanf(p, "%s", wordBuf);
            bool bFoundAppId = false;
            vector<GG_AppID*>::iterator ai;
            GG_String::capitalize(wordBuf);
            for( ai = appIDs->begin(); ai != appIDs->end(); ai++ )
            {
              if( (*ai)->cmpKeyword(wordBuf) )
              {
                bFoundAppId = true;
                appId = (*ai)->id;
                break;
              }
            }
            if( !bFoundAppId )
            {
              sprintf( msgBuf, "error: GG_Script::read -- undefined application ID name: '%s'", wordBuf );
              GG_LOG( msgBuf );
              return GG_ERR_FILEFORMAT;
            }
          }
        }

        script = new GG_Script(*i, appId);
        script->m_startTime = t;
        if( (rval = script->read(f, acts, appIDs)) != GG_OK )
          return rval;
        t += script->m_duration;
        if( t > m_duration )
          m_duration = t;                       //  Check if total duration has increased
        m_childScripts.push_back(script);
        break;
      }
    }

    if( !bFound )
    {
      //  Well, the first word *wasn't* referring to a known
      //  actor, so that means it is an Event

      if( (rval = readEvent(p, t)) < GG_OK )
        return rval;
    }
    //  continue and do next line..
  }

  //  Must break out of a Script with an END.  Otherwise..
  return GG_ERR_EOF;
}


GG_Rval GG_Script::write( GG_File *f )
{
  char buf[256] = "";


  if( m_actor != null )
  {
    sprintf( buf, "%d ", m_startTime );
    f->writeText(buf);
    m_actor->getKeyWord(buf);
    f->writeText(buf);
    f->writeText("\n");
  }
  else
    f->writeText("SCRIPT\n");

  for( vector<GG_Script*>::iterator s = m_childScripts.begin();
       s < m_childScripts.end(); s++ )
    (*s)->write(f);

  GG_Rval rval;
  for( int i = 0; i < GG_MOVIE_NUM_EVENTS; i++ )
    for( vector<GG_Event*>::iterator ev = m_events[i].begin();
         ev < m_events[i].end(); ev++ )
      if( (rval = (*ev)->write(f)) != GG_OK )
        return rval;

  f->writeText("END\n");
  return GG_OK;
}


//  Given the list of events provided, find the 2
//  events bracketing this time value.  If the time value
//  falls outside event time bounds, corresponding returned
//  value will be null.  If there are NO events, e1 and e2
//  will both be null.
//
//  findEvents may benefit from a binary search
//
void GG_Script::findEvents( int32 t,
                        vector<GG_Event*> *v,
                        GG_Event **e1, GG_Event **e2 )
{
  vector<GG_Event*>::iterator i;
  *e1 = null;
  *e2 = null;
  for( i = v->begin(); i < v->end(); i++ )
  {
    if( (*i)->t <= t )
      *e1 = *i;
    else
      break;
  }

  //  How about the next event..
  //
  if( i < v->end() )
    *e2 = *i;
}


//  Get all interpolated values of moviestate for this time.
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
//
//
bool GG_Script::getActorState( int32 t, GG_ActorState *state )
{
  //  Reject out-of-bounds
  //
  if( t > m_duration )
    return false;

  GG_Event *e1, *e2;
  int      id;
  char     *p = (char*)state;   //  Pointer used to "iterate" through state's members

  //  Go through each event type list, interp that
  //  value between the two events cosest
  //
  for( id = 0; id < GG_MOVIE_NUM_EVENTS; id++ )
  {
    //  Find the 2 closest bracketing events
    findEvents( t, &m_events[id], &e1, &e2 );
    if( e1 != null )                      //  Only if there was a prior
    {                                     //  event do we make a change..
      if( e2 != null )
        e1->interpolate( t, e2, p );  //  Only if there exist 2 values to we interp.
      else
        e1->getData(p);                   //  Otherwise, use last event state
    }
    p += GG_Event::getDataSize(id);       //  on to next member of state struct
  }

  return true;
}


GG_Rval GG_Script::doFrame( const GG_ActorState &parentState, int32 t0, int32 dt )
{
  GG_ActorState state;
  GG_Rval       rval = GG_OK;
  int32         curActT, dtAct, durAct, t0Act,
                tOffset, tChild, dtChild;      //  Used to calc time to send to child (recursive)
  vector<GG_Script*>::iterator i;

  //  See if we've just come to the end of the script
  //
  if( m_actor != null && t0 < m_duration && t0 + dt >= m_duration )
    m_actor->stop();    //  Some actors need to be "turned off"

  dtAct = dt;
  curActT = t0 + dtAct;

  if( !getActorState( curActT, &state ))
    return GG_OK;     //  Time range falls entirely outside of this script's bounds

  if( m_actor != null )
    durAct = m_actor->getDuration();
  else
    durAct = 0;

  t0Act = t0;

  //  Modify state by the parentState..
  parentState.apply(&state);

  //  Check if we've gone past the end of the actor's duration..
  //
  if( curActT >= durAct )
  {
    //  Adjust values if needed depending on playmode
    switch( state.playMode )
    {
      case GG_PLAYMODE_HANG:
        curActT = durAct;               //  "Hang" on last end T
        dtAct = curActT - t0;
        break;
      case GG_PLAYMODE_LOOP:
        if( durAct < 1 )             //  watch out for % 0
          curActT = 0;
        else
          curActT %= durAct;            //  loop around
        if( dtAct > curActT )
          dtAct = curActT;
        break;
    }
    //  Adjust t0 now that dtAct/curActT may have changed
    t0Act = curActT-dtAct;
  }

  //  Push matrix stack, apply current transform to this
  //  actor, and child scripts

  glPushMatrix();
  glTranslatef( state.pos.x, state.pos.y, 0.0f );
  glRotatef( state.rot, 0.0f, 0.0f, 1.0f );
  glScalef( state.scale.x, state.scale.y, 1.0f );

  if( m_actor != null )
    if( (rval = m_actor->doFrame( state, t0Act, dtAct )) != GG_OK )
      goto exitNow;

  //  Child scripts get drawn on top
  //
  for( i = m_childScripts.begin(); i < m_childScripts.end(); i++ ) // << optimize by tree?
  {
    tOffset = (*i)->m_startTime;            //  When does this script start
    if( t0 + dt >= tOffset && t0 <= tOffset + (*i)->m_duration)   //  Overlap?
    {
      dtChild = dt;
      if( t0 < tOffset )            //  Did we start before the script?
      {
        dtChild -= (tOffset - t0);  //  adjust the dtChild we send
        tChild = 0;                 //  Yup, set to 0
      }
      else
        tChild = t0 - tOffset;      //  Otherwise, subtract offset from t0
      if( (rval = (*i)->doFrame(state, tChild, dtChild )) != GG_OK )    // recurse for child
        goto exitNow;
    }
  }

exitNow:
  glPopMatrix();
  return rval;
}


void GG_Script::stop(void)
{
  if( m_actor != null )
    m_actor->stop();
  for( vector<GG_Script*>::iterator i = m_childScripts.begin();
       i < m_childScripts.end(); i++ )
    (*i)->stop();
}

/*
GG_Rval GG_Script::pushMatrix(void)
{
  GG_Matrix3f m = m_matrixStack.back();
  m_matrixStack.push_back(m);
  return GG_OK;
}


GG_Rval GG_Script::popMatrix(void)
{
  if( m_matrixStack.size() < 2 )
  {
    GG_LOG("GG_Script::popMatrix() -- Internal error: matrix stack underflow!");
    return GG_ERR;
  }
  m_matrixStack.pop_back();
  return GG_OK;
}


GG_Rval GG_Script::multMatrix( const GG_Matrix3f &m )
{
  vector<GG_Matrix3f>::iterator mCur;
  mCur = m_matrixStack.end() - 1;
  mCur->mult( *mCur, m );
  return GG_OK;
}


GG_Rval GG_Script::rotate( float r )
{
  GG_Matrix3f mr;
  vector<GG_Matrix3f>::iterator m;
  mr.makeRotation(-GG_PI * r / 180.0);
  m = m_matrixStack.end() - 1;
  m->mult( *m, mr );
  return GG_OK;
}


GG_Rval GG_Script::scale( float sx, float sy )
{
  GG_Matrix3f ms;
  vector<GG_Matrix3f>::iterator m;
  ms.makeScale( sx, sy );
  m = m_matrixStack.end() - 1;
  m->mult( *m, ms );
  return GG_OK;
}


GG_Rval GG_Script::translate( float dx, float dy )
{
  GG_Matrix3f mt;
  vector<GG_Matrix3f>::iterator m;
  mt.makeTranslation( dx, dy );
  m = m_matrixStack.end() - 1;
  m->mult( *m, mt );
  return GG_OK;
}
*/

bool GG_Script::detectHit( const GG_Matrix3f &m,
                           float x, float y, int32 t, int id )
{
  GG_ActorState state;
  if( !getActorState( t, &state ))
    return false;   //  Instance of actor not on stage at this time
                    //  i.e., not within this script's time bounds

  int32 tOffset,
        tChild;
  bool  rval = false;
  vector<GG_Script*>::iterator i;
  GG_Matrix3f mt, mtx = m;

  //  Push matrix stack, apply current transform
//  pushMatrix();
//  multMatrix(m);                            //  Apply what we received
//  translate( state.pos.x, state.pos.y );    //  Apply this script's state
//  rotate( state.rot );
//  scale( state.scale.x, state.scale.y );

  mt.makeTranslation( state.pos.x, state.pos.y );
  mtx.mult( mtx, mt );
  mt.makeRotation( -state.rot * GG_PI / 180.0 );
  mtx.mult( mtx, mt );
  mt.makeScale( state.scale.x, state.scale.y );
  mtx.mult( mtx, mt );

//  mtxChild = m;                             //  Get a copy of the current matrix

  //  Child scripts appear on top, so check for hits on them first..
  //
  for( i = m_childScripts.begin(); i < m_childScripts.end(); i++ )
  {
    tOffset = (*i)->m_startTime;            //  When does this script start
    if( t >= tOffset && t <= tOffset + (*i)->m_duration)   //  Within?
    {
      tChild = t - tOffset;
      if( (rval = (*i)->detectHit(mtx, x, y, tChild, id)) == true )    //  Hit?
        goto exitNow;     //  Yup, found it.  Clean up and return
    }
  }

  if( m_actor != null && (id == m_id || m_actor->hasChildren()) )
  {
    rval = m_actor->detectHit( mtx, x, y, t, id ); // pass along the cur matrix
  }

exitNow:
//  popMatrix();
  return rval;
}


bool GG_Script::findHit( const GG_Matrix3f &m,
                         float x, float y, int32 t, int *pId )
{
  GG_ActorState state;
  if( !getActorState( t, &state ))
    return false;   //  Actor not on stage at this time

  int32 tOffset,
        tChild;
  bool  rval = false;
  vector<GG_Script*>::iterator i;
  GG_Matrix3f mt, mtx = m;

  //  Push matrix stack, apply current transform
//  pushMatrix();
//  multMatrix(m);
//  translate( state.pos.x, state.pos.y );
//  rotate( state.rot );
//  scale( state.scale.x, state.scale.y );
//  mtxChild = m_matrixStack.back();

  mt.makeTranslation( state.pos.x, state.pos.y );
  mtx.mult( mtx, mt );
  mt.makeRotation( -state.rot * GG_PI / 180.0 );
  mtx.mult( mtx, mt );
  mt.makeScale( state.scale.x, state.scale.y );
  mtx.mult( mtx, mt );

  //  Child scripts appear on top, so check for hits on them first..
  //
  for( i = m_childScripts.begin(); i < m_childScripts.end(); i++ ) // << optimize by tree?
  {
    tOffset = (*i)->m_startTime;            //  When does this script start
    if( t >= tOffset && t <= tOffset + (*i)->m_duration)   //  Within?
    {
      tChild = t - tOffset;
      if( (rval = (*i)->findHit(mtx, x, y, tChild, pId)) == true )    //  Hit?
        goto exitNow;     //  Yup, found it.  Clean up and return
    }
  }

  if( m_actor != null && (m_id != 0 || m_actor->hasChildren()) )  //  only check if this instance's id was set
  {
    if( (rval = m_actor->findHit( mtx, x, y, t, pId )) == true )
    {
      if( !m_actor->hasChildren() )   //  If it had children, id was set already
        *pId = m_id;
      goto exitNow;
    }
  }

exitNow:
//  popMatrix();
  return rval;
}
