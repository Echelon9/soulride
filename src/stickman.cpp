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
// stickman.cpp	-thatcher 7/11/1999 Copyright Slingshot Game Technology

// GModel of an articulated snowboarder figure.  For boarder figure
// physics and animation.


#include <math.h>

#ifdef MACOSX 
#include "macosxworkaround.hpp" 
#endif

#include "utility.hpp"
#include "model.hpp"
#include "gameloop.hpp"
#include "boarder.hpp"
#include "ogl.hpp"
#include "render.hpp"
#include "config.hpp"


using namespace Boarder;


const float	BoardLength = 1.65f;
const float	BoardWidth = 0.3f;
const float	StanceWidth = 0.5f;
const float	LeftFootAngle = 0.30f;
const float	RightFootAngle = 0.18f;
const float	FootLength = 0.25f;
const float	AnkleOffset = 0.15f;
const float	LegLength = 1.0f;
const float	HipWidth = 0.22f;
const float	BackLength = 0.6f;
const float	HeadHeight = 0.25f;
const float	ShoulderWidth = 0.3f;
const float	ArmLength = 0.65f;


enum PartID {
	BOARD = 0,
	LEGLL,
	LEGLR,
	LEGUL,
	LEGUR,
	TORSOL,
	TORSOU,
	ARMLL,
	ARMLR,
	ARMUL,
	ARMUR,
	PART_COUNT
};


// Mass of each body part.
float	PartMass[PART_COUNT] = {
	5,	// board, boots and feet
	7,
	7,
	9,
	9,
	12,
	20,
	2.75,
	2.75,
	3.5,
	3.5,
};


matrix33	PartMoment[PART_COUNT];


void	BoxMoment(matrix33* m, float mass, float xdim, float ydim, float zdim)
// Fills *m with the inertia moment values for a box of the given mass, with
// the given extents in x,y,z.
{
	(*m).Identity();
	(*m)[0][0] = (ydim * ydim + zdim * zdim) / 12 * mass;
	(*m)[1][1] = (xdim * xdim + zdim * zdim) / 12 * mass;
	(*m)[2][2] = (xdim * xdim + ydim * ydim) / 12 * mass;
}


static void	InitPartMoment()
// Initialize the body-part inertia moments.
{
	BoxMoment(&PartMoment[BOARD], PartMass[BOARD], BoardLength, BoardWidth, 0.10f);	// Doesn't properly account for concentration of mass at the boots/feet.
	BoxMoment(&PartMoment[LEGLL], PartMass[LEGLL], 0.12f, LegLength * 0.5f, 0.12f);
	BoxMoment(&PartMoment[LEGLR], PartMass[LEGLR], 0.12f, LegLength * 0.5f, 0.12f);
	BoxMoment(&PartMoment[LEGUL], PartMass[LEGUL], 0.15f, LegLength * 0.5f, 0.15f);
	BoxMoment(&PartMoment[LEGUR], PartMass[LEGUR], 0.15f, LegLength * 0.5f, 0.15f);
	BoxMoment(&PartMoment[TORSOL], PartMass[TORSOL], 0.37f, BackLength * 0.33f, 0.22f);
	BoxMoment(&PartMoment[TORSOU], PartMass[TORSOU], 0.40f, BackLength * 0.67f, 0.22f);
	BoxMoment(&PartMoment[ARMLL], PartMass[ARMLL], 0.07f, ArmLength * 0.5f, 0.07f);
	BoxMoment(&PartMoment[ARMLR], PartMass[ARMLR], 0.07f, ArmLength * 0.5f, 0.07f);
	BoxMoment(&PartMoment[ARMUL], PartMass[ARMUL], 0.08f, ArmLength * 0.5f, 0.10f);
	BoxMoment(&PartMoment[ARMUR], PartMass[ARMUR], 0.08f, ArmLength * 0.5f, 0.10f);
}


// Points and vectors needed for rendering the figure.  Computed given the
// DOFs.
struct FigureRenderInfo {
	vec3	MiddleHip;
	vec3	HipAxis;
	vec3	LeftHip;
	vec3	RightHip;
	vec3	BoardAxis;
	vec3	BoardUpAxis;
	vec3	BoardCenter;
	vec3	LeftHeel;
	vec3	RightHeel;
	vec3	LeftToe;
	vec3	RightToe;
	vec3	LeftKneeBendAxis;
	vec3	RightKneeBendAxis;
	vec3	LeftKnee;
	vec3	RightKnee;
	vec3	BackAxis;
	vec3	MiddleShoulder;
	vec3	ShoulderAxis;
	vec3	LeftShoulder;
	vec3	RightShoulder;
	vec3	LeftElbow;
	vec3	RightElbow;
	vec3	LeftArmBendAxis;
	vec3	RightArmBendAxis;
	vec3	LeftHand;
	vec3	RightHand;
	vec3	HeadTop;

};


