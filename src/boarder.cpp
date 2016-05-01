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
// boarder.cpp	-thatcher 7/10/1999 Copyright Slingshot Game Technology

// Code for snowboarder physics.  Interaces with articulated figure and feedback controller code.

#ifdef MACOSX
#include "macosxworkaround.hpp"
#endif

#include <math.h>
#include "ogl.hpp"
#include "utility.hpp"
#include "psdread.hpp"
#include "config.hpp"
#include "model.hpp"
#include "gameloop.hpp"
#include "game.hpp"
#include "user.hpp"
#include "usercamera.hpp"
#include "boarder.hpp"
#include "terrain.hpp"
#include "polygonregion.hpp"
#include "ui.hpp"
#include "sound.hpp"
#include "particle.hpp"
#include "text.hpp"
#include "recording.hpp"
#include "weather.hpp"
#include "timer.hpp"

#include "multiplayer.hpp"

using namespace Boarder;


namespace Boarder {
	DOFLimit DOFLimits[DOF_COUNT] = {
		{ -1.5f, 1.5f, 4.0f },	// ANKLE_ANGLE
		{ -1.0f, 1.0f, 4.0f },	// BOARD_YAW
		{ -0.7f, 0.7f, 4.0f }, // BOARD_PITCH
		{ -0.25f, 0.25f, 3.5f },	// LEGS_X
		{ 0.40f, 0.95f, 4.0f },	// LEGS_Y
		{ -0.25f, 0.25f, 3.5f },	// LEGS_Z
		{ -1.0f, 2.0f, 4.0f },	// BACK_TWIST_ANGLE
		{ 0.25f, 0.63f, 1.0f }, // ARM_L_EXTENSION
		{ 0.25f, 0.63f, 1.0f }, // ARM_R_EXTENSION
		{ -1.5f, 1.5f, 8.0f }, // ARM_L_VERT_ANGLE
		{ -1.5f, 1.5f, 8.0f }, // ARM_R_VERT_ANGLE
		{ -1.5f, 3.0f, 8.0f },	// ARM_L_FORWARD_ANGLE
		{ -1.5f, 3.0f, 8.0f },	// ARM_R_FORWARD_ANGLE
		{ -0.4f, 0.4f, 2.0f },	// BACK_LEAN_ANGLE
		{ -0.45f, 1.2f, 3.0f },	// BACK_PITCH_ANGLE
		
		{ 0.20f, 0.95f, 2.0f },	// LEFT_LEG_MAX_EXT
		{ 0.20f, 0.96f, 2.0f },
		{ 0.0f, 3000.0f, 1000000.0f },	// LEFT_LEG_FORCE
		{ 0.0f, 3000.0f, 1000000.0f },
		
	};
};


//const float	AirDragCoeff = 0.7;

const float	DRAG_COEFF = 0.8f;	// 0.2 (full tight skier tuck) to 1.2 (straight up) by test
const float	FRONTAL_AREA = 0.7f;	// m^2	shrinks to maybe 0.47 in a crouch?
const float	AIR_DENSITY = 1.284f /* sea-level, 2 deg Celsius */ ;	// kg / m^3


const float	BOARD_CONTACT_SAMPLE_SEPARATION = 1.4f;	// Distance between front and rear "foot" contact samples.


const int	COLLISION_SPHERE_COUNT = 5;
const float	COLLISION_SPHERE_RADIUS = 0.2f;
struct CollisionSphereInfo {
	float	x, y, z;
} CollisionSphereOffset[COLLISION_SPHERE_COUNT] = {
	{ 0.25f, 0.5f, 0.0f },
	{ -0.25f, 0.5f, 0.0f },
	{ 0.0f, 0.3f, 0.0f },
	{ 0.7f, -0.6f, 0.0f },
	{ -0.7f, -0.6f, 0.0f },
};


bool	UserCreated[] = {false, false, false, false};


const int	MAX_RAY_COUNT = 10;

const int	BUMP_SOUND_HOLDOFF = 250;	// Don't play bump sounds closer than 1/4 second together.


bool	RecordPath = false;	// For recording a PerfTest path.

const int	CP_COUNT = 12;	// Checkpoint count.
const int	CHECKPOINT_INTERVAL = 500;	// Take a checkpoint every so often.


//xxxxxxxx
static int	DebugInputTestTicks = 0;
//xxxxxxxx



class Snowboarder : public MDynamic, public UserType {
public:

	float	FigureState[DOF_COUNT];
	FootStatus	Foot[2];
	float	Bouyancy;

	bool	BoardShadow;	// Cosmetic.
	
	BoarderController*	Controller;
	bool	Crashing;
	int	CrashingTicks;
	
	bool	SampledResetValues;
	vec3	ResetLocation;
	quaternion	ResetOrientation;

	bool	Active;
	
	int	SnowSoundID, IceSoundID, AirSoundID;
	int	LastBumpSoundTicks;
	int	LastUpdateSound;
	bool	LastAirborne;
	float	FilteredDragVolume;

	vec3	LastAccel;
	vec3	LastDerivOmega;
	
	// Each foot is potentially a source of particles (conceptually).
	struct ParticleInfo : public Particle::SourceInfo {
		bool	Valid;
		int	Count;
	} ParticleSource[2];
	int	ParticleAccumulator;
	int	LastParticleSampleTicks;	// To help calculate particle rate for recording.

	// Checkpoints are for restoring the boarder to a known valid state after a crash.
	struct CheckpointInfo {
		int	Ticks;
		vec3	Location;
		quaternion	Orientation;
		vec3	Velocity, AngMomentum;
		float	Bouyancy;
		int	Points;
		int	RunTimer;
		float	Figure[DOF_COUNT];
		uint8*	ControllerBuffer;
		int	cbufsize;

		CheckpointInfo() {
			ControllerBuffer = NULL;
			cbufsize = 0;
			Reset(ZeroVector, quaternion(0, ZeroVector), ZeroVector);
		}
		
		void	Reset(const vec3& loc, const quaternion& orient, const vec3& vel) {
			Ticks = 0;
			Location = loc;
			Orientation = orient;
			Velocity = vel;
			AngMomentum = ZeroVector;
			Bouyancy = 0;
			Points = 0;
			RunTimer = 0;
			
			int	i;
			for (i = 0; i < DOF_COUNT; i++) {
				Figure[i] = (DOFLimits[i].Min + DOFLimits[i].Max) * 0.5f;
			}

//			ControllerBuffer = NULL;
//			cbufsize = 0;
			if (ControllerBuffer) memset(ControllerBuffer, 0, cbufsize);
		}

		CheckpointInfo&	operator=(const CheckpointInfo& c)
		// Copy contents.
		{
			Ticks = c.Ticks;
			Location = c.Location;
			Orientation = c.Orientation;
			Velocity = c.Velocity;
			AngMomentum = c.AngMomentum;
			Bouyancy = c.Bouyancy;
			Points = c.Points;
			RunTimer = c.RunTimer;

			int	i;
			for (i = 0; i < DOF_COUNT; i++) {
				Figure[i] = c.Figure[i];
			}

			if (ControllerBuffer && c.ControllerBuffer) {
				memcpy(ControllerBuffer, c.ControllerBuffer, cbufsize);
			}

			return *this;
		}
		
		~CheckpointInfo() {
			if (ControllerBuffer) delete [] ControllerBuffer;
		}
	} Checkpoint[CP_COUNT];
	int	NextCheckpoint;
	int	CheckpointTimer;

	vec3	PreUpdateLocation;	// For checking motion against triggers.
	
	// xxxxxxxx for debugging.
	int	RedRayCount;
	int	GreenRayCount;
	vec3	RedRayStart[MAX_RAY_COUNT];
	vec3	RedRay[MAX_RAY_COUNT];
	vec3	GreenRayStart[MAX_RAY_COUNT];
	vec3	GreenRay[MAX_RAY_COUNT];
	// xxxxxxx
	
	Snowboarder()
	// Init members.
	{
		int	i;
		for (i = 0; i < DOF_COUNT; i++) {
			FigureState[i] = (DOFLimits[i].Min + DOFLimits[i].Max) * 0.5f;	// some kind of defaults.
		}
		for (i = 0; i < 2; i++) {
			Foot[i].SurfaceType = -1;
			ParticleSource[i].Valid = false;
		}
		ParticleAccumulator = 0;
		LastParticleSampleTicks = 0;
		Bouyancy = 0;
		BoardShadow = false;
		Controller = NULL;
		Crashing = false;
		CrashingTicks = 0;
		
		SampledResetValues = false;
		ResetLocation = ZeroVector;
		Active = true;

		SnowSoundID = 0;
		IceSoundID = 0;
		AirSoundID = 0;

		LastUpdateSound = 0;
		LastAirborne = false;
		LastBumpSoundTicks = 0;
		FilteredDragVolume = 0;

		LastAccel = ZeroVector;
		LastDerivOmega = ZeroVector;
		
		NextCheckpoint = 0;
		CheckpointTimer = 0;

		PreUpdateLocation = ZeroVector;
		
		RedRayCount = GreenRayCount = 0;
	}


	~Snowboarder()
	// Release any resources.
	{
	  int player_index = MultiPlayer::CurrentPlayerIndex();
	  UserCreated[player_index] = false;
	  
	  // Release sound handles.
	  if (SnowSoundID) {
	    Sound::Release(SnowSoundID);
	  }
	  if (IceSoundID) {
	    Sound::Release(IceSoundID);
	  }
	  if (AirSoundID) {
	    Sound::Release(AirSoundID);
	  }
	}
	
		
	void	Reset()
	// Reset state of boarder.
	{
		int	i;
		for (i = 0; i < DOF_COUNT; i++) {
			FigureState[i] = (DOFLimits[i].Min + DOFLimits[i].Max) * 0.5f;	// some kind of defaults.
		}
		for (i = 0; i < 2; i++) {
			Foot[i].SurfaceType = -1;
			ParticleSource[i].Valid = false;
		}
		ParticleAccumulator = 0;
		LastParticleSampleTicks = 0;

		SetVelocity(ZeroVector);
		SetAngMomentum(ZeroVector);
		SetLocation(ResetLocation);
		SetOrientation(ResetOrientation);

		Bouyancy = 0;
		BoardShadow = false;
		
		if (Controller) Controller->Reset();
		
		Crashing = false;
		CrashingTicks = 0;

		LastAirborne = false;

		LastAccel = ZeroVector;
		LastDerivOmega = ZeroVector;

		// Reset the checkpoints.
		for (i = 0; i < CP_COUNT; i++) {
			Checkpoint[i].Reset(ResetLocation, ResetOrientation, ZeroVector);
		}
		NextCheckpoint = 0;
		CheckpointTimer = 0;
	}
	
