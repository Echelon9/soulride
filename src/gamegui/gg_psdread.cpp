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
//  File: psdread.cpp
//
//  File created on 8/19/1998
//  Written by Thatcher Ulrich <ulrich@world.std.com>
//  I'm putting this module in the public domain.  Do what you want with it.
//
//  Some code to read Photoshop 2.5 .PSD files.
//
//  Modified by Mike Linkovich
//
///////////////////////////////

#include "gg_psdread.h"


PSDR_Rval GG_PSDRead::ScanForResolution(float* HRes, float* VRes, FILE* fp, int ByteCount)
// Scans through the next ByteCount bytes of the file, looking for an
// image resource block encoding the image's resolution.  Returns the resolution(s),
// if found, in the pointed-to floats.  Units are in pixels/meter.
{
  while (ByteCount) {
    // Read the image resource block header.
    uint32  Header = Read32(fp);
    if (Header != 0x3842494D /* "8BIM" */) {
      // Format problem.
//      Error e; e << "GG_PSDRead: image resource block has unknown signature.";
//      throw e;
      return PSDR_ERR_FORMAT;
    }
    int ID = Read16(fp);

    // Skip the name.
    int NameLength = fgetc(fp) | 1; // NameLength must be odd, so that total including size byte is even.
    fseek(fp, NameLength, SEEK_CUR);

    // Get the size of the data block.
    int DataSize = Read32(fp);
    if (DataSize & 1) {
      DataSize += 1;                // Block size must be even.
    }

    // Account for header size.
    ByteCount -= 11 + NameLength;

    // If this block is a ResolutionInfo structure, then get the resolution.
    if (ID == 0x03ED) {
      // Read ResolutionInfo.
      int HResFixed = Read32(fp);
      int junk = Read16(fp);        // display units of hres.
      junk = Read16(fp);            // display units of width.
      int VResFixed = Read32(fp);
      junk = Read16(fp);            // display units of vres.
      junk = Read16(fp);            // display units of height.

      ByteCount -= DataSize;
      DataSize -= 16;
      // Skip any extra bytes at the end of this block...
      if (DataSize > 0) {
        fseek(fp, DataSize, SEEK_CUR);
      }

      // Need to convert resolution figures from fixed point, pixels/inch to floating point,
      // pixels/meter.
      static const float  InchesPerMeter = 39.4f;
      *HRes = HResFixed * (InchesPerMeter / 65536.0f);
      *VRes = VResFixed * (InchesPerMeter / 65536.0f);

    } else {
      // Skip the rest of this block.
      fseek(fp, DataSize, SEEK_CUR);
      ByteCount -= DataSize;
    }
  }

  return PSDR_OK;
}


GG_PSDRead::bitmap16* GG_PSDRead::ReadImageData16(const char* filename, float* WidthPtr, float* HeightPtr)
// Reads the image from the specified filename, which must be in Photoshop .PSD format,
// and creates a new 16-bpp image containing the image data.  Creates a new bitmap to
// contain the image, and returns a pointer to the new bitmap.
// !!! The caller is responsible for freeing the bitmap.
// Returns NULL if the file can't be read or the image can't be created.
// If the given Width and/or Height parameters are not NULL, then stores the
// width and height of the image in the referenced locations, as encoded in the .PSD
// file but converted so that 1 cm in Photoshop == 1 meter here.
{
//  *data = NULL;

  // Open the data file for input.
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    return NULL;
  }

  // Check file type signature.
  uint32  sig = Read32(fp);
  if (sig != 0x38425053 /* "8BPS" */) {
    // Not .PSD format.
    fclose(fp);
    return NULL;
  }

  // Check file type version.
  if (Read16(fp) != 1) {
    fclose(fp);
    return NULL;
  }

  // 6 reserved bytes.
  Read32(fp);
  Read16(fp);

  // Read the number of channels (R, G, B, A, etc).
  int ChannelCount = Read16(fp);
  if (ChannelCount < 0 || ChannelCount > 16) {
    fclose(fp);
    return NULL;
  }

  // Read the rows and columns of the image.
  int height = Read32(fp);
  int width = Read32(fp);

  // Make sure the depth is 8 bits.
  if (Read16(fp) != 8) {
    fclose(fp);
    return NULL;
  }

  // Make sure the color mode is RGB.
  // Valid options are:
  //   0: Bitmap
  //   1: Grayscale
  //   2: Indexed color
  //   3: RGB color
  //   4: CMYK color
  //   7: Multichannel
  //   8: Duotone
  //   9: Lab color
  if (Read16(fp) != 3) {
    fclose(fp);
    return NULL;
  }

  // Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
  int ModeDataCount = Read32(fp);
  if (ModeDataCount) {
    fseek(fp, ModeDataCount, SEEK_CUR);
  }

  // Process the image resources.  (resolution, pen tool paths, etc)
  int ResourceDataCount = Read32(fp);
  if (ResourceDataCount) {
    // Read the image resources, looking for the info in the resolution block.
    float HRes = 1.0;
    float VRes = 1.0;
    if( ScanForResolution(&HRes, &VRes, fp, ResourceDataCount) != PSDR_OK )
      return null;

    // Set returns values for bitmap height/width.
    if (WidthPtr) {
      *WidthPtr = width / HRes * 100.0f; // Compute size in m, then multiply by 100 since we want 1 Photoshop cm == 1 Soul Ride meter.
    }
    if (HeightPtr) {
      *HeightPtr = height / VRes * 100.0f;
    }

//    fseek(fp, ResourceDataCount, SEEK_CUR);
  }

  // Skip the reserved data.
  int ReservedDataCount = Read32(fp);
  if (ReservedDataCount) {
    fseek(fp, ReservedDataCount, SEEK_CUR);
  }

  // Find out if the data is compressed.
  // Known values:
  //   0: no compression
  //   1: RLE compressed
  int Compression = Read16(fp);
  if (Compression > 1) {
    // Unknown compression type.
    fclose(fp);
    return NULL;
  }

  // Some formatting information about the channels.
  const struct ChannelInfo {
    int Shift, Mask, Default;
  } Channel[4] = {
    {  1, 0x7C00, 0 },  // Red.
    {  6, 0x03E0, 0 },  // Green.
    { 11, 0x001F, 0 },  // Blue.
    {  0, 0x8000, 255 } // Alpha
  };

  // Create the destination bitmap.
  bitmap16* b = new bitmap16(width, height);
  int PixelCount = height * width;
