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
// TerrainMesh.cpp	-thatcher 6/14/1999 Copyright Slingshot Game Technology

// Code for representing the terrain as an optimized mesh.
// Uses a quadtree structure, with the triangles defined implicitly.


#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#include "ogl.hpp"
#include "utility.hpp"
#include "text.hpp"
#include "terrain.hpp"
#include "clip.hpp"
#include "config.hpp"
#include "psdread.hpp"
#include "game.hpp"
#include "console.hpp"
#include "lua.hpp"


const float	ROOT_2 = 1.414213562f;


const int	MAX_QSQUARE_HEAP_SIZE = (1 << 20) * 32;	// 32 megs.
const int	QSQUARE_HEAP_BLOCK_SIZE = 100000;	// ~100K per block.
const int	MAX_QSQUARE_HEAP_BLOCKS = MAX_QSQUARE_HEAP_SIZE / QSQUARE_HEAP_BLOCK_SIZE;



inline int	iabs(int i)
// Returns absolute value of i.
{
	return i < 0 ? -i : i;
}


namespace TerrainMesh {
;


// Mapping from uint16 heights in quadtree to altitudes in meters.
const float	HEIGHT_TO_METERS_FACTOR = (1.0 / 16.0);
const int	YMINMAX_TO_METERS_SHIFT = 4;	// 8 - log2(HEIGHT_TO_METERS_FACTOR)


float	GetHeightToMetersFactor()
// Returns the height-to-meters factor.  For tricking the boarder shadow
// texgen projection planes.
{
	return HEIGHT_TO_METERS_FACTOR;
}


// Aux utility function.
float	CheckRayAgainstBilinearPatch(float xorg, float zorg, float size, uint16 Y[4], const vec3& point, const vec3& dir)
// Returns the distance along the ray specified by (point, dir) at which
// the ray intersects the bilinear patch with upper-left corner at
// (xorg,zorg), dimension (size) and corner heights in Y[].
// Returns -1 if there's no intersection.
{
	// Convert inputs.
	float	h00 = Y[1] * HEIGHT_TO_METERS_FACTOR;
	float	h01 = Y[0] * HEIGHT_TO_METERS_FACTOR;
	float	h10 = Y[2] * HEIGHT_TO_METERS_FACTOR;
	float	h11 = Y[3] * HEIGHT_TO_METERS_FACTOR;

#if 0
	//xxxxxxx
	if (fabs(dir.Y()) < 0.0000001) return false;
	float	avgy = (h00 + h01 + h10 + h11) * 0.25;
	float	u = (avgy - point.Y()) / dir.Y();
	if (u >= 0) {
		float	s = (point.X() + dir.X() * u - xorg) / size;
		float	t = (point.Z() + dir.Z() * u - zorg) / size;

		if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
			// Valid intersection.
			return true;
		}
	}
	return false;
	//xxxxxxxx
#endif // 0

#if 0
	// Plane tests.
	if (fabs(dir.Y()) < 0.0000001) return false;
	{
		// Check triangle h00, h10, h11
		float	u = -(h00 + ((h10 - h00)/size*(point.Z() - zorg)) + ((h11-h10)/size*(point.X() - xorg)) - point.Y()) /
			    (((h10 - h00)/size*dir.Z()) + ((h11-h10)/size*dir.X()) - dir.Y());
		float	s, t;
		if (u >= 0) {
			s = (point.X() + dir.X() * u - xorg) / size;
			t = (point.Z() + dir.Z() * u - zorg) / size;
			
			if (s >= 0 && s <= 1 && t >= s && t <= 1) return u;
		}

		// Check triangle h00, h01, h11
		u = -(h00 + ((h11 - h01)/size*(point.Z() - zorg)) + ((h01-h00)/size*(point.X() - xorg)) - point.Y()) /
			    (((h11 - h01)/size*dir.Z()) + ((h01-h00)/size*dir.X()) - dir.Y());
		if (u >= 0) {
			s = (point.X() + dir.X() * u - xorg) / size;
			t = (point.Z() + dir.Z() * u - zorg) / size;
			
			if (s >= t && s <= 1 && t >= 0 && t <= 1) return u;
		}
	}
	return -1;
#endif // 0
	
//	// xxx eliminate square term.
//	if (fabs(h11 - h00) > fabs(h01 - h10)) {
//		h01 = (h00 + h11) / 2;
//		h10 = h01;
//	} else {
//		h00 = (h01 + h10) / 2;
//		h11 = h00;
//	}

	// Basically the solution is to express the difference between the height of the patch
	// and the height of the ray as a function of the ray parameter u, which is a
	// quadratic equation.  If the equation has roots >= 0, then the line crosses the patch.
	// Then we test to see if those solutions are within the patch boundaries.
	
	// Intermediate terms.
	float	slopex = (h01 - h00) / size;
	float	slopez = (h10 - h00) / size;
	float	slopexz = (h00 - h01 - h10 + h11) / (size * size);
	
	// Quadratic formula terms:
	float	c = h00 + slopex * (point.X() - xorg) + slopez * (point.Z() - zorg) +
		    slopexz * (point.X() - xorg) * (point.Z() - zorg) - point.Y();
	float	b = slopex * dir.X() + slopez * dir.Z() +
		    slopexz * ((point.Z() - zorg) * dir.X() + (point.X() - xorg) * dir.Z()) - dir.Y();
	float	a = slopexz * dir.X() * dir.Z();
	
	// Now find roots.
	float	u0, u1;
	if (fabs(a) > 0.0000001) {
		float	d = b * b - 4 * a * c;
		if (d < 0) {
			// No real roots.
			return -1;
		}
		
		d = sqrtf(d);
		u0 = (-b - d) / (2 * a);
		u1 = (-b + d) / (2 * a);
		
	} else if (fabs(b) > 0.0000001) {
		// Degenerates to linear equation.
		u0 = -c / b;
		u1 = -1;
		
	} else {
		// Technically should check to see if c == 0, and if so, make sure ray
		// passes through patch square.
		return -1;
	}

	// Test roots to see if they're >= 0 and within the patch box.
	if (u0 >= 0) {
		float	s = (point.X() + dir.X() * u0 - xorg) / size;
		float	t = (point.Z() + dir.Z() * u0 - zorg) / size;

		if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
			// Valid intersection.
			return u0;
		}
	}
	if (u1 >= 0) {
		float	s = (point.X() + dir.X() * u1 - xorg) / size;
		float	t = (point.Z() + dir.Z() * u1 - zorg) / size;

		if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
			// Valid intersection.
			return u1;
		}
	}

	return -1;
}


// For accounting.
int	TriangleCount = 0;
int	NodesEnabled = 0;
int	NodesActive = 0;


//
// Detail control.
//


int	CurrentFrameNumber = -1;
int	ViewX, ViewY, ViewZ;
Plane	Frustum[6];	// For culling.


// Culling box, for detail and shadow passes.
int	CullBoxX, CullBoxZ;
int	CullBoxExtent;


const int	DETAIL_SIZE_SHIFT = 4;	// controls scaling of the detail texture map.  meters == 1 << shift
const int	DETAIL_DISTANCE_THRESHOLD = 100;	// How far out to do detail mapping (in meters)


// Master detail parameter.
// approx == screen width / max tolerable error (in pixels)
// (fov and other things affect actual pixel error)
float	DetailFactor = 128;	// Smaller --> more error allowed
float	LastDetailFactor = DetailFactor;
float	DetailNudge = 1.0;	// Smaller --> more error allowed.
float	LastNudge = DetailNudge;


#if 0


int	ThresholdDistance[256];
uint8	ErrorTable[8192];


void	InitVertexTestLUTs()
// Initialize some look-up tables used in vertex tests.
{
	int	i;

	for (i = 0; i < 8192; i++) {
		ErrorTable[i] = iclamp(0, frnd(28.299 * log(i+1)), 255);	// Maps 8191 --> 255, 0 --> 0
	}
	
	for (i = 0; i < 256; i++) {
		ThresholdDistance[i] = DetailFactor * DetailNudge * exp(i / 28.299) * HEIGHT_TO_METERS_FACTOR;
	}
}


bool	VertexTest(int x, int z, uint8 error)
// Returns true if the vertex at x,z with the specified error value
// should be enabled.  Returns false if it should be disabled.
{
	// Use simple box distance.
	int	dx = iabs(ViewX - x);
	int	dz = iabs(ViewZ - z);
	int	d = dx;
	if (dz > d) d = dz;

	return d < ThresholdDistance[error];
}


bool	BoxTest(int x, int z, int HalfBoxSize, uint8 error)
// Returns true if the closest distance between the viewpoint and the
// box centered at (x,z) with edge size 2 * HalfBoxSize, is within our
// threshold for specified given error.
// In other words, should the box be subdivided.
{
	int	dx = iabs(ViewX - x);
	int	dz = iabs(ViewZ - z);
	int	d = dx;
	if (dz > d) d = dz;
	d -= HalfBoxSize;

	return d < ThresholdDistance[error];
}


uint8	ErrorFromDelta(uint16 delta)
// Returns an 8-bit error value, given the 16-bit fixed-point delta Y value.
{
	if (delta > 8191) delta = 8191;	// Saturation; corresponds to about 256 meters.
	return ErrorTable[delta];
}


#else


//float	ThresholdDistance2[256];	// LUT to map from ADIndex values to actual distance^2 values.
int	ThresholdDistance[256];	// LUT to map from ADIndex values to actual distance^2 values.


void	InitVertexTestLUTs()
// Initialize some look-up tables used in vertex tests.
{
	int	i;

	for (i = 0; i < 256; i++) {
		ThresholdDistance[i] = frnd(expf(i / 23.734658f) * DetailNudge);	// Map 46341 --> 255, 0 --> 0
//		ThresholdDistance2[i] = powf(ThresholdDistance[i], 2);	// Square it, for the benefit of VertexTest()'s comparison.
	}
}


uint8	ADToIndex(float ad)
// Given an activation distance in meters, returns an 8-bit index value, which
// can be used later to retrieve the approximate activation distance.
{
	return iclamp(0, frnd(23.734658f * logf(ad + 1)), 255);	// 46340 --> 255, 0 --> 0
}


float	ActivationDistance(int error)
// Given a vertex error value, computes the maximum distance at which it
// should be activated.
{
	return (error * HEIGHT_TO_METERS_FACTOR) * DetailFactor;
}