	void	Reset(const vec3& Loc, const vec3& Dir, const vec3& Vel)
	// Resets location, velocity, and direction.
	{
		int	i;
		for (i = 0; i < DOF_COUNT; i++) {
			FigureState[i] = (DOFLimits[i].Min + DOFLimits[i].Max) * 0.5f;	// some kind of defaults.
		}
		for (i = 0; i < 2; i++) {
			Foot[i].SurfaceType = -1;
			ParticleSource[i].Valid = false;
		}
		ParticleAccumulator = 0;
		LastParticleSampleTicks = 0;
		
		if (Controller) Controller->Reset();
		
		Crashing = false;
		CrashingTicks = 0;
		
		LastAirborne = false;

		LastAccel = ZeroVector;
		LastDerivOmega = ZeroVector;
		
		SetDirection(Dir);
		SetUp((Dir.cross(YAxis).cross(Dir)).normalize());
		SetLocation(Loc);
		SetVelocity(Vel);
		SetAngMomentum(ZeroVector);
		
		// Silence sounds.
		Silence();
//
//		ViewIndex = -1;

		// Reset the checkpoints.
		for (i = 0; i < CP_COUNT; i++) {
			Checkpoint[i].Reset(Loc, quaternion(YAxis, atan2f(-Dir.Z(), Dir.X())), Vel);
		}
		NextCheckpoint = 0;
		CheckpointTimer = 0;
	}


	int	GetResumeCheckpointTicks()
	// Return the ticks value of the checkpoint we'll use for resuming.
	{
		CheckpointInfo&	c = Checkpoint[NextCheckpoint];	// Oldest checkpoint.
		return c.Ticks;
	}
		

	int	ResumeFromCheckpoint()
	// Resets the state of the boarder to the values from our oldest
	// checkpoint (a few seconds ago).  This is for resuming a run
	// after a crash, without having to start from the beginning.
	// Returns the ticks value at the time of the checkpoint we've rewound to.
	{
		CheckpointInfo&	c = Checkpoint[NextCheckpoint];	// Oldest checkpoint.

		SetLocation(c.Location);
		SetOrientation(c.Orientation);
		SetVelocity(c.Velocity);
		SetAngMomentum(c.AngMomentum);
		int	i;
		for (i = 0; i < DOF_COUNT; i++) {
			FigureState[i] = c.Figure[i];
		}
		
		for (i = 0; i < 2; i++) {
			Foot[i].SurfaceType = -1;
			ParticleSource[i].Valid = false;
		}
		ParticleAccumulator = 0;
		LastParticleSampleTicks = 0;
		
		if (Controller) {
			Controller->Reset();
			Controller->DecodeCheckpoint(c.ControllerBuffer, 0);
		}

		Bouyancy = c.Bouyancy;
		Game::SetScore(c.Points);
		Game::SetTimer(c.RunTimer);
		
		Crashing = false;
		CrashingTicks = 0;
		
		LastAirborne = false;
		
		// Silence sounds.
		Silence();

		// Reset the checkpoints to all reflect the same info,
		// in case the user crashes again before they're all
		// refilled with new data.
		for (i = 0; i < CP_COUNT; i++) {
			if (i != NextCheckpoint) Checkpoint[i] = c;
		}

		return c.Ticks;
	}


	void	TimestampCheckpoints(int Ticks)
	// Set the timestamp of current checkpoints to the given value.
	// This function is called before actually activating the boarder
	// when resuming.
	{
		int	i;
		for (i = 0; i < CP_COUNT; i++) {
			Checkpoint[i].Ticks = Ticks;
		}
	}

	
	void	SetActive(bool active)
	// Disables or enables logic & physics updates.
	{
		Active = active;

		// Make sure sounds are quieted.
		Silence();
	}
	

	void	Silence()
	// Shut off all the continuous sounds.
	{
		if (SnowSoundID) {
			Sound::Adjust(SnowSoundID, Sound::Controls(0));
		}
		if (IceSoundID) {
			Sound::Adjust(IceSoundID, Sound::Controls(0));
		}
		if (AirSoundID) {
			Sound::Adjust(AirSoundID, Sound::Controls(0));
		}

		FilteredDragVolume = 0;
	}

	
	float	GetMass()
	// Returns mass.
	{
		return 80.0;
	}
	
	float	GetInvMass()
	// Returns 1/mass.
	{
		return 1.0f / GetMass();
	}

	float	GetInvMoment()
	// Returns 1 / moment of inertia.
	{
		// one over 2/3 * Mass * radius^2
		return 1.0f / 30.0f;
	}
			
	void	AttachController(BoarderController* c)
	// Sets c as the controller for this boarder object.  c will be
	// passed user inputs and be called to get control outputs for
	// the figure.
	{
		Controller = c;

		// Create checkpoint buffers for the controller.
		int	buf_size = Controller->GetCheckpointBytes();
		int	i;
		for (i = 0; i < CP_COUNT; i++) {
			if (Checkpoint[i].ControllerBuffer == NULL) {
				Checkpoint[i].ControllerBuffer = new uint8[buf_size];
				Checkpoint[i].cbufsize = buf_size;
			}
		}
	}
	
	void	AddRedRay(const vec3& start, const vec3& ray)
	// Adds a ray to be rendered.  For debugging, for visualizing forces.
	{
		if (RedRayCount < MAX_RAY_COUNT) {
			RedRayStart[RedRayCount] = start;
			RedRay[RedRayCount] = ray;
			RedRayCount++;
		}
	}
	
	void	AddGreenRay(const vec3& start, const vec3& ray)
	// Adds a ray to be rendered.  For debugging, for visualizing forces.
	{
		if (GreenRayCount < MAX_RAY_COUNT) {
			GreenRayStart[GreenRayCount] = start;
			GreenRay[GreenRayCount] = ray;
			GreenRayCount++;
		}
	}

	
	void	Crash(const UpdateState& u)
	// Initiates the crash phase.
	{
		Crashing = true;
		CrashingTicks = 2000;
		// Bjorn 
		// TODO: check which player has crached
		int player_index = MultiPlayer::CurrentPlayerIndex();
		UI::SetMode(UI::SHOWCRASH, u.Ticks, player_index);
	}

	
	void	CollisionNotify(const UpdateState& u, const vec3& loc, const vec3& normal, const vec3& impulse)
	// Gets called on collision.  Opportunity to make a sound, or do other stuff.
	{
		// If we have a collision over say 1 gee or so, then that's it for the boarder.
		if (impulse.magnitude() > 10.0 && UI::GetMode() == UI::PLAYING) {
			Crash(u);
		}
	}
	
	void	Render(ViewState& s, int ClipHint)
	// Renders the model.  Passes DOF values to the articulated mesh.
	{
		// Pass current DOF values to the articulated mesh.
		GArticulated*	a = dynamic_cast<GArticulated*>(Visual);
		if (a) {
			int	i;
			for (i = 0; i < DOF_COUNT; i++) {
				a->SetParameter(i, FigureState[i]);
			}
		}

/*
		//xxxxxxxxx
		// Show the surface type under each foot.
		{
			char	buf[80];
			Text::FontID	f = Text::FIXEDSYS;
			
			int	y = 20;
			int	dy = Text::GetFontHeight(f);
			Text::FormatNumber(buf, Foot[0].SurfaceType, 2, 1);
			Text::DrawString(20, y, f, Text::ALIGN_LEFT, buf, 0xFF000000);
			y += dy;
			Text::FormatNumber(buf, Foot[1].SurfaceType, 2, 1);
			Text::DrawString(20, y, f, Text::ALIGN_LEFT, buf, 0xFF000000);
			y += dy;
		}
		//xxxxxxx
*/
		
		// Pass up to base-class for more processing.
		MDynamic::Render(s, ClipHint);

		if (Config::GetBoolValue("ShowBoarderRays")) {
			//xxxxxx some force vec3s, for debugging.
			int	i;
			// Red rays.
			glColor3f(1, 0, 0);
			for (i = 0; i < RedRayCount; i++) {
				glBegin(GL_LINES);
				glVertex3fv(RedRayStart[i]);
				glVertex3fv(RedRayStart[i] + RedRay[i]);
				glEnd();
			}
			// Green rays.
			glColor3f(0, 1, 0);
			glBegin(GL_LINES);
			for (i = 0; i < GreenRayCount; i++) {
				glVertex3fv(GreenRayStart[i]);
				glVertex3fv(GreenRayStart[i] + GreenRay[i]);
			};
			glEnd();
			
			//xxxxxx
		}

		if (Config::GetBoolValue("ShowBoarderFeet")) {
			//xxxxxxxxx Boarder feet friction points, for debugging
			// Red stars
			glColor3f(1, 0, 0);
			for (int i = 0; i < 2; i++) {
				vec3	v = Foot[i].Location;
			
				glBegin(GL_LINES);
				glVertex3fv(v + vec3(0, 0.1, 0));
				glVertex3fv(v + vec3(0, -0.1, 0));

				glVertex3fv(v + vec3(0.1, 0, 0));
				glVertex3fv(v + vec3(-0.1, 0, 0));

				glVertex3fv(v + vec3(0, 0, 0.1));
				glVertex3fv(v + vec3(0, 0, -0.1));
				glEnd();
			}
			//xxxxxxxx
		}
	}


