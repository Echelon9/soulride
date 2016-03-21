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
//  file: gg_resource.cpp
//
//  (c) 2000 by Mike Linkovich
//
//  start date: May 5 2000
//
///////////////////////////////


#include "gg_resource.h"
#include <algorithm>


int GG_Resource::addRef(void)
{
  m_refCount++;
  return m_refCount;
}


int GG_Resource::unRef(void)
{
  m_refCount--;
  int r = m_refCount;
  if( r == 0 )
  {
    if( m_ownerList != null )
    {
      vector<GG_Resource*>::iterator i =
               find( m_ownerList->begin(), m_ownerList->end(), this );
      if( i != m_ownerList->end() )
        m_ownerList->erase(i);
    }
    delete this;
  }
  return r;
}


GG_Rval GG_Resource::getFileName( char *fileName )
{
  strcpy( fileName, m_fileName );
  return GG_OK;
}


GG_Rval GG_Resource::setFileName( char *fileName )
{
  strncpy( m_fileName, fileName, 255 );
  return GG_OK;
}


bool GG_Resource::cmpFileName( char *fileName )
{
  if( strcmp( fileName, m_fileName ) == 0 )
    return true;
  return false;
}


GG_Res_GLTexture::GG_Res_GLTexture( GLuint texId, char *fileName,
                                    vector<GG_Resource*> *ownerList )
{
  m_type = GG_RES_TEXTUREGL;
  m_resource = texId;
  if( m_fileName != null )
    strncpy( m_fileName, fileName, 255 );
  m_ownerList = ownerList;
}


GG_Res_GLTexture::~GG_Res_GLTexture()
{
  if( m_resource != 0 )
  {
    glDeleteTextures(1, (GLuint*)&m_resource);      //  Delete this texture id..
    m_resource = null;
  }
}
