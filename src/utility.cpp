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
// utility.cpp	-thatcher 4/6/1998 Copyright Thatcher Ulrich

// Some utility functions.


#ifdef LINUX

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>

#endif // LINUX

#include <sys/types.h>
#include <sys/stat.h>
#include "config.hpp"
#include "utility.hpp"
#include "lua.hpp"
#include "error.hpp"


//
// Some little functions for reading/writing known-endian numbers.
//


uint32	Read32(FILE* fp)
// Reads the little-endian 32-bit int from the file and returns it.
{
	uint32	result;
	result = fgetc(fp);
	result |= fgetc(fp) << 8;
	result |= fgetc(fp) << 16;
	result |= fgetc(fp) << 24;
	return result;
}


uint32	Read16(FILE* fp)
// Reads the little-endian 16-bit int from the file and returns it.  Unsigned.
{
	uint32 result;
	result = fgetc(fp);
	result |= fgetc(fp) << 8;
	return result;
}


float	ReadFloat(FILE* fp)
// Reads a 32-bit float from the given stream.  Little endian format.
{
	union { float f; uint32 i; };
	i = Read32(fp);
	return f;
}


void	Write32(FILE* fp, uint32 d)
// Writes the data to the file as a 32-bit little-endian integer.
{
	fputc(d & 255, fp);
	fputc((d >> 8) & 255, fp);
	fputc((d >> 16) & 255, fp);
	fputc((d >> 24) & 255, fp);
}


void	Write16(FILE* fp, uint32 d)
// Writes the data as a 16-bit little-endian integer.
{
	fputc(d & 255, fp);
	fputc((d >> 8) & 255, fp);
}


void	WriteFloat(FILE* fp, float f)
// Writes the given float to the given stream.
{
	union { float fl; uint32 i; };
	fl = f;
	Write32(fp, i);
}


uint32	Read32BE(FILE* fp)
// Reads a big-endian 32-bit integer from the file and returns it.
{
	uint32	result;
	result = fgetc(fp) << 24;
	result |= fgetc(fp) << 16;
	result |= fgetc(fp) << 8;
	result |= fgetc(fp);
	return result;
}


uint32	Read16BE(FILE* fp) 
// Reads a big-endian 16-bit integer from the file and returns it.
{
	uint32	result;
	result = fgetc(fp) << 8;
	result |= fgetc(fp);
	return result;
}


int	Write32BE(FILE* fp, uint32 d)
// Writes the given int as a big-endian 32-bit integer.
// Returns the number of bytes written.
{
	fputc((d >> 24) & 255, fp);
	fputc((d >> 16) & 255, fp);
	fputc((d >> 8) & 255, fp);
	fputc((d) & 255, fp);
	return 4;
}


int	Write16BE(FILE* fp, uint32 d)
// Writes the given int as a big-endian 16-bit integer.
// Returns the number of bytes written.
{
	fputc((d >> 8) & 255, fp);
	fputc((d) & 255, fp);
	return 2;
}



//
// Some encoding/decoding functions for byte buffers.
//


int	DecodeUInt32(uint8* buf, int index, uint32* result)
// Extracts a 32-bit little-endian unsigned integer from the given
// buffer at the given index, and puts the result in *result.
// Returns the number of bytes extracted (in this case, always 4).
{
	uint32	r = buf[index];
	r |= buf[index+1] << 8;
	r |= buf[index+2] << 16;
	r |= buf[index+3] << 24;

	*result = r;
	
	return 4;
}


int	DecodeUInt16(uint8* buf, int index, uint16* result)
// Extracts a 16-bit little-endian unsigned integer from the given
// buffer at the given index, and puts the result in *result.
// Returns the number of bytes extracted (in this case, always 2).
{
	uint16	r = buf[index];
	r |= buf[index+1] << 8;

	*result = r;
	
	return 2;
}


int	DecodeFloat32(uint8* buf, int index, float* result)
// Extracts a 32-bit float from the given byte buffer at the
// given index, and stores the result in *result.
// Returns the number of bytes extracted (always 4).
{
	union {
		uint32	u;
		float	f;
	};
	DecodeUInt32(buf, index, &u);
	*result = f;
	return 4;
}


int	EncodeUInt32(uint8* buf, int index, uint32 data)
// Writes the given 32-bit integer data into the given byte buffer
// at the specified index.  Uses little-endian encoding.  Returns
// the number of bytes written (in this case, always 4).
{
	buf[index] = (data) & 0x0FF;
	buf[index+1] = (data >> 8) & 0x0FF;
	buf[index+2] = (data >> 16) & 0x0FF;
	buf[index+3] = (data >> 24) & 0x0FF;

	return 4;
}


