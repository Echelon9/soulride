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
//  File: gg_movie_impl.cpp
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#include "gg_movie_impl.h"
#include "gg_string.h"
#include "gg_log.h"

#include <algorithm>

//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#include <gl/gl.h>      //  OpenGL-aware code (GG_Movie_Actor::doFrame)


//  Must match this string to first meaningful line
//  in file, otherwise it is not a movie file
//
#define GG_VERSION_STRING "GAMEGUIMOVIE 1.0"

static char msgBuf[256] = "";  //  For debug strings


///////////////////////////////////////////////////////
//
//  GG_Movie_impl member functions
//
///////////////////////////////

int GG_Movie_impl::addRef(void)
{
  m_refCount++;
  return m_refCount;
}


int GG_Movie_impl::unRef(void)
{
  m_refCount--;
  int r = m_refCount;
  if( r < 1 )
  {
//     if( m_ownerList != null )
//     {
//       vector<GG_Movie_impl*>::iterator i =
//                find( m_ownerList->begin(), m_ownerList->end(), this );
//       if( i != m_ownerList->end() )
//         m_ownerList->erase(i);
//     }
    delete this;
  }
  return r;
}


void GG_Movie_impl::setOwnerList( vector<GG_Movie_impl*> *ownerList )
{
  m_ownerList = ownerList;
}


GG_Rval GG_Movie_impl::readIncludeFile( char *fileName,
                                        vector<GG_AppID*> *appIDs )
{
  if( fileName == null )
    return GG_ERR;

  char lineBuf[256] = "",
       wordBuf[128] = "",
       *p = null;
  int  id = 0;
  GG_File *f = GG_FileOpen( fileName, GGFILE_OPENTXT_READ );
  if( f == null )
  {
    GG_LOG( "error: Failed to open include file:" );
    GG_LOG( fileName );
    return GG_ERR_FILEOPEN;
  }

  while( f->readLine(lineBuf, 250) )
  {
    if( GG_String::stripComment(lineBuf) )
      continue;

    p = GG_String::getWord( lineBuf, wordBuf );
    if( strcmp( wordBuf, "#define" ) == 0 && p != null )
    {
      if( sscanf( p, "%s %d", wordBuf, &id ) == 2 )
      {
        GG_AppID *appId = new GG_AppID( wordBuf, id );
        appIDs->push_back(appId);

        sprintf( msgBuf, "Read APPID: '%s' = %d", appId->key, appId->id );
        GG_LOG(msgBuf);
      }
    }
  }

  f->close();
  return GG_OK;
}


