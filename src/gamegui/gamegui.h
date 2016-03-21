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
//  File: gamegui.h
//
//  Start date: Feb 8 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#ifndef _GAMEGUI_INCLUDED
#define _GAMEGUI_INCLUDED

#include "gg_config.h"

//  Some basic types compiler may/may not support
//  Modify gg_types.h to suit compiler
//
#include "gg_types.h"     //  For bool, true, false, uint32, etc.
#include "gg_file.h"      //  Some basic file utils

#include <math.h>

#ifdef MACOSX
#include "../macosxworkaround.hpp"
#endif

#define GG_PI  3.1415926535897932384626433832795


//  Return value type from functions
//
typedef int GG_Rval;

#define GG_PLAYERFINISHED        1    //  Okay, however player is finished & doing nothing
#define GG_OK                    0    //  All okay!
#define GG_ERR                  -1    //  Generic error
#define GG_ERR_NOTIMPLEMENTED   -2    //  Function called is a stub
#define GG_ERR_BADPARAM         -3    //  Bad parameter received by function
#define GG_ERR_FILEOPEN         -4    //  Function called failed to open a file at some point
#define GG_ERR_MEMORY           -5    //  Memory allocation error occurred
#define GG_ERR_EOF              -6    //  Premature EOF encountered (usually due to missing END or parameter)
#define GG_ERR_FILEFORMAT       -7    //  Error parsing file
#define GG_ERR_OBJECTNOTFOUND   -8    //  Object referred to was not found
#define GG_ERR_MAXINSTANCES     -9    //  Too many instances requested of object (is it a singleton?)
#define GG_ERR_NOFONT           -10   //  No font currently selected for text function call

//  Game GUI setup flags passed to GameGUICreate
//
typedef uint32 GG_Flags;

#define GG_FLAGS_GRAPHICS 0x01      //  Graphics ON
#define GG_FLAGS_SOUND    0x02      //  Audio ON
                                    //  If an option is not selected, it will be inaccessable

//  Maximum length of keyword identifiers
//
#define GG_MAX_KEYWORD_LEN  64


//  Movie Play modes
//
typedef int GG_PlayMode;

#define GG_PLAYMODE_TOEND 0         //  Play to end, stop, return message
#define GG_PLAYMODE_HANG  1         //  Play to end, hang on last graphic and/or audio loop, return message
#define GG_PLAYMODE_LOOP  2         //  Play continuously looping


//  Text alignment
//
#define GG_TEXTALIGN_LEFT   0
#define GG_TEXTALIGN_CENTER 1
#define GG_TEXTALIGN_RIGHT  2


//  Message struct returned by GG_Movie::getMessage()
//  Use getMessage() after every call to play()
//  Call repeatedly until id == GGMSG_NONE.
//
struct GG_Message
{
  uint32  id;         //  Message ID
  uint32  time;       //  Time stamp of message
  uint32  param;      //  Possible associated data
  uint32  param2;     //    and more data
};

//  Recognized message ID values
//
#define GGMSG_ERR      -1     //  Message is ERROR!
#define GGMSG_NONE      0     //  No message
#define GGMSG_ENDED     1     //  End reached and playmode == _TOEND or _HANG
#define GGMSG_KEYDOWN   2     //  Key down message, param is key ID
#define GGMSG_KEYUP     3     //  Key up message, param is key ID



//  2D Vector struct, fp values with useful methods
//
struct GG_Vector2D
{
  float x, y;

  inline void set( float x1, float y1 )
  {
    x = x1;  y = y1;
  }

  inline float length(void)
  {
    return sqrtf(x * x + y * y);
  }

  inline float lengthSq(void)
  {
    return x * x + y * y;
  }

  inline float dist(const GG_Vector2D &v)
  {
    GG_Vector2D d;
    d.set( v.x - x, v.y - y );
    return d.length();
  }

  inline float distSq(const GG_Vector2D &v)
  {
    GG_Vector2D d;
    d.set( v.x - x, v.y - y );
    return d.lengthSq();
  }
};


struct GG_Rect2D
{
  float x1, y1, x2, y2;

  inline void set( float xx1, float yy1, float xx2, float yy2 )
  {
    x1 = xx1;  y1 = yy1;  x2 = xx2;  y2 = yy2;
  }
};


//  2D line segment

class GG_LineSeg2D
{
  public:
    GG_Vector2D  p1, p2;

  inline void set( float x1, float y1, float x2, float y2 )
  {
    p1.x = x1;
    p1.y = y1;
    p2.x = x2;
    p2.y = y2;
  }

