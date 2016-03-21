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
//  File: gamegui.cpp
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#include "gamegui.h"
#include "gg_font_impl.h"
#include "gg_movie_impl.h"
#include "gg_player_impl.h"
#include "gg_log.h"


#define GG_VERSION_STRING "GAMEGUIMOVIE 1.0"


static char gg_logBuf[256] = "";    //  Used for debug strings



///////////////////////////////////////////////////////
//
//  GameGUI implementation class
//
class GameGUI_impl : public GameGUI
{
  protected:
    int                     m_refCount;       //  Reference count
    GG_Callbacks            m_callbacks;      //  Struct of callback function pointers
    vector<GG_Font_impl*>   m_fonts;          //  List of all fonts loaded
    GG_Font_impl            *m_curFont;       //  Current font selected
    GG_Font_Style           m_fontStyle;      //  Current font style
    float                   m_fontLeading;    //  Line spacing
//    vector<GG_Movie_impl*>  m_movies;         //  List of all movie heirarchies
//    vector<GG_Player_impl*> m_players;        //  List of all players created
    GG_Rect                 m_rcScreen;       //  Drawing rect in screen coordinates
    GG_Vector2D             m_aspect;         //  x/y aspect ratio
    vector<GG_Resource*>    m_resources;      //  All resources, for re-use. (textures, )

  public:
    //
    //  Functions that override interface
    //
    GameGUI_impl( GG_Rect *rc, GG_Callbacks *cb, GG_Flags )
    {
      m_refCount = 1;
      m_rcScreen = *rc;

      m_curFont = null;
      m_fontStyle.size = 0.025f;
      m_fontStyle.spacing = 0.0f;
      m_fontStyle.stretch = 1.0f;
      m_fontLeading = m_fontStyle.size * 1.5;

      int w = m_rcScreen.x2 - m_rcScreen.x1,    //  Figure out the aspect ratio
          h = m_rcScreen.y2 - m_rcScreen.y1;    //  then scale appropriately
      if( w > h )
      {
        m_aspect.x = (float)h / w;
        m_aspect.y = 1.0f;
      }
      else
      {
        m_aspect.x = 1.0f;
        m_aspect.y = (float)w / h;
      }

      if( cb != null )
        m_callbacks = *cb;
      else
        memset( &m_callbacks, 0, sizeof(m_callbacks) );
    }

    ~GameGUI_impl()
    {
      //  Delete all movies & players contained in these lists..
      //
      vector<GG_Font_impl*>::iterator f;
//      vector<GG_Movie_impl*>::iterator i;
//      vector<GG_Player_impl*>::iterator j;
      vector<GG_Resource*>::iterator r;

      //  Be sure to delete players first..
//       for( j = m_players.begin(); j != m_players.end(); j++ )
//         if( *j != null )
// 			(*j)->setOwnerList(NULL);

//       for( i = m_movies.begin(); i != m_movies.end(); i++ )
//         if( *i != null )
// //          delete *i;
// 			(*i)->setOwnerList(NULL);

      for( f = m_fonts.begin(); f != m_fonts.end(); f++ )
        if( *f != null )
          delete *f;

      for( r = m_resources.begin(); r != m_resources.end(); r++ )
        if( *r != null )
          delete *r;
    }


    int addRef(void)
    {
      m_refCount++;
      return m_refCount;
    }


    int unRef(void)
    {
      m_refCount--;
      int r = m_refCount;
      if( r < 1 )
        delete this;
      return r;
    }


    void getScreenRect( GG_Rect *rc )
    {
      *rc = m_rcScreen;
    }


    void getAspect( GG_Vector2D *asp )
    {
      *asp = m_aspect;
    }


    GG_Rval loadFont( GG_File *f, GG_Font **font )
    {
      GG_Rval rval = GG_OK;

      GG_Font_impl *newFont = new GG_Font_impl( &m_fonts );
      if( (rval = newFont->read(f, &m_resources)) != GG_OK )
      {
        delete newFont;
        return rval;
      }

      m_fonts.push_back(newFont);
      newFont->addRef();
      *font = newFont;

      return GG_OK;
    }


    //  Other loadFont parameter options
    //
    GG_Rval loadFont( FILE *fp, GG_Font **font )
    {
      GG_File *f = GG_FileAliasFILE( fp );
      GG_Rval rval = loadFont( f, font );
      f->close();
      return rval;
    }


