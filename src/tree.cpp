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
// tree.cpp	-thatcher 5/16/1998 Copyright Slingshot

// Code for tree and tree clump objects.  Includes special rendering code
// optimized for billboards.


#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#include "ogl.hpp"
#include "main.hpp"
#include "model.hpp"
#include "clip.hpp"
#include "terrain.hpp"
#include "gameloop.hpp"
#include "game.hpp"
#include "scylinder.hpp"
#include "gstamp.hpp"
#include "utility.hpp"


class Tree : public MObject
{
public:
	Tree() {}

	const vec3&	GetLocation() const { return Location; }
	void	SetLocation(const vec3& l) { Location = l; }
	
	void	Render(ViewState& s, int ClipHint)
	// Optimized version of default MObject::Render().  Takes advantage of
	// the fact that we know the tree won't be rotated.
	{
		if (Visual == NULL) return;
		
		// Save the current view translation.
		vec3	v = s.ViewMatrix.GetColumn(3);

		// Adjust translation using this object's position.
		const vec3&	l = GetLocation();
		glPushMatrix();
		glTranslatef(l.X(), l.Y(), l.Z());
		s.ViewMatrix.Translate(l);
		
		// Render using the modified matrix.
		Visual->Render(s, ClipHint);
		
		// Restore the original translation.
		s.ViewMatrix.SetColumn(3, v);
		glPopMatrix();
	}
	
	void	Update(const UpdateState& u) {}

private:
	vec3	Location;
};


// Placeholder visual which just contains the radius of the clump.
// All rendering is actually handled by the clump object.
class ClumpVisualThunk : public GModel {
public:
	ClumpVisualThunk(float radius) {
		Radius = radius;
	}
};


// Placeholder solid which just contains the radius of the clump.
// All collision-checking is actually handled directly by the clump object.
class ClumpSolidThunk : public SModel {
public:
	ClumpSolidThunk(float radius) {
		Radius = radius;
	}

	bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl) { return false; }
};


const int	BATCH_SIZE = 10;


class TreeClump : public MObject
{
public:
	vec3	Location;
	struct LocationBatch {
		int	Count;
		vec3	Location[BATCH_SIZE];
		LocationBatch*	Next;

		LocationBatch() { Count = 0; Next = NULL; }
	};
	LocationBatch*	Batches;
	LocationBatch*	LastBatch;
	vec3	Extent;	// x, y, z extent relative to center.
	
	int	TreeCount;

	// OGL display list?
	
	GStamp*	Stamp;
	SModel*	TreeSolid;

	float	Width, Height;
	
	
	TreeClump(float x, float z, GStamp* stamp, SModel* solid)
	// Constructor.  All trees within this clump will share the given stamp visual
	// and solid.
	{
		Visual = NULL;
		Solid = NULL;
		
		Location = vec3(x, 0, z);
		TreeCount = 0;
		Batches = LastBatch = NULL;

		Stamp = stamp;
		TreeSolid = solid;

		Width = Stamp->GetWidth();
		Height = Stamp->GetHeight();
	}

	
	~TreeClump()
	// Free the stuff we allocated.
	{
		if (Visual) delete Visual;
		if (Solid) delete Solid;
		
		LocationBatch*	b = Batches;
		while (b) {
			LocationBatch*	n = b->Next;
			delete b;
			b = n;
		}
	}
	

	const vec3&	GetLocation() const { return Location; }
	void	SetLocation(const vec3& l) { Location = l; }


