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
// geometry.cpp	-thatcher 1/28/1998 Copyright Thatcher Ulrich

// Implementations for the basic geometric types.


#include <math.h>

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif


#include <float.h>
#include "geometry.hpp"
#include "utility.hpp"


/**
 * Adds two vec3s.  Creates a temporary for the return value.
 */
vec3
vec3::operator+(const vec3& v) const
{
	vec3	result;
	result.x[0] = x[0] + v.x[0];
	result.x[1] = x[1] + v.x[1];
	result.x[2] = x[2] + v.x[2];
	return result;
}


/**
 * Subtracts two vec3s.  Creates a temporary for the return value.
 */
vec3
vec3::operator-(const vec3& v) const
{
	vec3	result;
	result.x[0] = x[0] - v.x[0];
	result.x[1] = x[1] - v.x[1];
	result.x[2] = x[2] - v.x[2];
	return result;
}


/**
 * Returns the negative of *this.  Creates a temporary for the return value.
 */
vec3
vec3::operator-() const
{
	vec3	result;
	result.x[0] = -x[0];
	result.x[1] = -x[1];
	result.x[2] = -x[2];
	return result;
}


#ifndef INLINE_VECTOR


/**
 * Dot product.
 */
float
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
vec3&
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
vec3&
vec3::operator-=(const vec3& v)
{
	x[0] -= v.x[0];
	x[1] -= v.x[1];
	x[2] -= v.x[2];
	return *this;
}


#endif // INLINE_VECTOR


/**
 * Scale a vec3 by a scalar.  Creates a temporary for the return value.
 */
vec3
vec3::operator*(float f) const
{
	vec3	result;
	result.x[0] = x[0] * f;
	result.x[1] = x[1] * f;
	result.x[2] = x[2] * f;
	return result;
}


/**
 * Cross product.  Creates a temporary for the return value.
 */
vec3
vec3::cross(const vec3& v) const
{
	vec3	result;
	result.x[0] = x[1] * v.x[2] - x[2] * v.x[1];
	result.x[1] = x[2] * v.x[0] - x[0] * v.x[2];
	result.x[2] = x[0] * v.x[1] - x[1] * v.x[0];
	return result;
}


/**
 * Scales the vec3 to unit length.  Preserves its direction.
 */
vec3&
vec3::normalize()
{
	float	f = magnitude();
	if (f < 0.0000001) {
		// Punt.
		x[0] = 1;
		x[1] = 0;
		x[2] = 0;
	} else {
		this->operator/=(f);
	}
	return *this;
}


/**
 * Scales *this by the given scalar.
 */
vec3&
vec3::operator*=(float f)
{
	x[0] *= f;
	x[1] *= f;
	x[2] *= f;
	return *this;
}


/**
 * Equality operator.
 */
bool
vec3::operator==(const vec3& rhs)
{
	return ( (*this).X() == rhs.X() &&
		 (*this).Y() == rhs.Y() &&
		 (*this).Z() == rhs.Z() );
}


/**
 * Inequality operator.
 */
bool
vec3::operator!=(const vec3& rhs)
{
	return !((*this) == rhs);
}


/**
 * Returns the length of *this.
 */
float
vec3::magnitude() const
{
	return sqrtf(sqrmag());
}


/**
 * Returns the square of the length of *this.
 */
float
vec3::sqrmag() const
{
	return x[0]*x[0] + x[1]*x[1] + x[2]*x[2];
}


#ifdef WIN32
#define isnan _isnan
#endif // WIN32


/**
 * Returns true if any component is nan.
 */
bool
vec3::checknan() const
{
	// @@ why do this check???
	if (fabsf(x[0]) > 10000000 || fabsf(x[1]) > 10000000 || fabsf(x[2]) > 10000000) {
		return true;//xxxxxxx
	}

	if (isnan(x[0]) || isnan(x[1]) || isnan(x[2])) {
		return true;
	}
	else return false;
}


vec3	ZeroVector(0, 0, 0);
vec3	XAxis(1, 0, 0);
vec3	YAxis(0, 1, 0);
vec3	ZAxis(0, 0, 1);


// class matrix33


/**
 * Sets this matrix to the identity matrix.
 */
void
matrix33::Identity()
{
	SetColumns(XAxis, YAxis, ZAxis);
}


/**
 * Sets the matrix columns (zero based indexing).
 *
 * @param col0 Column 0
 * @param col1 Column 1
 * @param col2 Column 2
 */
void
matrix33::SetColumns(const vec3& col0, const vec3& col1, const vec3& col2)
{
	SetColumn(0, col0);
	SetColumn(1, col1);
	SetColumn(2, col2);
}

/**
 * Constructs an orientation matrix which maps from object space to world space.
 *
 * @param dir World space direction.
 * @param up Up vectors of the object.
 */