bool	BoxTest(int x, int z, int HalfBoxSize, uint8 error)
// Returns true if the closest distance between the viewpoint and the
// box centered at (x,z) with edge size 2 * HalfBoxSize, is within our
// threshold for specified given error.
// In other words, should the box be subdivided.
{
	int	dx = iabs(ViewX - x);
	int	dz = iabs(ViewZ - z);
	int	d = dx;
	if (dz > d) d = dz;
	d -= HalfBoxSize;

	return d < ThresholdDistance[error];
}


bool	VertexTest(int x, int z, uint8 error)
// Returns true if the vertex at x,z with the specified error value
// should be enabled.  Returns false if it should be disabled.
{
	// Octagonal approximation to circle.
	int	dx = iabs(ViewX - x);
	int	dz = iabs(ViewZ - z);
	int	d;
	if (dz > dx) {
		d = (dz + (((dx<<3)-dx) >> 4));	
	} else {
		d = (dx + (((dz<<3)-dz) >> 4));
	}

	return d < ThresholdDistance[error];
	
//	float	dx = ViewX - x;
//	float	dz = ViewZ - z;
//	float	d = dx * dx + dz * dz;
//
//	return d < ThresholdDistance2[error];
}


#endif


// Texture aux code.


const int	TSHIFT = Surface::TEXTURE_BITS_PER_METER;
const int	TBITS = Surface::TEXTURE_BITS;


int	Log2(int in)
// Quickie log2 function, using a look-up table.
{
	static const int	TABLE_SIZE = 2048;
	static uint8 LogTable[TABLE_SIZE];
	static bool	Inited = false;

	if (!Inited) {
		// Init the look-up table.
		int	i;
		for (i = 0; i < TABLE_SIZE; i++) {
			int	log = 0;
			unsigned int	j = i;
			while (j) {
				log++;
				j >>= 1;
			}
			LogTable[i] = log;
		}

		Inited = true;
	}

	return LogTable[iclamp(0, in, TABLE_SIZE-1)];
}


int	TextureDetailDivisor = 6;


bool	ComputeDesiredBlockScaleLevel(int* result, int x, int z, int level)
// Computes the minimum desired texture scale level for the block specified by x, z, level and
// stores it in *result.
// If the desired level is uniform across the block, returns true, else returns
// false.
{
	int	size = 1 << level;

	if (ViewX >= x && ViewX <= x + size && ViewZ >= z && ViewZ <= z + size) {
		*result = Surface::TEXTURE_BITS - Surface::TEXTURE_BITS_PER_METER;
		return false;
	}
	
	int	dx = x - ViewX;
	if (dx < 0) {
		dx = -dx - size;
	}
	int	dz = z - ViewZ;
	if (dz < 0) {
		dz = -dz - size;
	}

	int	d0, d1;
	if (dz > dx) {
		d0 = ViewZ - z;
		d1 = ViewZ - (z + size);
	} else {
		d0 = ViewX - x;
		d1 = ViewX - (x + size);
	}
	d0 = abs(d0);
	d1 = abs(d1);
	if (d0 > d1) {
		int	temp = d1;
		d1 = d0;
		d0 = temp;
	}

	int	scale0 = Log2((d0 / TextureDetailDivisor) >> (TBITS - TSHIFT)) + (TBITS - TSHIFT);
	int	scale1 = Log2((d1 / TextureDetailDivisor) >> (TBITS - TSHIFT)) + (TBITS - TSHIFT);

	*result = scale0;

	if (scale0 == scale1) return true;
	else return false;
}


::Render::Texture*	CurrentSurfaceTexture = NULL;


void	RequestAndSetTexture(int reqx, int reqz, int reqscale, int x, int z, int scale /*, int* uvshift, float* uvscale, float* uoffset, float* voffset*/)
// Requests from the surface module and sets as current a texture
// covering the specified square (reqz, reqz, reqscale).  The actual
// square of interest is (x, z, scale).
// This function also initializes *uvshift, *uoffset, and *voffset,
// based on the actual texture that's set and the actual square
// of interest.
{
	int	ActualX = reqx + 32768;
	int	ActualZ = reqz + 32768;
	int	ActualLevel = reqscale;

/* For showing the texture tiling.
	//xxxxxxx
	Render::SetTexture(NULL);
	Render::CommitRenderState();
	int	color = ((reqx >> reqscale) ^ (reqz >> reqscale)) & 1;
	glColor3f(((reqscale & 3) + 1) / 4.0, color, color);
	return;
	//xxxxxxx
*/
	
	::Render::Texture*	new_texture = Surface::GetSurfaceTexture(&ActualX, &ActualZ, &ActualLevel, CurrentFrameNumber);

	if (new_texture == CurrentSurfaceTexture) {
		// No need to change textures -- current texture is already the one we want.
		return;
	}
	
	CurrentSurfaceTexture = new_texture;

	ActualX -= 32768;
	ActualZ -= 32768;
	
	::Render::SetTexture(CurrentSurfaceTexture);
	::Render::CommitRenderState();

	// Set up automatic texture-coordinate generation.
	// Basically we're just stretching the current texture over the desired block, leaving a half-texel fringe around the outside.
	float	scalefactor = float(Surface::TEXTURE_SIZE - 1) / float(Surface::TEXTURE_SIZE << ActualLevel);
	float	halftexel = 0.5f / float(Surface::TEXTURE_SIZE);
	
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	float	p[4] = { scalefactor, 0, 0, -ActualX * scalefactor + halftexel };
	glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
	
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	p[0] = 0;	p[2] = scalefactor;	p[3] = -ActualZ * scalefactor + halftexel;
	glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
}


//const static float	CornerUV[4] = { 1.0, 0.0, 0.0, 1.0 };
//const static float	EdgeUV[4] = { 1.0, 0.5, 0.0, 0.5 };


namespace trilist {

	const int	MAX_VERTS = 340;
	float	Vertex[MAX_VERTS * 3];
	uint32	VertexList[MAX_VERTS * 3];

	int	BaseVert = 0;
	int	NextVert = 0;
	int	NextTri = 0;
	
	void	Flush();

	void	Start()
	// Bracket trilist:: rendering calls with Start()/End().  Enables gl array
	// client state, etc.
	{
		// Tell GL about the vertex array.
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Vertex);
	}

	void	Stop()
	// Call when done with a session of trilist:: functions.  By
	// "session", I mean a series of trilist:: calls not broken up
	// by other OpenGL rendering calls.
	{
		Flush();

		// Tell GL we're done with the vertex array.
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	void	Flush()
	// Call when done with a batch -- force triangles to be
	// rendered.  Can change texture or whatever before next call
	// to trilist::AddVertices.
	{
		if (NextTri) {
			glDrawElements(GL_TRIANGLES, NextTri * 3, GL_UNSIGNED_INT, VertexList);

			NextTri = 0;
			NextVert = 0;
			BaseVert = 0;
		}
	}

	void	ReserveVertices(int count)
	// Call to allocate some more vertices in the triangle list.
	// Vertex indices refer to the vertex in the new group.  So if you
	// reserve 3 vertices, you can set them using indices 0, 1, 2, and
	// draw a triangle by doing AddTriangle(0, 1, 2).
	{
		// Make sure we have room.  If not, then flush the batch so far and start another batch.
		if (count + NextVert >= MAX_VERTS) {
			Flush();
		}

		BaseVert = NextVert;
		NextVert = BaseVert + count;
	}

	void	SetVertex(int index, float x, float y, float z)
	// Sets the value of a reserved vertex.
	{
		int	i = (BaseVert + index) * 3;
		Vertex[i] = x;
		Vertex[i+1] = y;
		Vertex[i+2] = z;
	}

	void	AddTriangle(int index0, int index1, int index2)
	// Adds a triangle to the list of tris to be rendered.
	{
		int	i = NextTri * 3;

		VertexList[i] = BaseVert + index0;
		VertexList[i+1] = BaseVert + index1;
		VertexList[i+2] = BaseVert + index2;
		
		NextTri++;
	}
}; // end namespace trilist


//
// Triangle patterns for different qsquare cases.
//
// PatternOffset[flags] takes the enabled flags byte of a qsquare and
// returns an offset index into the PatternData array.  The data in
// the PatternData array is composed of strings.  Each string has a
// vertex count, a triangle count, and then a set of vertex indices,
// one per triangle.  The two missing vertex indices are implicit;
// given a vertex index i, the triangle is (0, i, i+1).
//


int	PatternOffset[256] = {
	0,6,6,13,6,13,13,21,
	6,13,13,21,13,21,21,30,
	40,40,40,40,46,46,46,46,
	46,46,46,46,53,53,53,53,
	61,67,61,67,61,67,61,67,
	74,81,74,81,74,81,74,81,
	89,89,89,89,89,89,89,89,
	40,40,40,40,40,40,40,40,
	94,100,100,107,94,100,100,107,
	94,100,100,107,94,100,100,107,
	115,115,115,115,115,115,115,115,
	115,115,115,115,115,115,115,115,
	121,94,121,94,121,94,121,94,
	121,94,121,94,121,94,121,94,
	126,126,126,126,126,126,126,126,
	126,126,126,126,126,126,126,126,
	0,0,6,6,6,6,13,13,
	0,0,6,6,6,6,13,13,
	130,130,130,130,0,0,0,0,
	130,130,130,130,0,0,0,0,
	61,61,61,61,61,61,61,61,
	61,61,61,61,61,61,61,61,
	135,135,135,135,135,135,135,135,
	135,135,135,135,135,135,135,135,
	130,130,0,0,130,130,0,0,
	130,130,0,0,130,130,0,0,
	135,135,135,135,135,135,135,135,
	135,135,135,135,135,135,135,135,
	135,135,135,135,135,135,135,135,
	135,135,135,135,135,135,135,135,
	139,139,139,139,139,139,139,139,
	139,139,139,139,139,139,139,139,
	};
uint8	PatternData[141] = {
	6,4,1,2,3,4,
	7,5,1,2,3,4,5,
	8,6,1,2,3,4,5,6,
	9,7,1,2,3,4,5,6,7,
	10,8,1,2,3,4,5,6,7,8,
	7,4,1,3,4,5,
	8,5,1,3,4,5,6,
	9,6,1,3,4,5,6,7,
	7,4,1,2,4,5,
	8,5,1,2,3,5,6,
	8,5,1,2,4,5,6,
	9,6,1,2,3,5,6,7,
	6,3,1,3,4,
	7,4,1,2,3,5,
	8,5,1,2,3,4,6,
	9,6,1,2,3,4,5,7,
	8,4,1,3,4,6,
	6,3,1,2,4,
	5,2,1,3,
	5,3,1,2,3,
	4,2,1,2,
	0,0,
};


