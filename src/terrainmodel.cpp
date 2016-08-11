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
// TerrainModel.cpp	-thatcher 6/15/1999 Copyright Slingshot Game Technology

// Database of terrain data.


#include <float.h>
#include "utility.hpp"
#include "terrain.hpp"
#include "weather.hpp"
#include "config.hpp"
#include "lua.hpp"
#include "game.hpp"


namespace TerrainModel {
;


const int	TSHIFT = Surface::TEXTURE_BITS_PER_METER;
const int	TBITS = Surface::TEXTURE_BITS;
const float	TEX_SAMPLE_SPACING = (Surface::TEXTURE_SIZE >> TSHIFT) / float(Surface::TEXTURE_SIZE - 1);


//
// RLE-ish data structure and iterator for compact storage of surface type grids.
//


struct TypeGridData {
	int	Rows, Cols;
	int32*	RowOffset;	// Array of row starts.
	uint8*	Data;	// RLE-ish data.  Each byte encodes a run.  The high nibble is the run-length; the low nibble is the type.
	int	DataSize;

	TypeGridData() {
		Rows = Cols = 0;
		RowOffset = NULL;
		Data = NULL;
		DataSize = 0;
	}
	~TypeGridData() {
		Clear();
	}

	void	Clear()
	{
		if (RowOffset) {
			delete [] RowOffset;
			RowOffset = NULL;
		}
		if (Data) {
			delete [] Data;
			Data = NULL;
		}
	}

	void	Write(FILE* fp)
	{
		Write16(fp, Rows);
		Write16(fp, Cols);

		int	i;
		for (i = 0; i < Rows; i++) {
			Write32(fp, RowOffset[i]);
		}

		Write32(fp, DataSize);
		fwrite(Data, 1, DataSize, fp);
	}

	void	Read(FILE* fp, int r, int c)
	{
		Clear();
		
		Rows = Read16(fp);
		Cols = Read16(fp);

		RowOffset = new int32[Rows];
		int	i;
		for (i = 0; i < Rows; i++) {
			RowOffset[i] = Read32(fp);
		}

		DataSize = Read32(fp);
		Data = new uint8[DataSize];
		fread(Data, 1, DataSize, fp);
	}

	void	ReadCompare(FILE* fp, int r, int c)
	// This is a debugging function.
	// Read compressed data from the given cache file and compare it to our
	// computed data, looking for discrepancies.
	{
		printf("ReadCompare(%d, %d)\n", r, c);

		int	read_rows = Read16(fp);
		int	read_cols = Read16(fp);

		if (read_rows != Rows || read_cols != Cols) {
			printf("Discrepancy: this(%d, %d) != read(%d, %d)\n", Rows, Cols, read_rows, read_cols);
		}

		int	i;
		for (i = 0; i < read_rows; i++) {
			int	read_start = Read32(fp);
			if (read_start != RowOffset[i]) {
				printf("Discrepancy: row %d offset: this = %d, read = %d\n", i, RowOffset[i], read_start);
			}
		}

		int	read_data_size = Read32(fp);
		for (i = 0; i < DataSize; i++) {
			uint8	c = fgetc(fp);
			if (c != Data[i]) {
				printf("Discrepancy: data @ offset %d: this = %d, read = %d\n", i, Data[i], c);
			}
		}
	}
	
	int	Compress(int r, int c, uint8* buf)
	{
		Clear();
		
		Rows = r;
		Cols = c;
		
		// Count total bytes needed for compressed data.  Dry run.
		int	bytes = 0;
		int	i, j;
		uint8*	p = buf;
		for (j = 0; j < r; j++) {
			int	run = 1;
			for (i = 0; i < c; i++) {
				if (run >= 15 || i >= c-1 || *p != *(p+1)) {
					bytes++;
					run = 0;
				}
				run++;
				p++;
			}
		}

		// Allocate buffers.
		RowOffset = new int32[Rows];
		Data = new uint8[bytes];
		DataSize = bytes;

		// Fill buffers.  Wet run.
		uint8*	q = Data;
		p = buf;
		for (j = 0; j < r; j++) {
			RowOffset[j] = q - Data;
			int	run = 1;
			for (i = 0; i < c; i++) {
				if (run >= 15 || i >= c-1 || *p != *(p+1)) {
					*q++ = (run << 4) | (*p & 0x0F);
					run = 0;
				}
				run++;
				p++;
			}
		}

		return bytes;
	}
};


struct TypeGridIterator {
	TypeGridData*	tgd;
	int	row, col;
	uint8	current;
	uint8*	next_data;
	
	TypeGridIterator(TypeGridData* t) {
		tgd = t;
		row = col = 0;
		next_data = tgd->Data;
		current = *next_data++;
	}

