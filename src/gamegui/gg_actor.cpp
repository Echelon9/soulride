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
//  File: gg_actor.cpp
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#include "gg_actor.h"
#include "gg_string.h"
#include "gg_psdread.h"
#include "gg_log.h"

static const float GG_ACTOR_VOLUME_TOLERANCE = 0.01f;    //  Only call DS::SetVolume on changes of 1% or more


///////////////////////////////////////////////////////
//
//  Utility functions used by this file
//
///////////////////////////////

//
//  readFloatS()
//
//  Util function to read a floating point value.
//  If a % sign follows the numeric value,
//  the value is scaled by the supplied scale,
//  otherwise, the value is read literally.
//
static char* readFloatS( char *buf, float scale, float *fVal )
{
  char  wordBuf[256],
        *p = buf;
  int   len = 0;
  float s = 1.0f;
  float f;

  //  This function checks for the % modifier.
  //  Allows script file to specify percentage of viewport width/height..
  //
  if( (p = GG_String::getWord(p, wordBuf)) == null )
    return null;

  if( (len = strlen(wordBuf)) < 1 )
    return null;

  //  See if the last character is a %
  if( wordBuf[len-1] == '%' )
  {
    wordBuf[len-1] = '\0';    //  Yes, number is a % of range
    s = scale * 0.01f;
  }

  //  Now read the value..
  if( sscanf(wordBuf, "%f", &f) < 1 )
    return null;

  *fVal = f * s;

  return p;
}


#ifndef LINUX

//
//  getDSSoundBufDur()
//
//  Calculate (in ms) the duration
//  of a DirectSound buffer
//
static GG_Rval getDSSoundBufDur( LPDIRECTSOUNDBUFFER dsBuf, int32 *dur )
{
  LPWAVEFORMATEX      pWavFmt = null;
  DSBCAPS             dsbCaps;
  DWORD               fmtSize = 0;
  int32               sndDur = 0;
  GG_Rval             rval = GG_OK;

  memset(&dsbCaps, 0, sizeof(DSBCAPS));
  dsbCaps.dwSize = sizeof(dsbCaps);

  //  Get caps to find buffer size
  if( dsBuf->GetCaps(&dsbCaps) != DS_OK )
    return GG_ERR;

  // Get size of format struct.. <sigh, MS..>
  if( dsBuf->GetFormat(NULL, 0, &fmtSize) != DS_OK )
    return GG_ERR;
  pWavFmt = (LPWAVEFORMATEX)new char[fmtSize];
  memset(pWavFmt, 0, fmtSize);

  //  Get format to calculate duration from buffer size
  if( dsBuf->GetFormat(pWavFmt, fmtSize, NULL) != DS_OK )
  {
    rval = GG_ERR;
    goto bailExit;
  }

  //  Calculate with floats so we don't exceed int range
  sndDur = (int32)(1000.0f * dsbCaps.dwBufferBytes / pWavFmt->nChannels /
                  (pWavFmt->wBitsPerSample / 8) / pWavFmt->nSamplesPerSec);
  *dur = sndDur;  // maxes out at 2^31 ms

bailExit:
  if( pWavFmt != null )
    delete[] (char*)pWavFmt;
  return rval;
}


#endif // not LINUX


///////////////////////////////////////////////////////
//
//  GG_Actor member functions
//
///////////////////////////////

GG_Rval GG_Actor::readPlayMode( char *buf )
{
  char  wordBuf[256];

  GG_String::getWord(buf, wordBuf);

  if( strcmp(wordBuf, "TOEND") == 0 )
    m_playMode = GG_PLAYMODE_TOEND;
  else if( strcmp(wordBuf, "HANG") == 0 )
    m_playMode = GG_PLAYMODE_HANG;
  else if( strcmp(wordBuf, "LOOP") == 0 )
    m_playMode = GG_PLAYMODE_LOOP;
  else
    return GG_ERR_FILEFORMAT;   //  Unrecognized PLAYMODE type

  return GG_OK;
}