/* Here's the Perl script which generated the above data.

#!/usr/bin/perl -w

# Little program to help generate a triangle mesh table for quadtree node rendering.


use strict;


my $i;
my %pattern_hash;
my @pattern_list;
my $pattern_count = 0;
my @pattern_offset;


for ($i = 0; $i < 256; $i++) {
    my $pat = pattern_string($i);

    my $index = $pattern_hash{$pat};

    if (! defined($index)) {
	# Add a new pattern.
	$index = $pattern_count++;
	$pattern_hash{$pat} = $index;
	$pattern_list[$index] = $pat;
    }

#    print "$index,";
#    if ((($i+1) & 7) == 0) { print "
"; } else { print " "; }
}


# Compute pattern offsets.
my $offset = 0;
for ($i = 0; $i < $pattern_count; $i++) {
    $pattern_offset[$i] = $offset;

    # Increment offset by the number of elements in the pattern.
    my @temp = split(/,/, $pattern_list[$i]);
    $offset += scalar(@temp);
}
my $pattern_table_size = $offset;

# Write the pattern offsets.
print "uint8\tPatternOffset[256] = {
\t";
for ($i = 0; $i < 256; $i++) {
    my $offset = $pattern_offset[$pattern_hash{pattern_string($i)}];
    print "$offset,";

    if ((($i+1) & 7) == 0) { print "
\t"; }
}
print "};
";


# Write out the pattern table.
print "uint8\tPatternData[$pattern_table_size] = {
";
foreach $i (@pattern_list) {
    print "\t$i
";
}
print "};
";


sub pattern_string {
    # Generate pattern string, based on enabled flags.  Comma separated numbers.
    # return value is a string:
    # First value is the number of vertices in the mesh.
    # Second value is the number of triangles.
    # Remaining values are the vertex indices of the middle corner of each
    # triangle.  Given a vert_index, each triangle is always (0, vert_index, vert_index+1).

    my ($flags) = @_;

    # Take care of a special case.
    my $children = $flags & 0xF0;
    if ($children == 0xF0) {
	# All children enabled; nothing left for our mesh.
	return "0,0,";
    }

    # Fix up the flags, to make sure that if a child node is enabled,
    # the supporting edge verts are also enabled.
    if ($flags & 16) { $flags |= 3; }
    if ($flags & 32) { $flags |= 6; }
    if ($flags & 64) { $flags |= 12; }
    if ($flags & 128) { $flags |= 9; }

    # More fixups.  Clear edge flags if both neighboring child nodes
    # are enabled.
    if (($children & 0x30) == 0x30) { $flags &= ~2; }
    if (($children & 0x60) == 0x60) { $flags &= ~4; }
    if (($children & 0xC0) == 0xC0) { $flags &= ~8; }
    if (($children & 0x90) == 0x90) { $flags &= ~1; }

    my $result = '';
    my $index = 1;
    my $tricount = 0;

    my $vert = 1;

    # Make a bit map of the enabled verts.
    my @explode = ( 0, 1, 4, 5, 16, 17, 20, 21, 64, 65, 68, 69, 80, 81, 84, 85 );
    my $processed_children = ($children >> 4) ^ 15;
    $processed_children = (($processed_children << 1) & 15) | ($processed_children >> 3);
    my $vert_flags = ($explode[$processed_children]) | ($explode[$flags & 15] << 1);

    for ($vert = 1; $vert < 10; $vert++) {
	my $mask = 1 << (($vert-1) & 7);
	my $next_mask = 1 << ($vert & 7);

	if ($vert_flags & $mask) {
	    if ($vert == 9 ||
		(($vert & 1) == 0 && ($vert_flags & $next_mask) == 0))
	    {
		# dont emit;
	    } else {
		# emit a triangle.
		$result = $result . $index . ",";
		$tricount++;
	    }

	    $index++;
	}
    }

    return "$index,$tricount," . $result;
}

 */




float	VertexArray[9 * 3];
uint32	VertList[24];

static void	InitVert(int index, float x, float y, float z)
{
	int	i = index * 3;
	VertexArray[i] = x;
	VertexArray[i+1] = y;
	VertexArray[i+2] = z;
}


//
// qsquare stuff (the main meshing/rendering algorithms)
//


static int	LoadingTickCounter = 0;


struct qsquare;


struct quadcornerdata {
	const quadcornerdata*	Parent;
	qsquare*	Square;
	int	ChildIndex;
	int	Level;
	int	xorg, zorg;
	uint16	Y[4];

	static float	NodeDistance(const quadcornerdata& a, const quadcornerdata& b) {
		float	ahalf = float(1 << a.Level);
		float	bhalf = float(1 << b.Level);
		float	x = (a.xorg + ahalf) - (b.xorg + bhalf);
		float	y = 0 /* a.Square->Y[0] - b.Square->Y[0] */;
		float	z = (a.zorg + ahalf) - (b.zorg + bhalf);

		return sqrtf(x * x + y * y + z * z);
	}
};


struct qsquare {
	qsquare*	ChildLink[4];	// ne, nw, sw, se
	uint16	Y[5];	// center, e, n, w, s
	uint8	EnabledFlags;	// bits 0-7: e, n, w, s, ne, nw, sw, se
	uint8	CountAndFlags;	// bit 7: static flag
				// bit 6: culled flag
				// bits 0-2: east edge children enabled ct
				// bits 3-5: south edge children enabled ct
	uint8	ADIndex[6];	// e, s, ne, nw, sw, se
	uint8	YMin, YMax;	// Bounds for frustum culling.  (YMin << YMINMAX_TO_METERS_SHIFT) == meters

	qsquare(quadcornerdata* q)
	// Constructor.
	{
		q->Square = this;
		
		EnabledFlags = 0;
		CountAndFlags = 0;

		// Interpolate the verts.
		Y[0] = int(q->Y[0] + q->Y[1] + q->Y[2] + q->Y[3]) >> 2;
		Y[1] = int(q->Y[3] + q->Y[0]) >> 1;
		Y[2] = int(q->Y[0] + q->Y[1]) >> 1;
		Y[3] = int(q->Y[1] + q->Y[2]) >> 1;
		Y[4] = int(q->Y[2] + q->Y[3]) >> 1;

		ADIndex[0] = 0;
		ADIndex[1] = 0;

		float	quarter = (1 << q->Level) * 0.5f;
		
		// Compute child activation distance.
		int	i;
		for (i = 0; i < 4; i++) {
			ChildLink[i] = 0;
			int	TessY = (Y[0] + q->Y[i]) >> 1;	// Height of triangle edge passing through center of square.
			ADIndex[2 + i] = ADToIndex(fmax(quarter * ROOT_2, ActivationDistance(iabs(((Y[0] + q->Y[i] + Y[i+1] + Y[1 + ((i+1)&3)]) >> 2) - TessY))));
		}

		// Compute YMin and YMax.
		int	min = 1000000;
		int	max = -1000000;

		// Look at component verts.
		for (i = 0; i < 4; i++) {
			if (q->Y[i] < min) min = q->Y[i];
			if (q->Y[i] > max) max = q->Y[i];
		}
		for (i = 0; i < 5; i++) {
			if (Y[i] < min) min = Y[i];
			if (Y[i] > max) max = Y[i];
		}

		// Convert to 8-bit, by dropping low 8 bits and rounding conservatively.
		YMax = iclamp(0, (max + 255) >> 8, 255);
		YMin = iclamp(0, min >> 8, 255);
	}


	int	Load(const quadcornerdata& cd, FILE* fp)
	// Loads the data for this square, and for descendants as well
	// if the data is in the file.  Creates the descendants as
	// needed.
	// Returns a check code based on a hash of contents.
	{
		int	checkcode = 0;
		
		if ((LoadingTickCounter++ & 0x01FFF) == 0) Game::LoadingTick();
		
		CountAndFlags = 0x80;	// Mark as static & clear counts and flags.
		EnabledFlags = 0;
		
		// Load center, east, and south heights.
		Y[0] = Read16(fp);
		Y[1] = Read16(fp);
		Y[4] = Read16(fp);

		checkcode += Y[0] + Y[1] + Y[4];
		
#if 0
		Error[0] = ErrorFromDelta(iabs(Y[1] - ((cd.Y[0] + cd.Y[3]) >> 1)));
		Error[1] = ErrorFromDelta(iabs(Y[4] - ((cd.Y[2] + cd.Y[3]) >> 1)));
#endif

		// Query neighbors (if they exist) for north & west edge
		// heights.  If there's no neighbor for an edge, just
		// use the interpolated value from the corners.
		qsquare*	s = GetNeighbor(1, cd);
		if (s) Y[2] = s->Y[4];
		else Y[2] = int(cd.Y[0] + cd.Y[1]) >> 1;

		s = GetNeighbor(2, cd);
		if (s) Y[3] = s->Y[1];
		else Y[3] = int(cd.Y[1] + cd.Y[2]) >> 1;

		// Get child flags.
		uint8	flags = fgetc(fp);

		checkcode += (flags << 4);

		// Recursively load children.
		int	i;
		for (i = 0; i < 4; i++) {
			int	index = i ^ (i < 2 ? 1 : 0);	// Funky indexing to preserve east-west, north-south loading order.

#if 0
			uint8	VertError;
			uint16	TessY = int(Y[0] + cd.Y[index]) >> 1;	// Height of triangle edge passing through center of square.
#endif

			if (flags & (1 << index)) {
				quadcornerdata	q;
				SetupCornerData(&q, cd, index);
				
				if (ChildLink[index] == NULL) {
					// Create.
					ChildLink[index] = new qsquare(&q);
				}
				// Load.
				checkcode += (cd.Level << (index * 4)) ^ ChildLink[index]->Load(q, fp);

#if 0
				VertError = ErrorFromDelta(iabs(ChildLink[index]->Y[0] - TessY));
				uint8	e = ChildLink[index]->GetMaxError();
				if (e > VertError) VertError = e;
#endif
				
			} else {
				if (ChildLink[index]) {
					delete ChildLink[index];
					ChildLink[index] = NULL;
				}

#if 0
				VertError = ErrorFromDelta(iabs(((Y[0] + cd.Y[index] + Y[index+1] + Y[1 + ((index+1)&3)]) >> 2) - TessY));
#endif			
			}
			
#if 0
			Error[2 + index] = VertError;
#endif
		}
			
		// Compute YMin and YMax.
		int	min = 1000000;
		int	max = -1000000;

		// Look at component verts.
		for (i = 0; i < 4; i++) {
			if (cd.Y[i] < min) min = cd.Y[i];
			if (cd.Y[i] > max) max = cd.Y[i];
		}
		for (i = 0; i < 5; i++) {
			if (Y[i] < min) min = Y[i];
			if (Y[i] > max) max = Y[i];
		}

		// Convert to 8-bit, by dropping low 8 bits and rounding conservatively.
		YMax = iclamp(0, (max + 255) >> 8, 255);
		YMin = iclamp(0, min >> 8, 255);
		
		// Also look at any child squares.
		for (i = 0; i < 4; i++) {
			qsquare*	n = ChildLink[i];
			if (n) {
				if (n->YMax > YMax) YMax = n->YMax;
				if (n->YMin < YMin) YMin = n->YMin;
			}
		}

		return checkcode;
	}