	int	GetType() const { return current & 15; }
	void	Advance(int ct)
	// Advance the column pointer by ct steps.
	{
		col += ct;
		for (;;) {
			int	left = current >> 4;
			if (left > ct) {
				current -= (ct << 4);
				return;
			} else {
				ct -= left;
				current = *next_data;
				next_data++;
			}
		}
	}
	void	SetRowCol(int r, int c)
	// Move iterator to the given location.
	{
		if (r != row || c < col) {
			// Go to the start of the desired row.
			next_data = tgd->Data + tgd->RowOffset[r];
			row = r;
			col = 0;
			current = *next_data++;
		}

		Advance(c - col);
	}
};


struct TGIShortcut {
	uint32	rowcolcurrent;
	uint8*	next_data;
	
	void	StoreIterator(const TypeGridIterator& t)
	// Compress the iterator info into this struct, for later expansion.
	{
		// 12 bits each for row and col, and store current in the top 8 bits of rowcolcurrent.
		rowcolcurrent = ((t.row & 0x0FFF) << 12) | ((t.col & 0x0FFF)) | (t.current << 24);
		next_data = t.next_data;
	}

	void	RecallIterator(TypeGridIterator* tgi)
	// Reconstitute previously stored iterator state.
	{
		tgi->row = ((rowcolcurrent) >> 12) & 0x0FFF;
		tgi->col = ((rowcolcurrent)) & 0x0FFF;
		tgi->current = (rowcolcurrent >> 24) & 0x0FF;
		tgi->next_data = next_data;
	}
};


//
// Overlapping multi-scale grids, used to represent surface type coverage.
//


bool	HaveGridHeights = false;


class Grid;

Grid**	Grids = NULL;
int	GridCount = 0;


class Grid
// Rectangular array of surface type values.  Can have varying scale values.
{
public:
	Grid()
	{
		IntXOrigin = 0;
		IntZOrigin = 0;
		XOrigin = 0;
		ZOrigin = 0;
		Pitch = 1.0;
		XSize = 0;
		ZSize = 0;
		ScaleLevel = 0;
		Lightmap = NULL;
	}
	
	Grid(FILE* fp, FILE* fp_cache, bool Write)
	// Initializes the grid from the given file (should change to a generic stream type).
	// Leaves the read cursor just past the end of this grid's data.
	{
		Game::LoadingTick();
		
		// Load the grid parameters.
		XSize = Read32(fp);
		ZSize = Read32(fp);

		IntXOrigin = Read32(fp);
		IntZOrigin = Read32(fp);

		ScaleLevel = Read32(fp);

		IntXOrigin <<= ScaleLevel;
		IntZOrigin <<= ScaleLevel;
		
		Pitch = (float) (/* base grid pitch */ 1 * (1 << ScaleLevel));
		XOrigin = (float) (IntXOrigin * /* base grid pitch */ 1);
		ZOrigin = (float) (IntZOrigin * /* base grid pitch */ 1);

		// Allocate and clear the lightmap & typemap.

		int	mask = ~((1 << TSHIFT)-1);
		TexXOrigin = Surface::WorldToTex(IntXOrigin) & mask;
		TexZOrigin = Surface::WorldToTex(IntZOrigin) & mask;

		int	x1 = (Surface::WorldToTex(IntXOrigin + (XSize << ScaleLevel)) + ~mask) & mask;
		int	z1 = (Surface::WorldToTex(IntZOrigin + (ZSize << ScaleLevel)) + ~mask) & mask;

		// Compute lightmap size, so that we'll have approximately one lightmap sample
		// for every height sample.
		TexXSize = (x1 - TexXOrigin) >> (ScaleLevel + TSHIFT);
		TexZSize = (z1 - TexZOrigin) >> (ScaleLevel + TSHIFT);

		int	values = TexXSize * TexZSize;
		Lightmap = new uint8[values];

		if (fp_cache == NULL || Write == true) {
			uint8*	TempTypeMap = new uint8[values];
			memset(TempTypeMap, 0, values);
			
			// Load the typemap data from the file into a temporary array.
			Game::LoadingTick();
			uint8*	ScratchTypeMap = new uint8[XSize * ZSize];
			fread(ScratchTypeMap, 1, XSize * ZSize, fp);
		
			// Resample the data from the temporary typemap data, into our real typemap array.
			// For now just do nearest neighbor sampling.
			Game::LoadingTick();
			for (int tz = 0; tz < TexZSize; tz++) {
				int	z = (Surface::TexToWorld((tz << (ScaleLevel + TSHIFT)) + TexZOrigin) - IntZOrigin) >> ScaleLevel;
				for (int tx = 0; tx < TexXSize; tx++) {
					int	x = (Surface::TexToWorld((tx << (ScaleLevel + TSHIFT)) + TexXOrigin) - IntXOrigin) >> ScaleLevel;
					
					// Out of bounds sample point: this is kind of a problem.
					// Should perhaps sample from underlying grid?
					// Which may not exist yet, since we're reading unsorted grids.
					// Try just clamping, for now.
					x = iclamp(0, x, XSize-1);
					z = iclamp(0, z, ZSize-1);
					
					TempTypeMap[tx + tz * TexXSize] = ScratchTypeMap[x + z * XSize];
				}
			}
			
			// Create compressed version of typemap.
			Game::LoadingTick();
			int	bytes = TypeGrid.Compress(TexZSize, TexXSize, TempTypeMap);
			
			delete [] ScratchTypeMap;
			delete [] TempTypeMap;

			if (fp_cache && Write) {
				// Write preprocessed data to disk.
				TypeGrid.Write(fp_cache);
			}

			// Compute lightmap data.
			ComputeMaps();

			if (fp_cache && Write) {
				// Write lightmap to cache file.
				Game::LoadingTick();
				fwrite(Lightmap, 1, values, fp_cache);
			}
			
		} else {
			// Load preprocessed data directly from disk.
			TypeGrid.Read(fp_cache, TexZSize, TexXSize);

			// Load lightmap data.
			fread(Lightmap, 1, values, fp_cache);
			
			// Skip uncompressed data from original file.
			fseek(fp, XSize * ZSize, SEEK_CUR);
#if 0
		} else /* test grid compression */ {
			uint8*	TempTypeMap = new uint8[values];
			memset(TempTypeMap, 0, values);
			
			// Load the typemap data from the file into a temporary array.
			Game::LoadingTick();
			uint8*	ScratchTypeMap = new uint8[XSize * ZSize];
			fread(ScratchTypeMap, 1, XSize * ZSize, fp);
		
			// Resample the data from the temporary typemap data, into our real typemap array.
			// For now just do nearest neighbor sampling.
			Game::LoadingTick();
			for (int tz = 0; tz < TexZSize; tz++) {
				int	z = (Surface::TexToWorld((tz << (ScaleLevel + TSHIFT)) + TexZOrigin) - IntZOrigin) >> ScaleLevel;
				for (int tx = 0; tx < TexXSize; tx++) {
					int	x = (Surface::TexToWorld((tx << (ScaleLevel + TSHIFT)) + TexXOrigin) - IntXOrigin) >> ScaleLevel;
					
					// Out of bounds sample point: this is kind of a problem.
					// Should perhaps sample from underlying grid?
					// Which may not exist yet, since we're reading unsorted grids.
					// Try just clamping, for now.
					x = iclamp(0, x, XSize-1);
					z = iclamp(0, z, ZSize-1);
					
					TempTypeMap[tx + tz * TexXSize] = ScratchTypeMap[x + z * XSize];
				}
			}
			
			// Create compressed version of typemap.
			Game::LoadingTick();
			int	bytes = TypeGrid.Compress(TexZSize, TexXSize, TempTypeMap);
			
			delete [] ScratchTypeMap;
			delete [] TempTypeMap;

			// Compare computed data against data from cache file.
			TypeGrid.ReadCompare(fp_cache, TexZSize, TexXSize);

			// Load lightmap data.
			fread(Lightmap, 1, values, fp_cache);
			
//			// Skip uncompressed data from original file.
//			fseek(fp, XSize * ZSize, SEEK_CUR);
#endif // 0
		}

		SetupTypeGridShortcuts();
	}

	
	~Grid()
	// Destructor.
	{
		delete [] Lightmap;
		delete [] Shortcut;
		TypeGrid.Clear();
	}


	void	SetupTypeGridShortcuts()
	{
		int	hct = ((TexXSize >> 6));
		if (hct > 0) {
			int	ct = TexZSize * hct;
			Shortcut = new TGIShortcut[ct];
			int	z, x;
			TypeGridIterator	tgd(&TypeGrid);
			for (z = 0; z < TexZSize; z++) {
				for (x = 0; x < hct; x++) {
					tgd.SetRowCol(z, (x + 1) << 6);
					Shortcut[z * hct + x].StoreIterator(tgd);
				}
			}
		} else {
			Shortcut = NULL;
		}
	}


	void	ShortcutToIterator(TypeGridIterator* tgi, int r, int c)
	// Points the given type grid iterator to a spot at or before the given r,c coordinates.
	// Uses shortcuts to avoid having to iterate over large stretches of data to get to
	// our destination.
	{
		int	hct = (TexXSize >> 6);
		int	x = ((c >> 6) - 1);
		if (hct > 0 && x > 0) {
			Shortcut[r * hct + x].RecallIterator(tgi);
		}
	}
	

	void	PaintSurfaceValues(uint8 OutMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE],
				   int xindex, int zindex, int level,
				   uint16 NoiseMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE])
	// Writes surface-type values from this grid into the given OutMap.
	{
		// Cull if boxes don't overlap.
		if (TexXOrigin >= xindex + (1 << level) ||
		    TexZOrigin >= zindex + (1 << level) ||
		    TexXOrigin + ((TexXSize-1) << (ScaleLevel + TSHIFT)) <= xindex ||
		    TexZOrigin + ((TexZSize-1) << (ScaleLevel + TSHIFT)) <= zindex
		   )
		{
			return;
		}

		int	i, j;

		int	GridShift = ScaleLevel + TSHIFT;	// global tex coords to grid coords.
		int	MapShift = level - TBITS;	// global tex coords to (local) map coords.
		int	mag = GridShift - MapShift;
		
		if (mag <= 0) {
			// The grid is unit-scaled or minified.  Point sample.

			// This code is totally lame.  Could be improved.
			TypeGridIterator	tgi(&TypeGrid);
			for (j = 0; j < Surface::TEXTURE_SIZE; j++) {
				int	z = ((j << MapShift) + zindex - TexZOrigin) >> GridShift;
				if (z < 0 || z >= TexZSize) continue;

				for (i = 0; i < Surface::TEXTURE_SIZE; i++) {
					int	x = ((i << MapShift) + xindex - TexXOrigin) >> GridShift;
					
					if (x >= 0 && x < TexXSize) {
						tgi.SetRowCol(z, x);
						OutMap[j][i] = tgi.GetType(); //Typemap[z * TexXSize + x];
					}
				}
			}

			return;
		} else {
			// The grid data is magnified.  Noise-interpolate between samples.
			int	steps = 1 << mag;

			// j is in dest map coords.
			j = (TexZOrigin - zindex) >> MapShift;
			int	z = 0;	// z is in Grid index coords.
			if (j < -1) {
				// Skip leading rows if they're outside the dest buffer.
				z = (-1 - j) >> mag;
				j += z << mag;
			}
			TypeGridIterator	tgi0(&TypeGrid);
			TypeGridIterator	tgi1(&TypeGrid);
			for ( ; z < TexZSize-1; z++) {
				i = (TexXOrigin - xindex) >> MapShift;
				int	x = 0;
				if (i < -1) {
					// Skip leading columns if they're outside the dest buffer.
					x = (-1 - i) >> mag;
					i += x << mag;
				}

				tgi0.SetRowCol(z, x);
				tgi1.SetRowCol(z+1, x);
				
				for ( ; x < TexXSize-1; x++) {
					uint8	s[4];	// Surface types at the corners of the source block to be interpolated.
//					s[0] = Typemap[x + z * TexXSize];
//					s[1] = Typemap[1 + x + z * TexXSize];
//					s[2] = Typemap[x + (z+1) * TexXSize];
//					s[3] = Typemap[1 + x + (z + 1) * TexXSize];
					s[0] = tgi0.GetType();
					tgi0.Advance(1);
					s[1] = tgi0.GetType();
					
					s[2] = tgi1.GetType();
					tgi1.Advance(1);
					s[3] = tgi1.GetType();

					bool	Blend = true;
					if (s[0] == s[1] && s[0] == s[2] && s[0] == s[3]) {
						// No blend, just fill.
						Blend = false;
					}
					
					int k = j;
					int zcount = steps;
					if (k < 0) {
						zcount += k;
						k = 0;
					}
					if (k + zcount > Surface::TEXTURE_SIZE) {
						zcount = Surface::TEXTURE_SIZE - k;
					}

//					int	tz = (k << MapShift) + zindex;
					
					for ( ; zcount; zcount--, k++) {
						// Scan across.

						int count = steps;
						int l = i;
						if (l < 0) {
							count += l;
							l = 0;
						}
						if (l + count > Surface::TEXTURE_SIZE) {
							count = Surface::TEXTURE_SIZE - l;
						}

						if (Blend == false) {
							// Just fill.
							for ( ; count; count--, l++) {
								OutMap[k][l] = s[0];
							}
						} else {
							// Blend the four types from the corners.
//							int	tx = (l << MapShift) + xindex;
							int	offset = (1 << 7) >> mag;
							int	zthresh = (((k - j) << 8) >> mag) + offset;
							for ( ; count; count--, l++) {
								uint8	n0 = (NoiseMap[k][l] >> 8) ^ 128;
								uint8	n1 = (NoiseMap[k][l]) ^ 128;
								int	index =
									((n0 < (((l - i) << 8) >> mag) + offset) ? 1 : 0) |
									((n1 < zthresh) ? 2 : 0);
//									((Surface::Noise(tx >> 1, tz >> 1) + 127 < (((l - i) << 8) >> mag)) ? 1 : 0)
//									+ ((Surface::Noise((tx + 20000) >> 1, tz >> 1) + 127 < zthresh) ? 2 : 0);
								OutMap[k][l] = s[index];
								
//								tx += (1 << MapShift);
							}
						}

//						tz += (1 << MapShift);
					}
					
					i += steps;
					if (i >= Surface::TEXTURE_SIZE) break;
				}
				
				j += steps;
				if (j >= Surface::TEXTURE_SIZE) break;
			}
			
			return;
		}
	}

	
	void	PaintLightValues(int Width, int Height,
				 uint8 LightMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE],
				 int xindex, int zindex, int level)
	// Writes light values from this grid into the given lightmap.
	// Also deals with slope.
	// Interpolates if the grid's light values are magnified.  If the
	// grid's light is minified, then point-samples the light values.
	// xindex and zindex are offsets into global texel space.
	// The level parameter indicates the number of texels covered by the lightmap, by the
	// formula: texels = 1 << level;
	{
		uint8*	lightmap = &LightMap[0][0];
		
		// Cull if boxes don't overlap.
		if (TexXOrigin >= xindex + (1 << level) ||
		    TexZOrigin >= zindex + (1 << level) ||
		    TexXOrigin + ((TexXSize-1) << (ScaleLevel + TSHIFT)) <= xindex ||
		    TexZOrigin + ((TexZSize-1) << (ScaleLevel + TSHIFT)) <= zindex
		   )
		{
			return;
		}

		int	i, j;

		int	GridShift = ScaleLevel + TSHIFT;	// global tex coords to grid coords.
		int	MapShift = level - TBITS;	// global tex coords to (local) map coords.
		int	mag = GridShift - MapShift;
		
		if (mag <= 0) {
			// The grid is unit-scaled or minified.  Point sample.

			// This code is totally lame.  Could be improved.
			for (j = 0; j < Height; j++) {
				int	z = ((j << MapShift) + zindex - TexZOrigin) >> GridShift;
				if (z < 0 || z >= TexZSize) continue;
				
				for (i = 0; i < Width; i++) {
					int	x = ((i << MapShift) + xindex - TexXOrigin) >> GridShift;
					
					if (x >= 0 && x < TexXSize) {
						lightmap[j * Width + i] = Lightmap[z * TexXSize + x];
					}
				}
			}

			return;
		} else {
			// The grid data is magnified.  Bilerp between samples.
			int	steps = 1 << mag;
			// j is in dest map coords.
			j = (TexZOrigin - zindex) >> MapShift;
			int	z = 0;	// z is in Grid index coords.
			if (j < -1) {
				// Skip leading rows if they're outside the dest buffer.
				z = (-1 - j) >> mag;
				j += z << mag;
			}
			for ( ; z < TexZSize-1; z++) {
				i = (TexXOrigin - xindex) >> MapShift;
				int	x = 0;
				if (i < -1) {
					// Skip leading columns if they're outside the dest buffer.
					x = (-1 - i) >> mag;
					i += x << mag;
				}
				for ( ; x < TexXSize-1; x++) {
					// We're going to step down the vertical edges of the block.
					float	sl = Lightmap[x + z * TexXSize];
					float	sr = Lightmap[1 + x + z * TexXSize];
					// Steps.
					float	dl = (Lightmap[x + (z+1) * TexXSize] - sl) / steps;
					float	dr = (Lightmap[1 + x + (z + 1) * TexXSize] - sr) / steps;

					int k = j;
					int zcount = steps;
					if (k < 0) {
						sl += dl * -k;
						sr += dr * -k;
						zcount += k;
						k = 0;
					}
					if (k + zcount > Height) {
						zcount = Height - k;
					}
					
					for ( ; zcount; zcount--, k++) {
						// Lerp across.
						float	s = sl;
						float	ds = (sr - sl) / steps;
						int count = steps;
						int l = i;
						if (l < 0) {
							s += ds * -l;
							count += l;
							l = 0;
						}
						if (l + count > Width) {
							count = Width - l;
						}
						
						LerpLight(lightmap + k * Width + l, s, ds,
							count);
						
						sl += dl;
						sr += dr;
					}
					
					i += steps;
					if (i >= Width) break;
				}
				
				j += steps;
				if (j >= Height) break;
			}
			
			return;
		}
	}

	static void	LerpLight(uint8* lightdestrow, float s, float ds,
				  int count)
	// Paint a row of interpolated light & slope values.
	{
		while (count > 0) {
			*lightdestrow = frnd(s);	// xxxxx
			s += ds;
			lightdestrow++;
			count--;
		}
	}

	void	ComputeMaps()
	// Initialize the lightmap.
	{
		int	i, j;
		
		if (Config::GetBool("NoLighting")) {
			// Full brightness everywhere.
			memset(Lightmap, 255, TexZSize * TexXSize);
			return;
		}

		bool	ComputeShadows = Config::GetBool("TerrainShadow");
		int	DiffuseFactor = int(Config::GetFloat("DiffuseLightingFactor") * 255);
		float	ShadowFactor = Config::GetFloat("ShadowLightingFactor");

		vec3	Sun = Weather::GetSunDirection();
		
		float	step = TEX_SAMPLE_SPACING * (1 << (ScaleLevel + Surface::TEXTURE_BITS_PER_METER));
		float	worldz = TexZOrigin * TEX_SAMPLE_SPACING - 32768;
		float	scale = 1.0f / step;
		
		for (j = 0; j < TexZSize; j++) {
			float	worldx = TexXOrigin * TEX_SAMPLE_SPACING - 32768;
			
			for (i = 0; i < TexXSize; i++) {

				// Measure the surface normal.
				float	xcomp = TerrainMesh::GetHeight(worldx+step, worldz) - TerrainMesh::GetHeight(worldx-step, worldz);
				float	zcomp = TerrainMesh::GetHeight(worldx, worldz+step) - TerrainMesh::GetHeight(worldx, worldz-step);

				vec3	norm(xcomp, -2 * step, zcomp);	// Actually this is the negative of the surface normal.
				norm.normalize();

				// Assign lightmap value according to slope at this point.
				float	dot = norm * Sun;
				
				int	c = 255 - frnd((1 - dot) * DiffuseFactor);	 // frnd

				if (ComputeShadows) {
					// Fire a ray towards the sun to see if this point is in shadow.
					if (TerrainMesh::CheckForRayHit(vec3(worldx, TerrainMesh::GetHeight(worldx, worldz) + 0.25f, worldz), -Sun)
						>= 0)
					{
						// Darken it...
						c = int(c * ShadowFactor);
					}
				}
				
				if (c < 0) c = 0;
				if (c > 255) c = 255;
				
				Lightmap[j * TexXSize + i] = c;

				worldx += step;
			}

			worldz += step;

			if (j % 10 == 0) Game::LoadingTick();

		}
	}
	
	float	GetXOrigin() const { return XOrigin; }
	float	GetZOrigin() const { return ZOrigin; }
	// GetPitch...
	int	GetXSize() const { return XSize; }
	int	GetZSize() const { return ZSize; }
	int	GetScaleLevel() const { return ScaleLevel; }

	float	GetXMin() const { return XOrigin; }
	float	GetZMin() const { return ZOrigin; }
	float	GetXMax() const { return GetXMin() + XSize * Pitch; }
	float	GetZMax() const { return GetZMin() + ZSize * Pitch; }

	int	IntXOrigin, IntZOrigin;
	float	XOrigin, ZOrigin;
	float	Pitch;
	int	XSize, ZSize, ScaleLevel;
	int	TexXOrigin, TexZOrigin, TexXSize, TexZSize;
	uint8*	Lightmap;	//xxxxx
	TypeGridData	TypeGrid;//x

	TGIShortcut*	Shortcut;
};


