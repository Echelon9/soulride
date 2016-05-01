/*
    Copyright 2016 Rhys Kidd

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

#include "gtest/gtest.h"

#include "geometry.hpp"


TEST(GeometryTest, PointCreationNull)
{
    point p = point();
    EXPECT_EQ(0, p.GetX());
    EXPECT_EQ(0, p.GetY());
}

TEST(GeometryTest, PointCreation)
{
    point p = point(1, 2);
    EXPECT_EQ(1, p.GetX());
    EXPECT_EQ(2, p.GetY());
}

TEST(GeometryTest, PointCopyConstructor)
{
    point p = point(10, 10);
    // Call copy constructor via direct initialization
    point q(p);
    
    EXPECT_EQ(10, q.GetX());
    EXPECT_EQ(10, q.GetY());
}

TEST(GeometryTest, PointAssignmentOperator)
{
    point p = point(10, 10);
    point q = point();
    
    q = p;
    
    EXPECT_EQ(10, q.GetX());
    EXPECT_EQ(10, q.GetY());
}

TEST(GeometryTest, PointModification)
{
    point p = point();
    
    p.SetX(4);
    p.SetY(5);
    EXPECT_EQ(4, p.GetX());
    EXPECT_EQ(5, p.GetY());
}

TEST(GeometryTest, PointModificationFail)
{
    point p = point();
    
    p.SetX(4);
    p.SetY(5);
    EXPECT_NE(0, p.GetX());
    EXPECT_NE(0, p.GetY());
}

// vec3

TEST(GeometryTest, Vec3Basic)
{
    vec3 v;

    EXPECT_EQ(sizeof(vec3), sizeof(float)*3);
}

/*
TEST(GeometryTest, Vec3CreationDefault)
{
    vec3 v;

    EXPECT_EQ(0.0f, v.X());
    EXPECT_EQ(0.0f, v.Y());
    EXPECT_EQ(0.0f, v.Z());
}
*/
TEST(GeometryTest, Vec3CreationInitialized)
{
    vec3 v(1.0f, 2.0f, 3.0f);

    EXPECT_EQ(1.0f, v.X());
    EXPECT_EQ(2.0f, v.Y());
    EXPECT_EQ(3.0f, v.Z());
}

TEST(GeometryTest, Vec3CopyConstructor)
{
    vec3 v(10.0f, 10.0f, 10.0f);
    // Call copy constructor via direct initialization
    vec3 w(v);

    EXPECT_EQ(10.0f, w.X());
    EXPECT_EQ(10.0f, w.Y());
    EXPECT_EQ(10.0f, w.Z());
}

TEST(GeometryTest, Vec3AssignmentOperator)
{
    vec3 v(10.0f, 10.0f, 10.0f);
    vec3 w;

    w = v;

    EXPECT_EQ(10.0f, w.X());
    EXPECT_EQ(10.0f, w.Y());
    EXPECT_EQ(10.0f, w.Z());
}

TEST(GeometryTest, Vec3SubscriptOperator)
{
    vec3 v(10.0f, 20.0f, 30.0f);

    EXPECT_EQ(10.0f, v[0]);
    EXPECT_EQ(20.0f, v[1]);
    EXPECT_EQ(30.0f, v[2]);
}

TEST(GeometryTest, Vec3SubscriptOperatorWrite)
{
    vec3 v(0.0f, 0.0f, 0.0f);
    v[0] = 100.0f;
    v[1] = 200.0f;
    v[2] = 300.0f;

    EXPECT_EQ(100.0f, v[0]);
    EXPECT_EQ(200.0f, v[1]);
    EXPECT_EQ(300.0f, v[2]);
}

TEST(GeometryTest, Vec3Setters)
{
    vec3 v(0.0f, 0.0f, 0.0f);

    v.SetX(35.0f);
    v.SetY(25.0f);
    v.SetZ(15.0f);

    EXPECT_EQ(35.0f, v.X());
    EXPECT_EQ(25.0f, v.Y());
    EXPECT_EQ(15.0f, v.Z());
}

