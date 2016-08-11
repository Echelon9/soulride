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
// psdread.cpp	-thatcher 8/19/1998 Copyright Slingshot

// Some code to read Photoshop 2.5 .PSD files.


#include <stdio.h>
#include "psdread.hpp"
#include "error.hpp"
#include "utility.hpp"


namespace PSDRead {
;


#ifdef NOT

int	Read16(FILE* fp)
// Reads a two-byte big-endian integer from the given file and returns its value.
// Assumes unsigned.
{
	int	hi = fgetc(fp);
	int	lo = fgetc(fp);
	return lo + (hi << 8);
}


uint32	Read32(FILE* fp)
// Reads a four-byte big-endian integer from the given file and returns its value.
// Assumes unsigned.
{
	uint32	b3 = fgetc(fp);
	uint32	b2 = fgetc(fp);
	uint32	b1 = fgetc(fp);
	uint32	b0 = fgetc(fp);
	return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
}


int	Write16(FILE* fp, unsigned short data)
// Writes the data as a two-byte big-endian integer to the given file.
// Returns count of bytes written.
{
	fputc(data >> 8, fp);
	fputc(data & 0x0FF, fp);
	return 2;
}


int	Write32BE(FILE* fp, unsigned long data)
// Writes the data as a four-byte big-endian integer to the given file.
// Returns count of bytes written.
{
	fputc((data >> 24) & 0x0FF, fp);
	fputc((data >> 16) & 0x0FF, fp);
	fputc((data >> 8) & 0x0FF, fp);
	fputc((data >> 0) & 0x0FF, fp);
	return 4;
}


#endif // NOT


void	ScanForResolution(float* HRes, float* VRes, FILE* fp, int ByteCount)
// Scans through the next ByteCount bytes of the file, looking for an
// image resource block encoding the image's resolution.  Returns the resolution(s),
// if found, in the pointed-to floats.  Units are in pixels/meter.
{
	while (ByteCount) {
		// Read the image resource block header.
		uint32	Header = Read32BE(fp);
		if (Header != 0x3842494D /* "8BIM" */) {
			// Format problem.
			Error e; e << "PSDRead: image resource block has unknown signature.";
			throw e;
		}
		int	ID = Read16BE(fp);

		// Skip the name.
		int	NameLength = fgetc(fp) | 1;	// NameLength must be odd, so that total including size byte is even.
		fseek(fp, NameLength, SEEK_CUR);

		// Get the size of the data block.
		int	DataSize = Read32BE(fp);
		if (DataSize & 1) {
			DataSize += 1;	// Block size must be even.
		}

		// Account for header size.
		ByteCount -= 11 + NameLength;

		// If this block is a ResolutionInfo structure, then get the resolution.
		if (ID == 0x03ED) {
			// Read ResolutionInfo.
			int	HResFixed = Read32BE(fp);
			int	junk = Read16BE(fp);	// display units of hres.
			junk = Read16BE(fp);	// display units of width.
			int	VResFixed = Read32BE(fp);
			junk = Read16BE(fp);	// display units of vres.
			junk = Read16BE(fp);	// display units of height.

			ByteCount -= DataSize;
			DataSize -= 16;
			// Skip any extra bytes at the end of this block...
			if (DataSize > 0) {
				fseek(fp, DataSize, SEEK_CUR);
			}

			// Need to convert resolution figures from fixed point, pixels/inch to floating point,
			// pixels/meter.
			static const float	InchesPerMeter = 39.4f;
			*HRes = HResFixed * (InchesPerMeter / 65536.0f);
			*VRes = VResFixed * (InchesPerMeter / 65536.0f);
			
		} else {
			// Skip the rest of this block.
			fseek(fp, DataSize, SEEK_CUR);
			ByteCount -= DataSize;
		}
	}
}


bitmap32*	ReadImageData32(const char* filename, float* WidthPtr, float* HeightPtr)
// Reads the image from the specified filename, which must be in Photoshop .PSD format,
// and creates a new 32-bpp image containing the image data.  Creates a new bitmap to
// contain the image, and returns a pointer to the new bitmap.
//
// The image data is in RGBA format, reading by byte with addresses increasing left to right,
// which is sort of a big-endian format.  It's what OpenGL wants.
//
// !!! The caller is responsible for freeing the bitmap.
// Returns NULL if the file can't be read or the image can't be created.
// If the given Width and/or Height parameters are not NULL, then stores the
// width and height of the image in the referenced locations, as encoded in the .PSD
// file but converted so that 1 cm in Photoshop == 1 meter here.
{
	// Open the data file for input.
	FILE*	fp = Utility::FileOpen(filename, "rb");
	if (fp == NULL) {
		return NULL;
	}

	// Check file type signature.
	uint32	sig = Read32BE(fp);
	if (sig != 0x38425053 /* "8BPS" */) {
		// Not .PSD format.
		fclose(fp);
		return NULL;
	}

	// Check file type version.
	if (Read16BE(fp) != 1) {
		fclose(fp);
		return NULL;
	}

	// 6 reserved bytes.
	Read32BE(fp);
	Read16BE(fp);

	// Read the number of channels (R, G, B, A, etc).
	int	ChannelCount = Read16BE(fp);
	if (ChannelCount < 0 || ChannelCount > 16) {
		fclose(fp);
		return NULL;
	}

	// Read the rows and columns of the image.
	int	height = Read32BE(fp);
	int	width = Read32BE(fp);

	// Make sure the depth is 8 bits.
	if (Read16BE(fp) != 8) {
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
	if (Read16BE(fp) != 3) {
		fclose(fp);
		return NULL;
	}

	// Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
	int	ModeDataCount = Read32BE(fp);
	if (ModeDataCount) {
		fseek(fp, ModeDataCount, SEEK_CUR);
	}

	// Process the image resources.  (resolution, pen tool paths, etc)
	int	ResourceDataCount = Read32BE(fp);
	if (ResourceDataCount) {
		// Read the image resources, looking for the info in the resolution block.
		float	HRes = 1.0;
		float	VRes = 1.0;
		ScanForResolution(&HRes, &VRes, fp, ResourceDataCount);

		// Set returns values for bitmap height/width.
		if (WidthPtr) {
			*WidthPtr = width / HRes * 100.0f;	// Compute size in m, then multiply by 100 since we want 1 Photoshop cm == 1 Soul Ride meter.
		}
		if (HeightPtr) {
			// Base the height on the greatest power-of-two
			// that fits in the pixel height.  This is to
			// compensate for additional mip-map data that
			// may be tacked onto the bottom of the image.
			int	HeightBits = height;
			int	i = height;
			for (;;) {
				i >>= 1;
				if (i == 0) break;
				HeightBits++;
			}
			
			*HeightPtr = (1 << HeightBits) / VRes * 100.0f;
		}
		
//		fseek(fp, ResourceDataCount, SEEK_CUR);
	}

//	// Skip the image resources.  (resolution, pen tool paths, etc)
//	int	ResourceDataCount = Read32BE(fp);
//	if (ResourceDataCount) {
//		fseek(fp, ResourceDataCount, SEEK_CUR);
//	}

	// Skip the reserved data.
	int	ReservedDataCount = Read32BE(fp);
	if (ReservedDataCount) {
		fseek(fp, ReservedDataCount, SEEK_CUR);
	}

	// Find out if the data is compressed.
	// Known values:
	//   0: no compression
	//   1: RLE compressed
	int	Compression = Read16BE(fp);
	if (Compression > 1) {
		// Unknown compression type.
		fclose(fp);
		return NULL;
	}

//	// Some formatting information about the channels.
//	const struct ChannelInfo {
//		int	Shift, Mask, Default;
//	} Channel[4] = {
//		{ 16, 0x00FF0000, 0 },	// Red.
//		{  8, 0x0000FF00, 0 },	// Green.
//		{  0, 0x000000FF, 0 },	// Blue.
//		{ 24, 0xFF000000, 255 }	// Alpha.
//	};
	// Some formatting information about the channels.
	struct ChannelInfo {
		int	Shift, Mask, Default;
	} Channel[4];

	// This is an endianness safe way to initialize the channel info
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
	bitmap32*	b = new bitmap32(width, height);
	int	PixelCount = height * width;

	// Initialize the data to zero.
	uint32*	p = b->GetData();
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
			const ChannelInfo&	c = Channel[channel];
			
			uint32*	p = b->GetData();
			if (channel >= ChannelCount) {
				// Fill this channel with default data.
				uint32	def = (c.Default << c.Shift) & c.Mask;	// default data for this channel.
				for (int i = 0; i < PixelCount; i++) {
					*p++ |= def;
				}
			} else {
				// Read the RLE data.
				int	count = 0;
				while (count < PixelCount) {
					int	len = fgetc(fp);
					if (len < 0) {
						// EOF or other problem.  Leave the remainder of
						// the channel filled w/ 0.
						break;
						
					} else if (len == 128) {
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
						uint32	val = (fgetc(fp) << c.Shift) & c.Mask;
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
			const ChannelInfo&	c = Channel[channel];
			
			uint32*	p = b->GetData();
			if (channel > ChannelCount) {
				// Fill this channel with default data.
				uint32	def = (c.Default << c.Shift) & c.Mask;
				for (int i = 0; i < PixelCount; i++) {
					*p++ |= def;
				}
			} else {
				// Read the data.
				for (int i = 0; i < PixelCount; i++) {
					*p++ |= (fgetc(fp) << c.Shift) & c.Mask;
				}
			}
		}
	}
	
	fclose(fp);

	return b;
}


//
// RLE as used by .PSD and .TIFF
//

//Loop until you get the number of unpacked bytes you are expecting:
//    Read the next source byte into n.
//    If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
//    Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
//    Else if n is 128, noop.
//Endloop


void	WriteImageData32(const char* filename, uint32* RGBAData, int Width, int Height, bool IncludeAlpha)
// Writes a .PSD file containing the given image data.  The data is assumed to
// start with the bottom row, and the red byte is assumed to come first.  (i.e.
// OpenGL's default format).
{
	int	channel, ChannelCount;
	
	// Open the file.
	FILE*	stream = fopen(filename, "wb");
	if (stream == NULL) {
		return;	// Could throw an exception or something.
	}
	
	// Write .PSD signature.
	int res = Write32BE(stream, 0x38425053);
	if (res != 4) goto io_error;

	// Write file version.
	res = Write16BE(stream, 1);
	if (res != 2) goto io_error;

	// Write 6 reserved bytes.
	Write32BE(stream, 0);
	Write16BE(stream, 0);

	// Write the number of channels: 4 (R, G, B, A) or 3 (R, G, B).
	if (IncludeAlpha) {
		Write16BE(stream, 4);
	} else {
		Write16BE(stream, 3);
	}

	// Write the row and column counts.
	Write32BE(stream, Height);
	Write32BE(stream, Width);

	// Write the bit depth (8).
	Write16BE(stream, 8);

	// Write the color mode (RGB).
	Write16BE(stream, 3);

	// Write the mode data count (0).
	Write32BE(stream, 0);

	// Write the image resource data count (0).
	Write32BE(stream, 0);

	// Write the reserved data count (0).
	Write32BE(stream, 0);

	// Write the compression flag (0 --> not compressed).
	Write16BE(stream, 0);
	
	// Write image data.

	ChannelCount = IncludeAlpha ? 4 : 3;
	
	for (channel = 0; channel < ChannelCount; channel++) {
		for (int h = 0; h < Height; h++) {
			// Get a row of data.
			uint8*	row = (uint8*) (RGBAData + Width * (Height - 1 - h));
			
			uint8*	p = row + channel;
			int	step = 4;

			// Write out the data.
			for (int w = 0; w < Width; w++) {
				fputc(*p, stream);
				p += step;
			}
		}
	}	

	fclose(stream);
	
	return;

io_error:
	fclose(stream);
	// throw an exception, log an error, or something?
	return;
}


}; // end namespace PSDRead