extern "C" int	CompareGrids(const void* a, const void* b)
{
	const Grid**	e0 = (const Grid**) (a);
	const Grid**	e1 = (const Grid**) (b);

	if ((*e0)->ScaleLevel < (*e1)->ScaleLevel) return -1;
	else if ((*e0)->ScaleLevel > (*e1)->ScaleLevel) return 1;
	else if ((*e0)->IntXOrigin < (*e1)->IntXOrigin) return -1;
	else if ((*e0)->IntXOrigin > (*e1)->IntXOrigin) return 1;
	else if ((*e0)->IntZOrigin < (*e1)->IntZOrigin) return -1;
	else if ((*e0)->IntZOrigin > (*e1)->IntZOrigin) return 1;
	else return 0;
}


vec3	GetNormal(const vec3& loc)
// Returns the normal vec3 of the terrain at the given x,z location
// (y coord of input vec3 is ignored).
// Kinda cheesy implementation at the moment.
{
	// Measure the surface normal.
	float	x = loc.X();
	float	z = loc.Z();
	float	step = 0.5;
	float	xcomp = TerrainMesh::GetHeight(x+step, z) - TerrainMesh::GetHeight(x-step, z);
	float	zcomp = TerrainMesh::GetHeight(x, z+step) - TerrainMesh::GetHeight(x, z-step);
	
	vec3	norm(-xcomp, 2 * step, -zcomp);
	norm.normalize();

	return norm;
}