  //  Test this line seg against supplied,
  //  output intersect point to pt.
  //  No intersection returns false.
  //
  bool intersect( const GG_LineSeg2D &l, GG_Vector2D *pt ) const
  {
    float ua, ub;
    float den = (l.p2.y - l.p1.y) * (p2.x - p1.x) - (l.p2.x - l.p1.x) * (p2.y - p1.y);

    //  Are the lines parallel? (within some degree of tolerance)
    //
    if( (-0.00000001 < den) && (den < 0.00000001) )
      return false;

    ua = (l.p2.x - l.p1.x) * (p1.y - l.p1.y) - (l.p2.y - l.p1.y) * (p1.x - l.p1.x);
    ua /= den;

    ub = (p2.x - p1.x) * (p1.y - l.p1.y) - (p2.y - p1.y) * (p1.x - l.p1.x);
    ub /= den;

    //  Is the intersection within the segments of both lines?
    //
    if( (0.0 <= ua) && (ua <= 1.0) && (0.0 <= ub) && (ub <= 1.0) )
    {
      if( pt != null )    //  Send back the intersect point
      {                   //  (if point supplied)
        pt->x = p1.x + ua * (p2.x - p1.x);
        pt->y = p1.y + ua * (p2.y - p1.y);
      }
      return true;
    }

    return false;
  }
};


struct GG_Point
{
  int x, y;

  inline void set( int x1, int y1 )
  {
    x = x1;  y = y1;
  }

  bool getAspect( GG_Vector2D *aspect ) const
  {
    if( x < y )
    {
      if( x == 0 )
        return false;
      aspect->x = 1.0f;
      aspect->y = (float)y / x;
    }
    else
    {
      if( y == 0 )
        return false;
      aspect->x = (float)x / y;
      aspect->y = 1.0f;
    }
    return true;
  }
};


//  Integer rectangle
//
struct GG_Rect
{
  int x1, y1, x2, y2;

  inline void set( int xx1, int yy1, int xx2, int yy2 )
  {
    x1 = xx1; y1 = yy1; x2 = xx2; y2 = yy2;
  }

  inline bool getAspect( GG_Vector2D *aspect ) const
  {
    GG_Point p;
    p.set( x2-x1, y2-y1 );
    return p.getAspect(aspect);
  }

  inline int width(void) const
  {
    return x2 - x1;
  }

  inline int height(void) const
  {
    return y2 - y1;
  }

  inline bool inside( const GG_Point &pt ) const
  {
    if( pt.x >= x1 && pt.x < x2 &&
        pt.y >= y1 && pt.y < y2 )
      return true;
    return false;
  }

  inline bool inside( int x, int y ) const
  {
    if( x >= x1 && x < x2 && y >= y1 && y < y2 )
      return true;
    return false;
  }
};



//  ARGB Color struct, floating point values
//
struct GG_ColorARGBf
{
  float a, r, g, b;

  inline void set( float a1, float r1, float g1, float b1 )
  {
    a = a1;  r = r1;  g = g1;  b = b1;
  }

  void clamp(void)          //  Ensures safe color range
  {
    if( a > 1.0f )
      a = 1.0f;
    else if( a < 0.0f )
      a = 0.0f;

    if( r > 1.0f )
      r = 1.0f;
    else if( r < 0.0f )
      r = 0.0f;

    if( g > 1.0f )
      g = 1.0f;
    else if( g < 0.0f )
      g = 0.0f;

    if( b > 1.0f )
      b = 1.0f;
    else if( b < 0.0f )
      b = 0.0f;
  }

  void add( const GG_ColorARGBf &c )
  {
    a += c.a;               //  Capped addition
    if( a > 1.0f )          //  User must make sure clr values are all positive!
      a = 1.0f;

    r += c.r;
    if( r > 1.0f )
      r = 1.0f;

    g += c.g;
    if( g > 1.0f )
      g = 1.0f;

    b += c.b;
    if( b > 1.0f )
      b = 1.0f;
  }

  void sub( const GG_ColorARGBf &c )
  {
    a -= c.a;               //  Capped subtraction
    if( a < 0.0f )          //  User must make sure clr values are all positive!
      a = 0.0f;

    r -= c.r;
    if( r < 0.0f )
      r = 0.0f;

    g -= c.g;
    if( g < 0.0f )
      g = 0.0f;

    b -= c.b;
    if( b < 0.0f )
      b = 0.0f;
  }

