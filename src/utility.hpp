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
// utility.hpp	-thatcher 3/27/1998 Copyright Thatcher Ulrich

// Basic, universal stuff.


#ifndef UTILITY_HPP
#define UTILITY_HPP


#include <stdio.h>
#include <string.h>
#include <math.h>
#include "types.hpp"


#ifdef LINUX
#define PATH_SEPARATOR "/"
#else // not LINUX
#define PATH_SEPARATOR "\\"
#endif // not LINUX


// min, max's, clamp.


template<class T>
T	tmin(T a, T b)
// Return the smaller of the two values.
{
	if (a < b) return a;
	else return b;
}

inline float	fmin(float a, float b) { return tmin(a, b); }
inline int	imin(int a, int b) { return tmin(a, b); }


template<class T>
T	tmax(T a, T b)
// Return the larger of the two values.
{
	if (a > b) return a;
	else return b;
}

inline float	fmax(float a, float b) { return tmax(a, b); }
inline int	imax(int a, int b) { return tmax(a, b); }


template<class T>
T	tclamp(T min, T val, T max)
// Return the value, but make sure it's within the given bounds.
{
	if (val <= min) return min;
	else if (val >= max) return max;
	else return val;
}

inline float	fclamp(float min, float val, float max) { return tclamp(min, val, max); }
inline int	iclamp(int min, int val, int max) { return tclamp(min, val, max); }


#define PI	3.141592654f



inline int	frnd(float f)
// Rounds f to the nearest int.
{
#ifdef _MSC_VER	// MS Visual C++
	int	i;
	_asm {
		fld	f;
		fistp	i;
	}
	return i;
#else // LINUX (really gcc)
 #ifdef __i386__
	int	i;
	asm("fistpl %0" : "=m" (i) : "t" (f) : "st");
	return i;
 #else
	return lrintf(f);
 #endif
#endif // LINUX (really gcc)
}


inline int	fchop(float f)
// Truncates f and returns int.
{
	return frnd(f - 0.5f);
}


//
// Short functions for reading/writing to/from files.
//


// Functions for little-endian data on disk.
uint32	Read32(FILE* fp);
uint32	Read16(FILE* fp);
float	ReadFloat(FILE* fp);
void	Write32(FILE* fp, uint32 d);
void	Write16(FILE* fp, uint32 d);
void	WriteFloat(FILE* fp, float f);

// Functions for big-endian data on disk.
uint32	Read32BE(FILE* fp);
uint32	Read16BE(FILE* fp);
int	Write32BE(FILE* fp, uint32 d);
int	Write16BE(FILE* fp, uint32 d);


//
// Functions for encoding/decoding values from/to a byte buffer.
// Each function returns the number of buffer bytes read/written.
//

int	EncodeUInt32(uint8* buf, int index, uint32 value);
int	EncodeFloat32(uint8* buf, int index, float value);
int	EncodeUInt16(uint8* buf, int index, uint16 value);

int	DecodeUInt32(uint8* buf, int index, uint32* value);
int	DecodeFloat32(uint8* buf, int index, float* value);
int	DecodeUInt16(uint8* buf, int index, uint16* value);


// Class for keeping named objects.  Typically the "objects" will be
// pointers to some type.
template<class T>
class BagOf {
public:
	BagOf() {
		List = 0;
	}

	~BagOf() {
		Empty();
	}

	void	Empty()
	// Delete the contents of the bag.
	{
		ListElem*	n = List;

		while (n) {
			ListElem*	e = n;
			n = n->Next;
			delete e;
		}
		
		List = 0;
	}
	
	void	Add(T NewObject, const char* Name)
	// Adds a named object to the bag.  If an object with the
	// name is already in the bag, then replaces the existing
	// object with the new one.
	{
		// Lame-o flat linked-list implementation.
		// Change to a hashed tree or something later.

		ListElem*	e = FindElement(Name);
		if (e) {
			// Change the existing element.
			e->Object = NewObject;
		} else {
			// Create a new element and put it at the head of the list.
			e = new ListElem(NewObject, Name, List);
			List = e;
		}
	}

	bool	GetObject(T* ReturnValue, const char* Name)
	// Retrieves the object with the given name from the bag,
	// and copies it to *ReturnValue, and returns true.
	// If the object can't be found, returns false and leaves
	// *ReturnValue alone.
	{
		ListElem*	e = FindElement(Name);

		if (e) {
			// Found an element of the same name.
			*ReturnValue = e->Object;
			return true;
		} else {
			return false;
		}
	}