float	GetHeight(const vec3& loc)
// Returns the height of the terrain at the given point.
{
	return GetHeight(loc.X(), loc.Z());
}


float	GetHeight(float x, float z)
// Returns the height of the terrain at the given location.  (Same as above, but slightly different interface.)
{
	return TerrainMesh::GetHeight(x, z);
}


void	ProcessGrids();
void	RebuildLightmaps_lua();


void	Open()
// Do any initialization required.
{
	lua_register("terrain_rebuild_lightmaps", RebuildLightmaps_lua);
	
	Clear();
}


void	Close()
// Free data.
{
	Clear();
}


void	Clear()
// Empty our data, in preparation for closing, or for loading new data.
{
	// Clear the Grids list.
	if (Grids) {
		int	i;
		for (i = 0; i < GridCount; i++) {
			delete Grids[i];
		}
		delete [] Grids;
		Grids = NULL;
	}
	GridCount = 0;

}


void	Load(FILE* fp, FILE* fp_cache, bool Write)
// Loads the terrain information from the given filename.  If fp_cache
// is not NULL, then this routine reads or writes cached grid data from
// fp_cache.
{
	// Load the grids.
	GridCount = Read32(fp);
	Grids = new Grid*[GridCount];
	for (int i = 0; i < GridCount; i++) {
		Grids[i] = new Grid(fp, fp_cache, Write);
	}

	// Sort and sum grids.
	ProcessGrids();

	HaveGridHeights = true;
}