//  This internal function checks for the base set of
//  movie property settings.  It parses the supplied line,
//  and if necessary continues reading the file for more
//  data (eg: keyframes are multi-line defs)
//
//  Returns:  1 - parsed okay
//            0 - unrecognized, try parsing it yourself
//           -1 - semi-parsed until an error occurred
//
int GG_Movie_impl::parseLine( char *buf, GG_File *f, vector<GG_Resource*> *res )
{
  char      wordBuf[256] = "",
            buf2[256] = "";
  char      *p;
  int       rval = 0;
  GG_Actor  *act = null;
  int       appId = 0;

  if( GG_String::stripComment(buf) )          //  Is it just a comment?
    return 1;

  //  Get the first word..
  //
  if( (p = GG_String::getWord(buf, wordBuf)) == null )
    return 1;                                 //  Line was empty, so yes, we parsed it.

  if( strcmp(wordBuf, "#include") == 0 )
  {
    if( !GG_String::getQuote(buf, buf2) )
    {
      GG_LOG( "error: #include requires quoted filename" );
      return GG_ERR_FILEFORMAT;
    }

    if( (rval = readIncludeFile(buf2, &m_appIDs)) < GG_OK )
      return rval;
    return 1;
  }

  GG_String::capitalize(wordBuf);
  p = GG_String::getWord(p, buf2);                //  Get second word
  GG_String::capitalize(buf2);

  //  Check this against our keywords, take appropriate action.
  //
  //  First we check for "actor" defs..
  //
  if( strcmp(wordBuf, "POLYGON") == 0 )   //  Create a polygon
  {
    if( strlen(buf2) < 1 )
    {
      GG_LOG( "error: bad POLYGON definition in line:" );
      GG_LOG( buf );
      return GG_ERR_FILEFORMAT;
    }

    GG_Rect     rc;
    GG_Vector2D aspect;

    m_gg->getScreenRect(&rc);
    rc.getAspect(&aspect);

    GG_Act_Poly *poly = new GG_Act_Poly(aspect);
    if( (rval = poly->read(f, res)) != GG_OK )
    {
      delete poly;
      return rval;
    }
    poly->setKeyWord(buf2);     //  Give this actor it's name
    m_actors.push_back(poly);
    act = poly;                 //  remember this when checking for ID
    rval = 1;                   //  Parsed!
  }
  else if( strcmp(wordBuf, "TEXTLINE") == 0 )  //  Line of text
  {
    if( strlen(buf2) < 1 )
      return GG_ERR_FILEFORMAT;

    GG_Act_TextLine *txtLn = new GG_Act_TextLine(m_gg);
    if( (rval = txtLn->read(f)) != GG_OK )
    {
      delete txtLn;
      return rval;
    }
    txtLn->setKeyWord(buf2);
    m_actors.push_back(txtLn);
    act = txtLn;
    rval = 1;
  }
  else if( strcmp(wordBuf, "TEXTBOX") == 0 )  //  Line of text
  {
    if( strlen(buf2) < 1 )
      return GG_ERR_FILEFORMAT;

    GG_Act_TextBox *txtBox = new GG_Act_TextBox(m_gg);
    if( (rval = txtBox->read(f)) != GG_OK )
    {
      delete txtBox;
      return rval;
    }
    txtBox->setKeyWord(buf2);
    m_actors.push_back(txtBox);
    act = txtBox;
    rval = 1;
  }
  else if( strcmp(wordBuf, "SOUND") == 0 )     //  Create a sound actor
  {
    if( strlen(buf2) < 1 )
      return GG_ERR_FILEFORMAT;

    GG_Act_Sound *snd = new GG_Act_Sound(m_callbacks.loadSound, m_callbacks.loadSoundObj);
    if( (rval = snd->read(f)) != GG_OK )
    {
      delete snd;
      return rval;
    }
    snd->setKeyWord(buf2);
    m_actors.push_back(snd);
    act = snd;
    rval = 1;
  }
  else if( strcmp(wordBuf, "MOVIE") == 0 )     //  Load a movie from a file, create an actor of that
  {
    if( strlen(buf2) < 1 )
      return GG_ERR_FILEFORMAT;

    GG_Actor_Movie *actMov = new GG_Actor_Movie( m_gg, null );
    if( (rval = actMov->read(f)) != GG_OK )
    {
      delete actMov;
      return rval;
    }
    actMov->setKeyWord(buf2);
    m_actors.push_back(actMov);
    act = actMov;
    rval = 1;
  }
  else if( strcmp(wordBuf, "SCRIPT") == 0 )
  {
    if( m_script != null )
    {
      GG_LOG( "error: only 1 master script allowed in a movie file." );
      return GG_ERR_FILEFORMAT;
    }
    GG_Script *newScript = new GG_Script;
    if( (rval = newScript->read(f, &m_actors, &m_appIDs)) != GG_OK )
    {
      delete newScript;
      return rval;
    }
    m_script = newScript;
    m_duration = m_script->getDuration();
    return 1;
  }

  if( rval == 0 )
    return 0;       //  Unknown

  //  If we're still here, this was an actor that was
  //  read in successfully.  Now just check for an App. ID..
  //
  if( (p = GG_String::getWord(p, wordBuf)) == null )
    return rval;
  GG_String::capitalize(wordBuf);

  sprintf( msgBuf, "Checking for ID in '%s'", wordBuf );
  GG_LOG( msgBuf );

  if( strcmp(wordBuf, "ID=") != 0 )
    return rval;

  GG_LOG( "Found an ID" );

  if( sscanf(p, "%d", &appId) < 1 )
  {
    bool bFoundAppId = false;
    sscanf(p, "%s", wordBuf);
    vector<GG_AppID*>::iterator ai;
    GG_String::capitalize(wordBuf);
    for( ai = m_appIDs.begin(); ai != m_appIDs.end(); ai++ )
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
      sprintf( msgBuf, "error: Undefined application ID name: '%s'", wordBuf );
      GG_LOG( msgBuf );
      return GG_ERR_FILEFORMAT;
    }
  }

  sprintf( msgBuf, "Set App ID = %d", appId );
  GG_LOG( msgBuf );

  act->setID( appId );

  return rval;
}


