/*
    Copyright 2003 Bjorn Leffler

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

#include "multiplayer.hpp"

namespace MultiPlayer
{

	int number_of_local_players = 1;
	int current_player_index = 0;

	int NumberOfLocalPlayers(){
		return number_of_local_players;
	}

	void SetNumberOfLocalPlayers(int number){
		number_of_local_players = number;
	}
  
	int CurrentPlayerIndex(){
		return current_player_index;
	}

	void SetCurrentPlayerIndex(int index){
		current_player_index = index;
	}

}
