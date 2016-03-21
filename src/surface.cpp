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
// surface.cpp	-thatcher 8/11/1998 Copyright Thatcher Ulrich

// Code for maintaining a model of a polygonal tiling of a terrain region
// into different surface types.  Contains functions for querying the surface
// type, and for generating textures for regions of the terrain.


#include "surface.hpp"
#include "error.hpp"
#include "psdread.hpp"
#include "utility.hpp"
#include "terrain.hpp"
#include "timer.hpp"
#include "config.hpp"
#include "imageoverlay.hpp"

#include <assert.h>

namespace Surface {
;


// Table of surface types.

const int	SURFACE_IMAGE_BITS = 7;	// (1 << this) == surface bitmap size == 128


struct SurfaceTypeInfo {
	bitmap32*	Image[SURFACE_IMAGE_BITS + 1];	// 256x256, 128x128, 64x64, 32x32, 16x16, 8x8, 4x4, 2x2, 1x1.
	// int scale;
	// x,z offsets;
	// physical properties of the surface;
};


void	Open()
// Initialization stuff.
{
	// Init the surface shader module.
	OpenSurfaceShader();
}


static void	FreeAvailableTextures();


void	Close()
// Stuff to do on shutdown.
{
	Clear();
	FreeAvailableTextures();
}


void	Clear()
// Cleans out the surface cache.
{
	// Delete all the cached surfaces.
	FlushCache();
}


// Cache of surface textures.

// For accounting.
int	TexelCount = 0;


int	GetTexelCount()
// Returns current texel usage by the surface cache.
{
	return TexelCount;
}


static const int	MAX_TEXTURE_COUNT = 3000000 / (1 << (TEXTURE_BITS * 2));
static const int	TEXTURE_COUNT_MARGIN = MAX_TEXTURE_COUNT / 20;


// Simple stack to temporarily hold unused textures until they're ready to be reused.
static const int	MAX_AVAIL_TEXTURES = MAX_TEXTURE_COUNT + TEXTURE_COUNT_MARGIN;
static Render::Texture*	AvailableTextures[MAX_AVAIL_TEXTURES];
static int	AvailableTextureCount = 0;


void	FreeAvailableTextures()
// Delete the unused textures (do this prior to exiting the program).
{
	int	i;
	for (i = 0; i < AvailableTextureCount; i++) {
		delete AvailableTextures[i];
	}
	AvailableTextureCount = 0;
}


#ifndef NOT

// Really simple array, partly indexed by a hash-key.  Fixed-size set
// of elements for each possible key.  Kind of like a processor cache.
const int	HASH_BITS = 13;
const int	XBITS = HASH_BITS >> 1;
const int	ZBITS = HASH_BITS - XBITS;
const int	PROBE_COUNT = 15;
const int	CACHE_SIZE = (1 << HASH_BITS);
const int	KEY_MASK = (1 << HASH_BITS) - 1;


// Diagnostic.
int	SurfaceThrashCount = 0;


struct CacheNode {
	int	FrameLastUsed;
	uint16	xindex, zindex;
	int8	level;
	Render::Texture*	Texture;

	CacheNode() {
		FrameLastUsed = -1;
		xindex = zindex = 0;
		level = -1;
		Texture = NULL;
	}