//  *data = new uint16[PixelCount];

  unsigned char pixel[2];

  // Initialize the data to zero.
  uint16* p = b->GetData();
  for (int i = 0; i < PixelCount; i++) {
    *p++ = 0;
  }

  // Finally, the image data.
  if (Compression) {
    // The RLE-compressed data is preceeded by a 2-byte data count for each row in the data,
    // which we're going to just skip.
    fseek(fp, height * ChannelCount * 2, SEEK_CUR);

    // Read the RLE data by channel.
    for (int channel = 0; channel < 4; channel++) {
      const ChannelInfo&  c = Channel[channel];

      uint16* p = b->GetData();
      if (channel >= ChannelCount) {
        // Fill this channel with default data.
        *(uint16 *)pixel = (c.Default << 8 >> c.Shift) & c.Mask;
	*(uint16 *)pixel = pixel[0] | pixel[1] << 8;
        for (int i = 0; i < PixelCount; i++) {
          *p++ |= *(uint16 *)pixel;
        }
      } else {
        // Read the RLE data.
        int count = 0;
        while (count < PixelCount) {
          int len = fgetc(fp);
          if (len == 128) {
            // No-op.
          } else if (len < 128) {
            // Copy next len+1 bytes literally.
            len++;
            count += len;
            while (len) {
	      *(uint16 *)&pixel = ((fgetc(fp) << 8 >> c.Shift) & c.Mask);
              *p++ |= pixel[0] | pixel[1] << 8;
              len--;
            }
          } else if (len > 128) {
            // Next -len+1 bytes in the dest are replicated from next source byte.
            // (Interpret len as a negative 8-bit int.)
            len ^= 0x0FF;
            len += 2;
            *(uint16 *)&pixel = (fgetc(fp) << 8 >> c.Shift) & c.Mask;
	    *(uint16 *)&pixel = pixel[0] | pixel[1] << 8;
            count += len;
            while (len) {
              *p++ |= *(uint16 *)pixel;
              len--;
            }
          }
        }
      }
    }

  } else {
    // We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
    // where each channel consists of an 8-bit value for each pixel in the image.

    // Read the data by channel.
    for (int channel = 0; channel < 4; channel++) {
      const ChannelInfo&  c = Channel[channel];

      uint16* p = b->GetData();
      if (channel > ChannelCount) {
        // Fill this channel with default data.
        uint16  def = (c.Default << 8 >> c.Shift) & c.Mask;
        for (int i = 0; i < PixelCount; i++) {
          *p++ |= def;
        }
      } else {
        // Read the data.
        int count = 0;
        for (int i = 0; i < PixelCount; i++) {
          *p++ |= (fgetc(fp) << 8 >> c.Shift) & c.Mask;
        }
      }
    }
  }

  fclose(fp);

  return b;
}



