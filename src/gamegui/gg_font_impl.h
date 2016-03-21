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
//  File: gg_font_impl.h
//
//  Start date: Sep 2 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#ifndef _GG_FONT_IMPL_INCLUDED
#define _GG_FONT_IMPL_INCLUDED


#include "gamegui.h"
#include "gg_file.h"
#include "gg_resource.h"

#include <vector>

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


#define GG_FONT_MAX_CHARDEFS  256


//  Rendering info for drawText
//
struct GG_Font_Style
{
  float size,     //  Size of font
        spacing,  //  Spacing between letters
        stretch;  //  Horizontal scale factor
};


struct GG_Font_CharDef
{
  bool                bDefined; //  Character defined flag
  GLuint              texID;    //  OpenGL texture ID
  float               base,     //  Basline position on source bitmap
                      x,        //  X offset
                      width;    //  Character width
  vector<GG_Vector2D> verts;    //  Vertices (texture coordinates)

  GG_Font_CharDef()
  {
    bDefined = false;
    texID = 0;
    base = 0;
    x = 0;
    width = 0;
  }
};


class GG_Font_impl : public GG_Font
{
  protected:
    int                         m_refCount;
    vector<GG_Font_impl*>       *m_ownerList;
    vector<GG_Resource*>        m_resources;    //  List of all texture resources
    float                       m_scale;        //  Scale so that base to top is 1.0
    float                       m_leading;      //  Vertical spacing between lines
    GG_Font_CharDef             m_charDefs[GG_FONT_MAX_CHARDEFS];  //  Array index corresponds to ACII


  public:
    GG_Font_impl( vector<GG_Font_impl*> *ownerList )
    {
      m_refCount = 0;
      m_ownerList = ownerList;
      m_scale = 1.0f;
      m_leading = 0;
    }
    virtual ~GG_Font_impl()
    {
      for( vector<GG_Resource*>::iterator r = m_resources.begin();
           r != m_resources.end(); r++ )
        if( *r != null )
          (*r)->unRef();
    }

    int addRef(void);
    int unRef(void);

    //  Implementation only
    //
    GG_Rval read( GG_File *f, vector<GG_Resource*> *res );
    GG_Rval write( GG_File *f );

    GG_Rval getCharDef( uint cId, GG_Font_CharDef *cdef );
    GG_Rval setCharDef( uint cId, const GG_Font_CharDef &cdef );

    float getCharWidth( uint c, const GG_Font_Style &style );
    uint fitCharCount( char *str, float maxWidth, const GG_Font_Style &style,
                       float *finalWidth );
    uint fitWordCharCount( char *str, float maxWidth, const GG_Font_Style &style,
                       float *finalWidth );
    float getTextWidth( char *str, const GG_Font_Style &style );
    GG_Rval drawText( char *str, float x, float y, uint maxChars,
                      const GG_Font_Style &style,
                      GG_FontFX *fx, float *xNew );

    //  For editor only
    //
    GG_Rval getTexResources( vector<GG_Resource*> **res );
          //  Allows direct access to the texture resource list.
          //  For editor use only! Will majorly screw up char defs
          //  if the vector is messed around with.
};


#endif  // _GG_FONT__IMPL_INCLUDED