	int	DecodeParticleInfo(uint8* buf, int index, ParticleInfo *info, vec3& boarder_location)
	// Reads data from the given buffer and index, and uses it to initialize the members of *info.
	// boarder_location must be known, because particle source location is defined in relation to it.
	{
		int	index0 = index;
		int	i;
		
		for (i = 0; i < 3; i++) {
			int8	offset = (int8) buf[index++];
			info->Location.Set(i, boarder_location.Get(i) + (offset / 64.0f));
		}
		for (i = 0; i < 3; i++) {
			float	f;
			f = ((int8) buf[index++]) / 2.0f;
			info->Velocity.Set(i, f);
		}
		info->LocationVariance = expf(buf[index++] / 50.0f) - 1;
		info->VelocityVariance = expf(buf[index++] / 50.0f) - 1;

		return index - index0;
	}

	void	RunLogic(const UpdateState& u)
	// Logic step for boarder.
	{
		PreUpdateLocation = GetLocation();
		
// drop from a very great height.  For testing terminal descent velocity, to check air drag calibration.
//		//xxxxxxx
//		if (u.Inputs.Button[Input::F1].State) {
//			vec3	l = GetLocation();
//			l.SetY(10000);
//			SetLocation(l);
//		}
//		//xxxxxxx
		
		Recording::Mode	rmode = Recording::GetMode();
		if (rmode == Recording::PLAY || rmode == Recording::PAUSE || rmode == Recording::PAUSERECORD) {
			// Play back a recording.  Extract boarder state from recording module,
			// and set the state of the boarder using the extracted data.

			uint8*	bufa;
			uint8*	bufb;
			float	f;
			Recording::GetCurrentChunkPair(Recording::BOARDER_STATE, &bufa, &bufb, &f);

			// These location values are used to help decode the particle source info.
			vec3	boarder_location_a, boarder_location_b;
			boarder_location_a = boarder_location_b = GetLocation();	// Somewhat sane defaults.  Fixed if there's boarder data.

			if (bufa) {
				int	index = 0, i;
				float	temp;
				
				vec3	va, vb;
				for (i = 0; i < 3; i++) {
					float	temp;
					DecodeFloat32(bufa, index, &temp);
					va.Set(i, temp);
					index += DecodeFloat32(bufb, index, &temp);
					vb.Set(i, temp);
				}

				boarder_location_a = va;
				boarder_location_b = vb;

				quaternion	qa, qb;
				for (i = 0; i < 4; i++) {
					DecodeFloat32(bufa, index, &temp);
					qa.Set(i, temp);
					index += DecodeFloat32(bufb, index, &temp);
					qb.Set(i, temp);
				}

				va += (vb - va) * f;
				qa = qa.lerp(qb, f);

				SetLocation(va);
				SetOrientation(qa);

//				//xxxxxx
//				SetVelocity(XAxis);	// dummy velocity value.  For benefit of camera control.

				// Figure state.
				for (i = 0; i < DOF_RENDERING_COUNT; i++) {
					uint8	dofa = bufa[index];
					uint8	dofb = bufb[index++];
					DOFLimit&	d = DOFLimits[i];
					FigureState[i] = d.Min + (d.Max - d.Min) * (dofa + (dofb - dofa) * f) / 255.0f;
				}

				// Get encoded velocity direction.
				uint16	dira, dirb;
				DecodeUInt16(bufa, index, &dira);
				index += DecodeUInt16(bufb, index, &dirb);
				int16	deltdir = dirb - dira;
				uint16	dir = uint16(dira + deltdir * f);

				SetVelocity(Geometry::Rotate(dir * (2*PI/65536.0f), YAxis, XAxis) * 2);

				// Get BoardShadow flag.
				BoardShadow = (bufa[index++] & 1) ? true : false;
			}

			//
			// Re-generate recorded particles.
			//
			Recording::GetCurrentChunkPair(Recording::BOARDER_PARTICLES, &bufa, &bufb, &f);

			if (bufa) {
				uint8	flagsa, flagsb;
				float	rate_a, rate_b;
				int	index = 0;
				
				// Flags and count for both chunks.
				flagsa = bufa[index];
				flagsb = bufb[index];
				index++;

				uint16	temp;
				DecodeUInt16(bufa, index, &temp);
				rate_a = temp;
				index += DecodeUInt16(bufb, index, &temp);
				rate_b = temp;

				// Only continue if all four particle sources are valid.
				if ((flagsa & flagsb) == 3) {
					// Extract the four particle sources.
					ParticleInfo	source[2][2];

					int	i;
					for (i = 0; i < 2; i++) {
						DecodeParticleInfo(bufa, index, &source[0][i], boarder_location_a);
						index += DecodeParticleInfo(bufb, index, &source[1][i], boarder_location_b);
					}

					// Generate some particles!
					float	DeltaT = u.DeltaT * GameLoop::GetSpeedScale();
					int	DeltaTicks = (int) (u.DeltaTicks * GameLoop::GetSpeedScale());
					if (DeltaT > 0 && DeltaTicks > 0) {
						int	count = (int) ((rate_a + (rate_b - rate_a) * f) * DeltaT);
						Particle::LineSource(Particle::SNOW_POWDER, source[0][0], source[1][0], source[0][1], source[1][1], count, DeltaTicks);
					}
				}
			}
		}
		
		// Change camera view on demand.
		if ((
		     UI::GetMode() == UI::PLAYING ||
		     UI::GetMode() == UI::COUNTDOWN ||
		     UI::GetMode() == UI::RESUME ||
		     UI::GetMode() == UI::SHOWCRASH ||
		     UI::GetMode() == UI::SHOWFINISH
		    ) && Input::CheckForEventDown(Input::BUTTON2))
		{
			// Increment to the next view.
			int	m = Game::GetPlayerCameraMode();
			m = (m + 1) % (int) Game::CAMERA_MODES;
			Game::SetPlayerCameraMode((Game::CameraMode) m);
		}

		if (!Active) return;

		//
		// PerfTest path recording.
		//
		if (RecordPath) {
			// For demo path recording.
			static int	RecordPathPointCount = 0;
			static int	NextPointTicks = 0;
			static FILE*	PathFile = NULL;

			const int	PathPointCount = 450;
			if (NextPointTicks == 0) {
				// Initialize path recording.
				NextPointTicks = u.Ticks + 1000;
				PathFile = Utility::FileOpen("PerfTestPath.dat", "wb");
				if (PathFile == NULL) {
					Error e; e << "Can't open PerfTestPath.dat for output";
					throw e;
				}
				int	temp = PathPointCount;
				fwrite(&temp, sizeof(int), 1, PathFile);	// point count.
			}
			if (u.Ticks >= NextPointTicks) {
				// Record a point.
				vec3	v = GetLocation();
				fwrite(&v, sizeof(vec3), 1, PathFile);
				quaternion	q = GetMatrix().GetOrientation();
				fwrite(&q, sizeof(quaternion), 1, PathFile);

				NextPointTicks += 100;	// 10 Hz.
				RecordPathPointCount++;
				if (RecordPathPointCount >= PathPointCount) {
					RecordPath = false;
					fclose(PathFile);
				}
			}
		}

		
		if (!SampledResetValues) {
			SampledResetValues = true;
			ResetLocation = GetLocation();
			ResetOrientation = GetMatrix().GetOrientation();
		}
		
		if (UI::GetMode() == UI::CAMERA && u.Inputs.Button[2].State) {
			Reset();
			DebugInputTestTicks = 0;
		}
		
		// If we're in the process of crashing, countdown to reset.
		if (CrashingTicks) {
			CrashingTicks -= u.DeltaTicks;
			if (CrashingTicks <= 0) {
				// Done crashing, give the user some options.
				CrashingTicks = 0;
				UI::SetMode(UI::CRASHQUERY, u.Ticks);
			}
		}

		// Take a checkpoint if it's appropriate.  Checkpoints are
		// used to resume from a known OK prior state.
		if (UI::GetMode() == UI::PLAYING) {
			CheckpointTimer += u.DeltaTicks;

			if (CheckpointTimer >= CHECKPOINT_INTERVAL) {
				CheckpointTimer -= CHECKPOINT_INTERVAL;
				TakeCheckpoint(u.Ticks);
			}
		}
	}

	
	void	RunDynamics(const UpdateState& u)
	// Physics step for boarder.
	{
		// Break into short time steps.
		UpdateState	v;

		const int	MAX_UPDATE_TICKS = 5;
		int	ticks = u.DeltaTicks;
		while (ticks) {
			int	dticks = imin(MAX_UPDATE_TICKS, ticks);

			v = u;
			v.Ticks -= u.DeltaTicks - ticks + dticks;
			v.DeltaTicks = dticks;
			v.DeltaT = dticks / 1000.0f;

			RunDynamicsAux(v);

			ticks -= dticks;
		}
	}