int	EncodeUInt16(uint8* buf, int index, uint16 data)
// Writes the given 16-bit integer data into the given byte buffer
// at the specified index.  Uses little-endian encoding.  Returns
// the number of bytes written (in this case, always 2).
{
	buf[index] = (data) & 0x0FF;
	buf[index+1] = (data >> 8) & 0x0FF;

	return 2;
}


int	EncodeFloat32(uint8* buf, int index, float data)
// Writes the given 32-bit float data into the given byte buffer
// at the specified index.  Returns the number of bytes written (4).
{
	union {
		uint32	u;
		float	f;
	};

	f = data;

	return EncodeUInt32(buf, index, u);
}


//


namespace Utility {


bool	IsOpen = false;


static void	ARGB_lua()
// Utility function callable from Lua, which converts its input
// to a 32-bit ARGB value.
// If there's a single arg, then treats it as a string containing
// a hexadecimal value, and converts it.
// If there are 4 args, treats them as A, R, G, B values from 0.0 to 1.0 .
{
	uint32	val = 0;
	
	lua_Object	p1 = lua_getparam(1);
	lua_Object	p2 = lua_getparam(2);

	if (p2 == LUA_NOOBJECT) {
		// Only one parameter.  Treat as a hexadecimal string.
		const char*	p = lua_getstring(p1);

		// Skip any common hex prefixes.
		if (*p == 'x' || *p == '#') {
			p++;
		} else if (*p == '0' && *(p+1) == 'x') {
			p += 2;
		}

		// Convert from hex to int.
		while (*p) {
			val <<= 4;

			int	c = *p;
			if (c >= '0' && c <= '9') {
				c -= '0';
			} else if (c >= 'A' && c <= 'F') {
				c = c - 'A' + 10;
			} else if (c >= 'a' && c <= 'f') {
				c = c - 'a' + 10;
			} else {
				c = 0;
			}
			val += c;
			
			p++;
		}
	} else {
		// Treat input as four component params in the range 0.0 to 1.0.
		float	f[4];
		f[0] = (float) lua_getnumber(p1);
		f[1] = (float) lua_getnumber(p2);
		f[2] = (float) lua_getnumber(lua_getparam(3));
		f[3] = (float) lua_getnumber(lua_getparam(4));

		for (int i = 0; i < 4; i++) {
			val |= (iclamp(0, int(f[i] * 255), 255)) << (8 * (3 - i));
		}
	}

	// Return the result.
	lua_pushnumber(val);
}


void	Open()
// Set up.  Opens a log window, if config variable "Log" is true.
{
	if (IsOpen) return;

	lua_register("ARGB", ARGB_lua);

	IsOpen = true;
}


void	Close()
// Shut down.  Closes the log window.
{
	if (IsOpen) {
	}
	
	IsOpen = false;
}


const int	DEFAULT_PATH_MAXLEN = 200;
char	DefaultPath[DEFAULT_PATH_MAXLEN] = "";
int	PathLen = 0;


void	SetDefaultPath(const char* relpath)
// Sets the default relative path for opening data files.  This path is
// relative to the "data" subdirectory of the game install directory.
// If FileOpen() can't open the file in this subdirectory, then it tries
// again with the same file name, but without the default path prepended.
//
// For example, if the default path is "Mammoth", and FileOpen is called
// with a filename of "testfile.psd", FileOpen first tries to open
// "<gamedir>/data/Mammoth/testfile.psd".  If that fails, then FileOpen
// tries to open "<gamedir>/data/testfile.psd".  If that fails, then it
// returns NULL.
{
	// Store the default path.
	strncpy(DefaultPath, relpath, DEFAULT_PATH_MAXLEN-1);
	DefaultPath[DEFAULT_PATH_MAXLEN-2] = 0;
	strcat(DefaultPath, PATH_SEPARATOR);	// Store with a trailing slash.

	PathLen = strlen(DefaultPath);	// Cache the length for use later.
}


#ifdef LINUX

static FILE*	fopen_ignore_case(const char* filename, const char* mode)
// Looks in the given path for a file which matches the given filename
// and opens it if possible.  Compares filenames using
// case-independent compare.  This should result in Windows-like
// behavior under Linux, although it will prevent the engine from
// being able to distinguish between different filenames based on
// case.
{
	FILE*	fp = NULL;

	// First, separate the filename from the path.
	int	len = strlen(filename);
	const char*	fn = 0;
	int	i;
	for (i = len-1; i >= 0; i--) {
		if (filename[i] == PATH_SEPARATOR[0]) {
			fn = filename + i + 1;
			i++;
			break;
		}
	}
	if (fn == 0) {
		fn = filename;
		i = 0;
	}

	// Isolate the path in its own buffer.
	char	path[1000];
	if (i >= 1000) {
		Error e; e << "Buffer overflow in Utility::fopen_ignore_case().";
		throw e;
	}
	if (i) {
		strncpy(path, filename, i);
		path[i] = 0;
	} else {
		// If path is empty, then use "./"
		strcpy(path, "./");
	}


	// Now scan through the specified path, looking for a matching file.
	DIR*	dir = opendir(path);
	if (dir) {
		struct dirent*	ent;
		while ((ent = readdir(dir))) {
			if (StringCompareIgnoreCase(fn, ent->d_name) == 0) {
				// Found a matching filename; open 'er up.
				i += strlen(ent->d_name);
				if (i >= 1000) {
					Error e; e << "Buffer overflow in Utility::fopen_ignore_case().";
					throw e;
				}
				
				// Tack on the actual name of the file.
				strcat(path, ent->d_name);
				
				fp = fopen(path, mode);
				break;
			}
		}
		closedir(dir);
	}

	return fp;
}

#endif // LINUX


FILE*	FileOpen(const char* filename, const char* mode)
// Opens the file, just like the standard fopen().  Also logs the event to
// a file, for making a convenient list of data files used.
{
	static const int	maxlen = 1000;
	char	buf[maxlen];

	if (DefaultPath[0]) {
		// First, try prepending the DefaultPath to the filename.
		strcpy(buf, DefaultPath);
		strncpy(buf + PathLen, filename, maxlen - PathLen);
		buf[maxlen-1] = 0;

		FILE*	fp = fopen(buf, mode);
#ifdef LINUX		
		if (fp == NULL) fp = fopen_ignore_case(buf, mode);
#endif // LINUX
		if (fp) {
			FileOpenNotify(buf);
			return fp;
		}
	}

	// Now, try opening the file w/out the default path.
	FileOpenNotify(filename);
	FILE*	fp = fopen(filename, mode);
#ifdef LINUX
	if (fp == NULL) fp = fopen_ignore_case(filename, mode);
#endif // LINUX

	return fp;
}


void	FileOpenNotify(const char* filename)
// This function logs the given filename to a list file of all the files
// opened by the game engine.  It's strictly for convenience for later
// collecting the input files.
{
	static FILE*	FileListLog = NULL;
	static bool	FirstTime = true;

	if (FirstTime) {
		// The first time this function is called, make an attempt to open the log file.
		// If it fails, don't try again and don't do any logging.
		FirstTime = false;
		FileListLog = fopen("files-used.txt", "w");
	}

	if (FileListLog) fprintf(FileListLog, "%s\n", filename);
}


int	FileStat(const char* filename, struct stat* statbuf)
// Wrapper for OS function stat().  Tries using the default path
// before using current directory.
{
	static const int	maxlen = 1000;
	char	buf[maxlen];

	if (DefaultPath[0]) {
		// First, try prepending the DefaultPath to the filename.
		strcpy(buf, DefaultPath);
		strncpy(buf + PathLen, filename, maxlen - PathLen);
		buf[maxlen-1] = 0;

		int	res = ::stat(buf, statbuf);
		if (res == 0) return res;
	}

	// Now, try checking the file w/out the default path.
	return ::stat(filename, statbuf);
}


const char*	GetExtension(const char* filename)
// Returns the extension of the given filename.  Returns ""
// if the extension can't be located (i.e. if there's no '.' in the filename).
// Does not return the initial '.'.
{
	int	len = strlen(filename);

	// Search backwards for the first '.'.
	int	index = len - 1;
	while (index >= 0) {
		if (filename[index] == '.') {
			return filename + index + 1;
		}
		index--;
	}

	return filename + len;	// Null string at the very end of the filename.
}


void	StringLower(char* s)
// Force the given string to lowercase.
{
	while (*s) {
		*s = tolower(*s);
		s++;
	}
}


int	StringCompareIgnoreCase(const char* a, const char* b)
// Compare the two strings, ignoring case.  If they're equal,
// returns 0.  Else if a < b then returns -1, else returns +1.
{
	while (*a) {
		if (*b == 0) return 1;
		int	ca = tolower(*a);
		int	cb = tolower(*b);

		if (ca < cb) return -1;
		else if (ca > cb) return 1;

		a++;
		b++;
	}
	if (*b) return -1;
	else return 0;
}


static void	SplitList(StringElem** a, StringElem** b, StringElem* l)
// Splits l into two roughly equal sublists, and points *a and *b at the sublists.
{
	*a = *b = NULL;
	StringElem*	e;
	
	for (;;) {
		// One for a...
		if (l == NULL) break;
		e = l;
		l = l->Next;
		e->Next = *a;
		*a = e;

		// And one for b...
		if (l == NULL) break;
		e = l;
		l = l->Next;
		e->Next = *b;
		*b = e;
	}
}


static bool	StringElemLessThan(StringElem* one, StringElem* two)
// Returns true if one's string is "less than" two's string,
// by alphabetical order, ignoring case.
{
	char*	a = one->Name;
	char*	b = two->Name;
	while (*a) {
		if (*b == 0) return false;
		int	ca = tolower(*a);
		int	cb = tolower(*b);
		if (ca > cb) return false;
		else if (ca < cb) return true;
		a++;
		b++;
	}
	if (*b) return true;
	else return false;
}


static StringElem*	MergeLists(StringElem* a, StringElem* b)
// Takes two sorted sub-lists and joins them into one big sorted list.
// Returns the unified list.
{
	StringElem*	l = NULL;
	StringElem**	end = &l;

	for (;;) {
		if (a && b) {
			// Pick the head of either a or b, according to whichever has a smaller OrderKey.
			if (StringElemLessThan(a, b)) {
				*end = a;
				a = a->Next;
				(*end)->Next = NULL;
				end = &((*end)->Next);
			} else {
				*end = b;
				b = b->Next;
				(*end)->Next = NULL;
				end = &((*end)->Next);
			}
		} else if (a) {
			*end = a;
			break;
		} else if (b) {
			*end = b;
			break;
		} else {
			break;
		}
	}

	return l;
}


static StringElem*	MergeSort(StringElem* l)
// Given a linked list of string elements, sorts the list according to
// case-insenstive alphabetical order and returns the re-arranged list.
{
	if (l == NULL || l->Next == NULL) return l;

	StringElem	*a, *b;
	SplitList(&a, &b, l);
	a = MergeSort(a);
	b = MergeSort(b);

	return MergeLists(a, b);
}


StringElem*	SortStringList(StringElem* l)
// Sorts the given list of strings, in ascending alphabetical order,
// ignoring case.
{
	return MergeSort(l);
}


float	RandomFloat(float Min, float Max)
// Returns a random number between Min and Max.
{
	return (rand() * (Max - Min) / float(RAND_MAX)) + Min;
}


int	RandomInt(int Max)
// Returns a random integer between 0 and Max - 1.
{
	if (RAND_MAX > 2 << 16) {
		return ((rand() & 0x0FFFF) * Max) / (0x10000);
	} else {
		return (rand() * Max) / (RAND_MAX + 1);
	}
}


};	// namespace Utility