class StickMan : public BoarderModel {
	float	DOF[DOF_COUNT];

	GModel*	Part[PART_COUNT];	// Different body parts.
	
public:
	StickMan()
	// Constructor.
	{
		// Initial values for DOFs.
		DOF[ANKLE_ANGLE] = 0;
		DOF[BOARD_YAW] = 0;
		DOF[BOARD_PITCH] = 0;
		DOF[LEGS_Y] = LegLength * 0.9f;
		DOF[LEGS_X] = 0;
		DOF[LEGS_Z] = 0;
		DOF[BACK_TWIST_ANGLE] = 1;
		DOF[ARM_L_EXTENSION] = ArmLength * 0.9f;
		DOF[ARM_R_EXTENSION] = ArmLength * 0.9f;
		DOF[ARM_L_VERT_ANGLE] = -1;
		DOF[ARM_R_VERT_ANGLE] = -1;
		DOF[ARM_L_FORWARD_ANGLE] = 0;
		DOF[ARM_R_FORWARD_ANGLE] = 0;
		DOF[BACK_LEAN_ANGLE] = 0.1f;
		DOF[BACK_PITCH_ANGLE] = 0.1f;

		int	i;
		for (i = 0; i < PART_COUNT; i++) {
			Part[i] = 0;
		}

		Part[BOARD] = Model::LoadGModel("figure" PATH_SEPARATOR "board-and-feet.srm");
		Part[LEGLL] = Model::LoadGModel("figure" PATH_SEPARATOR "legll.srm");
		Part[LEGLR] = Model::LoadGModel("figure" PATH_SEPARATOR "leglr.srm");
		Part[LEGUL] = Model::LoadGModel("figure" PATH_SEPARATOR "legul.srm");
		Part[LEGUR] = Model::LoadGModel("figure" PATH_SEPARATOR "legur.srm");
		Part[TORSOL] = Model::LoadGModel("figure" PATH_SEPARATOR "torsol.srm");
		Part[TORSOU] = Model::LoadGModel("figure" PATH_SEPARATOR "torsou.srm");
		Part[ARMLL] = Model::LoadGModel("figure" PATH_SEPARATOR "armll.srm");
		Part[ARMLR] = Model::LoadGModel("figure" PATH_SEPARATOR "armlr.srm");
		Part[ARMUL] = Model::LoadGModel("figure" PATH_SEPARATOR "armul.srm");
		Part[ARMUR] = Model::LoadGModel("figure" PATH_SEPARATOR "armur.srm");
		
		// Set the radius (conservatively).
		Radius = 2;
	}

	virtual ~StickMan()
	// Destructor.
	{
		int	i;
		for (i = 0; i < PART_COUNT; i++) {
			delete Part[i];
		}
	}

	void	SetParameter(int index, float Value)
	// Sets the value of a DOF.
	{
		if (index >= 0 && index < DOF_COUNT) {
			DOF[index] = Value;
		}
	}


	static matrix33	ShiftMatrix(const vec3& v)
	// Returns the tensor offset matrix, which gives the inertia tensor of a particle
	// with mass 1 with COM at the given offset.
	{
		matrix33	m;
		float	x = v.X();
		float	y = v.Y();
		float	z = v.Z();
		float	x2 = x * x;
		float	y2 = y * y;
		float	z2 = z * z;
		m.SetColumns(vec3(y2 + z2, -y*x, -z*x), vec3(-x*y, x2 + z2, -z*y), vec3(-x*z, -y*z, x2 + y2));
		return m;
	}


	void	ComputeCOMInfo(vec3* HipOffset, float* TotalMass, vec3 PartCOM[PART_COUNT], const FigureRenderInfo& f)
	// Computes the location of the hips, relative to the center of
	// mass/object origin, based on the current figure state given
	// in f.  Also computes total mass, and the COMs of each figure
	// part.
	{
		PartCOM[BOARD] = f.BoardCenter;

		// Left leg.
		PartCOM[LEGLL] = (f.LeftKnee + f.LeftHeel) * 0.5;

		PartCOM[LEGUL] = (f.LeftHip + f.LeftKnee) * 0.5;

		// Right leg.
		PartCOM[LEGLR] = (f.RightKnee + f.RightHeel) * 0.5;

		PartCOM[LEGUR] = (f.RightHip + f.RightKnee) * 0.5;

		// Torso.
		PartCOM[TORSOL] = ZeroVector;
		
		PartCOM[TORSOU] = (f.MiddleHip + f.MiddleShoulder) * 0.5;

		// Left arm.
		PartCOM[ARMUL] = (f.LeftShoulder + f.LeftElbow) * 0.5;

		PartCOM[ARMLL] = (f.LeftElbow + f.LeftHand) * 0.5;

		// Right arm.
		PartCOM[ARMUR] = (f.RightShoulder + f.RightElbow) * 0.5;

		PartCOM[ARMLR] = (f.RightElbow + f.RightHand) * 0.5;

		//
		// Compute aggregate values.
		//
		*TotalMass = 0;
		vec3	COMSum(0,0,0);

		// Find the center of mass.
		int	i;
		for (i = 0; i < PART_COUNT; i++) {
			*TotalMass += PartMass[i];
			COMSum += PartCOM[i] * PartMass[i];
		}

		*HipOffset = COMSum / -*TotalMass;
	}
	

