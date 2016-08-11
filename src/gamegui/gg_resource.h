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
//  file: gg_resource.h
//
//  (c) 2000 by Mike Linkovich
//
//  start date: May 5 2000
//
///////////////////////////////

#ifndef _GG_RESOURCE_INCLUDED
#define _GG_RESOURCE_INCLUDED

#include "gamegui.h"

#include <vector>
#include <algorithm>
using namespace std;


#define GG_RES_NONE         0
#define GG_RES_TEXTUREGL    1
#define GG_RES_SOUND        2
#define GG_NUM_RES_TYPES    3


///////////////////////////////////////////////////////
//
//  Resource struct
//  (currently used to hold texture IDs)
//
///////////////////////////////

class GG_Resource
{
  protected:
    vector<GG_Resource*>  *m_ownerList;     //  list to remove from on refcount = 0
    int                   m_refCount;       //  reference count
    uint32                m_type;           //  Type of resource
    char                  m_fileName[256];  //  Filename
    uint32                m_resource;       //  Resource supplied by app (maybe ptr)

  public:
    GG_Resource()
    {
      m_ownerList = null;
      m_refCount = 0;
      m_type = GG_RES_NONE;
      m_fileName[0] = '\0';
      m_resource = 0;
    }

    virtual ~GG_Resource()  { }

    inline GG_Rval setOwnerList( vector<GG_Resource*> *ownerList )
    {
      m_ownerList = ownerList;
      return GG_OK;
    }

    inline uint32 type(void)
    {
      return m_type;
    }

    inline uint32 getResource(void)
    {
      return m_resource;
    }

    int addRef(void);
    int unRef(void);
    GG_Rval getFileName( char *fileName );
    GG_Rval setFileName( char *fileName );
    bool cmpFileName( char *fileName );
};


#ifdef LINUX
#else // not LINUX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // not LINUX

#ifndef MACOSX
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif


class GG_Res_GLTexture : public GG_Resource
{
  public:
    GG_Res_GLTexture()
    {
      m_type = GG_RES_TEXTUREGL;
      m_resource = 0;
    }
    GG_Res_GLTexture( GLuint texId, char *fileName, vector<GG_Resource*> *ownerList );
    virtual ~GG_Res_GLTexture();
};


#endif  //  _GG_RESOURCE_INCLUDED
