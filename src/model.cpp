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
// model.cpp	-thatcher 5/16/1998 Copyright Slingshot

// Code for maintaining the object database.


#include <math.h>

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif


#include <ctype.h>
#include "ogl.hpp"
#include "error.hpp"
#include "utility.hpp"
#include "model.hpp"
#include "clip.hpp"
#include "game.hpp"
#include "ui.hpp"

#include "multiplayer.hpp"

typedef BagOf<Render::Texture*> texbag;


namespace Model {
;


//
// Texture management.
//


texbag	Textures;
int	TexelCount = 0;


::Render::Texture*	GetTexture(const char* filename, bool NeedAlpha, bool MakeMIPMaps, bool Tile)
// Returns the texture loaded from the image data in the given file.
// Maintains a database of previously loaded textures, to prevent
// re-loading and re-allocating textures more than once.
// Returns NULL or throws an exception on error.
{
	// Lower-case the filename.
	char	temp[1000];
	strncpy(temp, filename, 1000);
	temp[1000-1] = 0;
	Utility::StringLower(temp);

	// First, see if it's in our bag.
	::Render::Texture*	result;
	if (Textures.GetObject(&result, temp)) {
		// Return pre-existing texture.
		return result;
	} else {
		// Try to create the texture.
		result = ::Render::NewTexture(temp, NeedAlpha, MakeMIPMaps, Tile);
		if (result) {
			// Remember for later.
			Textures.Add(result, temp);
			// Keep track of texel usage.
			TexelCount += result->GetTexelCount();
		}
		return result;
	}
}


int	GetTexelCount()
// Returns the number of texels used by model textures.
{
	return TexelCount;
}


//
// Code for handling GModel stuff
//


GModel**	GModelList = NULL;
int	GModelCount = 0;
BagOf<GModelLoaderFP>	GModelLoaders;


void	AddGModelLoader(const char* Type, GModelLoaderFP Loader)
// Adds the given function to our bag of GModel loaders.  Associates
// it with the given Type string, which is just the file extension of the
// type of file that the loader loads.
{
	GModelLoaders.Add(Loader, Type);
}


void	LoadGModels(FILE* fp)
// Loads a list of GModels from the given file.
{
	int i;
	
	// Read the GModel count.
	GModelCount = Read32(fp);
	GModelCount++;	// Add one, to account for GModelList[0] which will be NULL always.
	
	// Create the array to store GModel pointers.
	GModelList = new GModel*[GModelCount];

	// Create the GModels, and store pointers to them.

	GModelList[0] = NULL;	// First GModel is always NULL.
	
	for (i = 1; i < GModelCount; i++) {
		char	temp[80], filename[80];
		
		// Ignore name.
		fgets(temp, 80, fp);

		// Read filename.
		fgets(filename, 80, fp);
		// Delete trailing '\n'.
		filename[strlen(filename) - 1] = 0;
		// Convert to lower-case.
		Utility::StringLower(filename);

		// Ignore comment.
		fgets(temp, 80, fp);

		// Create and initialize, according to the filename.
		GModel*	g = LoadGModel(filename);
		
		GModelList[i] = g;
	}
}


GModel*	LoadGModel(const char* filename)
// Initializes a GModel from the specified file.  The filename extension
// determines the type of GModel to create.
// Returns a pointer to the new GModel.
{
	// Initialize geometry using filename.
	GModelLoaderFP	init;
	const char*	Type = Utility::GetExtension(filename);
	bool	success = GModelLoaders.GetObject(&init, Type);
	if (!success) {
		Error e; e << "No loader for GModel of type '" << Type << "'.";
		throw e;
	}
	
	return (*init)(filename);
}


GModel*	GetGModel(int index)
// Returns the indexed GModel, or NULL if the index is invalid.
// Index 0 should always return NULL as well.
{
	if (index < 0 || index >= GModelCount) {
		return NULL;
	} else {
		return GModelList[index];
	}
}


//
// Code for handling SModel stuff
//


SModel**	SModelList = NULL;
int	SModelCount = 0;

// Code for maintaining a list of model loaders, indexed by TypeID.
struct SModelLoaderBag {
	struct Node {
		SModelLoaderFP	Loader;
		int	TypeID;
		Node*	Next;
	};
	Node*	List;

