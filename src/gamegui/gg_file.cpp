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
/////////////////////////////////////////////////
//
//  File gg_file.cpp
//
//  GG_File I/O library
//
//  Author: Mike Linkovich
//
//  Start date: Feb 9 2000
//
/////////////////////////////////////////////////


#include "gg_file.h"

#ifdef LINUX
#include <string.h>
#else // not LINUX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // not LINUX


//
//  Globals
//

static char g_currentPath[512] = "";


/////////////////////////////////////////////////
//
//  Implementation classes
//

//
//  GG_FileC is basically a wrapper for FILE
//
class GG_FileC : public GG_File
{
  protected:
    FILE    *m_fp;
    bool    m_bOwnFILE;
    int32   m_numLinesRead;

  public:
    GG_FileC( FILE *fp, bool bOwnFILE )
      {
        m_fp = fp;
        m_bOwnFILE = bOwnFILE;              //  Do we own this FILE ptr?
        m_numLinesRead = 0;
      }
    ~GG_FileC()
      {
        if( m_bOwnFILE && m_fp != NULL )    //  Only if we own it
          fclose(m_fp);
      }

    int32 getChar(void)
      {
        int32 c = fgetc(m_fp);

        if( c == EOF )
          return GGFILE_EOF;
        else
          return c;
      }
    int32 read( void *buf, uint32 size )
      {
        return fread( buf, size, 1, m_fp );
      }
    int32 read( void *buf, uint32 size, uint32 num )
      {
        return fread( buf, size, num, m_fp );
      }
    int32 write( void *buf, uint32 size )
      {
        return fwrite( buf, size, 1, m_fp );
      }
    int32 write( void *buf, uint32 size, uint32 num )
      {
        return fwrite( buf, size, num, m_fp );
      }
    int32 writeText( const char *buf )
      {
        int32 len = strlen(buf);
        return fwrite( buf, len, 1, m_fp );
      }
    GGF_Rval read16( uint32 *v )
      {
        int hi, lo;
        if( (hi = fgetc(m_fp)) == EOF )
          return GGFILE_EOF;
        if( (lo = fgetc(m_fp)) == EOF )
          return GGFILE_EOF;
        *v = lo + (hi << 8);
        return GGFILE_OK;
      }
    GGF_Rval read32( uint32 *v )
      {
        uint32  b0, b1, b2, b3;
        if( (b3 = fgetc(m_fp)) == EOF )
          return GGFILE_EOF;
        if( (b2 = fgetc(m_fp)) == EOF )
          return GGFILE_EOF;
        if( (b1 = fgetc(m_fp)) == EOF )
          return GGFILE_EOF;
        if( (b0 = fgetc(m_fp)) == EOF )
          return GGFILE_EOF;
        *v = (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
        return GGFILE_OK;
      }
    int32 getPos(void)
      {
        return ftell(m_fp);
      }
    int32 seek( int32 d, int32 seekMode )
      {
        switch( seekMode )
        {
          case GGFILE_SEEK_ABS:
            return fseek( m_fp, d, SEEK_SET );

          case GGFILE_SEEK_REL:
            return fseek( m_fp, d, SEEK_CUR );

          case GGFILE_SEEK_ABSEND:
            return fseek( m_fp, d, SEEK_END );
        }
        //  got an illegal seekMode value
        return null;
      }
    bool readLine( char *buf, int32 maxChars )
      {
        if( fgets( buf, maxChars, m_fp) != NULL )
        {
          // Consume a '\r' if it follows the newline (in case we're reading
          // DOS/Windows text files on Unix).
          int c = fgetc(m_fp);
          if (c != '\r') {
            ungetc(c, m_fp);
          }

          m_numLinesRead++;
          return true;
        }
        return false;
      }
    int32 numLinesRead(void)
      {
        return m_numLinesRead;
      }
    void close(void)
      { delete this; }
};


class GG_FileRAM : public GG_File
{
  protected:
    char    *m_pData;
    int32   m_size;
    int32   m_pos;
    int32   m_numLinesRead;

  public:
    GG_FileRAM( char *pData, uint32 size)
      {
        m_pos = 0;
        m_size = size;
        m_pData = pData;
        m_numLinesRead = 0;
      }
    ~GG_FileRAM()
      {
        if( m_pData != null )
          delete[] m_pData;
      }

    int32 getChar(void)
      {
        int32 c;
        if( m_pos >= m_size )
          return GGFILE_EOF;
        c = (int32)m_pData[m_pos];
        m_pos++;
        return c;
      }