//  This internal function checks for the base set of
//  actor property settings.  It parses the supplied line,
//  and if necessary continues reading the file for more
//  data (possible multi-line defs?)
//
int GG_Actor::parseLine( char *buf, GG_File *f )
{
  char    wordBuf[256], *p;
  int32   dur = 0;
  int     rval;


  if( GG_String::stripComment(buf))   //  Is it only a comment?
    return 1;

  GG_String::getWord(buf, wordBuf);

  p = GG_String::skipNextWord(buf);   //  Move p to next token

  if( strcmp(wordBuf, "END") == 0 )
    return 2;                         //  End of actor def. reached, all okay.

  if( strcmp(wordBuf, "PLAYMODE") == 0 )
  {
    if( (rval = readPlayMode(p)) < GG_OK )
      return rval;
    return 1;       //  Parsed line successfully, set appropirate default playmode
  }

  if( strcmp(wordBuf, "DURATION") == 0 )
  {
    if( sscanf(p, "%d", &dur) < 1 )
      return GG_ERR_FILEFORMAT;   //  No duration value found
    m_duration = dur;
    return 1;
  }

  return 0;         //  Unknown command/keyword, not filtered
}


//  The base reader.  This only understands basic commands,
//  since the base Actor class doesn't really *do* much.
//
GG_Rval GG_Actor::read( GG_File *f )
{
  GG_Rval rval;
  char    buf[256];

  while( f->readLine( buf, 255 ) )
  {
    if( (rval = parseLine(buf, f)) < 0 )  //  Check for base cmds..
      return rval;                        //  Error read in line
    if( rval == 2 )                       //  Indicates END of actor; whatever
      return GG_OK;                       //    was read in was okay
    if( rval == 0 )                       //  Line wasn't recognized by parseLine
      return GG_ERR_FILEFORMAT;           //    bail out.
  }

  return GG_ERR_EOF;      //  Should've encountered an END.
}


GG_Rval GG_Actor::write( GG_File *f )
{
  switch( m_playMode )
  {
    case GG_PLAYMODE_TOEND:
      f->writeText( "  PLAYMODE TOEND\n" );
      break;
    case GG_PLAYMODE_HANG:
      f->writeText( "  PLAYMODE HANG\n" );
      break;
    case GG_PLAYMODE_LOOP:
      f->writeText( "  PLAYMODE LOOP\n" );
      break;
    default:
      f->writeText( "*ERROR*  PLAYMODE is corrupt\n" );
      return GG_ERR;
  }
  f->writeText( "END\n\n" );
  return GG_OK;
}


GG_Rval GG_Actor::setKeyWord( char *keyword )    // No longer than GG_MAX_KEYWORD_LEN!!
{
  if( strlen(keyword) < 1 || strlen(keyword) >= GG_MAX_KEYWORD_LEN )
    return GG_ERR_BADPARAM;
  strcpy( m_keyWord, keyword );
  return GG_OK;
}


void GG_Actor::getKeyWord( char *keyword )    //  Must be at least GG_MAX_KEYWORD_LEN!!
{
  strcpy( keyword, m_keyWord );
}


bool GG_Actor::cmpKeyWord( char *keyword )
{
  if( strcmp(keyword, m_keyWord) == 0 )
    return true;
  return false;
}


GG_Actor* GG_Actor::findChild( int id )
{
  return null;
}


void GG_Actor::setText( const char *str )
{
  if( m_text != null )
  {
    delete[] m_text;
    m_text = null;
  }

  uint len;
  if( str == null || (len = strlen(str)) == 0 )
    return;

  m_text = new char[len+1];
  strcpy( m_text, str );
}

void GG_Actor::getText( char *str, uint maxChars )
{
  if( m_text == null )
    return;

  strncpy( str, m_text, maxChars );
}


int32 GG_Actor::getDuration(void)
{
  return m_duration;
}


GG_PlayMode GG_Actor::getPlayMode(void)
{
  return m_playMode;
}


GG_Rval GG_Actor::setPlayMode( GG_PlayMode mode )
{
  m_playMode = mode;
  return GG_OK;
}


//  doFrame
//
//  "plays" this actor for the duration (t0 -> t0+dt) specified,
//  using the parent state modifiers, and do all child Scripts
//
GG_Rval GG_Actor::doFrame( const GG_ActorState &parentState, int32 t0, int32 dt )
{
  return GG_OK;
}


///////////////////////////////////////////////////////
//
//  GG_Act_TextLine member functions
//
///////////////////////////////