	void	GetDynamicsParameters(vec3* HipOffset, float* InverseMass, matrix33* InverseInertiaTensor)
	// Fills the specified variables with the values of some key
	// dynamics parameters.  HipOffset is the displacement of the
	// hips (i.e. the point on the figure which all other values
	// reference) to the object's actual center of mass; InverseMass
	// is 1 / Mass, and InverseInertiaTensor is the inverse of the
	// 3x3 inertia tensor matrix, with respect to the center of mass.
	{
		FigureRenderInfo	f;
		ComputeRenderInfo(&f);

		//
		// Compute dynamics parameters.
		//

		// Start by getting the mass and hip offset.
		float	TotalMass;
		vec3	PartCOM[PART_COUNT];
		ComputeCOMInfo(HipOffset, &TotalMass, PartCOM, f);
		
		*InverseMass = 1.0f / TotalMass;
		
		// for each part, compute contribution to TotalMass and I.
		// needed values:
		//   Rpart (origin, dir, up)
		//   PartOrigin
		//   Mpart
		//   Ipart

		matrix33	PartR[PART_COUNT];

		PartR[BOARD].Orient(f.BoardAxis, f.BoardUpAxis);

		// Left leg.
		vec3	up = (f.LeftKnee - f.LeftHeel).normalize();
		vec3	dir = f.LeftKneeBendAxis /* .cross(up) */;
		PartR[LEGLL].Orient(dir, up);

		up = (f.LeftHip - f.LeftKnee).normalize();
		dir = f.LeftKneeBendAxis/* .cross(up) */;
		PartR[LEGUL].Orient(dir, up);

		// Right leg.
		up = (f.RightKnee - f.RightHeel).normalize();
		dir = f.RightKneeBendAxis /* .cross(up) */;
		PartR[LEGLR].Orient(dir, up);

		up = (f.RightHip - f.RightKnee).normalize();
		dir = f.RightKneeBendAxis/* .cross(up) */;
		PartR[LEGUR].Orient(dir, up);

		// Torso.
		PartR[TORSOL].SetColumns(ZeroVector, ZeroVector, ZeroVector);
		
		PartR[TORSOU].Orient(f.HipAxis, f.BackAxis);

		// Left arm.
		up = (f.LeftShoulder - f.LeftElbow).normalize();
		dir = f.LeftArmBendAxis;
		PartR[ARMUL].Orient(dir, up);

		up = (f.LeftElbow - f.LeftHand).normalize();
		dir = f.LeftArmBendAxis;
		PartR[ARMLL].Orient(dir, up);

		// Right arm.
		up = (f.RightShoulder - f.RightElbow).normalize();
		dir = f.RightArmBendAxis;
		PartR[ARMUR].Orient(dir, up);

		up = (f.RightElbow - f.RightHand).normalize();
		dir = f.RightArmBendAxis;
		PartR[ARMLR].Orient(dir, up);

		// Find the inertia moment about the COM.
		matrix33	I;
		I.SetColumns(ZeroVector, ZeroVector, ZeroVector);
		int	i;
		for (i = 0; i < PART_COUNT; i++) {
			matrix33	Ipart = ShiftMatrix(PartCOM[i] + *HipOffset);
			Ipart *= PartMass[i];
			Ipart += Geometry::RotateTensor(PartR[i], PartMoment[i]);
			I += Ipart;
		}

		// More results.
		I.Invert();
		*InverseInertiaTensor = I;
	}


