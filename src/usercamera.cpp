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
// usercamera.cpp	-thatcher 1/1/1999 Copyright Slingshot

// Code for controlling the user's view of the world.


#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#include "utility.hpp"
#include "usercamera.hpp"
#include "gameloop.hpp"
#include "terrain.hpp"
#include "game.hpp"


namespace UserCamera {
;


const float	CUT_THRESHOLD_DISTANCE = 30;
const float	CAMERA_MAX_SPEED = 200;


class CameraObject;
CameraObject*	Instance = NULL;
AimMode	amode = LOOK_AT;
MotionMode	mmode = CHASE;


// Camera parameters.
MOriented*	Subject = NULL;
MDynamic*	DynamicSubject = NULL;
vec3	ChaseOffset = ZeroVector;
float	ChaseVelDirOffset = -4;
float	ChaseDirOffset = 0;
float	ChaseFixedDirOffset = 0;
float	ChaseHeight = 1.8f;
float	ChaseAngle = 0;

float	FlyMaxSpeed = 500;
float	FlyAverageHeight = 100;

bool	DoneFlying = false;


class CameraObject : public MOriented {
private:
	float	RotateAngle;
	vec3	SubjectDir;

	vec3	Velocity;	// For flying mode.
	
public:
	CameraObject()
	{
		Subject = NULL;
		RotateAngle = -PI/1;
		SubjectDir = XAxis;

		Velocity = ZeroVector;
	}


	void	SetVelocity(const vec3& v) { Velocity = v; }
	