void
matrix33::Orient(const vec3& dir, const vec3& up)
{
	vec3	z = dir.cross(up);
	SetColumns(dir, up, z);
}


/**
 * Multiplies the given vector by this matrix, and returns the resulting vector.
 */
vec3
matrix33::operator*(const vec3& v) const
{
	vec3	t;

	t.SetX(v.X() * m[0][0] + v.Y() * m[1][0] + v.Z() * m[2][0]);
	t.SetY(v.X() * m[0][1] + v.Y() * m[1][1] + v.Z() * m[2][1]);
	t.SetZ(v.X() * m[0][2] + v.Y() * m[1][2] + v.Z() * m[2][2]);

	return t;
}


/**
 * Multiplies this matrix by the given scalar.
 */
matrix33&
matrix33::operator*=(float f)
{
	int	i;
	for (i = 0; i < 3; i++) m[i] *= f;

	return *this;
}


/**
 * Matrix memberwise accumulate.
 */
matrix33&
matrix33::operator+=(const matrix33& mat)
{
	int	i;
	for (i = 0; i < 3; i++) {
		m[i] += mat.m[i];
	}

	return *this;
}


/**
 * Matrix memberwise subtraction.
 */
matrix33&
matrix33::operator-=(const matrix33& mat)
{
	int	i;
	for (i = 0; i < 3; i++) {
		m[i] -= mat.m[i];
	}

	return *this;
}


/**
 * Matrix multiplication assignment.
 */
matrix33&
matrix33::operator*=(const matrix33& mat)
{
	matrix33	t;
	int	i, j, k;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < 3; i++) {
			float	f = 0;
			for (k = 0; k < 3; k++) {
				f += m[k].Get(j) * mat.m[i].Get(k);
			}
			t.m[i].Set(j, f);
		}
	}

	*this = t;
	return *this;
}


/**
 * Matrix multiply.
 */
matrix33
matrix33::operator*(const matrix33& mat) const
{
	matrix33	t;
	int	i, j, k;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < 3; i++) {
			float	f = 0;
			for (k = 0; k < 3; k++) {
				f += m[k].Get(j) * mat.m[i].Get(k);
			}
			t.m[i].Set(j, f);
		}
	}

	return t;
}


/**
 * Equality operator.
 */
bool
matrix33::operator==(const matrix33& rhs)
{
	return ( (*this)[0] == rhs[0] &&
		 (*this)[1] == rhs[1] &&
		 (*this)[2] == rhs[2] );
}


/**
 * Inequality operator.
 */
bool
matrix33::operator!=(const matrix33& rhs)
{
	return !((*this) == rhs);
}

/**
 * Multiplies *this by the transpose of the given matrix.
 */
matrix33&
matrix33::MultTranspose(const matrix33& mat)
{
	matrix33	t;
	int	i, j, k;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < 3; i++) {
			float	f = 0;
			for (k = 0; k < 3; k++) {
				f += m[k].Get(j) * mat.m[k].Get(i);
			}
			t.m[i].Set(j, f);
		}
	}

	*this = t;
	return *this;
}


/**
 * Inverts *this matrix.
 *
 * For a singular matrix, set the the identity.
 * A square matrix is singular if and only if its determinant is zero.
 */
void
matrix33::Invert()
{
	matrix33 a(*this);
	matrix33 b;
	
	int c, r;
	int cc;
	int rowMax; // Points to max abs value row in this column
	int row;
	float tmp;
	
	// Go through columns
	for (c=0; c<3; c++) 
	{
		// Find the row with max value in this column
		rowMax = c;
		for (r=c+1; r<3; r++) 
		{
			if (fabsf(a[c][r]) > fabsf(a[c][rowMax]))
			{
				rowMax = r;
			}
		}
		
		// If the max value here is 0, we can't invert.  Return identity.
		if (a[rowMax][c] == 0.0F) {
			Identity();
			return;
		}
		
		// Swap row "rowMax" with row "c"
		for (cc=0; cc<3; cc++) 
		{
			tmp = a[cc][c];
			a[cc][c] = a[cc][rowMax];
			a[cc][rowMax] = tmp;
			tmp = b[cc][c];
			b[cc][c] = b[cc][rowMax];
			b[cc][rowMax] = tmp;
		}
		
		// Now everything we do is on row "c".
		// Set the max cell to 1 by dividing the entire row by that value
		tmp = a[c][c];
		for (cc=0; cc<3; cc++) 
		{
			a[cc][c] /= tmp;
			b[cc][c] /= tmp;
		}
		
		// Now do the other rows, so that this column only has a 1 and 0's
		for (row = 0; row < 3; row++) 
		{
			if (row != c) 
			{
				tmp = a[c][row];
				for (cc=0; cc<3; cc++) 
				{
					a[cc][row] -= a[cc][c] * tmp;
					b[cc][row] -= b[cc][c] * tmp;
				}
			}
		}
		
	}
	
	*this = b;
}


