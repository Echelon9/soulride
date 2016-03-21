// heightfield.h	- Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Dataset representing some height data.  Consists of a floatgrid,
// plus some metadata.


#ifndef HEIGHTFIELD_H
#define HEIGHTFIELD_H


#include "floatgrid.h"


struct heightfield
{
	heightfield();

	// Try to read heightfield data from a .BIL format dataset.  This
	// consists of a XXXXXX.BIL file that contains raster data, plus
	bool	read_bil_as_height(const char* filename);

	// My usual terrain coordinate system has x running from west to
	// east, and z running from north to south.  Thus, y is up, and
	// the x-y-z frame is right-handed.
	//
	// Return the height at the given coordinates.
	float	get_height(float x, float z);

	// @@ if we get around to reading land-cover data...
	// bool read_bil_as_land_cover(const char* filename);
	// enum land_cover { WATER, GRASS, ROCK, etc... };
	// land_cover	get_cover(float x, float z);	// land cover info

	// Access the underlying grid.
	const floatgrid&	grid() const { return m_grid; }
	floatgrid&	grid() { return m_grid; }

private:
	floatgrid	m_grid;
};


#endif // HEIGHTFIELD_H
