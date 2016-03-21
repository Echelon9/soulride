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
// clip.cpp	-thatcher 2/8/1998 Copyright Thatcher Ulrich

// Takes care of projection of vertices and clipping of triangles.


#include "clip.hpp"


namespace Clip {
;


#ifdef NOT


void	ClipVertex::Project(const ViewState& s)
// Projects *this according to the vertex's coordinates and the given
// view state.
{
	if (Projected) return;
	
	float	w = 1.0 / v.Z();
	SetW(w);
//xxx	w *= s.XProjectionFactor;
	vec3& result = const_cast<vec3&>(GetVector());
	result.SetX(/* s.XOffset - */ v.X() * w);
	result.SetY(/* s.YOffset - */ v.Y() * w);
//	result.SetZ((v.Z() - s.MinZ) * s.OneOverMaxZMinusMinZ);
	result.SetZ(1.0 - s.MinZ * GetW());

	SetW(1.0);	//xxxxxx
	
	Projected = true;
}


bool	ClipVertex::Preprocess(const ViewState& s)
// Computes the outcodes for the vertex, and projects it if it's within view.
// Returns true if the vertex requires clipping.
{
	ClipOutcode = ComputeClipOutcode(v, s);

	if (ClipOutcode == 0) {
		// The vertex is visible, so it's worth projecting it.
		Project(s);
		return false;
	} else {
		return true;
	}
}


#endif // NOT


int	ComputeClipOutcode(const vec3& v, const ViewState& s)
// Computes the visibility of the given vec3 with respect to the clipping planes
// in the given view state.
{
	int	result = 0;
	int	mask = 1;
	int	i;
	// Go through each of the planes and see which side the vec3 is on.
	for (i = 0; i < 6; i++, mask <<= 1) {
		float	d = v * s.ClipPlane[i].Normal;
		if (d < s.ClipPlane[i].D) {
			// The point is on the wrong side of this clipping plane.
			// Set the corresponding bit in the outcode.
			result |= mask;
		}
	}

	return result;
}


#ifdef NOT


void	FindIntersection(ClipVertex* ResultVertex, const ClipVertex& a, const ClipVertex& b, const vec3& Normal, float D)
// Helper function for ClipTriangle().
// Finds the intersection between the line defined by a and b, and
// the plane defined by Normal and D.  *ResultVertex is initialized with
// interpolated values for position, texture coordinates and vertex color.
{
	float f, da, db;
	da = a.GetCoordinates() * Normal;
	db = b.GetCoordinates() * Normal;

	// f is the ratio of the distance of a-to-intersection, to a-to-b.
	f = (da - D) / (da - db);

	// Compute the coordinates of the intersection point.
	vec3&	ResultVector = ResultVertex->GetViewVector();
	ResultVector = a.GetCoordinates();
	ResultVector += (b.GetCoordinates() - a.GetCoordinates()) * f;
	ResultVertex->SetDirty();

	// Set the members of *ResultVertex.
	// Interpolate the texture coordinates.
	ResultVertex->SetU(a.GetU() + (b.GetU() - a.GetU()) * f);
	ResultVertex->SetV(a.GetV() + (b.GetV() - a.GetV()) * f);

	// There's probably a clever way to compute this color without splitting the components.  Improve at some point.
	int ar, ag, ab, br, bg, bb;
	ar = (a.GetColor() & 0x0FF0000) >> 16;
	ag = (a.GetColor() & 0x0FF00) >> 8;
	ab = (a.GetColor() & 0x0FF);
	br = (b.GetColor() & 0x0FF0000) >> 16;
	bg = (b.GetColor() & 0x0FF00) >> 8;
	bb = (b.GetColor() & 0x0FF);
	ResultVertex->SetColor((int(ar + f * (br - ar)) << 16) |
				(int(ag + f * (bg - ag)) << 8) |
				(int(ab + f * (bb - ab))));
}


void	ClipTriangle(ClipVertex* a, ClipVertex* b, ClipVertex* c, const ViewState& s, int plane)
// Clips the triangle defined by the given vertices against the clipping
// plane of the given view state indexed by the 'plane' parameter.  Recurses
// on the next clipping plane, if there is one.  When it runs out of clip
// planes, it passes the triangle to DrawUnclippedTriangle().
{
	// Check to see if we're done clipping.
	if (plane >= s.ClipPlaneCount) {
		DrawUnclippedTriangle(a, b, c, s);
		return;
	}

	// Figure out which side of the clipping plane each of the vertices is on.
	int	mask = 1 << plane;
	bool	aclip, bclip, cclip;
	aclip = a->GetOutcode() & mask ? true : false;
	bclip = b->GetOutcode() & mask ? true : false;
	cclip = c->GetOutcode() & mask ? true : false;
	int	clipcount = int(aclip) + int(bclip) + int(cclip);

	// See if all the vertices are on the right side of the clipping plane.
	if (clipcount == 0) {
		// No clipping against this plane is needed.
		ClipTriangle(a, b, c, s, plane + 1);
		return;
	}

	// See if all the vertices are on the wrong side of the clipping plane.
	if (clipcount == 3) {
		// Triangle is not visible.
		return;
	}

	if (clipcount == 2) {
		// Sort so that a is on the right side.
		if (aclip) {
			if (bclip) {
				ClipVertex* temp = a; a = c; c = temp;
			} else {
				ClipVertex* temp = a; a = b; b = temp;
			}
		}

		// Convenience variables.
		const vec3&	Normal = s.ClipPlane[plane].Normal;
		float	D = s.ClipPlane[plane].D;
		
		// Find the vertices at the intersection of clipped edges.
		ClipVertex	d, e;
		FindIntersection(&d, *a, *b, Normal, D);
		d.Preprocess(s);
		FindIntersection(&e, *a, *c, Normal, D);
		e.Preprocess(s);

		// Draw the visible fragment.
		ClipTriangle(a, &d, &e, s, plane + 1);
		return;
	} else {
		// One vertex on the wrong side of the plane.
		// Sort so that a is on the wrong side.
		if (!aclip) {
			if (!bclip) {
				ClipVertex* temp = a; a = c; c = temp;
			} else {
				ClipVertex* temp = a; a = b; b = temp;
			}
		}

		// Convenience variables.
		const vec3&	Normal = s.ClipPlane[plane].Normal;
		float	D = s.ClipPlane[plane].D;
		
		// Find the vertices at the intersection of clipped edges.
		ClipVertex	d, e;
		FindIntersection(&d, *a, *b, Normal, D);
		d.Preprocess(s);
		FindIntersection(&e, *a, *c, Normal, D);
		e.Preprocess(s);

		// Draw the visible fragments.
		ClipTriangle(b, &d, &e, s, plane + 1);
		ClipTriangle(b, &e, c, s, plane + 1);
		return;
	}
}


void	DrawClippedTriangle(ClipVertex* a, ClipVertex* b, ClipVertex* c, const ViewState& s)
// Clips the triangle defined by the given vertices, projects the
// vertices if necessary, and passes any visible fragments to the
// renderer.
{
	// Make sure the outcodes are set.
	if (a->GetOutcode() == -1) a->Preprocess(s);
	if (b->GetOutcode() == -1) b->Preprocess(s);
	if (c->GetOutcode() == -1) c->Preprocess(s);

	// Check for trivial cull.
	if ((a->GetOutcode() & b->GetOutcode() & c->GetOutcode()) != 0) {
		// All the vertices are on the wrong side of some clip plane.  Don't need to draw.
		return;
	}

	ClipTriangle(a, b, c, s, 0);
#ifdef NOT
	//xxxxxxxxxxxx
	DrawUnclippedTriangle(a, b, c, s);
#endif // NOT
}


void	DrawUnclippedTriangle(ClipVertex* a, ClipVertex* b, ClipVertex* c, const ViewState& s)
// Does no clipping.  The triangle defined by the given vertices should
// be provably in the view volume.  Projects the vertices if necessary and passes the
// triangle to the renderer.
{
	// Make sure the vertices have been projected.
	if (a->GetProjected() == false) a->Project(s);
	if (b->GetProjected() == false) b->Project(s);
	if (c->GetProjected() == false) c->Project(s);

	// Pass to the renderer.
	Render::DrawTriangle(*a, *b, *c);
}


//
// Quad functions.
//


void	DrawClippedQuad(ClipVertex* a, ClipVertex* b, ClipVertex* c, ClipVertex* d, const ViewState& s)
// Clips the quad defined by the given vertices, projects the
// vertices if necessary, and passes any visible fragments to the
// renderer.
{
	// Make sure the outcodes are set.
	if (a->GetOutcode() == -1) a->Preprocess(s);
	if (b->GetOutcode() == -1) b->Preprocess(s);
	if (c->GetOutcode() == -1) c->Preprocess(s);
	if (d->GetOutcode() == -1) d->Preprocess(s);

	// Check for trivial cull.
	if ((a->GetOutcode() & b->GetOutcode() & c->GetOutcode() & d->GetOutcode()) != 0) {
		// All the vertices are on the wrong side of some clip plane.  Don't need to draw.
		return;
	}

	// Check for trivial accept.
	if ((a->GetOutcode() | b->GetOutcode() | c->GetOutcode() | d->GetOutcode()) == 0) {
		// All vertices OK, so go ahead and render as an unclipped quad.
		DrawUnclippedQuad(a, b, c, d, s);
	} else {
		// Split into tris and clip.
		ClipTriangle(a, b, c, s, 0);
		ClipTriangle(a, c, d, s, 0);
	}
	
#ifdef NOT
	//xxxxxxxxxxxx
	DrawUnclippedTriangle(a, b, c, s);
#endif // NOT
}


void	DrawUnclippedQuad(ClipVertex* a, ClipVertex* b, ClipVertex* c, ClipVertex* d, const ViewState& s)
// Draws the quad.  Should be planar and completely within the view frustum.
{
	if (a->GetProjected() == false) a->Project(s);
	if (b->GetProjected() == false) b->Project(s);
	if (c->GetProjected() == false) c->Project(s);
	if (d->GetProjected() == false) d->Project(s);
	
	Render::DrawQuad(*a, *b, *c, *d);
}


//
// line functions
//


void	DrawUnclippedLine(ClipVertex* a, ClipVertex* b, const ViewState& s)
// Draws the line, with no clipping.
{
	// Make sure vertices have been projected.
	if (a->GetProjected() == false) a->Project(s);
	if (b->GetProjected() == false) b->Project(s);

	// Pass to renderer.
	Render::DrawLine(*a, *b);
}


void	ClipLine(ClipVertex* a, ClipVertex* b, const ViewState& s, int plane)
// Clips the line defined by the given vertices against the clipping
// plane of the given view state indexed by the 'plane' parameter.  Recurses
// on the next clipping plane, if there is one.  When it runs out of clip
// planes, it passes the line to DrawUnclippedLine().
{
	// Check to see if we're done clipping.
	if (plane >= s.ClipPlaneCount) {
		DrawUnclippedLine(a, b, s);
		return;
	}

	// Figure out which side of the clipping plane each of the vertices is on.
	int	mask = 1 << plane;
	bool	aclip, bclip;
	aclip = a->GetOutcode() & mask ? true : false;
	bclip = b->GetOutcode() & mask ? true : false;
	int	clipcount = int(aclip) + int(bclip);

	// See if all the vertices are on the right side of the clipping plane.
	if (clipcount == 0) {
		// No clipping against this plane is needed.
		ClipLine(a, b, s, plane + 1);
		return;
	}

	// See if all the vertices are on the wrong side of the clipping plane.
	if (clipcount == 2) {
		// line is not visible.
		return;
	}

	// Sort so that a is on the right side.
	if (aclip) {
		ClipVertex* temp = a; a = b; b = temp;
	}

	// Convenience variables.
	const vec3&	Normal = s.ClipPlane[plane].Normal;
	float	D = s.ClipPlane[plane].D;
		
	// Find the vertex at the intersection of clipped line.
	ClipVertex	c;
	FindIntersection(&c, *a, *b, Normal, D);
	c.Preprocess(s);
	
	// Draw the visible fragment.
	ClipLine(a, &c, s, plane + 1);
}


void	DrawClippedLine(ClipVertex* a, ClipVertex* b, const ViewState& s)
// Clips the line defined by the given vertices, projects the
// vertices if necessary, and passes any visible fragment to the
// renderer.
{
	// Make sure the outcodes are set.
	if (a->GetOutcode() == -1) a->Preprocess(s);
	if (b->GetOutcode() == -1) b->Preprocess(s);

	// Check for trivial cull.
	if ((a->GetOutcode() & b->GetOutcode()) != 0) {
		// All the vertices are on the wrong side of some clip plane.  Don't need to draw.
		return;
	}

	ClipLine(a, b, s, 0);
}


#endif // NOT


//
// visibility tests
//


int	ComputeBoxVisibility(const vec3& min, const vec3& max, const ViewState& s, int ClipHint)
// Returns a visibility code indicating whether the axis-aligned box defined by {min, max} is
// completely inside the frustum, completely outside the frustum, or partially in.
// The return value corresponds to ClipHint.  The first six bits in ClipHint correspond
// to flags for each frustum plane; if a bit is set, that means the box is known to
// be inside the corresponding plane, so no additional test is needed.
{
	// Doesn't do a perfect test for NOT_VISIBLE.  Checks each
	// box vertex against each frustum plane.
	
	// Check each vertex of the box against the view frustum, and compute
	// bit codes for whether the point is outside each plane.
	int	OrCodes = 0, AndCodes = ~0;

	for (int i = 0; i < 8; i++) {
		
		vec3	v(min), w;
		if (i & 1) v.SetX(max.X());
		if (i & 2) v.SetY(max.Y());
		if (i & 4) v.SetZ(max.Z());
		
		s.ViewMatrix.Apply(&w, v);	// May need to pull matrix out of gl

		// Now check against the frustum planes.
		int	Code = ClipHint;
		int	Bit = 1;
		for (int j = 0; j < 6; j++, Bit <<= 1) {
			if ((ClipHint & Bit) == 0) {
				if (w * s.ClipPlane[j].Normal > s.ClipPlane[j].D) {
					// The point is inside this plane.
					Code |= Bit;
				}
			}
		}

		OrCodes |= Code;
		AndCodes &= Code;
	}

	// Based on bit-codes, return culling results.
	if (OrCodes != 0x3F) {
		// All verts are outside (at least) one of the planes.
		return NOT_VISIBLE;
	} else if (AndCodes == 0x3F) {
		// All verts are inside all planes.
		return NO_CLIP;
	} else {
		// All verts possibly inside some planes.
		return AndCodes;
	}
}


int	ComputeSphereVisibility(const vec3& center, float radius, const ViewState& s, int ClipHint)
// Returns a visibility code indicating whether the specified sphere is
// completely inside the frustum, completely outside the frustum, or
// partially in.
{
//	// Check the sphere against each plane of the view frustum.
//	Visibility	result = NO_CLIP;

	vec3	w;
	s.ViewMatrix.Apply(&w, center);

	// Now check against the frustum planes.
	int	Code = 0;
	int	Bit = 1;
	for (int j = 0; j < 6; j++, Bit <<= 1) {
		float	d = w * s.ClipPlane[j].Normal - s.ClipPlane[j].D;
		if (d < -radius) {
			// The sphere is outside this plane.  Early out.
			return NOT_VISIBLE;
			
		} else if (d < radius) {
			// The sphere is only partially inside this
			// plane, so finer clipping/culling might still
			// be required.
			return ClipHint;
		}
	}
	
	return NO_CLIP;
}


int	ComputeBoxVisibility(const vec3& min, const vec3& max, Plane Frustum[6], int ClipHint)
// Returns a visibility code indicating whether the axis-aligned box defined by {min, max} is
// completely inside the given frustum, completely outside the frustum, or partially in.
// The return value corresponds to ClipHint.  The first six bits in ClipHint correspond
// to flags for each frustum plane; if a bit is set, that means the box is known to
// be inside the corresponding plane, so no additional test is needed.
{
	// Doesn't do a perfect test for NOT_VISIBLE.  Checks each
	// box vertex against each frustum plane.
	
	// Check each vertex of the box against the view frustum, and compute
	// bit codes for whether the point is outside each plane.
	int	OrCodes = 0, AndCodes = ~0;

	for (int i = 0; i < 8; i++) {
		
		vec3	v(min);
		if (i & 1) v.SetX(max.X());
		if (i & 2) v.SetY(max.Y());
		if (i & 4) v.SetZ(max.Z());
		
		// Now check against the frustum planes.
		int	Code = ClipHint;
		int	Bit = 1;
		for (int j = 0; j < 6; j++, Bit <<= 1) {
			if ((ClipHint & Bit) == 0) {
				if (v * Frustum[j].Normal > Frustum[j].D) {
					// The point is inside this plane.
					Code |= Bit;
				}
			}
		}

		OrCodes |= Code;
		AndCodes &= Code;
	}

	// Based on bit-codes, return culling results.
	if (OrCodes != 0x3F) {
		// All verts are outside (at least) one of the planes.
		return NOT_VISIBLE;
	} else if (AndCodes == 0x3F) {
		// All verts are inside all planes.
		return NO_CLIP;
	} else {
		// All verts possibly inside some planes.
		return AndCodes;
	}
}


#if 0
// From Ville Miettinen, adapted from Thomas Moller.

// Intersection of AABB and a frustum. The frustum may contain 0-32 planes
(active planes are defined
// by inClipMask). Returns boolean value indicating whether AABB intersects
the view frustum or not.
// If AABB intersects the frustum, an output clip mask is returned as well
(indicating which
// planes are crossed by the AABB). This information can be used to optimize
testing of
// child nodes or objects inside the nodes (pass value as 'inClipMask').

bool intersectAABBFrustum (const AABB& a, const Vector4* p, unsigned int&
outClipMask, unsigned int inClipMask)
{
Vector3 m = a.getCenter(); // center of AABB
Vector3 d = a.getMax() - m; // half-diagonal
unsigned int mk = 1;
outClipMask = 0; // init outclip mask
while (mk <= inClipMask){ // loop while there are active planes..
if (inClipMask&mk){ // if clip plane is active...
float NP = (float)(d.x*fabs(p->x)+d.y*fabs(p->y)+d.z*fabs(p->z));
float MP = m.x*p->x+m.y*p->y+m.z*p->z+p->w;
if ((MP+NP) < 0.0f) return false; // behind clip plane
if ((MP-NP) < 0.0f) outClipMask |= mk;
}
mk+=mk; // mk = (1<<iter)
p++; // next plane
}
return true; // AABB intersects frustum
}
#endif // 0


int	ComputeSphereVisibility(const vec3& center, float radius, Plane Frustum[6], int ClipHint)
// Returns a visibility code indicating whether the specified sphere is
// completely inside the given frustum, completely outside the frustum, or
// partially in.
{
	// Check the sphere against each plane of the view frustum.
	int	Code = 0;
	int	Bit = 1;
	for (int j = 0; j < 6; j++, Bit <<= 1) {
		if (ClipHint & Bit) continue;	// Sphere is already known to be inside this plane.
		
		float	d = center * Frustum[j].Normal - Frustum[j].D;
		if (d < -radius) {
			// The sphere is completely outside this plane.  Early out.
			return NOT_VISIBLE;
			
		} else if (d > radius) {
			// The sphere is completely inside this
			// plane.
			ClipHint |= Bit;
		}
	}
	
	return ClipHint;
}


int	ComputeSphereSquaredVisibility(const vec3& center, float radius_squared, Plane Frustum[6], int ClipHint)
// Returns a visibility code indicating whether the specified sphere is
// completely inside the given frustum, completely outside the frustum, or
// partially in.
{
	// Check the sphere against each plane of the view frustum.
	int	Code = 0;
	int	Bit = 1;
	for (int j = 0; j < 6; j++, Bit <<= 1) {
		if (ClipHint & Bit) continue;	// Sphere is already known to be inside this plane.
		
		float	d = center * Frustum[j].Normal - Frustum[j].D;
		if (d * d > radius_squared) {
			if (d < 0) {
				// The sphere is completely outside this plane.  Early out.
				return NOT_VISIBLE;
			} else {
				// The sphere is completely inside this
				// plane.
				ClipHint |= Bit;
			}
		}
	}
	
	return ClipHint;
}


void	TransformFrustum(Plane Frustum[6], const matrix& obj_to_view)
// Given a transformation matrix from object space to view space,
// this function transforms the given view-space frustum into object-space,
// so that clipping/culling can be done in object space.
{
	int	i;
	vec3	n, p, q;
	
	for (i = 0; i < 6; i++) {
		// There's a faster, more direct way to do this.  Deal with later.
		obj_to_view.ApplyInverseRotation(&n, Frustum[i].Normal);
		p = Frustum[i].Normal * Frustum[i].D;
		obj_to_view.ApplyInverse(&q, p);
		Frustum[i].Normal = n;
		Frustum[i].D = n * q;
	}
}


};	// end of namespace Clip