	void	Update(const UpdateState& u)
	// Update for the camera.  Compute the desired camera location and orientation based
	// on Motion and Aim modes and parameters.
	{
		if (Subject == NULL) return;

		//
		// Move the camera.
		//
		
		vec3	DesiredLoc = GetLocation();
		bool	NoSpeedLimit = false;

		float	FlyFactor = 0;
		vec3	FlyDir = XAxis;
		
		switch (mmode) {
		default:
		case FOLLOW:
		{
			// Fix the camera position to the subject position.
			DesiredLoc = Subject->GetLocation();
			NoSpeedLimit = true;

			break;
		}
		
		case CHASE:
		case FLY:
		{
			// Find desired location in relation to subject.
			vec3	v;
			Subject->GetMatrix().Apply(&v, ChaseOffset);
			
			vec3	tdir = Subject->GetDirection();
			tdir.SetY(0);
			float	DirFlatMag = tdir.magnitude();
			if (DirFlatMag < 0.2) {
				tdir = GetDirection();
			} else {
				tdir *= 1.0f / DirFlatMag;
			}
			tdir = Geometry::Rotate(ChaseAngle, YAxis, tdir);

			vec3	tveldir = XAxis /* tdir */;
			if (DynamicSubject) tveldir = DynamicSubject->GetVelocity();
			tveldir.SetY(0);
			float	FlatSpeed = tveldir.magnitude();
			if (FlatSpeed < 1.0) {
				tveldir = GetDirection();
				tveldir.SetY(0);
				tveldir.normalize();
			} else {
				tveldir *= 1.0f / FlatSpeed;
			}
			tveldir = Geometry::Rotate(ChaseAngle, YAxis, tveldir);
			
			v += tdir * ChaseDirOffset;
			v += tveldir * ChaseVelDirOffset;
			v += Geometry::Rotate(ChaseAngle, YAxis, vec3(ChaseFixedDirOffset, 0, 0));

			v.SetY(v.Y() + ChaseHeight);

			DesiredLoc = v;

			if (mmode == FLY) {
				// Accelerate towards desired location.
				vec3	loc = DesiredLoc;
				vec3	disp = loc - GetLocation();
				FlyDir = disp;
				FlyDir.normalize();

				disp.SetY(0);
				float	dist = disp.magnitude();
				disp.normalize();
				
				float	speed = fmin(10 + dist / 2, 1000.0f);
				
				DesiredLoc = GetLocation() + disp * fmin(dist, u.DeltaT * speed /*FlyMaxSpeed*/);

				FlyFactor = fclamp(0, dist / 50, 1);

				float	c0 = expf(-u.DeltaT / 0.5f);
				float	h = FlyAverageHeight * fclamp(0, sqrtf(dist) / 15, 1);
				float	y = fmax(TerrainModel::GetHeight(DesiredLoc), TerrainModel::GetHeight(DesiredLoc + disp * 2.0f * speed))
					    + h;
				y = y * FlyFactor + loc.Y() * (1 - FlyFactor);
				y = GetLocation().Y() * c0 + y * (1 - c0);
				DesiredLoc.SetY(y);

				if (dist < 1) {
					DoneFlying = true;
				}
			}
			
			break;
		}

		case FIXED:
		{
			// Desired location is just where we happen to be right now.
			break;
		}
		}

		// Take care of actual motion, using speed limit and cut-threshold.
		vec3	NewLoc = GetLocation();
		vec3	disp = DesiredLoc - GetLocation();
		float	dist = disp.magnitude();
		if (NoSpeedLimit || dist > CUT_THRESHOLD_DISTANCE) {
			// Cut directly to the desired spot.
			NewLoc = DesiredLoc;
		} else {
			// Move towards the desired spot.
			float	limit = CAMERA_MAX_SPEED * u.DeltaT;
			if (dist > limit) {
				disp *= (limit / dist);
			}
			NewLoc = GetLocation() + disp;
		}
		
		// Prevent camera from poking through the ground surface.
		if (mmode != FOLLOW) {
			float	y = TerrainModel::GetHeight(NewLoc);
			y = fmax(y, TerrainModel::GetHeight(NewLoc + XAxis));
			y = fmax(y, TerrainModel::GetHeight(NewLoc - XAxis));
			y = fmax(y, TerrainModel::GetHeight(NewLoc + ZAxis));
			y = fmax(y, TerrainModel::GetHeight(NewLoc - ZAxis));
			y += 0.75;
			if (y > NewLoc.Y()) NewLoc.SetY(y);
		}

		SetLocation(NewLoc);

		
		//
		// Aim the camera.
		//
		
		switch (amode) {
		default:
		case LOOK_AT:
		{
			// Aim at the subject.
			vec3	dir = Subject->GetLocation() - GetLocation();
			dir.normalize();

			// Now set the orientation based on desired direction, etc.
			SetDirection(dir);
			
			vec3	right = dir.cross(YAxis);	// Roll the right vec3 about DesiredDir...?
			vec3	up = right.cross(dir);
			up.normalize();
			SetUp(up);
		
			break;
		}

		case LOOK_THROUGH:
		{
			// Use the Subject's view orientation.
			SetDirection(Subject->GetDirection());
			SetUp(Subject->GetUp());
			
			break;
		}

		case FIRST_PERSON:
		{
			// Align view with subject's velocity, except when the view dir is very different than
			// vel, in which case align with dir.
			vec3	dir = Subject->GetDirection();
			vec3	vel = dir;
			float	f = 0;

			MDynamic*	d = dynamic_cast<MDynamic*>(Subject);
			if (d) {
				vel = d->GetVelocity();
				float	velmag = vel.magnitude();
				vel /= fmax(1.0f, velmag);
				f = fclamp(0.0f, ((vel * dir) - 0.5f) * 2, 1.0f);
			}

			dir = dir * (1 - f) + vel * f;
			dir.normalize();

			vec3	up = (Subject->GetUp() * (1.3f - f) + YAxis * (f + 0.3f));
			up = dir.cross(up).cross(dir);
			up.normalize();
				
			SetDirection(dir);
			SetUp(up);
			
			break;
		}

		case FLY_AIM:
		{
			// Aim at the subject.
			vec3	dir = Subject->GetLocation() - GetLocation();
			dir.normalize();

//			dir = FlyDir * FlyFactor + dir * (1 - FlyFactor);
//			dir.normalize();
			

//			// Limit turn rate.
//			float	c0 = exp(-u.DeltaT / 0.25);
//			dir = dir * (1 - c0) + GetDirection() * c0;
//			dir.normalize();
			
			SetDirection(dir);
			
			vec3	right = dir.cross(YAxis);	// Roll the right vec3 about DesiredDir...?
			vec3	up = right.cross(dir);
			up.normalize();
			SetUp(up);
		
			break;
		}
		}
	}
};



void	SetAimMode(AimMode m)
// Sets the current aim mode of the camera.
{
	amode = m;
}


void	SetMotionMode(MotionMode m)
// Sets the current motion mode of the camera.
{
	mmode = m;

	if (m == FLY) {
		DoneFlying = false;
	}
}


void	SetSubject(MOriented* o)
// Sets the subject of the camera.
{
	Subject = o;
	DynamicSubject = dynamic_cast<MDynamic*>(o);
}


void	SetLocation(const vec3& loc)
// Move the camera to the specified location.
{
	Instance->SetLocation(loc);
}


void	SetChaseParams(const vec3& offset, float VelDirOffset, float DirOffset, float FixedDirOffset, float Height, float Angle)
// Sets some parameters for the CHASE motion mode.
// * offset is an object-space location for the reference location.
// * VelDirOffset is used to offset the camera location from the reference
// point in the direction of the subject's velocity.
// * DirOffset is used to offset the camera location from the reference
// point in the direction of the subject's direction.
// * FixedDirOffset is used to offset the camera location from the reference
// point along the x-axis.
// * Height is the vertical offset of the camera location from the reference
// point.
// * Angle is for rotating the camera about the reference point.
{
	ChaseOffset = offset;
	ChaseVelDirOffset = VelDirOffset;
	ChaseDirOffset = DirOffset;
	ChaseFixedDirOffset = FixedDirOffset;
	ChaseHeight = Height;
	ChaseAngle = Angle;
}


bool	GetDoneFlying()
// Returns true if we've reached our flying destination.
{
	return DoneFlying;
}


//
// AutoCamera stuff
//


struct AutoParams {
	AutoMode	Mode;
	float	Distance, DistanceSpeed;
	float	Angle, AngleSpeed;
	float	Height, HeightSpeed;
	
} Auto = {
	AUTO_OFF,
	0, 0,
	0, 0,
	0, 0,
};



void	RandomAutoCameraMode()
// Pick a random mode for the automatic camera.
{
	static const AutoMode	ModeTable[6] = {
		AUTO_CHASE_CLOSE,
		AUTO_CHASE_FAR,
		AUTO_CHASE_FAR,
		AUTO_FIXED,
		AUTO_FIXED,
		AUTO_FIXED
	};
	
	SetAutoCameraMode(ModeTable[Utility::RandomInt(6)]);
}


void	SetAutoCameraMode(AutoMode m)
// Sets a new auto-camera mode, and picks new motion parameters.
{
	// Set mode.
	Auto.Mode = m;

	// Pick new params.
	switch (Auto.Mode) {
	default:
	case AUTO_OFF:
		UserCamera::SetAimMode(UserCamera::LOOK_THROUGH);
		UserCamera::SetMotionMode(UserCamera::FOLLOW);
		
		return;
		break;
		
	case AUTO_CHASE_CLOSE:
	{
		// Get up close to the user.
		Auto.Distance = Utility::RandomFloat(4, 10);
		if (Utility::RandomInt(10) < 5) Auto.Distance = -Auto.Distance;
		Auto.DistanceSpeed = Utility::RandomFloat(-0.1f, 0.1f);
		Auto.Angle = Utility::RandomFloat(-PI, PI);
		Auto.AngleSpeed = Utility::RandomFloat(-0.1f, 0.1f);
		Auto.Height = Utility::RandomFloat(-0.5f, 1.0f);
		Auto.HeightSpeed = Utility::RandomFloat(-0.03f, 0.03f);
		
		UserCamera::SetAimMode(UserCamera::LOOK_AT);
		UserCamera::SetMotionMode(UserCamera::CHASE);
		
		break;
	}
					  
	case AUTO_CHASE_FAR:
	{
		// A more distant view of the user.
		Auto.Distance = Utility::RandomFloat(12, 20);
		if (Utility::RandomInt(10) < 5) Auto.Distance = -Auto.Distance;
		Auto.DistanceSpeed = Utility::RandomFloat(-1.0f, 1.0f);
		Auto.Angle = Utility::RandomFloat(-PI, PI);
		Auto.AngleSpeed = Utility::RandomFloat(-0.10f, 0.10f);
		Auto.Height = fabsf(Auto.Distance) * Utility::RandomFloat(0.1f, 0.2f);
		Auto.HeightSpeed = Utility::RandomFloat(-0.3f, 0.3f);

		UserCamera::SetAimMode(UserCamera::LOOK_AT);
		UserCamera::SetMotionMode(UserCamera::CHASE);
		
		break;
	}

	case AUTO_FIXED:
	{
		// Pick a location ahead of the boarder to watch from.
		vec3	v = ZeroVector;
		MOriented*	u = Game::GetUser();
		if (u) {
			v = u->GetLocation();
			vec3	dir = u->GetDirection();
			// Deviate the direction randomly.
			dir += u->GetUp() * Utility::RandomFloat(-0.4f, 0.4f);
			dir += u->GetRight() * Utility::RandomFloat(-0.2f, 0.2f);
			
			v += dir * Utility::RandomFloat(8, 35);
		}
		UserCamera::SetLocation(v);
		
		UserCamera::SetAimMode(UserCamera::LOOK_AT);
		UserCamera::SetMotionMode(UserCamera::FIXED);
		break;
	}
	}
}


void	AutoCameraUpdate(const UpdateState& u)
// Moves the automatic camera, based on the current mode and parameters.
{
	switch (Auto.Mode) {
	default:
	case AUTO_OFF:
		return;
		break;
		
	case AUTO_CHASE_CLOSE:
		break;

	case AUTO_CHASE_FAR:
		break;

	case AUTO_FIXED:
		break;
	}

	Auto.Angle += Auto.AngleSpeed * u.DeltaT;
	if (Auto.Angle > PI) Auto.Angle -= PI;
	if (Auto.Angle < -PI) Auto.Angle += PI;
	Auto.Distance += Auto.DistanceSpeed * u.DeltaT;
	Auto.Height += Auto.HeightSpeed * u.DeltaT;

#if 0
	// Check to see if subject is hidden by terrain.  If it is, then elevate
	// upwards to try to see over the obstruction.
	vec3	subject_dir = Subject->GetLocation() - Instance->GetLocation();
	subject_dir.SetY(subject_dir.Y() - 1);	// Make sure we can see the subject's feet.
	float	subject_distance = subject_dir.magnitude();
	if (subject_distance > 0.1) {
		subject_dir *= 1.0 / subject_distance;

		float	terrain_distance = TerrainMesh::CheckForRayHit(Instance->GetLocation(), subject_dir);
		if (terrain_distance > 0 && terrain_distance + 2.0 < subject_distance) {
			// Subject is obscured by terrain.  Elevate upwards.
			float	elevate = subject_distance * 1.0 * u.DeltaT;
			if (Auto.Mode == AUTO_FIXED) {
				Instance->SetLocation(Instance->GetLocation() + vec3(0, elevate, 0));
			} else {
				Auto.Height += elevate;
				Auto.HeightSpeed = 0;
			}
		}
	}
#endif // 0
	
	// Keep a bare minimum distance away from the figure.
	const float	MIN_D = 1.5;
	float	d = Auto.Distance;
	if (d > -MIN_D) {
		if (d < 0) d = -MIN_D;
		else if (d < MIN_D) d = MIN_D;
	}
	
	UserCamera::SetChaseParams(ZeroVector, 0, 0, d, Auto.Height, Auto.Angle);
}



static struct InitCameraObject {
	InitCameraObject() {
		Game::AddInitFunction(Init);
		Game::AddClearFunction(MountainClear);
	}

	static void	Init() {
		// Create the camera.
		Instance = new CameraObject;

		// Add to database.
		Model::AddDynamicObject(Instance);

		// Set as the viewer.
		GameLoop::SetViewer(Instance);
	}


	static void	MountainClear()
	// Called when a mountain dataset is being cleared.
	{
		// Make sure we're not trying to look at a deleted object.
		SetSubject(NULL);
	}
	
} InitCameraObject;


};