	SModelLoaderBag() { List = NULL; }
	~SModelLoaderBag() {
		Empty();
	}

	void	Empty() {
		while (List) {
			Node*	Next = List->Next;
			delete List;
			List = Next;
		}
		List = NULL;
	}

	void	Add(SModelLoaderFP Loader, int TypeID)
	// Adds the specified loader to the bag, indexed by the given TypeID.
	// *** Doesn't check to see if the ID has already been used.
	{
		Node*	n = new Node;
		n->Loader = Loader;
		n->TypeID = TypeID;
		n->Next = List;
		List = n;
	}

	SModelLoaderFP	GetLoader(int TypeID)
	// Returns the loader function associated with the given TypeID.  Returns NULL
	// if there's no loader for that type.
	// *** Really dumb linear search.
	{
		for (Node* n = List; n; n = n->Next) {
			if (n->TypeID == TypeID) return n->Loader;
		}
		return NULL;
	}
} SModelLoaders;


void	AddSModelLoader(int TypeID, SModelLoaderFP Loader)
// Adds the given function to our bag of SModel loaders.  Associates
// it with the given TypeID.
{
	SModelLoaders.Add(Loader, TypeID);
}


void	LoadSModels(FILE* fp)
// Loads a list of SModels from the given file.
{
	int i;
	
	// Read the SModel count.
	SModelCount = Read32(fp);
	SModelCount++;	// Add one, to account for SModelList[0] which will be NULL always.
	
	// Create the array to store SModel pointers.
	SModelList = new SModel*[SModelCount];

	// Create the SModels, and store pointers to them.

	SModelList[0] = NULL;	// First SModel is always NULL.
	
	for (i = 1; i < SModelCount; i++) {
		Game::LoadingTick();
		
		char	temp[80];
		
		// Ignore name.
		fgets(temp, 80, fp);

		// Ignore comment.
		fgets(temp, 80, fp);

		// Get the type.
		byte	TypeID;
		fread(&TypeID, sizeof(byte), 1, fp);
		
		SModelLoaderFP	init = SModelLoaders.GetLoader(TypeID);
		
		if (init == NULL) {
			Error e; e << "No loader for SModel of type " << TypeID << ".";
			throw e;
		}

		SModel*	s = (*init)(fp);

		SModelList[i] = s;
	}
}


SModel*	GetSModel(int index)
// Returns the indexed SModel, or NULL if the index is invalid.
// Index 0 should always return NULL as well.
{
	if (index < 0 || index >= SModelCount) {
		return NULL;
	} else {
		return SModelList[index];
	}
}


//
// Code for handling MObject stuff
//

	
MObject*	StaticObjects = 0;
MObject*	DynamicObjects[MultiPlayer::MAX_PLAYERS] = {0, 0, 0, 0};


// Table of object loaders, indexed by type id.
typedef void (*LoaderFP)(FILE*);
const int	TYPECOUNT = 20;
LoaderFP	Loader[TYPECOUNT];


// Quad-tree, for holding static objects.
const float	TotalQuadTreeSize = 65536;
const int	QuadTreeLevelCount = 12;
struct QuadTreeNode {
	QuadTreeNode(QuadTreeNode* parent, int level) {
		// Initialize sizes & stuff.
		Parent = parent;
		Level = level;
		ObjectList = NULL;
		MinY = 1000000;
		MaxY = -1000000;

		int	i, j;
		for (j = 0; j < 2; j++) {
			for (i = 0; i < 2; i++) {
				Child[j][i] = NULL;
			}
		}
	}

	~QuadTreeNode() {
		Empty();
	}

	void	Empty()
	// Delete objects and sub-nodes.
	{
		MObject*	e = ObjectList;
		while (e) {
			MObject*	n = e->Next;
			delete e;
			e = n;
		}
		ObjectList = NULL;

		int	i, j;
		for (j = 0; j < 2; j++) {
			for (i = 0; i < 2; i++) {
				if (Child[i][j]) delete Child[j][i];
				Child[j][i] = NULL;
			}
		}
	}

