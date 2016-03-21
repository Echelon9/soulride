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
// mesh.cpp	-thatcher 12/3/1998 Copyright Slingshot

// Code for a MeshModel class, which implements basic triangle mesh models.


#include <math.h>

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif


#include <map>
using namespace std;

#include "ogl.hpp"
#include "clip.hpp"
#include "model.hpp"
#include "gameloop.hpp"
#include "utility.hpp"
#include "console.hpp"


class MeshLoadError : public Error {};


class MeshModel : public GModel {
public:
	MeshModel(FILE* fp, const char* filename)
	// Initializes and loads mesh data from the given stream.  The
	// filename parameter is used to make more informative errors &
	// warnings.  Pass in "" if you don't have a filename.
	{
		// Read and check signature.
		uint32	sig = Read32(fp);

		if ((sig & 0x00FFFFFF) != 0x004D5253 /* "SRM" */) {
			Error e; e << "SRM file '" << filename << "' can't be loaded; has a bad signature.";
			throw e;
		}
		if ((sig >> 24) != 1) {
			Error e; e << "SRM file '" << filename << "' has an incompatible version.";
			throw e;
		}
			
		// Read mesh count.
		MeshCount = Read32(fp);

		// Create Mesh array.
		Mesh = new MeshData*[MeshCount];

		Radius = 0;
		
		// Load meshes.
		for (int i = 0; i < MeshCount; i++) {
			Mesh[i] = new MeshData(fp, filename);
			if (Mesh[i]->MaxRadius > Radius) Radius = Mesh[i]->MaxRadius;
		}	
	}

	
	virtual ~MeshModel()
	// Free everything.
	{
		for (int i = 0; i < MeshCount; i++) {
			delete Mesh[i];
		}
		delete [] Mesh;
	}


	void	Render(ViewState& s, int ClipHint)
	// Render the mesh using the given viewing parameters.
	{
		int	i;
		
		for (i = 0; i < MeshCount; i++) {
			Mesh[i]->Render(s);
		}
	}


// data:
	int	MeshCount;

	struct MeshData {

		MeshData(FILE* fp, const char* filename)
		// Construct the mesh from the given input stream.  The filename parameter is used
		// to make more informative warning messages.
		{
			MaxRadius = 0;
			
			int	i, j;
			
			// Vertices.
			VertexCount = Read16(fp);
			vec3*	TempVertex = new vec3[VertexCount];

			for (i = 0; i < VertexCount; i++) {
				// Load vertex.
				TempVertex[i].SetX(ReadFloat(fp));
				TempVertex[i].SetY(ReadFloat(fp));
				TempVertex[i].SetZ(ReadFloat(fp));

				// Update max radius if necessary.
				float	mag = TempVertex[i].magnitude();
				if (mag > MaxRadius) MaxRadius = mag;
			}

			// Texture vertices.
			TextureVertexCount = Read16(fp);
			UVData*	TempTextureVertex = new UVData[TextureVertexCount];

			for (i = 0; i < TextureVertexCount; i++) {
				TempTextureVertex[i].U = ReadFloat(fp);
				TempTextureVertex[i].V = ReadFloat(fp);
			}

			// Faces.
			FaceCount = Read32(fp);
			Texture = NULL;


			struct TempFaceData {
				uint16	v[3];
				uint16	tv[3];
			} *TempFace;
			TempFace = new TempFaceData[FaceCount];

			for (i = 0; i < FaceCount; i++) {
				int j;
				
				// Vertex references.
				for (j = 0; j < 3; j++) {
					TempFace[i].v[j] = Read16(fp);
				}

				// flags
				uint32 flags = Read32(fp);

				if (flags & 1) {
					// texture verts
					for (j = 0; j < 3; j++) {
						TempFace[i].tv[j] = Read16(fp);
					}

					// texture name
					char	name[80];
					fgets(name, 80, fp);
					// Chop off trailing '\n'.
					name[strlen(name) - 1] = 0;	// xxx bug if name is null string.

					if (Texture == NULL) {
						Texture = Model::GetTexture(name, false, true, true);
					}
					
				} else {

					// Log a message if we find a non-textured face.
					Console::Printf("MeshData(): non-textured face found in model '%s'\n", filename);
					
					// color
					/*color =*/ Read32(fp);
					
					for (j = 0; j < 3; j++) {
						TempFace[i].tv[j] = 0;
					}

//					// Swap the bytes around, since OpenGL expects a big-endian-influenced rgb format.
//					color = ((color & 0xFF000000)) |
//						((color & 0x00FF0000) >> 16) |
//						((color & 0x0000FF00)) |
//						((color & 0x000000FF) << 16);
				}
				
			}

//			// Load face data.
//			FaceData*	TempFace = new FaceData[FaceCount];
//			for (i = 0; i < FaceCount; i++) {
//				TempFace[i].Init(fp);
//			}

			// Sort out the indices problem.

			// Make a collection of vertex/uv values, removing duplicates.
			map<uint32, uint16>	verts;
			for (i = 0; i < FaceCount; i++) {
				for (j = 0; j < 3; j++) {
					uint32	key = (TempFace[i].v[j] << 16) | (TempFace[i].tv[j]);
					verts.insert(pair<uint32,uint16>(key, 0));
				}
			}

			// Read them in order, and assign indices.
			int	count = 0;
			map<uint32,uint16>::iterator	it;
			for (it = verts.begin(); it != verts.end(); it++) {
				(*it).second = count;	// Assign index.
				count++;
			}

			// Fill the face vert index array.
			FaceIndices = new uint16[FaceCount * 3];
			for (i = 0; i < FaceCount; i++) {
				for (j = 0; j < 3; j++) {
					uint32	key = (TempFace[i].v[j] << 16) | (TempFace[i].tv[j]);
					it = verts.find(key);
					FaceIndices[i*3 + j] = (*it).second;
				}
			}

			// Build the real vertex & texture arrays.
			Vertex = new vec3[count];
			TextureVertex = new UVData[count];
			for (it = verts.begin(); it != verts.end(); it++) {
				int	index = (*it).second;
				uint32	key = (*it).first;
				Vertex[index] = TempVertex[key >> 16];
				TextureVertex[index] = TempTextureVertex[key & 0x0FFFF];
			}

			// Delete temporary stuff.
			delete [] TempVertex;
			delete [] TempTextureVertex;
			delete [] TempFace;

			verts.clear();
		}
		