GG_Act_TextLine::GG_Act_TextLine()
{
  m_playMode = GG_PLAYMODE_HANG;
  m_gg = null;
  m_fontFileName[0] = '\0';
  m_font = null;
  m_align = GG_TEXTALIGN_LEFT;
  m_style.size = 0.1f;
  m_style.stretch = 1.0;
  m_style.spacing = 0.0;
}

GG_Act_TextLine::GG_Act_TextLine(GameGUI *gg)
{
  m_playMode = GG_PLAYMODE_HANG;
  m_gg = gg;
  m_fontFileName[0] = '\0';
  m_font = null;
  m_align = GG_TEXTALIGN_LEFT;
  m_style.size = 0.1f;
  m_style.stretch = 1.0;
  m_style.spacing = 0.0;
}

GG_Act_TextLine::~GG_Act_TextLine()
{
  if( m_font != null )
    m_font->unRef();
}

//
//  Override parseLine()
//  We have specific intercepts for TEXTLINE actor def
//
int GG_Act_TextLine::parseLine( char *buf, GG_File *f )
{
  char    wordBuf[256], *p;
  GG_Rval rval = GG_OK;

  //  First see if base reader can handle it!
  //
  if( (rval = GG_Actor::parseLine(buf, f)) != GG_OK )
    return rval;

  //  No, we must handle this line..
  if( !GG_String::getWord(buf, wordBuf) )
    return GG_OK;
  GG_String::capitalize(wordBuf);

  p = GG_String::skipNextWord(buf);

  if( strcmp(wordBuf, "SIZE") == 0 )
  {
    if( sscanf( p, "%f", &m_style.size ) < 1 )
      return GG_ERR_FILEFORMAT;

    return 1;   //  Indicate we handled it
  }

  if( strcmp(wordBuf, "SPACING") == 0 )
  {
    if( sscanf( p, "%f", &m_style.spacing ) < 1 )
      return GG_ERR_FILEFORMAT;

    return 1;   //  Indicate we handled it
  }

  if( strcmp(wordBuf, "ALIGN") == 0 )
  {
    if( !GG_String::getWord(p, wordBuf) )
      return GG_ERR_FILEFORMAT;
    GG_String::capitalize(wordBuf);
    if( strcmp(wordBuf, "LEFT") == 0 )
      m_align = GG_TEXTALIGN_LEFT;
    else if( strcmp(wordBuf, "CENTER") == 0 )
      m_align = GG_TEXTALIGN_CENTER;
    else if( strcmp(wordBuf, "RIGHT") == 0 )
      m_align = GG_TEXTALIGN_RIGHT;
    else
      return GG_ERR_FILEFORMAT;

    return 1;   //  Indicate we handled it
  }

  if( strcmp(wordBuf, "FONT") == 0 )
  {
    if( m_font != null )
      return GG_ERR_FILEFORMAT;
    if( !GG_String::getQuote(buf, wordBuf) )
      return GG_ERR_FILEFORMAT;

    if( (rval = m_gg->loadFont(wordBuf, (GG_Font**)&m_font)) < GG_OK )
      return rval;

    return 1;   //  Indicate we handled it
  }

  if( strcmp(wordBuf, "TEXT") == 0 )
  {
    int   len;
    char  *newStr = null, *pSrc, *pDst;

    if( !GG_String::getQuote(buf, wordBuf) )
      return GG_ERR_FILEFORMAT;
    len = strlen(wordBuf);
    if( m_text != null )
      len += strlen(m_text);
    newStr = new char[len+1];
    newStr[0] = '\0';
    if( m_text != null )
    {
      strcpy( newStr, m_text );
      delete[] m_text;
      m_text = null;
    }
    pDst = newStr + strlen(newStr);

    for( pSrc = wordBuf; *pSrc != '\0'; pSrc++, pDst++ )
    {
      if( *pSrc == '\\' && *(pSrc+1) == 'n' )
      {
        *pDst = '\n';
        pSrc++;
      }
      else
        *pDst = *pSrc;
    }
    *pDst = '\0';
    m_text = newStr;

    return 1;   //  Indicate we handled it
  }

  return GG_OK; //  Indicate we didn't handle it
}


