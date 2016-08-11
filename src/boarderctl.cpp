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
// boarderctl.cpp	-thatcher 8/6/1999 Copyright Slingshot Game Technology

// Code for snowboarder controllers.  A "controller" takes information
// about user input and the boarder state and outputs commands for the
// articulated figure degrees-of-freedom and leg forces, in order to
// keep the guy standing up and moving down the mountain in an
// appropriate way.

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif

#include <math.h>
#include "terrain.hpp"
#include "boarder.hpp"
#include "utility.hpp"
#include "config.hpp"
#include "text.hpp"
#include "console.hpp"
#include "game.hpp"
#include "sound.hpp"
#include "ui.hpp"


using namespace Boarder;


static float	scurve(float f, float power)
// Maps [-1..1] to [-1..1], using a curve.
// If power == 1, mapping is unity.
// If power < 1, curve is steeper in the middle (s-shape).
// If power > 1, curve is steeper towards extremes.
{
	if (f < 0) {
		return -powf(-f, power);
	} else {
		return powf(f, power);
	}
}


static float	DeadZone(float in, float zone)
// Maps the input, which should be in the range -1 to 1, to
// the return value, also in the range -1 to 1, but with a dead zone
// in the middle from -zone to zone which maps to 0.
{
	if (in < -zone) {
		// Input is below the dead zone.
		return (in + zone) / (1 - zone);
	} else if (in < zone) {
		// Input is in the dead zone.
		return 0;
	} else {
		// Input is above the dead zone.
		return (in - zone) / (1 - zone);
	}
}




// Some file-global variables to facilitate communication between sub-controllers.
static float	TargetNormSpeed = 0;
static float	TargetPitchSpeed = 0;
static float	TargetYawSpeed = 0;
static float	EstimatedFreeFallTime = 0;


// Some mouse configuration.
bool	MouseSteering = true;
float	MouseDivisor = 40.0;


// BoarderController default implementation.
int	BoarderController::GetCheckpointBytes() { return 0; }
int	BoarderController::EncodeCheckpoint(uint8* buf, int index) { return 0; }
int	BoarderController::DecodeCheckpoint(uint8* buf, int index) { return 0; }


// Controller which deals with leg extension for jumps and landings.
class LegExtension : public BoarderController {
	int	WindingUpTime;
	int	PushTimer;
	float	NormTarget;
	float	StabilityCrouch;
public:
	LegExtension() { Reset(); }
	void	Reset()
	{
		WindingUpTime = 0;
		PushTimer = 0;
		NormTarget = 0;
		StabilityCrouch = 0;
	}

