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
// SurfaceShader.cpp	-thatcher 4/23/1999 Copyright Slingshot Game Technology

// Code for painting surface textures, given various parameter arrays.
// This is a sub-module of Surface::.


#include "utility.hpp"
#include "psdread.hpp"
#include "surface.hpp"
#include "terrain.hpp"
#include "config.hpp"
#include "lua.hpp"
#include "game.hpp"


namespace Surface {
;


bool	NoiseInited = false;
static void	InitNoiseArray();
static void	InitShadeTable(const char* filename);


/**
 * Gets the next word from arg, and uses it as a bitmap filename for
 * initializing the shadetable gradient.
 */
static void
ShadeTableReset_lua()
{
	const char*	fn = lua_getstring(lua_getparam(1));
	if (fn == NULL) fn = "shadetable.psd";
	
	InitShadeTable(fn);

	FlushCache();
}


void	Update(const UpdateState& u)
// Update.  Special-purpose debug stuff.
{
	if (Config::GetBoolValue("F4Pressed")) {
		InitShadeTable("shadetable.psd");
		FlushCache();
	}
}


// Table of surface types.

const int	SURFACE_IMAGE_BITS = 7;	// (1 << this) == surface bitmap size == 128

struct SurfaceTileInfo {
	bitmap32*	Image[SURFACE_IMAGE_BITS + 1];	// Mip levels down to 1x1.
};

struct SurfaceTypeInfo {
	SurfaceTileInfo	Tile[2];
	PhysicalInfo	Physical;

