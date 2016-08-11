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
// player.cpp	-thatcher 8/16/1999 Copyright Slingshot Game Technology

// Code for keeping track of player information.


#include <stdio.h>
#include <string.h>
#ifdef LINUX
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#else // not LINUX
#include <io.h>
#include <direct.h>
#endif // not LINUX
#include "utility.hpp"
#include "player.hpp"
#include "game.hpp"

#ifdef MACOSX
#include <string>
#include <iostream>
#endif

const int	MAX_RUNS = 30;
const int	PLAYER_FILE_VERSION = 2;



class PlayerImp : public Player {
public:
;


char*	Name;
char*	LastMountain;


struct MountainInfo {
	int	HighestRunCompleted;
	int	HighScore[MAX_RUNS];

	MountainInfo() {
		HighestRunCompleted = -1;

		int	i;
		for (i = 0; i < MAX_RUNS; i++) HighScore[i] = 0;
	}

	void	Read(FILE* fp, int Version)
	{
		HighestRunCompleted = (int32) Read32(fp);

		int	i;
		for (i = 0; i < MAX_RUNS; i++) HighScore[i] = 0;

		if (Version >= 2) {
			for (i = 0; i < HighestRunCompleted; i++) {
				HighScore[i] = Read32(fp);
			}
		}
	}
	
