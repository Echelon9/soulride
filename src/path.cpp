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
// path.cpp	-thatcher 12/10/1998 Copyright Slingshot

// Code for Path class, for interpolated path following.


#include <stdio.h>

#include "utility.hpp"
#include "error.hpp"
#include "path.hpp"


Path::Path()
// Constructor.
{
	// Load a path.
	FILE*	fp = Utility::FileOpen("PerfTestPath.dat", "rb");
	if (fp == NULL) {
		Error e; e << "Can't load PerfTestPath.dat";
		throw e;
	}

	PointCount = Read32(fp);

	// Allocate points array.
	Points = new Path::Point[PointCount];

	// Load points.
	int	i;
	for (i = 0; i < PointCount; i++) {
		Points[i].Location.Set(0, ReadFloat(fp));
		Points[i].Location.Set(1, ReadFloat(fp));
		Points[i].Location.Set(2, ReadFloat(fp));
		Points[i].Orientation.Set(0, ReadFloat(fp));
		Points[i].Orientation.Set(1, ReadFloat(fp));
		Points[i].Orientation.Set(2, ReadFloat(fp));
		Points[i].Orientation.Set(3, ReadFloat(fp));
//		fread(&Points[i].Matrix, sizeof(matrix), 1, fp);
	}

	fclose(fp);
}


Path::~Path()
// Free allocated components.
{
	delete [] Points;
}


int	Path::GetPointCount()
// Returns the number of points in the path.
{
	return PointCount;
}


Path::Point*	Path::GetPoint(int index)
// Returns a pointer to the index'th point in the path.
{
	index %= PointCount;
	return &Points[index];
}


void	Path::GetInterpolatedPoint(Path::Point* result, float Index)
// Returns a point along the path.  Interpolates between the discrete
// points, based on the fractional part of the given Index.  The Index
// can range from 0 to PointCount.
{
	int	i = int(Index);
	float	f = Index - i;
	i %= PointCount;
	int	j = (i + 1) % PointCount;

	result->Location = Points[i].Location * (1 - f) + Points[j].Location * f;
	result->Orientation = Points[i].Orientation.lerp(Points[j].Orientation, f);
//	result->Matrix = Points[i].Matrix;
}

