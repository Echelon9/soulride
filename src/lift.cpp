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
// lift.cpp	-thatcher 12/20/1999 Copyright Slingshot Game Technology

// Implementation of lift classes.


#include "utility.hpp"
#include "main.hpp"
#include "model.hpp"
#include "clip.hpp"
#include "gameloop.hpp"
#include "game.hpp"
#include "ui.hpp"
#include "player.hpp"
#include "usercamera.hpp"
#include "text.hpp"


static int	ClosestLiftID = 0;	// Cheesy way to pass a parameter from LiftBase to RideLift UI.


class LiftBase;
class LiftTop;
class LiftSupport;

struct LiftSupportElem {
	LiftSupport*	Support;
	float	LiftDistance;
	LiftSupportElem*	Next;
};


struct LiftInfo {
	LiftBase*	Base;
	LiftTop*	Top;
	LiftSupportElem*	Supports;	// Linked list (ordered from base to top) of lift supports.
	float	TotalLiftDistance;
};


const int	LIFT_MAXCOUNT = 20;
LiftInfo	Lift[LIFT_MAXCOUNT];


const int	LIFT_NAME_MAXLEN = 256;


class LiftTop : virtual public MOriented
{
private:
	int	LiftID;
	
public:
	LiftTop()
	// Constructor.
	{
		LiftID = 0;
	}

	~LiftTop()
	// Remove from the lift list.
	{
		Lift[LiftID].Top = NULL;
	}
	

	void	LoadExtras(FILE* fp)
	// Load the lift ID and orientation (i.e. the stuff beyond the basic object data).
	{
		// Load the Lift ID.
		LiftID = Read32(fp) - 1;

		// Load and init the orientation.
		LoadOrientation(fp);

		// Add to the lift list.
		if (LiftID < 0 || LiftID >= LIFT_MAXCOUNT) {
			Error e; e << "Error loading lift top: LiftID must be between 1 and " << LIFT_MAXCOUNT;
			throw e;
		}
		if (Lift[LiftID].Top != NULL) {
			Error e; e << "Error loading lift top: Redefinition of lift # " << LiftID + 1;
			throw e;
		}
		Lift[LiftID].Top = this;
	}

	
	void	Update(const UpdateState& u)
	// No need to do anything.
	{
	}
};


class LiftSupport : virtual public MOriented
{
private:
	int	LiftID;
	
public:
	LiftSupport()
	// Constructor.
	{
		LiftID = 0;
	}

	~LiftSupport()
	// Remove from the lift list.
	{
	}
	

	void	LoadExtras(FILE* fp)
	// Load the lift ID and orientation (i.e. the stuff beyond the basic object data).
	{
		// Load the Lift ID.
		LiftID = Read32(fp) - 1;

		// Load and init the orientation.
		LoadOrientation(fp);

		// Add to the lift list.
		if (LiftID < 0 || LiftID >= LIFT_MAXCOUNT) {
			Error e; e << "Error loading lift support: LiftID must be between 1 and " << LIFT_MAXCOUNT;
			throw e;
		}
		// Link into the list of lift supports.
		LiftSupportElem*	e = new LiftSupportElem;
		e->Support = this;
		e->Next = Lift[LiftID].Supports;
		Lift[LiftID].Supports = e;
	}

	
	void	Update(const UpdateState& u)
	// No need to do anything.
	{
	}
};


class LiftBase : virtual public MOriented
{
private:
	int	LiftID;
	char	LiftName[LIFT_NAME_MAXLEN];
	
public:
	LiftBase()
	// Constructor.
	{
		LiftID = 0;
		LiftName[0] = 0;
	}

	~LiftBase()
	// Remove from the lift list.
	{
		Lift[LiftID].Base = NULL;
	}

	const char*	GetName() { return LiftName; }
	