TEST(GeometryTest, Vec3CombinedSetter)
{
    vec3 v(0.0f, 0.0f, 0.0f);

    v.SetXYZ(35.0f, 25.0f, 15.0f);

    EXPECT_EQ(35.0f, v.X());
    EXPECT_EQ(25.0f, v.Y());
    EXPECT_EQ(15.0f, v.Z());
}

TEST(GeometryTest, Vec3Getters)
{
    vec3 v(0.5f, 1.5f, 2.5f);

    EXPECT_EQ(0.5f, v.Get(0));
    EXPECT_EQ(1.5f, v.Get(1));
    EXPECT_EQ(2.5f, v.Get(2));
}

TEST(GeometryTest, Vec3UnaryOperators)
{
    vec3 v0(1.0f, 2.0f, 3.0f);

    v0 *= 2.0f;
    EXPECT_EQ(2.0f, v0.X());
    EXPECT_EQ(4.0f, v0.Y());
    EXPECT_EQ(6.0f, v0.Z());

    v0 /= 2.0f;
    EXPECT_EQ(1.0f, v0.X());
    EXPECT_EQ(2.0f, v0.Y());
    EXPECT_EQ(3.0f, v0.Z());

    vec3 v1(10.0f, 20.0f, 30.0f);

    v0 += v1;
    EXPECT_EQ(11.0f, v0.X());
    EXPECT_EQ(22.0f, v0.Y());
    EXPECT_EQ(33.0f, v0.Z());

    v0 -= v1;
    EXPECT_EQ(1.0f, v0.X());
    EXPECT_EQ(2.0f, v0.Y());
    EXPECT_EQ(3.0f, v0.Z());

    /*
    v0 *= v1;
    EXPECT_EQ(10.0f, v0.X());
    EXPECT_EQ(40.0f, v0.Y());
    EXPECT_EQ(90.0f, v0.Z());

    v0 /= v1;
    EXPECT_EQ(1.0f, v0.X());
    EXPECT_EQ(2.0f, v0.Y());
    EXPECT_EQ(3.0f, v0.Z());
    */
}

TEST(GeometryTest, Vec3ArithmeticOperators)
{
    vec3 v0(1.0f, 2.0f, 3.0f);
    vec3 v1(10.0f, 20.0f, 30.0f);

    v0 = v0 + v1;
    EXPECT_EQ(11.0f, v0.X());
    EXPECT_EQ(22.0f, v0.Y());
    EXPECT_EQ(33.0f, v0.Z());

    v0 = v0 - v1;
    EXPECT_EQ(1.0f, v0.X());
    EXPECT_EQ(2.0f, v0.Y());
    EXPECT_EQ(3.0f, v0.Z());

    v0 = v1 * 2.0f;
    EXPECT_EQ(20.0f, v0.X());
    EXPECT_EQ(40.0f, v0.Y());
    EXPECT_EQ(60.0f, v0.Z());

    /*
    v0 = 2.0f * v1;
    EXPECT_EQ(20.0f, v0.X());
    EXPECT_EQ(40.0f, v0.Y());
    EXPECT_EQ(60.0f, v0.Z());
    */

    v0 = v0 / 2.0f;
    EXPECT_EQ(10.0f, v0.X());
    EXPECT_EQ(20.0f, v0.Y());
    EXPECT_EQ(30.0f, v0.Z());

    v0 = -v0;
    EXPECT_EQ(-10.0f, v0.X());
    EXPECT_EQ(-20.0f, v0.Y());
    EXPECT_EQ(-30.0f, v0.Z());
}

TEST(GeometryTest, Vec3ComparisonOperators)
{
    vec3 v0(1.0f, 2.0f, 3.0f);
    vec3 v1(10.0f, 20.0f, 30.0f);

    EXPECT_TRUE(v0 == v0);
    EXPECT_TRUE(v0 != v1);
    EXPECT_FALSE(v0 != v0);
    EXPECT_FALSE(v0 == v1);
}