// class matrix


void	matrix::Identity()
// Sets *this to be an identity matrix.
{
	SetColumn(0, XAxis);
	SetColumn(1, YAxis);
	SetColumn(2, ZAxis);
	SetColumn(3, ZeroVector);
}


void	matrix::View(const vec3& ViewNormal, const vec3& ViewUp, const vec3& ViewLocation)
// Turns *this into a view matrix, given the direction the camera is
// looking (ViewNormal) and the camera's up vec3 (ViewUp), and its
// location (ViewLocation) (all vec3s in world-coordinates).  The
// resulting matrix will transform points from world coordinates to view
// coordinates, which is a right-handed system with the x axis pointing
// left, the y axis pointing up, and the z axis pointing into the scene.
{
	vec3	ViewX = ViewUp.cross(ViewNormal);

	// Construct the view-to-world matrix.
	Orient(ViewX, ViewUp, ViewLocation);

	// Turn it around, to make it world-to-view.
	Invert();
}


void	matrix::Orient(const vec3& Direction, const vec3& Up, const vec3& Location)
// Turns *this into a transformation matrix, that transforms vec3s
// from object coordinates to world coordinates, given the object's Direction, Up,
// and Location in world coordinates.
{
	vec3	ZAxis = Direction.cross(Up);

	SetColumn(0, Direction);
	SetColumn(1, Up);
	SetColumn(2, ZAxis);
	SetColumn(3, Location);
}


vec3	matrix::operator*(const vec3& v) const
// Applies *this to the given vec3, and returns the transformed vec3.
{
	vec3	result;
	Apply(&result, v);
	return result;
}


matrix	matrix::operator*(const matrix& a) const
// Composes the two matrices, returns the product.  Creates a temporary
// for the return value.
{
	matrix result;

	Compose(&result, *this, a);

	return result;
}


void	matrix::Compose(matrix* dest, const matrix& left, const matrix& right)
// Multiplies left * right, and puts the result in *dest.
{
	left.ApplyRotation(&const_cast<vec3&>(dest->GetColumn(0)), right.GetColumn(0));
	left.ApplyRotation(&const_cast<vec3&>(dest->GetColumn(1)), right.GetColumn(1));
	left.ApplyRotation(&const_cast<vec3&>(dest->GetColumn(2)), right.GetColumn(2));
	left.Apply(&const_cast<vec3&>(dest->GetColumn(3)), right.GetColumn(3));
}


matrix&	matrix::operator*=(float f)
// Scalar multiply of a matrix.
{
	int	i;
	for (i = 0; i < 4; i++) m[i] *= f;
	return *this;
}


matrix&	matrix::operator+=(const matrix& mat)
// Memberwise matrix addition.
{
	int	i;
	for (i = 0; i < 4; i++) m[i] += mat.m[i];
	return *this;
}


void	matrix::Invert()
// Inverts *this.  Uses transpose property of rotation matrices.
{
	InvertRotation();

	// Compute the translation part of the inverted matrix, by applying
	// the inverse rotation to the original translation.
	vec3	TransPrime;
	ApplyRotation(&TransPrime, GetColumn(3));

	SetColumn(3, -TransPrime);	// Could optimize the negation by doing it in-place.
}


void	matrix::InvertRotation()
// Inverts the rotation part of *this.  Ignores the translation.
// Uses the transpose property of rotation matrices.
{
	float	f;

	// Swap elements across the diagonal.
	f = m[1].Get(0);
	m[1].Set(0, m[0].Get(1));
	m[0].Set(1, f);

	f = m[2].Get(0);
	m[2].Set(0, m[0].Get(2));
	m[0].Set(2, f);

	f = m[2].Get(1);
	m[2].Set(1, m[1].Get(2));
	m[1].Set(2, f);
}


void	matrix::NormalizeRotation()
// Normalizes the rotation part of the matrix.
{
	m[0].normalize();
	m[1] = m[2].cross(m[0]);
	m[1].normalize();
	m[2] = m[0].cross(m[1]);
}


void	matrix::Apply(vec3* result, const vec3& v) const
// Applies v to *this, and puts the transformed result in *result.
{
	// Do the rotation.
	ApplyRotation(result, v);
	// Do the translation.
	*result += m[3];
}


void	matrix::ApplyRotation(vec3* result, const vec3& v) const
// Applies the rotation portion of *this, and puts the transformed result in *result.
{
	result->Set(0, m[0].Get(0) * v.Get(0) + m[1].Get(0) * v.Get(1) + m[2].Get(0) * v.Get(2));
	result->Set(1, m[0].Get(1) * v.Get(0) + m[1].Get(1) * v.Get(1) + m[2].Get(1) * v.Get(2));
	result->Set(2, m[0].Get(2) * v.Get(0) + m[1].Get(2) * v.Get(1) + m[2].Get(2) * v.Get(2));
}