	void	GetOriginOffset(vec3* HipOffset)
	// HipOffset is the displacement of the hips (i.e. the point on
	// the figure which all other values reference) to the object's
	// actual center of mass.
	{
		FigureRenderInfo	f;
		ComputeRenderInfo(&f);

		// Start by getting the mass and hip offset.
		float	TotalMass;
		vec3	PartCOM[PART_COUNT];
		ComputeCOMInfo(HipOffset, &TotalMass, PartCOM, f);
	}		

	
	void	ComputeRenderInfo(FigureRenderInfo* pf)
	// Compute the many points and vectors used to render the figure, based on the values
	// of the DOFs.
	{
		// Assume origin is in the middle of the hips.
		// +x is towards the right leg, -x towards the left.
		// +y is up.  +z is to the front of the boarder's body.
		// boarder normally travels either towards -x or +x (depending on goofiness).

		FigureRenderInfo&	f = *pf;

		f.MiddleHip = ZeroVector - ZAxis * (0.06f + 0.3f * DOF[BACK_PITCH_ANGLE]);
		f.MiddleHip -= XAxis * sinf(DOF[BACK_TWIST_ANGLE]) * sinf(DOF[BACK_TWIST_ANGLE]) * 0.35f * BackLength;
		f.HipAxis = Geometry::Rotate(DOF[BACK_TWIST_ANGLE] * 0.55f, YAxis, XAxis);
		f.LeftHip = f.MiddleHip + f.HipAxis * 0.5f * HipWidth;
		f.RightHip = f.MiddleHip - f.HipAxis * 0.5f * HipWidth;

		f.BoardCenter = vec3(DOF[LEGS_X], -DOF[LEGS_Y], DOF[LEGS_Z]);
		
		f.BoardAxis = Geometry::Rotate(DOF[BOARD_YAW], YAxis, XAxis); //(DOF[LF_X] - DOF[RF_X], DOF[LF_Y] - DOF[RF_Y], DOF[LF_Z] - DOF[RF_Z]);
		vec3	BoardPitchAxis = f.BoardAxis.cross(YAxis);

		f.BoardAxis = Geometry::Rotate(DOF[BOARD_PITCH], BoardPitchAxis, f.BoardAxis);
		vec3	CrossBoardAxis = Geometry::Rotate(DOF[ANKLE_ANGLE], f.BoardAxis, BoardPitchAxis);
		f.BoardUpAxis = CrossBoardAxis.cross(f.BoardAxis);
		
		vec3	LeftFootAxis = Geometry::Rotate(LeftFootAngle, f.BoardUpAxis, CrossBoardAxis);
		vec3	LeftFootCenter = f.BoardCenter + f.BoardAxis * StanceWidth * 0.5 + f.BoardUpAxis * AnkleOffset;
		f.LeftHeel = LeftFootCenter - LeftFootAxis * 0.5 * FootLength;
		vec3	RightFootAxis = Geometry::Rotate(RightFootAngle, f.BoardUpAxis, CrossBoardAxis);
		vec3	RightFootCenter = f.BoardCenter - f.BoardAxis * StanceWidth * 0.5 + f.BoardUpAxis * AnkleOffset;
		f.RightHeel = RightFootCenter - RightFootAxis * 0.5 * FootLength;

		f.LeftToe = f.LeftHeel + LeftFootAxis * FootLength;
		f.RightToe = f.RightHeel + RightFootAxis * FootLength;
		
		float	LLTheta = acosf(fclamp(0, (f.LeftHip - f.LeftHeel).magnitude() / LegLength, 1));
		vec3	LeftLegBendDir = Geometry::Rotate((DOF[BOARD_YAW] + LeftFootAngle) * 0.5f, YAxis, ZAxis);
		f.LeftKneeBendAxis = LeftLegBendDir.cross(YAxis);
		LeftLegBendDir = (f.LeftHip - f.LeftHeel).cross(f.LeftKneeBendAxis).normalize();
		f.LeftKnee = f.LeftHip + (f.LeftHeel - f.LeftHip) * 0.5f + LeftLegBendDir * 0.5f * LegLength * sinf(LLTheta);

		float	RLTheta = acosf(fclamp(0, (f.RightHip - f.RightHeel).magnitude() / LegLength, 1));
		vec3	RightLegBendDir = Geometry::Rotate((DOF[BOARD_YAW] + RightFootAngle) * 0.5f, YAxis, ZAxis);
		f.RightKneeBendAxis = RightLegBendDir.cross(YAxis);
		RightLegBendDir = (f.RightHip - f.RightHeel).cross(f.RightKneeBendAxis).normalize();
		f.RightKnee = f.RightHip + (f.RightHeel - f.RightHip) * 0.5f + RightLegBendDir * 0.5f * LegLength * sinf(RLTheta);

		// Upper body.
		
		f.BackAxis = Geometry::Rotate(DOF[BACK_LEAN_ANGLE], ZAxis, YAxis);
		f.BackAxis = Geometry::Rotate(DOF[BACK_PITCH_ANGLE], XAxis/*f.HipAxis*/, f.BackAxis);
		f.MiddleShoulder = f.MiddleHip + f.BackAxis * BackLength;

		f.ShoulderAxis = Geometry::Rotate(DOF[BACK_LEAN_ANGLE], ZAxis, XAxis);
		f.ShoulderAxis = Geometry::Rotate(DOF[BACK_PITCH_ANGLE], XAxis, f.ShoulderAxis);
		f.ShoulderAxis = Geometry::Rotate(DOF[BACK_TWIST_ANGLE], f.BackAxis, f.ShoulderAxis);
		
		
		f.LeftShoulder = f.MiddleShoulder + f.ShoulderAxis * 0.5 * ShoulderWidth;
		f.RightShoulder = f.MiddleShoulder - f.ShoulderAxis * 0.5 * ShoulderWidth;

		vec3	LeftUpperArmAxis = (f.ShoulderAxis * cosf(DOF[ARM_L_VERT_ANGLE]) + f.BackAxis * sinf(DOF[ARM_L_VERT_ANGLE]));
		f.LeftArmBendAxis = LeftUpperArmAxis.cross(f.ShoulderAxis.cross(f.BackAxis));
		LeftUpperArmAxis = Geometry::Rotate(DOF[ARM_L_FORWARD_ANGLE], f.LeftArmBendAxis, LeftUpperArmAxis);
		f.LeftElbow = f.LeftShoulder + LeftUpperArmAxis * 0.5 * ArmLength;
		
		vec3	RightUpperArmAxis = (-f.ShoulderAxis * cosf(DOF[ARM_R_VERT_ANGLE]) + f.BackAxis * sinf(DOF[ARM_R_VERT_ANGLE]));
		f.RightArmBendAxis = RightUpperArmAxis.cross(f.ShoulderAxis.cross(f.BackAxis));
		RightUpperArmAxis = Geometry::Rotate(DOF[ARM_R_FORWARD_ANGLE], f.RightArmBendAxis, RightUpperArmAxis);
		f.RightElbow = f.RightShoulder + RightUpperArmAxis * 0.5 * ArmLength;

//		f.LeftArmBendAxis = LeftUpperArmAxis.cross(f.BackAxis.cross(f.ShoulderAxis));
		float	ArmLTheta = acosf(DOF[ARM_L_EXTENSION] / ArmLength * 2 - 1);
		f.LeftHand = f.LeftElbow + Geometry::Rotate(ArmLTheta, f.LeftArmBendAxis, LeftUpperArmAxis) * 0.5 * ArmLength;

//		f.RightArmBendAxis = -RightUpperArmAxis.cross(f.BackAxis.cross(f.ShoulderAxis));
		float	ArmRTheta = acosf(DOF[ARM_R_EXTENSION] / ArmLength * 2 - 1);
		f.RightHand = f.RightElbow + Geometry::Rotate(ArmRTheta, f.RightArmBendAxis, RightUpperArmAxis) * 0.5 * ArmLength;

		f.HeadTop = f.MiddleShoulder + f.BackAxis * HeadHeight;
	}

	
	void	ApplyOffset(FigureRenderInfo* pf, vec3 off)
	// Adds the given offset to the figure points in *pf.
	{
		pf->MiddleHip += off;
		pf->HipAxis += off;
		pf->LeftHip += off;
		pf->RightHip += off;
		pf->BoardCenter += off;
		pf->LeftHeel += off;
		pf->RightHeel += off;
		pf->LeftToe += off;
		pf->RightToe += off;
		pf->LeftKnee += off;
		pf->RightKnee += off;
		pf->MiddleShoulder += off;
		pf->LeftShoulder += off;
		pf->RightShoulder += off;
		pf->LeftElbow += off;
		pf->RightElbow += off;
		pf->LeftHand += off;
		pf->RightHand += off;
		pf->HeadTop += off;
	}


