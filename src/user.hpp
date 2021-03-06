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
// user.hpp	-thatcher 1/7/1999 Copyright Slingshot

// Defines UserType, an interface base class for user objects.


#ifndef USER_HPP
#define USER_HPP


class UserType {
public:
	virtual ~UserType() {}
	virtual void	SetActive(bool active) = 0;
//	virtual void	SetControlsActive(bool active) = 0;

	virtual int	GetResumeCheckpointTicks() = 0;
	virtual int	ResumeFromCheckpoint() = 0;	// Returns the ticks value at the time of the checkpoint.
	virtual void	TimestampCheckpoints(int Ticks) = 0;
	
	// xxx whatever other methods would be specific to the user.
};


#endif // USER_HPP