void	matrix::ApplyInverse(vec3* result, const vec3& v) const
// Applies v to the inverse of *this, and puts the transformed result in *result.
{
	ApplyInverseRotation(result, v - m[3]);
}


void	matrix::ApplyInverseRotation(vec3* result, const vec3& v) const
// Applies v to the inverse rotation part of *this, and puts the result in *result.
{
	result->Set(0, m[0] * v);
	result->Set(1, m[1] * v);
	result->Set(2, m[2] * v);
}


void	matrix::Translate(const vec3& v)
// Composes a translation on the right of *this.
{
	vec3	newtrans;
	Apply(&newtrans, v);
	SetColumn(3, newtrans);
}


void	matrix::SetOrientation(const quaternion& q)
// Sets the rotation part of the matrix to the values which correspond to the given
// quaternion orientation.
{
	float	S = q.GetS();
	const vec3&	V = q.GetV();
	
	m[0].Set(0, 1 - 2 * V.Y() * V.Y() - 2 * V.Z() * V.Z());
	m[0].Set(1, 2 * V.X() * V.Y() + 2 * S * V.Z());
	m[0].Set(2, 2 * V.X() * V.Z() - 2 * S * V.Y());

	m[1].Set(0, 2 * V.X() * V.Y() - 2 * S * V.Z());
	m[1].Set(1, 1 - 2 * V.X() * V.X() - 2 * V.Z() * V.Z());
	m[1].Set(2, 2 * V.Y() * V.Z() + 2 * S * V.X());

	m[2].Set(0, 2 * V.X() * V.Z() + 2 * S * V.Y());
	m[2].Set(1, 2 * V.Y() * V.Z() - 2 * S * V.X());
	m[2].Set(2, 1 - 2 * V.X() * V.X() - 2 * V.Y() * V.Y());
}


quaternion	matrix::GetOrientation() const
// Converts the rotation part of *this into a quaternion, and returns it.
{
	// Code adapted from Baraff, "Rigid Body Simulation", from SIGGRAPH 95 course notes for Physically Based Modeling.
	quaternion	q;
	float	tr, s;

	tr = m[0].Get(0) + m[1].Get(1) + m[2].Get(2);	// trace

	if (tr >= 0) {
		s = sqrtf(tr + 1);
		q.SetS(0.5f * s);
		s = 0.5f / s;
		q.SetV(vec3(m[1].Get(2) - m[2].Get(1), m[2].Get(0) - m[0].Get(2), m[0].Get(1) - m[1].Get(0)) * s);
	} else {
		int	i = 0;

		if (m[1].Get(1) > m[0].Get(0)) {
			i = 1;
		}
		if (m[2].Get(2) > m[i].Get(i)) {
			i = 2;
		}

		float	qr, qi, qj, qk;
		switch (i) {
		case 0:
			s = sqrtf((m[0].Get(0) - (m[1].Get(1) + m[2].Get(2))) + 1);
			qi = 0.5f * s;
			s = 0.5f / s;
			qj = (m[1].Get(0) + m[0].Get(1)) * s;
			qk = (m[0].Get(2) + m[2].Get(0)) * s;
			qr = (m[1].Get(2) - m[2].Get(1)) * s;
			break;

		case 1:
			s = sqrtf((m[1].Get(1) - (m[2].Get(2) + m[0].Get(0))) + 1);
			qj = 0.5f * s;
			s = 0.5f / s;
			qk = (m[2].Get(1) + m[1].Get(2)) * s;
			qi = (m[1].Get(0) + m[0].Get(1)) * s;
			qr = (m[2].Get(0) - m[0].Get(2)) * s;
			break;

		case 2:
			s = sqrtf((m[2].Get(2) - (m[0].Get(0) + m[1].Get(1))) + 1);
			qk = 0.5f * s;
			s = 0.5f / s;
			qi = (m[0].Get(2) + m[2].Get(0)) * s;
			qj = (m[2].Get(1) + m[1].Get(2)) * s;
			qr = (m[0].Get(1) - m[1].Get(0)) * s;
			break;
		}
		q.SetS(qr);
		q.SetV(vec3(qi, qj, qk));
	}

	return q;
}


//
// class quaternion
//


quaternion::quaternion(const vec3& Axis, float Angle)
// Constructs the quaternion defined by the rotation about the given axis of the given angle (in radians).
{
	S = cosf(Angle / 2);
	V = Axis;
	V *= sinf(Angle / 2);
}


quaternion	quaternion::operator*(const quaternion& q) const
// Multiplication of two quaternions.  Returns a new quaternion
// result.
{
	return quaternion(S * q.S - V * q.V, q.V * S + V * q.S + V.cross(q.V));
}