void GG_Movie_impl::logStats(void)
{
  char  buf[256] = "";
  vector<GG_Actor*>::iterator a;

  GG_LOG( "--- GG_Movie stats ---");
  sprintf( msgBuf, "Duration: %d ms.", m_duration );
  GG_LOG( msgBuf );

  GG_LOG( "Actors:" );
  for( a = m_actors.begin(); a < m_actors.end(); a++ )
  {
    (*a)->getKeyWord(buf);
    sprintf( msgBuf, "%s (duration=%d)", buf, (*a)->getDuration() );
    GG_LOG( msgBuf );
  }
}


//  Movie reader
//  Passes off details to more specific functions
//
GG_Rval GG_Movie_impl::read( GG_File *f, vector<GG_Resource*> *res )
{
  GG_Rval rval;
  char    buf[256];
  bool    bVerified = false;

  //  Movie loader code.
  //  Read each line of the file, until end.
  //  Note: line length max is 255 chars.
  //
  while( f->readLine( buf, 255 ) )
  {
    if( GG_String::stripComment(buf) )
      continue;   // only comment

    //  The first line with content must be a file version ID string
    //
    if( !bVerified )
    {
      if( strcmp(buf, GG_VERSION_STRING) != 0 )
      {
        GG_LOG( "error: file missing version ID, not a proper .ggm file" );
        return GG_ERR_FILEFORMAT;                 //  Bad file format
      }
      bVerified = true;
      continue;
    }

    //  Use parseLine() once file is verified..
    //
    if( (rval = parseLine(buf, f, res)) < 0 )  //  Check for base cmds..
      return rval;                        //  Error read in line
    if( rval == 2 )                       //  Indicates END of movie; whatever was
      return GG_OK;                       //    read was okay (even no data).  END interpreted to truncate file
    if( rval == 0 )                       //  Line wasn't recognized by parseLine
      return GG_ERR_FILEFORMAT;           //    bail out.
  }

  return GG_OK;           //  End of file reached. (END keyword not necessary, but can be used to truncate file)
}


GG_Rval GG_Movie_impl::write( GG_File *f )
{
  GG_Rval rval;

  f->writeText( GG_VERSION_STRING );
  f->writeText( "\n\n\n" );

  for( vector<GG_Actor*>::iterator i = m_actors.begin(); i < m_actors.end(); i++ )
    if( (rval = (*i)->write(f)) != GG_OK )
      return rval;

  if( m_script != null )
    m_script->write(f);

  return GG_OK;
}


int32 GG_Movie_impl::getDuration(void)
{
  return m_duration;
}


GG_PlayMode GG_Movie_impl::getPlayMode(void)
{
  return m_playMode;
}


GG_Rval GG_Movie_impl::setPlayMode( GG_PlayMode mode )
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


GG_Rval GG_Movie_impl::setActorText( int id, const char *str )
{
  GG_Actor *act;
  vector<GG_Actor*>::iterator a;

  if( id == 0 )
    return GG_ERR_BADPARAM;

  for( a = m_actors.begin(); a != m_actors.end(); a++ )
  {
    if( (*a)->getID() == id )
    {
      (*a)->setText(str);
      return GG_OK;
    }
    else
    {
      if( (act = (*a)->findChild(id)) != null )
      {
        act->setText(str);
        return GG_OK;
      }
    }
  }

  return GG_ERR_OBJECTNOTFOUND;
}


//  doFrame
//
//  "plays" this movie for the duration (t0 -> t0+dt) specified,
//  using the parent state modifiers, and do all child Scripts
//
GG_Rval GG_Movie_impl::doFrame( const GG_ActorState &state, int32 t0, int32 dt )
{
  GG_Rval rval = GG_OK;

  if( m_script != null )
    rval = m_script->doFrame( state, t0, dt );

  return rval;
}


void GG_Movie_impl::stop(void)
{
  if( m_script != null )
    m_script->stop();
}


bool GG_Movie_impl::detectHit( const GG_Matrix3f &m,
                               float x, float y, int32 t, int id )
{
  if( m_script != null )
    return m_script->detectHit( m, x, y, t, id );
  return false;
}


bool GG_Movie_impl::findHit( const GG_Matrix3f &m,
                             float x, float y, int32 t, int *pId )
{
  if( m_script != null )
    return m_script->findHit( m, x, y, t, pId );
  return false;
}