	void	LoadExtras(FILE* fp)
	// Load the lift ID, lift name, etc (the stuff beyond the basic object data).
	{
		// Load the Lift ID.
		LiftID = Read32(fp) - 1;

		// Load the lift name.
		fgets(LiftName, LIFT_NAME_MAXLEN, fp);
		// Delete trailing '\n'.
		LiftName[strlen(LiftName) - 1] = 0;

		// Load and init the orientation.
		LoadOrientation(fp);

		// Add to the lift list.
		if (LiftID < 0 || LiftID >= LIFT_MAXCOUNT) {
			Error e; e << "Error loading lift base: LiftID must be between 1 and " << LIFT_MAXCOUNT;
			throw e;
		}
		if (Lift[LiftID].Base != NULL) {
			Error e; e << "Error loading lift base: Redefinition of lift # " << LiftID + 1;
			throw e;
		}
		Lift[LiftID].Base = this;
	}

	
	void	Update(const UpdateState& u)
	// Look for the user, and see if s/he is really close.  If so, then lift them
	// to the LiftTop.
	{
//		if (Active == false) return;
		
		MDynamic*	User = Game::GetUser();
		if (User) {
			if (UI::GetMode() == UI::PLAYING && (User->GetLocation() - GetLocation()).magnitude() < 15) {
//				UI::SetMode(UI::SHOWFINISH, u.Ticks);

//				// Credit the player with finishing this run.
//				Player*	p = Game::GetCurrentPlayer();
//				p->CompletedRun(Game::GetCurrentRun());

				LiftTop*	top = Lift[LiftID].Top;
				if (top) {
					// Teleport user to the top of the lift.
					User->SetMatrix(top->GetMatrix());
					User->SetLocation(top->GetLocation());
					User->SetVelocity(GetDirection() * 3);
					User->SetAngMomentum(ZeroVector);
				}

				// Set the UI mode to RIDELIFT.
				ClosestLiftID = LiftID;	// So that the UI mode knows which lift to ride.
				UI::SetMode(UI::RIDELIFT, u.Ticks);
			}
		}
	}
};


static void	SplitList(LiftSupportElem** a, LiftSupportElem** b, LiftSupportElem* l)
// Splits l into two roughly equal sublists, and points *a and *b at the sublists.
{
	*a = *b = NULL;
	LiftSupportElem*	e;
	
	for (;;) {
		// One for a...
		if (l == NULL) break;
		e = l;
		l = l->Next;
		e->Next = *a;
		*a = e;

		// And one for b...
		if (l == NULL) break;
		e = l;
		l = l->Next;
		e->Next = *b;
		*b = e;
	}
}


static LiftSupportElem*	MergeLists(LiftSupportElem* a, LiftSupportElem* b)
// Takes two sorted sub-lists and joins them into one big sorted list.
// Returns the unified list.
{
	LiftSupportElem*	l = NULL;
	LiftSupportElem**	end = &l;

	for (;;) {
		if (a && b) {
			// Pick the head of either a or b, according to whichever has a smaller OrderKey.
			if (a->LiftDistance <= b->LiftDistance) {
				*end = a;
				a = a->Next;
				(*end)->Next = NULL;
				end = &((*end)->Next);
			} else {
				*end = b;
				b = b->Next;
				(*end)->Next = NULL;
				end = &((*end)->Next);
			}
		} else if (a) {
			*end = a;
			break;
		} else if (b) {
			*end = b;
			break;
		} else {
			break;
		}
	}

	return l;
}


static LiftSupportElem*	MergeSort(LiftSupportElem* l)
// Given a linked list of lift support elements, sorts the list according
// to OrderKey and returns the re-arranged list.
{
	if (l == NULL || l->Next == NULL) return l;

	LiftSupportElem	*a, *b;
	SplitList(&a, &b, l);
	a = MergeSort(a);
	b = MergeSort(b);

	return MergeLists(a, b);
}


class LiftRider : public MOriented {
public:
	unsigned int	LastTicks;
	bool	Active;
	float	Progress;
	int	LiftID;
	float	Theta;

	
	LiftRider()
	// Constructor.  Initialize the members.
	{
		LastTicks = 0;
		Active = false;
		Progress = 0;
		LiftID = 0;
		Theta = 0;
	}

	void	RideLift(int id)
	// Ride the specified lift.
	{
		LiftID = id;
		Progress = 0;
		Active = true;
		Theta = 0;
	}

	void	SetActive(bool a)
	// Enables/disables.
	{
		Active = a;
	}
	
