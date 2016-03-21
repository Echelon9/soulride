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
// script.cpp	-thatcher 2/16/2000 Copyright Slingshot Game Technology

// Code for a simple scripting/configuration parser.


#include "utility.hpp"
#include "config.hpp"
#include "error.hpp"
#include "script.hpp"


namespace Script {
;


BagOf<KeywordHandlerFP>	Handlers;


void	AddKeywordHandler(const char* keyword, KeywordHandlerFP handler)
// Associates the given handler function with the specified keyword.
// When a script line beginning with that keyword is processed, the
// handler is called with the remainder of the script line.  The handler
// is expected to return a pointer to any remaining characters it didn't
// process.
{
	// Check for existing handler.
	KeywordHandlerFP	dummy = NULL;
	Handlers.GetObject(&dummy, keyword);
	if (dummy) {
		HandlerRedefinedError e; e << "Script::AddKeywordHandler: keyword '" << keyword << "' already has a handler.";
		throw e;
	}

	Handlers.Add(handler, keyword);
}


static bool	whitespace(char c)
// Returns true if the given character is whitespace; false otherwise.
{
	if (c == ' ' || c == '\t' || c == '
' || c == '\r') return true;
	else return false;
}


void	ProcessLine(const char* line)
// Reads the first word of the line, which should be some sort of
// command.  Passes the remainder of the line to the handler associated
// with that command.
// If there's no handler, throws an exception.
{
	if (line == NULL) return;
	
	char	keyword[200];

	// Skip leading whitespace.
	while (*line && whitespace(*line)) line++;

	// Get keyword.
	int	i = 0;
	for (;;) {
		char	c = line[i];
		if (c == 0 || whitespace(c) == true) break;
		keyword[i] = c;
		i++;
	}
	keyword[i] = 0;

	if (keyword[0] == 0) {
		// No keyword.
		return;
	}

	// Jump past the keyword.
	line += i;

	// Skip more whitespace.
	while (*line && whitespace(*line)) line++;
	
	// Get the handler associated with this keyword.
	KeywordHandlerFP	handler = NULL;
	Handlers.GetObject(&handler, keyword);
	if (handler == NULL) {
		NoHandlerError e; e << "Script error: no handler for keyword '" << keyword << "'.";
		throw e;
	}

	// Call the handler with the remainder of the line.
	(*handler)(line);
}


void	Load(const char* filename)
// Opens the specified file, and processes each line as a script statement.
{
	FILE*	fp = Utility::FileOpen(filename, "r");
	if (fp == NULL) return;	// Throw an exception instead?

	char	line[1000];

	for (;;) {
		if (fgets(line, 1000, fp) == NULL) break;

		ProcessLine(line);
	}

	fclose(fp);
}


const char*	GetNextWord(char* dest, const char* args)
// Given the argument string, this function copies the next word from it
// into dest[], and returns the remainder of the string.  In the future,
// perhaps this function would evaluate the next argument expression instead
// of just doing a simple copy...
{
	// Skip leading whitespace.
	while (*args && whitespace(*args)) args++;

	// Get keyword.
	int	i = 0;
	for (;;) {
		char	c = args[i];
		if (c == 0 || whitespace(c) == true) break;
		dest[i] = c;
		i++;
	}
	dest[i] = 0;

	// Jump past the keyword.
	args += i;

	return args;
}


}	// end namespace Script.
