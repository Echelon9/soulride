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
// highscore.cpp	-thatcher 9/26/2000 Copyright Slingshot Game Technology

// Code for maintaining high score tables.


#include "utility.hpp"
#include "game.hpp"
#include "highscore.hpp"
#include "player.hpp"

namespace HighScore {
;


const int	PLAYERINFO_MAXLEN = 80;


struct HSEntryData {
	int	Score;
	char	PlayerName[PLAYERNAME_MAXLEN];
	char	PlayerInfo[PLAYERINFO_MAXLEN];
};


const int	MAX_RUNS = 20;
HSEntryData	Local[MAX_RUNS];
//HSEntryData	World[MAX_RUNS];


void	Open()
{
	Clear();
}


void	Close()
{
	Clear();
}


void	Clear()
{
	int	i;
	for (i = 0; i < MAX_RUNS; i++) {
		Local[i].Score = -1;
//		World[i].Score = -1;
	}
}


void	Read()
// Reads the high-scores corresponding to the current mountain.
{
	Clear();
	
	char	filename[200];

	// Read the local high-score table.
	strcpy(filename, ".." PATH_SEPARATOR "PlayerData" PATH_SEPARATOR);

	strcat(filename, Game::GetCurrentMountain());
	strcat(filename, ".local-hs.txt");

	FILE*	fp = fopen(filename, "r");
	if (fp) {
		for (;;) {
			int	Run, Rank, Score;
			int	eof = fscanf(fp, "%d %d %d \n", &Run, &Rank, &Score);
			if (eof == EOF) break;
			
			Run -= 1;
			
			char	Name[PLAYERNAME_MAXLEN];
			char	Info[PLAYERINFO_MAXLEN];
			Name[0] = '\n'; Name[1] = 0;
			Info[0] = '\n'; Info[1] = 0;
			
			fgets(Name, PLAYERNAME_MAXLEN, fp);
			if (Name[strlen(Name)-1] == '\n') Name[strlen(Name)-1] = 0;
			
			fgets(Info, PLAYERINFO_MAXLEN, fp);
			if (Info[strlen(Info)-1] == '\n') Info[strlen(Info)-1] = 0;
			
			if (Rank == 1 && Run >= 0 && Run < MAX_RUNS) {
				HSEntryData&	h = Local[Run];
				h.Score = Score;
				strcpy(h.PlayerName, Name);
				strcpy(h.PlayerInfo, Info);
			}
		}
		fclose(fp);
	}

#if 0
	// Read the world high-score table.
	strcpy(filename, ".." PATH_SEPARATOR "PlayerData" PATH_SEPARATOR);

	strcat(filename, Game::GetCurrentMountain());
	strcat(filename, ".world-hs.txt");

	fp = fopen(filename, "r");
	if (fp) {
		for (;;) {
			int	Run, Rank, Score;
			int	eof = fscanf(fp, "%d %d %d \n", &Run, &Rank, &Score);
			if (eof == EOF) break;

			Run -= 1;
			
			char	Name[PLAYERNAME_MAXLEN];
			char	Info[PLAYERINFO_MAXLEN];
			Name[0] = '\n'; Name[1] = 0;
			Info[0] = '\n'; Info[1] = 0;
			
			fgets(Name, PLAYERNAME_MAXLEN, fp);
			if (Name[strlen(Name)-1] == '\n') Name[strlen(Name)-1] = 0;
			
			fgets(Info, PLAYERINFO_MAXLEN, fp);
			if (Info[strlen(Info)-1] == '\n') Info[strlen(Info)-1] = 0;
			
			if (Rank == 1 && Run >= 0 && Run < MAX_RUNS) {
				HSEntryData&	h = World[Run];
				h.Score = Score;
				strcpy(h.PlayerName, Name);
				strcpy(h.PlayerInfo, Info);
			}
		}
		fclose(fp);
	}
#endif // 0
}


void	RegisterScore(const char* PlayerName, int run, int score)
// Registers a new score.  If the score deserves to be in the local
// high-score table, then stores it.
{
	if (run < 0 || run >= MAX_RUNS) return;
	
	// Check local high-score table to see if this entry is higher than
	// the last entry.
	HSEntryData&	h = Local[run];
	if (h.Score == -1 || h.Score < score) {
		// Replace the current entry with the new entry.
		h.Score = score;
		strncpy(h.PlayerName, PlayerName, PLAYERNAME_MAXLEN);
		h.PlayerName[PLAYERNAME_MAXLEN-1] = 0;
		strcpy(h.PlayerInfo, "---");

		// Write out the new dataset.
		char	filename[200];

		strcpy(filename, ".." PATH_SEPARATOR "PlayerData" PATH_SEPARATOR);

		strcat(filename, Game::GetCurrentMountain());
		strcat(filename, ".local-hs.txt");

		FILE*	fp = fopen(filename, "w");
		if (fp) {
			int	i;
			for (i = 0; i < MAX_RUNS; i++) {
				if (Local[i].Score == -1) continue;
				fprintf(fp, "%d 1 %d\n%s\n%s\n\n", i+1, Local[i].Score, Local[i].PlayerName, Local[i].PlayerInfo);
			}
			fclose(fp);
		}
	}
}


int	GetLocalHighScore(int run, int rank)
// Gets the rank'th high score from the local high-score table
// for the specified run.
{
	if (rank == 1 && run >= 0 && run < MAX_RUNS && Local[run].Score != -1) {
		return Local[run].Score;
	}
	
	return 0;
}


const char*	GetLocalHighPlayer(int run, int rank)
// Gets the player who has the rank'th high score from the local high-score table
// on the specified run.
{
	if (rank == 1 && run >= 0 && run < MAX_RUNS && Local[run].Score != -1) {
		return Local[run].PlayerName;
	}
	
	return 0;
}


#if 0
int	GetWorldHighScore(int run, int rank)
// Gets the rank'th high score from the world high-score table
// for the specified run.
{
	if (rank == 1 && run >= 0 && run < MAX_RUNS && World[run].Score != -1) {
		return World[run].Score;
	}
	
	return 0;
}
#endif // 0


#if 0
const char*	GetWorldHighPlayer(int run, int rank)
// Gets the player who has the rank'th high score from the world
// high-score table on the specified run.
{
	if (rank == 1 && run >= 0 && run < MAX_RUNS && World[run].Score != -1) {
		return World[run].PlayerName;
	}
	
	return 0;
}
#endif // 0


} // end namespace HighScore

