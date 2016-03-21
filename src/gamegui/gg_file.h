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
/////////////////////////////////////////////////
//
//  File gg_file.h
//
//  GameGUI File I/O library
//  - Wraps some internal Windows calls, as
//    well as calls to stdio functions
//  - provides support for "files" in RAM
//    as well as FILE ptrs
//
//  Other components generally use GG_File
//  in their internal loading code, so they
//  don't need to be re-written for ram/disk
//  reads.
//
//  Start date: Feb 9 2000
//
//  Author: Mike Linkovich
//
/////////////////////////////////////////////////


#ifndef _GG_FILE_INCLUDED
#define _GG_FILE_INCLUDED

#include "gg_types.h"
#include <stdio.h>


//  Return value type
//
typedef int32 GGF_Rval;

//  Error return codes
//
#define GGFILE_OK                   0
#define GGFILEERR_GENERIC          -1
#define GGFILEERR_BADPARAM         -2
#define GGFILEERR_FILENOTFOUND     -3
#define GGFILEERR_CONTEXT          -4
#define GGFILEERR_ALREADYOPEN      -5
#define GGFILEERR_ALREADYCLOSED    -6
#define GGFILEERR_STRINGTOOLONG    -7

//  EOF
//
#define GGFILE_EOF                 -1


//  Max file+pathname string length for anything.
//  Change this and re-compile to use longer strings.
//
#define GGFILE_MAXNAMELEN         256


//  File types
//
#define GGFILE_TYPE_UNKNOWN         0
#define GGFILE_TYPE_FILE            1
#define GGFILE_TYPE_DIRECTORY       2

//  Open modes
//
#define GGFILE_OPENBIN_READ         1
#define GGFILE_OPENBIN_WRITENEW     2
#define GGFILE_OPENBIN_WRITEAPPEND  3
#define GGFILE_OPENTXT_READ         4
#define GGFILE_OPENTXT_WRITENEW     5
#define GGFILE_OPENTXT_WRITEAPPEND  6

//  Seek modes
//
#define GGFILE_SEEK_ABS             1
#define GGFILE_SEEK_REL             2
#define GGFILE_SEEK_ABSEND          3


//
//  GG_File class interface
//
//
class GG_File
{
  public:
    GG_File()  { }
        //  Use a GG_File--- function to create an instance

    virtual ~GG_File()  { }
        //  closes any open file, cleans up any allocs

    virtual int32 getChar(void) = 0;
        //  gets next char in stream
        //  Returns EOF at EOF

    virtual int32 read( void *buf, uint32 size ) = 0;
    virtual int32 read( void *buf, uint32 size, uint32 num ) = 0;
    virtual int32 write( void *buf, uint32 size ) = 0;
    virtual int32 write( void *buf, uint32 size, uint32 num ) = 0;
    virtual int32 writeText( char *text ) = 0;
        //  No string formatting provided, use sprintf or
        //  whatever to format strings beforehand
        //  (% signs are safe to use though)
        //  Writes until null terminator reached

    virtual GGF_Rval read32( uint32 *val ) = 0;
        //  Reads 32 bits of the file
        //  byte-order is according to the file's current setting
        //  returns GGFILE_EOF on fail, GGFILE_OK on success.

    virtual GGF_Rval read16( uint32 *v ) = 0;
        //  Reads 16 bits of the file, returns in the low-word of the uint32
        //  Byte-order is according to the file's current setting
        //  returns GGFILE_EOF on fail, GGFILE_OK on success.

    virtual int32 getPos(void) = 0;
    virtual int32 seek( int32 bytes, int32 seekMode ) = 0;

    virtual bool readLine( char *buf, int32 maxChars ) = 0;
        //  Returns true if line read successfully

    virtual int32 numLinesRead(void) = 0;
        //  Returns # of times readLine() was successfully called on this file.
        //  May or may not have any relation to read/write position

    virtual void close(void) = 0;
        //  NOTE: Calls delete, so do not reference after closing!


    //
    //  Find file info struct
    //
    struct FileInfo
    {
      char    pathStr[GGFILE_MAXNAMELEN];   //  Note size
      uint32  flags;                        //  Type of file (_FILE, _DIRECTORY, etc.)
      uint32  reserved;                     //  Don't use!
    };

    //
    //  GG_FileFind functions
    //
    //  - GG_FileFindFirst Finds the first file matching given
    //    search string, puts path of that file into fileInfo.pathStr
    //    and sets appropriate flags.
    //  - Returns: GGFILE_OK successful if a file is found.
    //  - Begins enumeration for subsequent calls to
    //    GG_FileFindNext()
    //  - Use GG_FileFindClose() to end search
    //
    static GGF_Rval findFirstFile( char *searchPathStr, FileInfo *fileInfo );
    static GGF_Rval findNextFile( FileInfo *fileInfo );
    static GGF_Rval findClose( FileInfo *fileInfo );

    //
    //  setCurrentPath
    //
    //  - Sets the current path.  Calls to GG_File.. will
    //    prefix any filenames with this path.
    //
    static GGF_Rval setCurrentPath( char *pathName );                // null == no path, GGFILE_MAXNAMELEN==maximum string length
    static GGF_Rval getCurrentPath( char *pathName, int maxChars );  // up to GGFILE_MAXNAMELEN chars may be needed
};


//
//  "Factory" functions.  These create a GG_File instance,
//  based on the function called and the type of media storing the data
//
//  Check return values, since open attemts may obviously fail
//
GG_File *GG_FileOpen( char *fileName, uint32 mode );
    //  Opens a file using FILE internally

GG_File *GG_FileCreateRAM( char *fileName, uint32 size );
    //  Opens a file using FILE internally, copies the whole
    //  thing into a block of ram, which can be used as a file

GG_File *GG_FileAliasFILE( FILE *fp );
    //  Uses a currently open FILE.  This FILE may then
    //  be manipulated as a GG_File

GG_File *GG_FileAliasRAM( char *data, uint32 size );
    //  You can do I/O on an existing block of ram via
    //  a GG_File interface


#endif  // _GG_FILE_INCLUDED
