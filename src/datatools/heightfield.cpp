// heightfield.cpp	- Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Dataset representing some height data.  Consists of a floatgrid,
// plus some metadata.


#include "heightfield.h"


heightfield::heightfield()
{
}


bool	heightfield::read_bil_as_height(const char* filename)
// Try to read heightfield data from a .BIL format dataset.  This
// consists of a XXXXXX.BIL file that contains raster data, plus a
// .BLW file that contains info about the coordinates covered by the
// file.
//
// Given filename should be the path and base name of the dataset
// files, not including the extensions.
//
// Returns true on successful read; false on failure.
{
	// These should be decimal lon/lat
	double	x_dim, y_dim, x_nw, y_nw;

	// Try to open the .BLW file.
	char*	filename_buffer = new char[strlen(filename) + 5];
	strcpy(filename_buffer, filename);
	strcat(filename_buffer, ".BLW");

	const static int	BUF_SIZE = 500;
	char	line_buf[BUF_SIZE];

	FILE*	blw_file = fopen(filename_buffer, "r");
	if (blw_file == NULL)
	{
		// Can't open the .BLW file.
		delete [] filename_buffer;
		return false;
	}

	// Read data from .BLW.

	// Get the x dimension.
	if (fgets(line_buf, BUF_SIZE, blw_file) == NULL)
	{
		// Read error.
		delete [] filename_buffer;
		fclose(blw_file);
		return false;
	}
	x_dim = atof(line_buf);

	// Skip the next two lines (rotation terms; hopefully they're 0!)
	fgets(line_buf, BUF_SIZE, blw_file);
	fgets(line_buf, BUF_SIZE, blw_file);

	// Get the y dimension.
	if (fgets(line_buf, BUF_SIZE, blw_file) == NULL)
	{
		// Read error.
		delete [] filename_buffer;
		fclose(blw_file);
		return false;
	}
	y_dim = atof(line_buf);
	
	// Get the x value of the nw corner.
	if (fgets(line_buf, BUF_SIZE, blw_file) == NULL)
	{
		// Read error.
		delete [] filename_buffer;
		fclose(blw_file);
		return false;
	}
	x_nw = atof(line_buf);
	
	// Get the y value of the nw corner.
	if (fgets(line_buf, BUF_SIZE, blw_file) == NULL)
	{
		// Read error.
		delete [] filename_buffer;
		fclose(blw_file);
		return false;
	}
	y_nw = atof(line_buf);

	fclose(blw_file);

	// Read the .HDR file, to get a format.
	strcpy(filename_buffer, filename);
	strcat(filename_buffer, ".HDR");

	FILE*	hdr_file = fopen(filename_buffer, "r");
	if (hdr_file == NULL)
	{
		// Can't open the .HDR file.
		delete [] filename_buffer;
		return false;
	}

	// @@ we're going to assume 16 bits, little endian, so really
	// we're just looking for the lines with NROWS and NCOLS
	// info...
	int	row_count = 0, col_count = 0;

	for (;;)
	{
		// Get a line.
		if (fgets(line_buf, BUF_SIZE, blw_file) == NULL)
		{
			// Read error.
			delete [] filename_buffer;
			fclose(hdr_file);
			return false;
		}

		// Is this one we want?
		if (row_count == 0 && strstr(line_buf, "NROWS"))
		{
			row_count = atof(line_buf + 5);
		}
		else if (col_count == 0 && strstr(line_buf, "NCOLS"))
		{
			col_count = atof(line_buf + 5);
		}
		if (row_count && col_count)
		{
			break;
		}
	}

	fclose(hdr_file);
	
	m_grid.set_size(1, col_count, row_count);

	// Try loading the .BIL file itself, containing the raster data.
	strcpy(filename_buffer, filename);
	strcat(filename_buffer, ".BIL");

	FILE*	bil_file = fopen(filename_buffer, "r");
	if (bil_file == NULL)
	{
		// Can't open the .BIL file.
		delete [] filename_buffer;
		return false;
	}

	int	byte_count = 2 * row_count * col_count;
	unsigned char*	input_data = new unsigned char[byte_count];
	if (fread(input_data, 1, byte_count, bil_file) != byte_count)
	{
		// Data read error.
		fclose(bil_file);
		delete [] input_data;
		delete [] filename_buffer;
		return false;
	}

	// Unpack the data and cram it into our float-grid.
	for (int y = 0; y < row_count; y++)
	{
		unsigned char*	row_start = input_data + 2 * col_count * y;
		for (int x = 0; x < col_count; x++)
		{
			int	value = row_start[x * 2] + (row_start[x * 2 + 1] << 8);
			// sign-extend (watch out for 64-bit architectures...)
			if (value & 0x08000)
			{
				value |= ~0x7FFF;
			}
			m_grid.set(0, x, y, float(value));
		}
	}
}


float	heightfield::get_height(float x, float z);
// My usual terrain coordinate system has x running from west to
// east, and z running from north to south.  Thus, y is up, and
// the x-y-z frame is right-handed.
//
// Return the height at the given coordinates; bilerp in between
// samples.
{
	assert(m_grid.get_layer_count() > 0);
	assert(m_grid.get_width() > 1);
	assert(m_grid.get_height() > 1);
	
	int	ix = floorf(x);
	int	iy = floorf(y);
	float	fx = x - ix;
	float	fy = y - iy;

	// Clamp to edge values.
	if (ix < 0)
	{
		ix = 0;
		fx = 0;
	}
	if (ix >= m_grid.get_width() - 1)
	{
		ix = m_grid.get_width() - 2;
		fx = 1;
	}

	if (iy < 0)
	{
		iy = 0;
		fy = 0;
	}
	if (iy >= m_grid.get_height() - 1)
	{
		iy = m_grid.get_height() - 2;
		fy = 1;
	}

	// bilerp.
	float	s00 = m_grid.get(0, ix + 0, iy + 0);
	float	s10 = m_grid.get(0, ix + 1, iy + 0);
	float	s01 = m_grid.get(0, ix + 0, iy + 1);
	float	s11 = m_grid.get(0, ix + 1, iy + 1);

	return
		s00 * (1 - fx) * (1 - fy)
		+ s10 * (fx) * (1 - fy)
		+ s01 * (1 - fx) * (fy)
		+ s11 * (fx) * (fy);
}


#endif // HEIGHTFIELD_H
