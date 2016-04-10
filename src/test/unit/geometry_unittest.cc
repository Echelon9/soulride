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