TEST(GeometryTest, Vec3Normalize)
{
    vec3 v0(2.0f, 4.0f, 8.0f);
    v0.normalize();

    EXPECT_FLOAT_EQ(0.21821788f, v0.X());
    EXPECT_FLOAT_EQ(0.43643576f, v0.Y());
    EXPECT_FLOAT_EQ(0.87287152f, v0.Z());
}

TEST(GeometryTest, Vec3NormalizeZero)
{
    vec3 v(0.0f, 0.0f, 0.0f);
    v.normalize();
    vec3 output(1.0f, 0.0f, 0.0f);

    EXPECT_EQ(output.X(), v.X());
    EXPECT_EQ(output.Y(), v.Y());
    EXPECT_EQ(output.Z(), v.Z());
}

TEST(GeometryTest, Vec3Magnitude)
{
    vec3 east(1.0f, 0.0f, 0.0f);

    EXPECT_EQ(1.0f, east.magnitude());

    vec3 v0(1.0f, 2.0f, 3.0f);
    vec3 vn = v0.normalize();

    EXPECT_FLOAT_EQ(1, vn.magnitude());
    EXPECT_FLOAT_EQ(vn.magnitude(), v0 * vn);
}

TEST(GeometryTest, Vec3SquareMagnitude)
{
    vec3 v0(1.0f, 2.0f, 3.0f);

    EXPECT_FLOAT_EQ(14.0f, v0.sqrmag());
}

TEST(GeometryTest, Vec3Cross)
{
    vec3 east(1.0f, 0.0f, 0.0f);
    vec3 north(0.0f, 1.0f, 0.0f);
    vec3 output(0.0f, 0.0f, 1.0f);

    vec3 up = east.cross(north);

    EXPECT_EQ(output.X(), up.X());
    EXPECT_EQ(output.Y(), up.Y());
    EXPECT_EQ(output.Z(), up.Z());
}

TEST(GeometryTest, Vec3CheckNaN)
{
    vec3 v0(0.0f, 0.0f, 0.0f);
    vec3 v1(1.0f, 0.0f, 0.0f);
    vec3 v2(-1.0f, -2.0f, -3.0f);

    EXPECT_FALSE(v0.checknan());
    EXPECT_FALSE(v1.checknan());
    EXPECT_FALSE(v2.checknan());
}

// matrix33

TEST(GeometryTest, Matrix33Basic)
{
    matrix33 m;

    EXPECT_EQ(sizeof(m), sizeof(float)*3*3);
}

TEST(GeometryTest, Matrix33CreationDefault)
{
    matrix33 m;

    vec3 col0(1, 0, 0);
    vec3 col1(0, 1, 0);
    vec3 col2(0, 0, 1);

    EXPECT_TRUE(col0 == m.GetColumn(0));
    EXPECT_TRUE(col1 == m.GetColumn(1));
    EXPECT_TRUE(col2 == m.GetColumn(2));
}

TEST(GeometryTest, Matrix33CreateInitializedVec3)
{
    vec3 v1(1, 1, 1);
    vec3 v2(2, 2, 2);
    vec3 v3(3, 3, 3);

    matrix33 m(v1, v2, v3);

    EXPECT_TRUE(v1 == m.GetColumn(0));
    EXPECT_TRUE(v2 == m.GetColumn(1));
    EXPECT_TRUE(v3 == m.GetColumn(2));
}

TEST(GeometryTest, Matrix33CreateInitializedFloat)
{
    matrix33 m1(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);

    EXPECT_EQ(1.0f, m1[0].X());
    EXPECT_EQ(1.0f, m1[0].Y());
    EXPECT_EQ(1.0f, m1[0].Z());
    EXPECT_EQ(3.0f, m1[1].X());
    EXPECT_EQ(3.0f, m1[1].Y());
    EXPECT_EQ(3.0f, m1[1].Z());
    EXPECT_EQ(5.0f, m1[2].X());
    EXPECT_EQ(5.0f, m1[2].Y());
    EXPECT_EQ(5.0f, m1[2].Z());
}

