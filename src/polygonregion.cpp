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
// polygonregion.cpp	-thatcher 8/18/2000 Copyright Slingshot Game Technology

// Code for dealing with polygonal terrain regions.


#include "polygonregion.hpp"
#include "lua.hpp"
#include "console.hpp"
#include "game.hpp"


namespace PolygonRegion {
;


struct Vertex {
	float	x, z;
//	Vertex	*Next, *Previous;
};


struct Poly {
	bool	Closed;
	int	TriggerScript;
	int	VertexCount;
	Vertex*	Vert;
	float	minx, minz, maxx, maxz;	// Bounding box.
	Poly*	Next;
	
	Poly() {
		Closed = false;
		TriggerScript = -1;
		VertexCount = 0;
		Vert = NULL;

		minx = minz = maxx = maxz = 0;

		Next = NULL;
	}

	~Poly() {
//		// Delete script string.
//		if (TriggerScript) delete [] TriggerScript;
		if (TriggerScript != -1) lua_unref(TriggerScript);

		// Delete vertex loop.
		if (Vert) delete [] Vert;
	}

	void	Load(FILE* fp)
	// Initialize this polygon from the given file.
	{
		// Get the surface type.  But we don't actually use it.
		int	Type = Read32(fp);

		// Get the vertex count.
		VertexCount = Read32(fp);
		
		// If the polygon has zero vertices, then we're done.
		if (VertexCount <= 0) return;

		minx = 1000000;
		minz = 1000000;
		maxx = -1000000;
		maxz = -1000000;
	
		Vert = new Vertex[VertexCount];
		
		// Load the vertices.
		for (int i = 0; i < VertexCount; i++) {
			float	x = ReadFloat(fp), z = ReadFloat(fp);

			x -= 32768;
			z -= 32768;
			
			Vertex*	v = &Vert[i];
			v->x = x;
			v->z = z;

			// Update bounding box.
			if (x < minx) minx = x;
			if (z < minz) minz = z;
			if (x > maxx) maxx = x;
			if (z > maxz) maxz = z;
		}

		// Load the 'Closed' flag.
		unsigned char	c = fgetc(fp);
		Closed = (c != 0);
		
		// Load the script.
		uint16	len = Read16(fp);

		if (len) {
			char	buf[5000];
			if (len >= 5000) { Error e; e << "Trigger script exceeds 5000-char limit."; throw e; }
			
//			TriggerScript = new char[len+1];
//			fread(TriggerScript, len, 1, fp);
//			TriggerScript[len] = 0;
			fread(buf, len, 1, fp);
			buf[len] = 0;
			lua_beginblock();
			lua_dostring(buf);
			lua_Object	result = lua_getresult(1);
			if (result != LUA_NOOBJECT && lua_isfunction(result)) {
				lua_pushobject(result);
				TriggerScript = lua_ref(true);
			} else {
				Console::Log("Warning: while loading polygon, lua script did not return a function");
			}
			lua_endblock();
		}
	}
};


Poly*	PolyList = NULL;


void	Open()
// Initialize internal data.
{
}


void	Close()
// Free stuff.
{
	Clear();	// Make sure polys are deleted.
}

void	Clear()
// Empties out our polygon list.
{
	Poly*	p = PolyList;

	while (p) {
		Poly*	n = p->Next;
		delete p;
		p = n;
	}

	PolyList = NULL;
}


//void	LoadPolygon(FILE* fp);


void	Load(FILE* fp)
// Load the polygonal surface-type regions from the given file, and
// Decompose them into trapezoids
{
	// Load the polygons.
	int	PolygonCount = Read32(fp);
	
	// Don't read if we're at the end of the file already.
	if (feof(fp)) return;

	for ( ; PolygonCount; PolygonCount--) {
		Game::LoadingTick();
		
		// Load this polygon and add it to the database.
		Poly*	p = new Poly;
		p->Load(fp);

		p->Next = PolyList;
		PolyList = p;
	}

	// other processing steps?
}


void	CheckAndCallTriggers(MObject* obj, const vec3& loc0, const vec3& loc1)
// Checks the line segment between loc0 and loc1, against each edge of
// each polygon.  If the segment crosses any edge in the x,z plane, call
// the TriggerScript associated with the polygon.
{
	float	x0, x1, z0, z1;

	// Figure out the segment bounding box, to streamline culling.
	if (loc0.X() < loc1.X()) {
		x0 = loc0.X();
		x1 = loc1.X();
	} else {
		x0 = loc1.X();
		x1 = loc0.X();
	}		
	if (loc0.Z() < loc1.Z()) {
		z0 = loc0.Z();
		z1 = loc1.Z();
	} else {
		z0 = loc1.Z();
		z1 = loc0.Z();
	}

	// Go through the polygon list.
	Poly*	p = PolyList;
	for (Poly* o = PolyList; p; p = p->Next) {
		// Reject if bounding boxes don't overlap.
		if (x1 < p->minx || x0 > p->maxx || z1 < p->minz || z0 > p->maxz) {
			continue;
		}

		// Check each segment of the poly.
		int	ct = p->VertexCount;
		for (int i = 0; i < p->VertexCount; i++) {
			Vertex	*v0, *v1;
			v0 = &p->Vert[i];
			if (i == p->VertexCount-1) {
				if (p->Closed == false) break;

				v1 = &p->Vert[0];	// Loop back to the beginning.
			} else {
				v1 = &p->Vert[i+1];
			}
			
			float	sa0, sa1, sa2, sa3;

			// Compute signed areas of joining triangles.  If their signs match, they can't intersect.
			sa0 = (loc0.Z() - v0->z) * (v1->x - loc0.X()) - (loc0.X() - v0->x) * (v1->z - loc0.Z());
			sa1 = (loc1.Z() - v0->z) * (v1->x - loc1.X()) - (loc1.X() - v0->x) * (v1->z - loc1.Z());

			sa2 = (v0->z - loc0.Z()) * (loc1.X() - v0->x) - (v0->x - loc0.X()) * (loc1.Z() - v0->z);
			sa3 = (v1->z - loc0.Z()) * (loc1.X() - v1->x) - (v1->x - loc0.X()) * (loc1.Z() - v1->z);
			
			if (sa0 * sa1 > 0 || sa2 * sa3 > 0) continue;

			// Segments intersect.  Call trigger script.
			if (p->TriggerScript != -1) {
//				lua_dostring(p->TriggerScript);
				lua_beginblock();
				lua_pushusertag(obj, 0);
				lua_callfunction(lua_getref(p->TriggerScript));
				lua_endblock();
			}
		}
	}
}



#ifdef NOT
int	GetSurfaceType(const vec3& location)
// Returns the type of the surface under the (x,z) coordinates in the given vec3.
// The y coordinate is ignored.
{
	float	x = location.X();
	float	z = location.Z();
	
	// Go backwards through the trapezoid list and return the type of the first trapezoid which
	// contains the query.
	for (Trapezoid* t = TrapezoidListEnd; t; t = t->Previous) {
		if (z < t->z0 || z > t->z1) continue;
		if (x < t->xl0 && x < t->xl1) continue;
		if (x > t->xr0 && x > t->xr1) continue;

		float	lx = t->xl0 + (t->xl1 - t->xl0) * (z - t->z0) / (t->z1 - t->z0);
		float	rx = t->xr0 + (t->xr1 - t->xr0) * (z - t->z0) / (t->z1 - t->z0);
		if (x >= lx && x <= rx) {
			// Query is inside this trapezoid.
			return t->SurfaceType;
		}
	}

	// Query isn't inside any trapezoid.
	return 0;
}
#endif // NOT


#ifdef OLD_TRAPEZOID_STUFF


const int	TEMP_VERTEX_COUNT = 1000;
static Vertex	TempVertex[TEMP_VERTEX_COUNT];

void	MakeTrapezoids(Vertex* Loop, int Type);


void	LoadPolygon(FILE* fp)
// Loads a polygon description from the given file, and stores it in our
// polygon database.
{
	// Get the surface type.
	int	Type = Read32(fp);

	// Get the vertex count.
	int	VertexCount = Read32(fp);

	// Guard against overflow of the temp vertex array.
	if (VertexCount > TEMP_VERTEX_COUNT) {
		Error e; e << "Polygon exceeds 1000 vertex limit.";
		throw e;
	}

	// If the polygon has zero vertices, then we're done.
	if (VertexCount <= 0) return;

	// Load the vertices.
	for (int i = 0; i < VertexCount; i++) {
		float	x = ReadFloat(fp), z = ReadFloat(fp);
		
		Vertex*	v = &TempVertex[i];
		v->x = x;
		v->z = z;
	}

	// Initialize vertex links.
	for (i = 0; i < VertexCount-1; i++) {
		TempVertex[i].Next = &TempVertex[i+1];
	}
	TempVertex[i].Next = &TempVertex[0];

	TempVertex[0].Previous = &TempVertex[VertexCount-1];
	for (i = 1; i < VertexCount; i++) {
		TempVertex[i].Previous = &TempVertex[i-1];
	}

	// Break into trapezoids.
	MakeTrapezoids(&TempVertex[0], Type);
}


// Database of trapezoids.  Should partition geometrically.  (clip to grid squares?)


struct Trapezoid {
	float	z0, z1;
	float	xl0, xl1, xr0, xr1;
	int	SurfaceType;