#ifdef NOT

/*
 * Good Random Number Generator
 * Supposed to return numbers between 0.0 and 1.0 
 *   getRand() * n produces 0 <= i < n
 */
long seed;

#define  IM1 2147483563L
#define  IM2 2147483399L
#define IMM1 (IM1 - 1L)
#define  IA1 40014L
#define  IA2 40692L
#define  IQ1 53668L
#define  IQ2 52774L
#define  IR1 12211L
#define  IR2  3791L
#define NTAB    32L
#define NDIV (1L + IMM1 / (long)NTAB)

#define RNMX (1.0F - FLT_EPSILON)  // 1.192092896e-07F for windows
#define AM   (1.0F / 2147483563.0F)

void setSeed(long initSeed)
{
  if (initSeed < 0) 
    seed = initSeed;
  else 
    seed = -initSeed;
}

float getRand()
{
  long j, k;
  static long idum2 = 123456789L;
  static long iy    = 0L;
  static long iv[(size_t)NTAB];
  float temp;

  if (seed <= 0L) {
    if (-seed < 1L)
      seed = 1L;
    else
      seed = -seed;

    idum2 = seed;

    for (j = NTAB + 7; j >= 0; --j) {
      k = seed / IQ1;
      seed = IA1 * (seed - k * IQ1) - k * IR1;

      if (seed < 0L)
        seed += IM1;

      if (j < NTAB)
        iv[(size_t)j] = seed;
    }

    iy = iv[0];
  }

  k = seed / IQ1;

  seed = IA1 * (seed - k * IQ1) - k * IR1;

  if (seed < 0L)
    seed += IM1;

  k = idum2 / IQ2;

  idum2 = IA2 * (idum2 - k * IQ2) - k * IR2;

  if (idum2 < 0L)
    idum2 += IM2;

  j  = iy / NDIV;
  iy = iv[(size_t)j] - idum2;
  iv[(size_t)j] = seed;

  if (iy < 1L)
    iy += IMM1;

  temp = AM * (float)iy;

  if (temp > RNMX)
    return RNMX;
  else
    return temp;
}


//// Basic VC approach:
//holdrand = (holdrand * 214013L) + 2531011L;



// via Kent Quirk:
// the hash is a standard hash algorithm from Sedgewick; he used 
// 101 for the prime, but I wanted a larger pool to minimize the 
// prospect of a collision. 101111 is also
// a prime (both in decimal and in binary!)
inline int hash(const char* key)
{
  for (int h=0; *key; ++key)
    h = (64*h + *key) % 101111;
  return h;
}



#endif	// NOT