quaternion&	quaternion::normalize()
// Ensures that the quaternion has magnitude 1.
{
	float	mag = sqrtf(S * S + V * V);
	if (mag > 0.0000001f) {
		float	inv = 1.0f / mag;
		S *= inv;
		V *= inv;
	} else {
		// Quaternion is messed up.  Turn it into a null rotation.
		S = 1;
		V = ZeroVector;
	}

	return *this;
}


quaternion&	quaternion::operator*=(const quaternion& q)
// In-place quaternion multiplication.
{
	*this = *this * q;	// I don't think we can avoid making temporaries.

	return *this;
}


void	quaternion::ApplyRotation(vec3* result, const vec3& v)
// Rotates the given vec3 v by the rotation represented by this quaternion.
// Stores the result in the given result vec3.
{
	quaternion	q(*this * quaternion(0, v) * quaternion(S, -V));	// There's definitely a shortcut here.  Deal with later.

	*result = q.V;
}


quaternion	quaternion::lerp(const quaternion& q, float f) const
// Does a spherical linear interpolation between *this and q, using f as
// the blend factor.  f == 0 --> result == *this, f == 1 --> result == q.
{
	quaternion	result;

	float	f0, f1;

	float	cos_omega = V * q.V + S * q.S;
	quaternion	qtemp(q);

	// Adjust signs if necessary.
	if (cos_omega < 0) {
		cos_omega = -cos_omega;
		qtemp.V = -qtemp.V;
		qtemp.S = -qtemp.S;
	}

	if (cos_omega < 0.99) {
		// Do the spherical interp.
		float	omega = acosf(cos_omega);
		float	sin_omega = sinf(omega);
		f0 = sinf((1 - f) * omega) / sin_omega;
		f1 = sinf(f * omega) / sin_omega;
	} else {
		// Quaternions are close; just do straight lerp and avoid division by near-zero.
		f0 = 1 - f;
		f1 = f;
	}
	
	result.S = S * f0 + qtemp.S * f1;
	result.V = V * f0 + qtemp.V * f1;
	result.normalize();

	return result;
}



#ifdef NOT
QuatSlerp(QUAT * from, QUAT * to, float t, QUAT * res)
      {
        float           to1[4];
        double        omega, cosom, sinom, scale0, scale1;

        // calc cosine
        cosom = from->x * to->x + from->y * to->y + from->z * to->z
			       + from->w * to->w;

        // adjust signs (if necessary)
        if ( cosom <0.0 ){ cosom = -cosom; to1[0] = - to->x;
		to1[1] = - to->y;
		to1[2] = - to->z;
		to1[3] = - to->w;
        } else  {
		to1[0] = to->x;
		to1[1] = to->y;
		to1[2] = to->z;
		to1[3] = to->w;
        }

        // calculate coefficients

       if ( (1.0 - cosom) > DELTA ) {
                // standard case (slerp)
                omega = acosf(cosom);
                sinom = sinf(omega);
                scale0 = sinf((1.0 - t) * omega) / sinom;
                scale1 = sinf(t * omega) / sinom;

        } else {        
    // "from" and "to" quaternions are very close 
	    //  ... so we can do a linear interpolation
                scale0 = 1.0 - t;
                scale1 = t;
        }
	// calculate final values
	res->x = scale0 * from->x + scale1 * to1[0];
	res->y = scale0 * from->y + scale1 * to1[1];
	res->z = scale0 * from->z + scale1 * to1[2];
	res->w = scale0 * from->w + scale1 * to1[3];
}
 
 
#endif // NOT


#ifdef NOT
void	bitmap32::ProcessForColorKeyZero()
// Examine alpha channel, and set pixels that have alpha==0 to color 0.
// Pixels that have alpha > 0.5, but have color 0, should be tweaked slightly
// so they don't get knocked out when blitting.
{
	uint32	Key32 = 0;
	uint32*	p = GetData();
	int	pixels = GetHeight() * GetWidth();
	for (int i = 0; i < pixels; i++, p++) {
		if ((*p >> 24) >= 128) {
			// Alpha >= 0.5.  Make sure color isn't equal to color key.
			if ((*p & 0x00FFFFFF) == Key32) {
				*p ^= 8;	// Twiddle a low blue bit.
			}
		} else {
			// Set pixel's color equal to color key.
			*p = (*p & 0xFF000000) | Key32;
		}
	}		
}
#endif // NOT