	void	Render(ViewState& s, float xorigin, float zorigin, float size, int ClipHint, float MaxDistance, bool NoCheckDistance)
	// Render visible objects in this node and in sub-nodes.
	{
		// Do culling checks.
		if (ClipHint != Clip::NO_CLIP) {
			vec3	min(xorigin, MinY, zorigin);
			vec3	max(xorigin + size, MaxY, zorigin + size);
			ClipHint = Clip::ComputeBoxVisibility(min, max, s, ClipHint);
			if (ClipHint == Clip::NOT_VISIBLE) return;
		}

		float	HalfSize = size * 0.5f;
		
		// Do distance check.
		if (NoCheckDistance == false) {
			float	x = (xorigin + HalfSize) - s.Viewpoint.X();
			float	y = (MaxY + MinY) * 0.5f - s.Viewpoint.Y();
			float	z = (zorigin + HalfSize) - s.Viewpoint.Z();
			float	dist = sqrtf(x * x + y * y + z * z);
			float	yextent = (MaxY - MinY) * 0.5f;
			float	r = sqrtf(HalfSize * size + yextent * yextent);	// r is max radius of the block.
			
			if (dist + r < MaxDistance) {
				// Block is within max distance, so don't bother checking distance
				// of contained blocks or objects from now on.
				NoCheckDistance = true;
			} else if (dist - r > MaxDistance) {
				// Block is fully outside the max view distance, so don't render its contents.
				return;
			}
		}
			    
		// Render objects in this node.
		if (NoCheckDistance == false) {
			// Check each object to make sure it's within the max distance before rendering.
			vec3	v;
			float	d2 = MaxDistance * MaxDistance;
			for (MObject* e = ObjectList; e; e = e->Next) {
				v = e->GetLocation();
				v -= s.Viewpoint;
				if (v * v < d2) {
					e->Render(s, ClipHint);
				}
			}
		} else {
			// Render all the objects.
			for (MObject* e = ObjectList; e; e = e->Next) {
				e->Render(s, ClipHint);
			}
		}

		// Recurse to child nodes that exist.
		for (int j = 0; j < 2; j++) {
			for (int i = 0; i < 2; i++) {
				QuadTreeNode*	q = Child[j][i];
				if (q) {
					q->Render(s, xorigin + HalfSize * i, zorigin + HalfSize * j, HalfSize, ClipHint, MaxDistance, NoCheckDistance);
				}
			}
		}
	}
	
	void	AddObject(MObject* o, int level, int x, int z)
	// Adds an object to this node or a child node.  Level encodes the number
	// of levels below the current one, and x and z encode the relative block coordinates
	// of the eventual node for the object.
	{
		if (level == 0) {
			// Add object to this node.
			AddObject(o);
		} else {
			// Figure out which child to forward to...
			int	shift = level - 1;
			int	j = (z >> shift) & 1;
			int	i = (x >> shift) & 1;
			// Allocate the child node if it doesn't exist already...
			if (Child[j][i] == NULL) {
				Child[j][i] = new QuadTreeNode(this, Level + 1);
			}
			// Pass the object down to the child...
			Child[j][i]->AddObject(o, level - 1, x, z);
		}
	}
	
	void	AddObject(MObject* o)
	// Adds the object to this node.  Recalculates min/max y for the node.
	{
		// Link in the object.
		o->Next = ObjectList;
		o->Previous = NULL;
		if (ObjectList) {
			ObjectList->Previous = o;
		}
		ObjectList = o;

		// Recalc min/max y.
		bool	MinMaxChanged = false;
		MObject*	p = o;
//		for (MObject* p = ObjectList; p; p = p->Next) {
			float	radius = p->GetRadius();
			float	min = p->GetLocation().Y() - radius;
			float	max = p->GetLocation().Y() + radius;

			if (min < MinY) {
				MinY = min;
				MinMaxChanged = true;
			}
			if (max > MaxY) {
				MaxY = max;
				MinMaxChanged = true;
			}
//		}

		// Notify parent node if our min or max has changed, so it can update if necessary.
		if (MinMaxChanged && Parent != NULL) {
			Parent->ChildMinMaxChanged(MinY, MaxY);
		}
	}

