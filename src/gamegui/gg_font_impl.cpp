/* -*- coding: utf-8;-*-
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
//  File: gg_font_impl.cpp
//
//  Start date: Sep 2 2000
//
//  Author: Mike Linkovich
//
///////////////////////////////////


#include "gg_font_impl.h"
#include "gg_string.h"
#include "gg_psdread.h"
#include "gg_log.h"

#include <algorithm>


static char g_fileID[] = "GAMEGUIFONT 1.0";
static char msgBuf[256] = "";


int GG_Font_impl::addRef(void)
{
  m_refCount++;
  return m_refCount;
}


int GG_Font_impl::unRef(void)
{
  m_refCount--;
  int r = m_refCount;
  if( r < 1 )
  {
    if( m_ownerList != null )
    {
      vector<GG_Font_impl*>::iterator i =
               find( m_ownerList->begin(), m_ownerList->end(), this );
      if( i != m_ownerList->end() )
        m_ownerList->erase(i);
    }
    delete this;
  }
  return r;
}


GG_Rval GG_Font_impl::read( GG_File *f, vector<GG_Resource*> *res )
{
  if( f == null )
    return GG_ERR_BADPARAM;

  char            lineBuf[256] = "",
                  wordBuf[128] = "",
                  *p = null;
  bool            bVerified = false;
  GLuint          curTexId = 0;
  uint            cid = 0;
  GG_Font_CharDef *cdef = null;
  GG_Vector2D     vtx;

  while( f->readLine(lineBuf, 250) )
  {
    if( GG_String::stripComment(lineBuf) )
      continue;

    if( !bVerified )
    {
      if( strcmp( lineBuf, g_fileID ) != 0 )
      {
        sprintf( msgBuf, "GG_Font_impl::read() -- bad file format, '%s' must be first non-comment line", g_fileID );
        GG_LOG( msgBuf );
        return GG_ERR_FILEFORMAT;
      }
      bVerified = true;
      continue;
    }

    if( (p = GG_String::getWord(lineBuf, wordBuf)) == null )
    {
      GG_LOG("GG_Font_impl::read() -- failed to get 1st word in line:");
      GG_LOG(lineBuf);
      return GG_ERR_FILEFORMAT;
    }

    GG_String::capitalize(wordBuf);

    if( strcmp( wordBuf, "LEADING" ) == 0 )
    {
      if( m_leading != 0 )
      {
        GG_LOG("GG_Font_impl::read() -- LEADING defined more than once");
        return GG_ERR_FILEFORMAT;
      }

      if( sscanf(p, "%f", &m_leading) < 1 )
      {
        GG_LOG("GG_Font_impl::read() -- no value given to LEADING");
        return GG_ERR_FILEFORMAT;
      }

      if( m_leading <= 0 )
      {
        GG_LOG("GG_Font_impl::read() -- illegal value read for LEADING. Must be > 0");
        m_leading = 0;
        return GG_ERR_FILEFORMAT;
      }

      continue;
    }

    if( strcmp( wordBuf, "BITMAP" ) == 0 )
    {
      GG_PSDRead::bitmap32  *bitmap = null;
      char    resFileName[256] = "";
      GLuint  texId = 0;
      vector<GG_Resource*>::iterator r;

      if( !GG_String::getQuote(lineBuf, wordBuf))
      {
        curTexId = 0;   //  No quoted filename means NULL texture
        continue;
      }

      GG_File::getCurrentPath( resFileName, 255 );
      strcat( resFileName, wordBuf );

      bool bFound = false;

      //  First search this font's list of resources for previously
      //  loaded copy..
      //
      for( r = m_resources.begin(); r != m_resources.end(); r++ )
      {
        if( (*r)->cmpFileName(resFileName) )
        {
          texId = (GLuint)((*r)->getResource());
          bFound = true;
          break;
        }
      }

      //  If it wasn't there, search supplied resource list..
      //
      if( !bFound && res != null )
      {
        for( r = res->begin(); r != res->end(); r++ )
        {
          if( (*r)->cmpFileName(resFileName) )
          {
            texId = (GLuint)((*r)->getResource());
            (*r)->addRef();   //  Inc. ref count; another actor using this tex
            m_resources.push_back(*r);  //  This actor must remember to un-ref this resource on destruct
            bFound = true;
            break;
          }
        }
      }

      if( !bFound )
      {
        GG_Res_GLTexture *rc = null;

        //  Not found in resource list, load it from a file
        //
        if( (bitmap = GG_PSDRead::ReadImageData32(resFileName)) == null )
          return GG_ERR_FILEOPEN;

        //  Create OpenGL texture from this bitmap
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);

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
        rc = new GG_Res_GLTexture( texId, resFileName, res );
        rc->addRef();   //  Start refcount at 1
        m_resources.push_back(rc);

        if( res != null )
          res->push_back(rc);

        delete bitmap;
        bitmap = null;
      }

      curTexId = texId;    //  This is now the current tex ID for char defs following

      //  Clean up
      //
      continue;
    }

    if( strlen(wordBuf) > 1 )
    {
      if( strcmp(wordBuf, "DEFAULT") == 0 )
      {
        cid = 0;
      }
      else if( strcmp(wordBuf, "SPACE") == 0 )
      {
        cid = (uint)' ';
      }
      else
      {
        GG_LOG("GG_Font_impl::read() -- unknown keyword encountered in line:");
        GG_LOG(lineBuf);
        return GG_ERR_FILEFORMAT;
      }
    }
    else
    {
      cid = (uint)lineBuf[0];
    }

    //  This is a single character,
    //  so we assume this character is being defined..
    //
    cdef = &m_charDefs[cid & 0x0FF];

    if( cdef->bDefined )
    {
      GG_LOG("GG_Font_impl::read() -- redefinition of character in line:");
      GG_LOG(lineBuf);
      return GG_ERR_FILEFORMAT;
    }

    if( sscanf( p, "%f %f %f", &cdef->base, &cdef->x, &cdef->width ) < 3 )
    {
      GG_LOG("GG_Font_impl::read() -- incomplete character def in line:");
      GG_LOG(lineBuf);
      return GG_ERR_FILEFORMAT;
    }

    cdef->texID = curTexId;

    //  Read vertex pairs describing texture coordinate vertices
    //  of the polygon used to outline the shape.  Skip ahead
    //  to the vertex defs..
    //
    p = GG_String::skipNextWord(p);
    p = GG_String::skipNextWord(p);
    p = GG_String::skipNextWord(p);

    while( p != null && *p != '\0' )
    {
      if( sscanf( p, "%f %f", &vtx.x, &vtx.y ) < 2 )
      {
        GG_LOG("GG_Font_impl::read() -- incomplete vertex pair in line:");
        GG_LOG(lineBuf);
        return GG_ERR_FILEFORMAT;
      }
      cdef->verts.push_back(vtx);
      p = GG_String::skipNextWord(p);
      p = GG_String::skipNextWord(p);
    }

    if( cdef->verts.size() > 0 && cdef->verts.size() < 3 )
    {
      GG_LOG("GG_Font_impl::read() -- Character def must have at least 3 verticies.  In line:");
      GG_LOG(lineBuf);
      return GG_ERR_FILEFORMAT;
    }

    cdef->bDefined = true;
  }

  //  Now that the font has been read in, we need to scan the char defs
  //  to determine the maximum base to top dist, and use that to find an
  //  appropriate scale value...
  //
  float hmax = 0;
  vector<GG_Vector2D>::iterator v;
  for( cid = 0, cdef = &m_charDefs[0]; cid < GG_FONT_MAX_CHARDEFS; cid++, cdef++ )
  {
    if( cdef->bDefined )
    {
      for( v = cdef->verts.begin(); v != cdef->verts.end(); v++ )
      {
        if( (1.0f - v->y) - (1.0f - cdef->base) > hmax )
          hmax = (1.0f - v->y) - (1.0f - cdef->base);
      }
    }
  }

  if( hmax > 0 )
    m_scale = 1.0f / hmax;

  return GG_OK;
}


GG_Rval GG_Font_impl::write( GG_File *f )
{
  if( f == null )
    return GG_ERR_BADPARAM;

  char  strCh[16] = "",
        bmName[256] = "";
  uint  c;
  vector<GG_Vector2D>::iterator v;
  GG_Font_CharDef *cdef;
  GLuint  prevTexId = 0;


  f->writeText( g_fileID );
  f->writeText( "\n\n" );
  sprintf( msgBuf, "LEADING %1.4f\n\n", m_leading );
  f->writeText( msgBuf );

  for( c = 0, cdef = &m_charDefs[0]; c < GG_FONT_MAX_CHARDEFS; c++, cdef++ )
  {
    if( !cdef->bDefined )
      continue;   //  Skip undefined characters

    //  Check for texture change, if so, insert a BITMAP command..
    //
    if( prevTexId != cdef->texID )
    {
      if( cdef->texID == 0 )
      {
        f->writeText( "BITMAP NULL\n" );
      }
      else
      {
        bool bFound = false;
        for( vector<GG_Resource*>::iterator r = m_resources.begin();
             r != m_resources.end(); r++ )
        {
          if( (GLuint)((*r)->getResource()) == cdef->texID )
          {
            (*r)->getFileName( bmName );
            sprintf( msgBuf, "BITMAP \"%s\"\n", GG_String::skipDOSPath(bmName) );
            f->writeText( msgBuf );
            prevTexId = cdef->texID;
            bFound = true;
            break;
          }
        }
        if( !bFound )
        {
          GG_LOG("GG_Font_impl::write() -- texture ID does not correspond to any texture resource");
          return GG_ERR;
        }
      }
    }

    if( c == 0 )
      strcpy( strCh, "DEFAULT" );
    else if( c == ' ' )
      strcpy( strCh, "SPACE" );
    else
    {
      strCh[0] = (char)c;
      strCh[1] = '\0';
    }

    sprintf( msgBuf, "%s %1.4f %1.4f %1.4f", strCh, cdef->base, cdef->x, cdef->width );
    f->writeText( msgBuf );

    for( v = cdef->verts.begin(); v != cdef->verts.end(); v++ )
    {
      sprintf( msgBuf, " %1.4f %1.4f", v->x, v->y );
      f->writeText( msgBuf );
    }

    f->writeText( "\n" );
  }

  return GG_OK;
}


GG_Rval GG_Font_impl::getCharDef( uint cId, GG_Font_CharDef *cdef )
{
  if( cId >= GG_FONT_MAX_CHARDEFS || cdef == null )
    return GG_ERR_BADPARAM;

  *cdef = m_charDefs[cId & 0x0FF];
  return GG_OK;
}


GG_Rval GG_Font_impl::setCharDef( uint cId, const GG_Font_CharDef &cdef )
{
  if( cId >= GG_FONT_MAX_CHARDEFS )
    return GG_ERR_BADPARAM;

  m_charDefs[cId & 0x0FF] = cdef;
  return GG_OK;
}


GG_Rval GG_Font_impl::getTexResources( vector<GG_Resource*> **res )
{
  *res = &m_resources;
  return GG_OK;
}


float GG_Font_impl::getCharWidth( uint c, const GG_Font_Style &style )
{
  if( c > GG_FONT_MAX_CHARDEFS )
    return 0;
  GG_Font_CharDef *cd = &m_charDefs[c & 0x0FF];

  return (cd->width * m_scale + style.spacing) * style.size * style.stretch;
}



// Struct to describe a character for rendering; has info
// for composing characters.
struct CharRenderingInfo
{
	int	m_unicodeChar;
	unsigned char	m_baseChar;
	GG_Font_CharDef*	m_composeCdef0;
	GG_Font_CharDef*	m_composeCdef1;
	float	m_composeHeightAdjust;
	float	m_composeXAdjust0;
	float	m_composeXAdjust1;
	bool	m_composeReverse0;
	bool	m_composeStroke0;
	float	m_composeStrokeXSize;

	CharRenderingInfo()
		:
		m_unicodeChar(0),
		m_baseChar(0),
		m_composeCdef0(null),
		m_composeCdef1(null),
		m_composeHeightAdjust(0),
		m_composeXAdjust0(0),
		m_composeXAdjust1(0),
		m_composeReverse0(false),
		m_composeStroke0(false),
		m_composeStrokeXSize(0.0f)
	{
	}
};


// Helper macros for composing chars.

#define UMLAUT(height)							\
	  ri->m_composeCdef0 = &charDefs['.'];		\
	  ri->m_composeCdef1 = ri->m_composeCdef0;	\
	  ri->m_composeXAdjust0 = -0.08f;			\
	  ri->m_composeXAdjust1 =	 0.22f;			\
	  ri->m_composeHeightAdjust = height;

#define RIGHT_ACCENT(height)					\
	  ri->m_composeCdef0 = &charDefs['`'];		\
	  ri->m_composeReverse0 = true;				\
	  ri->m_composeXAdjust0 = 0.07f;			\
	  ri->m_composeHeightAdjust = height;

#define BACK_ACCENT(height)					\
	  ri->m_composeCdef0 = &charDefs['`'];		\
	  ri->m_composeReverse0 = true;				\
	  ri->m_composeXAdjust0 = 0;				\
	  ri->m_composeHeightAdjust = height;

// #define SLASH(xadj)								\
// 	  ri->m_composeCdef0 = &charDefs['_'];		\
// 	  ri->m_composeXAdjust0 = xadj;				\
// 	  ri->m_composeHeightAdjust = 0.50f;		\
// 	  ri->m_composeRotate0 = 0.785f;

#define SLASH(xsize)	\
	  ri->m_composeCdef0 = &charDefs['_'];		\
	  ri->m_composeStroke0 = true;				\
	  ri->m_composeStrokeXSize = xsize;

// Little tail for some Polish chars
#define OGANEK									\
	  ri->m_composeCdef0 = &charDefs[','];		\
	  ri->m_composeXAdjust0 = 0.18f;			\
	  ri->m_composeHeightAdjust = 0;			\
	  ri->m_composeReverse0 = true;

#define SINGLE_DOT(height)						\
	  ri->m_composeCdef0 = &charDefs['.'];		\
	  ri->m_composeXAdjust0 = 0.07f;			\
	  ri->m_composeHeightAdjust = height;


static void	GetCharacterInfo(CharRenderingInfo* ri, unsigned char** pp, GG_Font_CharDef charDefs[GG_FONT_MAX_CHARDEFS])
// Decode a single, possibly UTF-8 encoded, character, and put the rendering
// info in *ri.
//
// p is the source text.
{
	unsigned char	ch = **pp;
	ri->m_unicodeChar = ch;

	// UTF-8 decoding.
	if (ch & 0x80)
	{
		if ((ch & 0xE0) == 0xC0)
		{
			(*pp)++;
			unsigned char	ch1 = **pp;
			if (ch1 == 0
				|| (ch1 & 0xC0) != 0x80)
			{
				// invalid utf-8, but maybe this is valid Latin-1.  Try using ch as-is.
				(*pp)--;
				ri->m_unicodeChar = ch;
			}
			else
			{
				ri->m_unicodeChar = ((ch & 0x1F) << 6) | (ch1 & 0x3F);
			}
		}
		else
		{
			// We don't handle higher unicode ranges.
			// Possibly this is valid Latin-1, so treat it as such.
			ri->m_unicodeChar = ch;
		}
	}

	if (ri->m_unicodeChar >= 0x0080)
	{
		// Non-ASCII characters.
		if (ri->m_unicodeChar == 0x0E4) // Latin Small Letter A With Diaeresis '�'
		{
			ch = 'a';
			UMLAUT(0.75f);
		}
		else if (ri->m_unicodeChar == 0x0C4) // Latin Capital Letter A With Diaeresis '�'
		{
			ch = 'A';
			UMLAUT(0.95f);
		}
		else if (ri->m_unicodeChar == 0x0F6)	// o with diaeresis '�'
		{
			ch = 'o';
			UMLAUT(0.75f);
		}
		else if (ri->m_unicodeChar == 0x0D6)	// O with diaeresis '�'
		{
			ch = 'O';
			UMLAUT(0.95f);
		}
		else if (ri->m_unicodeChar == 0x0FC)	// u with diaeresis '�'
		{
			ch = 'u';
			UMLAUT(0.75f);
		}
		else if (ri->m_unicodeChar == 0x0DC)	// U with diaeresis '�'
		{
			ch = 'U';
			UMLAUT(0.95f);
		}
		else if (ri->m_unicodeChar == 0x104)	// A with oganek Ą
		{
			ch = 'A';
			OGANEK;
		}
		else if (ri->m_unicodeChar == 0x105)	// a with oganek ą
		{
			ch = 'a';
			OGANEK;
		}
		else if (ri->m_unicodeChar == 0x106)	// C with acute Ć
		{
			ch = 'C';
			RIGHT_ACCENT(0.12f);
		}
		else if (ri->m_unicodeChar == 0x107)	// c with acute ć
		{
			ch = 'c';
			RIGHT_ACCENT(-0.05f);
		}
		else if (ri->m_unicodeChar == 0x118)	// E with oganek Ę
		{
			ch = 'E';	
			OGANEK;
		}
		else if (ri->m_unicodeChar == 0x119)	// e with oganek ę
		{
			ch = 'e';
			OGANEK;
		}
		else if (ri->m_unicodeChar == 0x141)	// L with stroke Ł
		{
			ch = 'L';
			SLASH(0.60f);
		}
		else if (ri->m_unicodeChar == 0x142)	// l with stroke ł
		{
			ch = 'l';
			SLASH(1.60f);
		}
		else if (ri->m_unicodeChar == 0x143)	// N with acute Ń
		{
			ch = 'N';
			RIGHT_ACCENT(0.12f);
		}
		else if (ri->m_unicodeChar == 0x144)	// n with acute ń
		{
			ch = 'n';
			RIGHT_ACCENT(-0.05f);
		}
		else if (ri->m_unicodeChar == 0x0D3)	// 'O' with acute Ó
		{
			ch = 'O';
			RIGHT_ACCENT(0.12f);
		}
		else if (ri->m_unicodeChar == 0x0F3)	// 'o' with acute ó
		{
			ch = 'o';
			RIGHT_ACCENT(-0.05f);
		}
		else if (ri->m_unicodeChar == 0x15A)	// S with acute Ś
		{
			ch = 'S';
			RIGHT_ACCENT(0.12f);
		}
		else if (ri->m_unicodeChar == 0x15B)	// s with acute ś
		{
			ch = 's';
			RIGHT_ACCENT(-0.05f);
		}
		else if (ri->m_unicodeChar == 0x179)	// Z with acute Ź
		{
			ch = 'Z';
			RIGHT_ACCENT(0.12f);
		}
		else if (ri->m_unicodeChar == 0x17A)	// z with acute ź
		{
			ch = 'z';
			RIGHT_ACCENT(-0.05f);
		}
		else if (ri->m_unicodeChar == 0x17B)	// Z with dot above Ż
		{
			ch = 'Z';
			SINGLE_DOT(0.95f);
		}
		else if (ri->m_unicodeChar == 0x17C)	// z with dot above ż
		{
			ch = 'z';
			SINGLE_DOT(0.75f);
		}
		else if (ri->m_unicodeChar == 0xC0)	// À
		{
			ch = 'A';
			BACK_ACCENT(0.25f);
		}
		else if (ri->m_unicodeChar == 0xE0)	// à
		{
			ch = 'a';
			BACK_ACCENT(0.00f);
		}
		else if (ri->m_unicodeChar == 0xC8)	// È
		{
			ch = 'E';
			BACK_ACCENT(0.25f);
		}
		else if (ri->m_unicodeChar == 0xE8)	// è
		{
			ch = 'e';
			BACK_ACCENT(0.00f);
		}
		else if (ri->m_unicodeChar == 0xCC)	// Ì
		{
			ch = 'I';
			BACK_ACCENT(0.25f);
		}
		else if (ri->m_unicodeChar == 0xEC)	// ì
		{
			ch = 'i';
			BACK_ACCENT(0.00f);
		}
		else if (ri->m_unicodeChar == 0xD2)	// Ò
		{
			ch = 'O';
			BACK_ACCENT(0.25f);
		}
		else if (ri->m_unicodeChar == 0xF2)	// ò
		{
			ch = 'o';
			BACK_ACCENT(0.00f);
		}
		else if (ri->m_unicodeChar == 0xD9)	// Ù
		{
			ch = 'U';
			BACK_ACCENT(0.25f);
		}
		else if (ri->m_unicodeChar == 0xF9)	// ù
		{
			ch = 'u';
			BACK_ACCENT(0.00f);
		}
		// etc.
	}

	ri->m_baseChar = ch;
}



uint GG_Font_impl::fitWordCharCount( char *str, float maxWidth,
                                     const GG_Font_Style &style,
                                     float *finalWidth )
{
  unsigned char  *p = (unsigned char*) str;
  float w, cw = 0, prevBreakWidth = 0;
  uint  n, prevBreakCount = 0;

  for( p = (unsigned char*) str, n = 0, w = 0; *p != '\0'; p++, w += cw )
  {
    if( *p == '\n' || *p == '\r' )  // break on a newline or c/r
    {
      if( finalWidth != null )
        *finalWidth = w;
      return (p - (unsigned char*) str) + 1;
    }

    CharRenderingInfo   ri;
    GetCharacterInfo(&ri, &p, m_charDefs);
    cw = getCharWidth(ri.m_baseChar, style);

    if( ri.m_baseChar == ' ' || ri.m_baseChar == '\t' || ri.m_baseChar == '-' )
    {
      prevBreakWidth = w;
      prevBreakCount = (p - (unsigned char*) str) + 1;
    }

    if( w + cw > maxWidth )
    {
      if( prevBreakCount < 1 )
      {
        prevBreakCount = (p - (unsigned char*) str);
        prevBreakWidth = w;
      }
      if( finalWidth != null )
        *finalWidth = prevBreakWidth;
      return prevBreakCount;
    }
  }

  if( finalWidth != null )
    *finalWidth = w;

  return (p - (unsigned char*) str);
}


uint GG_Font_impl::fitCharCount( char *str, float maxWidth,
                                 const GG_Font_Style &style,
                                 float *finalWidth )
{
  unsigned char  *p = (unsigned char*) str;
  float w = 0, cw = 0;

  for( p = (unsigned char*) str, w = 0; *p != '\0'; p++, w += cw )
  {
    CharRenderingInfo   ri;
    GetCharacterInfo(&ri, &p, m_charDefs);
    cw = getCharWidth(ri.m_baseChar, style);
    if( w + cw > maxWidth )
    {
      if( finalWidth != null )
        *finalWidth = w;
      return (p - (unsigned char*) str);
    }
  }

  if( finalWidth != null )
    *finalWidth = w;

  return (p - (unsigned char*) str);
}


float GG_Font_impl::getTextWidth( char *str, const GG_Font_Style &style )
{
  if( str == null )
    return 0;

  unsigned char            *p;
  GG_Font_CharDef *cdef;
  float           width = 0;

  for( p = (unsigned char*) str; *p != '\0'; p++ )
  {
    CharRenderingInfo   ri;
    GetCharacterInfo(&ri, &p, m_charDefs);

    cdef = &m_charDefs[ri.m_baseChar & 0x0FF];
    if( !cdef->bDefined )
      cdef = &m_charDefs[0];
    width += (cdef->width * m_scale + style.spacing) * style.size * style.stretch;
  }

  return width;
}


GG_Rval GG_Font_impl::drawText( char *str, float x, float y,
                                uint maxBytes,
                                const GG_Font_Style &style,
                                GG_FontFX *fx, float *newX )
{
	if( str == null || style.size <= 0 )
		return GG_ERR_BADPARAM;

	vector<GG_Vector2D>::iterator v, vEnd;
	GG_Font_CharDef *cdef = null;
	GG_ColorARGBf   clr = { 1, 1, 1, 1 };
	GLuint  prevTexId = 0;
	float   xPos;
	uint    nc;
	unsigned char    *p;
	GG_Rval rval = GG_OK;

	if( fx != null )
	{
		if( fx->flags && GG_FONTFX_COLORMOD )
			clr = fx->clrMod;
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f( clr.r, clr.g, clr.b, clr.a );

	xPos = x;
	for( p = (unsigned char*) str, nc = 0; *p != '\0'; p++)
	{
		if( maxBytes > 0 && (p - (unsigned char*) str) >= maxBytes )
			break;

		// Hacks for composed characters; i.e. umlauts and the like.
		CharRenderingInfo	ri;
		GetCharacterInfo(&ri, &p, m_charDefs);

		cdef = &m_charDefs[ri.m_baseChar & 0x0FF];
		if( !cdef->bDefined )
		{
			xPos += (m_charDefs[0].width * m_scale + style.spacing) * style.size * style.stretch;
			continue;
		}

		if( prevTexId != cdef->texID )
		{
			prevTexId = cdef->texID;
			glBindTexture(GL_TEXTURE_2D, prevTexId);
		}

		//	Send each character as a polygon..
		//
		glBegin( GL_POLYGON );
		for( v = cdef->verts.begin(), vEnd = cdef->verts.end(); v != vEnd; v++ )
		{
			glTexCoord2f( v->x, v->y );
			glVertex2f( xPos + (v->x - cdef->x) * m_scale * style.size * style.stretch,
						y + ((1.0f - v->y) - (1.0f - cdef->base)) * m_scale * style.size );
		}
		glEnd();

		// Compose extra characters here, if requested.
		if (ri.m_composeCdef0 != null
			&& ri.m_composeCdef0->bDefined)
		{
			if( prevTexId != ri.m_composeCdef0->texID )
			{
				prevTexId = ri.m_composeCdef0->texID;
				glBindTexture(GL_TEXTURE_2D, prevTexId);
			}

			if (ri.m_composeStroke0)
			{
				// Very special purpose (and ugly) -- draw a 45-degree quad in the middle of the glyph.
				// For the Polish 'L' characters w/ stroke.
				prevTexId = 0;
				glBindTexture(GL_TEXTURE_2D, 0);	// no texture

				float	kx = (cdef->width * m_scale + style.spacing) * style.size * style.stretch;
				float	ky = style.size;

				glBegin( GL_POLYGON );
				glVertex2f(xPos +                  -0.03f * kx, y + 0.30f * ky);
				glVertex2f(xPos + ri.m_composeStrokeXSize * kx, y + 0.60f * ky);
				glVertex2f(xPos + ri.m_composeStrokeXSize * kx, y + 0.68f * ky);
				glVertex2f(xPos +                  -0.03f * kx, y + 0.38f * ky);
				glEnd();
			}
			else if (ri.m_composeReverse0 == false)
			{
				// Normal rendering.
				glBegin( GL_POLYGON );
				for( v = ri.m_composeCdef0->verts.begin(), vEnd = ri.m_composeCdef0->verts.end(); v != vEnd; v++ )
				{
					glTexCoord2f( v->x, v->y );
					glVertex2f( xPos
								+ ((v->x - ri.m_composeCdef0->x) * m_scale
								 + ri.m_composeXAdjust0) * style.size * style.stretch,
								y
								+ (((1.0f - v->y)
									- (1.0f - ri.m_composeCdef0->base))
								   * m_scale + ri.m_composeHeightAdjust) * style.size );
				}
				glEnd();
			}
			else
			{
				// Reverse this character.
				v = ri.m_composeCdef0->verts.begin();
				float	minU = v->x;
				float	maxU = minU;
				for(v++, vEnd = ri.m_composeCdef0->verts.end(); v != vEnd; v++ )
				{
					if (v->x < minU) minU = v->x;
					if (v->x > maxU) maxU = v->x;
				}
				float	midU = (minU + maxU) * 0.5f;

				glBegin( GL_POLYGON );
				for( v = ri.m_composeCdef0->verts.begin(), vEnd = ri.m_composeCdef0->verts.end(); v != vEnd; v++ )
				{
					glTexCoord2f( midU * 2 - v->x, v->y );
					glVertex2f( xPos
								+ ((v->x - ri.m_composeCdef0->x) * m_scale
								   + ri.m_composeXAdjust0)
								* style.size * style.stretch,
								y
								+ (((1.0f - v->y)
									- (1.0f - ri.m_composeCdef0->base)) * m_scale
								   + ri.m_composeHeightAdjust) * style.size );
				}
				glEnd();
			}
		}
		if (ri.m_composeCdef1 != null
			&& ri.m_composeCdef1->bDefined)
		{
			if( prevTexId != ri.m_composeCdef1->texID )
			{
				prevTexId = ri.m_composeCdef1->texID;
				glBindTexture(GL_TEXTURE_2D, prevTexId);
			}

			glBegin( GL_POLYGON );
			for( v = ri.m_composeCdef1->verts.begin(), vEnd = ri.m_composeCdef1->verts.end(); v != vEnd; v++ )
			{
				glTexCoord2f( v->x, v->y );
				glVertex2f( xPos
							+ ((v->x - ri.m_composeCdef1->x) * m_scale
							   + ri.m_composeXAdjust1) * style.size * style.stretch,
							y +
							(((1.0f - v->y) - (1.0f - ri.m_composeCdef1->base)) * m_scale
							 + ri.m_composeHeightAdjust) * style.size );
			}
			glEnd();
		}

		xPos += (cdef->width * m_scale + style.spacing) * style.size * style.stretch;
	}

	if( newX != null )
		*newX = xPos;

// exitNow:
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	return rval;
}
