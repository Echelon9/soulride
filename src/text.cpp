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
// text.cpp	-thatcher 11/15/1998 Copyright Slingshot

// Some routines for processing and displaying text.


#include <stdio.h>
#include <string.h>
#include "text.hpp"
#include "utility.hpp"
#include "render.hpp"
#include "clip.hpp"
#include "psdread.hpp"
#include "ogl.hpp"
#include "overlay.hpp"
#include "gameloop.hpp"


namespace Text {
;


struct Character {
	Render::Texture*	image;
	int	Width, Height;
	int	TopY, BaselineY, LeftX;

	Character() {
		image = NULL;
		Width = Height = 0;
		TopY = BaselineY = LeftX = 0;
	}
};


struct Font {
	// name, size...
	float	PixelHeight;	// Natural height of the character blank.
	GG_Font*	ggf;
	
	Font() {
		PixelHeight = 0;
		ggf = NULL;
	}

	void	Load(const char* fontname)
	{
		GameLoop::AutoPauseInput	autoPause;
		if (GUI->loadFont((char*) fontname, &ggf) != GG_OK) {
			Error e; e << "Can't load font '" << fontname << "'";
			throw e;
		}
	}
};


Font	Fonts[FONTCOUNT];


struct TextChunk : public Overlay::Chunk {
	int	x, y;
	int	MaxWidth;	// Set to 0 for ordinary single-line output.
	FontID	f;
	Alignment	a;
	uint32	ARGBColor;
};
static void	RenderTextChunk(Overlay::Chunk* c);


void	Open()
// Initialize fonts, etc.
{
//	Fonts[DEFAULT].Load("swiss-xcbi.ggf");
//	Fonts[DEFAULT].PixelHeight = 23;
	Fonts[DEFAULT].Load("menus.ggf");
	Fonts[DEFAULT].PixelHeight = 18;

	Fonts[FIXEDSYS].Load("fixedsys.ggf");
	Fonts[FIXEDSYS].PixelHeight = 14;

	Fonts[SCORE].Load("score.ggf");
	Fonts[SCORE].PixelHeight = 44;

	// Register the text overlay chunk rendering function.
	Overlay::RegisterChunkRenderer(Overlay::TEXT, RenderTextChunk);
}


void	Close()
// Release stuff.
{
}


static void	RenderString(int x, int y, FontID f, Alignment a, const char* buf, uint32 ARGBColor);
static void	RenderMultiLineString(int x, int y, FontID f, Alignment a, int MaxWidth, const char* buf, uint32 ARGBColor);


void	DrawString(int x, int y, FontID f, Alignment a, const char* buf, uint32 ARGBColor)
// Queues the specified string for drawing.
{
	// Compute the size of the chunk (structure size plus space for the string),
	// and allocate the memory.
	int	len = strlen(buf);
	int	size = sizeof(TextChunk) + len + 1;
	TextChunk*	c = static_cast<TextChunk*>(Overlay::NewChunk(size, Overlay::TEXT));
	if (c == NULL) return;

	// Set the structure parameters.
	c->x = x;
	c->y = y;
	c->MaxWidth = 0;
	c->f = f;
	c->a = a;
	c->ARGBColor = ARGBColor;

	// Copy the string onto the end of the chunk.
	char*	p = reinterpret_cast<char*>(c) + sizeof(TextChunk);
	strcpy(p, buf);
}


void	DrawMultiLineString(int x, int y, FontID f, Alignment a, int MaxWidth, const char* buf, uint32 ARGBColor)
// Queues the specified mulit-line string for drawing.
{
	// Compute the size of the chunk (structure size plus space for the string),
	// and allocate the memory.
	int	len = strlen(buf);
	int	size = sizeof(TextChunk) + len + 1;
	TextChunk*	c = static_cast<TextChunk*>(Overlay::NewChunk(size, Overlay::TEXT));
	if (c == NULL) return;

	// Set the structure parameters.
	c->x = x;
	c->y = y;
	c->MaxWidth = MaxWidth;
	c->f = f;
	c->a = a;
	c->ARGBColor = ARGBColor;

	// Copy the string onto the end of the chunk.
	char*	p = reinterpret_cast<char*>(c) + sizeof(TextChunk);
	strcpy(p, buf);
}


void	RenderTextChunk(Overlay::Chunk* chunk)
// Interprets the given chunk as a TextChunk, and renders the contained string.
{
	TextChunk*	c = static_cast<TextChunk*>(chunk);

	// String data is stored in the chunk after the structure data.
	char*	buf = reinterpret_cast<char*>(c) + sizeof(TextChunk);
	
	if (c->MaxWidth) {
		// It's a multi-line string.
		RenderMultiLineString(c->x, c->y, c->f, c->a, c->MaxWidth, buf, c->ARGBColor);
	} else {
		// It's an ordinary single-line string.
		RenderString(c->x, c->y, c->f, c->a, buf, c->ARGBColor);
	}
}


void	FormatNumber(char* buf, float number, int DigitsToLeftOfDecimal, int DigitsToRightOfDecimal)
// Converts the given number to a decimal string in the given buffer.
// Formats the number using leading spaces and trailing zeros, to conform
// to the number of specified places to either side of the decimal point.
// This function *will* create more digits to the left of the decimal point
// than specified, if it needs to to represent the number, so make sure
// buf[] has enough room to accommodate the number.
{
	static const float	DigitsFactor[10] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
	
	int	dr = iclamp(0, DigitsToRightOfDecimal, 9);
	int	dl = iclamp(0, DigitsToLeftOfDecimal - 1, 9);

	// Make the number positive, for processing.  Keep a flag for adding the '-' later.
	bool	Minus = number < 0;
	if (Minus) number = -number;

	// Split into integer and fraction portions.
	number += 0.5f / DigitsFactor[dr];
	int	IntPart = int(number);
	int	FracPart = (int) ((number - IntPart) * DigitsFactor[dr]);

	// Figure out how many spaces we need for padding on the left.
	int	pad = -int(Minus);
	for (int place = dl; place > 0 && DigitsFactor[place] > IntPart; place--) {
		pad++;
	}

	// Add the padding, and the minus sign if necessary.
	char*	p = buf;
	for ( ; pad > 0; pad--) { *p++ = ' '; }
	if (Minus) *p++ = '-';

	// Add the digits of the integer part.
	sprintf(p, "%d", IntPart);
	while (*p) p++;

	if (dr) {
		// Add decimal point and fractional part of the number.
		*p++ = '.';
		sprintf(p, "%.*d", dr, FracPart);
	}
}


void	FormatTime(char* buf, int TimerTicks)
// Takes the ticks value in milliseconds, and converts it to a MM:SS.HH
// time format into the given buffer.
{
	int	Hundredths = TimerTicks / 10;
	int	Minutes = Hundredths / 6000;
	Hundredths -= Minutes * 6000;
	int	Seconds = Hundredths / 100;
	Hundredths -= Seconds * 100;

	buf[0] = 0;
	char	temp[80];
	if (Minutes) {
		sprintf(temp, "%d", Minutes);
		strcat(buf, temp);
		strcat(buf, ":");
		sprintf(temp, "%02d", Seconds);
	} else {
		sprintf(temp, "%d", Seconds);
	}
	strcat(buf, temp);
	strcat(buf, ".");
	sprintf(temp, "%02d", Hundredths);
	strcat(buf, temp);
}


int	GetWidth(FontID f, const char* buf)
// Compute the pixel width of the given string as rendered in the current font.
{
	GUI->setFont(Fonts[f].ggf);
	GUI->setFontSize(Fonts[f].PixelHeight / 240.0f);
	GUI->setFontSpacing(0.0);
	GUI->setFontStretch(1.0);
	GUI->setFontLeading(Fonts[f].PixelHeight / 240.0f * 1.25f);
	
	return (int) (GUI->getTextWidth((char*) buf) * 240.0f);
}


void	RenderString(int x, int y, FontID f, Alignment a, const char* buf, uint32 ARGBColor)
// Draws the given string to the screen, at the given location.
{
	GUI->setFont(Fonts[f].ggf);
	GUI->setFontSize(Fonts[f].PixelHeight / 240.0f);
	GUI->setFontSpacing(0.0f);
	GUI->setFontStretch(1.0f);
	GUI->setFontLeading(Fonts[f].PixelHeight / 240.0f * 1.25f);

	GG_FontFX     fontFx;
	fontFx.flags = GG_FONTFX_COLORMOD;
	fontFx.clrMod.set(((ARGBColor >> 24) & 0xFF) / 255.0f,
			  ((ARGBColor >> 16) & 0xFF) / 255.0f,
			  ((ARGBColor >> 8) & 0xFF) / 255.0f,
			  ((ARGBColor) & 0xFF) / 255.0f);

	static const uint	atab[4] = {
		GG_TEXTALIGN_LEFT,
		GG_TEXTALIGN_CENTER,
		GG_TEXTALIGN_RIGHT,
		GG_TEXTALIGN_RIGHT	// xxxx ALIGN_DECIMAL
	};
	uint	align = atab[a];


	GameLoop::GUIBegin();
	GUI->drawText((char*) buf, (x - 320) / 240.0f, -((y - 240) / 240.0f), align, &fontFx);
	GameLoop::GUIEnd();
	// Make sure blending is still on.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
}


void	RenderMultiLineString(int x, int y, FontID f, Alignment a, int MaxWidth, const char* buf, uint32 ARGBColor)
// Draws the given string to the screen, at the given location.  Breaks the text onto
// multiple lines, with each line being no longer than MaxWidth pixels.
{
	GUI->setFont(Fonts[f].ggf);
	GUI->setFontSize(Fonts[f].PixelHeight / 240.0f);
	GUI->setFontSpacing(0.0f);
	GUI->setFontStretch(1.0f);
	GUI->setFontLeading(Fonts[f].PixelHeight / 240.0f * 1.25f);

	GG_FontFX     fontFx;
	fontFx.flags = GG_FONTFX_COLORMOD;
	fontFx.clrMod.set(((ARGBColor >> 24) & 0xFF) / 255.0f,
			  ((ARGBColor >> 16) & 0xFF) / 255.0f,
			  ((ARGBColor >> 8) & 0xFF) / 255.0f,
			  ((ARGBColor) & 0xFF) / 255.0f);

	static const uint	atab[4] = {
		GG_TEXTALIGN_LEFT,
		GG_TEXTALIGN_CENTER,
		GG_TEXTALIGN_RIGHT,
		GG_TEXTALIGN_RIGHT	// xxxx ALIGN_DECIMAL
	};
	uint	align = atab[a];

	GG_Rect2D	r;
	r.set((x - 320) / 240.0f, -((y - 240) / 240.0f), (x + MaxWidth - 320) / 240.0f, -((y - 240 + 1000) / 240.0f));

	GameLoop::GUIBegin();
	GUI->drawTextBox((char*) buf, &r, align, &fontFx);
	GameLoop::GUIEnd();
	// Make sure blending is still on.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
}


int	GetFontHeight(FontID f)
// Returns the height, in pixels, of the specified font.
{
	return (int) (Fonts[f].PixelHeight);
}


int	GetFontBaseline(FontID f)
// Returns the offset from the top of the specified font's height to the
// baseline, in pixels.  The baseline is the coordinate that's aligned
// with the y value passed in a DrawString() call.
{
	return (int) (Fonts[f].PixelHeight);
}


};	// end namespace Text