	int	GetCheckpointBytes() { return 5 * 4 + 2 * 2; }
	int	EncodeCheckpoint(uint8* buf, int index) {
		int	index0 = index;
		index += EncodeFloat32(buf, index, TargetNormSpeed);
		index += EncodeFloat32(buf, index, TargetPitchSpeed);
		index += EncodeFloat32(buf, index, TargetYawSpeed);
		index += EncodeFloat32(buf, index, NormTarget);
		index += EncodeFloat32(buf, index, StabilityCrouch);
		index += EncodeUInt16(buf, index, WindingUpTime);
		index += EncodeUInt16(buf, index, PushTimer);
		return index - index0;
	}
	int	DecodeCheckpoint(uint8* buf, int index) {
		int	index0 = index;
		index += DecodeFloat32(buf, index, &TargetNormSpeed);
		index += DecodeFloat32(buf, index, &TargetPitchSpeed);
		index += DecodeFloat32(buf, index, &TargetYawSpeed);
		index += DecodeFloat32(buf, index, &NormTarget);
		index += DecodeFloat32(buf, index, &StabilityCrouch);
		uint16	temp;
		index += DecodeUInt16(buf, index, &temp); WindingUpTime = temp;
		index += DecodeUInt16(buf, index, &temp); PushTimer = temp;
		return index - index0;
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// Tries to implement jumps and stuff like that by effecting Target[LEGS_Y].
	{
		float	Tilt = u.Inputs.Tilt;
		if (MouseSteering) Tilt += u.Inputs.MouseSpeedX / MouseDivisor;
		Tilt = fclamp(-1, Tilt, 1);

		float	Pitch = u.Inputs.Pitch;
		if (MouseSteering) Pitch += -u.Inputs.MouseSpeedY / MouseDivisor;
		Pitch = fclamp(-1, Pitch, 1);

		if (d.Crashing) {
			Target[LEGS_Y] = DOFLimits[LEGS_Y].Max * 0.90f;
			return;
		}
		
		// If the user is pressing button 0, it means they're winding up for a jump.
		if (u.Inputs.Button[0].State || u.Inputs.MouseButtons & 1) {
			if (WindingUpTime == 0) WindingUpTime = 1;
			WindingUpTime += u.DeltaTicks;
			PushTimer = 0;	//xxx
		} else {
			if (WindingUpTime) {
				// User has just released button 0.  Try to jump.
				NormTarget = fclamp(0.6f, WindingUpTime / 400.0f, 2.5f);	// Vert speed dependent on wind-up time.
				WindingUpTime = 0;
				PushTimer = 500;

				TargetPitchSpeed = 0;
				if (Pitch < -0.5) TargetPitchSpeed = 4 * PI;
				else if (Pitch > 0.5) TargetPitchSpeed = -4 * PI;
				
				TargetYawSpeed = DeadZone(-Tilt, 0.1f) * 1 * (2 * PI);
			}
		}
		
		vec3	ObjZ;
		o->GetMatrix().ApplyRotation(&ObjZ, ZAxis);
		float	PitchSpeed = d.Omega * ObjZ;
		float	PitchError = Target[BOARD_PITCH] - FigureState[BOARD_PITCH];
		
		float	crouch = fclamp(0, fabsf(-PitchError - PitchSpeed * 0.8f) * 0.4f + (1 - fabsf(Tilt)) * 0.10f, 1) * 0.25f;
		if (Foot[0].SurfaceType < 0 || Foot[1].SurfaceType < 0) crouch = 0;
		float	c0 = expf(-u.DeltaT / 0.10f);
		StabilityCrouch = StabilityCrouch * c0 + crouch * (1 - c0);

		float	ext;
		if (PushTimer) {
			PushTimer = imax(0, PushTimer - u.DeltaTicks);
			ext = fclamp(0, 0.85f - StabilityCrouch + PushTimer / 1300.0f, 1);
			TargetNormSpeed = NormTarget * fmin(1.0f, PushTimer / 250.0f);
		} else {
			ext = 0.90f - fclamp(0, fclamp(0, WindingUpTime / 300.0f, 1) * 0.22f + StabilityCrouch, 0.25f);
			TargetNormSpeed = 0;
			TargetPitchSpeed = 0;
			TargetYawSpeed = 0;
		}
		
		Target[LEGS_Y] = DOFLimits[LEGS_Y].Max * ext;
	}
};


// Compute the force and torque we desire, for executing ideal turns.
class CheatTurn : public BoarderController {
public:
	CheatTurn()
	// Initialize stuff.
	{
		Reset();
	}
	void	Reset()
	// Reset internal state.
	{
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// o, FigureState and FootStatus give the state of the boarder.  This function sets the
	// values of Commands[] and FootOut[] according to the internal control algorithms.
	//
	// This control function tries for a nice, smooth, efficient turn, at a rate controlled by
	// the input value u.Tilt.
	{
		if (d.Crashing) return;
		
		float	Tilt = u.Inputs.Tilt;
		if (MouseSteering) Tilt += u.Inputs.MouseSpeedX / MouseDivisor;
		
		float	Turn = fclamp(-1, Tilt, 1);
		
		vec3	BoardCenter = (Foot[0].Location + Foot[1].Location) * 0.5;

		bool	Airborne = false;
		vec3	AvgNorm = o->GetUp();
		if (Foot[0].SurfaceType >= 0 && Foot[1].SurfaceType >= 0) {
			AvgNorm = Foot[0].SurfaceNormal + Foot[1].SurfaceNormal;
			AvgNorm.normalize();
		} else {
			// We're airborne.  Sample the ground under the board in order to make some reasonable values
			// for the board pitch and the AvgNorm vec3.
			Airborne = true;
		}

		vec3	veldir = o->GetVelocity();
		float	Speed = veldir.magnitude();
		veldir /= fmax(0.1f, Speed);

		vec3	vdflat = veldir;
		vdflat -= AvgNorm * (veldir * AvgNorm);
		vdflat /= fmax(0.1f, vdflat.magnitude());
		float	FlatSpeed = vdflat * o->GetVelocity();
		vec3	vdright = vdflat.cross(AvgNorm);


		// Compute errors.
		vec3	right = d.BoardAxis.cross(AvgNorm);
		right.normalize();

		// Compute yaw error.
		float	DesiredYaw = PI/2 * fclamp(-1, scurve(Turn, 10) * 0.50f, 1);
		float	Yaw = -asinf(fclamp(-1, vdflat * right, 1));
		float	YawError = DesiredYaw - Yaw;
		float	YawSpeed = TargetYawSpeed - d.Omega * AvgNorm;

		float	temp = AvgNorm * d.BoardAxis.cross(YAxis).normalize();
		float	SlopeOffsetRoll = asinf(fclamp(-1, temp, 1));
		float	DesiredRoll = fclamp(-PI/2 + SlopeOffsetRoll * 1.5f, scurve(Turn, 0.5f) * (0.10f + Speed/28 /* * Speed / 700*/), PI/2 - SlopeOffsetRoll * 1.5f);

		vec3	COMDir = (o->GetLocation() - BoardCenter);
		COMDir.normalize();
		
		vec3	rr = d.BoardAxis.cross(COMDir).normalize();
		float	Roll = asinf(fclamp(-1, -rr.Y(), 1));
		
		vec3	RollAxis = d.BoardAxis;
		
		float	RollError = sinf(DesiredRoll) - sinf(Roll);

		float	DesiredRollSpeed = 0; //RollError * -0.2;
		float	RollSpeed = d.Omega * RollAxis;
		float	RollSpeedError = DesiredRollSpeed - RollSpeed;

		// Angle of COM to slope, w/r/t board axis.
		float	Theta = asinf(fclamp(-1, d.BoardAxis.cross(AvgNorm) * COMDir, 1));
		
		//
		// Compute desired force & torque
		//

		const float	SIDE_CUT_RADIUS = 10;//xxxxxx
		
		float	Sign = Roll > 0 ? 1.0f : -1.0f;
		float	Rt = SIDE_CUT_RADIUS * cosf(Theta) / fmax(0.01f, cosf(Yaw));
		if (Rt < 2) Rt = 2;
		float	turn_force = Sign * (1.0f / d.InverseMass) * powf(FlatSpeed, 2) / Rt /* * fabsf(d.BoardAxis * veldir)*/;

		float	DesiredYawSpeed = Sign * FlatSpeed / Rt;
		float	YawSpeedError = DesiredYawSpeed - YawSpeed;
		
		float	brake_force = /* Slip * */ fabsf(sinf(Roll)) * fabsf(sinf(Yaw)) * 1/d.InverseMass * 40.0f;	// avgnormalforce, surface params
		float	speed_factor = fmin(1.0f, powf(FlatSpeed / 4, 2));
		vec3	force = vdright * turn_force + veldir * -brake_force;
		vec3	torque = RollAxis * (RollError * 2500 + RollSpeedError * 500) * speed_factor +
				 AvgNorm * (YawError * -1500 + YawSpeedError * -250) * speed_factor;
//		float	tmag = torque.magnitude();
//		if (tmag > 10000) torque *= 10000 / tmag;

		Target[FORCE_X] = force.X();
		Target[FORCE_Y] = force.Y();
		Target[FORCE_Z] = force.Z();

		Target[TORQUE_X] = torque.X();
		Target[TORQUE_Y] = torque.Y();
		Target[TORQUE_Z] = torque.Z();

		//
		// Secondary motion: move the figure a little in response to the error values.
		//
		
		
		{
			if (0 && Config::GetBoolValue("LastUpdateBeforeRender")) {
				char	buf[80];

				Text::FormatNumber(buf, Turn, 2, 2);
				Text::DrawString(50, 30, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 30, Text::FIXEDSYS, Text::ALIGN_RIGHT, "t = ", 0xFF000000);

				Text::FormatNumber(buf, Rt, 2, 2);
				Text::DrawString(50, 40, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 40, Text::FIXEDSYS, Text::ALIGN_RIGHT, "Rt = ", 0xFF000000);

				Text::FormatNumber(buf, Roll, 2, 2);
				Text::DrawString(50, 50, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 50, Text::FIXEDSYS, Text::ALIGN_RIGHT, "r = ", 0xFF000000);
				
				Text::FormatNumber(buf, RollError, 2, 2);
				Text::DrawString(50, 60, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 60, Text::FIXEDSYS, Text::ALIGN_RIGHT, "re = ", 0xFF000000);
				
				Text::FormatNumber(buf, Yaw, 2, 2);
				Text::DrawString(50, 70, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 70, Text::FIXEDSYS, Text::ALIGN_RIGHT, "y = ", 0xFF000000);

				Text::FormatNumber(buf, YawError, 2, 2);
				Text::DrawString(50, 80, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 80, Text::FIXEDSYS, Text::ALIGN_RIGHT, "ye = ", 0xFF000000);

			}
		}

	}
};




// Controller which tries to do smooth turns with minimal slippage.
class CarvedTurn : public BoarderController {
//	float	LastTilt;
//	float	FilteredDerivTilt;
public:
	CarvedTurn()
	// Initialize stuff.
	{
		Reset();
	}
	void	Reset()
	// Reset internal state.
	{
//		LastTilt = 0;
//		FilteredDerivTilt = 0;
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// o, FigureState and FootStatus give the state of the boarder.  This function sets the
	// values of Commands[] and FootOut[] according to the internal control algorithms.
	//
	// This control function tries for a nice, smooth, efficient turn, at a rate controlled by
	// the input value u.Tilt.
	{
		if (d.Crashing) return;
		
		float	Tilt = u.Inputs.Tilt;
		if (MouseSteering) Tilt += u.Inputs.MouseSpeedX / MouseDivisor;
//		LastTilt = Tilt;
		
		float	Turn = fclamp(-1, Tilt * 1.6f, 1);
		
		Target[BACK_TWIST_ANGLE] = 1.45f /* depends on goofiness */ - (Turn < 0 ? 0.20f : 0.30f) * Turn;
		
		vec3	BoardCenter = (Foot[0].Location + Foot[1].Location) * 0.5f;

		bool	Airborne = false;
		vec3	AvgNorm = o->GetUp();
		if (Foot[0].SurfaceType >= 0 && Foot[1].SurfaceType >= 0) {
			Target[BOARD_PITCH] = 0 /*FigureState[BOARD_PITCH]*/;
			AvgNorm = Foot[0].SurfaceNormal + Foot[1].SurfaceNormal;
			AvgNorm.normalize();
		} else {
			// We're airborne.  Sample the ground under the board in order to make some reasonable values
			// for the board pitch and the AvgNorm vec3.
			Airborne = true;

			// First, guess where we're going to land.
			float	DeltaY = TerrainModel::GetHeight(BoardCenter) - BoardCenter.Y();
			float	t_imp =	0;
			vec3	vel = o->GetVelocity();
			if (fabsf(vel.Y()) > 0.1) {
				t_imp = DeltaY / vel.Y();	// Estimate a time of impact.
			} else {
				t_imp = DeltaY / 1;
			}
			vec3	ContactGuess = BoardCenter + vel * t_imp;

			AvgNorm = TerrainModel::GetNormal(ContactGuess);

			// Now figure out the pitch angle that will match the ground plane angle.
			vec3	BoardAxis = Geometry::Rotate(FigureState[BOARD_YAW], YAxis, XAxis);
			vec3	n;
			o->GetMatrix().ApplyInverseRotation(&n, AvgNorm);	// Convert ground norm into object coords.
			float	pitch = -asinf(fclamp(-1, n * BoardAxis, 1)) + 0.05f /*xxxx*/;
			float	blend = fclamp(0.2f, 1.3f - fabsf(t_imp) * 0.5f, 1.0f);
			
			Target[BOARD_PITCH] = pitch * blend + 0.05f * (1 - blend);
		}

		vec3	veldir = o->GetVelocity();
		float	Speed = veldir.magnitude();
		veldir /= fmax(0.1f, Speed);

		vec3	vdflat = veldir;
		vdflat -= AvgNorm * (veldir * AvgNorm);
		vdflat.normalize();

		vec3	vdright = vdflat.cross(AvgNorm);


		// Compute errors.
		vec3	right = d.BoardAxis.cross(AvgNorm);
		right.normalize();

		// Compute yaw error.
		float	DesiredYaw = fclamp(-PI/2, Turn * PI/2 * 0.15f, PI/2);
		float	Yaw = -asinf(fclamp(-1, veldir * right, 1));
		float	YawSpeed = 0 - d.Omega * AvgNorm;
		
		// Roll error.
		float	SlopeOffsetRoll = asinf(fclamp(-1, AvgNorm * d.BoardAxis.cross(YAxis).normalize(), 1));
		SlopeOffsetRoll = 0.00;//xxxxxxx
		float	DesiredRoll = fclamp(-PI/2 + SlopeOffsetRoll * 1.5f, Turn * (0.06f + Speed/26 /* * Speed / 700*/), PI/2 + SlopeOffsetRoll * 1.5f);
		vec3	UpNormalToAxis = d.BoardAxis.cross(YAxis).cross(d.BoardAxis);
		UpNormalToAxis.normalize();
		
		vec3	COMDir = (o->GetLocation() - BoardCenter);
		COMDir.normalize();
		float	rblend = scurve(fclamp(0, fabsf(YAxis.cross(AvgNorm) * vdflat) * 2, 1), 1);
		float	Roll = (1 - rblend) * asinf(fclamp(-1, AvgNorm.cross(COMDir) * d.BoardAxis, 1)) +
			       rblend * asinf(fclamp(-1, YAxis.cross(COMDir) * veldir, 1));

		float	RollError = sinf(DesiredRoll) - sinf(Roll);

		float	DesiredRollSpeed = RollError * -0.2f;
		float	RollSpeed = d.Omega * d.BoardAxis;
		float	RollSpeedError = DesiredRollSpeed - RollSpeed;

		float	angulation_factor = fclamp(-1, (fabsf(Roll) - PI/5.5f) * -5, 1);
		angulation_factor = 1;
		
		float	desired_theta = Roll + 0.2f * fclamp(-1, (RollError * 0.05f + RollSpeedError * 0.08f) * -2, 1);


		float	limit = 0.75f * Target[LEGS_Y] / 0.765f;
		float	LeanGain = fclamp(0, 1 - fabsf(Roll / (PI/4) * 0.5f), 1);
		float	sideways_lean = fclamp(-1, (RollError * 0.20f + RollSpeedError * 0.10f) * -10, 1);
		sideways_lean = limit * (1.00f * sideways_lean + 0.00f * scurve(sideways_lean, 2)) * angulation_factor;

		
		float	desired_edge_angle = fclamp(
			-1,
			(FigureState[BOARD_YAW] * -0.40f + YawSpeed * -0.00f + RollError * 0.18f + RollSpeedError * 0.06f) * 10,
			1);	// YawError... YawSpeedError...
		desired_edge_angle = 0.50f * desired_edge_angle + 0.00f * scurve(desired_edge_angle, 2);
		vec3	desired_edge_dir = Geometry::Rotate(desired_edge_angle, AvgNorm, vdflat);


		float	Sign = 1;
		if ((Yaw < 0 && DesiredYaw < Yaw) || (Yaw > 0 && DesiredYaw > Yaw)) {
			Sign = 1;
		} else {
			Sign = -1;
		}
		float	ye = fclamp(-1, RollError * 1.5f + RollSpeedError * 0.6f, 1);
		float	forward_lean = -0.10f + 0.18f * fabsf(ye) * Sign /*+ 0.10f * fabsf(ye) * Sign*/;

		if (0 && Airborne) {
			desired_edge_dir = (vdflat + YAxis * 0.1f).normalize();

			forward_lean = fclamp(-0.3f, COMDir * vdflat * 0.7f, 0.3f);
			sideways_lean = fclamp(-0.5f, COMDir * vdright * 0.7f, 0.5f);
			
			desired_theta = 0;
		}

		
		vec3	desired_edge_loc = vdright * sideways_lean + vdflat * (forward_lean);
		
		// compute ANKLE_ANGLE, LEGS_Z, BACK_PITCH, BOARD_PITCH, BOARD_YAW
		// Transform angles and locations into body-relative coords.
		vec3	v = desired_edge_loc;
		o->GetMatrix().ApplyInverseRotation(&desired_edge_loc, v);
		desired_edge_loc.SetY(desired_edge_loc.Y() - FigureState[LEGS_Y]);

		v = desired_edge_dir;
		o->GetMatrix().ApplyInverseRotation(&desired_edge_dir, v);

		// compute BOARD_PITCH, BOARD_YAW based on desired_edge_dir.
//		if (1 || !Airborne) {
			Target[BOARD_YAW] = atan2f(-desired_edge_dir.Z(), desired_edge_dir.X());
			Target[BOARD_PITCH] = asinf(fclamp(-1, -desired_edge_dir.Y(), 1)) /* * (o->GetUp() * AvgNorm)*/;
//			Target[BOARD_PITCH] = forward_lean * -1.5;
//		}

		// compute LEGS_Z, LEGS_X, BACK_PITCH & BACK_LEAN based on desired_edge_loc.
		// xxxx some kind of iterative IK...
		Target[LEGS_Z] = desired_edge_loc.Z() * LeanGain;
		Target[LEGS_X] = desired_edge_loc.X();
		// BACK_PITCH_ANGLE, BACK_LEAN_ANGLE?  Can be used to move the COM.  Also affects angulation.
		Target[BACK_PITCH_ANGLE] = desired_edge_loc.Z() * -1.0f;
		Target[BACK_LEAN_ANGLE] = -0.1f + desired_edge_loc.X() * 1.0f;

		
		Target[ANKLE_ANGLE] = -0.15f * fclamp(-1, (desired_theta - Roll), 1);

		{
			if (0 && Config::GetBoolValue("LastUpdateBeforeRender")) {
				char	buf[80];

				Text::FormatNumber(buf, Roll, 2, 2);
				Text::DrawString(50, 30, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 30, Text::FIXEDSYS, Text::ALIGN_RIGHT, "r = ", 0xFF000000);
				
				Text::FormatNumber(buf, RollError, 2, 2);
				Text::DrawString(50, 40, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 40, Text::FIXEDSYS, Text::ALIGN_RIGHT, "re = ", 0xFF000000);
				
				Text::FormatNumber(buf, RollSpeedError, 2, 2);
				Text::DrawString(50, 50, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 50, Text::FIXEDSYS, Text::ALIGN_RIGHT, "rse = ", 0xFF000000);

				Text::FormatNumber(buf, SlopeOffsetRoll, 2, 2);
				Text::DrawString(50, 60, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 60, Text::FIXEDSYS, Text::ALIGN_RIGHT, "sor = ", 0xFF000000);
			}
		}


	}
};



//xxxx
const int	GM_X = 4;
const int	GM_Y = 4;
float	GainMatrix[GM_Y][GM_X] = {
//	{ -0.30f, -0.25f, -1.00f, -0.30f },
	{ -0.00f, -0.00f, -0.00f, -0.00f },
//	{  0.45f,  0.10f,  1.05f,  0.20f },
	{  0.15f,  0.05f,  0.05f,  0.10f },
	{  0.15f,  0.10f,  0.00f,  0.00f },
	{  0.00f,  0.06f,  0.25f,  0.40f },
};
	


// Controller which tries to do controlled skidding.
class SkidTurn : public BoarderController {
//	float	LastTilt;
//	float	FilteredDerivTilt;

public:
	SkidTurn()
	// Initialize stuff.
	{
		Reset();
	}
	