	float	ComputeActivationDistance(const quadcornerdata& cd, int CenterError)
	// Computes and returns the maximum distance from the viewpoint
	// to the center of this square, at which this square should be
	// subdivided.  Computes and stores the activation distance for
	// this node's vertices and children as well.
	{
		if ((LoadingTickCounter++ & 0x00FFF) == 0) Game::LoadingTick();
		
		int	i;
		
		float	maxdist = ActivationDistance(CenterError);

		float	half = float(1 << cd.Level);
		float	quarter = half * 0.5f;
		
		// East and South edge verts.
		float	ad;
		for (i = 0; i < 2; i++) {
			int	e = i ?	iabs(Y[4] - ((cd.Y[2] + cd.Y[3]) >> 1)) :
				    iabs(Y[1] - ((cd.Y[3] + cd.Y[0]) >> 1));
			ad = ActivationDistance(e);
			ADIndex[i] = ADToIndex(ad);
			
			ad += half;
			if (ad > maxdist) maxdist = ad;
		}

		if (cd.Parent) {
			cd.Parent->Square->ADIndex[cd.ChildIndex + 2] = ADToIndex(maxdist);
//			cd.Parent->Square->ADIndex[cd.ChildIndex + 2] = 0;
		}
		
		// Crawl back up the tree, through our ancestors, and
		// see if our computed activation radius plus the
		// distance between nodes worsens their effective
		// activation distance.  If so, then update their
		// ADIndex.
		quadcornerdata*	p = (quadcornerdata*) &cd;
		while (p->Parent) {
			ad = maxdist + quadcornerdata::NodeDistance(cd, *p);
			uint8	idx = ADToIndex(ad);
			if (p->Parent->Square->ADIndex[p->ChildIndex + 2] >= idx) {
				// Early out -- this node does not expand parent.
				break;
			}

			p->Parent->Square->ADIndex[p->ChildIndex + 2] = idx;
			
			p = (quadcornerdata*) p->Parent;
		}
		
		float	childcenterdist = quarter * ROOT_2;

		// Children.
		if (cd.Level > 0) {
			for (i = 0; i < 4; i++) {
				ADIndex[i + 2] = 0;
				
				uint16	TessY = (Y[0] + cd.Y[i]) >> 1;	// Height of triangle edge passing through center of square.
				if (ChildLink[i]) {
					int	e = iabs(ChildLink[i]->Y[0] - TessY);
					quadcornerdata	q;
					SetupCornerData(&q, cd, i);
					ad = ChildLink[i]->ComputeActivationDistance(q, e);
				} else {
					// What's the activation distance for a bilinear square, as a function of central error?
					// It must be at least the bounding radius of the square, or more depending on the error.
					// Not sure how accurate this is, but it should work OK.
					int	e = iabs(((Y[0] + cd.Y[i] + Y[i+1] + Y[1 + ((i+1)&3)]) >> 2) - TessY);
//					ad = fmax(ActivationDistance(e), childcenterdist);
					ad = ActivationDistance(e);
					
//					ADIndex[i + 2] = ADToIndex(ad);
					
					// propagate to ancestors...(q, ad);
					quadcornerdata	q;
					SetupCornerData(&q, cd, i);
					quadcornerdata*	p = &q;
					while (p->Parent) {
						float	d = ad + quadcornerdata::NodeDistance(q, *p);
						uint8	idx = ADToIndex(d);
						if (p->Parent->Square->ADIndex[p->ChildIndex + 2] < idx) {
							p->Parent->Square->ADIndex[p->ChildIndex + 2] = idx;
						}
						
						p = (quadcornerdata*) p->Parent;
					}
					
				}
				
//				ADIndex[i + 2] = ADToIndex(ad);
				
				ad += childcenterdist;
				if (ad > maxdist) maxdist = ad;			
			}
		}

		return maxdist;
	}
	

	float	GetHeight(const quadcornerdata& cd, int x, int z, float fx, float fz)
	// Returns the height of the point under (fx, fz).  (x, z) is assumed to be the floor
	// of (fx, fz).
	{
		// See which child node the query should go under.
		int	child = 0;
		int	half = 1 << cd.Level;
		int	xcenter = cd.xorg + half;
		int	zcenter = cd.zorg + half;

		if (x < xcenter) child = 1;
		if (z >= zcenter) {
			child += 2;
			child ^= 1;
		}

		quadcornerdata	q;
		SetupCornerData(&q, cd, child);
		
		if (ChildLink[child] /* && (ChildLink[child]->CountAndFlags & 0x80) */) {	// Child exists, and is static.
			return ChildLink[child]->GetHeight(q, x, z, fx, fz);
		} else {
			// Otherwise, interpolate the heights from the info we have in this node.

			fx = (fx - q.xorg) / half;
			fz = (fz - q.zorg) / half;

			return ((q.Y[1] * (1 - fx) + q.Y[0] * (fx)) * (1 - fz) +
				(q.Y[2] * (1 - fx) + q.Y[3] * (fx)) * fz) * HEIGHT_TO_METERS_FACTOR;
		}
	}

	
	float	CheckForRayHit(const quadcornerdata& cd, const vec3& point, const vec3& dir)
	// Returns true if the given ray intersects any part of the heightfield
	// contained under this node.
	{
		int	half = 1 << cd.Level;
		int	size = 2 << cd.Level;
		
		// Test ray against node bounding sphere.
		float	RadiusSquared = float((YMax - YMin) << (YMINMAX_TO_METERS_SHIFT-1));
		RadiusSquared = RadiusSquared*RadiusSquared + float(size) * float(half);	// xxx Loose approximation!

		// Find the origin of the ray, relative to sphere center.
		vec3	v = (point - vec3(float(cd.xorg + half), float((YMin + YMax) << (YMINMAX_TO_METERS_SHIFT-1)), float(cd.zorg + half)));
	
		// The distance from point to the closest approach to the origin
		// is given by -(v*dir).

		float	t = -(v*dir);
//xxxx		if (t < 0) t = 0;	// Ray only can shoot forward from its starting point.

		vec3	closest = v + dir * t;
		if (closest * closest > RadiusSquared) return -1;

		// OK, so the ray passes through our bounding sphere.  Check children and/or quadrants for
		// more definitive info.
		
		for (int i = 0; i < 4; i++) {
			quadcornerdata	q;
			SetupCornerData(&q, cd, i);
			if (ChildLink[i] && (ChildLink[i]->CountAndFlags & 0x80)) {
				float	u = ChildLink[i]->CheckForRayHit(q, point, dir);
				if (u != -1) return u;
 			} else {
				float	u = CheckRayAgainstBilinearPatch(
					float(q.xorg),
					float(q.zorg),
					float(1 << cd.Level),
					q.Y,
					point,
					dir);
				if (u != -1) return u;
			}
		}

		return -1;
	}
	

	void	SetupCornerData(quadcornerdata* q, const quadcornerdata& cd, int ChildIndex)
	// Fills the given structure with the appropriate corner values for the
	// specified child block, given our own vertex data and our corner
	// vertex data from cd.
	//
	// ChildIndex mapping:
	// +-+-+
	// |1|0|
	// +-+-+
	// |2|3|
	// +-+-+
	//
	// quadcornerdata::Y[] mapping:
	// 1-0
	// | |
	// 2-3
	//
	// qsquare::Y[] mapping:
	// +-2-+
	// | | |
	// 3-0-1
	// | | |
	// +-4-+
	{
		int	half = 1 << cd.Level;
		
		q->Parent = &cd;
		q->Square = ChildLink[ChildIndex];
		q->Level = cd.Level - 1;
		q->ChildIndex = ChildIndex;
		
		switch (ChildIndex) {
		default:
		case 0:
			q->xorg = cd.xorg + half;
			q->zorg = cd.zorg;
			q->Y[0] = cd.Y[0];
			q->Y[1] = Y[2];
			q->Y[2] = Y[0];
			q->Y[3] = Y[1];
			break;
			
		case 1:
			q->xorg = cd.xorg;
			q->zorg = cd.zorg;
			q->Y[0] = Y[2];
			q->Y[1] = cd.Y[1];
			q->Y[2] = Y[3];
			q->Y[3] = Y[0];
			break;
			
		case 2:
			q->xorg = cd.xorg;
			q->zorg = cd.zorg + half;
			q->Y[0] = Y[0];
			q->Y[1] = Y[3];
			q->Y[2] = cd.Y[2];
			q->Y[3] = Y[4];
			break;
			
		case 3:
			q->xorg = cd.xorg + half;
			q->zorg = cd.zorg + half;
			q->Y[0] = Y[1];
			q->Y[1] = Y[0];
			q->Y[2] = Y[4];
			q->Y[3] = cd.Y[3];
			break;
		}	
	}


	void	SetupChildCorners(int index, uint16 CornerY[4], uint16 cy[4])
	// Fills cy[] with the values corresponding to the corners of
	// this square's index'th child quadrant.  CornerY[] should contain
	// this square's corner y values.
	{
		switch (index) {
		case 0:
			cy[0] = CornerY[0];
			cy[1] = Y[2];
			cy[2] = Y[0];
			cy[3] = Y[1];
			break;
		case 1:
			cy[0] = Y[2];
			cy[1] = CornerY[1];
			cy[2] = Y[3];
			cy[3] = Y[0];
			break;
		case 2:
			cy[0] = Y[0];
			cy[1] = Y[3];
			cy[2] = CornerY[2];
			cy[3] = Y[4];
			break;
		case 3:
			cy[0] = Y[1];
			cy[1] = Y[0];
			cy[2] = Y[4];
			cy[3] = CornerY[3];
			break;
		}
	}