		~MeshData()
		// Destructor.  Free everything.
		{
			delete [] Vertex;
			delete [] TextureVertex;
//			delete [] Face;
			delete [] FaceIndices;
		}

		void	Render(ViewState& s)
		// Renders this mesh using the given view parameters.
		{
//			TransformVertices(s);

			DrawFaces(s, Vertex);
		}

		void	DrawFaces(ViewState& s, vec3 Vertices[])
		// Draw the mesh faces.
		{
			int	i;

#if 0
			// Draw the faces.
			for (i = 0; i < FaceCount; i++) {
				FaceData&	f = Face[i];
				if (f.flags & 1) {
					// Textured.
					Render::SetTexture(f.texture);
					Render::CommitRenderState();
					glBegin(GL_TRIANGLES);
					glColor3f(1, 1, 1);
					
					for (int j = 0; j < 3; j++) {
						glTexCoord2fv(&TextureVertex[f.tv[j]].U);
						glVertex3fv((float*) &Vertices[f.v[j]]);
					}
					
					glEnd();
				} else {
					// Flat shaded.
					Render::SetTexture(NULL);
					Render::CommitRenderState();
					glBegin(GL_TRIANGLES);
					
					glColor3f(((f.color >> 16) & 255) / 255.0,	// This retarded thing is an attempt to run with Quake MiniGL drivers.
						((f.color >> 8) & 255) / 255.0,
						(f.color & 255) / 255.0);
						
					for (int j = 0; j < 3; j++) {
						glVertex3fv((float*) &Vertices[f.v[j]]);
					}
					glEnd();
				}
			}
#endif // 0

			Render::SetTexture(Texture);
			Render::CommitRenderState();
			
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Vertex);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, (float*) TextureVertex);
//			glLockArraysEXT(0, VertexCount);

			glDrawElements(GL_TRIANGLES, FaceCount * 3, GL_UNSIGNED_SHORT, FaceIndices);

//			glUnlockArraysEXT();
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
			
			Model::AddToTriangleCount(FaceCount);
		}

		
		int	VertexCount;
		vec3*	Vertex;
		
		int	TextureVertexCount;
		struct UVData {
			float	U, V;
		};
		UVData*	TextureVertex;
		
		int	FaceCount;
		uint16*	FaceIndices;
		Render::Texture*	Texture;
		
		struct FaceData {
			
			void Init(FILE* fp)
			// Initialize a face from the input stream.
			{
				int i;
				
				// Vertex references.
				for (i = 0; i < 3; i++) {
					v[i] = Read16(fp);
				}

				// flags
				flags = Read32(fp);

				if (flags & 1) {
					// texture verts
					for (i = 0; i < 3; i++) {
						t.tv[i] = Read16(fp);
					}

					// texture name
					char	name[80];
					fgets(name, 80, fp);
					// Chop off trailing '\n'.
					name[strlen(name) - 1] = 0;	// xxx bug if name is null string.

					t.texture = Model::GetTexture(name, false, true, true);
					
				} else {
					// color
					color = Read32(fp);

					// Swap the bytes around, since OpenGL expects a big-endian-influenced rgb format.
					color = ((color & 0xFF000000)) |
						((color & 0x00FF0000) >> 16) |
						((color & 0x0000FF00)) |
						((color & 0x000000FF) << 16);
				}
			}

			uint16	v[3];	// Vertices.
			int	flags;	// bit 0: 1 --> textured, 0 --> flat.
			union {
				struct {
					uint16	tv[3];	// Texture vertices.
					::Render::Texture*	texture;
				} t;
				uint32	color;
			};
		};
		FaceData*	Face;

		float	MaxRadius;
	};
	MeshData**	Mesh;

};