	void	Apply(void (*func)(T obj, const char* Name))
	// Calls func() for every object in the bag.
	{
		for (ListElem* e = List; e; e = e->Next) { func(e->Object, e->Name); }
	}

	
private:
	struct ListElem {
		T	Object;
		char*	Name;
		ListElem*	Next;

		ListElem(T o, const char* name, ListElem* next) : Object(o), Next(next)
		{
			Name = new char[strlen(name) + 1];
			strcpy(Name, name);
		}

		~ListElem()
		{
			delete [] Name;
//			delete Object;	// hm.  Only valid if T is a data pointer type.  Should generate a compile error if not.
		}
	};

	ListElem*	FindElement(const char* Name)
	// Returns a pointer to the element, if any, which
	// has the given name.
	{
		for (ListElem* e = List; e; e = e->Next) {
			if (strcmp(e->Name, Name) == 0) {
				return e;
			}
		}

		// Element with matching name wasn't found.
		return 0;
	}

	ListElem*	List;

public:
	struct Iterator {
		ListElem*	Elem;

		T	operator++(int) { T result = Elem->Object; Elem = Elem->Next; return result; }
		T&	operator*() { return Elem->Object; }
		bool	IsDone() { return Elem == NULL; }
		const char*	GetName() { return Elem->Name; }
	};

	Iterator	GetIterator()
	// Returns an iterator that can be used to step through all the elements
	// in this bag.
	{
		Iterator	b;
		b.Elem = List;
		return b;
	}
};


class SampleFIFO {
public:
	SampleFIFO(int count)
	// Construct a buffer that can hold the specified number of samples.
	{
		// Allocate.
		Count = count;
		Sample = new SampleInfo[count];

		// Initialize.
		for (int i = 0; i < Count; i++) {
			Sample[i].Timestamp = 0;
			Sample[i].Value = 0;
		}
	}

	void	AddSample(float value, int ticks)
	// Adds a new sample value into the data set.  (For the moment,
	// at least) assume that samples come in chronological order, so
	// don't bother trying to search for the proper insertion point.
	// Just assume we insert at the head.
	{
		for (int i = Count - 1; i > 0; i--) {
			Sample[i] = Sample[i-1];
		}
		Sample[0].Value = value;
		Sample[0].Timestamp = ticks;
	}
	
	float	GetInterpolatedValue(int ticks) const
	// Returns the value of the function represented by the samples
	// in the FIFO.  Interpolates if necessary between sample values.
	{
		// Search for the index of the first sample whose timestamp is less than ticks...
		int	i;
		for (i = 0; i < Count; i++) {
			if (Sample[i].Timestamp == ticks) {
				// Dead nuts on a sample value.
				return Sample[i].Value;
			} else if (Sample[i].Timestamp < ticks) {
				break;
			}
		}

		if (i == 0) {
			// If we're outside the defined domain of the function,
			// then return 0.
			return 0;
		} else if (i >= Count) {
			// Outside the domain on the other end.
			return 0;
		} else {
			// Return the interpolated value.
			float f = float((ticks - Sample[i].Timestamp) / (Sample[i-1].Timestamp - Sample[i].Timestamp));
			return Sample[i].Value + (Sample[i-1].Value - Sample[i].Value) * f;
		}
	}