	void	RunDynamicsAux(const UpdateState& u)
	// Does the actual work of an update step.
	{
		int	i;

		//xxxxxxxxx
		RedRayCount = GreenRayCount = 0;
		//xxxxxxxxxx

		if (!Active) return;

		// Get figure dynamics parameters, to use for adjusting dynamics.
		vec3	HipOffset;
		float	InverseMass;
		matrix33	InverseInertiaTensor;
		GArticulated*	art = dynamic_cast<GArticulated*>(Visual);
		if (art) {
			art->GetDynamicsParameters(&HipOffset, &InverseMass, &InverseInertiaTensor);

		} else {
			// Reasonable defaults.
			InverseMass = 1.0f / GetMass();
			HipOffset = ZeroVector;
			InverseInertiaTensor.SetColumns(vec3(1.0f/30.0f, 0, 0), vec3(0, 1.0f/30.0f, 0), vec3(0, 0, 1.0f/30.0f));
		}
		// Rotate the inertia tensor into world coordinates.
		InverseInertiaTensor = Geometry::RotateTensor(GetMatrix(), InverseInertiaTensor);

		// Compute angular velocity for use later.
		vec3	Omega = InverseInertiaTensor * GetAngMomentum();
		
		// Initiate continuous sounds.
		if (SnowSoundID == 0) {
			SnowSoundID = Sound::Play("snowdrag.wav", Sound::Controls(0, 0, 1.0, true));
		}
		if (IceSoundID == 0) {
			IceSoundID = Sound::Play("icedrag.wav", Sound::Controls(0, 0, 1.0, true));
		}
		if (AirSoundID == 0) {
		  AirSoundID = Sound::Play("airdrag.wav", Sound::Controls(1.0, 0, 1.0, true));
		}

		// Sound parameters.
		int	DragType = 0;	// 0 for snow, 1 for ice.
		float	DragVolume = 0;
		float	DragPitch = 1.0;
		bool	Airborne = true;

		// Prepare the particle parameters.
		ParticleInfo	psource[2];
		for (i = 0; i < 2; i++) {
			psource[i].Valid = false;
		}
		
		//
		// Run controller to get DOF commands.
		//

		//xxxxxxxxxx
		DebugInputTestTicks += u.DeltaTicks;
		if (Config::GetIntValue("DebugSteer") == 1) {
			if (DebugInputTestTicks < 4000) {
				const_cast<UpdateState&>(u).Inputs.Tilt = 0;
			} else if (DebugInputTestTicks < 7000) {
				const_cast<UpdateState&>(u).Inputs.Tilt = -0.75;
			} else if (DebugInputTestTicks < 10000) {
				const_cast<UpdateState&>(u).Inputs.Tilt = 0.75;
			}
		}
		if (Config::GetIntValue("DebugSteer") == 2) {
			if (DebugInputTestTicks < 3000) {
				const_cast<UpdateState&>(u).Inputs.Tilt = -0.25;
			} else if (DebugInputTestTicks < 5000) {
				const_cast<UpdateState&>(u).Inputs.Tilt = 0.25;
			} else if (DebugInputTestTicks < 7000) {
				const_cast<UpdateState&>(u).Inputs.Tilt = -0.25;
			} else if (DebugInputTestTicks < 9000) {
				const_cast<UpdateState&>(u).Inputs.Tilt = 0.25;
			}
		}
		//xxxxxxxxxxx
		
		// Set outputs to default values.
		DOFCommand	FigureTargets[DOF_COUNT];
		for (i = 0; i < DOF_COUNT; i++) {
			FigureTargets[i] = (DOFLimits[i].Min + DOFLimits[i].Max) * 0.5f;
			FigureTargets[i].SetWeight(1.0);
		}

		vec3	BoardAxis = Foot[0].Location - Foot[1].Location;
		BoardAxis.normalize();
		
		if (Controller) {
			DynamicState	d;
			d.InverseMass = InverseMass;
			d.InverseInertiaTensor = InverseInertiaTensor;
			d.Velocity = GetVelocity();
			d.Omega = Omega;
			d.Accel = LastAccel;
			d.DerivOmega = LastDerivOmega;

			d.BoardAxis = BoardAxis;
			d.BoardUp = (Foot[0].BoardNormal + Foot[1].BoardNormal).normalize();
			d.BoardRight = d.BoardAxis.cross(d.BoardUp).normalize();

			d.Crashing = Crashing;
			
			Controller->Update(this, u, d, FigureState, Foot, FigureTargets);
		}

		// xxxxxx
		for (i = 0; i < 2; i++) {
			Foot[i].LegForce = FigureTargets[LEFT_LEG_FORCE + i];
		}
		//xxxxxxx

		//xxxxx
		vec3	CommandForce(FigureTargets[FORCE_X], FigureTargets[FORCE_Y], FigureTargets[FORCE_Z]);
		vec3	CommandTorque(FigureTargets[TORQUE_X], FigureTargets[TORQUE_Y], FigureTargets[TORQUE_Z]);
		//xxxxx
		
		// (probly not: compute forces due to DOF commands (momentum accels (?)))

		float	NewBouyancy = 0;

		// Accumulate forces.
		vec3	Force  = ZeroVector;
		vec3	Torque = ZeroVector;

		if (Crashing == false) {
			// Compute ground forces (using status, norms, etc).
			vec3	FootForce[2];
			for (i = 0; i < 2; i++) {
				if (Foot[i].SurfaceType >= 0) {
					vec3	COMDir = (GetLocation() - Foot[i].Location);
					COMDir -= BoardAxis * (COMDir * BoardAxis);
					COMDir.normalize();
					FootForce[i] = (COMDir/*Foot[i].BoardNormal*/ * Foot[i].LegForce /*FigureState[LEG_FORCE_LEFT + i]*/);
				} else {
					FootForce[i] = ZeroVector;
				}
			}
//			float	avg_normal_force = 0;
//			if (fcount) avg_normal_force = normmag / fcount;
			vec3	sum_force = FootForce[0] + FootForce[1];

			const float	SIDE_CUT_RADIUS = 10;//xxxxxx
			
			for (i = 0; i < 2; i++) {
				if (Foot[i].SurfaceType < 0) continue;

				Airborne = false;
				
				const Surface::PhysicalInfo&	pi = Surface::GetSurfaceInfo(Foot[i].SurfaceType);
				//const float	surface_factor = 2.5;
				float	surface_factor = 3.0f * pi.EdgeHold;

				// Force at foot due to leg.
//				vec3	COMDir = (GetLocation() - Foot[i].Location);
//				COMDir -= BoardAxis * (COMDir * BoardAxis);
//				COMDir.normalize();
//				vec3	f = (COMDir/*Foot[i].BoardNormal*/ * Foot[i].LegForce /*FigureState[LEG_FORCE_LEFT + i]*/);
				vec3	f = FootForce[i];

				vec3	n = Foot[i].SurfaceNormal;
				vec3	ax = BoardAxis - n * (BoardAxis * n);
				ax.normalize();
				vec3	tr = ax.cross(n);	// Tangent to surface, points across board axis to the right (z-axis of the figure).

				vec3	r = Foot[i].ForceLocation - GetLocation();
				vec3	vel = GetVelocity() + Omega.cross(r);
				vec3	velp = vel - n * (vel * n);	// Point velocity parallel to surface.

				// Compute normal force.
				float	normal_force = f * n;
				float	normvel = -(vel * n);
				float	tanvel = (vel + n * normvel).magnitude();

				NewBouyancy += 0.5f * fclamp(0, powf((tanvel - normvel * 1) / pi.MaxFloatSpeed, 2), 1);
				
				if (Foot[i].HardContact == false) {
					float	flimit = fmax(0.0f, pi.NormalBouyancyFactor * normvel);
//						+ pi.TangentBouyancyFactor * tanvel * tanvel
						;	// Board size & orientation factors....

					normal_force = fclamp(0, normal_force, flimit);
					normal_force = fclamp(0, flimit, 2500);//xxxxx
//					normal_force = fclamp(0, normal_force, pi.NormalBouyancyFactor * normvel);
//					normal_force = fclamp(0, normal_force, pi.TangentBouyancyFactor * tanvel * tanvel + pi.NormalBouyancyFactor * normvel);
				}

				float	SinkDepth = pi.MaxDepth * (1 - NewBouyancy);

				float	tangentmag = f * tr;	// Component of force tangent to surface.
				
				// Compute veldir.  Smoothly vanishes to nothing if vel. mag is small.
				vec3	veldir = velp;
				float	velmag = fmax(0.5f, veldir.magnitude());
				veldir /= velmag;

				float	sin_theta = Foot[i].BoardNormal * tr;
				float	cos_theta = Foot[i].BoardNormal * n;
				
				float	shovel_shadow = veldir * ax * (i == 0 ? 1 : -1);
				shovel_shadow = powf(fclamp(0, (shovel_shadow - 0.8f) * 10, 1), 0.5f);
//				shovel_shadow = 0;
				
#ifdef OLD
				const static float	EDGE_ANGLE = 0.187;	// depends on side-cut radius & board length.
				vec3	HeelEdgeNorm = Geometry::Rotate(EDGE_ANGLE + PI/2, n, ax);
				vec3	ToeEdgeNorm = Geometry::Rotate(-(EDGE_ANGLE + PI/2), n, ax);

				// Angles are reversed for the right foot.
				if (i==1) {
					vec3	temp = HeelEdgeNorm;
					HeelEdgeNorm = ToeEdgeNorm;
					ToeEdgeNorm = temp;
				}

				// Preload depends which direction we're going.
				float	preload = 0.1;
				if (i==0) {
					preload = 0.0;
				} else {
					preload = 0.1;
				}
				
				float	balance = Foot[i].BoardNormal * tr;

				// Compute force due to the board's side-cuts.
				float	ToeEdge = fclamp(0, balance + preload, 1);
				float	HeelEdge = fclamp(0, -balance + preload, 1);

				float	EdgeForce = fmax(70.0f, velmag * 250);
				vec3	EdgeCutForce = HeelEdgeNorm * (HeelEdge * -(HeelEdgeNorm * veldir) * fabs(BoardAxis * veldir) * EdgeForce);
				EdgeCutForce += ToeEdgeNorm * (ToeEdge * -(ToeEdgeNorm * veldir) * fabs(BoardAxis * veldir) * EdgeForce);
#else
				
				// NEW

				float	Rt = SIDE_CUT_RADIUS * cos_theta;
				float	bd_len = BOARD_CONTACT_SAMPLE_SEPARATION;
				if (Rt < bd_len) Rt = bd_len;
				
				vec3	edge_norm = tr * sqrtf(Rt * Rt - 0.25f * bd_len * bd_len) * (sin_theta > 0 ? 1.f : -1.f)
					+ ax * 0.5f * bd_len * (i==0 ? -1.f : 1.f);
				edge_norm.normalize();

				edge_norm = tr;//CHEAT
				
				if (veldir * edge_norm > 0) edge_norm = -edge_norm;

				// Carve vs. sliding friction... i.e. cut-in edge vs. sliding edge.  Should be a surface property... i.e. like ratio between static and dynamic friction.
				float	EdgeGripFactor = pi.EdgeHold;
				float	edge_grip = 1 + fclamp(0, (0.7f - fabsf(veldir * edge_norm)) * 2, 1) * EdgeGripFactor;
//				float	edge_grip = 1;
				
				float	edge_factor = fabsf(sin_theta);
				edge_factor = fclamp(0, (edge_factor - 0.10f * shovel_shadow) * 20, 1);
//				if (veldir * edge_norm < 0) {
					edge_factor *= fabsf(sin_theta) + (1 - shovel_shadow) * 0.20f;
//				} else {
//					edge_factor *= (fabsf(sin_theta) + (-shovel_shadow) * 0.10) * 0.5;//xxxxxx
//				}
				edge_factor = fclamp(0, edge_factor, 1);

				float	edot = fabsf(veldir * edge_norm);
				float	Ftrack = edot * (normal_force * 0.75f + sum_force * n * 0.5f * 0.25f) * surface_factor * edge_factor * edge_grip
//					+ edot * /* constant? */ velmag * velmag * 0.02
					;
//				float	flimit = Foot[i].LegForce * fabs(sin_theta) + 1000;
//				Ftrack = fclamp(-flimit, Ftrack, flimit);//xxxxxxxxx


				// CHEAT apply half the command to each foot.
				vec3	fc = CommandForce * 0.5;
				float	force_limit = edot * normal_force * surface_factor * (fabsf(sin_theta)) * edge_grip;
				Ftrack = fclamp(0, fc * edge_norm, force_limit);

				vec3	COMDir = (GetLocation() - Foot[i].Location);
				COMDir -= BoardAxis * (COMDir * BoardAxis);
				COMDir.normalize();
				float	torque_limit = (normal_force + force_limit) * (0.1f + fabsf(COMDir * n)) * 0.95f;
				vec3	com_torque = CommandTorque;
				float	tmag = com_torque.magnitude();
				if (tmag > torque_limit) com_torque *= torque_limit / tmag;
				// CHEAT

				
				vec3	EdgeCutForce = edge_norm * Ftrack;

//				AddRedRay(Foot[i].ForceLocation + n*0.5, EdgeCutForce / 200);
				// end NEW
#endif // NEW
				
				
				// Plain old drag.
				vec3	drag = -veldir * (normal_force * pi.SlidingDrag + velmag * velmag * pi.FluidDrag);

				AddGreenRay(Foot[i].ForceLocation + n, n * (normal_force) / 200);
				AddRedRay(Foot[i].ForceLocation + n, EdgeCutForce / 200);
				
				f = n * (normal_force) + EdgeCutForce + drag;

				Torque += com_torque /*CommandTorque*/ * 0.5;	// CHEAT
				Torque += r.cross(f);
				Force += f;

				DragVolume += fabsf((EdgeCutForce + drag) * veldir) /*.magnitude()*/;	// For sound.

				// Particle parameters.
				float	dig = fmax(0.0f, -Foot[i].BoardNormal * veldir);
				float	sink = fclamp(0.0f, SinkDepth * 3, 1.0f);
				float	sign = fclamp(-1.0f, veldir * tr * 20, 1.0f);
				vec3	bc = BoardAxis.cross(Foot[i].BoardNormal);
				psource[i].Valid = true;
				psource[i].Location = Foot[i].Location + bc * sign * 0.20f - n * 0.05f;
				psource[i].Velocity = velp * 0.1f + (bc * sign + n * 0.0f * (1-bc.Y())) * ((fabsf(velp * tr)) * 2.2f * fmin(dig * 10, 1.0f) + 0.5f);
				psource[i].LocationVariance = 0.07f;
				psource[i].VelocityVariance = sqrtf(fabsf(velp * tr)) * 2.6f + 2.0f;
				psource[i].Count = int((fmin(EdgeCutForce.magnitude() * 20, 1500.0f) * dig + fclamp(0, sqrtf(velmag) / 5, 1) * (sink + 0.2f) * 2000.0f) * u.DeltaT);
			}
		}
		
		// Add gravity & air drag.
		Force += vec3(0, -9.8f, 0) * GetMass();	// Gravity.
		float	Speed = GetVelocity().magnitude();
		Force += -GetVelocity() * (Speed * DRAG_COEFF * AIR_DENSITY * 0.5f * FRONTAL_AREA);

		AddRedRay(GetLocation(), Force / 200);//xxxxx
			
		// xxxx Bogus jetpack thrust.
		if (UI::GetMode() == UI::PLAYING
			&& Airborne == false
			&& u.Inputs.Button[1].State
			&& Config::GetBoolValue("EnableJets"))
		{
			vec3	ax = BoardAxis;
			ax.normalize();
			Force += ax * 600;
		}
				
		//
		// Integrate and update object state.
		//

		{
			float	tc;
			if (NewBouyancy > Bouyancy) tc = 0.70f;
			else tc = 0.15f;
			
			float	c0 = expf(-u.DeltaT / tc);
			Bouyancy = fclamp(0, c0 * Bouyancy + (1 - c0) * NewBouyancy, 1);

			Config::SetFloat("Bouyancy", Bouyancy);
		}
		
		LastAccel = Force * InverseMass /* GetInvMass() */;
		vec3	NewVel(GetVelocity() + LastAccel * u.DeltaT);
		LastDerivOmega = InverseInertiaTensor * Torque;
		vec3	NewL(GetAngMomentum() + Torque * (/*GetInvMoment() * */ u.DeltaT));

		// Linear motion.
		vec3	OriginalLocation = GetLocation();
		SetLocation(GetLocation() + (GetVelocity() + NewVel) * (u.DeltaT * 0.5f));	// Trapezoidal integration.

		// Rotational motion.
		vec3	AvgOmega(InverseInertiaTensor * ((GetAngMomentum() + NewL) * 0.5f));
		
		float	OmegaMag = AvgOmega.magnitude();
		if (OmegaMag > 0.000001f) {
			vec3	OmegaDir(AvgOmega / OmegaMag);

			quaternion	o(GetMatrix().GetOrientation());
			quaternion	delt_o(quaternion(0, AvgOmega * 0.5f) * o);
			delt_o *= u.DeltaT;
			o += delt_o;
			o.normalize();
			
			SetOrientation(o);
		}

		SetVelocity(NewVel);
		SetAngMomentum(NewL);

		Omega = InverseInertiaTensor * GetAngMomentum();

		//
		// Take care of sounds.
		//

		bool	UpdateSound = false;
		if (u.Ticks - LastUpdateSound > 25) {
			UpdateSound = true;
			LastUpdateSound = u.Ticks;
		}
		
		// Compute sound parameters.
		if (Airborne) {
			// No drag sound.
			DragVolume = 0;
			DragPitch = 1.0;
		} else {
			// Compute drag sound parameters.
			DragVolume = fclamp(0, DragVolume * InverseMass /* GetInvMass() */ * 0.18f, 1);
			DragPitch = 0.6f + Speed / 80.0f;

			// Decide if we're on ice or snow, and select the sound accordingly.
			int	surf = Foot[0].SurfaceType;
			if (surf < 0) surf = Foot[1].SurfaceType;
			if (surf == 0) {
				DragType = 0;
			} else {
				DragType = 1;
			}
		}

		// Update the snow-drag sound.
		if (SnowSoundID > 0 && IceSoundID > 0) {
			float	c0 = expf(-u.DeltaT / 0.15f);
			FilteredDragVolume = FilteredDragVolume * c0 + DragVolume * (1 - c0);
//			FilteredDragPitch = FilteredDragPitch * c0 + DragPitch * (1 - c0);
			if (UpdateSound) {
				if (DragType == 0) {
					Sound::Adjust(SnowSoundID, Sound::Controls(fclamp(0.0, FilteredDragVolume * 0.7f, 0.7f), 0, 1.0f, true));
					Sound::Adjust(IceSoundID, Sound::Controls(0, 0, 1.0f, true));
				} else {
					Sound::Adjust(SnowSoundID, Sound::Controls(0, 0, 1.0f, true));
					Sound::Adjust(IceSoundID, Sound::Controls(fclamp(0.0f, FilteredDragVolume * 0.7f, 0.7f), 0, 1.0f, true));
				}
			}
		}
		
		// Update the air-drag sound.
		if (AirSoundID > 0) {
			float	vol = fclamp(0.0, sqrtf(Speed / 15.0f), 1.0f);
//			float	pitch = 0.8 + vol * 0.4;
			float	pitch = 1.0f;
			if (UpdateSound) {
				Sound::Adjust(AirSoundID, Sound::Controls(vol, 0, pitch, true));
			}
		}

		// See about making a bump noise...
		if (Airborne == false) {
			vec3	AvgNorm = ZeroVector;
			if (Foot[0].SurfaceType >= 0) AvgNorm += Foot[0].SurfaceNormal;
			if (Foot[1].SurfaceType >= 0) AvgNorm += Foot[1].SurfaceNormal;
			AvgNorm.normalize();
			
			float	ContrarySpeed = GetVelocity() * AvgNorm;
			if (ContrarySpeed < 0) {
				if (LastAirborne && u.Ticks > LastBumpSoundTicks + BUMP_SOUND_HOLDOFF) {
					float	vol = fclamp(0.0f, ContrarySpeed / -15.0f + 0.65f, 1.0f);
					Sound::Play("snowbump.wav", Sound::Controls(vol, 0, 1.0f, false));
					LastBumpSoundTicks = u.Ticks;
				}
			}
		}

		LastAirborne = Airborne;

		//
		// Make some particles.
		//
		if (ParticleSource[0].Valid && ParticleSource[1].Valid && psource[0].Valid && psource[1].Valid) {
			// Particle sources are valid.  So let 'em fly...
			int	count = (ParticleSource[0].Count + ParticleSource[1].Count + psource[0].Count + psource[1].Count) / 4;
			Particle::LineSource(Particle::SNOW_POWDER, ParticleSource[0], psource[0], ParticleSource[1], psource[1], count, u.DeltaTicks);
			ParticleAccumulator += count;
		}
		for (i = 0; i < 2; i++) {
			ParticleSource[i] = psource[i];
		}

		//
		// Move figure.
		//
		for (i = 0; i < DOF_COUNT; i++) {
			DOFLimit&	d = DOFLimits[i];
			float	MaxDelta = u.DeltaT * d.MaxRateOfChange;
			float	delta = fclamp(-MaxDelta, FigureTargets[i] - FigureState[i], MaxDelta);
			FigureState[i] = fclamp(d.Min, FigureState[i] + delta, d.Max);
		}
		
		//
		// Rationalize the ground contact.
		//

		// Locate the feet, and solve the figure position and leg forces based on terrain.
		SolveFeet();

		BoardShadow = true;
		if (Foot[0].SurfaceType >= 0 && Foot[1].SurfaceType >= 0) {
			if (Bouyancy < 0.95) BoardShadow = false;
		}
		
		// Detect crashes.

		// check heights at three or four spots on the body.  Apply impulses to keep those
		// spots above ground.
		int	count = Crashing ? COLLISION_SPHERE_COUNT : COLLISION_SPHERE_COUNT - 3;
		for (i = 0; i < count; i++) {
			CollisionSphereInfo&	c = CollisionSphereOffset[i];
			vec3	v = GetMatrix() * vec3(c.x, c.y, c.z);
			float	y = TerrainModel::GetHeight(v);
			const Surface::PhysicalInfo&	pi = Surface::GetSurfaceInfo(TerrainModel::GetType(v));
			float	penetration = y - (v.Y() - COLLISION_SPHERE_RADIUS) - pi.MaxDepth;
			if (penetration >= 0) {
				if (Crashing == false) {
					Crashing = true;
				}

				v.SetY(y);	// Contact point.
				// Compute velocity reflection.
				float	Restitution = 0.2f;
				vec3	r = v - (GetLocation());
				vec3	vel = GetVelocity() + Omega.cross(r);
				vec3	n = TerrainModel::GetNormal(v);
				float	ContrarySpeed = vel * n;

				vec3	SlideVel = vel - n * (n * vel);
				float	SlideSpeed = SlideVel.magnitude();

				float	ExitPush;
				if (ContrarySpeed < 0) {
					ExitPush = -ContrarySpeed * (1 + Restitution);
				} else {
					ExitPush = 0;
				}

				// Boost the impulse to fix interpenetration.
				ExitPush = fmax(ExitPush, fmin(penetration, 0.2f) * 1.5f);
				
				float	impmag = ExitPush / (InverseMass /* GetInvMass() */ + (n * (InverseInertiaTensor * r.cross(n)).cross(r)));
				vec3	impulse = n * impmag;
				vec3	FrictionImpulse = ZeroVector;
				if (SlideSpeed > 0.0001f) {
					FrictionImpulse = SlideVel / SlideSpeed * impmag * -0.4f;
					impulse += FrictionImpulse;
				}
				vec3	imptorque = r.cross(impulse);

				// Apply impulse.
				SetVelocity(GetVelocity() + impulse * InverseMass /*GetInvMass()*/);
				SetAngMomentum(GetAngMomentum() + imptorque /* * GetInvMoment() */);
				Omega = InverseInertiaTensor * GetAngMomentum();

				// Make some particles.
				Particle::SourceInfo	s;
				vec3	spray = (impulse - FrictionImpulse * 8) / 110.0f;
				float	spraymag = spray.magnitude();
				s.Location = v;
				s.Velocity = spray;
				s.LocationVariance = 0.2f;
				s.VelocityVariance = spraymag * 0.5f;
				Particle::PointSource(Particle::SNOW_POWDER, s, s, (int) fmax(0.0f, (spraymag - 0.3f) * 35.0f), u.DeltaTicks);
				
				AddRedRay(v, impulse * InverseMass /* GetInvMass() */ * 5);	//xxxxxx
			}
		}

		if (Crashing == true && UI::GetMode() == UI::PLAYING) {
			Crash(u);
		}
	}

