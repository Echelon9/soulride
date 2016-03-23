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
//  File: gg_actor.h
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#ifndef _GG_ACTOR_INCLUDED
#define _GG_ACTOR_INCLUDED


#include "gamegui.h"
#include "gg_file.h"
#include "gg_resource.h"
#include "gg_font_impl.h"
#include <string.h>


///////////////////////////////////////////////////////
//
//  GG_AppID
//  - struct to hold application-defined name/ID pairs
//
///////////////////////////////

struct GG_AppID
{
  char  key [GG_MAX_KEYWORD_LEN];
  int   id;

  GG_AppID()
  {
    key[0] = '\0';
    id = 0;
  }
  GG_AppID( char *k, int i )
  {
    if( k != null )
      strncpy( key, k, GG_MAX_KEYWORD_LEN-1 );
    else
      key[0] = '\0';
    id = i;
  }
  ~GG_AppID() { }

  bool cmpKeyword( char *str )
  {
    if( str == null )
      return false;
    return( strcmp(str, key) == 0 );
  }
};


///////////////////////////////////////////////////////
//
//  Snapshot of this Actor's state, at a given time.
//  Actually, it's a snapshot of the *Script* at a
//  given time, but an actor uses this state as an
//  override to render itself properly at the time.
//
//  Kind of like a transform matrix for a variety
//  of properties.
//
struct GG_ActorState
{
  int           bPlaying;       //  Boolean state swtiches
  GG_Vector2D   spacing;
  int           bPaused,
                bMuted,
                bHidden;
  GG_PlayMode   playMode;       //  Mode of play (TOEND, HANG, LOOP)
  float         timeScale;      //  Time scaling -- speedup/slowdown
  GG_Vector2D   pos;            //  Center position of this actor
  float         rot;            //  Rotation about local center
  GG_Vector2D   scale;          //  Scale at local center
  GG_ColorARGBf color;          //  Color applied to (mult)
  float         volume;         //  Volume applied to (mult)

  GG_ActorState()   //  Just so's we don't have to type this in
  {                 //  every time.
    bPlaying = true;
    spacing.x = 0;  spacing.y = 0;
    bPaused = false;
    bMuted = false;
    bHidden = false;
    playMode = GG_PLAYMODE_TOEND;
    timeScale = 1.0f;
    pos.x = 0.0;  pos.y = 0.0;
    rot = 0.0;
    scale.x = 1.0;  scale.y = 1.0;
    color.a = 1.0; color.r = 1.0; color.g = 1.0; color.b = 1.0;
    volume = 1.0;
  }

  //  Override this object's state with the supplied object
  //  Note: this is not a copy!  Each parent state value
  //  "applies" itself to the state being modified in its
  //  own way.
  //
  void apply( GG_ActorState *s ) const
  {
    if( !bPlaying )
      s->bPlaying = false;           //  Playback 'off' prevents playback
    if( bPaused )
      s->bPaused = true;             //  pause 'on' overrides pause state
    if( bMuted )
      s->bMuted = true;              //  muted 'on' overrides mute state
    if( bHidden )
      s->bHidden = true;             //  hidden 'on' overrides hide state
    s->timeScale *= timeScale;       //  Multiplied
    s->color.mult(color);            //  Multiplied
  }
};


///////////////////////////////////////////////////////
//
//  GG_Actor class
//
//  Can be a polygon, bitmap, sound or a whole
//  other GameGUI Movie
//
//
class GG_Actor
{
  protected:
    char          m_keyWord[GG_MAX_KEYWORD_LEN];  //  As defined in file
    int           m_id;               //  ID set in movie file, can be accessed by app via this ID if non-zero
    char          *m_text;            //  Text associated with this actor (may have no effect)
    GG_Resource   *m_resource;        //  Resource used by actor (if any)
    GG_Rect2D     m_rcExtents;        //  Max extents rect
    GG_PlayMode   m_playMode;         //  default depends on Actor type
    bool          m_bHasChildren;     //  Has child movies t/f
    int32         m_duration;         //  Duration of actor (eg sound, movie)
                                      //   = 0 if bitmap/poly, etc.

    //  Internal funcs to help read & parse files
    //
    GG_Rval readPlayMode( char *buf );
    virtual int parseLine( char *buf, GG_File *f );

  public:
    GG_Actor()
    {
      m_keyWord[0] = '\0';
      m_id = 0;
      m_text = null;
      m_resource = null;
      m_playMode = GG_PLAYMODE_TOEND;
      m_duration = 0;
      m_rcExtents.set(0,0,0,0);
      m_bHasChildren = false;
    }
    virtual ~GG_Actor()
    {
      if( m_text != null )
        delete[] m_text;
      if( m_resource != null )
        m_resource->unRef();
    }

    int getID(void)
    {
      return m_id;
    }

    void setID(int id)
    {
      m_id = id;
    }

    virtual GG_Rval read( GG_File *f );
          //  Reads Actor data from a file, using the callback
          //  to first allow caller (GameGUI_impl::loadActor)
          //  to filter the line for commands it must parse.
          //  Returns appropriate error code.
          //  An Actor should be "safe" if it does't read ok,
          //  but will likely not play as intended.

    virtual GG_Rval write(GG_File *f);
          //  Writes actor data to a script file
          //  (doesn't write resource file data, just filenames)

    GG_Rval setKeyWord( char *keyword );
          //  Re-sets this actors keyword name
          //  Error if "" or too long

    void getKeyWord( char *keyword );
          //  Copies name to keyword
          //  Keyword must have GG_MAX_KEYWORD_LEN space to be safe

    bool cmpKeyWord( char *keyword );
          //  Compares supplied keyword for match
          //  (case sensitive, caller must capitalize before compare)

