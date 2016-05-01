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
// geometry.hpp	-thatcher 1/28/1998 Copyright Thatcher Ulrich

// Some basic geometric types.


#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP


#include "types.hpp"


/**
 * 2D point class defined by way of its (x,y) co-ordinate
 *
 * @todo add, subtract, scalar mult?, dot?
 */
class point
{
public:
	point() { X = 0; Y = 0; }
	point(int x, int y) : X(x), Y(y) {}

	int     GetX() const { return X; }
	int     GetY() const { return Y; }
	void    SetX(int newx) { X = newx; }
	void    SetY(int newy) { Y = newy; }

private:
	int	X, Y;
};


/**
 * 3-element vec3 class, for 3D math.
 *
 * @todo Default initializer set (x, y, z) = (0, 0 ,0)?
 */
class	vec3
{
public:
	vec3() {}
	vec3(float X, float Y, float Z) { x[0] = X; x[1] = Y; x[2] = Z; }
	vec3(const vec3& v) { x[0] = v.x[0]; x[1] = v.x[1]; x[2] = v.x[2]; }

	operator	const float*() const { return &x[0]; }

	vec3&	Init(float X, float Y, float Z) { x[0] = X; x[1] = Y; x[2] = Z; return *this; }

	float&	operator[](int i) { return x[i]; }
	const float&	operator[](int i) const { return x[i]; }
	
	float	Get(int element) const { return x[element]; }
	void	Set(int element, float NewValue) { x[element] = NewValue; }
	float	X() const { return x[0]; }
	float	Y() const { return x[1]; }
	float	Z() const { return x[2]; }
	void	SetX(float newx) { x[0] = newx; }
	void	SetY(float newy) { x[1] = newy; }
	void	SetZ(float newz) { x[2] = newz; }
	void	SetXYZ(float newx, float newy, float newz) { x[0] = newx; x[1] = newy; x[2] = newz; }
	
	vec3	operator+(const vec3& v) const;
	vec3	operator-(const vec3& v) const;
	vec3	operator-() const;
	float	operator*(const vec3& v) const;
	vec3	operator*(float f) const;
	vec3	operator/(float f) const { return this->operator*(1.0f / f); }
	vec3	cross(const vec3& v) const;

	vec3&	normalize();
	vec3&	operator=(const vec3& v) { x[0] = v.x[0]; x[1] = v.x[1]; x[2] = v.x[2]; return *this; }
	vec3&	operator+=(const vec3& v);
	vec3&	operator-=(const vec3& v);
	vec3&	operator*=(float f);
	vec3&	operator/=(float f) { return this->operator*=(1.0f / f); }
	bool	operator==(const vec3& rhs);
	bool	operator!=(const vec3& rhs);

	float	magnitude() const;
	float	sqrmag() const;
//	float	min() const;
//	float	max() const;
//	float	minabs() const;
//	float	maxabs() const;

	bool	checknan() const;
private:
	float	x[3];
};


#define INLINE_VECTOR


#ifdef INLINE_VECTOR


/**
 * Dot product.
 */
inline float
vec3::operator*(const vec3& v) const
{
	float	result;
	result = x[0] * v.x[0];
	result += x[1] * v.x[1];
	result += x[2] * v.x[2];
	return result;
}


/**
 * Adds a vec3 to *this.
 */
inline vec3&
vec3::operator+=(const vec3& v)
{
	x[0] += v.x[0];
	x[1] += v.x[1];
	x[2] += v.x[2];
	return *this;
}


/**
 * Subtracts a vec3 from *this.
 */
inline vec3&
vec3::operator-=(const vec3& v)
{
	x[0] -= v.x[0];
	x[1] -= v.x[1];
	x[2] -= v.x[2];
	return *this;
}


#endif // INLINE_VECTOR




extern vec3	ZeroVector, XAxis, YAxis, ZAxis;


class	quaternion;


/**
 * 3x3 matrix class, for 3D rotations and inertia tensors.
 *
 */
class matrix33
{
public:
	matrix33() 	{ Identity(); }
	matrix33(vec3& v1,vec3& v2, vec3& v3) { m[0] = v1; m[1] = v2; m[2] = v3; }
	matrix33(float v1X, float v1Y, float v1Z,
	         float v2X, float v2Y, float v2Z,
	         float v3X, float v3Y, float v3Z)
	         {
			m[0] = vec3(v1X, v1Y, v1Z);
			m[1] = vec3(v2X, v2Y, v2Z);
			m[2] = vec3(v3X, v3Y, v3Z);
	         }
	matrix33(const matrix33& m1) { m[0] = m1[0]; m[1] = m1[1]; m[2] = m1[2]; }

	vec3&		operator[](int i) { return m[i]; }
	const vec3&	operator[](int i) const { return m[i]; }

	void		SetColumn(int column, const vec3& v) { m[column] = v; }
	const vec3&	GetColumn(int column) const { return m[column]; }
	void		SetColumns(const vec3& col0, const vec3& col1, const vec3& col2);

	vec3		operator*(const vec3& v) const;
	matrix33	operator*(const matrix33& m) const;

	matrix33&	operator=(const matrix33& rhs) { m[0] = rhs[0]; m[1] = rhs[1]; m[2] = rhs[2]; return *this; }
	matrix33&	operator*=(float f);
	matrix33&	operator/=(float f) { return this->operator*=(1.0f / f); }
	matrix33&	operator+=(const matrix33& m);
	matrix33&	operator-=(const matrix33& m);
	matrix33&	operator*=(const matrix33& m);
	bool		operator==(const matrix33& rhs);
	bool		operator!=(const matrix33& rhs);
	