  void mult( const GG_ColorARGBf &c )
  {
    a *= c.a;           //  Use legal values, or else!
    r *= c.r;
    g *= c.g;
    b *= c.b;
  }

  void blend( const GG_ColorARGBf &c, float scale )
  {
    float invScale = 1.0f - scale;    //  Scale must be between 0 and 1!
    a = a * invScale + c.a * scale;   //  Supplied color must be legal!
    r = r * invScale + c.r * scale;
    g = g * invScale + c.g * scale;
    b = b * invScale + c.b * scale;
  }
};


//
//  A 3x3 matrix for 2D transformations
//
struct GG_Matrix3f
{
  float _11, _12, _13,
        _21, _22, _23,
        _31, _32, _33;

  //  Assign result of multiplication to 'this'
  void mult( const GG_Matrix3f &m1, const GG_Matrix3f &m2 )
  {
    GG_Matrix3f tmp;

    tmp._11 = (m1._11 * m2._11) + (m1._12 * m2._21) + (m1._13 * m2._31);  // 1 top row
    tmp._12 = (m1._11 * m2._12) + (m1._12 * m2._22) + (m1._13 * m2._32);
    tmp._13 = (m1._11 * m2._13) + (m1._12 * m2._23) + (m1._13 * m2._33);

    tmp._21 = (m1._21 * m2._11) + (m1._22 * m2._21) + (m1._23 * m2._31);
    tmp._22 = (m1._21 * m2._12) + (m1._22 * m2._22) + (m1._23 * m2._32);
    tmp._23 = (m1._21 * m2._13) + (m1._22 * m2._23) + (m1._23 * m2._33);

    tmp._31 = (m1._31 * m2._11) + (m1._32 * m2._21) + (m1._33 * m2._31);  // 3 bottom row
    tmp._32 = (m1._31 * m2._12) + (m1._32 * m2._22) + (m1._33 * m2._32);
    tmp._33 = (m1._31 * m2._13) + (m1._32 * m2._23) + (m1._33 * m2._33);

    *this = tmp;
  }

  //  Assign result of mult to dst
  static void mult( const GG_Matrix3f &m1, const GG_Matrix3f &m2,
                    GG_Matrix3f *dst )
  {
    GG_Matrix3f tmp;

    tmp._11 = (m1._11 * m2._11) + (m1._12 * m2._21) + (m1._13 * m2._31);  // 1 top row
    tmp._12 = (m1._11 * m2._12) + (m1._12 * m2._22) + (m1._13 * m2._32);
    tmp._13 = (m1._11 * m2._13) + (m1._12 * m2._23) + (m1._13 * m2._33);

    tmp._21 = (m1._21 * m2._11) + (m1._22 * m2._21) + (m1._23 * m2._31);
    tmp._22 = (m1._21 * m2._12) + (m1._22 * m2._22) + (m1._23 * m2._32);
    tmp._23 = (m1._21 * m2._13) + (m1._22 * m2._23) + (m1._23 * m2._33);

    tmp._31 = (m1._31 * m2._11) + (m1._32 * m2._21) + (m1._33 * m2._31);  // 3 bottom row
    tmp._32 = (m1._31 * m2._12) + (m1._32 * m2._22) + (m1._33 * m2._32);
    tmp._33 = (m1._31 * m2._13) + (m1._32 * m2._23) + (m1._33 * m2._33);

    *dst = tmp;
  }

  //  Supply a ptr to array of 2 floats for both params.
  //  v1 is the result.  Will work if vStart and v1 point
  //  to the same space, but vStart values will be lost (natch)
  void multVector2D( const float *vOriginal, float *v1 ) const
  {
    float v0[2] = { vOriginal[0], vOriginal[1] };
    v1[0] = _11 * v0[0] + _12 * v0[1] + _13;
    v1[1] = _21 * v0[0] + _22 * v0[1] + _23;
  }

  void makeIdentity(void)
  {
    _11 = 1;  _12 = 0;  _13 = 0;
    _21 = 0;  _22 = 1;  _23 = 0;
    _31 = 0;  _32 = 0;  _33 = 1;
  }

  void makeTranslation( float x, float y )
  {
    makeIdentity();
    _13 = x;
    _23 = y;
  }

  void makeRotation( float r )
  {
    makeIdentity();
    _11 = cosf(r);
    _12 = sinf(r);
    _21 = -_12;
    _22 = _11;
  }

  void makeScale( float x, float y )
  {
    makeIdentity();
    _11 = x;
    _22 = y;
  }
};


