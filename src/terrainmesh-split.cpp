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
		
		d = sqrt(d);
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


//float	ThresholdDistance2[256];	// LUT to map from ADIndex values to actual distance^2 values.
int	ThresholdDistance[256];	// LUT to map from ADIndex values to actual distance^2 values.


void	InitVertexTestLUTs()
// Initialize some look-up tables used in vertex tests.
{
	int	i;

	for (i = 0; i < 256; i++) {
		ThresholdDistance[i] = frnd(exp(i / 23.734658) * DetailNudge);	// Map 46341 --> 255, 0 --> 0
//		ThresholdDistance2[i] = powf(ThresholdDistance[i], 2);	// Square it, for the benefit of VertexTest()'s comparison.
	}
}


uint8	ADToIndex(float ad)
// Given an activation distance in meters, returns an 8-bit index value, which
// can be used later to retrieve the approximate activation distance.
{
	return iclamp(0, frnd(23.734658 * log(ad + 1)), 255);	// 46340 --> 255, 0 --> 0
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


Render::Texture*	CurrentSurfaceTexture = NULL;


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
	
	CurrentSurfaceTexture = Surface::GetSurfaceTexture(&ActualX, &ActualZ, &ActualLevel, CurrentFrameNumber);

	ActualX -= 32768;
	ActualZ -= 32768;
	
	Render::SetTexture(CurrentSurfaceTexture);
	Render::CommitRenderState();

//	*uvscale = float(Surface::TEXTURE_SIZE - 1) / float(Surface::TEXTURE_SIZE);
//	*uvshift = ActualLevel - scale;
//	*uoffset = 0.5 / float(Surface::TEXTURE_SIZE);
//	*voffset = *uoffset;
//	
//	if (*uvshift) {
//		int	DeltaX = (x & ((1 << ActualLevel) - 1)) >> scale;
//		int	DeltaZ = (z & ((1 << ActualLevel) - 1)) >> scale;
//
//		*uvscale /= float(1 << *uvshift);
//		*uoffset += DeltaX * *uvscale;
//		*voffset += DeltaZ * *uvscale;
//	}

	// Set up automatic texture-coordinate generation.
	// Basically we're just stretching the current texture over the desired block, leaving a half-texel fringe around the outside.
	float	scalefactor = float(Surface::TEXTURE_SIZE - 1) / float(Surface::TEXTURE_SIZE << ActualLevel);
	float	halftexel = 0.5 / float(Surface::TEXTURE_SIZE);
	
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	float	p[4] = { scalefactor, 0, 0, -ActualX * scalefactor + halftexel };
	glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
	
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	p[0] = 0;	p[2] = scalefactor;	p[3] = -ActualZ * scalefactor + halftexel;
	glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
}


const static float	CornerUV[4] = { 1.0, 0.0, 0.0, 1.0 };
const static float	EdgeUV[4] = { 1.0, 0.5, 0.0, 0.5 };


//
// qsquare stuff (the main meshing/rendering algorithms)
//


int	VertexArray[9 * 3];
unsigned int	ColorArray[9];
unsigned int	VertList[24];

static void	InitVert(int index, int x, int y, int z)
{
	int	i = index * 3;
	VertexArray[i] = x;
	VertexArray[i+1] = y;
	VertexArray[i+2] = z;
}


static int	LoadingTickCounter = 0;


//
// storage nodes.
//

struct snode_header;


struct scornerdata {
	const scornerdata*	Parent;
	snode_header*	PackedNode;
	uint16 NodeY[5];	// unpacked node heights: center, e, n, w, s
	int	ChildIndex;
	int	Level;
	int	xorg, zorg;
	uint16	Y[4];	// Corners.
};


struct snode_header {
	uint8	ADIndex[3];	// activation distance of center, e, s
	uint8	flags;	// bits 0-3: children 0-3 present/not-present
			// bits 4-5: size of y values:
			//   0 == uint16-packed -- 4 bits for center, 3 bits each for e, n, w, s, deltas.
			//   1 == uint8[5] -- 8 bits each, delta.
			//   2 == uint16[5] -- 16 bits each, absolute.
			//   3 == reserved.  (escape flag... for paging etc)
			// bits 6-7: size of sibling offset:
			//   0 == no offset (last sibling, or sibling follows directly).  14 bytes
			//   1 == 1 byte offset.                                          15 bytes
			//   2 == 2 byte offset.                                          16 bytes
			//   3 == 4 byte offset.                                          18 bytes