void	ProcessGrids()
// Sorts the grids, and computes the cumulative sums, so that the
// topmost grid at any given (x,z) coordinates contains the y value that
// is the sum of all grids below and including the topmost grid.  This
// means that retrieval of height at any point only requires querying a
// single grid.  It also simplifies somewhat the computation of min/max
// values for terrain blocks.
// Then compute lighting for all grids.
{
	// Sort so that smaller grids are closer to the start of the list.
	qsort(Grids, GridCount, sizeof(Grid**), CompareGrids);	// Grids.sort();

//	// Compute lightmaps for the grids, using processed heights.
//	Grid**	it;
//	for (it = Grids; it != Grids + GridCount; it++) {
//		Grid*	g = *it;
//		g->ComputeMaps();
//	}
}


void	RebuildLightmaps_lua()
// Relights the terrain.
{
	// Make sure sun direction is up to date.
	lua_dostring("weather_recalc_sun_direction()");
	
	// Compute lightmaps for the grids, using processed heights.
	Grid**	it;
	for (it = Grids; it != Grids + GridCount; it++) {
		Grid*	g = *it;
		g->ComputeMaps();
	}

	// Flush the surface cache so new lighting can be seen.
	Surface::FlushCache();
}


void	GetMaps(uint8 LightMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE],
		uint8 TypeMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE],
		int xindex, int zindex, int scale,
		uint16 NoiseMap[Surface::TEXTURE_SIZE][Surface::TEXTURE_SIZE])