#define GG_FONTFX_COLORMOD  0x01    //  Color modulation

struct GG_FontFX
{
  uint32        flags;      //  Which FX are to be used
  GG_ColorARGBf clrMod;
};


///////////////////////////////////////////////////////
//
//  Callback function typedefs
//
//  These callback functions should help to
//  avoid duplicate loader code, aid management of
//  system resources like DirectX and minimize
//  further  dependencies with app.
//
///////////////////////////////

typedef GG_Rval (*GG_CallbackFn_LoadSound)( void *appObj,                 //  App usage, can be null
                                            char *filename,               //  Sound filename to load
                                            void **directSound1Buffer );  //  DirectSoundBuffer
      //  Create a function in your app that takes a null-terminated filename string
      //  and fills a pointer to a DirectSound1 Sound Buffer (LPDIRECTSOUNDBUFFER)
      //  It should return GG_OK if successful, othewise GG will assume it failed.


//  This struct needs to be filled and passed to GameGUICreate()
//
struct GG_Callbacks
{
  GG_CallbackFn_LoadSound loadSound;    //  Ptr to app's loadSound function
  void*                   loadSoundObj; //  Ptr to app object, supplied to callback by GG
};


///////////////////////////////////////////////////////
//
//  GG_Font class
//
//
class GG_Font
{
  public:
    GG_Font()  { }
    virtual ~GG_Font()  { }

    virtual int addRef(void) = 0;
    virtual int unRef(void) = 0;
};


///////////////////////////////////////////////////////
//
//  GG_Movie class
//
//  Base class for all movie objects
//  Movie specifics are determined on loading data files
//
//
class GG_Movie
{
  public:
    GG_Movie()  { }
    virtual ~GG_Movie()  { }
          //  Use GameGUI::loadMovie to create an instance of this object
          //  if the GameGUI object used to create this Movie is destroyed,
          //  this movie will also be destroyed.
          //  use unRef() to release it.

    virtual int addRef(void) = 0;
    virtual int unRef(void) = 0;
          //  Reference management.  If unRef() returns 0, do not use as it has
          //  been deleted.  Returned value is generally only used for debugging.

    virtual int32 getDuration(void) = 0;
          //  Duration of this movie in milliseconds,
          //  not adjusted by time scale

    virtual GG_PlayMode getPlayMode(void) = 0;
    virtual GG_Rval setPlayMode( GG_PlayMode mode ) = 0;
          //  Playmodes:  GG_PLAYMODE_TOEND, GG_PLAYMODE_HANG, GG_PLAYMODE_LOOP
          //    _TOEND stops all audio at end, stops drawing all gfx after T end
          //    _HANG continues final audio loop (if one exists), hangs on last graphic frame
          //    _LOOP loops the movie back around to time index 0

    virtual GG_Rval setActorText( int actId, const char *txt ) = 0;
          //  Sets the text of the actor identified by the ID value.
          //  Returns GG_OK if successful,
          //          GG_ERR_OBJECTNOTFOUND if actor ID not found,
          //          GG_ERR_BADPARAM if id == 0 (not valid ID)
};


class GG_Player
{
  public:
    GG_Player()  { }
    virtual ~GG_Player()  { }
          //  Use GameGUI::createPlayer to create an instance of this object
          //  Use unRef() to release it.

    virtual int addRef(void) = 0;
    virtual int unRef(void) = 0;
          //  Reference management.  If unRef returns 0, do not use as it has been deleted.
          //  Returned value is generally only used for debugging.

	virtual GG_Movie*	getMovie() = 0;

    virtual int32 getDuration(void) = 0;
          //  Duration of the movie player is playing in milliseconds,
          //  -- not adjusted by time scale

    virtual GG_Rval play( int32 dt ) = 0;
          //  Play movie for 1 frame, using this delta t in ms

    virtual void stop(void) = 0;
          //  Stops playback.  Calls to play will start it up again,
          //  resuming at position it was stopped at.

    virtual int32 getTime(void) = 0;
    virtual GG_Rval setTime( int32 t ) = 0;
          //  get/set time index in millis of movie.  (0 = start)
          //  out of bounds values to setTime will clip or wrap
          //  depending on PLAYMODE

    virtual GG_Rval setScale( float x, float y ) = 0;
    virtual GG_Rval getScale( GG_Vector2D *s ) = 0;