	vec3	GetHandLocation(int hand)
	// Return the object-coordinates location of the desired hand (0 --> left, 1 --> right).
	{
		FigureRenderInfo	f;
		
		ComputeRenderInfo(&f);

		// Find the hips.
		vec3	HipOffset;
		float	TotalMass;
		vec3	PartCOM[PART_COUNT];
		ComputeCOMInfo(&HipOffset, &TotalMass, PartCOM, f);

		// Offset the figure state by the hip offset.
		ApplyOffset(&f, HipOffset);

		// Return computed hand location.
		if (hand == 0) return f.LeftHand;
		else return f.RightHand;
	}
	
	void	Render(ViewState& s, int ClipHint)
	// Render the figure, with limbs positioned according to DOF values.
	{
		FigureRenderInfo	f;
		
		ComputeRenderInfo(&f);

		// Find the hips.
		vec3	HipOffset;
		float	TotalMass;
		vec3	PartCOM[PART_COUNT];
		ComputeCOMInfo(&HipOffset, &TotalMass, PartCOM, f);

		// Offset the figure state by the hip offset.
		ApplyOffset(&f, HipOffset);
		
		if (Config::GetBoolValue("FigureWireframe")) {

			//
			// Draw the figure as a stick-man.
			//
			
			glColor3f(0, 1, 0);
			Render::SetTexture(NULL);
			Render::CommitRenderState();
			
			// Board.
			vec3	Board[4];
			vec3	CrossBoardAxis = f.BoardAxis.cross(f.BoardUpAxis);
			Board[0] = f.BoardCenter + f.BoardAxis * 0.5 * BoardLength + CrossBoardAxis * 0.5 * BoardWidth;
			Board[1] = f.BoardCenter + f.BoardAxis * 0.5 * BoardLength - CrossBoardAxis * 0.5 * BoardWidth;
			Board[2] = f.BoardCenter - f.BoardAxis * 0.5 * BoardLength - CrossBoardAxis * 0.5 * BoardWidth;
			Board[3] = f.BoardCenter - f.BoardAxis * 0.5 * BoardLength + CrossBoardAxis * 0.5 * BoardWidth;
			
			glBegin(GL_LINE_STRIP);
			glVertex3fv(Board[0]);
			glVertex3fv(Board[1]);
			glVertex3fv(Board[2]);
			glVertex3fv(Board[3]);
			glVertex3fv(Board[0]);
			glEnd();
			
			// Legs and hips.
			glBegin(GL_LINE_STRIP);
			glVertex3fv(f.LeftToe);
			glVertex3fv(f.LeftHeel);
			glVertex3fv(f.LeftKnee);
			glVertex3fv(f.LeftHip);
			glVertex3fv(f.MiddleHip);
			glVertex3fv(f.RightHip);
			glVertex3fv(f.RightKnee);
			glVertex3fv(f.RightHeel);
			glVertex3fv(f.RightToe);
			glEnd();
			
			// Back.
			glBegin(GL_LINE_STRIP);
			glVertex3fv(f.MiddleHip);
			glVertex3fv(f.MiddleShoulder);
			glEnd();
			
			// Arms & shoulders.
			glBegin(GL_LINE_STRIP);
			glVertex3fv(f.LeftHand);
			glVertex3fv(f.LeftElbow);
			glVertex3fv(f.LeftShoulder);
			glVertex3fv(f.MiddleShoulder);
			glVertex3fv(f.RightShoulder);
			glVertex3fv(f.RightElbow);
			glVertex3fv(f.RightHand);
			glEnd();
			
			// head
			glBegin(GL_LINE_STRIP);
			glVertex3fv(f.MiddleShoulder);
			glVertex3fv(f.HeadTop);
			glEnd();

		} else {

			//
			// Render the figure using separate body-part models, i.e. Tomb Raider style.
			//
			
			matrix	m = s.ViewMatrix;	// Save the view matrix.
			matrix	temp;
			
			// Board and feet.
			temp.Orient(f.BoardAxis, f.BoardUpAxis, f.BoardCenter);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[BOARD]->Render(s, ClipHint);
			glPopMatrix();
			
			// Left leg.
			vec3	up = (f.LeftKnee - f.LeftHeel).normalize();
			vec3	dir = f.LeftKneeBendAxis /* .cross(up) */;
			temp.Orient(dir, up, f.LeftKnee);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[LEGLL]->Render(s, ClipHint);
			glPopMatrix();
			
			up = (f.LeftHip - f.LeftKnee).normalize();
			dir = f.LeftKneeBendAxis/* .cross(up) */;
			temp.Orient(dir, up, f.LeftHip);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[LEGUL]->Render(s, ClipHint);
			glPopMatrix();
			
			// Right leg.
			up = (f.RightKnee - f.RightHeel).normalize();
			dir = f.RightKneeBendAxis /* .cross(up) */;
			temp.Orient(dir, up, f.RightKnee);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[LEGLR]->Render(s, ClipHint);
			glPopMatrix();
			
			up = (f.RightHip - f.RightKnee).normalize();
			dir = f.RightKneeBendAxis /* .cross(up) */;
			temp.Orient(dir, up, f.RightHip);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[LEGUR]->Render(s, ClipHint);
			glPopMatrix();
			
			// Torso/head
			up = f.BackAxis;
			dir = f.BackAxis.cross(f.HipAxis.cross(f.BackAxis));
			dir.normalize();
			temp.Orient(dir, up, f.MiddleHip);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[TORSOL]->Render(s, ClipHint);
			glPopMatrix();
			
			up = f.BackAxis;
			dir = f.BackAxis.cross(f.ShoulderAxis.cross(f.BackAxis));
			dir.normalize();
			temp.Orient(dir, up, f.MiddleShoulder);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[TORSOU]->Render(s, ClipHint);
			glPopMatrix();

			// Left arm.

			// Upper.
			up = (f.LeftShoulder - f.LeftElbow).normalize();
			dir = f.LeftArmBendAxis;
			temp.Orient(dir, up, f.LeftShoulder);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[ARMUL]->Render(s, ClipHint);
			glPopMatrix();

			// Lower.
			up = (f.LeftElbow - f.LeftHand).normalize();
			dir = f.LeftArmBendAxis;
			temp.Orient(dir, up, f.LeftElbow);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[ARMLL]->Render(s, ClipHint);
			glPopMatrix();
			
			// Right arm.
			
			// Upper.
			up = (f.RightShoulder - f.RightElbow).normalize();
			dir = -f.RightArmBendAxis;
			temp.Orient(dir, up, f.RightShoulder);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[ARMUR]->Render(s, ClipHint);
			glPopMatrix();
			
			// Lower.
			up = (f.RightElbow - f.RightHand).normalize();
			dir = -f.RightArmBendAxis;
			temp.Orient(dir, up, f.RightElbow);
			matrix::Compose(&s.ViewMatrix, m, temp);
			glPushMatrix();
			Render::MultMatrix(temp);
			Part[ARMLR]->Render(s, ClipHint);
			glPopMatrix();
			
			s.ViewMatrix = m;	// Restore the view matrix.
		}
	}