    void setText( const char *str );
    void getText( char *str, uint maxChars );
          //  Gets/sets text associated with this actor.

    bool hasChildren(void)
    {
      return m_bHasChildren;
    }

    virtual GG_Actor* findChild( int id );

    int32 getDuration(void);
    void setDuration( int32 dur );
          //  Default duration of actor, when added to a script

    GG_PlayMode getPlayMode(void);
    GG_Rval setPlayMode( GG_PlayMode mode );

    //  For picking/detecting clicks (coarse rect detection)
    GG_Rval getExtentsRect( GG_Rect2D *rc )
    {
      *rc = m_rcExtents;
      return GG_OK;
    }

    GG_Rval setExtentsRect( GG_Rect2D *rc )
    {
      m_rcExtents = *rc;
      return GG_OK;
    }

    virtual GG_Rval doFrame( const GG_ActorState &state, int32 t0, int32 dt );
          //  This plays one frame of the Actor
          //  It will play along the timeline from t0 to
          //  t0+dt drawing gfx, playing sounds, etc.
          //
          //  state - script-computed state of Actor
          //  t0 - start time of play frame
          //  dt - amount of time (ms) to play through

    virtual void stop(void)  { }
          //  Ensures actor is stopping
          //  (most actors don't need this except sound)

    virtual bool detectHit( const GG_Matrix3f &m, float x, float y, int32 t, int id )
    {
      return false;
    }

    virtual bool findHit( const GG_Matrix3f &m, float x, float y, int32 t, int *pId )
    {
      return false;
    }
};


///////////////////////////////////////////////////////
//
//  Derivations of GG_Actor..
//

//
//  TextLine
//

class GG_Act_TextLine : public GG_Actor
{
  protected:
    GameGUI       *m_gg;
    char          m_fontFileName[256];
    GG_Font_impl  *m_font;
    uint          m_align;
    GG_Font_Style m_style;

    GG_Rval parseLine( char *buf, GG_File *f );  //  Internal line parser func
                                                 //  Overrides default parseLine()
  public:
    GG_Act_TextLine();
    GG_Act_TextLine(GameGUI *gg);
    virtual ~GG_Act_TextLine();
    GG_Rval read( GG_File *f );
    GG_Rval write( GG_File *f );
    GG_Rval doFrame( const GG_ActorState &state, int32 t0, int32 dt );
};


//
//  TextBox (inherits from TextLine)
//

class GG_Act_TextBox : public GG_Act_TextLine
{
  protected:
    GG_Rect2D m_rect;
    float     m_leading;

  public:
    GG_Act_TextBox(GameGUI *gg);
    virtual ~GG_Act_TextBox();
    GG_Rval read( GG_File *f );
    GG_Rval write( GG_File *f );
    GG_Rval doFrame( const GG_ActorState &state, int32 t0, int32 dt );
};


//
//  Polygon
//

//  #ifdef GG_USE_OPENGL or something to control Windows & OpenGL include dependencies

#ifdef LINUX
#else // not LINUX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // not LINUX

#ifndef MACOSX
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif // not MACOSX


class GG_Act_Poly : public GG_Actor
{
  public:
    struct Vertex
    {
      GG_Vector2D   p;
      GG_ColorARGBf c;
      GG_Vector2D   t;
    };

  protected:

    char                m_bitmapFileName[256];  //  Bitmap filename (if textured)
    GLuint              m_texID;                //  OpenGL texture ID
    bool                m_bTexture;             //  Is/Not textured
    vector<Vertex>      m_verts;                //  All vertices
    vector<GG_Vector2D> m_vtBuf;                //  Transformed point buffer
    GG_Vector2D         m_aspect;               //  Aspect ratio of viewport

  public:
    GG_Act_Poly(const GG_Vector2D &aspect);
    virtual ~GG_Act_Poly()  {  }

    void tesselate(void);             //  Tesselate to convex.
    GG_Rval read( GG_File *f );
    GG_Rval read( GG_File *f, vector<GG_Resource*> *res );
    GG_Rval write( GG_File *f );
    GG_Rval doFrame( const GG_ActorState &state, int32 t0, int32 dt );
    bool detectHit( const GG_Matrix3f &m, float x, float y, int32 t, int id );
    bool findHit( const GG_Matrix3f &m, float x, float y, int32 t, int *pId );

    vector<Vertex> *getVertices(void);
          //  Get the "live" list of verts.  External changes made to this list stick!

    void setVertices( const vector<Vertex> &verts );
          //  Copy from a set of supplied vertices, replacing what exists now.
          //  Memory re-alloc'ed to appropriate size via <vector> =
};



//  #ifdef GG_USE_DIRECTSOUND or something to control DirectX include dependencies

#ifndef LINUX
#include <mmreg.h>
#include <dsound.h>
#endif // not LINUX

//  #endif

//
//  DirectSound Buffer actor
//
class GG_Act_Sound : public GG_Actor
{
  protected:
    char                    m_fileName[256];
#ifndef LINUX
    LPDIRECTSOUNDBUFFER     m_dsSoundBuf;
#endif // not LINUX
    float                   m_volume;             //  Previous volume setting
    bool                    m_bLooping;           //  Sound is currently looping
                                                  //  This sound is unique, it has a unique state
    GG_CallbackFn_LoadSound m_fnSoundLoader;
    void                    *m_soundLoaderObj;

  public:
    GG_Act_Sound(GG_CallbackFn_LoadSound sndLoader, void *slObj);
    virtual ~GG_Act_Sound();

    GG_Rval read( GG_File *f );
    GG_Rval write( GG_File *f );
    GG_Rval doFrame( const GG_ActorState &state, int32 t0, int32 dt );
    void stop(void);
};


#endif  // _GG_ACTOR_INCLUDED
