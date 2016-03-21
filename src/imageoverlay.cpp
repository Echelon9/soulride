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
// ImageOverlay.cpp	-thatcher 12/30/1999 Copyright Slingshot Game Technology

// Code for drawing image overlays.  Interfaces with the Overlay module
// to defer drawing overlays until the end of the frame.


#include "render.hpp"
#include "overlay.hpp"


namespace ImageOverlay {
;


static void	ImageChunkRenderer(Overlay::Chunk* c);


void	Open()
// Register with Overlay:: .
{
	Overlay::RegisterChunkRenderer(Overlay::IMAGE, ImageChunkRenderer);
}


void	Close()
{
}


struct ImageChunk : public Overlay::Chunk {
	int	x, y, width, height, u, v;
	float	scale;
	Render::Texture*	im;
	uint32	ARGBColor;
};


void	Draw(int x, int y, int width, int height, Render::Texture* im, int u, int v, uint32 ARGBColor, float scale)
// Store the given image command in the overlay buffer, to be drawn at the end of the frame.
{
	ImageChunk*	c = static_cast<ImageChunk*>(Overlay::NewChunk(sizeof(ImageChunk), Overlay::IMAGE));
	if (c == NULL) return;

	// Fill the structure contents.
	c->x = x;
	c->y = y;
	c->width = width;
	c->height = height;
	c->im = im;
	c->u = u;
	c->v = v;
	c->scale = scale;
	c->ARGBColor = ARGBColor;
}


void	ImageChunkRenderer(Overlay::Chunk* chunk)
// Cast c to an ImageChunk, and draw it.
{
	ImageChunk*	c = static_cast<ImageChunk*>(chunk);

	Render::BlitImage(c->x, c->y, c->width, c->height, c->im, c->u, c->v, c->ARGBColor, c->scale);
}

	
};	// namespace ImageOverlay