	Trapezoid*	Next;
	Trapezoid*	Previous;
};


Trapezoid*	TrapezoidList = NULL;
Trapezoid*	TrapezoidListEnd = NULL;


void	AddTrapezoid(float z0, float z1, float xl0, float xl1, float xr0, float xr1, int SurfaceType)
// Adds a new trapezoid with the given parameters to our database.
// Examines xl0 and xr0 to see which is smaller, and reorders if
// necessary.  Same with xl0 and xr1.  Untangles trapezoids whose side
// edges cross.
// This trapezoid obscures anything underneath it.
{
//	return;	//xxxxxxxx
	
	Trapezoid*	t = new Trapezoid;

	// Convert all coordinates from world coords to texture coords.
	static const float	W2T = (TEXTURE_SIZE - 1) / float(1 << (TEXTURE_BITS - TEXTURE_BITS_PER_METER));
	t->z0 = z0 * W2T;
	t->z1 = z1 * W2T;
	if (xl0 <= xr0) {
		t->xl0 = xl0 * W2T;
		t->xr0 = xr0 * W2T;
	} else {
		t->xl0 = xr0 * W2T;
		t->xr0 = xl0 * W2T;
	}
	if (xl1 < xr1) {
		t->xl1 = xl1 * W2T;
		t->xr1 = xr1 * W2T;
	} else {
		t->xl1 = xr1 * W2T;
		t->xr1 = xl1 * W2T;
	}
	t->SurfaceType = SurfaceType;

	// Add to the end of the trapezoid list.
	t->Next = NULL;
	t->Previous = TrapezoidListEnd;
	if (TrapezoidListEnd) TrapezoidListEnd->Next = t;
	else TrapezoidList = t;
	TrapezoidListEnd = t;
}


void	MakeTrapezoids(Vertex* Loop, int Type)
// Takes the polygon defined by the given loop of vertices, and breaks it
// into trapezoids which it passes to AddTrapezoid().
{
	while (1) {
		// Chop off the top trapezoid and add it to our list.

		// Find the top corner vertice(s).
		Vertex*	top0;
		Vertex*	top1;
		Vertex*	top0prev;
		Vertex*	top1next;

		// Find the top of the next trapezoid in this polygon.
		// Basically, find a highest vertex in the loop, and
		// walk in either direction until the loop turns
		// downward.  Set top0 and top1 to the rightmost and
		// leftmost vertices along this top edge.  If the shape
		// is not flat at the top, then top0 and top1 will be
		// the same vertex, which is OK.

		// First find a vertex on the top edge.
		top0 = Loop;
		Vertex*	v = Loop;
		do {
			if (v->z < top0->z) {
				top0 = v;	// Higher vertex.
			}
			v = v->Next;
		} while (v != Loop);

		// Now walk backward through the loop, until it bends downward, to find top0.
		v = top0;
		while (1) {
			if (v->Previous->z > top0->z) {
				top0 = v;
				break;
			}
			
			v = v->Previous;
			if (v == top0) {
				// We went all the way around and didn't find a corner.
				// There's no area left in the polygon, so we're done.
				return;
			}
		}

		// Walk forward from top0, until we find a corner.  The previous vertex is top1.
		v = top0;
		while (1) {
			if (v->Next->z > top0->z) {
				top1 = v;
				break;
			}

			v = v->Next;
			if (v == top0) {
				// We went all the way around and didn't find a corner.
				// There's no area left in the polygon, so we're done.
				// (This condition should already have been caught by the search for top0)
				return;
			}
		}

		top0prev = top0->Previous;
		top1next = top1->Next;

		// Check for a vertex that's higher than top0prev or top1next, and in between those edges,
		// which would require us to split the remainder of the polygon.
		Vertex*	split = NULL;
		float	splitprevx, splitnextx;
		v = top0prev;
		while (v != top1next) {
			if (v->z < top0prev->z && v->z < top1next->z) {
				// Check to see if it's between the edges.
				float	prevx = top0->x + (top0prev->x - top0->x) * (v->z - top0->z) / (top0prev->z - top0->z);
				float	nextx = top1->x + (top1next->x - top1->x) * (v->z - top1->z) / (top1next->z - top1->z);

				if ((v->x > prevx && v->x < nextx) || (v->x < prevx && v->x > nextx)) {
					// Is this split higher than any previous split?
					if (split == NULL || v->z < split->z) {
						split = v;
						splitprevx = prevx;
						splitnextx = nextx;
					}
				}
			}
			
			v = v->Previous;
		}

		if (split) {
			// We have a split.  Generate the top trapezoid, then split the polygon in two
			// and recurse on the pieces.
			AddTrapezoid(top0->z, split->z, top0->x, splitprevx, top1->x, splitnextx, Type);
			
			// Split & recurse.
			float	sx = split->x;
			float	sz = split->z;
			Vertex*	sprev = split->Previous;
			Vertex	top1copy(*top1);
			
			// Move top0 and top1 down to the intersection points, & reuse them.
			top0->x = splitprevx;
			top0->z = sz;
			
			top0->Next = split;
			split->Previous = top0;
			
			MakeTrapezoids(top0, Type);

			*top1 = top1copy;
			
			// Resuscitate split for the other poly.
			split->x = sx;
			split->z = sz;
			split->Previous = sprev;
			
			top1->x = splitnextx;
			top1->z = sz;
			split->Next = top1;
			top1->Previous = split;
			
			MakeTrapezoids(top1, Type);
			
			return;
		}
		
		// Compute the coordinates of the bottom corners of this trapezoid.
		float	bottomz;
		float	prevx1, nextx1;
		if (top1next->z < top0prev->z) {
			bottomz = top1next->z;
			nextx1 = top1next->x;
			prevx1 = top0->x + (top0prev->x - top0->x) * (bottomz - top0->z) / (top0prev->z - top0->z);
			
		} else if (top0prev->z < top1next->z) {
			bottomz = top0prev->z;
			prevx1 = top0prev->x;
			nextx1 = top1->x + (top1next->x - top1->x) * (bottomz - top1->z) / (top1next->z - top1->z);
			
		} else {
			bottomz = top0prev->z;
			prevx1 = top0prev->x;
			nextx1 = top1next->x;
		}

		// Add this trapezoid to our database.
		AddTrapezoid(top0->z, bottomz, top0->x, nextx1, top1->x, prevx1, Type);

		// Splice top vertices out, and add new vertex in if needed.
		Loop = top1next;
		if (top1next->z > top0prev->z) {
			// Create new vertex above top1next.
			Vertex*	newvert = top0;	// Reuse top0, since we're discarding it anyway.
			top1next->Previous = newvert;
			newvert->Next = top1next;

			newvert->x = nextx1;
			newvert->z = bottomz;
			newvert->Previous = top0prev;
			top0prev->Next = newvert;
			
		} else if (top0prev->z > top1next->z) {
			// Create new vertex above top0prev.
			Vertex*	newvert = top0;	// Reuse top0, since we're discarding it anyway.
			top0prev->Next = newvert;
			newvert->Previous = top0prev;

			newvert->x = prevx1;
			newvert->z = bottomz;
			newvert->Next = top1next;
			top1next->Previous = newvert;
			
		} else {
			// No new vertex needed, since next verts are at the same z.
			// Just splice out the top vertices.
			top1next->Previous = top0prev;
			top0prev->Next = top1next;
		}
	}
}


#endif	// OLD_TRAPEZOID_STUFF


};	// end namespace PolygonRegion