	void	Update(const UpdateState& u)
	// Runs behavior for the LiftRider.
	{
		if (Active == false) return;

		//
		// Move up the lift.
		//

		float	SpeedModifier = fclamp(0, 1 + u.Inputs.Pitch * 5, 6);
		
		Progress += 10 * u.DeltaT * SpeedModifier;	// 20-ish mph?
		
		if (Progress >= Lift[LiftID].TotalLiftDistance) {
			// We're done lifting.  Let the user go.
			UI::SetMode(UI::PLAYING, u.Ticks);
			return;
		}

		// Figure out our location based on our progress so far.
		MOriented*	prev = Lift[LiftID].Base;
		float	PrevDistance = 0;
		LiftSupportElem*	e = Lift[LiftID].Supports;
		MOriented*	next = e->Support;
		float	NextDistance = 0;
		for (;;) {
			next = e->Support;
			NextDistance = e->LiftDistance;

			if (e->LiftDistance > Progress) {
				break;
			}

			e = e->Next;

			prev = next;
			PrevDistance = NextDistance;

			if (e == NULL) {
				next = Lift[LiftID].Top;
				NextDistance = Lift[LiftID].TotalLiftDistance;
				break;
			}
		}

		// Our location is on the line between the previous and next lift objects.
		float	n = Progress - PrevDistance;
		float	d = NextDistance - PrevDistance;
		float	f = 0;
		if (d > 0) f = n / d;
		vec3	pl = prev->GetMatrix() * vec3(0, 7, 7);
		vec3	nl = next->GetMatrix() * vec3(0, 7, 7);
		SetLocation(pl + (nl - pl) * f);

		vec3	dir = nl - pl;
		dir.normalize();

		//
		// Get control inputs and swivel the view.
		//
		float	pitch = u.Inputs.Pitch;
		float	yaw = u.Inputs.Tilt;
		float	roll = u.Inputs.Yaw;
		float	throttle = u.Inputs.Throttle;

		int mode = (u.Inputs.Button[0].State ? 1 : 0) |
			   (u.Inputs.Button[1].State ? 2 : 0);


		Theta += yaw * -1.0f * u.DeltaT;
		if (Theta < -PI) Theta += 2*PI;
		if (Theta > PI) Theta -= 2*PI;

		vec3	 up = dir.cross(YAxis).cross(dir);
		up.normalize();
		
		dir = Geometry::Rotate(Theta, up, dir);
		up = dir.cross(YAxis).cross(dir);
		up.normalize();
		
		SetDirection(dir);
		SetUp(up);
		
	}
};


//
// UI mode for riding the lift.
//


class RideLiftUI : public UI::ModeHandler {
	LiftRider*	Rider;
	int	LiftID;
	
public:
	RideLiftUI() {
	}

	~RideLiftUI() {
	}
	
	void	Open(int Ticks)
	// Called by UI when entering mode.
	{
		// Deactivate the snowboarder behavior.
		Game::SetUserActive(false);

		// Use the LiftRider as the viewpoint.
		UserCamera::SetSubject(Rider);
		UserCamera::SetMotionMode(UserCamera::FOLLOW);
		UserCamera::SetAimMode(UserCamera::LOOK_THROUGH);

		// Figure out which lift to ride.
		LiftID = ClosestLiftID;
		Rider->RideLift(LiftID);
		Rider->SetActive(true);
		
	}

	void	Close()
	// Called by UI when exiting mode.
	{
		Rider->SetActive(false);
	}

	void	MountainInit()
	// Create a LiftRider instance, for controlling the camera later.
	{
		// Set up LiftRider object.
		Rider = new LiftRider;

		// Attach to database xxxxx
		Rider->SetLocation(vec3(0, 1500, 0));
		Rider->SetDirection(ZAxis);
		Rider->SetUp(YAxis);
//		Rider->SetVisual(Dummy);//xxxxxxx

		Model::AddDynamicObject(Rider);
	}

	void	MountainClear()
	{
		Rider = NULL;
	}

	void	Render(const ViewState& s)
	// Called every render cycle, to allow additional 2D rendering behind the UI.
	{
		// Show the lift name.
		Text::DrawString(620, 50, Text::DEFAULT, Text::ALIGN_RIGHT, Lift[LiftID].Base->GetName());
	}
	
	
	void	Update(const UpdateState& u)
	// Let the user adjust their view, change the lift speed, etc.
	{
	}

	void	Action(int ElementIndex, UI::ActionCode code, int Ticks)
	// Called by UI when the user takes an action.
	{
		if (code == UI::ESCAPE) {
			UI::SetMode(UI::PLAYING, Ticks);
		}
	}
} RideLiftInstance;



static struct InitLift {
	InitLift() {
		GameLoop::AddInitFunction(Init);

		Game::AddInitFunction(InitLoad);
		Game::AddPostLoadFunction(PostLoad);
		Game::AddClearFunction(Clear);
	}