GG_Rval GG_Act_TextLine::read( GG_File *f )
{
  char    buf[256];
  GG_Rval rval = GG_OK;

  while( f->readLine(buf, 255) )
  {
    //  Pass each line off to parseLine..
    //  GG_Act_TextLine::parseLine() that is!
    rval = parseLine(buf, f);
    if( rval == 1 )
      continue;       //  Line parsed ok.  On to the next..
    if( rval == 2 )
      return GG_OK;   // END encountered, we're done

    return GG_ERR_FILEFORMAT;   //  skipped by parser, dunno what this is
  }

  return GG_ERR_FILEFORMAT;   //  unexpected EOF
}


GG_Rval GG_Act_TextLine::write( GG_File *f )
{
  return GG_OK;
}

GG_Rval GG_Act_TextLine::doFrame( const GG_ActorState &state, int32 t0, int32 dt )
{
  if( m_playMode == GG_PLAYMODE_TOEND && t0 + dt > m_duration )
    return GG_OK;

  if( m_font == null || m_text == null )
    return GG_OK;

  float         x = 0;
  GG_FontFX     fx;
  GG_Rval       rval;
  GG_Font_Style style = m_style;
  style.spacing = m_style.spacing + state.spacing.x;

  if( m_align != GG_TEXTALIGN_LEFT )
  {
    float w = m_font->getTextWidth( m_text, style );
    if( m_align == GG_TEXTALIGN_CENTER )
      x = -w * 0.5;
    else
      x = -w;
  }

  fx.flags = GG_FONTFX_COLORMOD;
  fx.clrMod = state.color;

  rval = m_font->drawText(m_text, x, 0.0f, 0, style, &fx, null);

  return rval;
}

///////////////////////////////////////////////////////
//
//  GG_Act_TextBox member functions
//
///////////////////////////////

GG_Act_TextBox::GG_Act_TextBox( GameGUI *gg )
{
  m_gg = gg;
  m_leading = m_style.size * 1.25;
  m_rect.set( -1.0, 1.0, 1.0, -1.0 );
}

GG_Act_TextBox::~GG_Act_TextBox()
{
}


GG_Rval GG_Act_TextBox::read( GG_File *f )
{
  char    buf[256], wordBuf[256];
  char    *p;
  GG_Rval rval = GG_OK;

  while( f->readLine(buf, 255) )
  {
    //  Pass each line off to parseLine..
    //  GG_Act_TextLine::parseLine() that is!
    rval = GG_Act_TextLine::parseLine(buf, f);
    if( rval == 1 )
      continue;       //  Line parsed ok.  On to the next..
    if( rval == 2 )
      return GG_OK;   // END encountered, we're done

    //  Not handled by parseLine(), we must handle this line..
    if( !GG_String::getWord(buf, wordBuf) )
      return GG_OK;
    GG_String::capitalize(wordBuf);
    p = GG_String::skipNextWord(buf);

    if( strcmp(wordBuf, "RECT") == 0 )
    {
      if( sscanf( p, "%f %f %f %f", &m_rect.x1, &m_rect.y1, &m_rect.x2, &m_rect.y2 ) < 4 )
        return GG_ERR_FILEFORMAT;
      continue;
    }

    if( strcmp(wordBuf, "LEADING") == 0 )
    {
      if( sscanf( p, "%f", &m_leading ) < 1 )
        return GG_ERR_FILEFORMAT;
      continue;
    }

    return GG_ERR_FILEFORMAT;   //  dunno what this is
  }

  return GG_ERR_FILEFORMAT;   //  unexpected EOF
}


GG_Rval GG_Act_TextBox::write( GG_File *f )
{
  return GG_OK;
}