	void	ChildMinMaxChanged(float min, float max)
	// Called by a child node when its min/max y change.  Check to see if
	// we should update our own min/max y.
	{
		bool	Changed = false;
		if (min < MinY) {
			MinY = min;
			Changed = true;
		}
		if (max > MaxY) {
			MaxY = max;
			Changed = true;
		}

		// Notify parent if min/max changed.
		if (Changed && Parent != NULL) {
			Parent->ChildMinMaxChanged(MinY, MaxY);
		}
	}

	bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl, float xorigin, float zorigin, float size)
	// Looks for a contact between the given cylinder swept along the given segment,
	// and the solid of any object contained within this node or its children.
	// If a contact is found, returns true and fills in the *result info;
	// otherwise just returns false.
	{
		// First, see if the segment touches this node at all.
		Collide::BoxInfo	b(vec3(xorigin - cyl.Radius, MinY - cyl.Height / 2, zorigin - cyl.Radius), vec3(xorigin + size + cyl.Radius, MaxY + cyl.Height / 2, zorigin + size + cyl.Radius));
		Collide::ContactInfo	temp;
		if (Collide::CheckForBoxContact(&temp, seg, b) == false) return false;

		bool	FoundContact = false;
		
		// Check against objects within this node.
		Collide::SegmentInfo	tempseg(seg);
		for (MObject* o = ObjectList; o; o = o->Next) {
			if (o->CheckForContact(result, tempseg, cyl)) {
				// Found a contact.  Reduce the segment length for further checks.
				FoundContact = true;
				tempseg.LimitTime = result->EnterTime;
			}
		}

		// Recurse to child nodes that exist.
		float	ChildSize = size * 0.5f;
		for (int j = 0; j < 2; j++) {
			for (int i = 0; i < 2; i++) {
				QuadTreeNode*	q = Child[j][i];
				if (q && q->CheckForContact(result, seg, cyl, xorigin + ChildSize * i, zorigin + ChildSize * j, ChildSize)) {
					FoundContact = true;
					tempseg.LimitTime = result->EnterTime;
				}
			}
		}

		return FoundContact;
	}
	
	QuadTreeNode*	Parent;
	int	Level;
	QuadTreeNode*	Child[2][2];
	MObject*	ObjectList;
	float	MinY, MaxY;
};
QuadTreeNode	StaticRoot(NULL, 0);



void	Open()
// Initialize the model database.
{
	// Initialize the array of object loaders.
	int	i;
	for (i = 0; i < TYPECOUNT; i++) {
		Loader[i] = NULL;
	}
}


void	Close()
// Free everything.
{
	Clear();
}


void	DeleteTexture(::Render::Texture* t, const char* Name)
// A thunk, to help us clean out the bag of textures.  Also does
// some texel accounting.
{
	TexelCount -= t->GetTexelCount();
	delete t;
}


void	Clear()
// Empty the database, in preparation for closing, or for loading new data.
{
	// Delete the static objects.
	StaticRoot.Empty();

	// Delete the dynamic objects.
	for (int i=0; i<MultiPlayer::MAX_PLAYERS; i++){
	  MObject*	e = DynamicObjects[i];
	  while (e) {
	    MObject*	n = e->Next;
	    delete e;
	    e = n;
	  }
	  DynamicObjects[i] = NULL;
	}

	// GModels, SModels.
	int	i;
	for (i = 0; i < GModelCount; i++) {
		delete GModelList[i];
	}
	delete [] GModelList;
	GModelCount = 0;

	for (i = 0; i < SModelCount; i++) {
		delete SModelList[i];
	}
	delete [] SModelList;
	SModelCount = 0;

	// Empty the texture bag.
//	Textures.Apply(DeleteTexture);
	texbag::Iterator	b = Textures.GetIterator();
	while (b.IsDone() == false) {
		TexelCount -= (*b)->GetTexelCount();
		delete (*b);
		b++;
	}
	Textures.Empty();
}


