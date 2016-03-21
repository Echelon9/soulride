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
//  File: psdread.h
//
//  File created on 8/19/1998
//  Written by Thatcher Ulrich <ulrich@world.std.com>
//  I'm putting this module in the public domain.  Do what you want with it.
//
//  Some code to read Photoshop 2.5 .PSD files.
//
//  Modified by Mike Linkovich
//  Mod started March 2 2000
//
///////////////////////////////


#include "gg_types.h"
#include <stdio.h>
//#include "error.hpp"


typedef int32 PSDR_Rval;

#define PSDR_OK            0
#define PSDR_ERR          -1
#define PSDR_ERR_FORMAT   -2


class GG_PSDRead {

  public:

  // Generic base class for bitmaps.

  class bitmap {
    public:
      virtual ~bitmap() {}
      int GetWidth() { return Width; }
      int GetHeight() { return Height; }

    protected:
      int Width, Height;
      bitmap(int w, int h) { Width = w; Height = h; };
  };


  // class bitmap16 -- for storing 16-bit ARGB textures.

  class bitmap16 : public bitmap {
    public:
      bitmap16(int w, int h) : bitmap(w, h) { Data = new uint16[Width * Height]; }
      ~bitmap16() { delete [] Data; }
      uint16* GetData() { return Data; }
    //  void  ProcessForColorKeyZero();
    private:
      uint16* Data;
  };


  // class bitmap32 -- for storing 32-bit ARGB textures.

  class bitmap32 : public bitmap {
    public:
      bitmap32(int w, int h) : bitmap(w, h) { Data = new uint32[Width * Height]; }
      ~bitmap32() { delete [] Data; }
      uint32* GetData() { return Data; }
    //  void  ProcessForColorKeyZero();
    private:
      uint32* Data;
  };



  static int	Read16(FILE* fp)
  // Reads a two-byte big-endian integer from the given file and returns its value.
  // Assumes unsigned.
  {
    int	hi = fgetc(fp);
    int	lo = fgetc(fp);
    return lo + (hi << 8);
  }


  static uint32	Read32(FILE* fp)
  // Reads a four-byte big-endian integer from the given file and returns its value.
  // Assumes unsigned.
  {
    uint32	b3 = fgetc(fp);
    uint32	b2 = fgetc(fp);
    uint32	b1 = fgetc(fp);
    uint32	b0 = fgetc(fp);
    return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
  }


  static PSDR_Rval ScanForResolution(float* HRes, float* VRes, FILE* fp, int ByteCount);
  // Scans through the next ByteCount bytes of the file, looking for an
  // image resource block encoding the image's resolution.  Returns the resolution(s),
  // if found, in the pointed-to floats.  Units are in pixels/meter.


  static bitmap16* ReadImageData16(const char* filename, float* WidthPtr, float* HeightPtr);
  // Reads the image from the specified filename, which must be in Photoshop .PSD format,
  // and creates a new 16-bpp image containing the image data.  Creates a new bitmap to
  // contain the image, and returns a pointer to the new bitmap.
  // !!! The caller is responsible for freeing the bitmap.
  // Returns NULL if the file can't be read or the image can't be created.
  // If the given Width and/or Height parameters are not NULL, then stores the
  // width and height of the image in the referenced locations, as encoded in the .PSD
  // file but converted so that 1 cm in Photoshop == 1 meter here.


  static bitmap32*	ReadImageData32(const char* filename);
  // Reads the image from the specified filename, which must be in Photoshop .PSD format,
  // and creates a new 32-bpp image containing the image data.  Creates a new bitmap to
  // contain the image, and returns a pointer to the new bitmap.
  // !!! The caller is responsible for freeing the bitmap.
  // Returns NULL if the file can't be read or the image can't be created.

}; // end class GG_PSDRead


//
// RLE as used by .PSD and .TIFF
//

//Loop until you get the number of unpacked bytes you are expecting:
//    Read the next source byte into n.
//    If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
//    Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
//    Else if n is 128, noop.
//Endloop