	void	ComputeBounds()
	// Recompute our location, radius, and extent to best fit our constituent
	// trees.  Allocate and attach a visual placeholder with our radius.
	{
		float	VisualRadius = 0;
		float	SolidRadius = 0;
		
		if (TreeCount <= 0) {
			Extent = ZeroVector;
		} else {
			
			int	i;
			LocationBatch*	b = Batches;
			
			vec3	min(1000000, 1000000, 1000000), max(-1000000, -1000000, -1000000);

			// Examine all trees to find our min/max dimensions.
			for (i = 0; ; i++) {
				if (i >= b->Count) {
					// Go to next clump.
					b = b->Next;
					i = 0;
					if (b == NULL || b->Count == 0) break;
				}
				
				const vec3&	loc = b->Location[i];
				
				vec3	objmin = loc - vec3(Width * 0.5f, 0, Width * 0.5f);
				vec3	objmax = loc + vec3(Width * 0.5f, Height, Width * 0.5f);

				// Component-wise min/max check.
				for (int c = 0; c < 3; c++) {
					if (objmin.Get(c) < min.Get(c)) min.Set(c, objmin.Get(c));
					if (objmax.Get(c) > max.Get(c)) max.Set(c, objmax.Get(c));
				}
			}

			// Set radius, location, extent.
			Location = (min + max) * 0.5f;
			Extent = (max - min) * 0.5f;
			VisualRadius = sqrtf(Extent * Extent);

			// Now check objects to find maximum solid radius relative to location.
			if (TreeSolid) {
				b = Batches;
				for (i = 0; ; i++) {
					if (i >= b->Count) {
						// Go to next clump.
						b = b->Next;
						i = 0;
						if (b == NULL || b->Count == 0) break;
					}
					
					vec3	r = b->Location[i];
					r -= Location;
					
					float	rad = TreeSolid->GetRadius() + sqrtf(r * r);
					if (rad > SolidRadius) SolidRadius = rad;
				}
				
			}
		}

		// Attach a visual with the correct radius.
		if (Visual) delete Visual;
		Visual = new ClumpVisualThunk(VisualRadius);

		if (Solid) delete Solid;
		if (TreeSolid) Solid = new ClumpSolidThunk(SolidRadius);
	}
	
	
	void	AddTree(const vec3& loc)
	// Adds a tree to the clump, at the specified location.
	// Updates radius and extent if necessary.
	{
		if (LastBatch == NULL) {
			Batches = LastBatch = new LocationBatch;
		}

		if (LastBatch->Count >= BATCH_SIZE) {
			// Must tack on a new batch to the batch list.
			LastBatch->Next = new LocationBatch;
			LastBatch = LastBatch->Next;
		}

		LastBatch->Location[LastBatch->Count++] = loc;
		TreeCount++;
	}

			
	void	Render(ViewState& s, int ClipHint)
	// Render our trees in a batch.
	{
		if (TreeCount <= 0) return;
		
		if (ClipHint != Clip::NO_CLIP) {
			// Do box culling test, which is tighter than the default object sphere test done by caller.
			ClipHint = Clip::ComputeBoxVisibility(Location - Extent, Location + Extent, s, ClipHint);
			if (ClipHint == Clip::NOT_VISIBLE) return;
		}

		vec3	Right = vec3(-s.ViewMatrix.GetColumn(2).Z(), 0, s.ViewMatrix.GetColumn(0).Z());
		Right.normalize();

		float	UV[8] = {
			0, 1,
			1, 1,
			1, 0,
			0, 0
		};
		vec3	Corner[4];
		Corner[0] = Right * (Width * -0.5f);
		Corner[1] = -Corner[0];
		Corner[2] = Corner[1];	Corner[2].SetY(Height);
		Corner[3] = Corner[0];	Corner[3].SetY(Height);

		//
		// Setup render state.
		//
		
		Render::SetTexture(Stamp->GetTexture());
		Render::EnableAlphaTest();
		Render::CommitRenderState();
		glColor3f(1, 1, 1);

		// Set up vertex and tex coord arrays.
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, (const float*) Corner[0]);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, UV);

		// Initialize the translation info...
		glPushMatrix();
		vec3	v = Batches->Location[0];
		glTranslatef(v.X(), v.Y(), v.Z());

		// Draw each tree.
		int	i = 0;
		LocationBatch*	b = Batches;

		for (i = 0; ; i++) {
			if (i >= b->Count) {
				// Go to the next batch...
				b = b->Next;
				i = 0;

				if (b == NULL || b->Count == 0) break;
			}

			// Compute difference between this location and the last one.
			vec3	delta = b->Location[i];
			delta -= v;
			v = b->Location[i];

			glTranslatef(delta.X(), delta.Y(), delta.Z());
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		Model::AddToTriangleCount(TreeCount * 2);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		
		Render::DisableAlphaTest();
		glPopMatrix();
	}
	
	void	Update(const UpdateState& u) {}

	bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl)
	// Check for contact between the given cylinder traveling along
	// the given line segment, and the myriad trees within this
	// clump.  Returns true and fills in *result for the earliest
	// contact if there are any contacts.
	{
		if (TreeSolid == NULL || TreeCount == 0) return false;
		
		// Check against all the trees in this clump.  It's the same solid,
		// just in a lot of different locations.
		Collide::SegmentInfo	queryseg(seg);
		bool	FoundContact = false;
		
		int	i = 0;
		LocationBatch*	b = Batches;
		for (i = 0; ; i++) {
			if (i >= b->Count) {
				// Go to the next batch...
				b = b->Next;
				i = 0;
				if (b == NULL || b->Count == 0) break;
			}

			Collide::SegmentInfo	temp(queryseg);
			temp.Start -= b->Location[i];
			
			if (TreeSolid->CheckForContact(result, temp, cyl)) {
				// Found a contact.
				FoundContact = true;
				result->Location += b->Location[i];
				
				// Reduce the segment length for further checks.
				queryseg.LimitTime = result->EnterTime;
			}
		}

		return FoundContact;
	}
};


