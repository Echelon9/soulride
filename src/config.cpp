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
// config.cpp	-thatcher 4/6/1998 Copyright Thatcher Ulrich

// Code for Config module.  For getting/setting configuration variables.
// The config variables are actually Lua globals, which can also be
// manipulated by Lua scripts or the Lua API.


#ifdef LINUX
#include <unistd.h>
#endif // LINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.hpp"
#include "config.hpp"
#include "lua.hpp"
extern "C" {
#include "lua/include/lualib.h"
}


namespace Config {


bool	IsOpen = false;


void	Open()
// Set up.
{
	if (IsOpen) return;

	lua_open();
	lua_mathlibopen();
	
	IsOpen = true;
}


void	Close()
// Shut down.
{
	lua_close();
	
	IsOpen = false;
}


static bool	whitespace(char c)
// Returns true if the given character is whitespace; false otherwise.
{
	if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return true;
	else return false;
}


const char*	ProcessOption(const char* OptionText)
// Scans the given option text looking for something that looks like
// "varname = value ", "varname=value ", "varname ", etc.  Assigns the
// value to the variable, and returns a pointer to the next character to
// read in OptionText if there's any left after the read expression.  If
// there're no more characters in the string, returns NULL.
{
	if (OptionText == NULL || *OptionText == 0) return NULL;
	
	char	Variable[200], Value[200];
	enum { WHITESPACE0, VARIABLE, WHITESPACE1, EQUALS, WHITESPACE2, VALUE, DO_ASSIGNMENT } Phase = WHITESPACE0;

	char*	NextVar = Variable;
	char*	NextVal = Value;
	bool	Done = false;
	bool	QuotesOn = false;
	do {
		int	c = *OptionText;

		switch (Phase) {
		default:
		case WHITESPACE0:
			if (c == 0) {
				OptionText = NULL;
				Done = true;
			} else if (whitespace(c)) {
				// Consume the whitespace character.
				OptionText++;
			} else {
				Phase = VARIABLE;
			}
			break;

		case VARIABLE:
			if (c == 0 || whitespace(c) || c == '=') {
				// Terminate the variable name and change phases.
				*NextVar = 0;
				NextVar = Variable;
				Phase = WHITESPACE1;
			} else {
				// Add the character to the variable name.
				*NextVar++ = c;
				OptionText++;
			}
			break;

		case WHITESPACE1:
			if (c == 0 || !whitespace(c)) {
				Phase = EQUALS;
			} else {
				OptionText++;
			}
			break;

		case EQUALS:
			if (c == '=') {
				OptionText++;
				Phase = WHITESPACE2;
			} else {
				// Looks like the start of another variable name.
				// Assume "=1" and make the assignment to the current variable name before continuing.
				strcpy(Value, "1");
				Phase = DO_ASSIGNMENT;
			}
			break;

		case WHITESPACE2:
			if (c == '\"') {
				QuotesOn = true;
				Phase = VALUE;
				OptionText++;
				
			} else if (c == 0 || !whitespace(c)) {
				Phase = VALUE;
			} else {
				OptionText++;
			}
			break;

		case VALUE:
			if (QuotesOn == true && c == '\"') {
				QuotesOn = false;
				OptionText++;
				
				*NextVal = 0;
				NextVal = Value;
				Phase = DO_ASSIGNMENT;
			} else if (c == 0 || (QuotesOn == false && whitespace(c))) {
				// Terminate the value string and change phases.
				*NextVal = 0;
				NextVal = Value;
				Phase = DO_ASSIGNMENT;
			} else {
				// Add the character to the value string.
				*NextVal++ = c;
				OptionText++;
			}
			break;

		case DO_ASSIGNMENT:
			// Assign the value to the variable name.
			if (Variable[0]) {	// Don't allow empty variable name strings.
				Set(Variable, Value);
			}
			Phase = WHITESPACE0;
			QuotesOn = false;
			break;
		}
	} while (!Done);

	return OptionText;
}


void	ProcessCommandLine(const char* CommandLine)
// Processes command-line variable assignments.  The command line is
// expected to consist of a list of assignments.  Each assignment
// consists of a variable name, followed by an '=' character, followed
// by the variable's desired value.  Whitespace is used to separate
// assignments.  Whitespace is optional around the '=' character.
// If the '=' character is missing, "1" is assumed for the variable's
// value.
{
	while (CommandLine) {
		CommandLine = ProcessOption(CommandLine);
	}
}


char*	ScopeName = NULL;


void	SetLocalScope(const char* scope)
// Defines a local scope for config variables.  The scopename is just
// an ordinary Lua table name, which may contain variables specific to
// this scope.
//
// All the Config::Get...() functions check the local scope first,
// before checking the global scope.
//
// Local scope variables can be set in Lua using normal table
// assignment.
{
	// Delete any previous scopename, if any.

	if (ScopeName) {
		delete [] ScopeName;
	}

	ScopeName = new char[strlen(scope) + 1];
	strcpy(ScopeName, scope);
}


void	Set(const char* VarName, const char* NewValue)
// Adds a new variable with the given value, or if the variable with the
// given name already exists, then sets its value.
{
	lua_beginblock();
	if (NewValue) {
		lua_pushstring(const_cast<char*>(NewValue));
	} else {
		lua_pushnil();
	}
	lua_setglobal(const_cast<char*>(VarName));
	lua_endblock();
}


void	SetBool(const char* VarName, bool NewValue)
// Sets the value of the configuration variable, as a bool.
{
	lua_beginblock();
	lua_pushnumber(NewValue ? 1 : 0);
	lua_setglobal(const_cast<char*>(VarName));
	lua_endblock();
}


void	SetInt(const char* VarName, int NewValue)
// Sets the value of the configuration variable, as an int.
{
	lua_beginblock();
	lua_pushnumber(NewValue);
	lua_setglobal(const_cast<char*>(VarName));
	lua_endblock();
}


void	SetFloat(const char* VarName, float NewValue)
// Sets the value of the configuration variable, as a float.
{
	lua_beginblock();
	lua_pushnumber(NewValue);
	lua_setglobal(const_cast<char*>(VarName));
	lua_endblock();
}


lua_Object	GetScopedValue(const char* varname)
// Finds the varname in the local scope, or in the global variables,
// and leaves it on the C2lua stack.
//
// Accepts names of the form "table1.table2.var", where var is nested
// inside table1 and table2.
{
	const int	BUFLEN = 1000;
	char	buf[BUFLEN];

	lua_Object	l = LUA_NOOBJECT;
	if (ScopeName) {
		strncpy(buf, varname, BUFLEN);
		buf[BUFLEN - 1] = 0;

		lua_Object	table = lua_getglobal(ScopeName);

		char*	p = buf;
		char*	next_dot = strchr(p, '.');
		while (next_dot && lua_istable(table))
		{
			*next_dot = 0;
			lua_pushobject(table);
			lua_pushstring(p);
			table = lua_gettable();
			p = next_dot + 1;
			next_dot = strchr(p, '.');
		}

		if (lua_istable(table))
		{
			lua_pushobject(table);
			lua_pushstring(p);
			l = lua_gettable();
		}
	}

	// If we didn't find what we wanted within ScopeName, try again in
	// the globals.
	if (l == LUA_NOOBJECT || lua_isnil(l)) {
		strncpy(buf, varname, BUFLEN);
		buf[BUFLEN - 1] = 0;

		lua_Object	table = LUA_NOOBJECT;
		char*	p = buf;
		char*	next_dot = strchr(p, '.');
		// get the global root.
		if (next_dot)
		{
			*next_dot = 0;
			table = lua_getglobal(p);
			p = next_dot + 1;
		}
		else
		{
			return lua_getglobal(p);
		}

		next_dot = strchr(p, '.');
		while (next_dot && lua_istable(table))
		{
			*next_dot = 0;
			lua_pushobject(table);
			lua_pushstring(p);
			table = lua_gettable();
			p = next_dot + 1;
			next_dot = strchr(p, '.');
		}
		
		// Finally, get the value.
		if (lua_istable(table))
		{
			lua_pushobject(table);
			lua_pushstring(p);
			l = lua_gettable();
		}
	}

	return l;

// old
#if 0
	lua_Object	l = LUA_NOOBJECT;
	if (ScopeName) {
		lua_Object	table = lua_getglobal(ScopeName);
		if (lua_istable(table)) {
			lua_pushobject(table);
			lua_pushstring(const_cast<char*>(varname));
			l = lua_gettable();
		}
	}
	if (l == LUA_NOOBJECT || lua_isnil(l)) {
		l = lua_getglobal(const_cast<char*>(varname));
	}

	return l;
#endif // 0
}


const char*	Get(const char* VarName)
// Returns the value of the specified variable.  If it doesn't exist, returns
// NULL.
{
	lua_beginblock();
	lua_Object	l = GetScopedValue(VarName);
	const char*	p = lua_getstring(l);
	lua_endblock();
	return p;
}


bool	GetBool(const char* VarName)
// Returns the value of the variable, interpreted as a bool.
{
	lua_beginblock();
	lua_Object	l = GetScopedValue(VarName);
	bool	b = lua_getnumber(l) != 0 ? true : false;
	lua_endblock();
	return b;
}


int	GetInt(const char* VarName)
// Returns the value of the variable, interpreted as an int.
{
	lua_beginblock();
	lua_Object	l = GetScopedValue(VarName);
	int	i = int(lua_getnumber(l));
	lua_endblock();
	return i;
}


float	GetFloat(const char* VarName)
// Returns the value of the variable, interpreted as a float.
{
	lua_beginblock();
	lua_Object	l = GetScopedValue(VarName);
	float	f = float(lua_getnumber(l));
	lua_endblock();
	return f;
}


void	Toggle(const char* var)
// Inverts the value of a boolean Config:: value.
{
	SetBool(var, !GetBool(var));
}


void	ExportValue(const char* VarName)
// Read "config.txt" looking for a line of the form "<VarName> = <value>".
// Substitutes the current value of VarName for the value stored in the file.
// If the file doesn't already have a VarName entry, then adds it to the end.
{
	FILE*	out = fopen(".." PATH_SEPARATOR "config.tmp", "w");
	if (out == NULL) return;	// Failure.

	FILE*	in = fopen(".." PATH_SEPARATOR "config.txt", "r");
	if (in == NULL) {
		// No existing config.txt.  Just write our single option and be done with it.
		fprintf(out, "%s = \"%s\"\n", VarName, Get(VarName));
		fclose(out);
		rename(".." PATH_SEPARATOR "config.tmp", ".." PATH_SEPARATOR "config.txt");
		return;
	}

	// Find and substitute VarName.
	char	line[1000];
	char	var[1000];
	for (;;) {
		char*	res = fgets(line, 1000, in);
		if (res == NULL) {
			// End of file.  Didn't find our variable, so just add it to the end
			// of the output file.
			fprintf(out, "%s = \"%s\"\n", VarName, Get(VarName));
			break;
		}
		// See if this line starts with the variable we're interested in.
		var[0] = 0;
		sscanf(line, "%s[ =\n]", var);
		if (strcmp(var, VarName) == 0) {
			// Found our line.  Replace it with our own line, and then
			// copy the remainder unchanged.
			fprintf(out, "%s = \"%s\"\n", VarName, Get(VarName));
			int	c;
			while ((c = fgetc(in)) != EOF) fputc(c, out);
			break;
		}
		// Otherwise, copy the input line to the output file.
		fputs(line, out);
	}

	fclose(in);
	fclose(out);

	// Rename the files.
	unlink(".." PATH_SEPARATOR "config.txt");
	rename(".." PATH_SEPARATOR "config.tmp", ".." PATH_SEPARATOR "config.txt");
}


};