    int32 read( void *buf, uint32 size )
      {
        if( m_pos + (int) size < m_size )
        {
          memcpy( buf, &m_pData[m_pos], size );
          m_pos += size;

          return 1;
        }
        return 0;
      }
    int32 read( void *buf, uint32 size, uint32 num )
      {
        int32 i;
        char *p = (char *)buf;
        for( i = 0; i < (int) num; i++ )
        {
          if( read(p, size) < 1 )
            return i;
          p += size;
        }

        return i;
      }

    int32 write( void *buf, uint32 size )
      {
        if( m_pos + (int) size < m_size )
        {
          memcpy( &m_pData[m_pos], buf, size );
          m_pos += size;

          return 1;
        }
        return 0;
      }
    int32 write( void *buf, uint32 size, uint32 num )
      {
        int32 i;
        char  *p = (char *)buf;
        for( i = 0; i < (int) num; i++ )
        {
          if( write(p, size) < 1 )
            return i;
          p += size;
        }

        return i;
      }
    int32 writeText( const char *buf )
      {
        int32 size = strlen(buf);
        if( m_pos + size < m_size )
        {
          memcpy( &m_pData[m_pos], buf, size );
          m_pos += size;
          return 1;
        }
        return 0;
      }
    GGF_Rval read16( uint32 *v )
      {
        if( m_pos + 2 >= m_size )
          return GGFILE_EOF;

        int hi, lo;
        hi = m_pData[m_pos];
        lo = m_pData[m_pos+1];
        m_pos += 2;
        *v = lo + (hi << 8);
        return GGFILE_OK;
      }
    GGF_Rval read32( uint32 *v )
      {
        if( m_pos + 4 >= m_size )
          return GGFILE_EOF;

        uint32 b3 = m_pData[m_pos],
               b2 = m_pData[m_pos+1],
               b1 = m_pData[m_pos+2],
               b0 = m_pData[m_pos+3];
        m_pos += 4;

        *v = (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
        return GGFILE_OK;
      }
    int32 getPos(void)
      {
        return m_pos;
      }
    int32 seek( int32 d, int32 seekMode )
      {
        switch( seekMode )
        {
          case GGFILE_SEEK_ABS:
            if( d < 0 || d > m_size )
              return -1;
            m_pos = d;
            return 0;

          case GGFILE_SEEK_REL:
            if( (m_pos + d < 0) || (m_pos + d >= m_size) )
              return -1;
            m_pos += d;
            return 0;

          case GGFILE_SEEK_ABSEND:
            if( d <= -m_size || d > 0 )
              return -1;
            m_pos = m_size + d;
            return 0;
        }

        return -1;
      }
    bool readLine( char *buf, int32 maxChars )
      {
        if( m_pos >= m_size )
          return false;

        char  c;
        int32 n = 0;
        while( (m_pos < m_size) && n < maxChars )
        {
          c = m_pData[m_pos];

          if( c == '\n' )
          {
            if( maxChars - n < 2 )
              return false;

            buf[n] = '\n';
            buf[n+1] = '0';
            m_numLinesRead++;
            return true;
          }
          else if( c == '\0' )
          {
            buf[n] = '\0';
            m_numLinesRead++;
            return true;
          }

          buf[n] = c;
          n++;
          m_pos++;
        }

        return false;   //  too big
      }
    int32 numLinesRead(void)
      {
        return m_numLinesRead;
      }
    void close(void)
      { delete this; }
};


/////////////////////////////////////////////////
//
//  Public factory functions for GG_File
//
//

//  GG_FileOpen()
//
//  Opens a file from the disk by name & mode in flags
//
//
GG_File *GG_FileOpen( char *fileName, uint32 openMode )
{
  GG_File  *f = null;
  const char    *modeStr;
  FILE    *fp = NULL;
  char    name[GGFILE_MAXNAMELEN*2] = "";

  sprintf( name, "%s%s", g_currentPath, fileName );

  switch( openMode )
  {
    case GGFILE_OPENBIN_READ:
      modeStr = "rb";
      break;

    case GGFILE_OPENBIN_WRITENEW:
      modeStr = "wb";
      break;

    case GGFILE_OPENBIN_WRITEAPPEND:
      modeStr = "ab";
      break;

    case GGFILE_OPENTXT_READ:
      modeStr = "r";
      break;

    case GGFILE_OPENTXT_WRITENEW:
      modeStr = "w";
      break;

    case GGFILE_OPENTXT_WRITEAPPEND:
      modeStr = "a";
      break;

    default:
      return null;
  }

  fp = fopen( name, modeStr );
  if( fp == NULL )
    return null;

  f = (GG_File *)(new GG_FileC(fp, true));

  return f;
}


//  GG_FileCreateRAM
//
//  Create a file by loading the file supplied into ram
//
//
GG_File *GG_FileCreateRAM( char *fileName )
{
  GG_File  *f = null;
  char  *pData = null;
  char  name[GGFILE_MAXNAMELEN*2] = "";

  sprintf( name, "%s%s", g_currentPath, fileName );

  FILE  *fp = fopen( name, "rb" );
  int32 size, pos;

  if( fp == NULL )
    return null;

  pos = ftell(fp);
  fseek( fp, 0, SEEK_END );
  size = ftell(fp);
  fseek( fp, pos, SEEK_SET );

  pData = new char[size];
  if( fread( pData, size, 1, fp ) < 1 )
  {
//    GG_LOG("GG_FileCreateRAM() -- error reading source file");
    goto failExit;
  }

  fclose(fp);
  fp = NULL;

  f = (GG_File *)(new GG_FileRAM(pData, size));

  return f;

failExit:
  if( pData != null )
    delete[] pData;

  if( fp != NULL )
    fclose(fp);

  return null;
}


GG_File *GG_FileAliasFILE( FILE *fp )
{
  GG_File *f = null;

  f = (GG_File *)(new GG_FileC(fp, false) );

  return f;
}


/////////////////////////////////////////////////
//
//  Internally used functions
//
//

#ifndef LINUX

static GGF_Rval translateFindInfo( WIN32_FIND_DATA *findInfo,
                                   GG_File::FileInfo *fileInfo )
{
  strncpy( fileInfo->pathStr, findInfo->cFileName, GGFILE_MAXNAMELEN );
  switch( findInfo->dwFileAttributes )
  {
    case FILE_ATTRIBUTE_DIRECTORY:
      fileInfo->flags = GGFILE_TYPE_DIRECTORY;
      break;

    default:
      fileInfo->flags = GGFILE_TYPE_FILE;
  }

  return GGFILE_OK;
}

#endif // not LINUX


/////////////////////////////////////////////////
//
//  Static public functions
//
//
GGF_Rval
GG_File::setCurrentPath( const char *str )
{
  if( str == null )
  {
    g_currentPath[0] = '\0';
    return GGFILE_OK;
  }

  int len = strlen(str);

  if( len >= 255 )
    return GGFILE_MAXNAMELEN;

  strcpy( g_currentPath, str );

  if( len < 1 )
    return GGFILE_OK;

#ifdef LINUX
  if (g_currentPath[len-1] != '/') {
    g_currentPath[len] = '/';
    g_currentPath[len+1] = '\0';
  }
#else // not LINUX
  if( g_currentPath[len-1] != '\\' )
  {
    g_currentPath[len] = '\\';
    g_currentPath[len+1] = '\0';
  }
#endif // not LINUX

  return GGFILE_OK;
}


GGF_Rval GG_File::getCurrentPath( char *str, int maxChars )
{
  if( str == null )
    return GGFILEERR_BADPARAM;
  strncpy( str, g_currentPath, maxChars );
  return GGFILE_OK;
}


#ifndef LINUX

GGF_Rval GG_File::findFirstFile( char *searchStr, FileInfo *fileInfo )
{
  WIN32_FIND_DATA winFindInfo;
  HANDLE          winSearchHandle = INVALID_HANDLE_VALUE;

  winSearchHandle = FindFirstFile( searchStr, &winFindInfo );

  if( winSearchHandle == INVALID_HANDLE_VALUE )
    return GGFILEERR_FILENOTFOUND;

  fileInfo->reserved = (uint32)winSearchHandle;

  return translateFindInfo( &winFindInfo, fileInfo );
}


GGF_Rval GG_File::findNextFile( FileInfo *fileInfo )
{
  WIN32_FIND_DATA winFindInfo;

  if( !FindNextFile( (HANDLE)fileInfo->reserved, &winFindInfo) )
    return GGFILEERR_FILENOTFOUND;

  return translateFindInfo( &winFindInfo, fileInfo );
}


GGF_Rval GG_File::findClose( FileInfo *fileInfo )
{
  if( (HANDLE)fileInfo->reserved == INVALID_HANDLE_VALUE )
    return GGFILEERR_CONTEXT;

  if( !FindClose((HANDLE)fileInfo->reserved) )
    return GGFILEERR_GENERIC;

  return GGFILE_OK;
}

#endif // not LINUX