///////////////////////////////////////////////////////
//
//  GG_Actor_Movie member functions
//
///////////////////////////////

//
//  Read a movie as a child (actor)
//
GG_Rval GG_Actor_Movie::read( GG_File *f )
{
  char buf[256] = "",
       wordBuf[128] = "",
       fileName[256] = "";

  if( m_gg == null )      //  Need this object, otherwise read() is disabled
    return GG_ERR;

  GG_Rval rval;
  while( f->readLine(buf, 255) )
  {
    //  See if base reader can handle it
    //
    rval = parseLine(buf, f);
    if( rval == 1 )       //  line was parsed by parseLine, move on..
      continue;
    if( rval < 0 )
      return rval;        //  Bad line.
    if( rval == 2 )
      return GG_OK;       //  END was encountered, we're done.

    //  otherwise, parseLine returned 0, meaning we should give it a shot..
    //
    if( !GG_String::getWord(buf, wordBuf) )
      continue;
    GG_String::capitalize(wordBuf);

    //
    //  Check wordBuf against GG_Poly specific keywords..
    //
    if( strcmp(wordBuf, "FILENAME") == 0 )
    {
      if( !GG_String::getQuote(buf, fileName))
        return GG_ERR_FILEFORMAT;

      GG_Movie_impl *newMovie = null;

      if( (rval = m_gg->loadMovie( fileName, (GG_Movie**)&newMovie)) != GG_OK )
        return rval;

      if( m_movie != null )
        m_movie->unRef();
      m_movie = newMovie;
      m_duration = m_movie->getDuration();
      strcpy( m_fileName, fileName );
      continue;
    }

    //  Unknown word encountered..
    return GG_ERR_FILEFORMAT;
  }

  return GG_OK;
}


GG_Rval GG_Actor_Movie::write( GG_File *f )
{
  f->writeText( "MOVIE " );
  f->writeText( m_keyWord );
  f->writeText( "\n" );
  f->writeText( "  FILENAME \"" );
  f->writeText( m_fileName );
  f->writeText( "\"\n" );
  return GG_Actor::write(f);
}


GG_Rval GG_Actor_Movie::doFrame( const GG_ActorState &state, int32 t0, int32 dt )
{
  GG_Rval rval = GG_OK;
  int32   curT = t0 + dt;

  //  Check if we've gone past the end of the movie..
  //
  if( curT >= m_duration )
  {
    //  Yes, each playmode will handle this a little differently..
    switch( m_playMode )
    {
      case GG_PLAYMODE_TOEND:
        if( t0 >= m_duration )
          return GG_OK;           //  Don't play anything after end of movie reached
        //  no break, follow through..
      case GG_PLAYMODE_HANG:
        curT = m_duration;      //  "Hang" on last end T
        dt = curT - t0;
        break;
      case GG_PLAYMODE_LOOP:
        if( m_duration < 1 )      //  watch out for % 0
        {
          curT = 0;
          t0 = 0;
          dt = 0;
        }
        else
        {
          curT %= m_duration;     //  loop around
          if( dt > curT )
            dt = curT;
        }
        break;
    }
    //  Adjust prevT now that dt/curT may have changed
    t0 = curT-dt;
  }

  if( m_movie != null )
    rval = m_movie->doFrame( state, t0, dt );

  return rval;
}


bool GG_Actor_Movie::detectHit( const GG_Matrix3f &m, float x, float y,
                                int32 t, int id )
{
  if( m_movie == null )
    return false;

  return m_movie->detectHit( m, x, y, t, id );
}


bool GG_Actor_Movie::findHit( const GG_Matrix3f &m, float x, float y,
                              int32 t, int *pId )
{
  if( m_movie == null )
    return false;

  return m_movie->findHit( m, x, y, t, pId );
}


GG_Actor* GG_Actor_Movie::findChild( int id )
{
  if( id == 0 )
    return null;

  GG_Actor* act;
  vector<GG_Actor*> *actors = m_movie->getActors();
  for( vector<GG_Actor*>::iterator a = actors->begin();
       a != actors->end(); a++ )
  {
    if( (*a)->getID() == id ) {
      return *a;
    } else if ((act = (*a)->findChild(id)) != null) {
      return act;
    }
  }

  return null;
}