	qsquare*	GetNeighbor(int dir, const quadcornerdata& cd)
	// Traverses the tree in search of the qsquare neighboring this
	// square to the specified direction.  The directions are 0-3
	// --> { E, N, W, S }.  Returns NULL if the neighbor doesn't
	// exist.
	{
		// If we don't have a parent, then we don't have a neighbor.
		// (Actually, we could have inter-tree connectivity at this level
		// for connecting separate trees together.)
		if (cd.Parent == 0) return 0;
		
		// Find the parent and the child-index of the square we want to locate or create.
		qsquare*	p = 0;
		
		int	index = cd.ChildIndex ^ 1 ^ ((dir & 1) << 1);
		bool	SameParent = ((dir - cd.ChildIndex) & 2) ? true : false;
		
		if (SameParent) {
			p = cd.Parent->Square;
		} else {
			p = cd.Parent->Square->GetNeighbor(dir, *cd.Parent);
			
			if (p == 0) return 0;
		}
		
		qsquare*	n = p->ChildLink[index];
		
		return n;
	}

	
	void	EnableEdgeVertex(int index, bool IncrementCount, const quadcornerdata& cd)
	// Enable the specified edge vertex.  Indices go { e, n, w, s }.
	// Increments the appropriate reference-count if IncrementCount is true.
	{
		if ((EnabledFlags & (1 << index)) && IncrementCount == false) return;
		
		static const int	Inc[4] = { 1, 0, 0, 8 };

		// Turn on flag and deal with reference count.
		EnabledFlags |= 1 << index;
		if (IncrementCount == true && (index == 0 || index == 3)) {
			CountAndFlags += Inc[index];
		}

		// Now we need to enable the opposite edge vertex of the adjacent square (i.e. the alias vertex).
		
		// This is a little tricky, since the desired neighbor node may not exist, in which
		// case we have to create it, in order to prevent cracks.  Creating it may in turn cause
		// further edge vertices to be enabled, propagating updates through the tree.
		
		// The sticking point is the quadcornerdata list, which
		// conceptually is just a linked list of activation structures.
		// In this function, however, we will introduce branching into
		// the "list", making it in actuality a tree.  This is all kind
		// of obscure and hard to explain in words, but basically what
		// it means is that our implementation has to be properly
		// recursive.
		
		// Travel upwards through the tree, looking for the parent in common with our desired neighbor.
		// Remember the path through the tree, so we can travel down the complementary path to get to the neighbor.
		qsquare*	p = this;
		const quadcornerdata*	pcd = &cd;
		int	ct = 0;
		int	stack[32];
		for (;;) {
			int	ci = pcd->ChildIndex;
			
			if (pcd->Parent == NULL || pcd->Parent->Square == NULL) {
				// Neighbor doesn't exist (it's outside the tree), so there's no alias vertex to enable.
				return;
			}
			p = pcd->Parent->Square;
			pcd = pcd->Parent;
			
			bool	SameParent = ((index - ci) & 2) ? true : false;
			
			ci = ci ^ 1 ^ ((index & 1) << 1);	// Child index of neighbor node.
			
			stack[ct] = ci;
			ct++;
			
			if (SameParent) break;
		}
		
		// Get a pointer to our neighbor (create if necessary), by walking down
		// the quadtree from our shared ancestor.
		p = p->EnableDescendant(ct, stack, *pcd);
	
		// Finally: enable the vertex on the opposite edge of our neighbor, the alias of the original vertex.
		index ^= 2;
		p->EnabledFlags |= (1 << index);
		if (IncrementCount == true && (index == 0 || index == 3)) {
			p->CountAndFlags += Inc[index];
		}
	}
	

	qsquare*	qsquare::EnableDescendant(int count, int path[], const quadcornerdata& cd)
	// This function enables the descendant node 'count' generations below
	// us, located by following the list of child indices in path[].
	// Creates the node if necessary, and returns a pointer to it.
	{
		count--;
		int	ChildIndex = path[count];
		
		if ((EnabledFlags & (16 << ChildIndex)) == 0) {
			EnableChild(ChildIndex, cd);
		}
		
		if (count > 0) {
			quadcornerdata	q;
			SetupCornerData(&q, cd, ChildIndex);
			return ChildLink[ChildIndex]->EnableDescendant(count, path, q);
		} else {
			return ChildLink[ChildIndex];
		}
	}