GG_Rval GG_Act_TextBox::doFrame( const GG_ActorState &state, int32 t0, int32 dt )
{
  if( m_playMode == GG_PLAYMODE_TOEND && t0 + dt > m_duration )
    return GG_OK;

  if( m_font == null || m_text == null )
    return GG_OK;

  GG_FontFX fx;
  char      *p = m_text;
  float     x, y, w, widthRc;
  uint      nc;
  GG_Rval   rval = GG_OK;

  y = m_rect.y1 - m_style.size;
  if( (widthRc = m_rect.x2 - m_rect.x1) <= 0 )
    return GG_OK;

  fx.flags = GG_FONTFX_COLORMOD;
  fx.clrMod = state.color;

  //  Draw the text, word-wrapping at each line
  while( *p != '\0' && y >= m_rect.y2 )
  {
    nc = m_font->fitWordCharCount( p, widthRc, m_style, &w );

    switch( m_align )
    {
      case GG_TEXTALIGN_LEFT:
        x = m_rect.x1;
        break;
      case GG_TEXTALIGN_CENTER:
        x = (m_rect.x1 + m_rect.x2 - w) * 0.5;
        break;
      case GG_TEXTALIGN_RIGHT:
        x = m_rect.x2 - w;
    }

    if( nc < 1 )
      return GG_OK;   //  No characters fit
    if( (rval = m_font->drawText(p, x, y, nc, m_style, &fx, null)) < GG_OK )
      return rval;   //  err
    p += nc;
    y -= m_leading;
  }

  return GG_OK;
}


///////////////////////////////////////////////////////
//
//  GG_Act_Poly member functions
//
///////////////////////////////

GG_Act_Poly::GG_Act_Poly( const GG_Vector2D &aspect )
{
  m_playMode = GG_PLAYMODE_HANG;
  m_bitmapFileName[0] = '\0';
  m_texID = 0;
  m_bTexture = false;
  m_aspect = aspect;
}


GG_Rval GG_Act_Poly::read( GG_File *f )
{
  return read(f, null);
}


GG_Rval GG_Act_Poly::read( GG_File *f, vector<GG_Resource*> *res )
{
  GG_PSDRead::bitmap32  *bitmap = null;
  char        buf[256], wordBuf[256], resFileName[256];
  char        *p = null;
  int         rval;
  GG_Vector2D v2d = { 0, 0 };

  m_rcExtents.set(0,0,0,0);

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
    //  Check wordBuf against GG_Act_Poly specific keywords..
    //
    if( strcmp(wordBuf, "V") == 0 )
    {
      //  A vertex.  Read as much vertex data as was supplied..
      //  [x y] [a r g b] [u v]

      Vertex  v;

      p = GG_String::skipNextWord(buf);

      //  X and Y values may either be absolute values,
      //  or percentage of screen (indicated by trailing %)
      //
      if( (p = readFloatS(p, m_aspect.x, &v.p.x)) == null )
        return GG_ERR_FILEFORMAT;
      if( (p = readFloatS(p, m_aspect.y, &v.p.y)) == null )
        return GG_ERR_FILEFORMAT;

      //  Scan remaining values normally
      //
      rval = sscanf( p, "%f %f %f %f %f %f",
                     &v.c.a, &v.c.r, &v.c.g, &v.c.b,
                     &v.t.x, &v.t.y );
      switch(rval)
      {
        case 6:               //  All values were read in
          break;

        case 2:               //  Tex coords only, default white col.
          v.t.x = v.c.a;
          v.t.y = v.c.r;
          v.c.set(1,1,1,1);
          break;

        case 0:
          v.c.set(1,1,1,1);   //  Full white is default if none spec'd
        case 4:               //  Color was read if 4 values read
          v.t.x = v.p.x;      //  If no tex coords, default == poly coords (flipped y)
          v.t.y = -v.p.y;
          break;              //  (this should be changed to -1.0 <= t <= 1.0)

        default:              //  Bad number of values read in
          return GG_ERR_FILEFORMAT;
      }

      v.c.clamp();            //  Just ensure colors are within limits.

      //  Check if we need to enlarge the actor's extents rect..
      //
      if( v.p.x < m_rcExtents.x1 )
        m_rcExtents.x1 = v.p.x;
      else if( v.p.x > m_rcExtents.x2 )
        m_rcExtents.x2 = v.p.x;
      if( v.p.y < m_rcExtents.y1 )
        m_rcExtents.y1 = v.p.y;
      else if( v.p.y > m_rcExtents.y2 )
        m_rcExtents.y2 = v.p.y;

      m_verts.push_back(v);   //  Save this vertex
      m_vtBuf.push_back(v2d); //  Add an entry to our transformed point buffer
      continue;               //  Now on to the next line..
    }

    if( strcmp(wordBuf, "BITMAP") == 0 )
    {
      if( m_bTexture )
      {
        GG_LOG( "Polygon actor cannot use more than one texture bitmap" );
        return GG_ERR_FILEFORMAT;   //  Already have a texture!
      }

      //  Load the bitmap (assuming .psd files)
      GLuint  id = 0;

      if( !GG_String::getQuote(buf, wordBuf))
        return GG_ERR_FILEFORMAT;

      GG_File::getCurrentPath( resFileName, 255 );
      strcat( resFileName, wordBuf );
      strcpy( m_bitmapFileName, wordBuf );

      //  First search our resource list for an already-loaded identical..
      //
      bool bFound = false;
      if( res != null )
      {
        for( vector<GG_Resource*>::iterator r = res->begin();
             r != res->end();
             r++ )
        {
          if( (*r)->cmpFileName(resFileName) )
          {
            id = (GLuint)((*r)->getResource());
            (*r)->addRef();   //  Inc. ref count; another actor using this tex
            m_resource = *r;  //  This actor must remember to un-ref this resource on destruct
            bFound = true;
            break;
          }
        }
      }

      if( !bFound )
      {
        //  Not found in resource list, load it from a file
        //
        if( (bitmap = GG_PSDRead::ReadImageData32(resFileName)) == null )
          return GG_ERR_FILEOPEN;

        //  Create OpenGL texture from this bitmap
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   //  clamp?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // GL_NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (float)GL_MODULATE);

        //  The money shot
        glTexImage2D( GL_TEXTURE_2D, 0,
                      GL_RGBA4,             //  GL_RGB for no alpha bitmaps
                      bitmap->GetWidth(), bitmap->GetHeight(),
                      0, GL_RGBA,                 // GL_RGBA4, // GL_RGB5_A1,
                      GL_UNSIGNED_BYTE,
                      (GLubyte*)bitmap->GetData() );

        //  Add this resource to the list
        //
        if( res != null )
        {
          GG_Res_GLTexture *rc = new GG_Res_GLTexture( id, resFileName, res );
          res->push_back(rc);
          rc->addRef();   //  Start refcount at 1
          m_resource = rc;
        }
      }

      m_texID = id;       //  Save texture name
      m_bTexture = true;  //  Indicate this actor has a texture

      //  Clean up
      //
      delete bitmap;
      bitmap = null;
      continue;
    }

    //  unrecognized keyword in this line
    return GG_ERR_FILEFORMAT;
  }

  if( bitmap != null )
    delete bitmap;
  return GG_ERR_EOF;
}


