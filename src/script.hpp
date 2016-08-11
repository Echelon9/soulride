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
// script.hpp	-thatcher 2/16/2000 Copyright Slingshot Game Technology

// Interface to scripting/configuration parser.


#ifndef SCRIPT_HPP
#define SCRIPT_HPP


#include "error.hpp"


namespace Script {

	void	ProcessLine(const char* line);
	// void	Process(const char* ScriptText);
	void	Load(const char* filename);

	const char*	GetNextWord(char* dest, const char* args);
	
	typedef const char* (*KeywordHandlerFP)(const char*);
	void	AddKeywordHandler(const char* keyword, KeywordHandlerFP handler);

	class NoHandlerError : public Error {};
	class HandlerRedefinedError : public Error {};
};



#endif // SCRIPT_HPP