	SurfaceTypeInfo() {
		int	i;
		for (i = 0; i < SURFACE_IMAGE_BITS+1; i++) {
			Tile[0].Image[i] = NULL;
			Tile[1].Image[i] = NULL;
		}
	}
};


const int	MAX_SURFACE_TYPES = 16;
SurfaceTypeInfo	SurfaceType[MAX_SURFACE_TYPES];
int	SurfaceTypeCount = 0;

// added "surfaceInitInfoType" to remove g++ 4.0 compiler errors
struct surfaceInitInfoType {
	const char*	BitmapName0;
	const char*	BitmapName1;
	PhysicalInfo	Physical;
} SurfaceInitInfo[] = {
	{ "forest0.psd", "forest1.psd", { 0, 0.70f, 0, 0, 1, 0.5f } },
	{ "ice0.psd", "ice1.psd", { 0, 0.020f, 0.0000, 0, 1, 0.2f } },
	{ "granular0.psd", "granular1.psd", { 0.10f, 0.040f, 0.002f, 300, 3, 0.8f } },
	{ "hardpack0.psd", "hardpack1.psd", { 0, 0.030f, 0.001f, 0, 1, 1.0f } },
	{ "powder0.psd", "powder1.psd", { 0.40f, 0.050f, 0.010f, 600, 5, 0.8f } },
	{ "rock0.psd", "rock1.psd", { 0, 0.80f, 0, 0, 1, 0.2f } },
	{ "sand0.psd", "sand1.psd", { 0, 0.200f, 0.0030f, 0, 1, 0.8f } },
	{ "asphalt0.psd", "asphalt1.psd", { 0, 0.80f, 0, 0, 1, 0.2f } },
	{ "water0.psd", "water1.psd", { 2, 0.05f, 0.0070f, 500, 8, 0.8f } },
	{ NULL, NULL, { 0, 0, 0, 0, 0 } }

	// Corn snow, more types of rock, ice, windblown crap, mixtures...
};


bitmap32*	SubFilter(bitmap32* source)
// Creates a bitmap that's half the size of the source bitmap,
// and filters the source bitmap's data into the new bitmap.
{
	bitmap32*	dest = new bitmap32(source->GetWidth() >> 1, source->GetHeight() >> 1);

	Geometry::HalfScaleFilterSinc(source->GetWidth(), source->GetHeight(), source->GetData(), dest->GetData());

	return dest;
}


/**
 * Creates a new surface type using the given filename as the image.
 * @param s             SurfaceTileInfo type to output
 * @param ImageFileName Filename
 */
static void
InitSurfaceImages(SurfaceTileInfo* s, const char* ImageFileName)
{
	bitmap32*	b = PSDRead::ReadImageData32(ImageFileName);
	if (b == NULL) {
		Error e; e << "Can't load image '" << ImageFileName << "'.";
		throw e;
	}

	// Compute log2(height) here to see if bitmap includes pre-built mip-maps, and how many.
	// If there are no mip-maps, the height will be a power-of-two.  If there are mip-maps,
	// they'll be stacked below the main image, and the height value will indicate how many
	// there are.
	
	int	HeightBits = 0;
	int	i = b->GetHeight();
	for (;;) {
		i >>= 1;
		if (i == 0) break;
		HeightBits++;
	}
	
	int	Height = 1 << HeightBits;
	
	int	PremadeMIPMapCount = 0;
	i = b->GetHeight() << 1;
	while (Height & i) {
		PremadeMIPMapCount++;
		i <<= 1;
	}
	
	int	Width = b->GetWidth();

	int	dim = Width;
	PremadeMIPMapCount += 1;

	uint32*	p = b->GetData();

	// Construct the MIP pyramid, using loaded image data as
	// available, and then switching to filtering.
	for (i = 0; i < SURFACE_IMAGE_BITS+1; i++) {
		Game::LoadingTick();
		
		if (i < PremadeMIPMapCount) {
			// Use data loaded from the bitmap file.
			s->Image[i] = new bitmap32(dim, dim);

			// Copy data from b.
			for (int row = 0; row < dim; row++) {
				uint32*	q = s->Image[i]->GetData() + row * dim;
				for (int col = 0; col < dim; col++) {
					q[col] = p[col];
				}
				p += Width;
			}
		} else {
			// Sub-sample the previous image in the pyramid.
			s->Image[i] = SubFilter(s->Image[i-1]);
		}

		dim >>= 1;
		if (dim <= 0) dim = 1;
	}

	delete b;
}


int	GetSurfaceType(const vec3& loc)
// Returns the surface type under the given spot (uses x,z coords; ignores y).
{
	return TerrainModel::GetType(loc);
}


const PhysicalInfo&	GetSurfaceInfo(int type)
// Returns a reference to a structure filled with information about
// the specified type of surface.
{
	int	index = type /* >> 4 */;
	if (index < 0 || index >= SurfaceTypeCount) index = 0;

	return SurfaceType[index].Physical;
}


/**
 * Initialize the surface shader. Load any needed bitmaps.
 */
void
OpenSurfaceShader()
{
	// Set up to do Perlin noise.
	InitNoiseArray();

	// Set up lightmapping table.
	InitShadeTable("shadetable.psd");

	// Attach a handler to "shadetable-reset".
	lua_register("shadetable_reset", ShadeTableReset_lua);
}


void	ClearSurfaceShader();


void	CloseSurfaceShader()
// Close.  Free any resources used.
{
	ClearSurfaceShader();
}


void	ResetSurfaceShader()
// Load surface bitmaps, generate MIP maps.
{
	ClearSurfaceShader();

	// Load surfaces.
	for (SurfaceTypeCount = 0; SurfaceTypeCount < MAX_SURFACE_TYPES; SurfaceTypeCount++) {
		if (SurfaceInitInfo[SurfaceTypeCount].BitmapName0 == NULL) break;

		Game::LoadingTick();
		InitSurfaceImages(&SurfaceType[SurfaceTypeCount].Tile[0],
		                  SurfaceInitInfo[SurfaceTypeCount].BitmapName0);	// Init first tile bitmap.
		
		Game::LoadingTick();
		InitSurfaceImages(&SurfaceType[SurfaceTypeCount].Tile[1],
		                  SurfaceInitInfo[SurfaceTypeCount].BitmapName1);	// Init second tile bitmap.
		
		SurfaceType[SurfaceTypeCount].Physical = SurfaceInitInfo[SurfaceTypeCount].Physical;	// Copy physical params.
	}
}


void	ClearSurfaceShader()
// Free the surface bitmaps.
{
	int	i, j;
	
	for (j = 0; j < SurfaceTypeCount; j++) {
		for (i = 0; i < SURFACE_IMAGE_BITS+1; i++) {
			if (SurfaceType[j].Tile[0].Image[i]) {
				delete SurfaceType[j].Tile[0].Image[i];
				SurfaceType[j].Tile[0].Image[i] = NULL;
			}
			if (SurfaceType[j].Tile[1].Image[i]) {
				delete SurfaceType[j].Tile[1].Image[i];
				SurfaceType[j].Tile[1].Image[i] = NULL;
			}
		}
	}

	SurfaceTypeCount = 0;
}


static void	ApplyLightmap(uint32 dest[TEXTURE_SIZE][TEXTURE_SIZE], uint8 LightMap[TEXTURE_SIZE][TEXTURE_SIZE]);


typedef uint32 (TextureArray)[TEXTURE_SIZE][TEXTURE_SIZE];


void	GenerateTexture(bitmap32* output, int xindex, int zindex, int level,
			uint8 LightMap[TEXTURE_SIZE][TEXTURE_SIZE],
			uint8 TypeMap[TEXTURE_SIZE][TEXTURE_SIZE],
			uint16 NoiseMap[TEXTURE_SIZE][TEXTURE_SIZE])
// Given the various parameter arrays, paints a surface texture into the given output
// bitmap.
// xindex, zindex, and level are in texture coordinates.
{
	int	i, j;
	
	// Paint the texture.

	//xxxxxxxxxxx
	uint32*	dest = output->GetData();
	uint8*	typesrc = &TypeMap[0][0];
	
	int	size;
	int	mask;
	int	shift;
	int	index = iclamp(0, level - TEXTURE_BITS, SURFACE_IMAGE_BITS);

	size = SurfaceType[0].Tile[0].Image[index]->GetWidth();
	mask = size - 1;
	shift = SURFACE_IMAGE_BITS - index;
	
	int	sz = zindex >> index;
	int	sx = xindex >> index;

	int	cutoff = level - TEXTURE_BITS;
	
	// Set up source texture pointers.
	uint32*	SourceData[16][2];
	for (i = 0; i < 16; i++) {
		SourceData[i][0] = SurfaceType[iclamp(0, i, SurfaceTypeCount-1)].Tile[0].Image[index]->GetData();
		SourceData[i][1] = SurfaceType[iclamp(0, i, SurfaceTypeCount-1)].Tile[1].Image[index]->GetData();
	}

	// Fill the texture map.
	uint16*	noisesrc = &NoiseMap[0][0];
	for (j = 0; j < TEXTURE_SIZE; j++) {
		int	RowOffset = (sz & mask) * size;
		for (i = 0; i < TEXTURE_SIZE; i++) {
			int	t = ((*typesrc++)/* >> 4 */) & 15;
			uint32	type0pixel = SourceData[t][((*noisesrc++) & 0x8000) ? 1 : 0][RowOffset + ((sx + i) & mask)];
			
			*dest++ = type0pixel;
		}
		sz++;
	}

	// Apply the lightmap.
	TextureArray&	p = *(TextureArray*) (output->GetData());
	ApplyLightmap(p, LightMap);
}


struct RGBTriple {
	uint8	r, g, b;
} ShadeTable[256];


void	GetShadeTable(uint8 table[256][3])
// Fills the given array with the current values of our shade table.
{
	int	i;
	for (i = 0; i < 256; i++) {
		table[i][0] = ShadeTable[i].r;
		table[i][1] = ShadeTable[i].g;
		table[i][2] = ShadeTable[i].b;
	}
}



void	SetShadeTable(uint8 table[256][3])
// Sets our current shade table to the given values.
// Flushes the surface cache if the shade table has changed.
{
	int	i;
	bool	Change = false;
	for (i = 0; i < 256; i++) {
		if (ShadeTable[i].r != table[i][0]) { ShadeTable[i].r = table[i][0]; Change = true; }
		if (ShadeTable[i].g != table[i][1]) { ShadeTable[i].g = table[i][1]; Change = true; }
		if (ShadeTable[i].b != table[i][2]) { ShadeTable[i].b = table[i][2]; Change = true; }
	}

	if (Change) FlushCache();
}



void	InitShadeTable(const char* tablefilename)
// Initialize the table of rgb colors we use for lightmap shading.
{
	int	i;
	
	bitmap32*	b = PSDRead::ReadImageData32(tablefilename);
	if (b) {
		for (i = 0; i < 256; i++) {
			uint8 *color = (uint8 *)&b->GetData()[i];
			ShadeTable[i].r = color[0];
			ShadeTable[i].g = color[1];
			ShadeTable[i].b = color[2];
		}
		delete b;
	} else {
		for (i = 0; i < 256; i++) {
			ShadeTable[i].r = i;
			ShadeTable[i].g = i;
			ShadeTable[i].b = ((i >> 1) + (i >> 2) + 50);
		}
	}
}


void	ApplyLightmap(uint32 dest[TEXTURE_SIZE][TEXTURE_SIZE], uint8 LightMap[TEXTURE_SIZE][TEXTURE_SIZE])
// Applies the given lightmap to the given texture.
{
//	int	dither = 1;	// sequence through { { 1, 2} , { 3, 0 } }

	uint8*	p = (uint8 *)&dest[0][0];
	uint8*	l = &LightMap[0][0];
	
	for (int j = 0; j < TEXTURE_SIZE; j++) {
		for (int i = 0; i < TEXTURE_SIZE; i++) {
			RGBTriple&	shade = ShadeTable[*l];
//					+ dither
			int	r = p[0] * shade.r;
			int	g = p[1] * shade.g;
			int	b = p[2] * shade.b;

			*p++ = (r & 0xFF00) >> 8;
			*p++ = (g & 0xFF00) >> 8;
			*p++ = (b & 0xFF00) >> 8;
			*p++ = 0xFF;
			
			l++;
//			dither ^= 3;
		}
//		dither ^= 2;
	}
}


static const int	NABITS = 6;
static const int	NASIZE = (1 << NABITS);
static const int	NAMASK = (NASIZE - 1);

//struct NAElem {
//	float	a, b, d;
//	float	dp;
int8 NoiseArray[NASIZE][NASIZE];


void	InitNoiseArray()
// Fills the noise array with random data.
{
	if (NoiseInited) return;
	
	int	i, j;
	for (j = 0; j < NASIZE; j++) {
		for (i = 0; i < NASIZE; i++) {
			NoiseArray[j][i] = ((rand() >> 4) & 255) - 128;
		}
	}

	NoiseInited = true;
}


void	AddNoise(uint16 out[TEXTURE_SIZE][TEXTURE_SIZE], int x0, int z0, int mag, int coeff)
// Adds noise into the given output buffer.  x0 and z0 are offsets into
// output space, and mag determines the scaling between noise space and
// output space.  There are (1 << mag) output samples for every noise
// sample; the noise is bilinearly interpolated between samples.
{
	int	i, j;
	
	if (mag <= 0) {
		// Point sample.
		int	step = 1 << -mag;
		int	z = z0;
		for (j = 0; j < TEXTURE_SIZE; j++) {
			int	x = x0;
			for (i = 0; i < TEXTURE_SIZE; i++) {
				out[j][i] += (NoiseArray[z & NAMASK][x & NAMASK] * coeff) >> 5;
				x += step;
			}
			z += step;
		}
	} else {
		// Bilinearly scale up the noise.
		int	step = 1 << mag;

		int	z = z0 >> mag;
		int	zbase = z0 & ((1 << mag) - 1);
		
		for (j = 0; j < TEXTURE_SIZE; j += step) {
			int	height = step - zbase;
			if (j + height > TEXTURE_SIZE) height = TEXTURE_SIZE - j;
				
			int	x = x0 >> mag;
			int	xbase = x0 & ((1 << mag) - 1);

			for (i = 0; i < TEXTURE_SIZE; i += step) {
				// First sample channel.
				int	sl0 = (NoiseArray[z&NAMASK][x&NAMASK] * coeff) << 16;
				int	sl1 = (NoiseArray[(z+1)&NAMASK][x&NAMASK] * coeff) << 16;
				int	sr0 = (NoiseArray[z&NAMASK][(x+1)&NAMASK] * coeff) << 16;
				int	sr1 = (NoiseArray[(z+1)&NAMASK][(x+1)&NAMASK] * coeff) << 16;

				// Second sample channel, from a different part of the noise array.
				int	tl0 = (NoiseArray[(z + 10)&NAMASK][x&NAMASK] * coeff) << 16;
				int	tl1 = (NoiseArray[(z + 10 + 1)&NAMASK][x&NAMASK] * coeff) << 16;
				int	tr0 = (NoiseArray[(z + 10)&NAMASK][(x+1)&NAMASK] * coeff) << 16;
				int	tr1 = (NoiseArray[(z + 10 + 1)&NAMASK][(x+1)&NAMASK] * coeff) << 16;

				int	dsl = (sl1 - sl0) >> mag;
				int	dsr = (sr1 - sr0) >> mag;
				int	dtl = (tl1 - tl0) >> mag;
				int	dtr = (tr1 - tr0) >> mag;

				int	sl = sl0;
				int	sr = sr0;
				int	tl = tl0;
				int	tr = tr0;
				
				if (zbase) {
					sl += (dsl * zbase);
					sr += (dsr * zbase);
					tl += (dtl * zbase);
					tr += (dtr * zbase);
				}
				
				uint16*	row = &out[j][i];

				int	width = step - xbase;
				if (i + width > TEXTURE_SIZE) width = TEXTURE_SIZE - i;
					
				for (int v = 0; v < height; v++) {
					int	ds = (sr - sl) >> mag;
					int	s = sl;
					int	dt = (tr - tl) >> mag;
					int	t = tl;
					
					if (xbase) {
						s += (ds * xbase);
						t += (dt * xbase);
					}
					uint16*	p = row;

					for (int h = 0; h < width; h++) {
						int8	v0 = *p >> 8;
						int8	v1 = *p & 0x0FF;
						v0 += (s >> 21);
						v1 += (t >> 21);
						*p = (v0 << 8) | (v1 & 0x0FF);

						p++;
						s += ds;
						t += dt;
					}
					sl += dsl;
					sr += dsr;
					tl += dtl;
					tr += dtr;
					row += TEXTURE_SIZE;
				}

				x += 1;
				i -= xbase;
				xbase = 0;
			}
			z += 1;
			j -= zbase;
			zbase = 0;
		}
	}
}


static const int	OCTAVES = 6;
//static const int	coeff[OCTAVES] = {  4,  5,  5,  4,  4,  3 };
static const int	coeff[OCTAVES] = {  10,  15,  15,  17,  18,  15 };


void	GenerateNoise(uint16 NoiseMap[TEXTURE_SIZE][TEXTURE_SIZE], int xindex, int zindex, int level)
// Generates a noise map corresponding to the specified
// patch of texture space.  The noise map is actually two independent channels
// of 8-bit signed noise, packed into a 16-bit package.
{
	if (!NoiseInited) InitNoiseArray();
	
	int	i, j;

	// Clear the array.
	for (j = 0; j < TEXTURE_SIZE; j++) {
		for (i = 0; i < TEXTURE_SIZE; i++) {
			NoiseMap[j][i] = 0;
		}
	}

	int	x0, z0;
	if (level >= TEXTURE_BITS) {
		x0 = xindex >> (level - TEXTURE_BITS);
		z0 = zindex >> (level - TEXTURE_BITS);
	} else {
		x0 = xindex << (TEXTURE_BITS - level);
		z0 = zindex << (TEXTURE_BITS - level);
	}
	
	// Sum scaled noise patterns into the array.
	for (i = 0; i < OCTAVES; i++) {
		if (coeff[i] == 0) continue;
		int	mag = i + 2 - (level - TEXTURE_BITS);
		if (mag < -1) continue;	// should do some kind of MIP-ing.
		AddNoise(NoiseMap, x0, z0, mag, coeff[i]);
	}
}


int	Noise(int x, int y)
// Given coordinates in texture-space, return a noise value.  Uses a combination
// of Perlin-style bandwidth-limited noise functions.
{
	if (!NoiseInited) InitNoiseArray();
	
	int	i;

	int	total = 0;
	for (i = 0; i < OCTAVES; i++) {
		total += InterpNoise((x << 8) >> (i + 2), (y << 8) >> (i + 2), coeff[i]);
	}

	return total;
}


int	InterpNoise(int x, int y, int coeff)
// Returns a noise value, given the input.  The low 8-bits of x and y are considered
// fractional, and control interpolation.
{
	if (!NoiseInited) InitNoiseArray();
	
	int	mask = (1 << (NABITS + 8)) - 1;
	x &= mask;
	y &= mask;
	
	int	fx = x & 255;
	int	fy = y & 255;

	int	ix = (x >> 8);
	int	iy = (y >> 8);
	
	int	n0 = NoiseArray[iy][ix];
	int	n1 = NoiseArray[iy][(ix + 1) & NAMASK];
	int	n2 = NoiseArray[(iy + 1) & NAMASK][ix];
	int	n3 = NoiseArray[(iy + 1) & NAMASK][(ix + 1) & NAMASK];

	uint8	nx = ((n0 * (256 - fx) * (256 - fy) + n1 * fx * (256 - fy) + n2 * (256 - fx) * fy + n3 * fx * fy) * coeff) >> 13;

	iy = (iy + 10) & NAMASK;
	n0 = NoiseArray[iy][ix];
	n1 = NoiseArray[iy][(ix + 1) & NAMASK];
	n2 = NoiseArray[(iy + 1) & NAMASK][ix];
	n3 = NoiseArray[(iy + 1) & NAMASK][(ix + 1) & NAMASK];

	uint8	nz = ((n0 * (256 - fx) * (256 - fy) + n1 * fx * (256 - fy) + n2 * (256 - fx) * fy + n3 * fx * fy) * coeff) >> 13;

	return uint16((nx << 8) | (nz & 0xFF));
}


};	// end namespace Surface