const int	X_DIM = 120;
const int	Z_DIM = 120;
const float	SPACING = 50;
const float	X_ORIGIN = 0 - X_DIM * SPACING / 2;
const float	Z_ORIGIN = 0 - Z_DIM * SPACING / 2;


struct ClumpNode {
	TreeClump*	Clump;
	ClumpNode*	Next;
};
ClumpNode*	(ClumpGrid[Z_DIM][X_DIM]);


static MObject*	dummy = NULL;


static struct InitTreeClump {
	InitTreeClump() {
		GameLoop::AddInitFunction(Init);

		Game::AddInitFunction(DataInit);
		Game::AddPostLoadFunction(PostLoad);
		Game::AddClearFunction(DataClear);
	}

	static void	Init()
	{
		Model::AddObjectLoader(1, TreeLoader);
	}
	
	static void	DataInit()
	{
		// Initialize ClumpGrid.
		int	i, j;
		for (j = 0; j < Z_DIM; j++) {
			for (i = 0; i < X_DIM; i++) {
				ClumpGrid[j][i] = NULL;
			}
		}
	}
	
	static void	PostLoad()
	// After all the trees have been loaded and assigned to clumps,
	// link the clumps into the static database.  The reason we have
	// to defer linking is so that we know the exact dimensions of
	// the clump, so that each clump can be put in the right spot in
	// the static database quadtree.
	{
		// Add clumps to database.
		int	i, j;
		for (j = 0; j < Z_DIM; j++) {
			for (i = 0; i < X_DIM; i++) {
				ClumpNode*	c = ClumpGrid[j][i];
				while (c) {
					c->Clump->ComputeBounds();
					Model::AddStaticObject(c->Clump);
					c = c->Next;
				}
			}
		}
	}
	

	static void	DataClear()
	// Delete nodes.
	{
		// Clear out ClumpGrid.
		int	i, j;
		for (j = 0; j < Z_DIM; j++) {
			for (i = 0; i < X_DIM; i++) {
				ClumpNode*	c = ClumpGrid[j][i];
				while (c) {
					ClumpNode*	n = c->Next;
					delete c;
					c = n;
				}
				ClumpGrid[j][i] = NULL;
			}
		}
	}

	static void	TreeLoader(FILE* fp)
	// Loads information from the given file and uses it to
	// initialize a Tree.  Either adds the tree to an existing
	// clump, creates a new clump for it, or creates it as an
	// independent object.
	{
		if (dummy == NULL) dummy = new Tree();

		// Initialize location, visual, solid.
		dummy->LoadLocation(fp);

		// compute indices.
		const vec3&	loc = dummy->GetLocation();
		int	xindex = (int) ((loc.X() - X_ORIGIN) / SPACING);
		int	zindex = (int) ((loc.Z() - Z_ORIGIN) / SPACING);

		// Get a GStamp pointer to the visual.  TreeClump needs this to set up batch rendering.
		GStamp*	Stamp = dynamic_cast<GStamp*>(dummy->GetVisual());
		SModel*	Solid = dummy->GetSolid();
		
		if (Stamp == NULL || zindex < 0 || zindex >= Z_DIM || xindex < 0 || xindex >= X_DIM) {
			// Tree is outside the grid covered by clumps
			// (or can't be put in a clump).  Link the tree
			// to the database as an individual object.
			Model::AddStaticObject(dummy);
			dummy = NULL;
		} else {
			ClumpNode*	n = ClumpGrid[zindex][xindex];
			TreeClump*	c = NULL;
			while (n) {
				// Look for matching clump.
				if (n->Clump->Stamp == Stamp && n->Clump->TreeSolid == Solid) {
					c = n->Clump;
					break;
				}
				n = n->Next;
			}

			if (c == NULL) {
				// No previously existing clump is appropriate, so create a new one and put it in the grid.
				c = new TreeClump(X_ORIGIN + (xindex + 0.5f) * SPACING, Z_ORIGIN + (zindex + 0.5f) * SPACING, Stamp, Solid);
				n = new ClumpNode;
				n->Clump = c;
				n->Next = ClumpGrid[zindex][xindex];
				ClumpGrid[zindex][xindex] = n;

//				Model::AddStaticObject(c);
			}

			c->AddTree(dummy->GetLocation());
		}
	}

} InitTreeClump;

