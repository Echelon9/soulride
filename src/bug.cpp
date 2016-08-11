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
// bug.cpp	4/6/1998 Thatcher Ulrich <ulrich@world.std.com>

// Demonstrates apparent cvpack bug.  The same bug occurs in when linked with
// my whole application.


//#include "config.hpp"
// xxxx following code would ordinarily come from "config.hpp"
namespace Config {
	void	Open();
	void	Close();

	// void	ProcessCommandLineOptions(argc, argv);

	// Variable values are strings.
	void	SetValue(char* VarName, char* NewValue);
	const char*	GetValue(char* VarName);
};
// xxxx


#include <string>
#include <map>


// Disable warning about truncating identifiers in debug info.
#pragma warning(disable:4786)


using namespace std;


namespace Config {


bool	IsOpen = false;
map<string, string> ConfigValues;


void	Open()
// Set up.
{
	if (IsOpen) return;

	// Clear ConfigValues?
	
	IsOpen = true;
}


void	Close()
// Shut down.
{
	// Clear ConfigValues?
	
	IsOpen = false;
}


void	SetValue(char* VarName, char* NewValue)
// Adds a new variable with the given value, or if the variable with the
// given name already exists, then sets its value.
{
	ConfigValues[VarName] = NewValue;	// ???
}


const char*	GetValue(char* VarName)
// Returns the value of the specified variable.  If it doesn't exist, returns
// NULL.
{
	return ConfigValues[VarName].c_str();
}


};



// Dummy WinMain, for demonstrating cvpack bug.

#include <windows.h>


int PASCAL	WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// Windows app entry point.
{
	return 0;
}
