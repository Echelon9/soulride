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
// usercamera.hpp	-thatcher 1/1/1999 Copyright Slingshot

// This module controls the user's view of the world.


#ifndef USERCAMERA_HPP
#define USERCAMERA_HPP


#include "model.hpp"


namespace UserCamera {

	enum AimMode {
		LOOK_AT = 0,
		LOOK_THROUGH,
		FIRST_PERSON,
		FLY_AIM,

		AIM_MODE_COUNT
	};

	enum MotionMode {
		FOLLOW = 0,
		CHASE,
		FIXED,
		FULCRUM,
		PATH,
		FLY,

		MOTION_MODE_COUNT
	};

	void	SetAimMode(AimMode m);
	void	SetMotionMode(MotionMode m);

	void	SetSubject(MOriented* o);
	void	SetLocation(const vec3& loc);
	void	SetChaseParams(const vec3& offset, float VelDirOffset, float DirOffset, float FixedDirOffset, float Height, float Angle);
	void	SetFlyParams();//xxxxx
	bool	GetDoneFlying();	//xxxx
	
	// Auto-camera stuff.
	enum AutoMode {
		AUTO_OFF = 0,
		AUTO_CHASE_CLOSE,
		AUTO_CHASE_FAR,
		AUTO_FIXED,

		AUTO_MODE_COUNT
	};
	void	RandomAutoCameraMode();
	void	SetAutoCameraMode(AutoMode m);
	void	AutoCameraUpdate(const UpdateState& u);
};


#endif // USERCAMERA_HPP