GG_PSDRead::bitmap32* GG_PSDRead::ReadImageData32(const char* filename)
// Reads the image from the specified filename, which must be in Photoshop .PSD format,
// and creates a new 32-bpp image containing the image data.  Creates a new bitmap to
// contain the image, and returns a pointer to the new bitmap.
// !!! The caller is responsible for freeing the bitmap.
// Returns NULL if the file can't be read or the image can't be created.
{
  // Open the data file for input.
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) {
    return NULL;
  }

  // Check file type signature.
  uint32  sig = Read32(fp);
  if (sig != 0x38425053 /* "8BPS" */) {
    // Not .PSD format.
    fclose(fp);
    return NULL;
  }

  // Check file type version.
  if (Read16(fp) != 1) {
    fclose(fp);
    return NULL;
  }

  // 6 reserved bytes.
  Read32(fp);
  Read16(fp);

  // Read the number of channels (R, G, B, A, etc).
  int ChannelCount = Read16(fp);
  if (ChannelCount < 0 || ChannelCount > 16) {
    fclose(fp);
    return NULL;
  }

  // Read the rows and columns of the image.
  int height = Read32(fp);
  int width = Read32(fp);

  // Make sure the depth is 8 bits.
  if (Read16(fp) != 8) {
    fclose(fp);
    return NULL;
  }

  // Make sure the color mode is RGB.
  // Valid options are:
  //   0: Bitmap
  //   1: Grayscale
  //   2: Indexed color
  //   3: RGB color
  //   4: CMYK color
  //   7: Multichannel
  //   8: Duotone
  //   9: Lab color
  if (Read16(fp) != 3) {
    fclose(fp);
    return NULL;
  }

  // Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
  int ModeDataCount = Read32(fp);
  if (ModeDataCount) {
    fseek(fp, ModeDataCount, SEEK_CUR);
  }

  // Skip the image resources.  (resolution, pen tool paths, etc)
  int ResourceDataCount = Read32(fp);
  if (ResourceDataCount) {
    fseek(fp, ResourceDataCount, SEEK_CUR);
  }

  // Skip the reserved data.
  int ReservedDataCount = Read32(fp);
  if (ReservedDataCount) {
    fseek(fp, ReservedDataCount, SEEK_CUR);
  }

  // Find out if the data is compressed.
  // Known values:
  //   0: no compression
  //   1: RLE compressed
  int Compression = Read16(fp);
  if (Compression > 1) {
    // Unknown compression type.
    fclose(fp);
    return NULL;
  }

  // Some formatting information about the channels.
  struct ChannelInfo {
    int Shift, Mask, Default;
  } Channel[4];

  /* This is an endianness safe way to initialize the channel info */
  for (int i = 0; i < 4; i++) {
    int Mask, Shift;
    Channel[i].Mask = 0;
    ((unsigned char*)&Channel[i].Mask)[i] = 0xFF;
    for (Mask = Channel[i].Mask, Shift = 0; !(Mask & 1); Shift++, Mask >>= 1);
    Channel[i].Shift = Shift;
    Channel[i].Default = 0;
  }
  Channel[3].Default = 255;

  // Create the destination bitmap.
  bitmap32* b = new bitmap32(width, height);
  int PixelCount = height * width;

  // Initialize the data to zero.
  uint32* p = b->GetData();
  {for (int i = 0; i < PixelCount; i++) {
    *p++ = 0;
  }}

  // Finally, the image data.
  if (Compression) {
    // The RLE-compressed data is preceeded by a 2-byte data count for each row in the data,
    // which we're going to just skip.
    fseek(fp, height * ChannelCount * 2, SEEK_CUR);

    // Read the RLE data by channel.
    for (int channel = 0; channel < 4; channel++) {
      const ChannelInfo&  c = Channel[channel];

      uint32* p = b->GetData();
      if (channel >= ChannelCount) {
        // Fill this channel with default data.
        uint32  def = (c.Default << c.Shift) & c.Mask;
        for (int i = 0; i < PixelCount; i++) {
          *p++ |= def;
        }
      } else {
        // Read the RLE data.
        int count = 0;
        while (count < PixelCount) {
          int len = fgetc(fp);
          if (len == 128) {
            // No-op.
          } else if (len < 128) {
            // Copy next len+1 bytes literally.
            len++;
            count += len;
            while (len) {
              *p++ |= ((fgetc(fp) << c.Shift) & c.Mask);
              len--;
            }
          } else if (len > 128) {
            // Next -len+1 bytes in the dest are replicated from next source byte.
            // (Interpret len as a negative 8-bit int.)
            len ^= 0x0FF;
            len += 2;
            uint32  val = (fgetc(fp) << c.Shift) & c.Mask;
            count += len;
            while (len) {
              *p++ |= val;
              len--;
            }
          }
        }
      }
    }

  } else {
    // We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
    // where each channel consists of an 8-bit value for each pixel in the image.

    // Read the data by channel.
    for (int channel = 0; channel < 4; channel++) {
      const ChannelInfo&  c = Channel[channel];

      uint32* p = b->GetData();
      if (channel > ChannelCount) {
        // Fill this channel with default data.
        uint32  def = (c.Default << c.Shift) & c.Mask;
        for (int i = 0; i < PixelCount; i++) {
          *p++ |= def;
        }
      } else {
        // Read the data.
        int count = 0;
        for (int i = 0; i < PixelCount; i++) {
          *p++ |= (fgetc(fp) << c.Shift) & c.Mask;
        }
      }
    }
  }

  fclose(fp);

  return b;
}