	static float	Probe(int* SurfaceType, const vec3& v, const vec3& dir, float maxext, float Bouyancy)
	// Probes the max depth of the terrain, starting at v and going in the dir direction,
	// looking for contact.  Goes up to maxext units along dir.  Returns
	// the distance at first hard contact, or a value > maxext if there's no hard contact.
	// Returns the surface type at the max probe point.  -1 if there's no contact at all,
	// or the surface type if there's either hard contact, or "soft" contact; i.e. the probe
	// point is within the top of a compressible surface, but not touching the base.
	{
//		vec3	n = TerrainModel::GetNormal(v);
//		float	tanvel = (velocity - n * (velocity * n)).magnitude();
//		float	normvel = fmax(0.f, -(velocity * n));
//
//		tanvel = 0;//xxx
		
		float	ext = 0;
		float	LastDeltaHeight = 0;
		const float	STEP = 0.20f;
		for ( ; ;) {
			vec3	sample = v + dir * ext;
			int	t = TerrainModel::GetType(sample);
			const Surface::PhysicalInfo&	pi = Surface::GetSurfaceInfo(t);
			float	SurfaceDepth = pi.MaxDepth;
			SurfaceDepth *= 1 - Bouyancy /*fclamp(0, powf((tanvel - normvel*0) / pi.MaxFloatSpeed, 2), 1)*/;
			float	y = TerrainModel::GetHeight(sample) - SurfaceDepth;
			float	dh = y - sample.Y();
			if (dh >= 0) {
				// Hard contact.
				*SurfaceType = t;
				// Compute exact height of contact.
				float	sum_dh = dh - LastDeltaHeight;
				float	f = 1.0;
				if (sum_dh > 0.001) {
					f = -LastDeltaHeight / sum_dh;
				}
				ext -= (1 - f) * STEP;
				return ext;
			}
			LastDeltaHeight = dh;

			// Increment the query.
			ext += STEP;
			if (ext > maxext + STEP) {
				// Time to end the loop.  See if the probe is in "soft contact" or not.
				if (dh + SurfaceDepth >= 0) {
					// Probe is beneath the upper surface, so return the surface type.
					*SurfaceType = t;
				} else {
					*SurfaceType = -1;
				}
				break;
			}
		}

		return maxext + 1;
	}
	
	
	void	SolveFeet()
	// Finds rational values for the foot and leg DOFs, given the command values
	// and the terrain.
	{
		int	i;
		vec3	Offset = ZeroVector;
		
		GArticulated*	a = dynamic_cast<GArticulated*>(Visual);
		if (a) {
			for (i = 0; i < DOF_RENDERING_COUNT; i++) {
				a->SetParameter(i, FigureState[i]);
			}
			a->GetOriginOffset(&Offset);
		}

		int	LimitCount;
		for (LimitCount = 0; LimitCount < 30; LimitCount++) {
			// Find the feet location based on current figure DOFs.
			vec3	BoardAxis = Geometry::Rotate(FigureState[BOARD_YAW], YAxis, XAxis);
			vec3	BoardPitchAxis = BoardAxis.cross(YAxis);
			BoardAxis = Geometry::Rotate(FigureState[BOARD_PITCH], BoardPitchAxis, BoardAxis);
			vec3	BoardCenter(FigureState[LEGS_X], -FigureState[LEGS_Y], FigureState[LEGS_Z]);
			BoardCenter += Offset;
			
			Foot[0].Location = GetMatrix() * (BoardCenter + BoardAxis * BOARD_CONTACT_SAMPLE_SEPARATION * 0.5);
			Foot[1].Location = GetMatrix() * (BoardCenter - BoardAxis * BOARD_CONTACT_SAMPLE_SEPARATION * 0.5);

//			vec3	BoarderDown = -GetMatrix().GetColumn(1);
			
			// Fixup 'foot' pos's to legal values; set foot statuses & normals.
			bool	Contact = false;
			for (i = 0; i < 2; i++) {
				vec3	LegDown = GetMatrix() * BoardCenter - (GetLocation() + Offset);
				LegDown.normalize();
				LegDown = -GetMatrix().GetColumn(1);//xxxx
//				LegDown = -Foot[i].BoardNormal;

				float	maxext = FigureState[LEFT_LEG_MAX_EXT + i];
				float	ext = (Foot[i].Location - (GetLocation() + Offset)) * LegDown /*BoarderDown - Offset.Y()*/;

				int	SurfType;
				float	newext = Probe(&SurfType, Foot[i].Location - LegDown /*BoarderDown*/ * ext, LegDown /* BoarderDown */, maxext, Bouyancy);

				Foot[i].SurfaceType = SurfType;
				if (newext <= maxext) {
					Foot[i].Location += LegDown /*BoarderDown*/ * (newext - ext);
//					Foot[i].SurfaceType = 0;
					Foot[i].HardContact = true;
					Contact = true;
				} else {
					Foot[i].Location += LegDown /*BoarderDown*/ * (maxext - ext);
//					Foot[i].SurfaceType = -1;
					Foot[i].HardContact = false;
				}
			}
			
			// If either foot contacted, then convert tweaked foot positions into figure DOF coords.
			if (Contact || 1) {
				// Avg of foot positions gives LEGS_X, Y, Z.
				vec3	avg;
				GetMatrix().ApplyInverse(&avg, (Foot[0].Location + Foot[1].Location) * 0.5);
				avg -= Offset;
				FigureState[LEGS_X] = fclamp(DOFLimits[LEGS_X].Min, avg.X(), DOFLimits[LEGS_X].Max);
				FigureState[LEGS_Y] = fclamp(DOFLimits[LEGS_Y].Min, -avg.Y(), DOFLimits[LEGS_Y].Max);
				FigureState[LEGS_Z] = fclamp(DOFLimits[LEGS_Z].Min, avg.Z(), DOFLimits[LEGS_Z].Max);
				
				// Figure out the board angles.
				vec3	BoardAxis = Foot[0].Location - Foot[1].Location;
				BoardAxis.normalize();
				vec3	v;
				GetMatrix().ApplyInverseRotation(&v, BoardAxis);
				
				FigureState[BOARD_PITCH] = fclamp(DOFLimits[BOARD_PITCH].Min, asinf(fclamp(-1, v.Y(), 1)), DOFLimits[BOARD_PITCH].Max);
				FigureState[BOARD_YAW] = fclamp(DOFLimits[BOARD_YAW].Min, atan2f(-v.Z(), v.X()), DOFLimits[BOARD_YAW].Max);
			}
			
			vec3	OldOffset = Offset;
			if (a) {
				// Pass DOF values to the figure.
				for (i = 0; i < DOF_RENDERING_COUNT; i++) {
					a->SetParameter(i, FigureState[i]);
				}
				a->GetOriginOffset(&Offset);
			}
			vec3	OffsetDelta = Offset - OldOffset;

			float	off = OffsetDelta.magnitude();
			if (off < 0.001) {	// 1mm error tolerance.
				// Everything's cool; we're done finding the feet.
				break;
			}

			// Move our estimated offset towards the actual offset for this figure position.
			//xxxx
			Offset = OldOffset + OffsetDelta * 0.25;
		}

//		//xxxxxxxx
//		char	buf[200];
//		sprintf(buf, "lc = %d", LimitCount);
//		Text::DrawString(50, 10, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
//		//xxxxxxxxx

		//
		// Now that we know where the feet are, compute other relevant contact info.
		//
		
		// Figure out the board up-axis.
		vec3	BoardAxis = Foot[0].Location - Foot[1].Location;
		BoardAxis.normalize();
		vec3	v;
		GetMatrix().ApplyInverseRotation(&v, BoardAxis);
				
		vec3	w = Geometry::Rotate(FigureState[BOARD_YAW], YAxis, XAxis);
		vec3	r = w.cross(YAxis);
		vec3	up = r.cross(v);
		vec3	BoardNormal;
		GetMatrix().ApplyRotation(&BoardNormal, Geometry::Rotate(FigureState[ANKLE_ANGLE], v, up));
		
		Foot[0].BoardNormal = BoardNormal;
		Foot[1].BoardNormal = BoardNormal;
		Foot[0].BoardForceCoeff = 0.5f + 0.5f * (Foot[0].BoardNormal * Foot[0].SurfaceNormal);
		Foot[1].BoardForceCoeff = 0.5f + 0.5f * (Foot[1].BoardNormal * Foot[1].SurfaceNormal);
		
		vec3	BoarderDown = -GetMatrix().GetColumn(1);

		for (i = 0; i < 2; i++) {
//			float	y = TerrainModel::GetHeight(Foot[i].Location);
			if (Foot[i].SurfaceType != -1) {
				vec3	n = TerrainModel::GetNormal(Foot[i].Location);
				int	t = Surface::GetSurfaceType(Foot[i].Location);
				
//				Contact = true;
//				const Surface::PhysicalInfo&	p = Surface::GetSurfaceInfo(t);
				Foot[i].SurfaceType = t;
				Foot[i].SurfaceNormal = n;
//				if (Foot[i].Location.Y() < y - p.MaxDepth) {
//					Foot[i].Location.SetY(y - p.MaxDepth);
//				}
				Foot[i].ForceLocation = Foot[i].Location;

/*
				const float	MAX_LEG_FORCE = 3000;
				float	ext = (Foot[i].Location - GetLocation()) * BoarderDown;
				float	maxext = FigureState[LEFT_LEG_MAX_EXT + i];
				float	minext = FigureState[LEFT_LEG_MIN_EXT + i];
				float	force0 = FigureState[LEFT_LEG_MIN_FORCE_FACTOR + i];

				if (ext > maxext) {
					Foot[i].LegForce = 0;
				} else if (ext < minext || (maxext - minext) < 0.01) {
					Foot[i].LegForce = MAX_LEG_FORCE;
				} else {
					Foot[i].LegForce = (maxext - ext) / (maxext - minext) * (MAX_LEG_FORCE - force0) + force0;
				}
*/
			}
		}

//		// Offset the location of the foot force, based on where the center of contact with the surface should be due to edging.
//		vec3	CrossBoardAxis;
//		GetMatrix().ApplyRotation(&CrossBoardAxis, Geometry::Rotate(FigureState[ANKLE_ANGLE], v, r));
//		for (i = 0; i < 2; i++) {
//			if (Foot[i].SurfaceType >= 0) {
//				Foot[i].ForceLocation += CrossBoardAxis * fclamp(-1, (Foot[i].SurfaceNormal * CrossBoardAxis) * 2, 1) * -0.15; // Some fraction of 1/2 board width.
//			}
//		}

	}
				

