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
// staticlod.cpp	-thatcher 12/6/1999 Copyright Slingshot

// Code for old-fashioned switching between different static LOD models, based
// on camera z-distance.


#include <ctype.h>
#include "clip.hpp"
#include "model.hpp"
#include "gameloop.hpp"
#include "utility.hpp"


class StaticLOD : public GModel {
	int	ModelCount;
	struct ModelInfo {
		float	Threshold;
		GModel*	Model;
	};
	ModelInfo*	Models;
	
public:
	StaticLOD(FILE* fp)
	// Initialize and load LOD info from the given file.
	{
		uint32	sig, version;
		
		// Read signature.
		sig = Read32(fp);
		if (sig != 0x00444F4C /* "LOD\0" */) {
			Error e; e << "StaticLOD: Unknown file type; looking for LOD";
			throw e;
		}
		
		// Read version #.
		version = Read32(fp);
		if (version != 1) {
			Error e; e << "StaticLOD: incompatible file version";
			throw e;
		}
		
		// Read model count.
		ModelCount = Read32(fp);

		// Allocate model array.
		Models = new ModelInfo[ModelCount];
		
		// Read the model info.
		char	temp[256];
		int	i;
		for (i = 0; i < ModelCount; i++) {
			// Get the threshold distance.
			Models[i].Threshold = ReadFloat(fp);

			// Get the model name.
			fgets(temp, 256, fp);
			temp[strlen(temp) - 1] = 0;	// Delete trailing '\n'.
			// Convert to lower-case.
			for (char* p = temp; *p; p++) {
				*p = tolower(*p);
			}

			Models[i].Model = Model::LoadGModel(temp);
		}
	}

	~StaticLOD()
	// Free stuff.
	{
		if (Models) {
			delete [] Models;
			Models = NULL;
			ModelCount = 0;
		}
	}

	void	Render(ViewState& s, int ClipHint)
	// Pick the model whose threshold is largest but still less than the view z,
	// and render it.
	{
		if (ModelCount <= 0) return;
		
		// Get the view z of the object origin.
		float	ViewZ = s.ViewMatrix.GetColumn(3).Z();
		
		// Scan furthest-first in order to waste less time on
		// (presumably more numerous) distant models.
		GModel*	model = Models[ModelCount-1].Model;
		int	i;
		for (i = ModelCount-1; i >= 0; i--) {
			if (Models[i].Threshold < ViewZ) {
				model = Models[i].Model;
				break;
			}
		}

		// Render the chosen model.
		if (model) model->Render(s, ClipHint);
	}
};



// Initialization hooks.
static struct InitStaticLOD {
	InitStaticLOD() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init() {
		Model::AddGModelLoader("lod", LODLoader);
	}

	static GModel*	LODLoader(const char* filename) {
		FILE*	fp = Utility::FileOpen(filename, "rb");
		if (fp == NULL) {
			Error e; e << "Can't open file '" << filename << "' for input.";
			throw e;
		}
		StaticLOD*	m = new StaticLOD(fp);
		fclose(fp);
		return m;
	}
} InitStaticLOD;