	void	Clear()
	// Cleanup this node.
	{
		if (Texture) {
			if (AvailableTextureCount < MAX_AVAIL_TEXTURES) {
				AvailableTextures[AvailableTextureCount++] = Texture;
			} else {
				TexelCount -= Texture->GetTexelCount();
				delete Texture;
			}
		}
		Texture = NULL;
		FrameLastUsed = -1;
		level = -1;
	}
	
} Cache[CACHE_SIZE];


static int	ActiveNodeCount = 0;
static int	NodesBuilt = 0;


int	GetActiveNodeCount() { return ActiveNodeCount; }
int	GetThrashCount() { return SurfaceThrashCount; }
int	GetNodesBuilt() { return NodesBuilt; }


int	NodeBuildLimit = 3;


int	HashFunction(int xindex, int zindex, int level)
// Returns a hash key, given the parameters of a block.
{
#if 1
	// This is a poor hash for this purpose.
	int	xcomponent = (xindex >> level);
	int	zcomponent = (zindex >> level);
	return (xcomponent + zcomponent * 97 + level * 193) & KEY_MASK;
#endif // 0

#if 0
	// This is a good hash, attributed to djb.
	struct {
		int	x, z, level;
	} s;
	assert(sizeof(s) == sizeof(s.x) + sizeof(s.z) + sizeof(s.level));

	s.x = xindex;
	s.z = zindex;
	s.level = level;

	int	hash = 0;
	const char*	p = (const char*) &s;
	int	size = sizeof(s);
	while (size > 0)
	{
		hash = ((hash << 5) + hash) ^ *p;
		p++;
		size--;
	}
	return hash & KEY_MASK;
#endif // 0

#if 0
	// This is actually a good hash for this specific use.
	// It probably sucks for anything else.
	int	xcomponent = (xindex >> level);
	int	zcomponent = (zindex >> level);

	xcomponent = ((xcomponent) ^ ((xcomponent) << 4)) & ((1 << XBITS) - 1);
	zcomponent = (((zcomponent) ^ ((zcomponent) << 2)) & ((1 << ZBITS) - 1)) << XBITS;
	
	return ((xcomponent + zcomponent) + level*13 + (level << XBITS)) & KEY_MASK;
#endif // 0
}


CacheNode*	GetCacheNode(int* pxindex, int* pzindex, int* plevel, int FrameNumber)
// Looks up the desired cache node, or creates it if it's not in the
// cache already.
// If the node is not in the cache, and we've already built the allowed number
// of new nodes for this frame, then returns a lower-res parent node, and adjusts
// *pxindex, *pzindex, and *plevel accordingly.
{
	int	i;

	int	AdjustCount = 3;
	while (1) {
		int	key = HashFunction(*pxindex, *pzindex, *plevel);
		int	index = key;
		
		CacheNode*	OldestNode = NULL;
		int	OldestAge = FrameNumber;
		
		CacheNode*	p;
		for (i = 0; i < PROBE_COUNT; i++, index = (index+1)&KEY_MASK) {
			p = &Cache[index];
			
			// Is this the node we're looking for?
			if (p->xindex == *pxindex && p->zindex == *pzindex && p->level == *plevel) {
				return p;
			}

			// This is not the droid we're looking for.
			
//x			// Trim dead wood while we're traversing the cache.
//			if (AvailableTextureCount < MAX_AVAIL_TEXTURES && p->Texture && FrameNumber - p->FrameLastUsed > 50)
//			{
//				// Free this node.
//				p->Clear();
//				ActiveNodeCount--;
//			}

			// See if this is the best replacement node so far.
			if (p->FrameLastUsed < OldestAge) {
				OldestNode = p;
			}
		}

		// Node does not already exist.  If we've already built our allowed number
		// of new nodes, then look again for a higher-level node.
		if (AdjustCount > 0 && NodesBuilt >= NodeBuildLimit && *plevel < 16) {
			(*plevel)++;
			int	mask = ~((1 << *plevel) - 1);
			*pxindex &= mask;
			*pzindex &= mask;
			
			AdjustCount--;
			continue;
		}
		
		// Must create a new node.  Kick out an existing node if necessary.
		CacheNode*	NewNode = OldestNode;
		
		// Did we find a suitable node to replace?
		if (NewNode == NULL) {
			// xxx we're actually not very happy if this happens, because it means the cache is thrashing.
			// xxx should maybe have a diagnostic or something.
			SurfaceThrashCount++;

			// Just arbitrarily replace the first node in the set.
			NewNode = &Cache[index];
		}
		
		// Clean up the node to be replaced.
		if (NewNode->Texture) {
			if (AvailableTextureCount < MAX_AVAIL_TEXTURES) {
				AvailableTextures[AvailableTextureCount++] = NewNode->Texture;
			} else {
				TexelCount -= NewNode->Texture->GetTexelCount();
				delete NewNode->Texture;
			}
			NewNode->Texture = NULL;
		} else {
			// Bump the node count, since we're not replacing an older node.
			ActiveNodeCount ++;
		}
		
		NewNode->xindex = *pxindex;
		NewNode->zindex = *pzindex;
		NewNode->level = *plevel;

		NodesBuilt++;
		
		return NewNode;
	}
}


void	FlushCache()
// Clears all the nodes in the cache.
{
	int	i;
	for (i = 0; i < CACHE_SIZE; i++) {
		CacheNode*	p = &Cache[i];
		if (p->Texture) {
			p->Clear();
			ActiveNodeCount--;
		}
	}
}


void	DeleteNodes(int count, int FrameNumber)
// Deletes the specified number of nodes from the cache.
{
	int	i;
	
	int	MinimumAge = 50;

	// Start our linear search through the cache at a random place so we don't
	// disproportionately clean out one end or the other.
	int	StartIndex = rand() % CACHE_SIZE;
	
	while (count > 0) {
		int	index = StartIndex;
		CacheNode*	p = Cache + index;
		for (i = 0; i < CACHE_SIZE; i++) {
			if (p->Texture && FrameNumber - p->FrameLastUsed > MinimumAge) {
				p->Clear();
				ActiveNodeCount--;
				count--;
				if (count <= 0) break;
			}

			index++;
			p++;
			if (index >= CACHE_SIZE) {
				index = 0;
				p = Cache;
			}
		}

		if (MinimumAge == 0) {
			// We're out of nodes to delete.
			return;
		}

		MinimumAge -= 10;
		if (MinimumAge < 0) MinimumAge = 0;
	}
}


int	CountActiveImages(int FrameNumber)
// Returns the number of surface images in the cache that have been used
// for rendering since the specified frame.
{
	return 0;	//xxxxxxxxx
	
	int	i;
	int	count = 0;
	for (i = 0; i < CACHE_SIZE; i++) {
		if (Cache[i].FrameLastUsed >= FrameNumber) count++;
	}
	return count;
}


#else


static int	ActiveNodeCount = 0;
static int	NodesBuilt = 0;

int	GetActiveNodeCount() { return ActiveNodeCount; }
int	GetThrashCount() { return 0; }
int	GetNodesBuilt() { return NodesBuilt; }



class	CacheNode /* : public TexNode */ {
public:
	Render::Texture*	Texture;
	CacheNode*	Child[4];	// quadtree children.
	int	FrameLastUsed;
	int	FrameChildLastUsed;
	
//	TexNodeImp*	Parent;	// quadtree parent.
//	int	LastTouched;

