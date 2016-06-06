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
// model.hpp	-thatcher 2/6/1998 Copyright Thatcher Ulrich

// Declaration of MObject, a type used for simulated physical objects.


#ifndef MODEL_HPP
#define MODEL_HPP


#include <stdio.h>
#include "geometry.hpp"
#include "render.hpp"
#include "view.hpp"
#include "gameloop.hpp"
#include "collide.hpp"


struct UpdateState;


// Base class for geometric models.
class GModel {
public:
	GModel();
	virtual ~GModel();
	
	virtual void	Render(ViewState& s, int ClipHint);
	float	GetRadius() const { return Radius; }
	
protected:
	float	Radius;
	
	// collision stuff?
};


// GModel mix-in for articulation control.
class GArticulated : virtual public GModel {
public:
	virtual void	SetParameter(int dof, float value) = 0;	// For articulation of parameterized models.
	virtual void	GetDynamicsParameters(vec3* FigureOriginOffset, float* InverseMass, matrix33* InverseInertiaTensor) = 0;
//	virtual void	PaintShadow(bitmap32* dest, float DestWidth, const vec3& light_direction, const vec3& shadow_up) = 0;
	virtual void	GetOriginOffset(vec3* FigureOriginOffset) = 0;
};


// Base class for collision models.
class SModel {	// "S" for "Solid"
public:
	SModel();
	virtual ~SModel();
	
	virtual bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl) = 0;
	float	GetRadius() const { return Radius; }

protected:
	float	Radius;
};


// Base class for simulation objects.
class MObject {
public:
	MObject();
	virtual ~MObject();

	void	LoadLocation(FILE* fp);
	
	virtual const vec3&	GetLocation() const = 0;

	virtual void	SetLocation(const vec3& NewLocation) = 0;

	// Make this a member of ActiveObject.
	virtual void	Update(const UpdateState& u) = 0;
	
	virtual void	Render(ViewState& s, int ClipHint);

	virtual bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl);
	
	virtual void	Reset(const vec3& Loc, const vec3& Dir, const vec3& Vel);

	// Visual and Solid.
	GModel*	GetVisual() const { return Visual; }
	SModel*	GetSolid() const { return Solid; }
	void	SetVisual(GModel* vis) { Visual = vis; }
	void	SetSolid(SModel* sol) { Solid = sol; }

	float	GetRadius() const;
	
public:
	// For linking to the object databases.
	MObject*	Next;
	MObject*	Previous;
	
protected:
	GModel*	Visual;
	SModel*	Solid;
	
	// update object?  May require state.  Perhaps leave it here.
};


/**
 * @class MOriented
 * @brief Base class for oriented objects.
 */
class MOriented : virtual public MObject {
public:
	MOriented();
	
	void	LoadOrientation(FILE* fp);
	
	const vec3&	GetLocation() const { return const_cast<matrix&>(Matrix).GetColumn(3); }
	const vec3&	GetDirection() const { return const_cast<matrix&>(Matrix).GetColumn(0); }
	const vec3&	GetUp() const { return const_cast<matrix&>(Matrix).GetColumn(1); }
	const vec3&	GetRight() const { return const_cast<matrix&>(Matrix).GetColumn(2); }
	
	void	SetLocation(const vec3& NewLocation) { Matrix.SetColumn(3, NewLocation); }	// Edit later for spatial subdivision.
	
	const matrix&	GetMatrix() const { return Matrix; }
	quaternion	GetOrientation() const { return Matrix.GetOrientation(); }
	void	SetMatrix(const matrix& NewMatrix) { Matrix = NewMatrix; }
	void	SetOrientation(const quaternion& q) { Matrix.SetOrientation(q); }

	void	SetDirection(const vec3& NewDirection) { Matrix.SetColumn(0, NewDirection); }
	void	SetUp(const vec3& NewUp) { Matrix.SetColumn(1, NewUp); Matrix.SetColumn(2, Matrix.GetColumn(0).cross(NewUp)); /* should use dirty bits, lazy eval of cross product */ }

	bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl);
	virtual void	Render(ViewState& s, int ClipHint);

private:
	matrix	Matrix;
};

	
// Base class for dynamic objects.  Handles collision checking.
class MDynamic : public MOriented {
public:
	MDynamic();
	
	void	Update(const UpdateState& u);

	virtual void	RunDynamics(const UpdateState& u) = 0;
	virtual void	RunLogic(const UpdateState& u);
	virtual void	RunLogicPostDynamics(const UpdateState& u);
	virtual void	CollisionNotify(const UpdateState& u, const vec3& loc, const vec3& normal, const vec3& impulse);

	void	SetVelocity(const vec3& v) { Velocity = v; }
	const vec3&	GetVelocity() const { return Velocity; }

//	void	SetOmega(const vec3& o) { Omega = o; }
//	const vec3&	GetOmega() const { return Omega; }
	void	SetAngMomentum(const vec3& l) { L = l; }
	const vec3&	GetAngMomentum() const { return L; }
private:
	vec3	Velocity;
//	vec3	Omega;
	vec3	L;
};


namespace Model {

	void	Open();
	void	Close();
	void	Clear();

	// GModel functions.
	typedef GModel* (*GModelLoaderFP)(const char*);
	void	LoadGModels(FILE* fp);
	void	AddGModelLoader(const char* FileExtension, GModelLoaderFP FileLoader);
	GModel*	GetGModel(int index);
	GModel*	LoadGModel(const char* filename);

	// SModel functions.
	typedef SModel* (*SModelLoaderFP)(FILE* fp);
	void	LoadSModels(FILE* fp);
	void	AddSModelLoader(int TypeID, SModelLoaderFP FileLoader);
	SModel*	GetSModel(int index);
	
	// MObject functions.
	void	LoadObjects(FILE* fp);
	void	AddObjectLoader(int TypeID, void (*LoadFunction)(FILE*));
	void	AddStaticObject(MObject* o);
	void	AddDynamicObject(MObject* o);

	// Texture functions.
	Render::Texture*	GetTexture(const char* filename, bool NeedAlpha, bool MakeMIPMaps, bool Tile);
	int	GetTexelCount();

	int	GetRenderedTriangleCount();
	void	AddToTriangleCount(int amount);
	
	// Operations.
	void	Update(const UpdateState& u);
	void	Render(ViewState& s);
	bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl);
};


#endif // MODEL_HPP