	// Y values.  center, e, n, w, s.  either uint16-packed, int8[5] or uint16[5].
	// (2 bytes, 5 bytes, or 10 bytes)

	// sibling offset (0, 1, 2 or 4 bytes)

	
	snode_header*	GetChild(int ChildIndex)
	// Returns a pointer to the specified child.  Returns NULL if
	// no such child exists.
	{
		int	request_mask = 1 << ChildIndex;
		if ((flags & request_mask) == 0) return NULL;

		int	skip_count = bit_count[(request_mask - 1) & flags];
		int	offset = snode_size[flags >> 4];

		snode_header*	s = (snode_header*) (((char*) this) + offset);
		while (skip_count--) {
			s = (snode*) (((char*) s) + s->GetSiblingOffset());
		}

		return s;
	}
	
	int	GetSiblingOffset()
	// Returns the offset from the start of this node, to the
	// start of our next sibling.
	{
		// offset == our size, plus sibling offset.
		int	size = snode_size[flags >> 4];
		int	off_code = flags & 0xC0;
		int	offset;
		// Different cases, in order of likelihood.
		if (off_code == 0x00) {
			// No extra offset.  Next node follows immediately after this one.
			return size;
		if (off_code == 0x40) {
			// 1 byte offset.
			return size + *( ((char*)this) + size - 1);
		} else if (off_code == 0x80) {
			// 2 byte offset.
			return size + *((uint16*) (((char*)this) + size - 2));
		} else {
			// 4 byte offset.
			return size + *((uint32*) (((char*)this) + size - 4));
		}
	}
	
	void	UnpackHeights(uint16 NodeYOut[5], uint16 CornerY[4])
	// Decodes this node's height values into the given NodeYOut[]
	// array.  Some encodings require the CornerY[] information.
	{
		int	enc_type = flags & 0x30;

		// Encoding options, in order of likelihood.
		if (enc_type == 0x00) {
			// uint16-packed: deltas, 4 bits for center, 3 bits each for e, n, w, s.
			InterpolateHeights(NodeYOut, CornerY);
			NodeYOut[0] += sign extended 4 bits;
			etc;
		} else if (enc_type == 0x10) {
			// int8[5]: deltas, 8 bits each.
			InterpolateHeights(NodeYOut, CornerY);
			int8*	delta = ((int8*) this) + 4;
			NodeYOut[0] += delta[0];
			NodeYOut[1] += delta[1];
			NodeYOut[2] += delta[2];
			NodeYOut[3] += delta[3];
			NodeYOut[4] += delta[4];
		} else {
			// uint16[5]: absolute.  Just do a straight copy.
			memcpy(NodeYOut, ((int8*) this) + 4, sizeof(uint16) * 5);
		}
	}
};


//
// rendering nodes.
//

struct qsquare;


struct quadcornerdata {
	const quadcornerdata*	Parent;
	qsquare*	Square;
	int	ChildIndex;
	int	Level;
	int	xorg, zorg;
	uint16	Y[4];

	static float	NodeDistance(const quadcornerdata& a, const quadcornerdata& b) {
		float	ahalf = (1 << a.Level);
		float	bhalf = (1 << b.Level);
		float	x = (a.xorg + ahalf) - (b.xorg + bhalf);
		float	y = 0 /* a.Square->Y[0] - b.Square->Y[0] */;
		float	z = (a.zorg + ahalf) - (b.zorg + bhalf);

		return sqrt(x * x + y * y + z * z);
	}
};


struct qsquare {
	snode_header*	snode;
	uint16	ChildLink[4];	// ne, nw, sw, se
	uint16	Y[5];	// center, e, n, w, s
	uint8	EnabledFlags;	// bits 0-7: e, n, w, s, ne, nw, sw, se
	uint8	CountAndFlags;	// bit 7: reserved
				// bit 6: culled flag
				// bits 0-2: east edge children enabled ct
				// bits 3-5: south edge children enabled ct
	uint8	ADIndex[6];	// e, s, ne, nw, sw, se
	uint8	YMin, YMax;	// Bounds for frustum culling.  (YMin << YMINMAX_TO_METERS_SHIFT) == meters