// Fills the given LightMap and TypeMap arrays with lighting and surface
// type information, which will be used for texturing.  xindex, zindex,
// and scale determine what part of the terrain the texture covers.
// (1 << scale) == meters covered by texture.
{
//xxxx	FunctionTimer(PerfData.Lightmapping);

#if 0
	//xxxxxxxxxx
	int i;
	uint8*	p = &LightMap[0][0];
	uint8*	q = &TypeMap[0][0];
	for (i = 0; i < Surface::TEXTURE_SIZE * Surface::TEXTURE_SIZE; i++) {
		*p++ = 240;
		*q++ = 255;
	}
	//xxxxxxxxxx
#endif // 0

	Grid** it;

	// Find the topmost grid which completely encloses the desired square.
	for (it = Grids; it < Grids + GridCount - 1; it++) {
		// See if this grid completely contains the desired square.
		Grid&	g = **it;
		if (g.TexXOrigin <= xindex &&
		    g.TexZOrigin <= zindex &&
		    g.TexXOrigin + ((g.TexXSize-1) << (g.ScaleLevel + TSHIFT)) >= xindex + (1 << scale) &&
		    g.TexZOrigin + ((g.TexZSize-1) << (g.ScaleLevel + TSHIFT)) >= zindex + (1 << scale)
		   )
		{
			break;
		}
	}

	// Paint grids, bottom to top.
	for (/*it = Grids + GridCount - 1*/; it > Grids - 1; it--) {
		(*it)->PaintLightValues(Surface::TEXTURE_SIZE, Surface::TEXTURE_SIZE, LightMap, xindex, zindex, scale);
		(*it)->PaintSurfaceValues(TypeMap, xindex, zindex, scale, NoiseMap);
	}
}