	void	RunLogicPostDynamics(const UpdateState& u)
	// Does post-dynamics logic update.
	{
		// Check to see if we crossed any polygonal regions.
		UI::Mode	m = UI::GetMode();
		if (m == UI::PLAYING || m == UI::COUNTDOWN) {
			vec3	loc = GetLocation();

			PolygonRegion::CheckAndCallTriggers(this, PreUpdateLocation, loc);
		}
		
		// During recording, we need to save the boarder state to the recording module.
		if (Recording::GetMode() == Recording::RECORD) {
			// Save current state to the recording module, for later playback.

			int	bytes = 4 * (3 + 4) + 3;	// Pos & orientation, vel dir, flags.
			bytes += DOF_RENDERING_COUNT;	// One byte per relevant figure degree-of-freedom.
			uint8*	buf = NULL;
			if (Recording::IsReadyForStateData()) {
				buf = Recording::NewChunk(Recording::BOARDER_STATE, bytes);
			}
			if (buf) {
				// Fill in data...

				// Position & orientation.
				int	index = 0;
				const vec3&	v = GetLocation();
				int	i;
				for (i = 0; i < 3; i++) {
					index += EncodeFloat32(buf, index, v.Get(i));
				}
				
				const quaternion	q = GetOrientation();
				for (i = 0; i < 4; i++) {
					index += EncodeFloat32(buf, index, q.Get(i));
				}

				// Figure state.
				for (i = 0; i < DOF_RENDERING_COUNT; i++) {
					DOFLimit&	d = DOFLimits[i];
					uint8	data = uint8((FigureState[i] - d.Min) * 255 / (d.Max - d.Min));
					buf[index++] = data;
				}

				// Velocity direction.
				const vec3&	vel = GetVelocity();
				float	angle = 0;
				if (vel.Z() * vel.Z() + vel.X() * vel.X() > 0.1) {
					angle = atan2f(-vel.Z(), vel.X());
					if (angle < 0) angle += 2*PI;
				}
				index += EncodeUInt16(buf, index, (uint16) (angle * (65536.0 / (2*PI))));

				buf[index++] = (BoardShadow ? 1 : 0);
			}

			//
			// Particle sources.
			//
			uint8	flags = (ParticleSource[0].Valid ? 1 : 0) | (ParticleSource[1].Valid ? 2 : 0);
			int	count = flags;	// count is number of valid particle source structures we need to store.
			if (count >= 2) count -= 1;

			float	ParticleRate;

			bytes = 3 + count * (8 * 1);	// This value depends on the buf[] encoding!
			
			if (buf) {	// Only encode particle data if we encoded a corresponding boarder sample.
				if ((flags & 3) == 3) {
					// Both sources are valid.  Get storage for saving a particle sample in the recording.
					buf = Recording::NewChunk(Recording::BOARDER_PARTICLES, bytes);

					// Particle rate is how many particles we've emitted since the
					// last recording sample, divided by the time.
					if (LastParticleSampleTicks == 0 || LastParticleSampleTicks == u.Ticks) {
						// No valid previous sample.
						ParticleRate = 0;
					} else {
						ParticleRate = ParticleAccumulator * 1000.0f / (u.Ticks - LastParticleSampleTicks);
					}
					LastParticleSampleTicks = u.Ticks;
					ParticleAccumulator = 0;

				} else {
					// One or both sources is invalid.  Breaks our sampling continuity, for
					// calculating the particle emission rate.
					buf = 0;	// Don't bother encoding anything.
					LastParticleSampleTicks = 0;
					ParticleAccumulator = 0;
				}
			} else {
				buf = 0;
			}
			if (buf) {
				// Fill in the particle data.
				
				int	index = 0;
				int	i, j;

				buf[index++] = flags;

				// Encode the average rate at which we're emitting particles.
				index += EncodeUInt16(buf, index, (uint16) ParticleRate);

				const vec3&	boarder_location = GetLocation();

				for (j = 0; j < 2; j++) {
					// Encode source details.
					ParticleInfo&	p = ParticleSource[j];
					const vec3*	v = &(p.Location);
					// Encode particle source location as an offset from the boarder location.
					for (i = 0; i < 3; i++) {
						buf[index++] = (uint8) (int8) iclamp(-128, (int) ((v->Get(i) - boarder_location.Get(i)) * 64), 127);
					}

					v = &(p.Velocity);
					for (i = 0; i < 3; i++) {
						buf[index++] = (uint8) (int8) iclamp(-128, (int) (v->Get(i) * 2), 127);
					}
					buf[index++] = (uint8) (log(p.LocationVariance + 1) * 50.0);
					buf[index++] = (uint8) (log(p.VelocityVariance + 1) * 50.0);
				}
			}
		}
	}