	void	PaintShadow(bitmap32* dest, float DestWidth, const vec3& light_direction, const vec3& shadow_up, bool DrawFeet)
	// Paints a monochrome shadow image into the given bitmap.
	// DestWidth is the object-coord width of the destination
	// buffer, light_direction is normal to the shadow image, and
	// shadow_up is the object-coord up-direction of the destination
	// buffer.
	{
		int	i;
		
		FigureRenderInfo	f;
		
		ComputeRenderInfo(&f);

		// Find the hips.
		vec3	HipOffset;
		float	TotalMass;
		vec3	PartCOM[PART_COUNT];
		ComputeCOMInfo(&HipOffset, &TotalMass, PartCOM, f);

		// Offset the figure state by the hip offset.
		ApplyOffset(&f, HipOffset);
		
		// Clear dest buffer.
		uint32*	p = dest->GetData();
		int	count = dest->GetWidth() * dest->GetHeight();
		memset(p, 0, count * sizeof(uint32));

		uint8	col8[4] = { 0x20, 0x20, 0x60, 0xFF };
		uint32	color = *(uint32 *)&col8;

		// Compute basis vectors and offsets for mapping from object coords to shadow map coords.
		float	texels_per_meter = dest->GetWidth() / DestWidth;
		vec3	sr = light_direction.cross(shadow_up) * texels_per_meter;
		vec3	su = shadow_up * texels_per_meter;
		float	uoffset = float(dest->GetWidth() >> 1);
		float	voffset = float(dest->GetHeight() >> 1);
		
		// Draw each of the body segments.

		// Board: stretched octagon.
		vec3	vert[10];
		vec3	CrossBoardAxis = f.BoardAxis.cross(f.BoardUpAxis);
		vert[0] = f.BoardCenter + f.BoardAxis * 0.4f * BoardLength + CrossBoardAxis * 0.5f * BoardWidth;
		vert[1] = f.BoardCenter + f.BoardAxis * 0.5f * BoardLength + CrossBoardAxis * 0.35f * BoardWidth;
		vert[2] = f.BoardCenter + f.BoardAxis * 0.5f * BoardLength - CrossBoardAxis * 0.35f * BoardWidth;
		vert[3] = f.BoardCenter + f.BoardAxis * 0.4f * BoardLength - CrossBoardAxis * 0.5f * BoardWidth;
		vert[4] = f.BoardCenter - CrossBoardAxis * 0.45f * BoardWidth;
		vert[5] = f.BoardCenter - f.BoardAxis * 0.4f * BoardLength - CrossBoardAxis * 0.5f * BoardWidth;
		vert[6] = f.BoardCenter - f.BoardAxis * 0.5f * BoardLength - CrossBoardAxis * 0.35f * BoardWidth;
		vert[7] = f.BoardCenter - f.BoardAxis * 0.5f * BoardLength + CrossBoardAxis * 0.35f * BoardWidth;
		vert[8] = f.BoardCenter - f.BoardAxis * 0.4f * BoardLength + CrossBoardAxis * 0.5f * BoardWidth;
		vert[9] = f.BoardCenter + CrossBoardAxis * 0.45f * BoardWidth;

		// Transform.
		float	u[10], v[10];
		for (i = 0; i < 10; i++) {
			u[i] = uoffset + vert[i] * sr;
			v[i] = voffset + vert[i] * su;
		}

		// Paint the board poly.
		Geometry::FillTriangle(dest, u[0], v[0], u[2], v[2], u[1], v[1], color);
		Geometry::FillTriangle(dest, u[0], v[0], u[3], v[3], u[2], v[2], color);
		Geometry::FillTriangle(dest, u[0], v[0], u[3], v[3], u[4], v[4], color);
		Geometry::FillTriangle(dest, u[0], v[0], u[4], v[4], u[9], v[9], color);

		// Only draw feet and rear of board if we're above the snow surface.
		if (DrawFeet) {
			Geometry::FillTriangle(dest, u[4], v[4], u[5], v[5], u[9], v[9], color);
			Geometry::FillTriangle(dest, u[9], v[9], u[5], v[5], u[8], v[8], color);
			Geometry::FillTriangle(dest, u[5], v[5], u[6], v[6], u[7], v[7], color);
			Geometry::FillTriangle(dest, u[5], v[5], u[7], v[7], u[8], v[8], color);
			
			// feet: rect + 2 circles
			Capsule(dest, f.LeftToe, f.LeftHeel, 0.07f * texels_per_meter, sr, su, uoffset, voffset, color);
			Capsule(dest, f.RightToe, f.RightHeel, 0.07f * texels_per_meter, sr, su, uoffset, voffset, color);
		}

		// lower legs: rect + 1 circle (at knee)
		Capsule(dest, f.LeftHeel, f.LeftKnee, 0.08f * texels_per_meter, sr, su, uoffset, voffset, color);
		Capsule(dest, f.RightHeel, f.RightKnee, 0.08f * texels_per_meter, sr, su, uoffset, voffset, color);

		// upper legs: rect + 1 circle (at hip)
		Capsule(dest, f.LeftKnee, f.LeftHip, 0.10f * texels_per_meter, sr, su, uoffset, voffset, color);
		Capsule(dest, f.RightKnee, f.RightHip, 0.10f * texels_per_meter, sr, su, uoffset, voffset, color);

		// Torso.  Make it out of three capsules.
		vec3	MidLeftShoulder = f.MiddleShoulder + (f.LeftShoulder - f.MiddleShoulder) * 0.7f;
		vert[0] = f.LeftHip + (MidLeftShoulder - f.LeftHip) * 0.9f;
		vec3	MidRightShoulder = f.MiddleShoulder + (f.RightShoulder - f.MiddleShoulder) * 0.7f;
		vert[1] = f.RightHip + (MidRightShoulder - f.RightHip) * 0.9f;
		vec3	MidShoulder = f.MiddleHip + (f.MiddleShoulder - f.MiddleHip) * 0.9f;
		Capsule(dest, f.LeftHip, vert[0], 0.10f * texels_per_meter, sr, su, uoffset, voffset, color);
		Capsule(dest, f.RightHip, vert[1], 0.10f * texels_per_meter, sr, su, uoffset, voffset, color);
		Capsule(dest, f.MiddleHip, MidShoulder, 0.10f * texels_per_meter, sr, su, uoffset, voffset, color);

		// Left arm.
		Capsule(dest, f.LeftShoulder, f.LeftElbow, 0.065f * texels_per_meter, sr, su, uoffset, voffset, color);
		Capsule(dest, f.LeftElbow, f.LeftHand, 0.055f * texels_per_meter, sr, su, uoffset, voffset, color);

		// Right arm.
		Capsule(dest, f.RightShoulder, f.RightElbow, 0.065f * texels_per_meter, sr, su, uoffset, voffset, color);
		Capsule(dest, f.RightElbow, f.RightHand, 0.055f * texels_per_meter, sr, su, uoffset, voffset, color);

		// Neck.
		Capsule(dest, f.MiddleShoulder, f.HeadTop, 0.10f * texels_per_meter, sr, su, uoffset, voffset, color);
		
		// Head.
		vert[0] = f.MiddleShoulder + (f.HeadTop - f.MiddleShoulder) * 0.6f;
		Geometry::FillCircle(dest, vert[0] * sr + uoffset, vert[0] * su + voffset, 0.13f * texels_per_meter, color);

		// Blur dest buffer.
		
		// Clear edges of dest buffer.
	}

