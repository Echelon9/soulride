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
// boarder.hpp	-thatcher 7/11/1999 Copyright Slingshot Game Technology

// Some definitions required for interfaces between boarder physics,
// boarder controllers, and boarder figure.


#ifndef BOARDER_HPP
#define BOARDER_HPP


#include "geometry.hpp"
#include "gameloop.hpp"
#include "model.hpp"
#include "view.hpp"


// Define some interfaces that we need for the boarder figure model.
class BoarderModel : virtual public GArticulated {
public:
	virtual vec3	GetHandLocation(int hand) = 0;
	virtual void	PaintShadow(bitmap32* dest, float DestWidth, const vec3& light_direction, const vec3& shadow_up, bool DrawFeet) = 0;
};


namespace Boarder {
	// The different degrees of freedom in the boarder figure.
	enum DOF_ID {
		ANKLE_ANGLE = 0,	// i.e. edging angle of the board
		BOARD_YAW,	// Angle about y axis, relative to hips.
		BOARD_PITCH,
		LEGS_X,	// Offset of avg feet, in hips' left/right axis.
		LEGS_Y,	// Avg distance of feet from hips.
		LEGS_Z,	// Offset of avg feet, in hips' front/back axis.
		BACK_TWIST_ANGLE,	// about vertical axis
		ARM_L_EXTENSION,	// Left arm extension.
		ARM_R_EXTENSION,	// Right arm extension.
		ARM_L_VERT_ANGLE,	// up/down angle of the left arm.
		ARM_R_VERT_ANGLE,	// up/down angle of the right arm.
		ARM_L_FORWARD_ANGLE,	// forward/back angle of the left arm.
		ARM_R_FORWARD_ANGLE,	// forward/back angle of the right arm.
		BACK_LEAN_ANGLE,	// left/right, w/r/t front of boarder's body
		BACK_PITCH_ANGLE,	// front/back, w/r/t front of boarder's body

		// These don't affect the figure appearance; they're used by the physics/controller interface.
		DOF_RENDERING_COUNT,
//		LEG_FORCE_LEFT,		// Force applied by left leg.
//		LEG_FORCE_RIGHT,	// Force applied by right leg.
		LEFT_LEG_MAX_EXT = DOF_RENDERING_COUNT,	// Maximum extension of the left leg.
		RIGHT_LEG_MAX_EXT,
		LEFT_LEG_FORCE,	// force exerted by left leg.
		RIGHT_LEG_FORCE,

		FORCE_X,		// "Cheat" values from controller to physics.  Encodes the desired torque and
		FORCE_Y,		// force values (in world coords) to apply to the figure.
		FORCE_Z,
		TORQUE_X,
		TORQUE_Y,
		TORQUE_Z,
		
		DOF_COUNT
	};

	// Defined limits for each DOF.
	struct DOFLimit {
		float	Min, Max, MaxRateOfChange;
	};
	extern DOFLimit	DOFLimits[DOF_COUNT];

	// This is a class for holding degree-of-freedom commands.  It behaves like a float
	// except that it also has a weight value.  The weight value is used to assist in
	// combining commands.
	struct	DOFCommand {
		float	f, Weight;
//		bool	Valid;

		DOFCommand() { f = 0; Weight = 0; }
		operator float() const { return f; }
		DOFCommand&	operator=(float value) { f = value; if (Weight == 0) Weight = 1; return *this; }
		DOFCommand&	operator+=(float value) { f += value; if (Weight == 0) Weight = 1; return *this; }
		DOFCommand&	operator-=(float value) { return (*this += -value); }
//		void	SetValid(bool v) { Valid = v; }
		void	SetWeight(float w) { Weight = w; }
		float	GetWeight() const { return Weight; }
	};
	

	struct FootStatus {
		int	SurfaceType;	// -1 indicates that the foot isn't touching the ground.
		vec3	Location, ForceLocation;
		vec3	BoardNormal, SurfaceNormal;
		float	BoardForceCoeff;
		float	LegForce;
		bool	HardContact;

		FootStatus() {
			SurfaceType = -1;
			Location = ZeroVector;
			ForceLocation = ZeroVector;
			BoardNormal = YAxis;
			SurfaceNormal = YAxis;
			BoardForceCoeff = 0;
			LegForce = 0;
			HardContact = false;
		}
	};

	struct DynamicState {
		float	InverseMass;
		matrix33	InverseInertiaTensor;
		vec3	Velocity;
		vec3	Omega;
		vec3	Accel;
		vec3	DerivOmega;
		
		vec3	BoardAxis;
		vec3	BoardRight;
		vec3	BoardUp;

		bool	Crashing;
		// vec3	AngMomentum;
		// matrix33	InverseInertiaMoment;
		// whatever else might be helpful
	};
	
	class BoarderController {
	public:
		virtual void	Reset() = 0;
		virtual void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
				       DOFCommand FigureCommandsOut[DOF_COUNT]) = 0;
		virtual int	GetCheckpointBytes();
		virtual int	EncodeCheckpoint(uint8* buf, int index);
		virtual int	DecodeCheckpoint(uint8* buf, int index);

		virtual ~BoarderController() {}
	};

	BoarderController*	NewController(/* type? */);

	void	Clear();	// call this when closing down a level.
	void	RenderShadow(const ViewState& s);
};


#endif // BOARDER_HPP
