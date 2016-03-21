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
// particle.cpp	-thatcher 8/13/1999 Copyright Slingshot Game Technology

// Some code for handling particle effects.


#include <stdlib.h>
#include "particle.hpp"
#include "ogl.hpp"
#include "render.hpp"
#include "text.hpp"
#include "psdread.hpp"
#include "model.hpp"
#include "utility.hpp"
#include "config.hpp"


namespace Particle {
;


const int	PARTICLE_LIFE = 1000;	// Life of a particle, in ms.
const int	PARTICLE_FADE = 500;	// Time at the end of the particle life over which to fade to wholly transparent.


// Table of random vec3s with uniform distribution within the
// unit sphere.
const int	VEC_COUNT_BITS = 8;
const int	VEC_COUNT = (1 << VEC_COUNT_BITS);
const int	VEC_COUNT_MASK = VEC_COUNT - 1;
vec3	RandomVector[VEC_COUNT];


static float	Rand1()
// Returns a pseudo-random value between 0 and 1.
{
	return rand() / float(RAND_MAX - 1);
}


// Particle storage.
const int	PARTICLE_COUNT = 2048;
struct	ParticleInfo {
	int	TimeToLive;
	vec3	Location;
	vec3	Velocity;
	ParticleInfo*	Next;
};

ParticleInfo*	ParticleActiveList = NULL;
ParticleInfo*	ParticleFreeList = NULL;


::Render::Texture*	SnowParticle = NULL;
float	ParticleWidth = 0.15f, ParticleHeight = 0.15f;


static void	InsertParticle(const vec3& Location, const vec3& Velocity)
// Adds a particle with the specified attributes to the particle queue.
{
	if (ParticleFreeList == NULL) return;	// No free particles.
	
	ParticleInfo**	p = &ParticleActiveList;

	int	TimeToLive = PARTICLE_LIFE;

	// Find the last particle with a longer time to live than the new one.
	while (*p && (*p)->TimeToLive > TimeToLive) {
		p = &(*p)->Next;
	}

	// Insert the new particle.
	ParticleInfo*	part = ParticleFreeList;
	ParticleFreeList = part->Next;

	part->Location = Location;
	part->Velocity = Velocity;
	part->TimeToLive = TimeToLive;
	part->Next = *p;
	*p = part;
}


void	Open()
// Initialize the module.
{
	// Set up a table of random vec3s with uniform distribution within
	// the unit sphere.
	int	count = 0;
	while (count < VEC_COUNT) {
		vec3	v(Rand1() * 2 - 1, Rand1() * 2 - 1, Rand1() * 2 - 1);
		if (v.magnitude() <= 1) {
			RandomVector[count] = v;
			count++;
		}
	}

	// Allocate a pool of particles.
	int	i;
	for (i = 0; i < PARTICLE_COUNT; i++) {
		ParticleInfo*	p = new ParticleInfo;
		p->Next = ParticleFreeList;
		ParticleFreeList = p;
	}

	// Get a snow particle texture.
	bitmap32*	b = PSDRead::ReadImageData32("snow-particle.psd", &ParticleWidth, &ParticleHeight);
	if (b == NULL) {
		Error e; e << "Particle::Open() - can't load snow-particle.psd";
		throw e;
	}
	SnowParticle = ::Render::NewTextureFromBitmap(b, true, true, false);
	delete b;
}


void	Close()
// Free everything and shut down.
{
	Reset();

	// Delete the particle free list.
	ParticleInfo*	p = ParticleFreeList;
	while (p) {
		ParticleInfo*	n = p->Next;
		delete p;
		
		p = n;
	}
	ParticleActiveList = NULL;

	delete SnowParticle;
}


void	Reset()
// Reset all particles.
{
	// Move all the active particles to the free list.
	ParticleInfo*	p = ParticleActiveList;
	while (p) {
		ParticleInfo*	n = p->Next;
		p->Next = ParticleFreeList;
		ParticleFreeList = p;
		
		p = n;
	}
	ParticleActiveList = NULL;
}


float	TotalDeltaTicks = 0;


void	Update(const UpdateState& u)
// Move the particles.
{
	// Actually, just record the elapsed time in this function.  Do
	// the actual update in the render function.  This is because
	// the game loop may call the Update() function many times
	// between Render() calls, if the frame rate is low, to keep the
	// physics accurate.  But for particles we don't care much about
	// accuracy, and if the frame rate is low, we especially don't
	// want to incur the cost of doing a lot of extra particle
	// update computations.

	TotalDeltaTicks += GameLoop::GetSpeedScale() * u.DeltaTicks;
}


const float	PARTICLE_SIZE = 0.15f;


void	Render(const ViewState& s)
// Render the particles.
{
	int	i;
	
	// Don't draw the particles if "Particles" is false.
	if (Config::GetBool("Particles") == false) {
		TotalDeltaTicks = 0;
		return;
	}

	// Actually, move the particles and render them in one pass.

	// Set up for rendering.
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// View-coordinate offsets for the corners of a particle.
	vec3	offset[4];
	offset[0] = vec3(-ParticleWidth/2, ParticleHeight/2, 0);
	offset[1] = vec3(ParticleWidth/2, ParticleHeight/2, 0);
	offset[2] = vec3(ParticleWidth/2, -ParticleHeight/2, 0);
	offset[3] = vec3(-ParticleWidth/2, -ParticleHeight/2, 0);

	vec3	vbuf[4];
	float	uvbuf[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, (const float*) vbuf[0]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, uvbuf[0]);
	
	glColor3f(1, 1, 1);
	::Render::SetTexture(SnowParticle);	// xxxx
	::Render::DisableAlphaTest();
	::Render::EnableZBuffer();
	::Render::CommitRenderState();
	
	glDepthMask(GL_FALSE);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );	// alleged artifact-free unsorted transparent rendering.
//	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	
	// For updating.
	float	DeltaT = TotalDeltaTicks / 1000.0f;

