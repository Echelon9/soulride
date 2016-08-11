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
// terrain.hpp	-thatcher 2/5/1998 Copyright Thatcher Ulrich

// Headers for a module that models heightfield terrain.


#ifndef TERRAIN_HPP
#define TERRAIN_HPP


#include <stdio.h>
#include "types.hpp"
#include "view.hpp"
#include "surface.hpp"


namespace TerrainModel {
	void	Open();
	void	Close();
	void	Clear();

//	void	FreeHeights();
	
	uint16	QueryHeight(int x, int z);
	bool	DoSamplesExist(int x, int z, int level);

	void	Load(FILE* fp, FILE* fp_cache, bool Write);
	float	GetHeight(const vec3& location);
	float	GetHeight(float x, float z);
	vec3	GetNormal(const vec3& location);

	int	GetType(const vec3& location);
	void	GetMaps(uint8 LightMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE],
			uint8 TypeMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE],
			int xindex, int zindex, int level,
			uint16 NoiseMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE]);

	// Utility functions.
	bool	FindIntersectionCoarse(vec3* result, const vec3& loc, const vec3& ray);

};


namespace TerrainMesh {
	void	Open();
	void	Close();
	void	Clear();
	int	Load(FILE* fp);

//	void	BuildTree();
	
	void	Update(const ViewState& s);
	void	Render(const ViewState& s);

	void	RenderShadow(const ViewState& s, const vec3& center, float extent);

	float	GetHeight(float x, float z);

	float	CheckForRayHit(const vec3& point, const vec3& dir);
	
	float	GetHeightToMetersFactor();

	int	GetRenderedTriangleCount();
	int	GetNodesTotalCount();
	int	GetNodesActiveCount();
	float	GetDetailNudge();
};


#endif // TERRAIN_HPP