int	GetType(const vec3& location)
// Returns the type of the surface under the (x,z) coordinates in the given vec3.
// The y coordinate is ignored.
{
	// Convert world coordinates to texture coordinates.
	float	tx = (location.X() + 32768) / TEX_SAMPLE_SPACING;
	float	tz = (location.Z() + 32768) / TEX_SAMPLE_SPACING;
	
	int	ix = frnd(tx);
	int	iz = frnd(tz);
	
	// Go through the grid list from smallest to largest and find the first type map which contains the query.
	for (Grid** it = Grids; it != Grids + GridCount; it++) {
		Grid&	g = **it;

		// Convert query into grid's texture coordinates.
		int	sampx = (ix - g.TexXOrigin) >> (g.ScaleLevel + TSHIFT);
		int	sampz = (iz - g.TexZOrigin) >> (g.ScaleLevel + TSHIFT);

		TypeGridIterator	tgi(&(g.TypeGrid));
		if (sampx >= 0 && sampx < g.TexXSize-1 && sampz >= 0 && sampz < g.TexZSize-1) {
			// Query is within this type map.  Look up the value and return it.
			uint8	s[2][2];
//			s[0][0] = g.Typemap[sampx + sampz * g.TexXSize];
//			s[0][1] = g.Typemap[sampx + 1 + sampz * g.TexXSize];
//			s[1][0] = g.Typemap[sampx + (sampz + 1) * g.TexXSize];
//			s[1][1] = g.Typemap[sampx + 1 + (sampz + 1) * g.TexXSize];

			g.ShortcutToIterator(&tgi, sampz, sampx);
			tgi.SetRowCol(sampz, sampx);
			s[0][0] = tgi.GetType();
			tgi.Advance(1);
			s[0][1] = tgi.GetType();

			g.ShortcutToIterator(&tgi, sampz+1, sampx);
			tgi.SetRowCol(sampz+1, sampx);
			s[1][0] = tgi.GetType();
			tgi.Advance(1);
			s[1][1] = tgi.GetType();

			int	n = Surface::Noise(ix, iz);
			uint8	xthresh = (n >> 8) ^ 128;
			uint8	zthresh = (n) ^ 128;
			int	l = (((ix - g.TexXOrigin) << 8) >> (g.ScaleLevel + TSHIFT)) & 255;
			int	k = (((iz - g.TexZOrigin) << 8) >> (g.ScaleLevel + TSHIFT)) & 255;
			
			return s[k > zthresh][l > xthresh];
			
//			return g.Typemap[sampx + sampz * g.TexXSize];
		}
	}
	
	// Query isn't inside any grid.
	return 0;
}