	void	Write(FILE* fp)
	{
		Write32(fp, HighestRunCompleted);

		int	i;
		for (i = 0; i < HighestRunCompleted; i++) {
			Write32(fp, HighScore[i]);
		}
	}
};


BagOf<MountainInfo*>	Mountains;


static void	MakeFilename(char* buf, int bufsize, const char* Name)
// Makes a player-data filename by combining the given name with a subdirectory
// name and file extension.  Truncates to keep within the bufsize limit.
{
	char*	p = buf;
	int count = 0;

#ifdef MACOSX_CARBON

	std::string filename("");
	filename += MacOSX::PlayerData_directory();
	filename += PATH_SEPARATOR;
	filename += Name;
	filename += ".srp";
	
	int stringlength;
	if (filename.length() > bufsize-1)
	  stringlength = filename.length();
	else
	  stringlength = bufsize-1;

	strncpy(buf, filename.c_str(), stringlength);
	
#else // not MACOSX_CARBON

	// Start with "..\PlayerData\".
	const char*	s = ".." PATH_SEPARATOR "PlayerData" PATH_SEPARATOR;
	while (count < bufsize-1) {
		if (*s == 0) break;
		*p++ = *s++;
		count++;
	}

	// Tack on the basename.
	s = Name;
	while (count < bufsize-1) {
		if (*s == 0) break;
		*p++ = *s++;
		count++;
	}

	// Tack on the file extension.
	s = ".srp";
	while (count < bufsize-1) {
		if (*s == 0) break;
		*p++ = *s++;
		count++;
	}

	*p = 0;
#endif // not MACOSX_CARBON
	
}


PlayerImp(const char* name)
// Constructor.  Read data from the specified file.  The given name is only the basename;
// prepends a "../PlayerData/" and appends a ".srp" to form the full filename.
{
	Name = new char[strlen(name) + 1];
	strcpy(Name, name);

	LastMountain = NULL;

	// Form the filename and try to open the file.
	char	temp[1000];
	MakeFilename(temp, 1000, Name);
	FILE*	fp = fopen(temp, "rb");
	
	if (fp == NULL) {
		// Filename does not exist -- new player.  Use defaults.
	} else {
		// Load player info from disk.
		
		// Get the header, and check it.
		uint32	HeaderWord = Read32(fp);
		if ((HeaderWord & 0x00FFFFFF) != 0x00505253 /* "SRP" */) {
			Error e; e << "'" << temp << "' is not a Soul Ride player data file.";
			throw e;
		}
		int	Version = HeaderWord >> 24;
		
		if (Version > PLAYER_FILE_VERSION) {
			Error e; e << "Can't read '" << temp << "', version is unknown.";
			throw e;
		}
		
		// Get misc player info.
		
		// LastMountain
		
		// Mountain data.
		int	MountainCount = Read32(fp);
		for (int i = 0; i < MountainCount; i++) {
			// Load the mountain name.
			char	name[256];
			fgets(name, 256, fp);
			name[strlen(name) - 1] = 0;	// Get rid of trailing '\n'.
			Utility::StringLower(name);	// Convert to all lower-case to avoid confusion.

			// Get the user's stats on this mountain.
			MountainInfo*	m = new MountainInfo;
			m->Read(fp, Version);

			// Remember for later.
			Mountains.Add(m, name);
		}
		
		fclose(fp);
	}
}


~PlayerImp()
// Destructor.
{
	delete [] Name;
	if (LastMountain) delete [] LastMountain;

	Mountains.Empty();
}


void	Save()
// Write the player state info to our file.
{
	char	temp[1000];
	MakeFilename(temp, 1000, Name);

// MacOSX: We already know that PlayerData subdirectory exists.
#ifndef MACOSX_CARBON

	// Make sure PlayerData subdirectory exists.
	chdir("..");
	if (chdir("PlayerData") != 0) {

#ifdef LINUX
		mkdir("PlayerData", 0775);
#else // not LINUX
		mkdir("PlayerData");
#endif // not LINUX
	} else {
		chdir("..");
	}
	chdir("data");
#endif // not MACOSX_CARBON

	// Try to open the file for output.
	FILE* fp = fopen(temp, "wb");
	if (fp == NULL) {
		Error e; e << "Can't open '" << temp << "' for output.";
		throw e;
	}

	// Write the header.
	Write32(fp, 0x00505253 | (PLAYER_FILE_VERSION << 24));
	
	BagOf<MountainInfo*>::Iterator	b;
	
	// Count the mountains we have info on.
	b = Mountains.GetIterator();
	int	MountainCount = 0;
	while (b.IsDone() == false) {
		MountainCount++;
		b++;
	}

	// Write the count.
	Write32(fp, MountainCount);

	// Write the actual mountain info.
	b = Mountains.GetIterator();
	while (b.IsDone() == false) {
		fputs(b.GetName(), fp);
		fputc('\n', fp);
		(*b)->Write(fp);
		b++;
	}
	
	fclose(fp);
}


void	CompletedRun(int run)
// Notification message that tells us the player finished the specified run.
{
	MountainInfo*	m;

	char	mtn[1000];	// temporary buffer to hold mountain name.
	strncpy(mtn, Game::GetCurrentMountain(), 1000);
	mtn[999] = 0;
	Utility::StringLower(mtn);
	
	if (Mountains.GetObject(&m, mtn) == false) {
		m = new MountainInfo;
		Mountains.Add(m, mtn);
	}

	if (run > m->HighestRunCompleted) {
		m->HighestRunCompleted = run;
	}
}


void	RegisterScore(int run, int score)
// Notification message that tells us the player scored the given score
// on the specified run on the current mountain.
{
	MountainInfo*	m;

	char	mtn[1000];	// temporary buffer to hold mountain name.
	strncpy(mtn, Game::GetCurrentMountain(), 1000);
	mtn[999] = 0;
	Utility::StringLower(mtn);
	
	if (Mountains.GetObject(&m, mtn) == false) {
		m = new MountainInfo;
		Mountains.Add(m, mtn);
	}

	if (run >= 0 && run < MAX_RUNS) {
		int	hs = m->HighScore[run];
		if (score > hs) {
			m->HighScore[run] = score;
		}
	}
}


int	GetHighScore(int run)
// Returns this player's high score on the specified run.
{
	MountainInfo*	m;

	char	mtn[1000];	// temporary buffer to hold mountain name.
	strncpy(mtn, Game::GetCurrentMountain(), 1000);
	mtn[999] = 0;
	Utility::StringLower(mtn);
	
	if (Mountains.GetObject(&m, mtn) == false) {
		m = new MountainInfo;
		Mountains.Add(m, mtn);
	}

	if (run >= 0 && run < MAX_RUNS) {
		return m->HighScore[run];
	} else {
		return 0;
	}
}


const char*	GetName()
// Returns the name of this player.
{
	return Name;
}


int	GetHighestRunCompleted(const char* MountainName)
// Returns the highest run completed by the player on the specified mountain.
{
	if (MountainName == NULL) return -1;
	
	MountainInfo*	m;

	char	mtn[1000];	// temporary buffer to hold mountain name.
	strncpy(mtn, MountainName, 1000);
	mtn[999] = 0;
	Utility::StringLower(mtn);
	
	if (Mountains.GetObject(&m, mtn) == false) {
		return -1;
	} else {
		return m->HighestRunCompleted;
	}
}


const char*	GetLastMountainUsed()
// Returns the name of the last mountain used by this player.  Useful
// during start-up.
{
	return LastMountain;
}



};	//  end class PlayerImp.
//
//
//



Player*	Player::LoadPlayer(const char* name)
// Create a PlayerImp and return it.
{
	return new PlayerImp(name);
}


bool	Player::GetSavedPlayersAvailable()
// Return true if there are any player files that can be loaded.
{


#ifdef LINUX

#ifdef MACOSX_CARBON
  DIR*  d = opendir(MacOSX::PlayerData_directory());
#else  // not MACOSX_CARBON
  DIR*	d = opendir(".." PATH_SEPARATOR "PlayerData");
#endif // not MACOSX_CARBON

  if (d) {
		struct dirent*	dir;
		while (dir = readdir(d)) {
			if (Utility::StringCompareIgnoreCase(Utility::GetExtension(dir->d_name), "srp") == 0) {
				// There *is* a player file in this dir.
				closedir(d);
				return true;
			}
		}
		closedir(d);
	}
	// Couldn't find any player files.
	return false;

#else // not LINUX

	_finddata_t	d;
	long	handle = _findfirst(".." PATH_SEPARATOR "PlayerData" PATH_SEPARATOR "*.srp", &d);
	_findclose(handle);

	if (handle != -1) return true;
	else return false;
#endif // not LINUX
}