    GG_Rval loadFont( char *filename, GG_Font **font )
    {
      char  path[256] = "",
            fullPath[256] = "",
            oldPath[256] = "",
            *p = null;
      GG_Rval rval;
      GG_File *f;

      f = GG_FileOpen( filename, GGFILE_OPENTXT_READ );
      if( f == null )
      {
        GG_LOG( "error: Failed to open font file:" );
        GG_LOG( filename );
        return GG_ERR_FILEOPEN;
      }

      GG_File::getCurrentPath( oldPath, 255 );
      strcpy( fullPath, path );

      p = GG_String::skipDOSPath( filename );
      if( p > filename )
      {
        strncpy( path, filename, (uint32)(p - filename) );
        strcat( fullPath, path );
        GG_File::setCurrentPath( fullPath );

        GG_File::getCurrentPath( path, 255 );    //  Output for debug
      }

      if( (rval = loadFont(f, font)) != GG_OK )
      {
        sprintf( gg_logBuf, "ERROR type %d encountered reading font file: %s in line: %d", rval, filename, f->numLinesRead() );
        GG_LOG( gg_logBuf );
      }

      GG_File::setCurrentPath( oldPath );

      f->close();
      return rval;
    }


    //  Load movie, fill pointer with new movie object
    //
    GG_Rval loadMovie( GG_File *f, GG_Movie **movie )
    {
      GG_Rval rval;

      GG_Movie_impl *newMovie = new GG_Movie_impl(this, m_callbacks);
      if( (rval = newMovie->read(f, &m_resources)) != GG_OK )
      {
        delete newMovie;
        return rval;
      }

      newMovie->logStats();

//      m_movies.push_back(newMovie);
//      newMovie->setOwnerList( &m_movies );
      newMovie->addRef();     //  Set to 1

      *movie = newMovie;      //  Return it
      return GG_OK;
    }


    GG_Rval setFont( GG_Font *font )
    {
      if( font == m_curFont )
        return GG_OK;

      if( font != null )
      {
        bool bFound = false;
        for( vector<GG_Font_impl*>::iterator f = m_fonts.begin();
             f != m_fonts.end(); f++ )
        {
          if( *f == font )
          {
            bFound = true;
            break;
          }
        }
        if( !bFound )
          return GG_ERR_BADPARAM;
      }

      if( m_curFont != null )
        m_curFont->unRef();

      if( (m_curFont = (GG_Font_impl*)font) != null )
        m_curFont->addRef();

      return GG_OK;
    }


    GG_Rval setFontSize( float s )
    {
      if( s <= 0 )
        return GG_ERR_BADPARAM;

      m_fontStyle.size = s;
      return GG_OK;
    }

    float getFontSize(void)
    {
      return m_fontStyle.size;
    }

    GG_Rval setFontSpacing( float s )
    {
      m_fontStyle.spacing = s;
      return GG_OK;
    }

    float getFontSpacing(void)
    {
      return m_fontStyle.spacing;
    }

    GG_Rval setFontStretch( float s )
    {
      m_fontStyle.stretch = s;
      return GG_OK;
    }

    float getFontStretch(void)
    {
      return m_fontStyle.stretch;
    }

    GG_Rval setFontLeading( float l )
    {
      m_fontLeading = l;
      return GG_OK;
    }

    float getFontLeading(void)
    {
      return m_fontLeading;
    }

    float getTextWidth( char *str )
    {
      if( m_curFont == null )
        return 0;

      return m_curFont->getTextWidth( str, m_fontStyle );
    }

    GG_Rval drawText( char *str, float x, float y, uint align, GG_FontFX *fx )
    {
      if( m_curFont == null )
        return GG_ERR_NOFONT;

      if( str == null )
        return GG_ERR_BADPARAM;

      int     w, h;
      float   xPos, width, xs, ys;
      GG_Rval rval = GG_OK;

      switch( align )
      {
        case GG_TEXTALIGN_LEFT:
          xPos = x;
          break;
        case GG_TEXTALIGN_CENTER:
          width = m_curFont->getTextWidth( str, m_fontStyle );
          xPos = x - width * 0.5f;
          break;
        case GG_TEXTALIGN_RIGHT:
          width = m_curFont->getTextWidth( str, m_fontStyle );
          xPos = x - width;
          break;
        default:
          return GG_ERR_BADPARAM;
      }

      glPushMatrix();

      w = m_rcScreen.x2 - m_rcScreen.x1;    //  Figure out the aspect ratio
      h = m_rcScreen.y2 - m_rcScreen.y1;    //  then scale appropriately
      if( w > h )
      {
        xs = (float)h / w;
        ys = 1.0f;
      }
      else
      {
        xs = 1.0f;
        ys = (float)w / h;
      }

      glScalef( xs, ys, 1.0f );

      rval = m_curFont->drawText(str, xPos, y, 0, m_fontStyle, fx, null);

      glPopMatrix();

      return rval;
    }