GG_Rval GG_Act_Poly::write( GG_File *f )
{
  char buf[256];

  f->writeText( "POLYGON " );
  f->writeText( m_keyWord );
  f->writeText( "\n" );

  if( m_bTexture )
  {
    f->writeText( "  BITMAP \"" );
    f->writeText( m_bitmapFileName );
    f->writeText( "\"\n" );
  }

  vector<Vertex>::iterator v;
  for( v = m_verts.begin(); v < m_verts.end(); v++ )
  {
    sprintf( buf, "  V %f %f %f %f %f %f %f %f\n",
             v->p.x, v->p.y,
             v->c.a, v->c.r, v->c.g, v->c.b,
             v->t.x, v->t.y );
    f->writeText( buf );
  }
  return GG_Actor::write(f);
}


GG_Rval GG_Act_Poly::doFrame( const GG_ActorState &state, int32 t0, int32 dt )
{
  if( m_playMode == GG_PLAYMODE_TOEND && t0 + dt > m_duration )
    return GG_OK;

  bool  bAlpha = false;

  if( m_bTexture )    //  Textured?
  {
    glEnable(GL_TEXTURE_2D);
//    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // GL_REPLACE
    glBindTexture(GL_TEXTURE_2D, m_texID);
  }

//  if( state.color.a < 0.99 )
//  {
    bAlpha = true;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendFunc(GL_ONE, GL_ONE);
//  }

  //  Send the vertices..
  //
  vector<Vertex>::iterator v, vEnd = m_verts.end();

  glBegin( GL_POLYGON );
  for( v = m_verts.begin(); v != vEnd; v++ )
  {
    glColor4f( v->c.r * state.color.r,
               v->c.g * state.color.g,
               v->c.b * state.color.b,
               v->c.a * state.color.a );

    if( m_bTexture )
      glTexCoord2f( v->t.x, v->t.y );

    glVertex2f( v->p.x, v->p.y );
  }
  glEnd();

  if( bAlpha )
  {
    glDisable(GL_BLEND);
  }

  if( m_bTexture )    //  Turn off texturing (if it was on)
    glDisable(GL_TEXTURE_2D);

  return GG_OK;
}