	static void	Capsule(bitmap32* dest, const vec3& vert0, const vec3& vert1, float radius, const vec3& sr, const vec3& su, float uoffset, float voffset, uint32 color)
	// Aux function for shadow painting.  Draw a 2D capsule into the dest buffer.
	{
		float	u[4], v[4];

		// End caps.
		u[0] = uoffset + vert0 * sr;
		v[0] = voffset + vert0 * su;
		u[1] = uoffset + vert1 * sr;
		v[1] = uoffset + vert1 * su;

		Geometry::FillCircle(dest, u[0], v[0], radius, color);
		Geometry::FillCircle(dest, u[1], v[1], radius, color);

		// Body.
		float	pu, pv;
		pu = -(v[1] - v[0]);
		pv = (u[1] - u[0]);
		float	l = sqrtf(pu * pu + pv * pv);
		if (l < 0.1) return;
		l = radius / l;
		pu *= l;
		pv *= l;

		u[3] = u[0] + pu;
		v[3] = v[0] + pv;
		u[0] -= pu;
		v[0] -= pv;
		
		u[2] = u[1] + pu;
		v[2] = v[1] + pv;
		u[1] -= pu;
		v[1] -= pv;

		Geometry::FillTriangle(dest, u[0], v[0], u[2], v[2], u[1], v[1], color);
		Geometry::FillTriangle(dest, u[0], v[0], u[3], v[3], u[2], v[2], color);
	}
	
};


// Initialization hooks.
static struct InitStickMan {
	InitStickMan() {
		GameLoop::AddInitFunction(Init);

		InitPartMoment();
	}

	static void	Init() {
		Model::AddGModelLoader("stk", STKLoader);
	}

	static GModel*	STKLoader(const char* filename) {
		StickMan*	s = new StickMan;
		return s;
	}

} InitStickMan;