	void	EnableChild(int index, const quadcornerdata& cd)
	// Enables the child quadrant specified by index.  0-3 --> { ne, nw, sw, se }
	// Creates it if necessary.
	{
		NodesEnabled++;
		
		EnabledFlags |= 16 << index;
		EnableEdgeVertex(index, true, cd);
		EnableEdgeVertex((index + 1) & 3, true, cd);

		if (ChildLink[index] == 0) {
			quadcornerdata	q;
			SetupCornerData(&q, cd, index);
			
			ChildLink[index] = new qsquare(&q);
		}
	}

	
	void	NotifyChildDisable(const quadcornerdata& cd, int index, bool DeleteChild)
	// Marks the indexed child quadrant as disabled.  Deletes the child node
	// if DeleteChild is true.
	{
		// Clear enabled flag for the child.
		EnabledFlags &= ~(16 << index);

		// Update children enabled counts for the affected edge verts.
		qsquare*	s;
		
		if (index & 2) s = this;
		else s = GetNeighbor(1, cd);
		if (s) {
			s->CountAndFlags -= 8;
		}

		if (index == 1 || index == 2) s = GetNeighbor(2, cd);
		else s = this;
		if (s) {
			s->CountAndFlags -= 1;
		}
		
		if (DeleteChild) {
			delete ChildLink[index];
			ChildLink[index] = 0;
		}
	}

	
	void	Update(const quadcornerdata& cd,
		       uint8 ThisADIndex
		      )
	// Enables and disables vertices as necessary, and updates the tree.  May force
	// creation or deletion of qsquares.
	{
		int	half = 1 << cd.Level;
		int	whole = 2 << cd.Level;
		int	quarter = half >> 1;
		int	cx = cd.xorg + half;
		int	cz = cd.zorg + half;
		
		// See about enabling child verts.
		if ((EnabledFlags & 1) == 0 && VertexTest(cd.xorg + whole, cd.zorg + half, ADIndex[0] /*, ViewerLocation */) == true) EnableEdgeVertex(0, false, cd);	// East vert.
		if ((EnabledFlags & 8) == 0 && VertexTest(cd.xorg + half, cd.zorg + whole, ADIndex[1] /*, ViewerLocation */) == true) EnableEdgeVertex(3, false, cd);	// South vert.
		if (cd.Level > 0) {
			if ((EnabledFlags & 32) == 0) {
				if (BoxTest(cx - quarter, cz - quarter, quarter, /*MinY, MaxY,*/ ADIndex[3]/*, ViewerLocation*/) == true) EnableChild(1, cd);	// nw child.
			}
			if ((EnabledFlags & 16) == 0) {
				if (BoxTest(cx + quarter, cz - quarter, quarter, /*MinY, MaxY,*/ ADIndex[2]/*, ViewerLocation*/) == true) EnableChild(0, cd);	// ne child.
			}
			if ((EnabledFlags & 64) == 0) {
				if (BoxTest(cx - quarter, cz + quarter, quarter, /*MinY, MaxY,*/ ADIndex[4]/*, ViewerLocation*/) == true) EnableChild(2, cd);	// sw child.
			}
			if ((EnabledFlags & 128) == 0) {
				if (BoxTest(cx + quarter, cz + quarter, quarter, /*MinY, MaxY,*/ ADIndex[5]/*, ViewerLocation*/) == true) EnableChild(3, cd);	// se child.
			}
		
			// Recurse into child quadrants as necessary.
			quadcornerdata	q;
			
			if (EnabledFlags & 32) {
				NodesActive++;
				SetupCornerData(&q, cd, 1);
				ChildLink[1]->Update(q, /*ViewerLocation,*/ ADIndex[3]);
			}
			if (EnabledFlags & 16) {
				NodesActive++;
				SetupCornerData(&q, cd, 0);
				ChildLink[0]->Update(q, /*ViewerLocation,*/ ADIndex[2]);
			}
			if (EnabledFlags & 64) {
				NodesActive++;
				SetupCornerData(&q, cd, 2);
				ChildLink[2]->Update(q, /*ViewerLocation,*/ ADIndex[4]);
			}
			if (EnabledFlags & 128) {
				NodesActive++;
				SetupCornerData(&q, cd, 3);
				ChildLink[3]->Update(q, /*ViewerLocation,*/ ADIndex[5]);
			}
		}
		
		// Test for disabling.  East, South, and center.
		if ((EnabledFlags & 1) != 0 && (CountAndFlags & 7) == 0 && VertexTest(cd.xorg + whole, cd.zorg + half, ADIndex[0]) == false) {
			EnabledFlags &= 0xFE;
			qsquare*	s = GetNeighbor(0, cd);
			if (s) s->EnabledFlags &= 0xFB;
		}
		if ((EnabledFlags & 8) != 0 && (CountAndFlags & 0x38) == 0 && VertexTest(cd.xorg + half, cd.zorg + whole, ADIndex[1]) == false) {
			EnabledFlags &= 0xF7;
			qsquare*	s = GetNeighbor(3, cd);
			if (s) s->EnabledFlags &= 0xFD;
		}
		if (EnabledFlags == 0 && cd.Parent != 0 && BoxTest(cx, cz, half, ThisADIndex) == false) {
			// Disable ourself.
			cd.Parent->Square->NotifyChildDisable(*cd.Parent, cd.ChildIndex, CountAndFlags & 128 ? false : true);	// nb: possibly deletes 'this'.
		}
	}

	
	enum PassID { TEXTURE, DETAIL, SHADOW /* others? TEXTURE_AND_DETAIL for multitexture HW? */ };

	
	void	Render(const quadcornerdata& cd, const ViewState& s, int ClipHint, bool TextureSet, /* int uvshift, float uvscale, float uoffset, float voffset, */ PassID pass)
	// Draws this square.  Recurses to sub-squares if needed.
	{
		int	i;

		int	HalfSize = 1 << cd.Level;
		int	size = 2 << cd.Level;
		int	cx = cd.xorg + HalfSize;
		int	cz = cd.zorg + HalfSize;

		bool	BoundTexture = false;

		// Check block's visibility.
		if (pass == TEXTURE) {
			if (ClipHint != Clip::NO_CLIP /* && (EnabledFlags & 0xF0) */) {
				float	r2 = float(((YMax - YMin) << (YMINMAX_TO_METERS_SHIFT-1)));
				r2 = r2 * r2 + float(size) * float(HalfSize);
				ClipHint = Clip::ComputeSphereSquaredVisibility(
					vec3(float(cx), float((YMin + YMax) << (YMINMAX_TO_METERS_SHIFT-1)), float(cz)),
					r2,
					Frustum,
					ClipHint);
//				ClipHint |= Clip::ComputeSphereVisibility(vec3(cx, (YMin + YMax) << (YMINMAX_TO_METERS_SHIFT-1), cz), ROOT_2 * HalfSize + ((YMax - YMin) << (YMINMAX_TO_METERS_SHIFT-1)), Frustum, ClipHint);
//				if (ClipHint != Clip::NO_CLIP && ClipHint != Clip::NOT_VISIBLE) {
//					ClipHint = Clip::ComputeBoxVisibility(vec3(cd.xorg, YMin << 4, cd.zorg), vec3(cd.xorg + size, YMax << 4, cd.zorg + size), Frustum/*s*/, ClipHint);
//				}
				if (ClipHint == Clip::NOT_VISIBLE) {
					// Block is outside frustum... no rendering needed.
					CountAndFlags |= 0x40;	// Set the culled flag.
					return;
				}
			}
			CountAndFlags &= ~0x40;	// Clear the culled flag.
		} else {
			// In subsequent passes, just reuse the culled flag set by the first pass.
			if (CountAndFlags & 0x40) {
				// This square was culled on the first pass, so it's not visible.
				return;
			}
		}
		
		int	BlockScale = cd.Level + 1;

		// If we don't have one already, see about a texture for this square.
		if (pass == TEXTURE) {
			if (TextureSet == false) {
				int DesiredScale;
				if (ComputeDesiredBlockScaleLevel(&DesiredScale, cd.xorg, cd.zorg, BlockScale)) {
					if (DesiredScale >= BlockScale) {
//						trilist::Flush();

						int	RequestX = cd.xorg & ~((1 << DesiredScale) - 1);
						int	RequestZ = cd.zorg & ~((1 << DesiredScale) - 1);
						RequestAndSetTexture(RequestX, RequestZ, DesiredScale, cd.xorg, cd.zorg, BlockScale /*, &uvshift, &uvscale, &uoffset, &voffset*/);
						TextureSet = true;
						BoundTexture = true;
					}
				}
			}
		} else if (pass == DETAIL || pass == SHADOW) {
			// See if we can reject this square, by checking it against the culling box.
			int	dx = iabs(CullBoxX - cx) - (HalfSize /* << 1 */);
			int	dz = iabs(CullBoxZ - cz) - (HalfSize /* << 1 */);
			int	d = dx;
			if (dz > d) d = dz;
			
			if (d > CullBoxExtent) return;
		}
		
		// Recurse into subsquares if necessary.
		if (EnabledFlags & 0xF0) {
			quadcornerdata	q;

			int	mask = 16;
			for (i = 0; i < 4; i++) {
				if (EnabledFlags & mask) {
					// Recurse.
					SetupCornerData(&q, cd, i);
					ChildLink[i]->Render(q, s, ClipHint, TextureSet, pass);
				}
				mask <<= 1;
			}
		}
		
		if ((EnabledFlags & 0xF0) == 0xF0) {
			// All four sub-squares have already been covered.
			if (BoundTexture) {
				trilist::Flush();
			}
			return;
		}
		
		//
		// Render the parts of this square that weren't covered by recursion.
		//

		// We definitely need a texture now, if we don't have one already.
		if (pass == TEXTURE && TextureSet == false) {
//			trilist::Flush();

			// Set up the texture for this block.
			int	DesiredScale;
			ComputeDesiredBlockScaleLevel(&DesiredScale, cd.xorg, cd.zorg, BlockScale);
			if (DesiredScale < BlockScale) DesiredScale = BlockScale;
			int	RequestX = cd.xorg & ~((1 << DesiredScale) - 1);
			int	RequestZ = cd.zorg & ~((1 << DesiredScale) - 1);
			RequestAndSetTexture(RequestX, RequestZ, DesiredScale, cd.xorg, cd.zorg, BlockScale /*, &uvshift, &uvscale, &uoffset, &voffset */);
			
			TextureSet = true;
			BoundTexture = true;
		}

#if 0


#if 1
		int	half = HalfSize;
		int	pindex = PatternOffset[EnabledFlags];
		uint8*	pattern = &PatternData[pindex];
		trilist::ReserveVertices(pattern[0]);

		// Tweak the enabled flags, to turn off edge bits
		// which don't belong in this level mesh because
		// they're surrounded by enabled child nodes.
		uint8	flags = EnabledFlags;
		if ((EnabledFlags & 0x90) == 0x90) flags &= ~1;
		if ((EnabledFlags & 0x30) == 0x30) flags &= ~2;
		if ((EnabledFlags & 0x60) == 0x60) flags &= ~4;
		if ((EnabledFlags & 0xC0) == 0xC0) flags &= ~8;

		int	vert = 0;
//		const float	H = HEIGHT_TO_METERS_FACTOR;
		trilist::SetVertex(vert++, cx, Y[0], cz);	// center
		if ((flags & 0x80) == 0) trilist::SetVertex(vert++, cx + half, cd.Y[3], cz + half);	// SE corner
		if (flags & 1) trilist::SetVertex(vert++, cx + half, Y[1], cz);	// E edge
		if ((flags & 0x10) == 0) trilist::SetVertex(vert++, cx + half, cd.Y[0], cz - half);	// NE corner
		if (flags & 2) trilist::SetVertex(vert++, cx, Y[2], cz - half);	// N edge
		if ((flags & 0x20) == 0) trilist::SetVertex(vert++, cx - half, cd.Y[1], cz - half);	// NW corner
		if (flags & 4) trilist::SetVertex(vert++, cx - half, Y[3], cz);	// W edge
		if ((flags & 0x40) == 0) trilist::SetVertex(vert++, cx - half, cd.Y[2], cz + half);	// SW corner
		if (flags & 8) trilist::SetVertex(vert++, cx, Y[4], cz + half);	// S edge
		if ((flags & 0x80) == 0) trilist::SetVertex(vert++, cx + half, cd.Y[3], cz + half);	// repeat SE corner at end.

		// Copy the triangle pattern to the triangle list.
		int	triangle_count = pattern[1];
		uint8*	p = &pattern[2];
		for (i = 0; i < triangle_count; i++) {
			trilist::AddTriangle(0, *p, (*p) + 1);
			p++;
		}
		TriangleCount += triangle_count;

		if (BoundTexture) {
			trilist::Flush();
		}

#else // 0
		
		// Init vertex data.
		int	half = HalfSize;
		int	whole = half << 1;
		InitVert(0, cd.xorg + half, Y[0] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg + half);
		if (EnabledFlags & 1) InitVert(1, cd.xorg + whole, Y[1] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg + half);
		InitVert(2, cd.xorg + whole, cd.Y[0] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg);
		if (EnabledFlags & 2) InitVert(3, cd.xorg + half, Y[2] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg);
		InitVert(4, cd.xorg, cd.Y[1] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg);
		if (EnabledFlags & 4) InitVert(5, cd.xorg, Y[3] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg + half);
		InitVert(6, cd.xorg, cd.Y[2] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg + whole);
		if (EnabledFlags & 8) InitVert(7, cd.xorg + half, Y[4] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg + whole);
		InitVert(8, cd.xorg + whole, cd.Y[3] /* * HEIGHT_TO_METERS_FACTOR */, cd.zorg + whole);
		
		int	vcount = 0;

#if 0
// Local macro to make the triangle logic shorter & hopefully clearer.
#define tri(a,b,c) ( VertList[vcount++] = a, VertList[vcount++] = b, VertList[vcount++] = c )

		// Make the list of triangles to draw.
		if ((EnabledFlags & 1) == 0) tri(0, 8, 2);
		else {
			if ((EnabledFlags & 0x80) == 0) tri(0, 8, 1);
			if ((EnabledFlags & 0x10) == 0) tri(0, 1, 2);
		}
		if ((EnabledFlags & 2) == 0) tri(0, 2, 4);
		else {
			if ((EnabledFlags & 0x10) == 0) tri(0, 2, 3);
			if ((EnabledFlags & 0x20) == 0) tri(0, 3, 4);
		}
		if ((EnabledFlags & 4) == 0) tri(0, 4, 6);
		else {
			if ((EnabledFlags & 0x20) == 0) tri(0, 4, 5);
			if ((EnabledFlags & 0x40) == 0) tri(0, 5, 6);
		}
		if ((EnabledFlags & 8) == 0) tri(0, 6, 8);
		else {
			if ((EnabledFlags & 0x40) == 0) tri(0, 6, 7);
			if ((EnabledFlags & 0x80) == 0) tri(0, 7, 8);
		}
		
		// Draw 'em.
		glDrawElements(GL_TRIANGLES, vcount, GL_UNSIGNED_INT, VertList);

		// Count 'em.
		TriangleCount += vcount / 3;
#else
		// There are three main cases:
		// 0) All squares to be rendered, so it's a continuous fan all the way around.
		// 1) It's a single fan (between 1 and 3 contiguous squares), but there's a gap so it doesn't end with the start vertex.
		// 2) Two squares, diagonally opposite, requiring two separate fans.
		
		int	Case;
		if ((EnabledFlags & 0xF0) == 0) Case = 0;
		else if ((EnabledFlags & 0xF0) == 0x50 || (EnabledFlags & 0xF0) == 0xA0) Case = 2;
		else Case = 1;
		
		if (Case == 0) {
			TriangleCount += 4;	// Minimum number of tris.
			VertList[vcount++] = 0;	// center
			
			for (int i = 0; i < 4; i++) {
				VertList[vcount++] = 2 + (i*2);	// corner
				if (EnabledFlags & (1 << ((i + 1) & 3))) {
					TriangleCount++;	// There's one additional tri for each edge vert enabled.
					VertList[vcount++] = 1 + (((i+1)*2)&7); // edge
				}
			}
			VertList[vcount++] = 2;	// last corner.
			
			// Draw 'em.
			glDrawElements(GL_TRIANGLE_FAN, vcount, GL_UNSIGNED_INT, VertList);
			
		} else if (Case == 1) {
			// Find the start, and draw a single bounded fan.
			static const struct {
				int	StartVert, BlockCount;
			} FanInfo[16] = {
				{ 0, 0 }, { 1, 1 }, { 3, 1 }, { 1, 2 },
				{ 5, 1 }, { 0, 0 }, { 3, 2 }, { 1, 3 },
				{ 7, 1 }, { 7, 2 }, { 0, 0 }, { 7, 3 },
				{ 5, 2 }, { 5, 3 }, { 3, 3 }, { 0, 0 }
			};
			int	f = (EnabledFlags >> 4) ^ 15;

			int	vi = FanInfo[f].StartVert;
			int	ct = FanInfo[f].BlockCount;
			
			VertList[vcount++] = 0;	// center
			VertList[vcount++] = vi;	// initial edge
			
			// Render the blocks.
			int	mask = ((EnabledFlags & 15) | ((EnabledFlags & 15) << 4)) >> (vi >> 1);
			for (i = 0; i < ct; i++) {
				TriangleCount++;
				vi = 1 + (vi & 7);
				VertList[vcount++] = vi;	// corner

				vi = 1 + (vi & 7);
				mask >>= 1;
				if (mask & 1) {
					TriangleCount++;
					VertList[vcount++] = vi;	// edge
				}
			}
			
			// Draw 'em.
			glDrawElements(GL_TRIANGLE_FAN, vcount, GL_UNSIGNED_INT, VertList);
			
		} else {
			// We have two independent sub-squares.
			TriangleCount += 4;	// Always 4 tris, in this case.

			for (i = 0; i < 4; i++) {
				if ((EnabledFlags & (16 << i)) == 0) {
					VertList[vcount++] = 0;	// center
					VertList[vcount++] = i*2 + 1;	// edge
					VertList[vcount++] = i*2 + 2;	// corner
					VertList[vcount++] = ((i*2 + 2)&7) + 1;	// edge

					// Draw 'em.
					glDrawElements(GL_TRIANGLE_FAN, vcount, GL_UNSIGNED_INT, VertList);
				}
			}
		}
#endif
#endif		
		
#else
		
		// There are three main cases:
		// 0) All squares to be rendered, so it's a continuous fan all the way around.
		// 1) It's a single fan (between 1 and 3 contiguous squares), but there's a gap so it doesn't end with the start vertex.
		// 2) Two squares, diagonally opposite, requiring two separate fans.
		
		int	Case;
		if ((EnabledFlags & 0xF0) == 0) Case = 0;
		else if ((EnabledFlags & 0xF0) == 0x50 || (EnabledFlags & 0xF0) == 0xA0) Case = 2;
		else Case = 1;
		
//		::Render::CommitRenderState();

		int	HalfTable[4];
		HalfTable[0] = 1 << cd.Level;
		HalfTable[1] = 0;
		HalfTable[2] = -HalfTable[0];
		HalfTable[3] = 0;
		
		if (Case == 0) {
			FullFan(cd, cx, cz, HalfTable);
		} else if (Case == 1) {
			// Find the start, and draw a single bounded fan.
			static const struct {
				int	StartVert, BlockCount;
			} FanInfo[16] = {
				{ 0, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 },
				{ 2, 1 }, { 0, 0 }, { 1, 2 }, { 0, 3 },
				{ 3, 1 }, { 3, 2 }, { 0, 0 }, { 3, 3 },
				{ 2, 2 }, { 2, 3 }, { 1, 3 }, { 0, 0 }
			};
			int	f = (EnabledFlags >> 4) ^ 15;
			SingleFan(cd, cx, cz, HalfTable, FanInfo[f].StartVert, FanInfo[f].BlockCount);
		} else {
			// We have two independent sub-squares.
			SeparateFans(cd, cx, cz, HalfTable);
		}
#endif
	}