bool	FindIntersectionCoarse(vec3* result, const vec3& loc, const vec3& ray)
// Given a location and direction, this function searches for the first
// intersection of the specified ray with the terrain and puts the
// result in *result and returns true.  If no intersection can be found,
// then returns false and leaves *result unchanged.
{
	// Project the ray onto the x-z plane, and normalize it.
	vec3	r(ray);
	float	FlatMag = r.X() * r.X() + r.Z() * r.Z();
	if (FlatMag < 0.0000001) {
		// Degenerate case of shooting straight down.  No need to search for intersection.
		float	y = TerrainModel::GetHeight(loc);
		*result = vec3(loc.X(), y, loc.Z());
		return true;
	}

	r /= FlatMag;	// Now the x-z part is unit length.

	vec3	v0(loc);
	float	scale = 20.0;
	int	MaxCount = 1250;
	int	level = 0;

	for (int i = 0; i < MaxCount; i++) {
		vec3	probe(v0 + r * (scale * i));
		float	ProbeY = TerrainModel::GetHeight(probe);

		if (probe.Y() < ProbeY) {
			// Found intersection.
			if (scale < 2.0) {
				// OK, we've refined enough, return the result.
				*result = vec3(probe.X(), ProbeY, probe.Z());
				return true;
			} else {
				// Refine the search parameters, and search around this area with finer sampling.
				v0 = probe - r * scale;	// Back up to previous point which didn't intersect.
				scale = 1.0;	// Take shorter steps.
				MaxCount = 21;
				i = 0;
				
				continue;
			}
		}
	}

	// Haven't found anything worthwhile.
	return false;
}



};	// end namespace TerrainModel.
