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
// music.hpp	-thatcher 9/24/1999 Copyright Slingshot Game Technology

// Interface to the background-music manager.


#ifndef MUSIC_HPP
#define MUSIC_HPP


#include "gameloop.hpp"
#include "view.hpp"


namespace Music {
	void	Open();
	void	Close();
	void	Update(const UpdateState& u);
	void	Render(const ViewState& s);

	void	FadeUp();
	void	FadeDown();
	void	FadeOut();

	void	SetMaxVolume(uint8 vol);
	void	SetMinVolume(uint8 vol);

	// sequence policy (consecutive, random, ...)
	// next track, prev track, stop, pause, resume, eject, ....

	// Callbacks from the OS, to notify the sound module to check
	// the state of the CD player.  The point is to avoid having to
	// poll the player all the time to see if something's changed,
	// which can be incredibly slow.
	void	CDCheckEntry();
	void	CDNotify();
};


#endif // MUSIC_HPP