	void	TakeCheckpoint(int Ticks)
	// Records the current state of the boarder in the circular
	// checkpoint buffer, for later restoration upon call to
	// ResumeFromCheckpoint().
	{
		CheckpointInfo&	c = Checkpoint[NextCheckpoint++];
		if (NextCheckpoint >= CP_COUNT) NextCheckpoint = 0;

		c.Ticks = Ticks;
		c.Location = GetLocation();
		c.Orientation = GetOrientation();
		c.Velocity = GetVelocity();
		c.AngMomentum = GetAngMomentum();
		c.Bouyancy = Bouyancy;
		c.Points = Game::GetTotalScore(); // include flying points.
		c.RunTimer = Game::GetTimer();

		int	i;
		for (i = 0; i < DOF_COUNT; i++) {
			c.Figure[i] = FigureState[i];
		}

		// Checkpoint the controller state.
		if (Controller) {
			Controller->EncodeCheckpoint(c.ControllerBuffer, 0);
		}
	}

  //void	Update(const UpdateState& u) {}
};


Snowboarder*	BoarderInstance[] = {NULL, NULL, NULL, NULL};


void	Boarder::Clear()
// Make sure our BoarderInstance is cleared.
{
	// Don't delete, since the Model db actually owns this pointer and
	// will delete it when it is cleared.
	for (int i=0; i<MultiPlayer::NumberOfLocalPlayers(); i++)
	  BoarderInstance[i] = NULL;
}