	qsquare(quadcornerdata* q, snode_header* s)
	// Constructor.
	{
		q->Square = this;

		snode = s;
		
		EnabledFlags = 0;
		CountAndFlags = 0;

		if (snode) {
			// Get vertex heights from storage node.
			snode->UnpackHeights(Y, q->Y);
			ADIndex[0] = snode->ADIndex[1];
			ADIndex[1] = snode->ADIndex[2];
		} else {
			// Interpolate the vert heights from the given corners.
			Y[0] = int(q->Y[0] + q->Y[1] + q->Y[2] + q->Y[3]) >> 2;
			Y[1] = int(q->Y[3] + q->Y[0]) >> 1;
			Y[2] = int(q->Y[0] + q->Y[1]) >> 1;
			Y[3] = int(q->Y[1] + q->Y[2]) >> 1;
			Y[4] = int(q->Y[2] + q->Y[3]) >> 1;

			ADIndex[0] = 0;
			ADIndex[1] = 0;
		}

		int	compute_child_ad = 15;
		if (snode) {
			// Don't compute activation distance for any child which has an snode.
			compute_child_ad &= ~(snode->flags & 15);
		}
		
		float	quarter = (1 << q->Level) * 0.5;
		
		// Compute or retrieve child activation distance.
		int	i;
		for (i = 0; i < 4; i++) {
			ChildLink[i] = 0;
			if (compute_child_ad & 1) {
				int	TessY = (Y[0] + q->Y[i]) >> 1;	// Height of triangle edge passing through center of square.
				ADIndex[2 + i] = ADToIndex(fmax(quarter * ROOT_2, ActivationDistance(iabs(((Y[0] + q->Y[i] + Y[i+1] + Y[1 + ((i+1)&3)]) >> 2) - TessY))));
			} else {
				// Retrieve child AD from snode data.
				ADIndex[2 + i] = snode->GetChild(i)->ADIndex[0];
			}
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

		if (q.Parent) q.Parent->ExpandMinMax(YMin, YMax);
	}

#ifdef NOT
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

			if (flags & (1 << index)) {
				quadcornerdata	q;
				SetupCornerData(&q, cd, index);
				
				if (ChildLink[index] == NULL) {
					// Create.
					ChildLink[index] = new qsquare(&q);
				}
				// Load.
				checkcode += (cd.Level << (index * 4)) ^ ChildLink[index]->Load(q, fp);

			} else {
				if (ChildLink[index]) {
					delete ChildLink[index];
					ChildLink[index] = NULL;
				}

			}
			
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
#endif // NOT


	float	ComputeActivationDistance(const quadcornerdata& cd, int CenterError)
	// Computes and returns the maximum distance from the viewpoint
	// to the center of this square, at which this square should be
	// subdivided.  Computes and stores the activation distance for
	// this node's vertices and children as well.
	{
		if ((LoadingTickCounter++ & 0x00FFF) == 0) Game::LoadingTick();
		
		int	i;
		
		float	maxdist = ActivationDistance(CenterError);

		float	half = (1 << cd.Level);
		float	quarter = half * 0.5;
		
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
			if (p->Parent->Square->ADIndex[p->ChildIndex + 2] < idx) {
				p->Parent->Square->ADIndex[p->ChildIndex + 2] = idx;
			}
			
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
		float	RadiusSquared = ((YMax - YMin) << (YMINMAX_TO_METERS_SHIFT-1));
		RadiusSquared = RadiusSquared*RadiusSquared + float(size) * float(half);	// xxx Loose approximation!

		// Find the origin of the ray, relative to sphere center.
		vec3	v = (point - vec3(cd.xorg + half, (YMin + YMax) << (YMINMAX_TO_METERS_SHIFT-1), cd.zorg + half));
	
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
//				if (u >= 0) return u;
				if (u != -1) return u;
 			} else {
				float	u = CheckRayAgainstBilinearPatch(q.xorg, q.zorg, 1 << cd.Level, q.Y, point, dir);
//				if (u >= 0) return u;
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


#if USE_NSS
	uint8	GetMaxError()
	// Returns the maximum error value of this block's contained vertices and children.
	{
		uint8	e = 0;
		for (int i = 0; i < 6; i++) {
			if (Error[i] > e) e = Error[i];
		}
		return e;
	}
#endif
	
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
#if USE_NSS
		if ((EnabledFlags & 1) == 0 && VertexTest(cd.xorg + whole, /*Y[1],*/ cd.zorg + half, Error[0] /*, ViewerLocation */) == true) EnableEdgeVertex(0, false, cd);	// East vert.
		if ((EnabledFlags & 8) == 0 && VertexTest(cd.xorg + half, /*Y[4],*/ cd.zorg + whole, Error[1] /*, ViewerLocation */) == true) EnableEdgeVertex(3, false, cd);	// South vert.
#else
		if ((EnabledFlags & 1) == 0 && VertexTest(cd.xorg + whole, cd.zorg + half, ADIndex[0] /*, ViewerLocation */) == true) EnableEdgeVertex(0, false, cd);	// East vert.
		if ((EnabledFlags & 8) == 0 && VertexTest(cd.xorg + half, cd.zorg + whole, ADIndex[1] /*, ViewerLocation */) == true) EnableEdgeVertex(3, false, cd);	// South vert.
#endif
		if (cd.Level > 0) {
			if ((EnabledFlags & 32) == 0) {
#if !USE_NSS
				if (BoxTest(cx - quarter, cz - quarter, quarter, /*MinY, MaxY,*/ ADIndex[3]/*, ViewerLocation*/) == true) EnableChild(1, cd);	// nw child.
#else
				if (VertexTest(cx - quarter, cz - quarter, ADIndex[3]) == true) EnableChild(1, cd);	// nw child.
#endif
			}
			if ((EnabledFlags & 16) == 0) {
#if !USE_NSS
				if (BoxTest(cx + quarter, cz - quarter, quarter, /*MinY, MaxY,*/ ADIndex[2]/*, ViewerLocation*/) == true) EnableChild(0, cd);	// ne child.
#else
				if (VertexTest(cx + quarter, cz - quarter, ADIndex[2]) == true) EnableChild(0, cd);	// ne child.
#endif     
			}
			if ((EnabledFlags & 64) == 0) {
#if !USE_NSS
				if (BoxTest(cx - quarter, cz + quarter, quarter, /*MinY, MaxY,*/ ADIndex[4]/*, ViewerLocation*/) == true) EnableChild(2, cd);	// sw child.
#else
				if (VertexTest(cx - quarter, cz + quarter, ADIndex[4]) == true) EnableChild(2, cd);	// sw child.
#endif
			}
			if ((EnabledFlags & 128) == 0) {
#if !USE_NSS
				if (BoxTest(cx + quarter, cz + quarter, quarter, /*MinY, MaxY,*/ ADIndex[5]/*, ViewerLocation*/) == true) EnableChild(3, cd);	// se child.
#else
				if (VertexTest(cx + quarter, cz + quarter, ADIndex[5]) == true) EnableChild(3, cd);	// sw child.
#endif
			}
		
			// Recurse into child quadrants as necessary.
			quadcornerdata	q;
			
			if (EnabledFlags & 32) {
				NodesActive++;
				SetupCornerData(&q, cd, 1);
#if !USE_NSS
				ChildLink[1]->Update(q, /*ViewerLocation,*/ ADIndex[3]);
#else
				ChildLink[1]->Update(q, /*ViewerLocation,*/ ADIndex[3]);
#endif
			}
			if (EnabledFlags & 16) {
				NodesActive++;
				SetupCornerData(&q, cd, 0);
#if !USE_NSS
				ChildLink[0]->Update(q, /*ViewerLocation,*/ ADIndex[2]);
#else
				ChildLink[0]->Update(q, /*ViewerLocation,*/ ADIndex[2]);
#endif
			}
			if (EnabledFlags & 64) {
				NodesActive++;
				SetupCornerData(&q, cd, 2);
#if !USE_NSS
				ChildLink[2]->Update(q, /*ViewerLocation,*/ ADIndex[4]);
#else
				ChildLink[2]->Update(q, /*ViewerLocation,*/ ADIndex[4]);
#endif
			}
			if (EnabledFlags & 128) {
				NodesActive++;
				SetupCornerData(&q, cd, 3);
#if !USE_NSS
				ChildLink[3]->Update(q, /*ViewerLocation,*/ ADIndex[5]);
#else
				ChildLink[3]->Update(q, /*ViewerLocation,*/ ADIndex[5]);
#endif
			}
		}
		
		// Test for disabling.  East, South, and center.
#if !USE_NSS
		if ((EnabledFlags & 1) != 0 && (CountAndFlags & 7) == 0 && VertexTest(cd.xorg + whole, cd.zorg + half, ADIndex[0]) == false) {
#else
		if ((EnabledFlags & 1) != 0 && (CountAndFlags & 7) == 0 && VertexTest(cd.xorg + whole, cd.zorg + half, ADIndex[0]) == false) {
#endif
			EnabledFlags &= 0xFE;
			qsquare*	s = GetNeighbor(0, cd);
			if (s) s->EnabledFlags &= 0xFB;
		}
#if !USE_NSS
		if ((EnabledFlags & 8) != 0 && (CountAndFlags & 0x38) == 0 && VertexTest(cd.xorg + half, cd.zorg + whole, ADIndex[1]) == false) {
#else
		if ((EnabledFlags & 8) != 0 && (CountAndFlags & 0x38) == 0 && VertexTest(cd.xorg + half, cd.zorg + whole, ADIndex[1]) == false) {
#endif
			EnabledFlags &= 0xF7;
			qsquare*	s = GetNeighbor(3, cd);
			if (s) s->EnabledFlags &= 0xFD;
		}
#if !USE_NSS
		if (EnabledFlags == 0 && cd.Parent != 0 && BoxTest(cx, cz, half, ThisADIndex) == false) {
#else
		if (EnabledFlags == 0 && cd.Parent != 0 && VertexTest(cx, cz, ThisADIndex) == false) {
#endif
			// Disable ourself.
			cd.Parent->Square->NotifyChildDisable(*cd.Parent, cd.ChildIndex, CountAndFlags & 128 ? false : true);	// nb: possibly deletes 'this'.
		}
	}

	
	enum PassID { TEXTURE, DETAIL, SHADOW /* others? TEXTURE_AND_DETAIL for multitexture HW? */ };

	
	void	Render(const quadcornerdata& cd, const ViewState& s, int ClipHint, bool TextureSet, /* int uvshift, float uvscale, float uoffset, float voffset, */ PassID pass)
	// Draws this square.  Recurses to sub-squares if needed.
	{
		int	i, j;

		int	HalfSize = 1 << cd.Level;
		int	size = 2 << cd.Level;
		int	cx = cd.xorg + HalfSize;
		int	cz = cd.zorg + HalfSize;

		// Check block's visibility.
		if (pass == TEXTURE) {
			if (ClipHint != Clip::NO_CLIP /* && (EnabledFlags & 0xF0) */) {
				float	r2 = ((YMax - YMin) << (YMINMAX_TO_METERS_SHIFT-1));
				r2 = r2 * r2 + float(size) * float(HalfSize);
				ClipHint = Clip::ComputeSphereSquaredVisibility(vec3(cx, (YMin + YMax) << (YMINMAX_TO_METERS_SHIFT-1), cz), r2, Frustum, ClipHint);
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
						int	RequestX = cd.xorg & ~((1 << DesiredScale) - 1);
						int	RequestZ = cd.zorg & ~((1 << DesiredScale) - 1);
						RequestAndSetTexture(RequestX, RequestZ, DesiredScale, cd.xorg, cd.zorg, BlockScale /*, &uvshift, &uvscale, &uoffset, &voffset*/);
						TextureSet = true;
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
			return;
		}
		
		//
		// Render the parts of this square that weren't covered by recursion.
		//

		// We definitely need a texture now, if we don't have one already.
		if (pass == TEXTURE && TextureSet == false) {
			// Set up the texture for this block.
			int	DesiredScale;
			ComputeDesiredBlockScaleLevel(&DesiredScale, cd.xorg, cd.zorg, BlockScale);
			if (DesiredScale < BlockScale) DesiredScale = BlockScale;
			int	RequestX = cd.xorg & ~((1 << DesiredScale) - 1);
			int	RequestZ = cd.zorg & ~((1 << DesiredScale) - 1);
			RequestAndSetTexture(RequestX, RequestZ, DesiredScale, cd.xorg, cd.zorg, BlockScale /*, &uvshift, &uvscale, &uoffset, &voffset */);
			
			TextureSet = true;
		}

#ifdef NOT
		// Init vertex data.
		int	half = HalfSize;
		int	whole = half << 1;
		InitVert(0, cd.xorg + half, Y[0], cd.zorg + half);
		if (EnabledFlags & 1) InitVert(1, cd.xorg + whole, Y[1], cd.zorg + half);
		InitVert(2, cd.xorg + whole, cd.Y[0], cd.zorg);
		if (EnabledFlags & 2) InitVert(3, cd.xorg + half, Y[2], cd.zorg);
		InitVert(4, cd.xorg, cd.Y[1], cd.zorg);
		if (EnabledFlags & 4) InitVert(5, cd.xorg, Y[3], cd.zorg + half);
		InitVert(6, cd.xorg, cd.Y[2], cd.zorg + whole);
		if (EnabledFlags & 8) InitVert(7, cd.xorg + half, Y[4], cd.zorg + whole);
		InitVert(8, cd.xorg + whole, cd.Y[3], cd.zorg + whole);
		
		int	vcount = 0;

#ifdef NOT
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
#endif // NOT
		
		
#else
		
		// There are three main cases:
		// 0) All squares to be rendered, so it's a continuous fan all the way around.
		// 1) It's a single fan (between 1 and 3 contiguous squares), but there's a gap so it doesn't end with the start vertex.
		// 2) Two squares, diagonally opposite, requiring two separate fans.
		
		int	Case;
		if ((EnabledFlags & 0xF0) == 0) Case = 0;
		else if ((EnabledFlags & 0xF0) == 0x50 || (EnabledFlags & 0xF0) == 0xA0) Case = 2;
		else Case = 1;
		
//		Render::CommitRenderState();

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
#endif // NOT
	}


	void	FullFan(const quadcornerdata& cd, int cx, int cz, int HalfTable[4] /*, float uvscale, float uoffset, float voffset*/)
	// Draws the whole square as a single triangle fan that goes all the way around.
	{
		TriangleCount += 4;	// Minimum number of tris.
		
		glBegin(GL_TRIANGLE_FAN);
//		glTexCoord2f(0.5 * uvscale + uoffset, 0.5 * uvscale + voffset);
		glVertex3f(cx, Y[0] * HEIGHT_TO_METERS_FACTOR, cz);	// center
		
		for (int i = 0; i < 4; i++) {
//			glTexCoord2f(CornerUV[i&3] * uvscale + uoffset, CornerUV[(i+1)&3] * uvscale + voffset);
			glVertex3f(cx + HalfTable[(i+1)&2], cd.Y[i&3] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(i+2)&2]);	// corner
			if (EnabledFlags & (1 << ((i + 1) & 3))) {
				TriangleCount++;	// There's one additional tri for each edge vert enabled.
//				glTexCoord2f(EdgeUV[(i+1)&3] * uvscale + uoffset, EdgeUV[(i+2)&3] * uvscale + voffset);
				glVertex3f(cx + HalfTable[(i+1)&3], Y[((i+1)&3)+1] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(i+2)&3]);	// edge
			}
		}
		glVertex3f(cx + HalfTable[0], cd.Y[0] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[2]);	// last corner
		glEnd();
	}
	
	void	SingleFan(const quadcornerdata& cd, int cx, int cz, int HalfTable[4], int vi, int BlockCount /*, float uvscale, float uoffset, float voffset */)
	// Draws part of the square as a single triangle fan,
	// starting at edge vertex vi and comprising BlockCount blocks.
	{
		int	i;
		
		glBegin(GL_TRIANGLE_FAN);
//		glTexCoord2f(0.5 * uvscale + uoffset, 0.5 * uvscale + voffset);
		glVertex3f(cx, Y[0] * HEIGHT_TO_METERS_FACTOR, cz);	// center

//		glTexCoord2f(EdgeUV[vi] * uvscale + uoffset, EdgeUV[(vi+1)&3] * uvscale + voffset);
		glVertex3f(cx + HalfTable[vi], Y[vi+1] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(vi+1)&3]);	// start w/ edge vertex.

		// Render the blocks.
		int	mask = ((EnabledFlags & 15) | ((EnabledFlags & 15) << 4)) >> vi;
		for (i = 0; i < BlockCount; i++) {
			TriangleCount++;
//			glTexCoord2f(CornerUV[vi&3] * uvscale + uoffset, CornerUV[(vi+1)&3] * uvscale + voffset);
			glVertex3f(cx + HalfTable[(vi+1)&2], cd.Y[vi&3] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(vi+2)&2]);	// corner
			
			vi = (vi + 1) & 3;
			mask >>= 1;
			if (mask & 1) {
				TriangleCount++;
//				glTexCoord2f(EdgeUV[vi&3] * uvscale + uoffset, EdgeUV[(vi+1)&3] * uvscale + voffset);
				glVertex3f(cx + HalfTable[vi&3], Y[(vi&3)+1] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(vi+1)&3]);	// edge
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
				glVertex3f(cx, Y[0] * HEIGHT_TO_METERS_FACTOR, cz);	// center
				
//				glTexCoord2f(EdgeUV[i] * uvscale + uoffset, EdgeUV[(i+1)&3] * uvscale + voffset);
				glVertex3f(cx + HalfTable[i], Y[i+1] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(i+1)&3]);	// edge
				
//				glTexCoord2f(CornerUV[i] * uvscale + uoffset, CornerUV[(i+1)&3] * uvscale + voffset);
				glVertex3f(cx + HalfTable[(i+1)&2], cd.Y[i&3] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(i+2)&2]);	// corner

//				glTexCoord2f(EdgeUV[(i+1)&3] * uvscale + uoffset, EdgeUV[(i+2)&3] * uvscale + voffset);
				glVertex3f(cx + HalfTable[(i+1)&3], Y[((i+1)&3)+1] * HEIGHT_TO_METERS_FACTOR, cz + HalfTable[(i+2)&3]);	// edge
				
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
			delete [] HeapBlock[i];
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
	Console::Printf("HeapBlockCount = %d, MQHBLOCKS = %d
", qsquare::HeapBlockCount, MAX_QSQUARE_HEAP_BLOCKS);
	Console::Printf("SquareCount = %d, QHBSIZE = %d
", qsquare::SquareCount, QSQUARE_HEAP_BLOCK_SIZE);
	Console::Printf("sizeof(qsquare) = %d
", sizeof(qsquare));
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
#if !USE_NSS
	Root->ComputeActivationDistance(RootCornerData, Root->Y[0]);
#endif

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
	DetailNudge = fclamp(0.333, (Config::GetFloat("TerrainMeshSlider") - 1) / 10 + 0.5, 3);
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


Render::Texture*	Detail = NULL;


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
#if !USE_NSS
		// Recalculate activation distances.
		Root->ComputeActivationDistance(RootCornerData, Root->Y[0]);
#endif
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
		Detail = Render::NewTexture("detail.psd", false, true, true, true);
	}

	//xxxxxx
	glColor3f(1, 1, 1);
	Render::SetTexture(NULL);
	Render::DisableLightmapBlend();
	Render::CommitRenderState();

	// Tweak the modelview matrix so that our fixed-point Y coords get scaled by 1/16.  Then we can just pass
	// integers to glVertex3i().  This could be stupid; need to test.
//	glMatrixMode(GL_MODELVIEW);
//	glPushMatrix();
//	glScalef(1, HEIGHT_TO_METERS_FACTOR, 1);
	
	// Enable automatic texture-coord generation, for faster vertex specs.
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	
	// Set up vertex array.
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_INT /* GL_FLOAT */, 0, VertexArray);
	
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

	// Second pass: detail texture.
	bool	DoDetail = Config::GetBoolValue("DetailMapping");
	if (DoDetail) {
		glDisable(GL_FOG);
		Render::SetTexture(Detail);
		Render::EnableLightmapBlend();
		Render::CommitRenderState();

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

		Render::DisableLightmapBlend();
		if (Config::GetBoolValue("Fog")) glEnable(GL_FOG);
	}
	
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	// Disable the vertex array.
	glDisableClientState(GL_VERTEX_ARRAY);
	
//	// Restore original modelview matrix.
//	glPopMatrix();
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
//	glMatrixMode(GL_MODELVIEW);
//	glPushMatrix();
//	glScalef(1, HEIGHT_TO_METERS_FACTOR, 1);
	
	// Set up vertex array.
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_INT /* GL_FLOAT */, 0, VertexArray);

	// Set up culling box.
	CullBoxX = frnd(ShadowCenter.X());
	CullBoxZ = frnd(ShadowCenter.Z());
	CullBoxExtent = frnd(ShadowExtent + 0.5);
	
	// Render.

	// Special pass: shadow.
	Root->Render(RootCornerData, s, 0, false, qsquare::SHADOW);

	// Disable the vertex array.
	glDisableClientState(GL_VERTEX_ARRAY);
	
//	// Restore original modelview matrix.
//	glPopMatrix();
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
	printf("sizeof(Square) = %d
", sizeof(TerrainMesh::qsquare));

	return 0;
}


#endif // NOT