vector<GG_Act_Poly::Vertex>* GG_Act_Poly::getVertices(void)
{
  return &m_verts;
}


//  Copy from a set of supplied vertices, replacing what exists now.
//  Memory re-alloc'ed to appropriate size via <vector> =
void GG_Act_Poly::setVertices( const vector<GG_Act_Poly::Vertex> &verts )
{
  GG_Vector2D v = { 0, 0 };
  m_verts = verts;
  m_vtBuf.erase( m_vtBuf.begin(), m_vtBuf.end() );
  m_vtBuf.insert( m_vtBuf.begin(), m_verts.size(), v ); // resize xformed pt buffer
}


//
//  Detect a hit within this poly, transformed by the supplied matrix
//
bool GG_Act_Poly::detectHit( const GG_Matrix3f &m, float x, float y, int32 t, int id )
{
  vector<Vertex>::iterator      vtx, vtxEnd = m_verts.end();
  vector<GG_Vector2D>::iterator v, v2, vEnd = m_vtBuf.end();
  int                           numIntersects = 0;
  GG_LineSeg2D                  ln1, ln2;


  for( vtx = m_verts.begin(), v = m_vtBuf.begin();
       vtx != vtxEnd; vtx++, v++ )
  {
    m.multVector2D( &vtx->p.x, &v->x );
  }

  //  Now m_vtBuf contains a list of the transformed points.
  //  Now we check for intersections a ray cast from the
  //  center of the polygon outwards against each of the
  //  poly's line segments.  Even hits = outside, Odd hits = inside

  ln1.set( x, y, x + 100000000.0f, y + 300000000.0f );  //  uhh.. guaranteed to be outside bounds (I hope)

  numIntersects = 0;
  for( v = m_vtBuf.begin(), v2 = v+1; v2 != vEnd; v++, v2++ )
  {
    ln2.set( v->x, v->y, v2->x, v2->y );
    if( ln1.intersect(ln2, null) )
      numIntersects++;
  }

  // AND.. do the last->first pt segment

  v2 = m_vtBuf.begin();
  v = m_vtBuf.end() - 1;
  ln2.set( v->x, v->y, v2->x, v2->y );
  if( ln1.intersect(ln2, null) )
    numIntersects++;

  if( numIntersects == 0 )
    return false;               //  Even # intersections, not inside

  if( numIntersects % 2 == 0 )
    return false;               //  Even # intersections, not inside

  return true;                  //  Odd # intersections, inside!
}


bool GG_Act_Poly::findHit( const GG_Matrix3f &m, float x, float y, int32 t, int *pId )
{
  return detectHit( m, x, y, t, *pId );   //  For poly, detect/find is equivalent
}




///////////////////////////////////////////////////////
//
//  GG_Act_Sound
//
///////////////////////////////

GG_Act_Sound::GG_Act_Sound(GG_CallbackFn_LoadSound sndLoader, void *slObj)
{
  m_fileName[0] = '\0';
#ifndef LINUX
  m_dsSoundBuf = NULL;
#endif // not LINUX
  m_fnSoundLoader = sndLoader;
  m_soundLoaderObj = slObj;
  m_volume = 1.0;
  m_bLooping = false;
}


GG_Act_Sound::~GG_Act_Sound()
{
#ifndef LINUX
  if( m_dsSoundBuf != NULL )
  {
    m_dsSoundBuf->Stop();
    m_dsSoundBuf->Release();
  }
#endif // not LINUX
}