	void		Identity();
	void		Invert();
	matrix33&	MultTranspose(const matrix33& m);
	void		Orient(const vec3& dir, const vec3& up);
	
private:
	vec3	m[3];
};


class	matrix
// 3x4 matrix class, for 3D transformations.
{
public:
	matrix() { Identity(); }

	operator matrix33() const { return *((matrix33*) &m[0]); }	// Reinterpret the rotation part as a matrix33.
	
	void	Identity();
	void	View(const vec3& ViewNormal, const vec3& ViewUp, const vec3& ViewLocation);
	void	Orient(const vec3& ObjectDirection, const vec3& ObjectUp, const vec3& ObjectLocation);

	static void	Compose(matrix* dest, const matrix& left, const matrix& right);
	vec3	operator*(const vec3& v) const;
	matrix	operator*(const matrix& m) const;
//	operator*=(const quaternion& q);

	matrix&	operator*=(float f);
	matrix&	operator+=(const matrix& m);
	
	void	Invert();
	void	InvertRotation();
	void	NormalizeRotation();
	void	Apply(vec3* result, const vec3& v) const;
	void	ApplyRotation(vec3* result, const vec3& v) const;
	void	ApplyInverse(vec3* result, const vec3& v) const;
	void	ApplyInverseRotation(vec3* result, const vec3& v) const;
	void	Translate(const vec3& v);
	void	SetOrientation(const quaternion& q);
	quaternion	GetOrientation() const;
	
	void	SetColumn(int column, const vec3& v) { m[column] = v; }
	const vec3&	GetColumn(int column) const { return m[column]; }
private:
	vec3	m[4];
};


// class quaternion -- handy for representing rotations.

class quaternion {
public:
	quaternion() : S(1), V(ZeroVector) {}
	quaternion(const quaternion& q) : S(q.S), V(q.V) {}
	quaternion(float s, const vec3& v) : S(s), V(v) {}

	quaternion(const vec3& Axis, float Angle);	// Slightly dubious: semantics varies from other constructor depending on order of arg types.

	float	GetS() const { return S; }
	const vec3&	GetV() const { return V; }
	void	SetS(float s) { S = s; }
	void	SetV(const vec3& v) { V = v; }

	float	Get(int i) const { if (i==0) return GetS(); else return V.Get(i-1); }
	void	Set(int i, float f) { if (i==0) S = f; else V.Set(i-1, f); }

	quaternion	operator*(const quaternion& q) const;
	quaternion&	operator*=(float f) { S *= f; V *= f; return *this; }
	quaternion&	operator+=(const quaternion& q) { S += q.S; V += q.V; return *this; }

	quaternion&	operator=(const quaternion& q) { S = q.S; V = q.V; return *this; }
	quaternion&	normalize();
	quaternion&	operator*=(const quaternion& q);
	void	ApplyRotation(vec3* result, const vec3& v);
	
	quaternion	lerp(const quaternion& q, float f) const;
private:
	float	S;
	vec3	V;
};


// Generic base class for bitmaps.
class bitmap {
public:
	virtual ~bitmap() {}
	int	GetWidth() { return Width; }
	int	GetHeight() { return Height; }

	// These are not safe, in general!
	void	SetWidth(int w) { Width = w; }
	void	SetHeight(int h) { Height = h; }

protected:
	int	Width, Height;
	bitmap(int w, int h) { Width = w; Height = h; };
};


#ifdef NOT

// class bitmap16 -- for storing 16-bit ARGB textures.


class bitmap16 : public bitmap {
public:
	bitmap16(int w, int h) : bitmap(w, h) { Data = new uint16[Width * Height]; }
	~bitmap16() { delete [] Data; }
	uint16*	GetData() { return Data; }
	void	ProcessForColorKeyZero();
private:
	uint16*	Data;
};


#endif // NOT


// class bitmap32 -- for storing 32-bit ARGB textures.

class bitmap32 : public bitmap {
public:
	bitmap32(int w, int h) : bitmap(w, h) { Data = new uint32[Width * Height]; }
	~bitmap32() { delete [] Data; }
	uint32*	GetData() { return Data; }
//	void	ProcessForColorKeyZero();
private:
	uint32*	Data;
};



namespace Geometry {
	vec3	Rotate(float Angle, const vec3& Axis, const vec3& Point);

	matrix33	RotateTensor(const matrix33& R, const matrix33& I);
	
	// Bitmap processing.
	void	PreMultiplyAlpha(bitmap32* b);
	
	// For making mip-maps of power-of-two textures.
	void	HalfScaleFilterBox(int sw, int sh, uint32* src, uint32* dest);
	void	HalfScaleFilterSinc(int sw, int sh, uint32* src, uint32* dest);
	void	HalfScaleFilterSincNice(int sw, int sh, uint32* src, uint32* dest);
	void	HalfScaleFilterSincScaleAlpha(int sw, int sh, uint32* src, uint32* dest, float AlphaFactor);

	// Some crude software rendering functions.
	void	FillCircle(bitmap32* dest, float x, float y, float radius, uint32 color);
	void	FillTriangle(bitmap32* dest, float x0, float y0, float x1, float y1, float x2, float y2, uint32 color);
	// void	FillQuad();
	// void FillCapsule();
};



#endif // GEOMETRY_HPP