void	Update(const UpdateState& u)
// Call ::Update() on the dynamic objects.
{
  for (int player_index=0; player_index<MultiPlayer::NumberOfLocalPlayers(); player_index++){
	MultiPlayer::SetCurrentPlayerIndex(player_index);
	MObject*	e = DynamicObjects[player_index];
	while (e) {
		e->Update(u);
		e = e->Next;
	}
  }
}


const float	STATIC_MAX_RENDER_DISTANCE = 550.0;

int	TriangleCount = 0;


void	Render(ViewState& s)
// Render the objects.
{
	// Do some accounting.
	static int	LastFrame = -1;
	if (s.FrameNumber != LastFrame) {
		LastFrame = s.FrameNumber;
		TriangleCount = 0;
	}

	// Transform the frustum into world coordinates, to speed up culling.
	Plane	Frustum[6];
	int	i;
	for (i = 0; i < 6; i++) {
		Frustum[i] = s.ClipPlane[i];
	}
	Clip::TransformFrustum(Frustum, s.ViewMatrix);
	
	// Render static objects.  Use the quad-tree structure for quicker frustum culling.
	StaticRoot.Render(s, -32768, -32768, TotalQuadTreeSize, 0, STATIC_MAX_RENDER_DISTANCE, false);
	
	// Render dynamic objects.  No spatial partitioning; just go through the whole list.
	for (int i=0; i<MultiPlayer::NumberOfLocalPlayers(); i++){
	  for (MObject* e = DynamicObjects[i] ; e; e = e->Next) {
		// Do a culling check of the bounding sphere against the frustum.
		GModel*	visual = e->GetVisual();
		if (visual == NULL) continue;
		int	ClipHint = Clip::ComputeSphereVisibility(e->GetLocation(), visual->GetRadius(), Frustum, 0);
		if (ClipHint == Clip::NOT_VISIBLE) continue;
		
		// Object might be visible.  Render it.
		e->Render(s, ClipHint);
	  }
	}
}


int	GetRenderedTriangleCount()
// Returns the number of triangles rendered in the previous frame.
{
	return TriangleCount;
}


void	AddToTriangleCount(int amount)
// Increments our rendered-triangle count by the given amount.
{
	TriangleCount += amount;
}


void	AddStaticObject(MObject* o)
// Adds a static object to the database.
{
	float	x = o->GetLocation().X() + 32768;
	float	z = o->GetLocation().Z() + 32768;
	
	float	radius = o->GetRadius();
	float	MinBlockSize = TotalQuadTreeSize / (1 << (QuadTreeLevelCount - 1));
	int	x0 = int((x - radius) / MinBlockSize);
	int	z0 = int((z - radius) / MinBlockSize);
	int	x1 = int((x + radius) / MinBlockSize);
	int	z1 = int((z + radius) / MinBlockSize);

	// Find the level that contains the extent of the object.
	int	level;
	for (level = QuadTreeLevelCount - 1; level > 0; level--) {
		if (x0 == x1 && z0 == z1) {
			// The object can be contained at this level.
			break;
		}
		x0 >>= 1;
		x1 >>= 1;
		z0 >>= 1;
		z1 >>= 1;
	}

	// Add to the quadtree.
	StaticRoot.AddObject(o, level, x0, z0);
}


void	AddDynamicObject(MObject* o)
// Adds a dynamic object to the database.
{
	int player_index = MultiPlayer::CurrentPlayerIndex();
	o->Next = DynamicObjects[player_index];
	o->Previous = NULL;
	if (DynamicObjects[player_index]) {
		DynamicObjects[player_index]->Previous = o;
	}
	DynamicObjects[player_index] = o;
}