	//xxxxxxx
	int	ProcessedCount = 0;
	int	DeletedCount = 0;
	//xxxxxxxxx

	int	AdjustAlpha = PARTICLE_FADE;	// Fade out the particle at the end of its life.
	
	ParticleInfo**	p = &ParticleActiveList;
	while (*p) {
		ParticleInfo*	part = *p;
		
		// Move the particle.
		part->TimeToLive -= frnd(TotalDeltaTicks);
		if (part->TimeToLive < 0) {
			// This particle, and all particles after it in the list, have expired.

			*p = NULL;	// Snip off the end of the list.
			while (part) {
				ParticleInfo*	n = part->Next;
				part->Next = ParticleFreeList;
				ParticleFreeList = part;
				part = n;

				DeletedCount++;//xxxxxxx
			}
			break;
		} else if (part->TimeToLive > PARTICLE_LIFE) {
			// Particle's life is unnaturally long (caused
			// e.g. by playing a recording in reverse).

			// Expire it.
			*p = part->Next;	// Splice this particle out of the active list.

			// Splice it into the free list.
			part->Next = ParticleFreeList;
			ParticleFreeList = part;
			
			continue;
		}

		// Fade particles out towards the end of their lives.
		if (part->TimeToLive < AdjustAlpha) {
			float	f = part->TimeToLive / float(PARTICLE_FADE);
			glColor4f(1, 1, 1, f);
			AdjustAlpha = part->TimeToLive - 25;
		}
		
		part->Velocity.SetY(part->Velocity.Y() - 9.8f * DeltaT);	// Gravity.
		part->Velocity -= part->Velocity * (0.8f * DeltaT);	// Drag.  Should be v^2.
//		// drag, wind, whatever?
		part->Location += part->Velocity * DeltaT;

		// Render.
		vec3	v, w;
		s.ViewMatrix.Apply(&v, part->Location);

		for (i = 0; i < 4; i++) {
			vbuf[i] = v;
			vbuf[i] += offset[i];
		}
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		ProcessedCount++;	//xxxxxx
		
		p = &(*p)->Next;
	}

	Model::AddToTriangleCount(ProcessedCount * 2);

	glPopMatrix();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glColor4f(1, 1, 1, 1);
	
//	//xxxxxxx
//	char	buf[200];
//	sprintf(buf, "dt = %d", TotalDeltaTicks);
//	Text::DrawString(s, 500, 400, Text::FIXEDSYS, Text::ALIGN_RIGHT, buf, 0xFF000000);
//
//	sprintf(buf, "proc'd = %d", ProcessedCount);
//	Text::DrawString(s, 500, 412, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
//	
//	sprintf(buf, "dele'd = %d", DeletedCount);
//	Text::DrawString(s, 500, 424, Text::FIXEDSYS, Text::ALIGN_LEFT, buf, 0xFF000000);
//	//xxxxxxxxx
	
	TotalDeltaTicks = 0;
}


void	PointSource(TypeID Type, const SourceInfo& s0, const SourceInfo& s1, int Count, int DeltaTicks)
// Generates ~Count particles of the specified type, starting with
// parameters s0 and morphing to parameters s1 over the specified number
// of ticks.  Models a point source moving through space.
{
	SourceInfo	s;

	int	i;
	for (i = 0; i < Count; i++) {
		float	f = Rand1();

		s.Lerp(s0, s1, f);
		
		s.Location += RandomVector[rand() & VEC_COUNT_MASK] * s.LocationVariance;
		s.Velocity += RandomVector[rand() & VEC_COUNT_MASK] * s.VelocityVariance;

		float	dt = DeltaTicks * f;
		s.Location += s.Velocity * (dt / 1000);
		
		InsertParticle(s.Location, s.Velocity);
	}
}


void	LineSource(TypeID Type, const SourceInfo& a0, const SourceInfo& a1, const SourceInfo& b0, const SourceInfo& b1, int Count, int DeltaTicks)
// Generates ~Count particles of the specified type.  Models a line
// source moving through space, starting at (a0-b0) at time offset 0,
// and moving to (a1-b1) at time offset DeltaTicks.
{
	SourceInfo	s0, s1, s;

	int	i;
	for (i = 0; i < Count; i++) {
		float	fspace = Rand1();
		float	ftime = Rand1();

		s0.Lerp(a0, b0, fspace);
		s1.Lerp(a1, b1, fspace);
		s.Lerp(s0, s1, ftime);
		
		s.Location += RandomVector[rand() & VEC_COUNT_MASK] * s.LocationVariance;
		s.Velocity += RandomVector[rand() & VEC_COUNT_MASK] * s.VelocityVariance;

		float	dt = DeltaTicks * ftime;
		s.Location += s.Velocity * (dt / 1000);

//		// Slightly cheesy optimization: determine how long to
//		// show particle by its vertical velocity.  The
//		// reasoning being, when a particle drops below its
//		// initial y value then it's more likely to be
//		// underground and invisible.  Whereas checking the
//		// ground y for each particle on every update is
//		// probably too expensive.
//		int	life = 2 * s.Velocity.Y() / 9.8 * 1000.0;
//		life += 250;
//		int	life = PARTICLE_LIFE;
		
		InsertParticle(s.Location, s.Velocity);
	}
}




}; // end namespace Particle