TEST(GeometryTest, Matrix33CopyConstructor)
{
    vec3 v1(1, 1, 1);
    vec3 v2(2, 2, 2);
    vec3 v3(3, 3, 3);

    matrix33 m1(v1, v2, v3);
    // Call copy constructor via direct initialization
    matrix33 m2(m1);

    EXPECT_TRUE(v1 == m2.GetColumn(0));
    EXPECT_TRUE(v2 == m2.GetColumn(1));
    EXPECT_TRUE(v3 == m2.GetColumn(2));
}

TEST(GeometryTest, Matrix33AssignmentOperator)
{
    vec3 v1(1, 1, 1);
    vec3 v2(2, 2, 2);
    vec3 v3(3, 3, 3);

    matrix33 m1(v1, v2, v3);
    matrix33 m2;

    m2 = m1;

    EXPECT_TRUE(v1 == m2.GetColumn(0));
    EXPECT_TRUE(v2 == m2.GetColumn(1));
    EXPECT_TRUE(v3 == m2.GetColumn(2));
}

TEST(GeometryTest, Matrix33SubscriptOperator)
{
    vec3 v1(1, 1, 1);
    vec3 v2(2, 2, 2);
    vec3 v3(3, 3, 3);

    matrix33 m1(v1, v2, v3);

    EXPECT_TRUE(v1 == m1[0]);
    EXPECT_TRUE(v2 == m1[1]);
    EXPECT_TRUE(v3 == m1[2]);
}

TEST(GeometryTest, Matrix33SubscriptOperatorWrite)
{
    matrix33 m1(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);

    m1[1] = vec3(10.0f, 20.0f, 30.0f);

    EXPECT_EQ( 1.0f, m1[0].X());
    EXPECT_EQ( 1.0f, m1[0].Y());
    EXPECT_EQ( 1.0f, m1[0].Z());
    EXPECT_EQ(10.0f, m1[1].X());
    EXPECT_EQ(20.0f, m1[1].Y());
    EXPECT_EQ(30.0f, m1[1].Z());
    EXPECT_EQ( 5.0f, m1[2].X());
    EXPECT_EQ( 5.0f, m1[2].Y());
    EXPECT_EQ( 5.0f, m1[2].Z());
}

TEST(GeometryTest, Matrix33Setters)
{
    matrix33 m1(0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f);

    m1.SetColumn(0, vec3(1.0f, 1.0f, 1.0f));
    m1.SetColumn(2, vec3(100.0f, 200.0f, 300.0f));

    matrix33 m2(1.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 0.0f,
                100.0f, 200.0f, 300.0f);

    EXPECT_TRUE(m1 == m2);
}

TEST(GeometryTest, Matrix33CombinedSetter)
{
    matrix33 m1(0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f);

    m1.SetColumns(vec3(1.0f, 1.0f, 1.0f),
                  vec3(1.0f, 1.0f, 1.0f),
                  vec3(1.0f, 1.0f, 1.0f));

    matrix33 m2(1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f);

    EXPECT_TRUE(m1 == m2);
}

TEST(GeometryTest, Matrix33Getters)
{
    matrix33 m1(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);

    EXPECT_TRUE(vec3(1.0f, 1.0f, 1.0f) == m1.GetColumn(0));
    EXPECT_TRUE(vec3(3.0f, 3.0f, 3.0f) == m1.GetColumn(1));
    EXPECT_TRUE(vec3(5.0f, 5.0f, 5.0f) == m1.GetColumn(2));
}