    GG_Rval drawTextBox( char *str, GG_Rect2D *rc, uint align, GG_FontFX *fx )
    {
      if( m_curFont == null )
        return GG_ERR_NOFONT;

      float   w, h, widthRc,
              xs, ys,
              x, y;
      uint    nc;
      char    *p = str;
      GG_Rval rval = GG_OK;

      if( align != GG_TEXTALIGN_LEFT && align != GG_TEXTALIGN_RIGHT &&
          align != GG_TEXTALIGN_CENTER )
        return GG_ERR_BADPARAM;

      y = rc->y1 - m_fontStyle.size;
      widthRc = rc->x2 - rc->x1;
      if( widthRc <= 0 )
        return GG_ERR_BADPARAM;

      glPushMatrix();

      w = m_rcScreen.x2 - m_rcScreen.x1;    //  Figure out the aspect ratio
      h = m_rcScreen.y2 - m_rcScreen.y1;    //  then scale appropriately
      if( w > h )
      {
        xs = (float)h / w;
        ys = 1.0f;
      }
      else
      {
        xs = 1.0f;
        ys = (float)w / h;
      }

      glScalef( xs, ys, 1.0f );

      //  Draw the text, word-wrapping at each line
      while( *p != '\0' && y >= rc->y2 )
      {
        nc = m_curFont->fitWordCharCount( p, widthRc, m_fontStyle, &w );

        switch( align )
        {
          case GG_TEXTALIGN_LEFT:
            x = rc->x1;
            break;
          case GG_TEXTALIGN_CENTER:
            x = (rc->x1 + rc->x2 - w) * 0.5;
            break;
          case GG_TEXTALIGN_RIGHT:
            x = rc->x2 - w;
        }

        if( nc < 1 )
          goto exitNow;   //  No characters fit
        if( (rval = m_curFont->drawText(p, x, y, nc, m_fontStyle, fx, null)) < GG_OK )
          goto exitNow;   //  err
        p += nc;
        y -= m_fontLeading;
      }

    exitNow:
      glPopMatrix();
      return rval;
    }


    //  Other loadMovie parameter options
    //
    GG_Rval loadMovie( FILE *fp, GG_Movie **movie )
    {
      GG_File *f = GG_FileAliasFILE( fp );
      GG_Rval rval = loadMovie( f, movie );
      f->close();
      return rval;
    }


    GG_Rval loadMovie( char *filename, GG_Movie **movie )
    {
      char  path[256] = "",
            fullPath[256] = "",
            oldPath[256] = "",
            *p = null;
      GG_Rval rval;
      GG_File *f;

      f = GG_FileOpen( filename, GGFILE_OPENTXT_READ );
      if( f == null )
      {
        GG_LOG( "error: Failed to open movie file:" );
        GG_LOG( filename );
        return GG_ERR_FILEOPEN;
      }

      GG_File::getCurrentPath( oldPath, 255 );
      strcpy( fullPath, path );

      p = GG_String::skipDOSPath( filename );
      if( p > filename )
      {
        strncpy( path, filename, (uint32)(p - filename) );
        strcat( fullPath, path );
        GG_File::setCurrentPath( fullPath );

        GG_File::getCurrentPath( path, 255 );    //  Output for debug
      }

      GG_LOG( "GameGUI::loadMovie() -- Current path set to:" );
      GG_LOG( path );

      if( (rval = loadMovie(f, movie)) != GG_OK )
      {
        sprintf( gg_logBuf, "ERROR type %d encountered reading movie file: %s in line: %d", rval, filename, f->numLinesRead() );
        GG_LOG( gg_logBuf );
      }

      GG_File::setCurrentPath( oldPath );

      f->close();
      return rval;
    }


    GG_Rval writeMovie( char *filename, GG_Movie *movie )
    {
      GG_Rval rval;
      GG_File *f = GG_FileOpen( filename, GGFILE_OPENTXT_WRITENEW );
      if( f == null )
      {
        GG_LOG("Failed to open file for writing:");
        GG_LOG(filename);
        return GG_ERR_FILEOPEN;
      }

      rval = ((GG_Movie_impl*)movie)->write(f);
      f->close();
      return rval;
    }


    GG_Rval createPlayer( GG_Movie *mov, GG_Player **player )
    {
      GG_Player_impl *newPlayer = new GG_Player_impl((GG_Movie_impl*)mov, m_rcScreen);
//      m_players.push_back(newPlayer);
//      newPlayer->setOwnerList(&m_players);
      newPlayer->addRef();
      *player = newPlayer;
      return GG_OK;
    }
};


///////////////////////////////////////////////////////
//
//  "Factory" function -- creates a GameGUI
//  insance, returns ptr in gg (or fails, returning
//  error.
//

//
//  Create an OpenGL-based GameGUI object
//
GG_Rval GameGUICreate( GG_Rect *rc, GG_Callbacks *cb, GG_Flags flags, GameGUI **gg )
{
  GameGUI *newgg = new GameGUI_impl( rc, cb, flags );
  if( (*gg = newgg) != null )
    return GG_OK;
  return GG_ERR_MEMORY;
}