	void	FullFan(const quadcornerdata& cd, int cx, int cz, int HalfTable[4] /*, float uvscale, float uoffset, float voffset*/)
	// Draws the whole square as a single triangle fan that goes all the way around.
	{
		TriangleCount += 4;	// Minimum number of tris.
		
		glBegin(GL_TRIANGLE_FAN);
//		glTexCoord2f(0.5 * uvscale + uoffset, 0.5 * uvscale + voffset);
		glVertex3f(float(cx), float(Y[0]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz));	// center
		
		for (int i = 0; i < 4; i++) {
//			glTexCoord2f(CornerUV[i&3] * uvscale + uoffset, CornerUV[(i+1)&3] * uvscale + voffset);
			glVertex3f(float(cx + HalfTable[(i+1)&2]), float(cd.Y[i&3]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(i+2)&2]));	// corner
			if (EnabledFlags & (1 << ((i + 1) & 3))) {
				TriangleCount++;	// There's one additional tri for each edge vert enabled.
//				glTexCoord2f(EdgeUV[(i+1)&3] * uvscale + uoffset, EdgeUV[(i+2)&3] * uvscale + voffset);
				glVertex3f(float(cx + HalfTable[(i+1)&3]), float(Y[((i+1)&3)+1]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(i+2)&3]));	// edge
			}
		}
		glVertex3f(float(cx + HalfTable[0]), float(cd.Y[0]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[2]));	// last corner
		glEnd();
	}
	
	void	SingleFan(const quadcornerdata& cd, int cx, int cz, int HalfTable[4], int vi, int BlockCount /*, float uvscale, float uoffset, float voffset */)
	// Draws part of the square as a single triangle fan,
	// starting at edge vertex vi and comprising BlockCount blocks.
	{
		int	i;
		
		glBegin(GL_TRIANGLE_FAN);
//		glTexCoord2f(0.5 * uvscale + uoffset, 0.5 * uvscale + voffset);
		glVertex3f(float(cx), float(Y[0]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz));	// center

//		glTexCoord2f(EdgeUV[vi] * uvscale + uoffset, EdgeUV[(vi+1)&3] * uvscale + voffset);
		glVertex3f(float(cx + HalfTable[vi]), float(Y[vi+1]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(vi+1)&3]));	// start w/ edge vertex.

		// Render the blocks.
		int	mask = ((EnabledFlags & 15) | ((EnabledFlags & 15) << 4)) >> vi;
		for (i = 0; i < BlockCount; i++) {
			TriangleCount++;
//			glTexCoord2f(CornerUV[vi&3] * uvscale + uoffset, CornerUV[(vi+1)&3] * uvscale + voffset);
			glVertex3f(float(cx + HalfTable[(vi+1)&2]), float(cd.Y[vi&3]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(vi+2)&2]));	// corner
			
			vi = (vi + 1) & 3;
			mask >>= 1;
			if (mask & 1) {
				TriangleCount++;
//				glTexCoord2f(EdgeUV[vi&3] * uvscale + uoffset, EdgeUV[(vi+1)&3] * uvscale + voffset);
				glVertex3f(float(cx + HalfTable[vi&3]), float(Y[(vi&3)+1]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(vi+1)&3]));	// edge
			}
		}

		glEnd();
	}
	
	void	SeparateFans(const quadcornerdata& cd, int cx, int cz, int HalfTable[4] /*, float uvscale, float uoffset, float voffset*/)
	// Draws two diagonal quadrants of the square as two independent two-triangle fans.
	{
		TriangleCount += 4;	// Always 4 tris, in this case.
		
		int	i;

		for (i = 0; i < 4; i++) {
			if ((EnabledFlags & (16 << i)) == 0) {
				glBegin(GL_TRIANGLE_FAN);
				
//				glTexCoord2f(0.5 * uvscale + uoffset, 0.5 * uvscale + voffset);
				glVertex3f(float(cx), float(Y[0]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz));	// center
				
//				glTexCoord2f(EdgeUV[i] * uvscale + uoffset, EdgeUV[(i+1)&3] * uvscale + voffset);
				glVertex3f(float(cx + HalfTable[i]), float(Y[i+1]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(i+1)&3]));	// edge
				
//				glTexCoord2f(CornerUV[i] * uvscale + uoffset, CornerUV[(i+1)&3] * uvscale + voffset);
				glVertex3f(float(cx + HalfTable[(i+1)&2]), float(cd.Y[i&3]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(i+2)&2]));	// corner

//				glTexCoord2f(EdgeUV[(i+1)&3] * uvscale + uoffset, EdgeUV[(i+2)&3] * uvscale + voffset);
				glVertex3f(float(cx + HalfTable[(i+1)&3]), float(Y[((i+1)&3)+1]) /* * HEIGHT_TO_METERS_FACTOR */, float(cz + HalfTable[(i+2)&3]));	// edge
				
				glEnd();
			}
		}
	}


	//
	// Memory management for qsquares.
	//
	
	static void*	HeapBlock[MAX_QSQUARE_HEAP_BLOCKS];
	static int	HeapBlockCount;
	static int	SquareCount;
	static void**	FreeList;
	
	static void	OpenAllocator()
	// Sets up a private memory management system.  Must be called before qsquare::new or ::delete are called.
	{
		// Clear our array of bulk storage, and initialize the free list.
		int	i;
		for (i = 0; i < MAX_QSQUARE_HEAP_BLOCKS; i++) {
			HeapBlock[i] = NULL;
		}
		HeapBlockCount = 0;
		FreeList = 0;
		SquareCount = 0;
	}

	static void	CloseAllocator()
	// Free our private heap and associated structures.
	{
		DeleteAllSquares();
	}

	static void	DeleteAllSquares()
	// Clear the private qsquare heap.
	{
		int	i;
		for (i = 0; i < HeapBlockCount; i++) {
			delete [] (uint8*) HeapBlock[i];
			HeapBlock[i] = NULL;
		}
		HeapBlockCount = 0;
		SquareCount = 0;
		FreeList = NULL;
	}

	void*	operator new(size_t size)
	// Special allocator for qsquares.  Uses a free-list, created from a
	// set of fairly large block-allocated blocks.  A new block is created
	// when the free-list runs dry, and the storage is linked into the free list.
	{
		if (FreeList) {
			// Pop the next item from the free list.
			void*	storage = FreeList;
			FreeList = (void**) *FreeList;
			SquareCount++;
			
			return storage;
		}

		// Free list is empty.  Create a new block and link each component struct into the free list.

		if (HeapBlockCount >= MAX_QSQUARE_HEAP_BLOCKS) {
			// Plumb out of storage.  Throw an exception.
			::Error e; e << "TerrainMesh::qsquare::new() -- heap is full.";
			throw e;
		}

		int	SquaresInBlock = QSQUARE_HEAP_BLOCK_SIZE / sizeof(qsquare);
		int	BlockBytes = SquaresInBlock * sizeof(qsquare);
//xxx		void*	block = (void*) new uint8[BlockBytes];
		void*	block = (void*) new uint8[BlockBytes + 8];
		HeapBlock[HeapBlockCount++] = block;

		//xxx
		(*(uint32*)(block)) = 0xDEADBEEF;
		(*(uint32*)(((char*)block) + BlockBytes + 4)) = 0xDEADBEEF;
		block = (void*) (((char*) block) + 4);
		//xxx
		
		// Link the storage to the free list.
		int	i;
		for (i = SquaresInBlock-1; i >= 0; i--) {
			void*	b = (void*) (((uint8*) block) + sizeof(qsquare) * i);
			*((void**) b) = FreeList;
			FreeList = (void**) b;
		}

		// Finally, pop the new struct off the newly replenished free list.
		void*	storage = FreeList;
		FreeList = (void**) *FreeList;
		SquareCount++;

		return storage;
	}
		

	void	operator delete(void* o, size_t s)
	// Frees the given qsquare from the private heap.
	{
		// Push the block onto the free list.
		*((void**) o) = FreeList;	// The first dword in the block serves as the free list link.
		FreeList = (void**) o;

		SquareCount--;
	}
};