TEST(GeometryTest, Matrix33UnaryOperators)
{
    matrix33 m0(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);

    m0 *= 2.0f;
    EXPECT_TRUE(vec3( 2.0f,  2.0f,  2.0f) == m0[0]);
    EXPECT_TRUE(vec3( 6.0f,  6.0f,  6.0f) == m0[1]);
    EXPECT_TRUE(vec3(10.0f, 10.0f, 10.0f) == m0[2]);

    m0 /= 2.0f;
    EXPECT_TRUE(vec3(1.0f, 1.0f, 1.0f) == m0[0]);
    EXPECT_TRUE(vec3(3.0f, 3.0f, 3.0f) == m0[1]);
    EXPECT_TRUE(vec3(5.0f, 5.0f, 5.0f) == m0[2]);

    matrix33 m1(10.0f, 10.0f, 10.0f,
                20.0f, 20.0f, 20.0f,
                30.0f, 30.0f, 30.0f);

    m0 += m1;
    EXPECT_TRUE(vec3(11.0f, 11.0f, 11.0f) == m0[0]);
    EXPECT_TRUE(vec3(23.0f, 23.0f, 23.0f) == m0[1]);
    EXPECT_TRUE(vec3(35.0f, 35.0f, 35.0f) == m0[2]);

    m0 -= m1;
    EXPECT_TRUE(vec3(1.0f, 1.0f, 1.0f) == m0[0]);
    EXPECT_TRUE(vec3(3.0f, 3.0f, 3.0f) == m0[1]);
    EXPECT_TRUE(vec3(5.0f, 5.0f, 5.0f) == m0[2]);

    m0 *= m1;
    EXPECT_TRUE(vec3( 90.0f,  90.0f,  90.0f) == m0[0]);
    EXPECT_TRUE(vec3(180.0f, 180.0f, 180.0f) == m0[1]);
    EXPECT_TRUE(vec3(270.0f, 270.0f, 270.0f) == m0[2]);

//  m0 /= m1;
}

/**
TEST(GeometryTest, Matrix33ArithmeticOperators)
{
    matrix33 m0(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);
    matrix33 m1(10.0f, 10.0f, 10.0f,
                20.0f, 20.0f, 20.0f,
                30.0f, 30.0f, 30.0f);

//  m0 = m0 + m1;

//  m0 = m0 - m1;

//  m0 = m1 * 2.0f;

//  m0 = 2.0f * m1;

//  m0 = m0 / 2.0f;

//  m0 = -m0;
}
*/

TEST(GeometryTest, Matrix33ComparisonOperators)
{
    matrix33 m0(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);
    matrix33 m1(2.0f, 2.0f, 2.0f,
                4.0f, 4.0f, 4.0f,
                6.0f, 6.0f, 6.0f);

    EXPECT_TRUE(m0 == m0);
    EXPECT_TRUE(m0 != m1);
    EXPECT_FALSE(m0 != m0);
    EXPECT_FALSE(m0 == m1);
}

TEST(GeometryTest, Matrix33Identity)
{
    matrix33 m0(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);
    matrix33  I(1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f);

    m0.Identity();

    EXPECT_TRUE(m0 == I);
}

TEST(GeometryTest, Matrix33Invert)
{
    matrix33 m0(10.0f, 15.0f, 10.0f,
                 8.0f,  7.0f,  6.0f,
                 3.0f,  2.0f,  1.0f);

    m0.Invert();

    EXPECT_FLOAT_EQ(-0.1f, m0[0].X());
    EXPECT_FLOAT_EQ( 0.1f, m0[0].Y());
    EXPECT_FLOAT_EQ( 0.4f, m0[0].Z());
    EXPECT_FLOAT_EQ( 0.2f, m0[1].X());
    EXPECT_FLOAT_EQ(-0.4f, m0[1].Y());
    EXPECT_FLOAT_EQ( 0.4f, m0[1].Z());
    EXPECT_FLOAT_EQ(-0.1f, m0[2].X());
    EXPECT_FLOAT_EQ( 0.5f, m0[2].Y());
    EXPECT_FLOAT_EQ(-1.0f, m0[2].Z());
}