	static void	Init()
	{
		Model::AddObjectLoader(7, LiftBaseLoader);
		Model::AddObjectLoader(8, LiftSupportLoader);
		Model::AddObjectLoader(9, LiftTopLoader);

		UI::RegisterModeHandler(UI::RIDELIFT, &RideLiftInstance);
	}


	static void	InitLoad()
	// Prepare to load a new dataset.
	{
		// Init the lift list.
		int	i;
		for (i = 0; i < LIFT_MAXCOUNT; i++) {
			Lift[i].Base = NULL;
			Lift[i].Top = NULL;
			Lift[i].Supports = NULL;
		}
	}

	
	static void	PostLoad()
	// Sort the lift supports, in order from the base to the top.
	// Use the locations of the objects to figure out the order.
	{
		int	i;
		for (i = 0; i < LIFT_MAXCOUNT; i++) {
			if (Lift[i].Base == NULL || Lift[i].Top == NULL || Lift[i].Supports == NULL) continue;

			vec3	dir = Lift[i].Top->GetLocation() - Lift[i].Base->GetLocation();
			dir.normalize();
			
			// Compute the ordering parameter for each lift support.
			LiftSupportElem*	e = Lift[i].Supports;
			while (e) {
				e->LiftDistance = (e->Support->GetLocation() - Lift[i].Base->GetLocation()) * dir;
				e = e->Next;
			}

			// Sort using the sort key, which should put the
			// lifts in geometric order from the base up to
			// the top.
			Lift[i].Supports = MergeSort(Lift[i].Supports);

			//
			// Now compute the distance between each lift element.
			//
			e = Lift[i].Supports;
			LiftSupportElem*	prev = e;
			if (e) {
				// Distance from base to first support.
				e->LiftDistance = (e->Support->GetLocation() - Lift[i].Base->GetLocation()).magnitude();
				e = e->Next;
			}
			while (e) {
				// Distance between supports.
				e->LiftDistance = (e->Support->GetLocation() - prev->Support->GetLocation()).magnitude();
				prev = e;
				e = e->Next;
			}
			float	LastToTop = 0;
			if (prev) LastToTop = (Lift[i].Top->GetLocation() - prev->Support->GetLocation()).magnitude();

			//
			// Compute the total lift distance, and the distance of each support along the way.
			//
			float	total = 0;
			e = Lift[i].Supports;
			while (e) {
				total += e->LiftDistance;
				e->LiftDistance = total;
				e = e->Next;
			}
			total += LastToTop;
			Lift[i].TotalLiftDistance = total;
		}
	}
	

	static void	Clear()
	// Free the stuff we allocated; basically the nodes in the lift support linked-list.
	{
		int	i;
		for (i = 0; i < LIFT_MAXCOUNT; i++) {
			LiftSupportElem*	e = Lift[i].Supports;
			while (e) {
				LiftSupportElem*	n = e->Next;
				delete e;
				e = n;
			}
		}
	}
	
	
	static void	LiftBaseLoader(FILE* fp)
	// Loads information from the given file and uses it to initialize a LiftBase object.
	{
		LiftBase*	o = new LiftBase();

		// Load the standard object information.
		o->LoadLocation(fp);

		o->LoadExtras(fp);
		
		// Link to the database.
		Model::AddDynamicObject(o);	// Dynamic, so that Update() gets called.

//		// Register with ::Game.
//		Game::RegisterRunEndTrigger(o, RunID);
	}

	
	static void	LiftSupportLoader(FILE* fp)
	// Loads information from the given file and uses it to
	// initialize a LiftSupport object.  Links the new object into
	// the appropriate LiftSupport list in the appropriate Lift[]
	// structure.
	{
		LiftSupport*	o = new LiftSupport();

		// Load the standard object information.
		o->LoadLocation(fp);

		o->LoadExtras(fp);
		
		// Link to the database.
		Model::AddStaticObject(o);
	}

	
	static void	LiftTopLoader(FILE* fp)
	// Loads information from the given file and uses it to initialize a LiftTop object.
	{
		LiftTop*	o = new LiftTop();

		// Load the standard object information.
		o->LoadLocation(fp);

		o->LoadExtras(fp);
		
		// Link to the database.
		Model::AddStaticObject(o);
	}

} InitLift;

