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
// treeclump.cpp	-thatcher 3/10/1999 Copyright Slingshot Game Technology

// Module for TreeClump class, which aggregates a bunch of identical
// trees, to make for faster drawing and to save a lot of memory over
// storing each tree as a full independent object.


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


class TreeClump : public MObject
{
public:
	vec3	Extent;	// x, y, z extent relative to center.
	
	int	TreeCount, ArrayLength;
	vec3*	TreeLocation;

	// OGL display list?
	
	GStamp*	Stamp;
	SModel*	TreeSolid;
	
	TreeClump(GStamp* stamp, SModel* solid)
	// Constructor.  All trees within this clump will share the given stamp visual
	// and solid.
	{
		TreeCount = 0;
		ArrayLength = 0;
		TreeLocation = NULL;

		Stamp = stamp;
		TreeSolid = solid;
	}

	
	~TreeClump()
	// Free the stuff we allocated.
	{
		delete [] TreeLocation;
	}
	

	void	AddTree(const vec3& loc)
	// Adds a tree to the clump, at the specified location.
	// Updates radius and extent if necessary.
	{
		if (TreeCount >= ArrayLength) {
			// Grow the array each time it exceeds its bounds.
			
			vec3*	oldloc = TreeLocation;
			int	oldlen = ArrayLength;

			ArrayLength += 10;	// Grow in increments of 10 elements.
			TreeLocation = new vec3[ArrayLength];

			// Copy old array data.
			int	i;
			for (i = 0; i < oldlen; i++) TreeLocation[i] = oldloc[i];

			// Delete old array.
			if (oldloc) delete [] oldloc;
		}

		// Add the tree.
		TreeLocation[TreeCount++] = loc;
	}

			
	void	Render(ViewState& s, int ClipHint)
	// Render our trees in a batch.
	{
		if (ClipHint != Clip::NO_CLIP) {
			// Do box culling test, which is tighter than the default object sphere test done by caller.
			// vis = Clip::Compute....(....);
		}


#ifdef NOT		
		const vec3&	loc = GetLocation();

		// If this is a new frame, recalc some parameters used for drawing.
		if (s.FrameNumber != LastFrameNumber) {
			vec3	Right;
			s.ViewMatrix.ApplyInverseRotation(&Right, vec3(-1, 0, 0));
			
			LastFrameNumber = s.FrameNumber;

			// Compute offset vec3s in object coords.
			float	w = Stamp->GetWidth();
			float	h = Stamp->GetHeight();
			Corners[0] = Right * w * -0.5;
			Corners[1] = Corners[0] + Right * w;
			Corners[2] = Corners[1];	Corners[2].SetY(Corners[2].Y() + h);
			Corners[3] = Corners[0];	Corners[3].SetY(Corners[3].Y() + h);

			// Rotate into view coords.
			for (int i = 0; i < 4; i++) {
				vec3 v(Corners[i]);
				s.ViewMatrix.ApplyRotation(&Corners[i], v);
			}
		}

		// Generate tree locations if necessary.
		if (TreeLocations == NULL) {
			TreeLocations = new vec3[TreeCount];
			
			int	i;
			srand(RandomSeed);
			for (i = 0; i < TreeCount; i++) {
				// Randomize the location.  We want a uniform distribution within the clump.
				float	theta, r;
				theta = float(rand()) / RAND_MAX * 2 * PI;
				r = float(rand()) / RAND_MAX;
				r = sqrt(r) * ClumpRadius;
				float x = loc.X() + r * cosf(theta);
				float z = loc.Z() + r * sinf(theta);
				float y = TerrainModel::GetHeight(x, z);
	
				TreeLocations[i].SetXYZ(x, y, z);
			}
		}

		// Draw the trees.
		glPushMatrix();
		glLoadIdentity();

		Render::EnableAlphaTest();
		Render::SetTexture(Stamp->GetTexture());
		Render::CommitRenderState();

		glColor3f(1, 1, 1);

		float	UV[8] = {
			0, 1,
			1, 1,
			1, 0,
			0, 0
		};
		
		int	i;
		for (i = 0; i < TreeCount; i++) {
			vec3	tl;
			s.ViewMatrix.Apply(&tl, TreeLocations[i]);
//			tl = TreeLocations[i];
			
//			vec3	v;
			
			glBegin(GL_TRIANGLE_FAN);

			glTexCoord2fv(&UV[0]);
			glVertex3f(Corners[0].X() + tl.X(), Corners[0].Y() + tl.Y(), Corners[0].Z() + tl.Z());

			glTexCoord2fv(&UV[2]);
			glVertex3f(Corners[1].X() + tl.X(), Corners[1].Y() + tl.Y(), Corners[1].Z() + tl.Z());

			glTexCoord2fv(&UV[4]);
			glVertex3f(Corners[2].X() + tl.X(), Corners[2].Y() + tl.Y(), Corners[2].Z() + tl.Z());

			glTexCoord2fv(&UV[6]);
			glVertex3f(Corners[3].X() + tl.X(), Corners[3].Y() + tl.Y(), Corners[3].Z() + tl.Z());

			glEnd();
		}

		Model::AddToTriangleCount(TreeCount * 2);
		
		Render::DisableAlphaTest();
		glPopMatrix();
#endif // NOT
	}
	
	void	Update(const UpdateState& u) {}
};


const int	X_DIM = 40;
const int	Z_DIM = 40;
const float	SPACING = 80;
const float	X_ORIGIN = 0 - X_DIM * SPACING / 2;
const float	Z_ORIGIN = 0 - Z_DIM * SPACING / 2;


struct ClumpNode {
	TreeClump*	clump;
	ClumpNode*	next;
};
ClumpNode*	(ClumpGrid[Z_DIM][X_DIM]);


static MObject*	dummy = NULL;


static struct InitTreeClump {
	InitTreeClump() {
		GameLoop::AddInitFunction(Init);

		Game::AddInitFunction(DataInit);
		Game::AddClearFunction(DataClear);
	}

	static void	Init()
	{
//		Model::AddObjectLoader(1, TreeLoader);
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

	static void	DataClear()
	// Delete nodes.
	{
		// Clear out ClumpGrid.
		int	i, j;
		for (j = 0; j < Z_DIM; j++) {
			for (i = 0; i < X_DIM; i++) {
				ClumpNode*	c = ClumpGrid[j][i];
				while (c) {
					ClumpNode*	n = c->next;
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
#ifdef NOT
		if (dummy == NULL) dummy = new Tree();

		dummy->LoadLocation(fp);

		// Initialize location, visual, solid.
		LoadLocation(fp);

		// compute indices.
		int	xindex = (dummy->GetLocation().X() - X_ORIGIN) / SPACING;
		int	zindex = (dummy->GetLocation().Z() - Z_ORIGIN) / SPACING;

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
				if (n->clump->Stamp == Stamp && n->clump->Solid == Solid) {
					c = n->clump;
					break;
				}
			}

			if (c == NULL) {
				// No previously existing clump is appropriate, so create a new one and put it in the grid.
				c = new TreeClump(Stamp, Solid);
				n = new ClumpNode;
				n->clump = c;
				n->next = ClumpGrid[zindex][xindex];
				ClumpGrid[zindex][xindex] = n;

				Model::AddStaticObject(c);
			}

			c->AddTree(dummy->GetLocation());
		}
#endif // NOT
	}

} InitTreeClump;