TEST(GeometryTest, Matrix33InvertInvert)
{
    matrix33 m0(10.0f,  5.0f, 10.0f,
                 8.0f,  7.0f,  6.0f,
                 3.0f,  2.0f,  1.0f);
    matrix33 m1(m0);

    m0.Invert();
    m0.Invert();

    EXPECT_FLOAT_EQ(m1[0].X(), m0[0].X());
    EXPECT_FLOAT_EQ(m1[0].Y(), m0[0].Y());
    EXPECT_FLOAT_EQ(m1[0].Z(), m0[0].Z());
    EXPECT_FLOAT_EQ(m1[1].X(), m0[1].X());
    EXPECT_FLOAT_EQ(m1[1].Y(), m0[1].Y());
    EXPECT_FLOAT_EQ(m1[1].Z(), m0[1].Z());
    EXPECT_FLOAT_EQ(m1[2].X(), m0[2].X());
    EXPECT_FLOAT_EQ(m1[2].Y(), m0[2].Y());
    EXPECT_FLOAT_EQ(m1[2].Z(), m0[2].Z());
}

TEST(GeometryTest, Matrix33InvertIdentity)
{
    matrix33 m0(1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f);
    matrix33  I(1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f);

    m0.Invert();

    EXPECT_TRUE(m0 == I);
}

TEST(GeometryTest, Matrix33InvertSingularMatrix)
{
    matrix33 m0(1.0f, 1.0f, 1.0f,
                3.0f, 3.0f, 3.0f,
                5.0f, 5.0f, 5.0f);

    m0.Invert();

    EXPECT_FLOAT_EQ( 1.0f, m0[0].X());
    EXPECT_FLOAT_EQ( 0.0f, m0[0].Y());
    EXPECT_FLOAT_EQ( 0.0f, m0[0].Z());
    EXPECT_FLOAT_EQ( 0.0f, m0[1].X());
    EXPECT_FLOAT_EQ( 1.0f, m0[1].Y());
    EXPECT_FLOAT_EQ( 0.0f, m0[1].Z());
    EXPECT_FLOAT_EQ( 0.0f, m0[2].X());
    EXPECT_FLOAT_EQ( 0.0f, m0[2].Y());
    EXPECT_FLOAT_EQ( 1.0f, m0[2].Z());
}

TEST(GeometryTest, Matrix33MultTranspose)
{
    matrix33 m0(10.0f, 15.0f, 10.0f,
                 8.0f,  7.0f,  6.0f,
                 3.0f,  2.0f,  1.0f);
    matrix33 m1( 1.0f,  1.0f,  1.0f,
                 3.0f,  3.0f,  3.0f,
                 5.0f,  5.0f,  5.0f);

    m0.MultTranspose(m1);

    EXPECT_FLOAT_EQ(49.0f, m0[0].X());
    EXPECT_FLOAT_EQ(46.0f, m0[0].Y());
    EXPECT_FLOAT_EQ(33.0f, m0[0].Z());
    EXPECT_FLOAT_EQ(49.0f, m0[1].X());
    EXPECT_FLOAT_EQ(46.0f, m0[1].Y());
    EXPECT_FLOAT_EQ(33.0f, m0[1].Z());
    EXPECT_FLOAT_EQ(49.0f, m0[2].X());
    EXPECT_FLOAT_EQ(46.0f, m0[2].Y());
    EXPECT_FLOAT_EQ(33.0f, m0[2].Z());
}

TEST(GeometryTest, Matrix33Orient)
{
    matrix33 m0(10.0f, 15.0f, 10.0f,
                 8.0f,  7.0f,  6.0f,
                 3.0f,  2.0f,  1.0f);

    vec3 east(1.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 0.0f, 1.0f);

    m0.Orient(east, up);

    EXPECT_FLOAT_EQ( 1.0f, m0[0].X());
    EXPECT_FLOAT_EQ( 0.0f, m0[0].Y());
    EXPECT_FLOAT_EQ( 0.0f, m0[0].Z());
    EXPECT_FLOAT_EQ( 0.0f, m0[1].X());
    EXPECT_FLOAT_EQ( 0.0f, m0[1].Y());
    EXPECT_FLOAT_EQ( 1.0f, m0[1].Z());
    EXPECT_FLOAT_EQ( 0.0f, m0[2].X());
    EXPECT_FLOAT_EQ(-1.0f, m0[2].Y());
    EXPECT_FLOAT_EQ( 0.0f, m0[2].Z());
}