void*	qsquare::HeapBlock[MAX_QSQUARE_HEAP_BLOCKS];
int	qsquare::HeapBlockCount = 0;
int	qsquare::SquareCount = 0;
void**	qsquare::FreeList = NULL;


static void	HeapInfo_lua()
// Log a bunch of info about the qsquare heap.
{
	Console::Printf("HeapBlockCount = %d, MQHBLOCKS = %d\n", qsquare::HeapBlockCount, MAX_QSQUARE_HEAP_BLOCKS);
	Console::Printf("SquareCount = %d, QHBSIZE = %d\n", qsquare::SquareCount, QSQUARE_HEAP_BLOCK_SIZE);
	Console::Printf("sizeof(qsquare) = %d\n", sizeof(qsquare));
}


qsquare*	Root = 0;
quadcornerdata	RootCornerData = { NULL, NULL, 0, 15, -32768, -32768, { 0, 0, 0, 0 } };


void	Open()
// Initializes the module.
{
	lua_register("qsquare_heap_info", HeapInfo_lua);//xxxxxxx
	
	InitVertexTestLUTs();
	qsquare::OpenAllocator();
}


void	Clear()
// Free everything.  To change to a new terrain, issue a Clear()
// followed by an Update(), after the terrain model has been changed..
{
	qsquare::DeleteAllSquares();
	Root = 0;
}


void	Close()
// Free everything.
{
	Clear();
	qsquare::CloseAllocator();
}


int	Load(FILE* fp)
// Loads quadtree data from the given file and initializes our quadtree
// with it.
// Returns a check code, based on some hash of the file contents.
{
	int	checkcode = 0;
	
	Clear();
	
	Root = new qsquare(&RootCornerData);
	RootCornerData.Square = Root;

	checkcode = Root->Load(RootCornerData, fp);
	Root->ComputeActivationDistance(RootCornerData, Root->Y[0]);

	return checkcode;
}


void	Update(const ViewState& s)
// Enables/disables vertices in the mesh, so that it's rendered with the
// optimal number of triangles.
{
	CurrentFrameNumber = s.FrameNumber;
	ViewX = frnd(s.Viewpoint.X());
	ViewY = frnd(s.Viewpoint.Y());
	ViewZ = frnd(s.Viewpoint.Z());

//	DetailFactor = powf(10, fclamp(1, (Config::GetFloatValue("TerrainMeshSlider") - 1) / 9 * 2 + 1, 3));	// From 10 to 1000.
	DetailNudge = fclamp(0.333f, (Config::GetFloat("TerrainMeshSlider") - 1) / 10 + 0.5f, 3);
	if (DetailNudge != LastNudge) {
		InitVertexTestLUTs();
	}
	LastNudge = DetailNudge;
	
	// Update tree.
	NodesActive = 1;
	Root->Update(RootCornerData, 0);
}


int	GetRenderedTriangleCount()
// Returns the number of triangles in the terrain passed to OpenGL in the last frame.
{
	return TriangleCount;
}


int	GetNodesTotalCount()
// Returns the number of new nodes created this frame.
{
//	return NodesEnabled;
	return qsquare::SquareCount;
}


int	GetNodesActiveCount()
// Returns the number of currently active nodes.
{
	return NodesActive;
}


float	GetDetailNudge()
// Returns the detail nudge factor.  1.0 == nominal detail; Smaller == less detail,
// greater == more detail.
{
	return DetailNudge;
}


::Render::Texture*	Detail = NULL;


void	Render(const ViewState& s)
// Draw the terrain mesh.
{
	// Do some accounting.
	static int	LastFrame = -1;
	if (LastFrame != s.FrameNumber) {
		LastFrame = s.FrameNumber;
		TriangleCount = 0;
		NodesEnabled = 0;

		//xxxxxxx
		// show qsquare heap guard words.
		int	i;
		char	buf[80];
		int	SquaresInBlock = QSQUARE_HEAP_BLOCK_SIZE / sizeof(qsquare);
		int	BlockBytes = SquaresInBlock * sizeof(qsquare);
		for (i = 0; i < qsquare::HeapBlockCount; i++) {
			uint32	val0 = *((uint32*)qsquare::HeapBlock[i]);
			uint32	val1 = *((uint32*)(((char*)qsquare::HeapBlock[i]) + BlockBytes + 4));
			if (val0 != 0xDEADBEEF || val1 != 0xDEADBEEF) {
				sprintf(buf, "%d: x%X x%X", i, val0, val1);
				Text::DrawString(630, 200, Text::FIXEDSYS, Text::ALIGN_RIGHT, buf);
			}
		}
		//xxxxxxx
	}
	
	if (Root == 0) return;

//	// Compute the DetailFactor, based on the "TerrainMeshSlider" value.
//	DetailFactor = powf(10, fclamp(1, (Config::GetFloatValue("TerrainMeshSlider") - 1) / 9 * 2 + 1, 3));	// From 10 to 1000.
	if (fabs(DetailFactor - LastDetailFactor) / DetailFactor > 0.01) {
		// Recalculate activation distances.
		Root->ComputeActivationDistance(RootCornerData, Root->Y[0]);
		InitVertexTestLUTs();
		
		LastDetailFactor = DetailFactor;
	}

	TextureDetailDivisor = iclamp(1, Config::GetIntValue("TextureDetailSlider"), 10);
	
	
	// Make sure we have a detail texture.
	if (Config::GetBoolValue("F4Pressed")) {
		// Reload detail map.
		delete Detail;
		Detail = NULL;
	}
	if (Detail == NULL) {
		Detail = ::Render::NewTexture("detail.psd", false, true, true, true);
	}

	//xxxxxx
	glColor3f(1, 1, 1);
	::Render::SetTexture(NULL);
	::Render::DisableLightmapBlend();
	::Render::CommitRenderState();

	// Tweak the modelview matrix so that our fixed-point Y coords get scaled by 1/16.  Then we can just pass
	// integers to glVertex3i().  This could be stupid; need to test.
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(1, HEIGHT_TO_METERS_FACTOR, 1);
	
	// Enable automatic texture-coord generation, for faster vertex specs.
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	
//	// Set up vertex array.
//	glEnableClientState(GL_VERTEX_ARRAY);
//	glVertexPointer(3, GL_FLOAT, 0, VertexArray);
	trilist::Start();
	
	// Render.

	// Set up culling frustum in terrain-space.
	int	i;
	for (i = 0; i < 6; i++) {
		Frustum[i] = s.ClipPlane[i];
	}
	Clip::TransformFrustum(Frustum, s.ViewMatrix);
	
	// First pass: surface texture.
	bool	TexSet = Config::GetBool("NoTerrainTexture");//xxx
	Root->Render(RootCornerData, s, 0, TexSet, qsquare::TEXTURE);

	trilist::Flush();

	// Second pass: detail texture.
	bool	DoDetail = Config::GetBoolValue("DetailMapping");
	if (DoDetail) {
		glDisable(GL_FOG);
		::Render::SetTexture(Detail);
		::Render::EnableLightmapBlend();
		::Render::CommitRenderState();

		// Detail mapping is applied as a uniform tile over the whole terrain.
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		float	scalefactor = 1.0 / (1 << DETAIL_SIZE_SHIFT);
		float	p[4] = { scalefactor, 0, 0, -(ViewX & ~((1 << DETAIL_SIZE_SHIFT) - 1)) * scalefactor};
		glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
		
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		p[0] = 0;	p[2] = scalefactor;	p[3] = -(ViewZ & ~((1 << DETAIL_SIZE_SHIFT) - 1)) * scalefactor;
		glTexGenfv(GL_T, GL_OBJECT_PLANE, p);

		// Only draw geometry near the viewpoint.
		CullBoxX = ViewX;
		CullBoxZ = ViewZ;
		CullBoxExtent = DETAIL_DISTANCE_THRESHOLD;
		
		Root->Render(RootCornerData, s, 0, false, qsquare::DETAIL);

		::Render::DisableLightmapBlend();
		if (Config::GetBoolValue("Fog")) glEnable(GL_FOG);
	}
	
//	// Disable the vertex array.
//	glDisableClientState(GL_VERTEX_ARRAY);
	trilist::Stop();

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	// Restore original modelview matrix.
	glPopMatrix();
}


void	RenderShadow(const ViewState& s, const vec3& ShadowCenter, float ShadowExtent)
// Draw a shadow pass over the terrain mesh.  Only draws geometry inside the
// box centered at ShadowCenter and extending for ShadowExtent units
// to either direction in the x and z axes.  Caller should set up
// OpenGL tex-gen parameters to create the proper mapping.
{
	if (Root == 0) return;

	// Tweak the modelview matrix so that our fixed-point Y coords get scaled by 1/16.  Then we can just pass
	// integers to glVertex3i().  This could be stupid; need to test.
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(1, HEIGHT_TO_METERS_FACTOR, 1);
	
	trilist::Start();

	// Set up culling box.
	CullBoxX = frnd(ShadowCenter.X());
	CullBoxZ = frnd(ShadowCenter.Z());
	CullBoxExtent = frnd(ShadowExtent + 0.5f);
	
	// Render.

	// Special pass: shadow.
	Root->Render(RootCornerData, s, 0, false, qsquare::SHADOW);

	trilist::Stop();
	
	// Restore original modelview matrix.
	glPopMatrix();
}


float	GetHeight(float x, float z)
// Returns the height of the terrain at the specified x, z location,
// using the info stored in the quadtree.
{
	if (Root == 0) return 0;

	return Root->GetHeight(RootCornerData, fchop(x), fchop(z), x, z);
}


float	CheckForRayHit(const vec3& point, const vec3& dir)
// Checks the given ray to see if it touches the terrain.  If so,
// returns the distance to the hit; -1 otherwise.
{
	if (Root == 0) return -1;

	return Root->CheckForRayHit(RootCornerData, point, dir);
}


};	// end namespace TerrainMesh




#ifdef NOT


#include <stdio.h>


int	main()
{
	printf("sizeof(Square) = %d\n", sizeof(TerrainMesh::qsquare));

	return 0;
}


#endif // NOT