	void	Reset()
	// Reset internal state.
	{
//		LastTilt = 0;
//		FilteredDerivTilt = 0;
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// o, FigureState and FootStatus give the state of the boarder.  This function sets the
	// values of Commands[] and FootOut[] according to the internal control algorithms.
	{
		if (d.Crashing) return;	// Give up during crash.
		
		float	Tilt = u.Inputs.Tilt;
		if (MouseSteering) Tilt += u.Inputs.MouseSpeedX / MouseDivisor;
		float	Turn = fclamp(-1, Tilt, 1);
		
		
		Target[BACK_TWIST_ANGLE] = 1.05f /* depends on goofiness */ - (Turn < 0 ? 0.15f : 0.25f) * Turn;
		
		vec3	BoardCenter = (Foot[0].Location + Foot[1].Location) * 0.5;
		
		vec3	AvgNorm = o->GetUp();
		if (Foot[0].SurfaceType >= 0 && Foot[1].SurfaceType >= 0) {
			AvgNorm = Foot[0].SurfaceNormal + Foot[1].SurfaceNormal;
			AvgNorm.normalize();
		} else if (Foot[0].SurfaceType >= 0) {
			AvgNorm = Foot[0].SurfaceNormal;
		} else if (Foot[1].SurfaceType >= 0) {
			AvgNorm = Foot[1].SurfaceNormal;
		}
		
		float	Speed = o->GetVelocity().magnitude();
		vec3	veldir = o->GetVelocity();
		veldir /= fmax(0.1f, Speed);

		vec3	falldir = AvgNorm.cross(AvgNorm.cross(YAxis));	// Points along fall-line.
		falldir.normalize();
		float	fallfactor = 1 - fclamp(0, (Speed - 3) / 3, 1);
		vec3	forward_dir = veldir * (1 - fallfactor) + falldir * fallfactor;
		forward_dir.normalize();

		// Compute errors.
		vec3	right = d.BoardAxis.cross(YAxis);
		right.normalize();

		// Compute yaw error.
		float	DesiredYaw = fclamp(-PI/2, Turn * PI/2 * 0.35f, PI/2);
		float	Yaw = -asinf(fclamp(-1, forward_dir/*veldir*/ * right, 1));
		float	YawError = DesiredYaw - Yaw;
		float	YawSpeed = 0 - d.Omega * AvgNorm;
//		float	DesiredYawSpeed = fclamp(-1, Turn * fclamp(0, (80 - Speed) / 360, 2), 1) * 3;
		float	DesiredYawSpeed = 0;
		float	YawSpeedError = DesiredYawSpeed - YawSpeed;
		
		// Roll error.
		float	DesiredRoll = fclamp(-PI/2, Turn * fclamp(0, Speed * Speed / 600, 1) * PI/2, PI/2);
		float	Roll = asinf(fclamp(-1, -o->GetRight().Y(), 1));
		float	RollError = DesiredRoll - Roll;

		float	DesiredRollSpeed = 0 /* RollError * 0.2 */;
		float	RollSpeed = d.Omega * d.BoardAxis;
		float	RollSpeedError = DesiredRollSpeed - RollSpeed;


		//xxxxxxxxx Gain Matrix manipulation UI
		if (0) {
			static int	gmx = 0;
			static int	gmy = 0;
			int	i, j;

			char	buf[80];

			// Process input.
			Input::GetAlphaInput(buf, 80);
			char*	p = buf;
			while (*p) {
				if (*p == '[') { gmx--; }
				if (*p == ']') { gmx++; }

				if (gmx < 0) { gmx += GM_X; gmy--; }
				if (gmx >= GM_X) { gmx -= GM_X; gmy++; }
				if (gmy < 0) { gmy += GM_Y; }
				if (gmy >= GM_Y) { gmy -= GM_Y; }
				
				if (*p == '-') { GainMatrix[gmy][gmx] -= 0.01f; }
				if (*p == '=') { GainMatrix[gmy][gmx] += 0.01f; }
				if (*p == '_') { GainMatrix[gmy][gmx] -= 0.10f; }
				if (*p == '+') { GainMatrix[gmy][gmx] += 0.10f; }

				if (*p == 'Z') { for (j = 0; j < GM_Y; j++) { for (i = 0; i < GM_X; i++) { GainMatrix[j][i] = 0; } } }
				if (*p == 'S') {
					FILE*	fp = fopen("mat.tmp", "wb");
					for (j = 0; j < GM_Y; j++) { for (i = 0; i < GM_X; i++) { WriteFloat(fp, GainMatrix[j][i]); } }
					fclose(fp);
				}
				if (*p == 'L') {
					FILE*	fp = fopen("mat.tmp", "rb");
					for (j = 0; j < GM_Y; j++) { for (i = 0; i < GM_X; i++) { GainMatrix[j][i] = ReadFloat(fp); } }
					fclose(fp);
				}

				if (*p == '0') { Config::SetIntValue("DebugSteer", 0); }
				if (*p == '1') { Config::SetIntValue("DebugSteer", 1); }
				if (*p == '2') { Config::SetIntValue("DebugSteer", 2); }
				
				p++;
			}
			
			
			if (1 && Config::GetBoolValue("LastUpdateBeforeRender")) {
				char	buf[80];
				
//				Text::FormatNumber(buf, Applicability, 2, 2);
//				Text::DrawString(160, 10, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
//				Text::DrawString(160, 10, Text::FIXEDSYS, Text::ALIGN_RIGHT, "ap = ", 0xFF000000);
				
				Text::FormatNumber(buf, Roll, 2, 2);
				Text::DrawString(50, 10, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 10, Text::FIXEDSYS, Text::ALIGN_RIGHT, "r = ", 0xFF000000);
				
				Text::FormatNumber(buf, YawError, 2, 2);
				Text::DrawString(50, 20, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 20, Text::FIXEDSYS, Text::ALIGN_RIGHT, "ye = ", 0xFF000000);
				
				Text::FormatNumber(buf, YawSpeedError, 2, 2);
				Text::DrawString(50, 30, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 30, Text::FIXEDSYS, Text::ALIGN_RIGHT, "yse = ", 0xFF000000);
				
				Text::FormatNumber(buf, RollError, 2, 2);
				Text::DrawString(50, 40, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 40, Text::FIXEDSYS, Text::ALIGN_RIGHT, "re = ", 0xFF000000);
				
				Text::FormatNumber(buf, RollSpeedError, 2, 2);
				Text::DrawString(50, 50, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
				Text::DrawString(50, 50, Text::FIXEDSYS, Text::ALIGN_RIGHT, "rse = ", 0xFF000000);

				// Show gain matrix.
				for (j = 0; j < GM_Y; j++) {
					for (i = 0; i < GM_X; i++) {
						Text::FormatNumber(buf, GainMatrix[j][i], 2, 2);
						Text::DrawString(50 + 40*i, 70 + 10*j, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, (i==gmx && j==gmy) ? 0xFFFF0000 : 0xFF000000);
					}
				}
			}
		}
		//xxxxxxxxx
		
		
		// Shift weight in order to counter/effect roll.
		float	limit = 0.45f * Target[LEGS_Y] / 0.765f;
		float	LeanGain = fclamp(0.2f, 1 - fabsf(Roll / (PI/2) * 1.5f), 1);
		Target[LEGS_Z] = fclamp(-limit,
					(YawError * GainMatrix[0][0] +
					 YawSpeedError * GainMatrix[0][1] +
					 RollError * GainMatrix[0][2] +
					 RollSpeedError * GainMatrix[0][3]) * LeanGain, limit);

		// Yaw the board to effect turning.
		float	SpeedFactor = 1;
		if (Speed > 1) {
			SpeedFactor = (1 / Speed);
		}
		SpeedFactor = fclamp(0, SpeedFactor * 5, 1);
		
		Target[BOARD_YAW] = 0
			+ 0.45f * SpeedFactor * fclamp(
				-1,
				(
					YawError * GainMatrix[1][0] +
					YawSpeedError * GainMatrix[1][1] +
					RollError * GainMatrix[1][2] +
					RollSpeedError * GainMatrix[1][3]
					) * 2,
				1);
		
		// Axial weight-shift.  xxxx Direction of this should depend on regular vs. goofy
		float	Sign = 1;
		if ((Yaw < 0 && DesiredYaw < Yaw) || (Yaw > 0 && DesiredYaw > Yaw)) {
			Sign = 1;
		} else {
			Sign = -1;
		}
		float	ye = fclamp(-1,
				    YawError * GainMatrix[2][0] +
				    YawSpeedError * GainMatrix[2][1] +
				    RollError * GainMatrix[2][2] +
				    RollSpeedError * GainMatrix[2][3]
				    , 1);
		float	Lean = //-0.05 * (1 - fabsf(ye))
			      //+ 0.14 * fabsf(ye) * Sign
			      -0.10f * fabsf(ye) * Sign
//			      -0.10
			       ;
		Lean += FigureState[LEGS_Y] * tanf(FigureState[BOARD_PITCH]);	// pitch compensate
		Target[LEGS_X] = Lean;
		
		// Adjust edging to counter roll.
		float	FlatAngle = asinf(fclamp(-1, AvgNorm * d.BoardRight, 1));	// Angle which would lay the board flat.
		Target[ANKLE_ANGLE] = 0; // FlatAngle / 4;
		Target[ANKLE_ANGLE] +=
			fclamp(-0.3f, FlatAngle, 0.3f)
			+ -0.2f * fclamp(
				-1,
				(YawError * GainMatrix[3][0] +
				 YawSpeedError * GainMatrix[3][1] +
				 RollError * GainMatrix[3][2] +
				 RollSpeedError * GainMatrix[3][3]
					) * 1.8f,
				1)
			;
	}
};



class StableLegs : public BoarderController {
//	float	IntegralError;
public:
	StableLegs()
	// Initialize stuff.
	{
		Reset();
	}
	void	Reset()
	// Reset internal state.
	{
//		IntegralError = 0;
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// This controller just affects the leg forces of the figure, to try to keep him perpendicular to the
	// ground and at a reasonable height.
	{
		if (d.Crashing) {
			Target[LEFT_LEG_MAX_EXT] = DOFLimits[LEGS_Y].Max * 0.8f; //Target[LEGS_Y];
			Target[RIGHT_LEG_MAX_EXT] = DOFLimits[LEGS_Y].Max * 0.8f; //Target[LEGS_Y];
			Target[LEFT_LEG_FORCE] = 200;
			Target[RIGHT_LEG_FORCE] = 200;
			return;
		}
		
		int	i;

		vec3	AvgNorm = o->GetUp();
		if (Foot[0].SurfaceType >= 0 || Foot[1].SurfaceType >= 0) {
			AvgNorm = Foot[0].SurfaceNormal * (Foot[0].SurfaceType >= 0 ? 1.0f : 0.0f)
				  + Foot[1].SurfaceNormal * (Foot[1].SurfaceType >= 0 ? 1.0f : 0.0f);
			AvgNorm.normalize();
		}

		float	NormSpeed = AvgNorm * o->GetVelocity();

//		vec3	ObjZ;
//		o->GetMatrix().ApplyRotation(&ObjZ, ZAxis);
		
		float	PitchSpeed = d.Omega *
				     d.BoardRight
//				     ObjZ
//				     o->GetRight()
				     ;
		float	PitchError = FigureState[BOARD_PITCH]
				     - Target[BOARD_PITCH]
				     ;
		
		float	mag[2];
		float	SumError = 0;
		for (i = 0; i < 2; i++) {
			vec3	v;
			o->GetMatrix().ApplyInverse(&v, Foot[i].Location);
			
			float	error = (Target[LEGS_Y] + (i ? 1 : -1) * 0.7f * sinf(Target[BOARD_PITCH])) + v.Y();
			float	errorgain = 1;
			if (error > 0.20f && error < 2.0f) errorgain = expf((error - 0.20f) * 2);
			float	deriverror = TargetNormSpeed - NormSpeed;
			float	angleerror = PitchError * (i ? 1 : -1);
			float	pitchspeederror = (TargetPitchSpeed - PitchSpeed) * (i ? 1 : -1);
			mag[i] = error * errorgain * 2000 +
				 deriverror * errorgain * 750 +
//				 IntegralError * 000 +
				 fclamp(-1500, angleerror * -1000, 1500) +
				 pitchspeederror * -1200;
			if (mag[i] < 0) mag[i] = 0;

			if (Foot[i].SurfaceType != -1) {
				SumError += error;
			} else {
//				SumError += 0.10;//xxxx
			}
		}

		// Limit the sum of leg forces, but preserve the differential as much as possible.
		const float	MAX_LEGS_FORCE = 5000;
		float	diff = fclamp(-MAX_LEGS_FORCE, mag[0] - mag[1], MAX_LEGS_FORCE);
		if (mag[0] + mag[1] > MAX_LEGS_FORCE) {
			mag[0] = (MAX_LEGS_FORCE + diff) / 2;
			mag[1] = (MAX_LEGS_FORCE - diff) / 2;
		}

		for (i = 0; i < 2; i++) {
			float	 offset = sinf(PitchError) * 0.7f;
			if (i == 0) offset = -offset;
			
			Target[LEFT_LEG_MAX_EXT + i] = Target[LEGS_Y] + sinf(Target[BOARD_PITCH]) * 0.7f * (i ? 1 : -1) /* + 0.10 */;
			Target[LEFT_LEG_FORCE + i] = mag[i];
		}
	}
};


//
// This controller animates the arms & back angle to simulate bouncing due to accelerations.
//
class ArmBounce : public BoarderController {
	float	Crouch, CrouchSpeed;
public:
	ArmBounce()
	// Initialize stuff.
	{
		Reset();
	}
	void	Reset()
	// Reset internal state.
	{
		Crouch = -0.5;
		CrouchSpeed = 0;
	}

	int	GetCheckpointBytes() { return 2 * 4; }
	int	EncodeCheckpoint(uint8* buf, int index) {
		int	index0 = index;
		index += EncodeFloat32(buf, index, Crouch);
		index += EncodeFloat32(buf, index, CrouchSpeed);
		return index - index0;
	}
	int	DecodeCheckpoint(uint8* buf, int index) {
		int	index0 = index;
		index += DecodeFloat32(buf, index, &Crouch);
		index += DecodeFloat32(buf, index, &CrouchSpeed);
		return index - index0;
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// Compute a vertical acceleration for the arms/torso, and use
	// it to animate the arms and back angles.
	{
		float	DesiredCrouch = ((Target[LEGS_Y] - 0.5f) / 0.5f /* - 1 */) * 0.5f;

		CrouchSpeed += (-d.Accel.Y() - CrouchSpeed * 6 + (DesiredCrouch - Crouch) * 65) * u.DeltaT;
		Crouch = fclamp(-1, Crouch + CrouchSpeed * u.DeltaT, 1.0);

		Target[ARM_L_VERT_ANGLE] = 1.2f * (Crouch - 0.9f);
		Target[ARM_R_VERT_ANGLE] = 1.2f * (Crouch - 0.9f);

		Target[ARM_L_FORWARD_ANGLE] = 0;
		Target[ARM_R_FORWARD_ANGLE] = 0;
		
		Target[ARM_L_EXTENSION] = 0.35f * fclamp(-1, Crouch, 1) + 0.45f;
		Target[ARM_R_EXTENSION] = 0.35f * fclamp(-1, Crouch, 1) + 0.45f;
	}
};



//
// This controller is for moving the figure while in free-fall, to optimize the landing.
//
class FreeFall : public BoarderController {
public:
	FreeFall()
	// Initialize stuff.
	{
		Reset();
	}
	void	Reset()
	// Reset internal state.
	{
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// Detect when we're in the air, and crank our controller weights.
	// Actuate the figure to prepare for good landings.
	{
		float	Tilt = u.Inputs.Tilt;
		if (MouseSteering) Tilt += u.Inputs.MouseSpeedX / MouseDivisor;
		Tilt = fclamp(-1, Tilt, 1);

		EstimatedFreeFallTime = 0;
		
		if (Foot[0].SurfaceType == -1 && Foot[1].SurfaceType == -1) {
			// We're airborne.  Do some projections to decide when
			// and on what slope we'll probably land.

			// First, guess where we're going to land.
			// do a crude forward dynamics sim to get bd contact time, loc, normal, orientation, norm vel
			float	BoardY = (Foot[0].Location.Y() + Foot[1].Location.Y()) * 0.5f;
			float	BoardYOffset = fabsf(o->GetLocation().Y() - BoardY);
			
			vec3	loc = o->GetLocation();
			vec3	vel = d.Velocity;
			float	t_impact = 0;
			vec3	norm = YAxis;
			int	repcount = 0;
			const float	TIMESTEP = 0.033f;
			float	prev_dy = loc.Y() - BoardYOffset - TerrainModel::GetHeight(loc);

			do {
				// Advance the simulation.
				vec3	newvel = vel + YAxis * -9.8f * TIMESTEP;
				vec3	avgvel = (vel + newvel) * 0.5f;
				loc += avgvel * TIMESTEP;
				vel = newvel;
				t_impact += TIMESTEP;

				// Check the ground.
				float	dy = loc.Y() - BoardYOffset - TerrainModel::GetHeight(loc);
				norm = TerrainModel::GetNormal(loc);

				if (dy <= 0 && vel * norm < 0) {
					// Hit the ground.  Estimate exact location and time.
					float	sum_dy = dy - prev_dy;
					float	f = 1.0;
					if (sum_dy > 0.001) {
						f = -prev_dy / sum_dy;
					}
					f = fclamp(0, f, 1);
					loc -= avgvel * TIMESTEP * f;
					t_impact -= TIMESTEP * f;
					break;
				}

				prev_dy = dy;
				repcount++;
			} while (repcount < 5.0 / TIMESTEP);

			EstimatedFreeFallTime = t_impact;	// Expose free-fall calc to following controllers.

			// Effect a good balanced landing.
			float	LandWeight = 1 - fclamp(0, t_impact / 1.0f, 1);
			if (LandWeight > 0) {
				vec3	veldir = vel;
				float	Speed = veldir.magnitude();
				veldir /= fmax(0.1f, Speed);
				
				vec3	vdflat = veldir;
				vdflat -= norm * (veldir * norm);
				vdflat.normalize();
				
				vec3	vdright = vdflat.cross(norm);

				vec3	BoardCenter = (Foot[0].Location + Foot[1].Location) * 0.5;
				vec3	COMDir = (o->GetLocation() - BoardCenter);
				COMDir.normalize();
				
				// Align board center with vel, and point straight ahead.
				vec3	desired_edge_dir = (vdflat + YAxis * 0.1f).normalize();
				// Yaw slightly according to steering input.
				desired_edge_dir = Geometry::Rotate(Tilt * -0.8f, norm, desired_edge_dir);
				
				float	forward_lean = fclamp(-0.3f, veldir * vdflat * 0.7f, 0.3f);
				float	sideways_lean = fclamp(-0.5f, Tilt * -0.3f + veldir * vdright * 0.7f, 0.5f);
				
				vec3	desired_edge_loc = vdright * sideways_lean + vdflat * (forward_lean);
				
				// compute ANKLE_ANGLE, LEGS_Z, BACK_PITCH, BOARD_PITCH, BOARD_YAW
				// Transform angles and locations into body-relative coords.
				vec3	v = desired_edge_loc;
				o->GetMatrix().ApplyInverseRotation(&desired_edge_loc, v);
				desired_edge_loc.SetY(desired_edge_loc.Y() - FigureState[LEGS_Y]);
				
				v = desired_edge_dir;
				o->GetMatrix().ApplyInverseRotation(&desired_edge_dir, v);
				
				// compute BOARD_PITCH, BOARD_YAW based on desired_edge_dir.
				Target[BOARD_YAW] = /*LandWeight * */atan2f(-desired_edge_dir.Z(), desired_edge_dir.X());
				Target[BOARD_YAW].SetWeight(LandWeight);
				Target[BOARD_PITCH] = /*LandWeight * */asinf(fclamp(-1, -desired_edge_dir.Y(), 1)) /* * (o->GetUp() * AvgNorm)*/;
				Target[BOARD_PITCH].SetWeight(LandWeight);
				
				// compute LEGS_Z, LEGS_X, BACK_PITCH & BACK_LEAN based on desired_edge_loc.
				// xxxx some kind of iterative IK...
				Target[LEGS_Z] = /*LandWeight * */desired_edge_loc.Z();
				Target[LEGS_Z].SetWeight(LandWeight);
				Target[LEGS_X] = /*LandWeight * */desired_edge_loc.X();
				Target[LEGS_X].SetWeight(LandWeight);
				// BACK_PITCH_ANGLE, BACK_LEAN_ANGLE?  Can be used to move the COM.  Also affects angulation.
//				Target[BACK_PITCH_ANGLE] = LandWeight * desired_edge_loc.Z() * -0.3;
//				Target[BACK_PITCH_ANGLE].SetWeight(AirWeight);
				Target[BACK_LEAN_ANGLE] = LandWeight * (-0.1f + desired_edge_loc.X() * 1.0f);
				Target[BACK_LEAN_ANGLE].SetWeight(LandWeight);
				
				// Effect crouch inv. proportional to expected normal vel.  I.e. stretch out in anticipation of hitting hard.
			}
			
//			// Now figure out the pitch angle that will match the ground plane angle.
//			vec3	BoardAxis = Geometry::Rotate(FigureState[BOARD_YAW], YAxis, XAxis);
//			vec3	n;
//			o->GetMatrix().ApplyInverseRotation(&n, AvgNorm);	// Convert ground norm into object coords.
//			float	pitch = -asinf(n * BoardAxis) + 0.05/*xxxx*/;
//			float	blend = fclamp(0.2, 1.3 - fabsf(t_imp) * 0.5, 1.0);
//			
//			Target[BOARD_PITCH] = pitch * blend + 0.05 * (1 - blend);
		}

		
	}
};



//
// This controller is for posing and scoring the figure while in free-fall.
//

const int	MAX_SOUNDS = 10;


class FreeFallPose : public BoarderController {
	int	AirborneTimer;
	int	AirBonus;
	int	BonusTimer;

	int	HeelGrabHoldoff;
	int	ToeGrabHoldoff;
	int	TipGrabHoldoff;
	int	TailGrabHoldoff;

	vec3	VelDirFlat;
	int	RotationBonus;
	float	RotationAngle;

	int	PitchBonus;
	int	PitchCount;

public:
	FreeFallPose()
	// Initialize stuff.
	{
		Reset();
	}
	void	Reset()
	// Reset internal state.
	{
		AirborneTimer = 0;
		AirBonus = 0;
		BonusTimer = 0;
		
		HeelGrabHoldoff = ToeGrabHoldoff = 0;
		TipGrabHoldoff = TailGrabHoldoff = 0;

		VelDirFlat = XAxis;	// Direction of motion in horizontal plane, at point of leaving the ground.

		RotationBonus = 0;
//		RotationCount = 0;
		RotationAngle = 0;
		PitchBonus = PitchCount = 0;
	}

	int	GetCheckpointBytes() { return 10 * 2 + 4 * 4; }
	int	EncodeCheckpoint(uint8* buf, int index)
	// Take state info and encode it in the given buffer.
	{
		int	index0 = index;
		index += EncodeUInt16(buf, index, AirborneTimer);
		index += EncodeUInt16(buf, index, AirBonus);
		index += EncodeUInt16(buf, index, BonusTimer);
		index += EncodeUInt16(buf, index, HeelGrabHoldoff);
		index += EncodeUInt16(buf, index, ToeGrabHoldoff);
		index += EncodeUInt16(buf, index, TipGrabHoldoff);
		index += EncodeUInt16(buf, index, TailGrabHoldoff);
		index += EncodeFloat32(buf, index, VelDirFlat.X());
		index += EncodeFloat32(buf, index, VelDirFlat.Y());
		index += EncodeFloat32(buf, index, VelDirFlat.Z());
		index += EncodeFloat32(buf, index, RotationAngle);
		index += EncodeUInt16(buf, index, RotationBonus);
//		index += EncodeUInt16(buf, index, RotationCount);
		index += EncodeUInt16(buf, index, PitchBonus);
		index += EncodeUInt16(buf, index, PitchCount);
		return index - index0;
	}
	
	int	DecodeCheckpoint(uint8* buf, int index)
	// Set our internal state according to the data in the given buffer.
	{
		int	index0 = index;
		uint16	temp;
		index += DecodeUInt16(buf, index, &temp); AirborneTimer = temp;
		index += DecodeUInt16(buf, index, &temp); AirBonus = temp;
		index += DecodeUInt16(buf, index, &temp); BonusTimer = temp;
		index += DecodeUInt16(buf, index, &temp); HeelGrabHoldoff = temp;
		index += DecodeUInt16(buf, index, &temp); ToeGrabHoldoff = temp;
		index += DecodeUInt16(buf, index, &temp); TipGrabHoldoff = temp;
		index += DecodeUInt16(buf, index, &temp); TailGrabHoldoff = temp;
		float	f[3];
		index += DecodeFloat32(buf, index, &f[0]);
		index += DecodeFloat32(buf, index, &f[1]);
		index += DecodeFloat32(buf, index, &f[2]);
		index += DecodeFloat32(buf, index, &RotationAngle);
		VelDirFlat = vec3(f[0], f[1], f[2]);
		index += DecodeUInt16(buf, index, &temp); RotationBonus = temp;
//		index += DecodeUInt16(buf, index, &temp); RotationCount = (int) (int16) temp;
		index += DecodeUInt16(buf, index, &temp); PitchBonus = temp;
		index += DecodeUInt16(buf, index, &temp); PitchCount = (int) (int16) temp;
		return index - index0;
	}


	void	AwardAirBonus()
	{
		if (AirBonus) {
			// Award the air bonus.
			static const int	MSG_CT = 4;
			static struct {
				const char*	msg_id;
				const char*	default_msg;
			}
			msg[MSG_CT] = {
				{ "air_bonus", "air" },
				{ "nice_air_bonus", "nice air" },
				{ "big_air_bonus", "big air" },
				{ "tremendous_air_bonus", "tremendous air" },
			};
			static const char*	snd[MSG_CT] = {
				"ding1.wav",
				"ding2.wav",
				"ding3.wav",
				"ding4.wav",
			};

			int	index = iclamp(0, AirBonus-1, MSG_CT-1);

//			static const uint32	ctab[MSG_CT] = {
//				0xFFBFBF66,
//				0xFFD8CC4C,
//				0xFFFFE519,
//				0xFFFFE519,
//			};
//			int	color = ctab[iclamp(0, AirBonus-1, MSG_CT-1)];
			
			int	points = 50 * (1 << (AirBonus - 1));
			char	buf[80];
			sprintf(buf, "%s +%d", UI::String(msg[index].msg_id, msg[index].default_msg), points);
			Game::AddBonusMessage(buf, points, snd[index], Sound::Controls());
			
			AirBonus = 0;
		}

		if (RotationBonus) {
			// Award rotation bonus.
			int	points = 50 * RotationBonus * (1 << iclamp(0, (RotationBonus)>>1, 2));
			char	buf[80];
			sprintf(buf, "%d %s +%d", RotationBonus * 180, UI::String("degrees_bonus", "degrees"), points);
			
			Game::AddBonusMessage(buf, points, "deedoodeedoo.wav", Sound::Controls(1, 0, 1, false));
	
			RotationBonus = 0;
		}

		if (PitchBonus) {
			// Award pitch rotation bonus.
			int	points = 200 * PitchBonus * (1 << iclamp(0, (PitchBonus)>>1, 2));
			char	buf[80];
			sprintf(buf, "%s x%d +%d", UI::String("flip_bonus", "flip"), PitchBonus, points);
			
			Game::AddBonusMessage(buf, points, "doodooding.wav", Sound::Controls(1, 0, 1, false));

			PitchBonus = 0;
		}
	}
	
		
	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// Detect when we're in the air, and crank our controller weights.
	// Actuate the figure to reach certain poses.
	{
		float	Tilt = u.Inputs.Tilt;
		if (MouseSteering) Tilt += u.Inputs.MouseSpeedX / MouseDivisor;
		Tilt = fclamp(-1, Tilt, 1);

		float	Pitch = u.Inputs.Pitch;
		if (MouseSteering) Pitch += -u.Inputs.MouseSpeedY / MouseDivisor;
		Pitch = fclamp(-1, Pitch, 1);

		// Watch airborne state.  Increment a timer while we're airborne.
		bool	Airborne = Foot[0].SurfaceType == -1 && Foot[1].SurfaceType == -1;
		Config::SetBool("Airborne", Airborne);//xxxxxx

		if (Airborne && AirborneTimer == 0) {
			// We just left the ground.  Remember our flat velocity dir.
			VelDirFlat = o->GetVelocity();
			VelDirFlat.SetY(0);
			VelDirFlat.normalize();

			// Measure our current spin-angle w/r/t our
			// velocity direction and use it as the initial
			// total.
			const vec3&	dir = o->GetDirection();
			const vec3&	up = o->GetUp();
			if (up.Y() > 0.2) {
				vec3	flatright = VelDirFlat.cross(YAxis);
				RotationAngle = atan2f(-(dir * flatright), dir * VelDirFlat);
			}
		}
		
		if (Airborne && !d.Crashing) {
			// Award any pending air bonus.
			if (BonusTimer) {
				AwardAirBonus();
				AirborneTimer = 0;
//				RotationCount = 0;
				RotationAngle = 0;
				PitchCount = 0;
			}
			BonusTimer = 0;
			
			AirborneTimer += u.DeltaTicks;
			AirBonus = AirborneTimer / 500;

			// Check rotation.
			const vec3&	up = o->GetUp();
			const vec3	flatright = VelDirFlat.cross(YAxis);
			
			RotationAngle += u.DeltaT * (up * d.Omega);
			static const float	THRESH = 45 / 180.0 * PI;
			RotationBonus = fchop((fabsf(RotationAngle + FigureState[BOARD_YAW]) + THRESH) / PI);

			// Check pitch rotation.
			if (fabsf(up * flatright) < 0.85) {
				float	angle = atan2f(-(up * VelDirFlat), up.Y());
				const vec3&	L = o->GetAngMomentum();
				static const float	THRESH = 15 / 180 * PI;
				if (L * flatright > 0) {
					// ccw rotation.
					if (PitchCount & 1) {
						if (angle > -THRESH && angle < -THRESH + PI/2) {
							PitchCount++;
						}
					} else {
						if (angle > PI - THRESH || angle < -PI/2 - THRESH) {
							PitchCount++;
						}
					}
				} else {
					// cw rotation.
					if (PitchCount & 1) {
						if (angle < THRESH && angle > THRESH - PI/2) {
							PitchCount--;
						}
					} else {
						if (angle < -PI + THRESH || angle > PI/2 + THRESH) {
							PitchCount--;
						}
					}
				}

			}

			PitchBonus = (abs(PitchCount) + 1) >> 1;
		}

		if (!Airborne) {
			// If we just landed, set a timer to delay the awarding of points,
			// until the player is safe.
			if (AirborneTimer && (AirBonus || RotationBonus || PitchBonus)) {
				BonusTimer = 400;

//				//xxxxxxxx
//				char	buf[80];
//				sprintf(buf, "angle = %g", RotationAngle * 180 / PI);
//				Game::AddBonusMessage(buf);
//				//xxxxxxxxxx
			}

			// Cancel air bonus if we crash soon after landing.
			if (d.Crashing) {
				AirBonus = 0;
				RotationBonus = 0;
				PitchBonus = 0;
				BonusTimer = 0;
			}

			if (BonusTimer) {
				BonusTimer -= u.DeltaTicks;
				if (BonusTimer <= 0) {
					BonusTimer = 0;

					AwardAirBonus();
				}
			}
					
			AirborneTimer = 0;
//			RotationCount = 0;
			RotationAngle = 0;
			PitchCount = 0;
		}


		Config::SetFloat("EstimatedFreeFallTime", EstimatedFreeFallTime);//xxxxxxx

		float	Weight = fclamp(0, (EstimatedFreeFallTime - 0.2f) * 5, 1);
		Weight *= 10;

		if (Weight <= 0) {
			// Don't pose during normal riding on the ground.			
//			HeelGrabTicks = ToeGrabTicks = 0;
			HeelGrabHoldoff = ToeGrabHoldoff = 0;
			TipGrabHoldoff = TailGrabHoldoff = 0;
			return;
		}

		// Decrement holdoff timers.
		if (HeelGrabHoldoff) {
			HeelGrabHoldoff -= u.DeltaTicks;
			if (HeelGrabHoldoff < 0) HeelGrabHoldoff = 0;
		}
		if (ToeGrabHoldoff) {
			ToeGrabHoldoff -= u.DeltaTicks;
			if (ToeGrabHoldoff < 0) ToeGrabHoldoff = 0;
		}
		if (TipGrabHoldoff) {
			TipGrabHoldoff -= u.DeltaTicks;
			if (TipGrabHoldoff < 0) TipGrabHoldoff = 0;
		}
		if (TailGrabHoldoff) {
			TailGrabHoldoff -= u.DeltaTicks;
			if (TailGrabHoldoff < 0) TailGrabHoldoff = 0;
		}
		
		// Figure out what pose we're being asked to do, if any.
		int	GrabHand = 0;	// -1 == left, 0 == nothing, 1 == right
		int	BoardPitchDir = 0;	// -1 == down, 0 == nothing, 1 == up

		if (u.Inputs.Button[Input::BUTTON1].State || u.Inputs.MouseButtons & 2) {
			const float	POSE_TILT_THRESH = 0.3f;
			const float	POSE_PITCH_THRESH = 0.5f;
			
			if (Tilt < -POSE_TILT_THRESH) GrabHand = -1;
			else if (Tilt > POSE_TILT_THRESH) GrabHand = 1;

			if (Pitch < - POSE_PITCH_THRESH) BoardPitchDir = -1;
			else if (Pitch > POSE_PITCH_THRESH) BoardPitchDir = 1;
		}

		//
		// Execute the desired pose.
		//
		
		if (GrabHand) {
			// User wants a grab.  Figure out where the desired hand is now, and
			// where we want it to be.
			vec3	Hand = ZeroVector;
			
			int	index = GrabHand == -1 ? 0 : 1;
			BoarderModel*	b = dynamic_cast<BoarderModel*>(o->GetVisual());
			if (b) {
				Hand = b->GetHandLocation(index);
			}

			// Center of board, on left or right edge.
			vec3	HandTarget = (Foot[0].Location + Foot[1].Location) * 0.5;
			HandTarget += d.BoardRight * (GrabHand==-1 ? -0.08f : 0.15f);	// Not sure why it's asymetric -- may be a model bug.
			// Bring into object coordinates.
			vec3	v = HandTarget;
			o->GetMatrix().ApplyInverse(&HandTarget, v);

			// Now we have the actual location and target location in object coords.
			// Apply changes to the figure DOF targets to try to eventually reduce the error to 0.
			vec3	error = HandTarget - Hand;
			float	ErrorDistance = error.magnitude();

			if (GrabHand == -1) {
				// If it's a heel-side grab, we want to twist forward, arch our back and bend the knees back.

				Target[ARM_L_EXTENSION + index] = FigureState[ARM_L_EXTENSION + index] - fclamp(-1, error.Y()*20, 1) * 0.2f;
				Target[ARM_L_EXTENSION + index].SetWeight(Weight);
				Target[ARM_L_VERT_ANGLE + index] = FigureState[ARM_L_VERT_ANGLE + index] + GrabHand * fclamp(-1, error.Z()*20, 1) * 0.2f;
				Target[ARM_L_VERT_ANGLE + index].SetWeight(Weight);
				Target[ARM_L_FORWARD_ANGLE + index] = FigureState[ARM_L_FORWARD_ANGLE + index] + fclamp(-1, error.X()*20, 1) * 0.2f;
				Target[ARM_L_FORWARD_ANGLE + index].SetWeight(Weight);

				// Extend the opposite arm into space.
				Target[ARM_L_EXTENSION + (1 - index)] = 0.60f;
				Target[ARM_L_EXTENSION + (1 - index)].SetWeight(Weight);
				Target[ARM_L_VERT_ANGLE + (1 - index)] = 0.60f;
				Target[ARM_L_VERT_ANGLE + (1 - index)].SetWeight(Weight);

				Target[BACK_PITCH_ANGLE] = -PI / 4;
				Target[BACK_PITCH_ANGLE].SetWeight(Weight);
				Target[BACK_TWIST_ANGLE] = PI/2;
				Target[BACK_TWIST_ANGLE].SetWeight(Weight);
				Target[BACK_LEAN_ANGLE] = -1;
				Target[BACK_LEAN_ANGLE].SetWeight(Weight);

				Target[BOARD_YAW] = 0;
				Target[BOARD_YAW].SetWeight(Weight);
				Target[LEFT_LEG_MAX_EXT] = 0.1f;
				Target[LEFT_LEG_MAX_EXT].SetWeight(Weight);
				Target[RIGHT_LEG_MAX_EXT] = 0.1f;
				Target[RIGHT_LEG_MAX_EXT].SetWeight(Weight);
				Target[LEGS_Z] = -0.75;
				Target[LEGS_Z].SetWeight(Weight);
				Target[ANKLE_ANGLE] = 1.5;
				Target[ANKLE_ANGLE].SetWeight(Weight);

				if (ErrorDistance > 0.10 || HeelGrabHoldoff > 0) {
				} else {
					Game::AddBonusMessage(UI::String("heel_side_grab_bonus", "heel side grab +100"), 100, "ching0.wav", Sound::Controls(1, -1, 1, false));
					HeelGrabHoldoff = 1000;
				}
			} else {
				// Toe side grab... crouch down, lean forward, no twist.
				Target[ARM_L_EXTENSION + index] = FigureState[ARM_L_EXTENSION + index] - fclamp(-1, error.Y()*20, 1) * 0.2f;
				Target[ARM_L_EXTENSION + index].SetWeight(Weight);
				Target[ARM_L_VERT_ANGLE + index] = FigureState[ARM_L_VERT_ANGLE + index] + GrabHand * fclamp(-1, error.Z()*20, 1) * 0.2f;
				Target[ARM_L_VERT_ANGLE + index].SetWeight(Weight);
				Target[ARM_L_FORWARD_ANGLE + index] = FigureState[ARM_L_FORWARD_ANGLE + index] + fclamp(-1, error.X()*20, 1) * 0.2f;
				Target[ARM_L_FORWARD_ANGLE + index].SetWeight(Weight);

				// Extend the opposite arm into space.
				Target[ARM_L_EXTENSION + (1 - index)] = 0.60f;
				Target[ARM_L_EXTENSION + (1 - index)].SetWeight(Weight);
				Target[ARM_L_VERT_ANGLE + (1 - index)] = 1.0;
				Target[ARM_L_VERT_ANGLE + (1 - index)].SetWeight(Weight);

				Target[BACK_PITCH_ANGLE] = PI/3;
				Target[BACK_PITCH_ANGLE].SetWeight(Weight);
				Target[BACK_TWIST_ANGLE] = PI/4 /*FigureState[BACK_TWIST_ANGLE] + fclamp(-1, error.X()*20, 1) * 0.2*/;
				Target[BACK_TWIST_ANGLE].SetWeight(Weight);

				Target[BOARD_YAW] = 0;
				Target[BOARD_YAW].SetWeight(Weight);
				Target[LEFT_LEG_MAX_EXT] = 0.3f;
				Target[LEFT_LEG_MAX_EXT].SetWeight(Weight);
				Target[RIGHT_LEG_MAX_EXT] = 0.3f;
				Target[RIGHT_LEG_MAX_EXT].SetWeight(Weight);
				Target[LEGS_Z] = 0;
				Target[LEGS_Z].SetWeight(Weight);
				Target[ANKLE_ANGLE] = 0;
				Target[ANKLE_ANGLE].SetWeight(Weight);

				if (ErrorDistance > 0.10 || ToeGrabHoldoff > 0) {
				} else {
					Game::AddBonusMessage(UI::String("toe_side_grab_bonus", "toe side grab +100"), 100, "ching0.wav", Sound::Controls(1, 1, 1, false));
					ToeGrabHoldoff = 1000;
				}
			}
				
		} else if (BoardPitchDir) {
			// User wants a tip or tail grab.  Figure out where the desired hand is now, and
			// where we want it to be.
			vec3	Hand = ZeroVector;
			
			int	index = BoardPitchDir == 1 ? 0 : 1;
			BoarderModel*	b = dynamic_cast<BoarderModel*>(o->GetVisual());
			if (b) {
				Hand = b->GetHandLocation(index);
			}

			// Tip or tail of board.
			vec3	HandTarget = (Foot[0].Location + Foot[1].Location) * 0.5f;
			HandTarget += d.BoardAxis * float(BoardPitchDir);
			// Bring into object coordinates.
			vec3	v = HandTarget;
			o->GetMatrix().ApplyInverse(&HandTarget, v);

			// Now we have the actual location and target location in object coords.
			// Apply changes to the figure DOF targets to try to eventually reduce the error to 0.
			vec3	error = HandTarget - Hand;
			float	ErrorDistance = error.magnitude();

			if (BoardPitchDir == -1) {
//				printf("bpd = %d\n", BoardPitchDir);//xxxxxx
				// Tail grab -- pitch the board down, and grab tail with right hand.

				Target[ARM_L_EXTENSION + index] = FigureState[ARM_L_EXTENSION + index] - fclamp(-1, error.X()*20, 1) * 0.2f;
				Target[ARM_L_EXTENSION + index].SetWeight(Weight);
				Target[ARM_L_VERT_ANGLE + index] = FigureState[ARM_L_VERT_ANGLE + index] + BoardPitchDir * fclamp(-1, error.Y()*20, 1) * -0.2f;
				Target[ARM_L_VERT_ANGLE + index].SetWeight(Weight);
				Target[ARM_L_FORWARD_ANGLE + index] = FigureState[ARM_L_FORWARD_ANGLE + index] + fclamp(-1, error.Z()*20, 1) * 0.2f;
				Target[ARM_L_FORWARD_ANGLE + index].SetWeight(Weight);

//				// Extend the opposite arm into space.
//				Target[ARM_L_EXTENSION + (1 - index)] = 0.60;
//				Target[ARM_L_EXTENSION + (1 - index)].SetWeight(Weight);
//				Target[ARM_L_VERT_ANGLE + (1 - index)] = 0.6;
//				Target[ARM_L_VERT_ANGLE + (1 - index)].SetWeight(Weight);

				Target[BACK_PITCH_ANGLE] = 1;
				Target[BACK_PITCH_ANGLE].SetWeight(Weight);
				Target[BACK_TWIST_ANGLE] = 0;
				Target[BACK_TWIST_ANGLE].SetWeight(Weight);
				Target[BACK_LEAN_ANGLE] = 1;
				Target[BACK_LEAN_ANGLE].SetWeight(Weight);

				Target[BOARD_YAW] = 0;
				Target[BOARD_YAW].SetWeight(Weight);
//				Target[LEFT_LEG_MAX_EXT] = 1;
//				Target[LEFT_LEG_MAX_EXT].SetWeight(Weight);
//				Target[RIGHT_LEG_MAX_EXT] = 1;
//				Target[RIGHT_LEG_MAX_EXT].SetWeight(Weight);
				Target[LEGS_X] = -0.2f;
				Target[LEGS_X].SetWeight(Weight);
				Target[LEGS_Y] = 0.7f;
				Target[LEGS_Y].SetWeight(Weight);
				Target[BOARD_PITCH] = 1;
				Target[BOARD_PITCH].SetWeight(1000000/*xxx*/);
				Target[LEGS_Z] = 0;
				Target[LEGS_Z].SetWeight(Weight);
				Target[ANKLE_ANGLE] = 0;
				Target[ANKLE_ANGLE].SetWeight(Weight);

				if (ErrorDistance > 0.10 || TailGrabHoldoff > 0) {
				} else {
					Game::AddBonusMessage(UI::String("tail_grab_bonus", "tail grab +100"), 100, "ching0.wav", Sound::Controls(1, -1, 1, false));
					TailGrabHoldoff = 1000;
				}
			} else {
			}
		}
	}
};



static void	CombineCommands(DOFCommand Results[DOF_COUNT], float AccumWeights[DOF_COUNT], float AccumValues[DOF_COUNT],
			DOFCommand Input[DOF_COUNT], float Weight)
// This function does a weighted add of a given set of input commands with a set of current outputs.
{
	int	i;
	
	for (i = 0; i < DOF_COUNT; i++) {
		// For a given DOF, update the sum of weights and the
		// sum of values.   Then divide the sum of values by
		// the sum of weights to get an average.

		DOFCommand&	c = Input[i];
		float	w = Weight * c.GetWeight();
		
//		if (c.Valid) {
			AccumValues[i] += float(c) * w;
			AccumWeights[i] += w;
//		}
		
		if (AccumWeights[i] > 0.0001) {
			Results[i] = AccumValues[i] / AccumWeights[i];
		}
	}
}


// Controller which is a composite of some other simpler controllers.
// The commands from the other controllers are combined using a weighted sum scheme.

const int COMPOSITE_COUNT = 7;


class CompositeController : public BoarderController {
	BoarderController*	Controller[COMPOSITE_COUNT];
public:
	CompositeController()
	// Initialize stuff.
	{
		Controller[0] = new LegExtension;
		Controller[1] = new CheatTurn;
		Controller[2] = new SkidTurn;		
		Controller[3] = new StableLegs;
		Controller[4] = new ArmBounce;
		Controller[5] = new FreeFall;
		Controller[6] = new FreeFallPose;
		
		Reset();
	}
	
	virtual ~CompositeController()
	{
		int	i;
		
		for (i = 0; i < COMPOSITE_COUNT; i++) {
			delete Controller[i];
		}
	}
	
	void	Reset()
	// Reset internal state.
	{
		int	i;
		
		for (i = 0; i < COMPOSITE_COUNT; i++) {
			Controller[i]->Reset();
		}
	}

	void	Update(MDynamic* o, const UpdateState& u, const DynamicState& d, const float FigureState[DOF_COUNT], const FootStatus Foot[2],
		       DOFCommand Target[DOF_COUNT])
	// This controller combines the outputs of several sub-controllers.  The different
	// sub-controllers are weighted according to what we think the user is trying to do.
	{
		int	i;

		//
		// Update some configuration.
		//
		MouseSteering =
			(Config::GetBool("MouseSteering")
			 && Config::GetBool("Fullscreen"))
			|| Config::GetBool("ForceMouseSteering");
		MouseDivisor = 5 * expf((10 - fclamp(1, Config::GetFloat("MouseSteeringSensitivity"), 10)) * (5.298f / 9.0f));

		//
		// First, figure out what the user is trying to do and choose weights for the various sub-controllers
		// accordingly.
		//
		float	ControllerWeight[COMPOSITE_COUNT];

		for (i = 0; i < COMPOSITE_COUNT; i++) {
			ControllerWeight[i] = 0;
		}

		// xxxx fixed weights for the moment.
		for (i = 0; i < COMPOSITE_COUNT; i++) {
			ControllerWeight[i] = 1;
		}

//		// Balance between skidding and carving controllers based on magnitude of the Turn input.
//		float	SkidWeight = powf(fclamp(0, (fabsf(u.Inputs.Tilt) - 0.25f) * 4, 1), 0.5f);
//		SkidWeight = 1;//xxx
//		ControllerWeight[1] = 1 - SkidWeight;
//		ControllerWeight[2] = SkidWeight;

		//
		// Now, combine the outputs of the controllers according to the computed weights.
		//
		float	Weights[DOF_COUNT];
		float	Values[DOF_COUNT];

		// Reset the accumulated weights and values.
		for (i = 0; i < DOF_COUNT; i++) {
			Weights[i] = 0;
			Values[i] = 0;
		}

		// Run the controllers and combine their outputs.
		DOFCommand	Temp[DOF_COUNT];
		for (i = 0; i < COMPOSITE_COUNT; i++) {
			
			// Copy the results so far into a temporary results array.
			for (int j = 0; j < DOF_COUNT; j++) {
				Temp[j] = Target[j];
				Temp[j].SetWeight(0);
			}
			Controller[i]->Update(o, u, d, FigureState, Foot, Temp);
			CombineCommands(Target, Weights, Values, Temp, ControllerWeight[i]);
		}
	}


	int	GetCheckpointBytes()
	// Return the number of bytes needed to checkpoint this controller.
	{
		int	bytes = 0;
		int	i;
		for (i = 0; i < COMPOSITE_COUNT; i++) {
			bytes += Controller[i]->GetCheckpointBytes();
		}
		return bytes;
	}

	int	EncodeCheckpoint(uint8* buf, int index)
	// Encode our state info into the given buffer, starting at index.  Return
	// index + the number of bytes used.
	{
		int	index0 = index;
		int	i;
		for (i = 0; i < COMPOSITE_COUNT; i++) {
			index += Controller[i]->EncodeCheckpoint(buf, index);
		}
		return index - index0;
	}

	int	DecodeCheckpoint(uint8* buf, int index)
	// Reset our state from the encoded data starting at buf[index].  Returns
	// index + the number of bytes read.
	{
		int	index0 = index;
		int	i;
		for (i = 0; i < COMPOSITE_COUNT; i++) {
			index += Controller[i]->DecodeCheckpoint(buf, index);
		}
		return index - index0;
	}
};



BoarderController*	Boarder::NewController()
// Create and return a controller.  At the moment there's only one type;
// perhaps in the future this call will be able to create different
// types.
{
	return new CompositeController;
}

