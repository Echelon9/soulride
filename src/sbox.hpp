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
// sbox.hpp	-thatcher 1/4/1999 Copyright Slingshot

// Code for SBox class, a subclass of SModel which implements a solid axis-aligned box for collisions.


#ifndef SBOX_HPP
#define SBOX_HPP


#include "model.hpp"


class SBox : public SModel {
public:
	SBox(FILE* fp);
	bool	CheckForContact(Collide::ContactInfo* result, const Collide::SegmentInfo& seg, const Collide::CylinderInfo& cyl);

private:
	Collide::BoxInfo	b;
};


#endif // SBOX_HPP