void	LoadObjects(FILE* fp)
// Loads the object section of the input file.
{
	int	ObjectCount = Read32(fp);
	
	for ( ; ObjectCount; ObjectCount--) {
		Game::LoadingTick();
		
		// Read the object type.
		int	TypeID = Read32(fp);
		
		// Dispatch loader code based on object type.
		if (TypeID < 0 || TypeID >= TYPECOUNT) {
			// Illegal object type id.
			// If we implement some way to safely skip this object's data, then we could
			// log a warning and move on, but for now, throw an exception.
			Error	e; e << "Model::Load(): Can't load illegal type id " << TypeID << ".";
			throw e;
		}
		
		LoaderFP	p = Loader[TypeID];
		if (p == NULL) {
			// Can't load this object type, due to lack of a loader function.
			Error	e; e << "Model::Load(): type id " << TypeID << " lacks a loader function.";
			throw e;
		}

		// Call the loader, which should consume the input, create the object
		// and link it to the database.
		(*p)(fp);
	}
}


void	AddObjectLoader(int TypeID, void (*InitFunction)(FILE*))
// Associates the specified object type id with the given function.  When
// a block of data tagged with the given id is encountered in an input file,
// then the given function is called to interpret the data.
{
	// Make sure the given id is within bounds.
	if (TypeID < 0 || TypeID >= TYPECOUNT) {
		Error	e; e << "Model::AddObjectLoader(): type id " << TypeID << " is illegal.";
		throw e;
	}

	// Throw an exception if a loader for this type was already defined.
	if (Loader[TypeID]) {
		Error	e; e << "Model::AddObjectLoader(): type id " << TypeID << " already has a loader defined.";
		throw e;
	}

	// Stick the loader in the appropriate slot.
	Loader[TypeID] = InitFunction;
}


bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl)
// Checks the given cylinder, swept along the given line segment,
// against the solids of all the static objects in the database, looking
// for a first contact.
// If there's a contact, returns true and puts info in *result.  If
// there's no contat, returns false.
{
	// Recurse through quadtree.  Ignore nodes that the segment
	// doesn't touch, and check the objects within nodes that the
	// segment does touch.
	return StaticRoot.CheckForContact(result, seg, cyl, -32768, -32768, TotalQuadTreeSize);
}


};	// end namespace Model


//
// MObject base class functions.
//


MObject::MObject()
// Constructor.  Initialize members.
{
	Next = Previous = NULL;
	Visual = NULL;
	Solid = NULL;
}


MObject::~MObject() {}


void	MObject::Render(ViewState& s, int ClipHint)
// Default implementation.  Compose the object translation with the viewing
// transform, and render using the Visual.
{
	if (Visual == NULL) return;

	// Save the current view matrix.
	glPushMatrix();
	
	// Compose this object's transform.
	const vec3&	l = GetLocation();
	vec3	Save = s.ViewMatrix.GetColumn(3);
	s.ViewMatrix.Translate(l);
	glTranslatef(l.X(), l.Y(), l.Z());

	// Render using the modified matrix.
	Visual->Render(s, ClipHint);

	// Restore the original matrix.
	glPopMatrix();
	s.ViewMatrix.SetColumn(3, Save);
}


void	MObject::LoadLocation(FILE* fp)
// Loads and sets the location from the given file.  Skips the PinToGround flag
// which is assumed to be the next thing in the stream.
// Also initializes the Visual and Solid members using the indexes stored in the file.
// This is the first call an ordinary object should call when being initialized.
{
	// Skip flags.
	int	temp = Read32(fp);	// PinToGround flag.
	
	// Load location vec3.
	float	x = ReadFloat(fp), y = ReadFloat(fp), z = ReadFloat(fp);

	// Offset so that the center of the world is at 0,0.
	x -= 32768;
	z -= 32768;
	
	SetLocation(vec3(x, y, z));

	// Load the Visual index and look up the GModel.
	int	index = Read32(fp);
	Visual = Model::GetGModel(index);

	// Load the Solid index and look up the SModel.
	index= Read32(fp);
	Solid = Model::GetSModel(index);
}