#ifdef NOT


//
// class LerpMesh -- contains two meshes, and renders the interpolation between the two.
//


class LerpMesh : public GArticulated {
	MeshModel*	SourceMesh[2];
	float	Interpolant;
	
	static int	MaxVertices;
	static vec3*	TempVerts;
	
public:
	LerpMesh()
	// Constructor.  Load the two meshes.
	{
		FILE*	fp;
		fp = Utility::FileOpen("boarder.srm", "rb");
		SourceMesh[0] = new MeshModel(fp);
		fclose(fp);

		fp = Utility::FileOpen("boarder-crouched.srm", "rb");
		SourceMesh[1] = new MeshModel(fp);
		fclose(fp);

		Interpolant = 0;

		// Make sure lerp will work.
		if (SourceMesh[0]->MeshCount != SourceMesh[1]->MeshCount) {
			Error e; e << "Can't construct LerpMesh; mesh counts don't match.";
			throw e;
		}

		int	i;
		int	MeshCount = SourceMesh[0]->MeshCount;
		for (i = 0; i < MeshCount; i++) {
			int	Mesh0Verts = SourceMesh[0]->Mesh[i]->VertexCount;
			
			// Make sure the two meshes have the same number of verts.
			if (Mesh0Verts != SourceMesh[1]->Mesh[i]->VertexCount) {
				Error e; e << "Can't construct LerpMesh; meshes[" << i << "] have different numbers of verts.";
				throw e;
			}

			// Update MaxVertices if necessary.
			if (Mesh0Verts > MaxVertices) {
				if (TempVerts) {
					delete [] TempVerts;
					TempVerts = NULL;
				}
				MaxVertices = Mesh0Verts;
			}
		}

		// Set the radius.
		Radius = SourceMesh[0]->GetRadius();
		if (SourceMesh[1]->GetRadius() > Radius) Radius = SourceMesh[1]->GetRadius();
	}

	virtual ~LerpMesh()
	// Destructor.
	{
		if (TempVerts) delete [] TempVerts;
		delete SourceMesh[0];
		delete SourceMesh[1];
	}

	void	SetParameter(int index, float Value)
	// Only one parameter (index == 0), for degree of interpolation between the two models.
	{
		if (index == 0) {
			Interpolant = Value;
		}
	}
	
	void	Render(ViewState& s, int ClipHint)
	// Render the lerped mesh.
	{
		// Allocate temp vertex array if necessary.
		if (TempVerts == NULL) {
			// Create a scratch array of vec3s.
			TempVerts = new vec3[MaxVertices];
		}

		vec3	v;
		float	c0 = 1 - Interpolant;
		float	c1 = Interpolant;
		
		int	MeshCount = SourceMesh[0]->MeshCount;
		int	i;
		for (i = 0; i < MeshCount; i++) {
			// Transform the interpolated vertices.
			MeshModel::MeshData*	mesh0 = SourceMesh[0]->Mesh[i];
			MeshModel::MeshData*	mesh1 = SourceMesh[1]->Mesh[i];
			
			int	VertCount = mesh0->VertexCount;
			int	j;
			for (j = 0; j < VertCount; j++) {
				vec3&	v = TempVerts[j];
				
				v.SetX(mesh0->Vertex[j].X() * c0 + mesh1->Vertex[j].X() * c1);
				v.SetY(mesh0->Vertex[j].Y() * c0 + mesh1->Vertex[j].Y() * c1);
				v.SetZ(mesh0->Vertex[j].Z() * c0 + mesh1->Vertex[j].Z() * c1);

//				// xxx skip for OpenGL.
//				s.ViewMatrix.Apply(&MeshModel::TempVerts[j].GetViewVector(), v);
//				MeshModel::TempVerts[j].SetDirty();
				// vertex lighting?
			}

			// Render mesh[0] with the interpolated verts.
			mesh0->DrawFaces(s, TempVerts);
		}
	}
};


int	LerpMesh::MaxVertices = 10;
vec3*	LerpMesh::TempVerts = NULL;


#endif // NOT


//
//
//


// Initialization hooks.
static struct InitMeshModel {
	InitMeshModel() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init() {
		Model::AddGModelLoader("srm", SRMLoader);
//		Model::AddGModelLoader("lrp", LerpLoader);
	}

	static GModel*	SRMLoader(const char* filename) {
		FILE*	fp = Utility::FileOpen(filename, "rb");
		if (fp == NULL) {
			Error e; e << "Can't open file '" << filename << "' for input.";
			throw e;
		}
		MeshModel*	m;
		m = new MeshModel(fp, filename);
		
		fclose(fp);
		return m;
	}

//	static GModel*	LerpLoader(const char* filename) {
//		LerpMesh*	m = new LerpMesh();
//		return m;
//	}
} InitMeshModel;

