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
// avi_tools.cpp	-thatcher 12/30/2000 Copyright Slingshot Game Technology

// Some code to deal with AVI files.  There are a number of spots where
// correct output depends on little-endian memory layout and 32-bit
// aligned structure packing.  Could be rewritten without the fixed
// structure defs to port to Mac or something.


#include <stdio.h>
#include "avi_tools.hpp"
#include "error.hpp"
#include "utility.hpp"


namespace avi_tools {
;


/*

AVI file format info, excerpted from "John McGowan's AVI Overview:
Programming and Other Technical Topics" at
http://jmcgowan.com/avitech.html
© 2000 by John F. McGowan, Ph.D.

====================================================================

<H3>AVI File Format</H3>

	AVI is a specialization or "form" of RIFF, described below:


'RIFF' (4 byte file length) 'AVI '   // file header (a RIFF form)

'LIST' (4 byte list length) 'hdrl'   // list of headers for AVI file

The 'hdrl' list contains:

'avih' (4 byte chunk size)  (data)   // the AVI header  (a chunk)

'strl' lists of stream headers for each stream (audio, video, etc.) in
the AVI file.  An AVI file can contain zero or one video stream and
zero, one, or many audio streams.  For an AVI file with one video and
one audio stream:

'LIST' (4 byte list length) 'strl'   // video stream list (a list)

The video 'strl' list contains:

'strh' (4 byte chunk size)  (data)   // video stream header (a chunk)
'strf' (4 byte chunk size)  (data)   // video stream format (a chunk)

'LIST' (4 byte list length) 'strl'   // audio stream list (a list)

The audio 'strl' list contains:

'strh' (4 byte chunk size)  (data)   // audio stream header (a chunk)
'strf' (4 byte chunk size)  (data)   // audio stream format (a chunk)

'JUNK' (4 byte chunk size) (data - usually all zeros) // an OPTIONAL junk chunk to align on 2K byte boundary

'LIST' (4 byte list length) 'movi'   // list of movie data (a list)

The 'movi' list contains the actual audio and video data.   
 
This 'movi' list contains one or more ...
 'LIST' (4 byte list length) 'rec '  // list of movie records (a list)
 '##wb' (4 byte chunk size) (data)   // sound data (a chunk)
 '##dc' (4 byte chunk size) (data)   // video data (a chunk)
 '##db' (4 byte chunk size) (data)   // video data (a chunk)

A 'rec ' list (a record) contains the audio and video data for a single frame.
 '##wb' (4 byte chunk size)  (data)  // sound data (a chunk)
 '##dc' (4 byte chunk size)  (data)  // video data (a chunk)
 '##db' (4 byte chunk size)  (data)  // video data (a chunk)

The 'rec ' list may not be used for AVI files with only audio or only
video data.  I have seen video only uncompressed AVI files that did
not use the 'rec ' list, only '00db' chunks.  The 'rec ' list is used
for AVI files with interleaved audio and video streams.  The 'rec '
list may be used for AVI file with only video.

 ## in '##dc' refers to the stream number.  For example, video data chunks
belonging to stream 0 would use the identifier '00dc'.  A chunk of
video data contains a single video frame.

Alexander Grigoriev writes ...

John,

##dc chunk was intended to keep compressed data, whereas ##db chunk
nad(sic) to be used for uncompressed DIBs (device independent bitmap),
but actually they both can contain compressed data. For example,
Microsoft VidCap (more precisely, video capture window class) writes
MJPEG compressed data in ##db chunks, whereas Adobe Premiere writes
frames compressed with the same MJPEG codec as ##dc chunks.

----End of Alexander

The ##wb chunks contain the audio data.

The audio and video chunks in an AVI file do not contain 
time stamps or frame counts.  The data is ordered in time sequentially as
it appears in the AVI file.  A player application should display the
video frames at the frame rate indicated in the headers.  The
application should play the audio at the audio sample rate indicated
in the headers.  Usually, the streams are all assumed to start at
time zero since there are no explicit time stamps in the AVI file.

The lack of time stamps is a weakness of the original AVI file
format.  The OpenDML AVI Extensions add new chunks with time
stamps.  Microsoft's ASF (Advanced or Active Streaming Format), which
Microsoft claims will replace AVI, has time stamp "objects".

In principle, a video chunk contains a single frame of video.  By
design, the video chunk should be interleaved with an audio chunk
containing the audio associated with that video frame.  The data
consists of pairs of video and audio chunks.  These pairs may be
encapsulated in a 'REC ' list.  Not all AVI files obey this simple
scheme.  There are even AVI files with all the video followed by all
of the audio; this is not the way an AVI file should be made.

 The 'movi' list may be followed by:

 'idx1' (4 byte chunk size) (index data) // an optional index into movie (a chunk)

  The optional index contains a table of memory offsets to each
chunk within the 'movi' list.  The 'idx1' index supports rapid
seeking to frames within the video file.  

  The 'avih' (AVI Header) chunk contains the following information:

      Total Frames   (for example, 1500 frames in an AVI)
      Streams   (for example, 2 for audio and video together)
      InitialFrames
      MaxBytes
      BufferSize
      Microseconds Per Frame
      Frames Per Second   (for example, 15 fps)
      Size  (for example 320x240 pixels)
      Flags

  The 'strh' (Stream Header) chunk contains the following information:

      Stream Type  (for example, 'vids' for video  'auds' for audio)
      Stream Handler  (for example, 'cvid' for Cinepak)
      Samples Per Second  (for example 15 frames per second for video)
      Priority
      InitialFrames
      Start
      Length  (for example, 1500 frames for video)
      Length (sec)   (for example 100 seconds for video)
      Flags
      BufferSize
      Quality
      SampleSize

   For video, the 'strf' (Stream Format) chunk contains the following 
information:

      Size  (for example 320x240 pixels)
      Bit Depth (for example 24 bit color)
      Colors Used  (for example 236 for palettized color)
      Compression  (for example 'cvid' for Cinepak)

   For audio, the 'strf' (Stream Format) chunk contains the following
information:

      wFormatTag           (for example, WAVE_FORMAT_PCM)
      Number of Channels   (for example 2 for stereo sound)
      Samples Per Second   (for example 11025)
      Average Bytes Per Second   (for example 11025 for 8 bit sound)
      nBlockAlign
      Bits Per Sample      (for example 8 or 16 bits)

  Each 'rec ' list contains the sound data and video data for a single
frame in the sound data chunk and the video data chunk.  

  Other chunks are allowed within the AVI file.  For example, I have
seen info lists such as

    'LIST' (4 byte list size) 'INFO' (chunks with information on video)

  These chunks that are not part of the AVI standard are simply
ignored by the AVI parser.  AVI can be and has been extended by adding
lists and chunks not in the standard.  The 'INFO' list is a registered
global form type (across all RIFF files) to store information that
helps identify the contents of a chunk.

  The sound data is typically 8 or 16 bit PCM, stereo or mono,
sampled at 11, 22, or 44.1 KHz.  Traditionally, the sound has
typically been uncompressed Windows PCM.  With the advent of
the WorldWide Web and the severe bandwidth limitations of the
Internet, there has been increasing use of audio codecs.  The
wFormatTag field in the audio 'strf' (Stream Format) chunk
identifies the audio format and codec.

====================================================================
end excerpt


typedef struct _AviIndex
{
    DWORD Identifier;    // Chunk identifier reference
    DWORD Flags;         // Type of chunk referenced
    DWORD Offset;        // Position of chunk in file
    DWORD Length;        // Length of chunk in bytes
} AVIINDEX;

Identifier contains the 4-byte identifier of the chunk it references (strh, strf, strd,
and so on).
Flags bits are used to indicate the type of frame the chunk contains or to identify the
index structure as pointing to a LIST chunk.
Offset indicates the start of the chunk in bytes relative to the movi list chunk.
Length is the size of the chunk in bytes.

   
*/


typedef uint32 DWORD;
typedef int32 LONG;
typedef uint16 WORD;


// From MS documentation.


struct MainAVIHeader {
	DWORD dwMicroSecPerFrame;
	DWORD dwMaxBytesPerSec;
	DWORD dwReserved1;
	DWORD dwFlags;
	DWORD dwTotalFrames;
	DWORD dwInitialFrames;
	DWORD dwStreams;
	DWORD dwSuggestedBufferSize;
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwReserved[4];
};


typedef DWORD FOURCC;


struct AVIStreamHeader {
	FOURCC fccType;
	FOURCC fccHandler;
	DWORD  dwFlags;
	DWORD  dwPriority;
	DWORD  dwInitialFrames;
	DWORD  dwScale;
	DWORD  dwRate;
	DWORD  dwStart;
	DWORD  dwLength;
	DWORD  dwSuggestedBufferSize;
	DWORD  dwQuality;
	DWORD  dwSampleSize;
//	RECT   rcFrame;
	DWORD	FrameLeft, FrameTop, FrameRight, FrameBottom;
};


struct BITMAPINFO {
	DWORD  biSize;
	LONG   biWidth;
	LONG   biHeight;
	WORD   biPlanes;
	WORD   biBitCount;
	DWORD  biCompression;
	DWORD  biSizeImage;
	LONG   biXPelsPerMeter;
	LONG   biYPelsPerMeter;
	DWORD  biClrUsed;
	DWORD  biClrImportant;
};


#define BI_RGB 0L


void	avi_stream::open_video(FILE* f, int width, int height, int frame_period_us)
// Initializes this stream object for writing AVI video on the given file.
// width and height are the pixel dimensions.
// frame_period_us is the time duration of a frame, in microseconds.
{
	int	i;
	
	// Initialize members.
	fp = f;
	ChunkSP = 0;
	FrameCount = 0;
	FrameChunkSize = width * height * 3;

	// Write video header.
	open_chunk("RIFF");
	fwrite("AVI ", 1, 4, fp);

	open_chunk("LIST");
	fwrite("hdrl", 1, 4, fp);

	MainAVIHeader	mh;
	memset(&mh, 0, sizeof(mh));
	mh.dwMicroSecPerFrame = frame_period_us;
	mh.dwMaxBytesPerSec = 0;
	mh.dwFlags = 0x10 /* | AVIF_HASINDEX == 0x10 */;
	mh.dwTotalFrames = 0;	// framecount: fixup at offset 0x30
	mh.dwInitialFrames = 0;
	mh.dwStreams = 1;	// Increment if audio is included.
	mh.dwSuggestedBufferSize = width * height * 3;
	mh.dwWidth = width;
	mh.dwHeight = height;

	open_chunk("avih");
	fwrite(&mh, 1, sizeof(mh), fp);	// xxx little-endian
	close_chunk();

	// Video headers.
	open_chunk("LIST");
	fwrite("strl", 1, 4, fp);

	AVIStreamHeader	sh;
	memset(&sh, 0, sizeof(sh));
	sh.fccType = 0x73646976;	// "vids";	// xxx little-endian dependency.
	sh.fccHandler = 0x20424944;	// "DIB ";
	sh.dwScale = 1;
	sh.dwRate = 1000000 / frame_period_us;
	sh.dwLength = 0;	// depends on framecount: fixup @ offset 0x8C
	sh.dwSuggestedBufferSize = 3 * width * height;
	sh.dwQuality = (DWORD) -1;
	sh.FrameLeft = sh.FrameTop = 0;
	sh.FrameRight = width;
	sh.FrameBottom = height;
			
	open_chunk("strh");
	fwrite(&sh, 1, sizeof(sh), fp);	// xxx little-endian
	close_chunk();

	BITMAPINFO	bi;
	memset(&bi, 0, sizeof(bi));
	bi.biSize = sizeof(bi);
	bi.biWidth = width;
	bi.biHeight = height;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = 0;
	bi.biSizeImage = 3 * width * height;
	
	open_chunk("strf");
	fwrite(&bi, 1, sizeof(bi), fp);	// xxx little-endian
	close_chunk();

	close_chunk();	// LIST strl

	close_chunk();	// LIST hdrl

	// Put some JUNK padding in.
	open_chunk("JUNK");
	for (i = 0; i < 3856; i++) { fputc(0, fp); }
	close_chunk();
	
	// Movie data.
	open_chunk("LIST");
	fwrite("movi", 1, 4, fp);
}


void	avi_stream::write_video_frame(uint32* pixels, int width, int height)
// Writes the given frame of video to this open avi_stream.
// Pixels are assumed to be in RGBA format, in OpenGL default layout;
// i.e. left to right, bottom to top.
{
	open_chunk("00db");

	int	x, y;
	for (y = 0; y < height; y++) {
		uint8*	p = (uint8*) (pixels + ((y) * width));	// Write the rows in reverse order of the screenshot data.
		for (x = 0; x < width; x++) {
			fputc(p[2], fp);
			fputc(p[1], fp);
			fputc(p[0], fp);
//			fwrite(p, 1, 3, fp);
			p += 4;
		}
	}

	close_chunk();

	FrameCount++;
}


void	avi_stream::close()
// Closes all open chunks, and fixes up all required frame-count fixups.
{
	int	i;
	
	// Close the "movi" chunk.
	close_chunk();
	
	// Write an index chunk.
	open_chunk("idx1");
	int	offset = 4;	// offset of first 00db chunk from the start of the movi chunk.
	for (i = 0; i < FrameCount; i++) {
		Write32(fp, 0x62643030);	// 00db
		Write32(fp, 16);	// chunk type
		Write32(fp, offset);	// offset in file
		Write32(fp, FrameChunkSize);	// size of chunk
		offset += FrameChunkSize + 8;
	}
	close_chunk();

	// Close all open chunks.
	while (ChunkSP > 0) close_chunk();

	// Go back and fix a couple of spots which depend on framecount.
	fseek(fp, 0x30, SEEK_SET);
	Write32(fp, FrameCount);

	fseek(fp, 0x8C, SEEK_SET);
	Write32(fp, FrameCount);

	// Leave file pointer at end.
	fseek(fp, 0, SEEK_END);
}


/**
 * Opens a new chunk, and starts it with the given four-character-code tag.
 * @param fourcc Character code
 */
void
avi_stream::open_chunk(const char* fourcc)
{
	// Chunk type.
	fwrite(fourcc, 1, 4, fp);

	// Remember the location in the file of the chunk-size data, so we
	// can fill in the right value when we close the chunk.
	ChunkStack[ChunkSP] = ftell(fp);

	// Chunk size.
	Write32(fp, 0);	// Dummy value of 0 for now.

	ChunkSP++;
}


void	avi_stream::close_chunk()
// Closes the current chunk.  Writes the chunk size in the appropriate spot.
{
	ChunkSP--;
	
	long	here = ftell(fp);
	long	size_loc = ChunkStack[ChunkSP];

	// Compute the data size of this chunk.
	int	size = here - size_loc - 4;

	// Write the chunk's actual size into the chunk header.
	fseek(fp, size_loc, SEEK_SET);
	Write32(fp, size);
	fseek(fp, here, SEEK_SET);
}


}; // end namespace avi_tools

