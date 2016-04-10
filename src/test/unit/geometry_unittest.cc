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

