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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

// multiplayer.hpp	-Bjorn Leffler 18.10.2003

// Header for Multiplayer module, which coordinates 1-4 local players on the same screen.


#ifndef MULTIPLAYER_HPP
#define MULTIPLAYER_HPP

namespace MultiPlayer {

	const int MAX_PLAYERS = 4; // players on the same screen, not using networking

	int NumberOfLocalPlayers();

	void SetNumberOfLocalPlayers(int number);

	// Player 1-4 are indexed 0-3

	int CurrentPlayerIndex();

	void SetCurrentPlayerIndex(int index);

};


#endif // MULTIPLAYER_HPP