    virtual GG_PlayMode getPlayMode(void) = 0;
    virtual GG_Rval setPlayMode( GG_PlayMode mode ) = 0;
          //  Playmodes:  GG_PLAYMODE_TOEND, GG_PLAYMODE_HANG, GG_PLAYMODE_LOOP
          //    _TOEND stops all audio at end, stops drawing all gfx after T end
          //    _HANG continues final audio loop (if one exists), hangs on last graphic frame
          //    _LOOP loops the movie back around to time index 0

    virtual bool detectHiti( int x, int y, int id ) = 0;
    virtual bool detectHitf( float x, float y, int id ) = 0;
          //  Check if the x,y coordinate is within the specified actor's bounding region
          //  Test takes place at the current playback time.
          //  Returns true if hit, false if not.
          //  Use the integer version for client window coordinates.
          //  Use the fp version for canvas coordinates.

    virtual bool findHiti( int x, int y, int *id ) = 0;
    virtual bool findHitf( float x, float y, int *id ) = 0;
          //  Find an actor hit by the x,y coordinate.
          //  Test takes place at the current playback time.
};


class GameGUI
{
  public:
    GameGUI()  { }
    virtual ~GameGUI()  { }
          //  Don't use constructor, use GameGUICreate()

    virtual int addRef(void) = 0;
    virtual int unRef(void) = 0;
          //  Reference management.  If unRef returns 0, do
          //  not use as it has been deleted.
          //  Returned value is generally only used for debugging.

    virtual GG_Rval createPlayer( GG_Movie *movie, GG_Player **player ) = 0;
          //  Creates a player object from an existing movie object

    virtual GG_Rval loadMovie( char *filename, GG_Movie **movie ) = 0;
    virtual GG_Rval loadMovie( GG_File *f, GG_Movie **movie ) = 0;
    virtual GG_Rval loadMovie( FILE *fp, GG_Movie **movie ) = 0;
          //  Loads a movie file, various params for reading

    virtual GG_Rval loadFont( char *fileName, GG_Font **font ) = 0;
    virtual GG_Rval loadFont( GG_File *f, GG_Font **font ) = 0;
    virtual GG_Rval loadFont( FILE *fp, GG_Font **font ) = 0;
          //  Loads a font file, various params for reading

    virtual GG_Rval setFont( GG_Font *font ) = 0;
          //  Sets the current font to use.  Must be called
          //  before any drawText() calls.

    virtual GG_Rval setFontSize( float fontSize ) = 0;
    virtual float getFontSize(void) = 0;
          //  Set/Get font size, measured from baseline to font height,
          //  using screen coordinates.
          //  Eg: 1.0 would be 1/2 the height of viewport

    virtual GG_Rval setFontSpacing( float spacing ) = 0;
    virtual float getFontSpacing(void) = 0;
          //  Letter spacing.  0 is normal, -0.1 is tighter, 0.1 is looser
          //  Proportional to height (base to top) of font.

    virtual GG_Rval setFontStretch( float stretch ) = 0;
    virtual float getFontStretch(void) = 0;
          //  Sets/gets the horizontal scaling of rendered type.  1.0 is normal.

    virtual GG_Rval setFontLeading( float leading ) = 0;
    virtual float getFontLeading(void) = 0;
          //  Line spacing

    virtual float getTextWidth( char *str ) = 0;
          //  Returns width of text according to current font settings

    virtual GG_Rval drawText( char *str, float x, float y,
                              uint align, GG_FontFX *fx ) = 0;
          //  Draws a single line of text using current font, at the spec'd
          //  coordinates using alignment.  fx can contain optional
          //  additional effects.  Use null for no effects.

    virtual GG_Rval drawTextBox( char *str, GG_Rect2D *rc,
                                 uint align, GG_FontFX *fx ) = 0;
          //  Draws text constrained and word-wrapped to the rectangle
          //  specified.  fx is optional (null for no effects)

    virtual GG_Rval writeMovie( char *filename, GG_Movie *movie ) = 0;
          //  Saves a movie file (but not the assets)

    virtual void getScreenRect( GG_Rect *rc ) = 0;
          //  Retrieves the rectangle specifying screen draw space
};


//  "Factory" functions.  First function to call to create instance
//  of GameGUI.
//
GG_Rval GameGUICreate( GG_Rect *rc,           //  Screen coordinates of viewport
                       GG_Callbacks *cb,      //  Callback functions supplied by app
                       GG_Flags flags,        //  Flags (none yet..)
                       GameGUI **gg );        //  Empty GameGUI pointer, filled on return.


//  (D3D version...?)
//
//  GG_Rval GameGUICreate_D3D(...)


#endif  // _GAMEGUI_INCLUDED