bool	MObject::CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl)
// Checks for a collision between the given cylinder going through the
// motion described by seg, and the solid of this object.  The segment motion
// is in world coordinates.
// Returns true and fills *result if there's a contact; otherwise returns false.
{
	if (Solid == NULL) return false;

	// Transform query into object coordinates.
	Collide::SegmentInfo	temp(seg);

	temp.Start -= GetLocation();
	
//	Matrix.ApplyInverse(&temp.Start, seg.Start);
//	Matrix.ApplyInverseRotation(&temp.Ray, seg.Ray);

	if (Solid->CheckForContact(result, temp, cyl)) {
		// Contact.  Transform back into world coordinates and return.
//		vec3	loc, norm;

//		Matrix.Apply(&loc, result->Location);
		result->Location += GetLocation();
//		Matrix.ApplyRotation(&norm, result->Normal); // norm = result->Normal;

//		result->Location = loc;
//		result->Normal = norm;

		return true;
	} else {
		// No contact.
		return false;
	}
}


void	MObject::Reset(const vec3& Loc, const vec3& Dir, const vec3& Vel)
// Null default implementation.
{
}


float	MObject::GetRadius() const
// Returns the maximum extent of the object, taking into account both visual and solid
// aspects.
{
	// Return whichever is greater; the visual radius or the solid radius.
	float	vr = 0;
	float	sr = 0;

	if (Solid) sr = Solid->GetRadius();
	if (Visual) vr = Visual->GetRadius();

	if (sr > vr) return sr;
	else return vr;
}


//
// MOriented
//


MOriented::MOriented()
// Init members.
{
	Matrix.Identity();
}


void	MOriented::LoadOrientation(FILE* fp)
// Loads and sets the orientation from the given file.
// Currently orientation in the file only consists of yaw (rotation about y axis).
{
	// Load yaw angle.  It's in degrees.
	float	yaw = ReadFloat(fp);

	yaw *= PI / 180;	// Convert to radians.

	// Set the orientation.
	SetDirection(vec3(cosf(yaw), 0, -sinf(yaw)));
	SetUp(YAxis);
}


void	MOriented::Render(ViewState& s, int ClipHint)
// Default implementation.  Compose the object matrix with the viewing
// transform, and render using the Visual.
{
	if (Visual == NULL) return;

	// Save the current view matrix.
	matrix m(s.ViewMatrix);
	glPushMatrix();
	
	// Compose this object's transform.
	::Render::MultMatrix(Matrix);
	matrix::Compose(&s.ViewMatrix, m, Matrix);
	
//	// Copy to the 4x4 layout.
//	float	mat[16];
//	for (int col = 0; col < 4; col++) {
//		for (int row = 0; row < 3; row++) {
//			mat[col * 4 + row] = Matrix.GetColumn(col).Get(row);
//		}
//		if (col < 3) {
//			mat[col * 4 + 3] = 0;
//		} else {
//			mat[col * 4 + 3] = 1;
//		}
//	}
//	// Apply to the current OpenGL matrix.
//	glMultMatrixf(mat);
	
	// Render using the modified matrix.
	Visual->Render(s, ClipHint);

	// Restore the original matrix.
	glPopMatrix();
	s.ViewMatrix = m;
}


bool	MOriented::CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl)
// Checks for a collision between the given cylinder going through the
// motion described by seg, and the solid of this object.  The segment motion
// is in world coordinates.
// Returns true and fills *result if there's a contact; otherwise returns false.
{
	if (Solid == NULL) return false;

	// Transform query into object coordinates.
	Collide::SegmentInfo	temp(seg);

	Matrix.ApplyInverse(&temp.Start, seg.Start);
	Matrix.ApplyInverseRotation(&temp.Ray, seg.Ray);

	if (Solid->CheckForContact(result, temp, cyl)) {
		// Contact.  Transform back into world coordinates and return.
		vec3	loc, norm;

		Matrix.Apply(&loc, result->Location);
		Matrix.ApplyRotation(&norm, result->Normal); // norm = result->Normal;

		result->Location = loc;
		result->Normal = norm;

		return true;
	} else {
		// No contact.
		return false;
	}
}



//
// GModel base class functions.
//


void	GModel::Render(ViewState& s, int ClipHint)
// Default null implementation.
{
}

GModel::~GModel() {}

SModel::~SModel() {}


