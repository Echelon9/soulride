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
// config.hpp	-thatcher 4/6/1998 Copyright Thatcher Ulrich

// Interface for configuration variables.


#ifndef CONFIG_HPP
#define CONFIG_HPP


namespace Config {
	void	Open();
	void	Close();

	const char*	ProcessOption(const char* OptionText);
	void	ProcessCommandLine(const char* CommandLine);

	// Setting.
	void	Set(const char* VarName, const char* NewValue);
	void	SetBool(const char* VarName, bool NewValue);
	void	SetInt(const char* VarName, int NewValue);
	void	SetFloat(const char* VarName, float NewValue);

	// aliases
	inline void	SetValue(const char* VarName, const char* NewValue) { Set(VarName, NewValue); }
	inline void	SetBoolValue(const char* VarName, bool NewValue) { SetBool(VarName, NewValue); }
	inline void	SetIntValue(const char* VarName, int NewValue) { SetInt(VarName, NewValue); }
	inline void	SetFloatValue(const char* VarName, float NewValue) { SetFloat(VarName, NewValue); }

	// Getting.
	const char*	Get(const char* VarName);
	bool	GetBool(const char* VarName);	// Returns the value of the variable, interpreted as a bool.
	int	GetInt(const char* VarName);
	float	GetFloat(const char* VarName);	// Returns the value of the variable, interpreted as a float.

	// aliases
	inline const char*	GetValue(const char* VarName) { return Get(VarName); }
	inline bool	GetBoolValue(const char* VarName) { return GetBool(VarName); }
	inline int	GetIntValue(const char* VarName) { return GetInt(VarName); }
	inline float	GetFloatValue(const char* VarName) { return GetFloat(VarName); }

	void	Toggle(const char* var);	// Inverts the value of a boolean variable.

	// Persistent values (stored in config.txt)
	void	ExportValue(const char* VarName);

	// Defines a local scope for config variables.  Essentially a Lua
	// tablename, for mountain-specific values.
	void	SetLocalScope(const char* scopename);
};


#endif // CONFIG_HPP