	float	Product(const SampleFIFO& factor, int TimeOffset) const
	// Offsets the given factor function by the given time offset, and computes the integral
	// of the product of the two functions.
	// Time values are all in milliseconds.
	{
		int	AIndex = 0;
		int	BIndex = 0;
		int	t;
		float	AVal0;
		float	BVal0;
		float	Integral = 0;

		// Find the start of the integral of products -- start
		// with the earlier of the first samples of the two
		// functions.
		if (Sample[0].Timestamp < factor.Sample[0].Timestamp + TimeOffset) {
			t = Sample[0].Timestamp;
			AVal0 = Sample[0].Value;
			AIndex = 1;

			for (BIndex = 0; BIndex < factor.Count; BIndex++) {
				if (factor.Sample[BIndex].Timestamp + TimeOffset < t) break;
			}

			BVal0 = factor.GetInterpolatedValue(t - TimeOffset);
			
		} else if (Sample[0].Timestamp > factor.Sample[0].Timestamp + TimeOffset) {
			t = factor.Sample[0].Timestamp + TimeOffset;
			BVal0 = factor.Sample[0].Value;
			BIndex = 1;

			for (AIndex = 0; AIndex < Count; AIndex++) {
				if (Sample[BIndex].Timestamp < t) break;
			}

			AVal0 = GetInterpolatedValue(t);
		} else {
			t = Sample[0].Timestamp;
			AVal0 = Sample[0].Value;
			AIndex = 1;

			BVal0 = factor.Sample[0].Value;
			BIndex = 1;
		}
		
		// Now compute the integral as a sum of products of trapezoids.
		for (;;) {
			if (AIndex >= Count || BIndex >= factor.Count) break;
			
			int	nextt;
			float	AVal1, BVal1;
			// Advance to the next sample of either function.
			if (Sample[AIndex].Timestamp > factor.Sample[BIndex].Timestamp + TimeOffset) {
				nextt = Sample[AIndex].Timestamp;
				AVal1 = Sample[AIndex].Value;
				AIndex++;

				BVal1 = factor.GetInterpolatedValue(nextt - TimeOffset);	// could optimize this.
				
			} else if (Sample[AIndex].Timestamp < factor.Sample[BIndex].Timestamp + TimeOffset) {
				nextt = factor.Sample[BIndex].Timestamp + TimeOffset;
				BVal1 = factor.Sample[BIndex].Value;
				BIndex++;

				AVal1 = factor.GetInterpolatedValue(nextt);	// could optimize this.
				
			} else {
				// Both functions have a sample at this same point.
				nextt = Sample[AIndex].Timestamp;
				AVal1 = Sample[AIndex].Value;
				AIndex++;

				BVal1 = factor.Sample[BIndex].Value;
				BIndex++;
			}

			// Multiply the two trapezoids, and add the product to the integral.
			int	DeltaT = t - nextt;
			if (DeltaT) {
				float	DeltaA = AVal1 - AVal0;
				float	DeltaB = BVal1 - BVal0;
				Integral += (AVal0 * BVal0 +
					     0.5f * (DeltaA * BVal0 + DeltaB * AVal0) +
					     1.0f / 3.0f * DeltaA * DeltaB * DeltaT)
					    * DeltaT;
			}

			t = nextt;
			AVal0 = AVal1;
			BVal0 = BVal1;
		}

		return Integral / 1000.0f;	// Convert millisecond scale to seconds.
		
//weak		int	i = 0;
//		float	Total = 0;
//		for (i = 0; i < factor.Count; i++) {
//			Total += factor.Sample[i].Value * GetInterpolatedValue(factor.Sample[i].Timestamp);
//		}
//		return Total;
	}

//	int	GetSampleCount() const { return Count; }
	
private:
	struct SampleInfo {
		float	Value;
		int	Timestamp;
	} *Sample;
	int	Count;
};


// For single-linked lists of strings.
struct StringElem {
	StringElem*	Next;
	char*	Name;

	StringElem() {
		Next = NULL;
		Name = NULL;
	}
	StringElem(const char* val) {
		Next = NULL;
		int	len = strlen(val);
		Name = new char[len+1];
		strcpy(Name, val);
	}
	~StringElem() {
		if (Next) {
			delete Next;
		}
		if (Name) {
			delete [] Name;
		}
	}

	int	CountElements()
	{
		if (this == NULL) {
			return 0;
		} else if (Next == NULL) {
			return 1;
		} else {
			return 1 + Next->CountElements();
		}
	}
};
	

struct stat;


namespace Utility {
	void	Open();
	void	Close();

	void	SetDefaultPath(const char* relpath);	// Default data path, relative to "./data/".  For separating different mountains from each other.
	FILE*	FileOpen(const char* filename, const char* mode);
	void	FileOpenNotify(const char* filename);	// Strictly for logging purposes.

	int	FileStat(const char* filename, struct stat* buf);
	
//	void	Log(int Row, const char* message);

	const char*	GetExtension(const char* filename);

	float	RandomFloat(float Min, float Max);
	int	RandomInt(int Range);	// 0 <= result < Range

	void	StringLower(char* s);
	int	StringCompareIgnoreCase(const char* a, const char* b);

	StringElem*	SortStringList(StringElem* string_list);
};


#ifdef _WIN32
# define snprintf _snprintf
#endif // _WIN32


#endif // UTILITY_HPP