namespace Geometry {
;


vec3	Rotate(float Angle, const vec3& Axis, const vec3& Point)
// Rotates the given point through the given angle (in radians) about the given
// axis.
{
	quaternion	q(cosf(Angle/2), Axis * sinf(Angle/2));

	vec3	result;
	q.ApplyRotation(&result, Point);

	return result;
}


matrix33	RotateTensor(const matrix33& R, const matrix33& I)
// Returns R * I * R^-1.
{
	matrix33	m = R * I;
	m.MultTranspose(R);
	return m;
}


void	PreMultiplyAlpha(bitmap32* b)
// Multiplies the color components of b by the corresponding alpha component.
{
	uint32*	p = b->GetData();
	int	count = b->GetWidth() * b->GetHeight();

	while (count--) {
		uint32	c = *p;
		float	a = (c >> 24) / 255.0f;
		int	r = (int) (((c >> 16) & 0x0FF) * a);
		int	g = (int) (((c >> 8) & 0x0FF) * a);
		int	b = (int) (((c) & 0x0FF) * a);
		*p = (c & 0xFF000000) | (r << 16) | (g << 8) | (b);
		p++;
	}
}


void	HalfScaleFilterBox(int sw, int sh, uint32* src, uint32* dest)
// Given the source data and dimensions, computes an anti-aliased MIP-map
// of dimensions (sw >> 1, sh >> 1) into the destination array.
// Uses a simple 2x2 box filter.
{
	int	dw = sw >> 1;
	int	dh = sh >> 1;

	bool	filterw = true;
	bool	filterh = true;
	if (dw == 0) {
		dw = 1;
		filterw = false;
	}
	if (dh == 0) {
		dh = 1;
		filterh = false;
	}
	
	uint32*	p = src;
	uint32*	q = dest;
	int	row, col;
	for (row = 0; row < dh; row++) {
		for (col = 0; col < dw; col++) {
			// 2x2 box filter.
			uint32	s0, s1, s2, s3;
			s0 = *p;
			if (filterw) s1 = *(p+1);
			else s1 = s0;
			if (filterh) {
				s2 = *(p+sw);
				if (filterw) s3 = *(p+sw+1);
				else s3 = s2;
			} else {
				s2 = s0;
				s3 = s1;
			}
			uint32	r = ((s0 & 0x0FF) + (s1 & 0x0FF) + (s2 & 0x0FF) + (s3 & 0x0FF) + 2) >> 2;
			uint32	g = (((s0 >> 8) & 0x0FF) + ((s1 >> 8) & 0x0FF) + ((s2 >> 8) & 0x0FF) + ((s3 >> 8) & 0x0FF) + 2) >> 2;
			uint32	b = (((s0 >> 16) & 0x0FF) + ((s1 >> 16) & 0x0FF) + ((s2 >> 16) & 0x0FF) + ((s3 >> 16) & 0x0FF) + 2) >> 2;
			uint32	a = ((s0 >> 24) + (s1 >> 24) + (s2 >> 24) + (s3 >> 24) + 2) >> 2;
			
			*q++ = (r) | (g << 8) | (b << 16) | (a << 24);
			p += 2;
		}
		p += sw;
	}
}

	
void	HalfScaleFilterSinc(int sw, int sh, uint32* src, uint32* dest)
// Given the source data and dimensions, computes an anti-aliased
// MIP-map of dimensions (sw >> 1, sh >> 1) into the destination array.
// Uses a truncated sinc-function filter, if possible; otherwise a
// simple box filter.
{
	int	dw = sw >> 1;
	int	dh = sh >> 1;

	if (dw == 0 || dh == 0) {
		// One or the other dimension doesn't scale.  Use simpler 2x2 box filter.
		HalfScaleFilterBox(sw, sh, src, dest);
		return;
	}

	// Use a 4x4 convolution filter, based on sinc function.
	
	// Truncated sinf(x)/x * sinf(y)/y, sampled at -3pi/2, -pi/2, pi/2, 3pi/2
//	static const float	Kernel[4][4] = {
//		{ 0.045032, -0.135095, -0.135095,  0.045032 },
//		{ -0.135095, 0.405285,  0.405285, -0.135095 },
//		{ -0.135095, 0.405285,  0.405285, -0.135095 },
//		{ 0.045032, -0.135095, -0.135095,  0.045032 },
//	};

//	static const float	Kernel[4][4] = {
//		{ -0.5,  -1,  -1,  0.5 },
//		{ -1  ,  10,  10, -1 },
//		{ -1  ,  10,  10, -1 },
//		{ -0.5,  -1,  -1, -0.05 },
//	};

	static const float	Kernel[4][4] = {
		{  1  ,  -2,  -2,  1  },
		{ -2  ,  20,  20, -2 },
		{ -2  ,  20,  20, -2 },
		{  1  ,  -2,  -2,  1  },
	};

	uint32*	sp = src;
	int	row, col;
	for (row = 0; row < dh; row++, sp += sw * 2) {
		for (col = 0; col < dw; col++) {

			float	r = 0, g = 0, b = 0, a = 0, weight = 0;
			int	sourcerow = row * 2 - 1;
			
			int	i, j;
			for (j = 0; j < 4; j++, sourcerow++) {
				if (sourcerow < 0) continue;
				if (sourcerow >= sh) break;

				int	sourcecol = col * 2 - 1;
					
				for (i = 0; i < 4; i++, sourcecol++) {
					if (sourcecol < 0) continue;
					if (sourcecol >= sw) break;
					
					uint32	s = src[sourcerow * sw + sourcecol];
					float	w = Kernel[j][i];

					weight += w;
					r += ((s) & 0x0FF) * w;
					g += ((s >> 8) & 0x0FF) * w;
					b += ((s >> 16) & 0x0FF) * w;
					a += ((s >> 24) & 0x0FF) * w;
				}
			}

			float	rw = 1.0f / weight;
			r *= rw;
			g *= rw;
			b *= rw;
			a *= rw;
			dest[row * dw + col] =
				iclamp(0, (int) (r + 0.25f), 255)
				+ (iclamp(0, (int) (g + 0.25f), 255) << 8)
				+ (iclamp(0, (int) (b + 0.25f), 255) << 16)
				+ (iclamp(0, (int) (a + 0.25f), 255) << 24);
		}
	}
}

	
void	HalfScaleFilterSincNice(int sw, int sh, uint32* src, uint32* dest)
// Given the source data and dimensions, computes an anti-aliased
// MIP-map of dimensions (sw >> 1, sh >> 1) into the destination array.
// Uses a truncated sinc-function filter, if possible; otherwise a
// simple box filter.
// Assumes tiled texture.
{
	int	dw = sw >> 1;
	int	dh = sh >> 1;

	if (dw == 0 || dh == 0) {
		// One or the other dimension doesn't scale.  Use simpler 2x2 box filter.
		HalfScaleFilterBox(sw, sh, src, dest);
		return;
	}

	// Use a 4x4 convolution filter, based on sinc function.
	
	// Truncated sinf(x)/x * sinf(y)/y, sampled at -3pi/2, -pi/2, pi/2, 3pi/2 ...
	static const int	DIM = 8;
	static float	Kernel[DIM][DIM];
	static bool	KernelInited = false;
	if (KernelInited == false) {
		int	i, j;
		for (j = 0; j < DIM; j++) {
			for (i = 0; i < DIM; i++) {
				float	x = 0.5f + fabsf(float(i - DIM/2));
				float	y = 0.5f + fabsf(float(j - DIM/2));
				
				float	f = sinf(x * PI/2) * sinf(y * PI/2) / (x * y);
				Kernel[j][i] = f;
			}
		}
		KernelInited = true;
	}

	int	sourcecolmask = sw - 1;
	int	sourcerowmask = sh - 1;
	
	int	row, col;
	for (row = 0; row < dh; row++) {
		for (col = 0; col < dw; col++) {

			float	r = 0, g = 0, b = 0, a = 0, weight = 0;

			int	sourcerow = row * 2 - (DIM/2 - 1);
			
			int	i, j;
			for (j = 0; j < DIM; j++, sourcerow++) {

				int	sourcecol = col * 2 - (DIM/2 - 1);
					
				for (i = 0; i < DIM; i++, sourcecol++) {
					
					uint32	s = src[(sourcerow & sourcerowmask) * sw + (sourcecol & sourcecolmask)];
					float	w = Kernel[j][i];

					weight += w;
					r += ((s) & 0x0FF) * w;
					g += ((s >> 8) & 0x0FF) * w;
					b += ((s >> 16) & 0x0FF) * w;
					a += ((s >> 24) & 0x0FF) * w;
				}
			}

			float	rw = 1.0f / weight;
			r *= rw;
			g *= rw;
			b *= rw;
			a *= rw;
			dest[row * dw + col] =
				iclamp(0, (int) (r + 0.5f), 255)
				+ (iclamp(0, (int) (g + 0.5f), 255) << 8)
				+ (iclamp(0, (int) (b + 0.5f), 255) << 16)
				+ (iclamp(0, (int) (a + 0.5f), 255) << 24);
		}
	}
}

	
void	HalfScaleFilterSincScaleAlpha(int sw, int sh, uint32* src, uint32* dest, float AlphaFactor)
// Given the source data and dimensions, computes an anti-aliased
// MIP-map of dimensions (sw >> 1, sh >> 1) into the destination array.
// Uses a truncated sinc-function filter, if possible; otherwise a
// simple box filter.
// Scales alpha channel by given AlphaFactor.
{
	int	dw = sw >> 1;
	int	dh = sh >> 1;

	if (dw == 0 || dh == 0) {
		// One or the other dimension doesn't scale.  Use simpler 2x2 box filter.
		HalfScaleFilterBox(sw, sh, src, dest);
		return;
	}

	// Use a 4x4 convolution filter, based on sinc function.
	
	// Truncated sinf(x)/x * sinf(y)/y, sampled at -3pi/2, -pi/2, pi/2, 3pi/2
//	static const float	Kernel[4][4] = {
//		{ 0.045032, -0.135095, -0.135095,  0.045032 },
//		{ -0.135095, 0.405285,  0.405285, -0.135095 },
//		{ -0.135095, 0.405285,  0.405285, -0.135095 },
//		{ 0.045032, -0.135095, -0.135095,  0.045032 },
//	};
	static const float	Kernel[4][4] = {
		{  1  ,  -2,  -2,  1  },
		{ -2  ,  20,  20, -2 },
		{ -2  ,  20,  20, -2 },
		{  1  ,  -2,  -2,  1  },
	};

	uint32*	sp = src;
	int	row, col;
	for (row = 0; row < dh; row++, sp += sw * 2) {
		for (col = 0; col < dw; col++) {

			float	r = 0, g = 0, b = 0, a = 0, weight = 0;

			int	sourcerow = row * 2 - 1;
			
			int	i, j;
			for (j = 0; j < 4; j++, sourcerow++) {
				if (sourcerow < 0) continue;
				if (sourcerow >= sh) break;

				int	sourcecol = col * 2 - 1;
					
				for (i = 0; i < 4; i++, sourcecol++) {
					if (sourcecol < 0) continue;
					if (sourcecol >= sw) break;
					
					uint32	s = src[sourcerow * sw + sourcecol];
					float	w = Kernel[j][i];

					weight += w;
					r += ((s) & 0x0FF) * w;
					g += ((s >> 8) & 0x0FF) * w;
					b += ((s >> 16) & 0x0FF) * w;
					a += ((s >> 24) & 0x0FF) * w;
				}
			}

			float	rw = 1.0f / weight;
			r *= rw * AlphaFactor;
			g *= rw * AlphaFactor;
			b *= rw * AlphaFactor;
			a *= rw * AlphaFactor;
			dest[row * dw + col] =
				iclamp(0, (int) r, 255)
				+ (iclamp(0, (int) g, 255) << 8)
				+ (iclamp(0, (int) b, 255) << 16)
				+ (iclamp(0, (int) a, 255) << 24);
		}
	}
}


void	FillCircle(bitmap32* dest, float xcenter, float ycenter, float radius, uint32 color)
// Uses the given color to fill the circle within the destination
// bitmap centered at xcenter,ycenter with the given radius.  All
// coordinates are in texels.
{
	int	iy = frnd(ycenter - radius);
	int	bottomy = frnd(ycenter + radius);

	for ( ; iy <= bottomy; iy++) {
		float	h = ycenter - iy;
		float	halfwidth = sqrtf(fmax(0, radius * radius - h * h));

		int	x = frnd(xcenter - halfwidth);
		int	x1 = frnd(xcenter + halfwidth);
		uint32*	p = dest->GetData() + (iy * dest->GetWidth() + x);
		for ( ; x <= x1; x++) {
			*p++ = color;
		}
	}
}


void	FillTrap(bitmap32* dest, float xl0, float xr0, float y0, float xl1, float xr1, float y1, uint32 color)
// Fills the specified trapezoid with the given color.
{
	float	t;
	
	// Make sure the trapezoid isn't backwards.
	if (xl0 > xr0 || xl1 > xr1) {
		// Flip.
		t = xl0; xl0 = xr0; xr0 = t;
		t = xl1; xl1 = xr1; xr1 = t;
	}

	int	iy = frnd(y0);
	int	ycount = frnd(y1) - iy;

	float	dxl, dxr;
	if (ycount > 0) {
		dxl = (xl1 - xl0) / ycount;
		dxr = (xr1 - xr0) / ycount;
	} else {
		dxl = 0;
		dxr = 0;
	}
	
	// Scan down.

	while (ycount) {
		uint32*	p = dest->GetData() + (iy * dest->GetWidth() + frnd(xl0));

		int	count = frnd(xr0) - frnd(xl0);
		while (count) {
			*p++ = color;
			count--;
		}

		xl0 += dxl;
		xr0 += dxr;
		
		ycount--;
		iy++;
	}
}


void	FillTriangle(bitmap32* dest, float x0, float y0, float x1, float y1, float x2, float y2, uint32 color)
// Uses the given color to fill the specified triangle within the
// destination bitmap.  All coordinates are in texels.
{
	float	t;
	// sort vertices by y
	if (y1 < y0) {
		t = y0; y0 = y1; y1 = t;
		t = x0; x0 = x1; x1 = t;
	}
	if (y2 < y1) {
		t = y1; y1 = y2; y2 = t;
		t = x1; x1 = x2; x2 = t;
	}
	if (y1 < y0) {
		t = y0; y0 = y1; y1 = t;
		t = x0; x0 = x1; x1 = t;
	}
	
	// top trapezoid
	float	f = 0;
	if (y2 - y0 > 1) f = (y1 - y0) / (y2 - y0);
	float	xmid = (x2 - x0) * f + x0;
	FillTrap(dest, x0, x0, y0, x1, xmid, y1, color);

	// bottom trapezoid
	FillTrap(dest, x1, xmid, y1, x2, x2, y2, color);
}


};

