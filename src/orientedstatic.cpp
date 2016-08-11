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
// OrientedStatic.cpp	-thatcher 2/26/1999 Copyright Slingshot Game Technology

// Code for OrientedStatic objects.


#include "main.hpp"
#include "model.hpp"


class OrientedStatic : public MOriented
{
public:
	OrientedStatic() {}
	
	void	Update(const UpdateState& u) {}
private:
};


static struct InitOrientedStatic {
	InitOrientedStatic() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	{
		Model::AddObjectLoader(4, OrientedStaticLoader);
	}
	static void	OrientedStaticLoader(FILE* fp)
	// Loads information from the given file and uses it to initialize an OrientedStatic object.
	{
		OrientedStatic*	o = new OrientedStatic();

		// Initialize location.
		o->LoadLocation(fp);

		// Initialize orientation.
		o->LoadOrientation(fp);
		
		// Link to the database.
		Model::AddStaticObject(o);
	}

} InitOrientedStatic;