// StartLocation is for marking the location of the start of a run.
class StartLocation : virtual public RunStarter {
	int	RunIndex;
	char*	RunName;
	char*	RunInfoText;
	int	ParSeconds;
public:
	StartLocation()
	// Construct the start location.
	{
		RunIndex = 0;
		RunName = NULL;
		RunInfoText = NULL;
	}

	virtual ~StartLocation() {
		if (RunName) delete [] RunName;
		if (RunInfoText) delete [] RunInfoText;
	}
	
	void	LoadExtras(FILE* fp)
	// Loads the run index, run name, etc. (the stuff beyond the basic object data)
	{
		// Load the Run ID.
		int	ID = Read32(fp);
		RunIndex = ID - 1;

		// Load the run name.
		char	buf[5000];
		fgets(buf, 5000, fp);
		// Delete trailing '\n'.
		buf[strlen(buf) - 1] = 0;

		const char*	name = buf;

		// Assign the run name.
		if (RunName) delete [] RunName;
		RunName = new char[strlen(name) + 1];
		strcpy(RunName, name);

		// Load descriptive text.
		fgets(buf, 5000, fp);
		// Delete trailing '\n'.
		buf[strlen(buf) - 1] = 0;

		const char*	desc = buf;

		// Assign text.
		if (RunInfoText) delete [] RunInfoText;
		RunInfoText = new char[strlen(desc) + 1];
		strcpy(RunInfoText, desc);

		// Load and init the orientation.
		LoadOrientation(fp);

		// Load par seconds.
		int32	temp = Read32(fp);
		ParSeconds = temp;
	}

	void	PostLoad(int runIndex)
	// Look for overloads (for translations).
	// runIndex is 0-based.
	{
		// Is there an overload for the run name?
		{
			const int	BUFLEN = 1000;
			char	querybuf[BUFLEN];
			snprintf(querybuf, BUFLEN, "text.%s.run.%d.name", Config::Get("Language"), runIndex + 1);
			const char*	overload = Config::Get(querybuf);
			if (overload)
			{
				delete [] RunName;
				RunName = new char[strlen(overload) + 1];
				strcpy(RunName, overload);
			}
		}

		// Is there an overload for the run description?
		{
			const int	BUFLEN = 1000;
			char	querybuf[BUFLEN];
			snprintf(querybuf, BUFLEN, "text.%s.run.%d.desc", Config::Get("Language"), runIndex + 1);
			const char*	overload = Config::Get(querybuf);
			if (overload)
			{
				delete [] RunInfoText;
				RunInfoText = new char[strlen(overload) + 1];
				strcpy(RunInfoText, overload);
			}
		}
		
	}
	
	int	GetRunIndex() { return RunIndex; }

	void	ResetUser(MObject* User)
	// Called by Game:: when the user should be reset to the start of our run.
	{
		User->Reset(GetLocation(), GetDirection(), GetDirection() * 2 + YAxis * 1);
	}

	const char*	GetRunName()
	// Returns the name of this run.
	{
		return RunName;
	}

	const char*	GetRunInfoText()
	// Returns descriptive text about the run.
	{
		return RunInfoText;
	}

	int	GetRunParTicks()
	// Returns the par time for finishing this run.
	{
		return ParSeconds * 1000;
	}
	
  void	Update(const UpdateState& u) {}
};



Render::Texture*	Shadow[2] = { NULL, NULL };
bitmap32*	ShadowMap[2] = { NULL, NULL };
int	ActiveShadow = 0;


void	Boarder::RenderShadow(const ViewState& s)
// Draw the snowboarder's shadow.
{
  for (int player_index=0; player_index < MultiPlayer::NumberOfLocalPlayers(); player_index++){
	if (BoarderInstance[player_index] == NULL) return;
	Snowboarder&	b = *BoarderInstance[player_index];
	
	BoarderModel*	model = dynamic_cast<BoarderModel*>(b.GetVisual());
	if (model == NULL) return;

	// Estimate where the center of the shadow should be.
	vec3	v = b.GetLocation();

	// Prepare the shadow texture.
	if (Shadow[0] == NULL) {
		int	i;
		for (i = 0; i < 2; i++) {
			ShadowMap[i] = new bitmap32(128, 128);
			memset(ShadowMap[i]->GetData(), 0, ShadowMap[i]->GetWidth() * ShadowMap[i]->GetHeight() * sizeof(uint32));
			Shadow[i] = Render::NewTextureFromBitmap(ShadowMap[i], true, false, false);
		}
	}

	// Figure out how the shadow image plane is oriented.
	vec3	Sun = Weather::GetSunDirection();
	
	vec3	up = BoarderInstance[player_index]->GetUp();
	vec3	ShadowUp = Sun.cross(up).cross(Sun);//ShadowRight.cross(Sun);
	ShadowUp.normalize();

	// Draw into the shadow map.  Transform the shadow plane into object coords.
	const matrix&	mat = BoarderInstance[player_index]->GetMatrix();
	vec3	obj_light_dir, obj_shadow_up;
	mat.ApplyInverseRotation(&obj_light_dir, Sun);
	mat.ApplyInverseRotation(&obj_shadow_up, ShadowUp);
	model->PaintShadow(ShadowMap[ActiveShadow], 3.2f, obj_light_dir, obj_shadow_up, b.BoardShadow);
	
	// Load the new image into a texture map.
	Shadow[ActiveShadow]->ReplaceImage(ShadowMap[ActiveShadow]);

//	// Switch to the other shadow map for the next frame.  xxx or not?
//	ActiveShadow ^= 1;
	
	Render::SetTexture(Shadow[ActiveShadow]);
	
	Render::DisableAlphaTest();
	Render::CommitRenderState();
	glColor4f(1, 1, 1, 1);
	glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	// Set up texgen.  We want to project the shadow along the sun direction.
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	ShadowUp *= (1 / 3.2);
	
	vec3	ShadowRight = Sun.cross(ShadowUp);
	ShadowRight.normalize();
	ShadowRight *= (1 / 3.2);

	// Turn on texgen, and set up w/ parameters to project the shadow map.
	float	H = TerrainMesh::GetHeightToMetersFactor();
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	float	p[4] = { ShadowRight.X(), ShadowRight.Y() * H, ShadowRight.Z(), -(ShadowRight * v) + 0.5f };
	glTexGenfv(GL_S, GL_OBJECT_PLANE, p);
	
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	p[0] = ShadowUp.X();	p[1] = ShadowUp.Y() * H;	p[2] = ShadowUp.Z();	p[3] = -(ShadowUp * v) + 0.5f;
	glTexGenfv(GL_T, GL_OBJECT_PLANE, p);

	float	extent = 4;

	// Draw the shadow using terrain polys.
	TerrainMesh::RenderShadow(s, v, extent);

	// Turn off texgen.
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
  }
}


static struct BoarderInit {
	BoarderInit() {
		GameLoop::AddInitFunction(Init);
	}

	static void	Init()
	// Attach a loader.
	{
		Model::AddObjectLoader(2, LoadResetLocation);
		Model::AddObjectLoader(6, LoadTestBoarder);
	}

	static void	LoadResetLocation(FILE* fp)
	// Creates and initializes a reset location from the given stream.
	// Also creates a Boarder instance if one is needed.
	{
		// Create the StartLocation instance.
		StartLocation*	s = new StartLocation;

		int	dummy = Read32(fp);
		
		float	x = ReadFloat(fp), y = ReadFloat(fp), z = ReadFloat(fp);
		x -= 32768;
		z -= 32768;
		s->SetLocation(vec3(x, y, z));

		// Load the visual index.
		int	VisualIndex = Read32(fp);

		// Load the solid index.
		int	SolidIndex = Read32(fp);

		s->LoadExtras(fp);
		
		// Link into the database.
		Model::AddStaticObject(s);

		// Register with Game::
		if (s->GetRunIndex() >= 0) {
			Game::RegisterRunStarter(s, s->GetRunIndex());
		}
		
		//
		// Boarder.
		//
		
		for (int player_index=0; player_index<MultiPlayer::NumberOfLocalPlayers(); player_index++){
		  MultiPlayer::SetCurrentPlayerIndex(player_index);
		  if (UserCreated[player_index] == false
		      && Config::GetBoolValue("PerfTest") == false
		      && Config::GetBoolValue("Camera") == false)
		    {
			Snowboarder*	b = new Snowboarder();
			BoarderInstance[player_index] = b;
			b->AttachController(NewController());

			b->SetVisual(Model::GetGModel(VisualIndex));
			b->SetSolid(Model::GetSModel(SolidIndex));
			
//			b->LoadLocation(fp);

			Model::AddDynamicObject(b);

//			UserCamera::LookAtAndRotate(b);	//xxxxx
//			UserCamera::LookAt(b);	//xxxxx
			UserCamera::SetSubject(b);
			UserCamera::SetAimMode(UserCamera::LOOK_AT);
			UserCamera::SetMotionMode(UserCamera::CHASE);

			Game::RegisterUser(b);

			s->ResetUser(b);
		    }

		  UserCreated[player_index] = true;
		}
	}


	static void	LoadTestBoarder(FILE* fp)
	// Loads and initializes the stick man (for testing).  It's the same object
	// as boarder, but the data in the file is a little different.

	// This part is NOT updated for multi player mode

	{
		int player_index = 0;
		Snowboarder*	b = new Snowboarder();
		BoarderInstance[player_index] = b;
		b->AttachController(NewController());
		
		b->LoadLocation(fp);
		
		Model::AddDynamicObject(b);
		
		UserCamera::SetSubject(b);
		UserCamera::SetAimMode(UserCamera::LOOK_AT);
		UserCamera::SetMotionMode(UserCamera::CHASE);

//		UserCamera::LookAtAndRotate(b);	//xxxxx
//		UserCamera::LookAt(b);	//xxxxx
		
//		Game::RegisterUser(b);
		
//		s->ResetUser(b);
	}
	
} BoarderInit;