GG_Rval GG_Act_Sound::read( GG_File *f )
{
  char    buf[256], wordBuf[256];
  char    *p = null;
  int     rval;

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
    //  Check wordBuf against GG_Act_Sound specific keywords..
    //
    if( strcmp(wordBuf, "FILENAME") == 0 )
    {
#ifndef LINUX
      LPDIRECTSOUNDBUFFER dsBuf = NULL;
#endif // not LINUX
      char  path[256] = "",
            fileName[256] = "";

      //  Sound filename, get the quoted filename..
      if( !GG_String::getQuote(buf, wordBuf) )
        return GG_ERR_FILEFORMAT;

      //  We'll build the name manually, since the load is passed
      //  to a callback, which may or may not use GG_File
      //
      GG_File::getCurrentPath(path, 255);
      GG_File::setCurrentPath(null);    //  Save & disable path setting

      sprintf( fileName, "%s%s", path, wordBuf );   //  Build full name manually

#ifndef LINUX
      if( m_fnSoundLoader != null )
      {
        //  Load the sound, using the app-supplied loader callback
        rval = m_fnSoundLoader(m_soundLoaderObj, fileName, (void**)&dsBuf);
      }
      else
#endif // not LINUX
        rval = GG_OK;

      GG_File::setCurrentPath(path);    //  Restore path setting

      if( rval != GG_OK )   //  Now we can deal with problems
        return rval;
      m_duration = 0;
#ifndef LINUX
      if( m_dsSoundBuf != NULL )  //  Junk any previously loaded dsound buffer
        m_dsSoundBuf->Release();
      m_dsSoundBuf = dsBuf;       //  Save the new dsound buf
      if( m_dsSoundBuf != NULL )
      {
        if( (rval = getDSSoundBufDur(m_dsSoundBuf, &m_duration)) != GG_OK)
          return rval;
        m_dsSoundBuf->AddRef();
      }
      else
#endif // not LINUX
        m_duration = 0;
      strcpy( m_fileName, wordBuf );    //  Remember its filename
      continue;
    }
  }

  return GG_ERR_EOF;
}



GG_Rval GG_Act_Sound::write( GG_File *f )
{
  f->writeText( "SOUND " );
  f->writeText( m_keyWord );
  f->writeText( "\n" );
  f->writeText( "  FILENAME \"" );
  f->writeText( m_fileName );
  f->writeText( "\"\n" );
  return GG_Actor::write(f);
}


GG_Rval GG_Act_Sound::doFrame( const GG_ActorState &state, int32 t0, int32 dt )
{
  //  Condition to stop: is looping and  t0 < dur && t0+dt > dur
  //
//  if( state.playMode == GG_PLAYMODE_LOOP )
//  {
//    if( t0 < m_duration && t0+dt >= m_duration )
//    {
//      m_dsSoundBuf->Stop();
//      return GG_OK;
//    }
//  }

  if( fabs(state.volume - m_volume) >= GG_ACTOR_VOLUME_TOLERANCE )
  {
    m_volume = state.volume;
    if( m_volume < 0.0f )
      m_volume = 0.0;
    else if( m_volume > 1.0f )
      m_volume = 1.0;
#ifndef LINUX
    if( m_dsSoundBuf != NULL )
      m_dsSoundBuf->SetVolume((long)((m_volume - 1.0f) * fabs((float)(DSBVOLUME_MAX - DSBVOLUME_MIN))));
#endif // not LINUX
  }

#ifndef LINUX
  if( m_dsSoundBuf == NULL )
    return GG_OK;
#endif // not LINUX

  //  Condition to play:  Not looping &&
  //                      t0 -> t0+dt ==  0 -> >0
  //
  if( m_bLooping || t0 > 0 || t0 + dt < 1 )
    return GG_OK;   //  otherwise exit, nothing to do

#ifndef LINUX
  m_dsSoundBuf->SetCurrentPosition(0);   //  Make sure it's rewound..

  DWORD flags = (state.playMode == GG_PLAYMODE_LOOP) ? DSBPLAY_LOOPING : 0;
  m_dsSoundBuf->Play( 0, 0, flags );
#endif // not LINUX

  if( state.playMode == GG_PLAYMODE_LOOP )
    m_bLooping = true;
  else
    m_bLooping = false;

  return GG_OK;
}


void GG_Act_Sound::stop(void)
{
#ifndef LINUX
  if( m_dsSoundBuf != NULL )
    m_dsSoundBuf->Stop();
#endif // not LINUX
  m_bLooping = false;
}