	CacheNode() {
		ActiveNodeCount++;
		Texture = NULL;
		for (int i = 0; i < 4; i++) Child[i] = NULL;
		FrameLastUsed = -1;
		FrameChildLastUsed = -1;
	}
	~CacheNode() {
		ActiveNodeCount--;
		Clear();
		for (int i = 0; i < 4; i++) {
			if (Child[i]) delete Child[i];
		}
	}

	void	Clear()
	// Cleanup this node and child nodes.
	{
		if (Texture) {
			if (AvailableTextureCount < MAX_AVAIL_TEXTURES) {
				AvailableTextures[AvailableTextureCount++] = Texture;
			} else {
				TexelCount -= Texture->GetTexelCount();
				delete Texture;
			}
		}
		Texture = NULL;
		FrameLastUsed = -1;

		for (int i = 0; i < 4; i++) {
			if (Child[i]) Child[i]->Clear();
		}
		FrameChildLastUsed = -1;
	}
};


CacheNode*	CacheRoot = NULL;


static int	NODE_BUILD_LIMIT = 4;


struct CacheStackInfo {
	int	xindex, zindex, scale;
	CacheNode*	node;
} CacheStack[16];
int	CacheSP = -1;


CacheNode*	GetCacheNode(int* pxindex, int* pzindex, int* plevel, int FrameNumber)
// Looks up the desired cache node, or creates it if it doesn't exist.
// If the node doesn't exist and we've already built the allowed number of
// new nodes for this frame, then returns a lower-res parent node and
// adjusts *pxindex, *pzindex and *plevel accordingly.
{
	bool	DontBuild = false;
	if (NodesBuilt >= NODE_BUILD_LIMIT) {
		DontBuild = true;
	}

	// Find a common ancestor on the iterator stack.
	if (CacheSP < 0) {
		// Initialize.
		CacheSP = 0;
		CacheStack[0].xindex = CacheStack[0].zindex = 0;
		CacheStack[0].scale = 16;
		if (CacheRoot == NULL) CacheRoot = new CacheNode();
		CacheStack[0].node = CacheRoot;
	}

	for (;;) {
		CacheStackInfo&	c = CacheStack[CacheSP];
		if (c.scale >= *plevel) {
			int	mask = ~((1 << c.scale) - 1);
			if ((c.xindex & mask) == (*pxindex & mask) && (c.zindex & mask) == (*pzindex & mask)) {
				// OK; CacheSP is at a valid ancestor.
				break;
			}
		}
		CacheSP--;
	}

	// Follow the tree down to the desired level.
	for (;;) {
		CacheStackInfo&	c = CacheStack[CacheSP];
		
		int	scale = c.scale;
		int	mask = 1 << (c.scale - 1);
		CacheNode*	Node = c.node;
	
		if (scale <= *plevel) {
			// Return this node.
			Node->FrameLastUsed = FrameNumber;
			if (Node->Texture == NULL) NodesBuilt++;
			return Node;
		}

		// Figure out which child to go to next.
		int	index = ((*pzindex & mask) ? 2 : 0) | ((*pxindex & mask) ? 1 : 0);
		if (Node->Child[index] == NULL) {
			if (DontBuild) {
				// Not allowed to build.  Just return this parent node.
				*plevel = scale;
				mask = ~((mask << 1) - 1);
				*pxindex &= mask;
				*pzindex &= mask;
				Node->FrameLastUsed = FrameNumber;
				if (Node->Texture == NULL) NodesBuilt++;
				return Node;
			} else {
				// Build the child node.
				Node->Child[index] = new CacheNode();
			}
			Node->FrameLastChildUsed = FrameNumber;
		}

		// Iterate to next tree level.
		CacheSP++;
		CacheStack[CacheSP].node = Node->Child[index];
		CacheStack[CacheSP].scale = scale - 1;
		mask = ~(mask - 1);
		CacheStack[CacheSP].xindex = *pxindex & mask;
		CacheStack[CacheSP].zindex = *pzindex & mask;
	}
}


int	DeleteNodes(CacheNode* n, int count, int FrameNumber)
// Traverse the subtree oldest-first, deleting the specified number of
// unused nodes.
// Returns number of nodes deleted.
{
	int	deleted = 0;
	
	// Recurse to children first.
	int	index[4], age[4];
	int	i, j;
	
	for (i = 0; i < 4; i++) {
		if (n->Child[i]) {
			age[i] = FrameNumber - n->Child[i]->FrameChildLastUsed;
			index[i] = i;
		} else {
			age[i] = -1;
			index[i] = -1;
		}
	}

	// Sort ages.
	for (j = 0; j < 3; j++) {
		for (i = 0; i < j; i++) {
			if (age[i] < age[i+1]) {
				// Swap.
				int	temp = age[i];
				age[i] = age[i+1];
				age[i+1] = temp;
				temp = index[i];
				index[i] = index[i+1];
				index[i+1] = temp;
			}
		}
	}

	// Visit children.
	for (i = 0; i < 4; i++) {
		if (index[i] >= 0) {
			deleted += DeleteNodes(n->Child[index[i]], count - deleted, FrameNumber);
			if (deleted >= count) return deleted;
		}
	}

	FrameChildLastUsed = -1;
	for (i = 0; i < 4; i++) {
		if (n->Child[i]) {
			if (n->Child[i]->FrameLastUsed > FrameChildLastUsed) {
			}
		}
	}
	
	// Now see if we should delete this texture.
	if (count > deleted) {
	}
}


void	DeleteNodes(int count, int FrameNumber)
{
	// Invalidate cached requests since we're about to muck with the cache tree.
	CacheSp = -1;

	// Traverse the tree oldest-first, deleting unused nodes.
	DeleteNodes(CacheRoot, int count, int FrameNumber);
}


void	FlushCache()
// Flush out all cache nodes.
{
	if (CacheRoot) {
		delete CacheRoot;
		CacheRoot = NULL;
	}
	CacheSP = -1;
}


#endif // NOT


// Functions to build and manage textures.


Render::Texture*	GetSurfaceTexture(int* pxindex, int* pzindex, int* plevel, int FrameNumber)
// Returns a texture that tiles the specified block of terrain.
// level == power of 2 scaling factor.
// FrameNumber is used as a hint for the cache-replacement algorithm.
//
// Sets *pxindex, *pzindex, and *plevel to the description of the actual block that was
// returned.  Because it uses a caching mechanism, sometimes the desired surface is not
// immediately available, so a lower-resolution parent texture is returned instead.
{
	static int	LastFrameNumber = 0;
	if (LastFrameNumber != FrameNumber) {

		//xxxxxxxx
		// Dump hash keys, for testing.
		static bool	DumpedKeys = false;
		if (DumpedKeys == false
			&& LastFrameNumber > 100
			&& NodesBuilt == 0)
		{
			// Dump keys.
			DumpedKeys = true;
			FILE* dump = fopen("hash_keys.txt", "w");
			if (dump)
			{
				{for (int i = 0; i < CACHE_SIZE; i++)
				{
					if (Cache[i].Texture != NULL)
					{
						fprintf(dump, "%d %d %d\n", Cache[i].xindex, Cache[i].zindex, Cache[i].level);
					}
				}}
				fclose(dump);
			}
		}
		//xxxxxxxx

		NodesBuilt = 0;
		LastFrameNumber = FrameNumber;

		//xxxx
//xxxx		FlushCache();
		//xxxx

		// xxxx Show cache usage.
		if (Config::GetBool("ShowCacheUsage")) {
			static Render::Texture*	ctex = NULL;
			static bitmap32*	bmap = NULL;

			int	w = 1 << XBITS;
			int	h = 1 << ZBITS;
			if (ctex == NULL) {
				bmap = new bitmap32(w, h);
				ctex = Render::NewTextureFromBitmap(bmap, false, false, false);
			}

			int	i;
			uint32*	p = bmap->GetData();
			for (i = 0; i < CACHE_SIZE; i++) {
				unsigned char	color[4] = { 0, 0, 0, 0};
				if (Cache[i].Texture) {
					int	age = 255 - iclamp(0, FrameNumber - Cache[i].FrameLastUsed, 255);
					color[3] = 0xFF;
					color[0] = color[1] = color[2] = age;
//					color = i * 0x0010001;//xxxxxx
//					color = 0xFFFFFFFF;//xxxx
				}
				*p++ = color[0] | color[1] << 8 | color[2] << 16 | color[3] << 24;
			}
			ctex->ReplaceImage(bmap);

			ImageOverlay::Draw(0, 352, w, h, ctex, 0, 0, 0xFFFFFFFF);
		}

		NodeBuildLimit = Config::GetInt("SurfaceNodeBuildLimit");
	}
	
	
	// Make sure texture request is equal to or larger than the minimum size.
	const int	MIN_TEXTURE_LEVEL = TEXTURE_BITS - TEXTURE_BITS_PER_METER;
	if (*plevel < MIN_TEXTURE_LEVEL) {
		// Substitute a higher level request.
		*plevel = MIN_TEXTURE_LEVEL;
		*pxindex &= ~((1 << MIN_TEXTURE_LEVEL) - 1);
		*pzindex &= ~((1 << MIN_TEXTURE_LEVEL) - 1);
	}
	
	static CacheNode*	Memo = NULL;	// Lamo memo-ization optimization.  Not active at the moment.

	// Empty some of the image cache if necessary.
	if (ActiveNodeCount > MAX_TEXTURE_COUNT) {
		DeleteNodes(ActiveNodeCount - MAX_TEXTURE_COUNT + TEXTURE_COUNT_MARGIN, FrameNumber);
	}

	//int	HashKey = HashFunction(*pxindex, *pzindex, *plevel);
	
	// Retrieve or create the needed cache block node.
	CacheNode*	node = GetCacheNode(pxindex, pzindex, plevel, FrameNumber);

	int	level = *plevel;
	int	xindex = *pxindex;
	int	zindex = *pzindex;
	
	if (node->Texture == NULL) {
		// Build texture.
		
		static uint8	LightMap[TEXTURE_SIZE][TEXTURE_SIZE];
		static uint8	TypeMap[TEXTURE_SIZE][TEXTURE_SIZE];
		static uint16	NoiseMap[TEXTURE_SIZE][TEXTURE_SIZE];
		static bitmap32*	b = NULL;

//		static Render::Texture*	tex = NULL;//x
		if (b == NULL) {
			b = new bitmap32(TEXTURE_SIZE, TEXTURE_SIZE);
//			tex = Render::NewTextureFromBitmap(b, false, true, false);
		}

		// Paint the array.
		int	tx = WorldToTex(xindex);
		int	tz = WorldToTex(zindex);
		GenerateNoise(NoiseMap, tx, tz, level + Surface::TEXTURE_BITS_PER_METER);
		TerrainModel::GetMaps(LightMap, TypeMap, tx, tz, level + Surface::TEXTURE_BITS_PER_METER, NoiseMap);

		GenerateTexture(b, tx, tz, level + Surface::TEXTURE_BITS_PER_METER, LightMap, TypeMap, NoiseMap);

//		node->Texture = tex;//x
		// Register the new image as a texture map.
		if (AvailableTextureCount) {
			node->Texture = AvailableTextures[--AvailableTextureCount];
			node->Texture->ReplaceImage(b);
		} else {
			node->Texture = Render::NewTextureFromBitmap(b, false, true, false);
			TexelCount += node->Texture->GetTexelCount();
		}
	}

	node->FrameLastUsed = FrameNumber;

	// For lamo optimization.
	Memo = node;
	
	return node->Texture;
}


};

