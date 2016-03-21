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
// surface.hpp	-thatcher 8/11/1998 Copyright Thatcher Ulrich

// Headers for a module that models the appearance and physics parameters
// of the terrain surface.


#ifndef SURFACE_HPP
#define SURFACE_HPP


#include <stdio.h>
#include "geometry.hpp"
#include "render.hpp"
#include "gameloop.hpp"


namespace Surface {
	void	Open();
	void	Close();
	
//	void	Load(FILE* fp);
	void	Clear();

	void	Update(const UpdateState& u);

	// Debug info.
	int	GetTexelCount();
	int	GetActiveNodeCount();
	int	GetThrashCount();
	int	GetNodesBuilt();
	int	CountActiveImages(int FrameNumber);	//xxxx
	void	FlushCache();
	
	Render::Texture*	GetSurfaceTexture(int* xindex, int* zindex, int* ScaleLevel, int FrameNumber);

	void	CacheIteratorReset();
	void	CacheIteratorUp();
	void	CacheIteratorDown(int childindex, int xindex, int zindex, int level);
	
	//
	// Cache tree stuff.
	//
	class	TexNode {
	public:
		int	GetX() const { return x; }
		int	GetZ() const { return z; }
		int	GetScale() const { return scale; }
		Render::Texture*	GetTexture() const { return Texture; }

	protected:
		int	x, z, scale;
		Render::Texture*	Texture;
	};

	TexNode*	GetTexNodeRoot();
	TexNode*	RequestTexture(TexNode* t);
	void	TouchTexture(TexNode* t);
	TexNode*	GetChild(TexNode* t, int ChildIndex);

	//
	// Surface physics stuff.
	//
	
	int	GetSurfaceType(const vec3& location);

	struct PhysicalInfo {
		float	MaxDepth;	// Maximum compressibility, in meters (e.g. ice would be 0, deep fluffy powder could be > 1)
		float	SlidingDrag;	// Regular sliding friction, force not proportional to speed.
		float	FluidDrag;	// Fluid drag, force proportional to v^2.
		float	NormalBouyancyFactor, MaxFloatSpeed;	// Bouyancy params.
		float	EdgeHold;	// 0 == hard teflon, 1 == smooth packed powder
	};

	const PhysicalInfo&	GetSurfaceInfo(int type);

	const int	TEXTURE_BITS = 6;
	const int	TEXTURE_SIZE = (1 << TEXTURE_BITS);
	const int	TEXTURE_BITS_PER_METER = 2;


	inline int	WorldToTex(int w)
	// Converts integral world-coordinate index value w into texture
	// coords, which are slightly tweaked due to the fact that the
	// textures have TEXTURE_SIZE-1 (e.g. 31) samples per texture
	// square as opposed to TEXTURE_SIZE >> TEXTURE_BITS_PER_METER
	// (e.g. 16) meters in the same square.
	{
		return ((w << TEXTURE_BITS) - w) >> (TEXTURE_BITS - TEXTURE_BITS_PER_METER);
	}

	inline int	TexToWorld(int t)
	// Converts texture coord into approximately corresponding integral world coordinate.
	{
		return int((t << (TEXTURE_BITS - TEXTURE_BITS_PER_METER)) / float(TEXTURE_SIZE - 1));
	}
	
	// Interface to Surface shader, a separate sub-module.
	void	OpenSurfaceShader();
	void	GenerateNoise(uint16 NoiseMap[TEXTURE_SIZE][TEXTURE_SIZE], int xindex, int zindex, int level);
	void	GenerateTexture(bitmap32* output, int xindex, int zindex, int level,
				uint8 LightMap[TEXTURE_SIZE][TEXTURE_SIZE],
				uint8 TypeMap[TEXTURE_SIZE][TEXTURE_SIZE],
				uint16 NoiseMap[TEXTURE_SIZE][TEXTURE_SIZE]);
	void	CloseSurfaceShader();
	void	ResetSurfaceShader();

	int	Noise(int x, int z);
	int	InterpNoise(int x, int y, int coeff);

	void	GetShadeTable(uint8 table[256][3]);
	void	SetShadeTable(uint8 table[256][3]);
};



#endif // SURFACE_HPP
