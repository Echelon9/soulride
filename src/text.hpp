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
// text.hpp	-thatcher 11/15/1998

// Some functions for processing and displaying text.


#ifndef TEXT_HPP
#define TEXT_HPP


#include "view.hpp"


namespace Text {

	void	Open();
	void	Close();
	void	RenderText();

	void	FormatNumber(char* buf, float number, int DigitsToLeftOfDecimal, int DigitsToRightOfDecimal);
	void	FormatTime(char* buf, int TimerTicks);

	enum FontID {
		DEFAULT = 0,
		FIXEDSYS,
		SCORE,
		// SMALL, MEDIUM, LARGE, etc...

		FONTCOUNT
	};

	int	GetWidth(FontID font, const char* buf);

	enum Alignment { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT, ALIGN_DECIMAL };

	void	DrawString(int x, int y, FontID f, Alignment a, const char* buf, uint32 ARGBColor = 0xFFFFFFFF);
	void	DrawMultiLineString(int x, int y, FontID f, Alignment a, int MaxWidth, const char* buf, uint32 ARGBColor = 0xFFFFFFFF);

	int	GetFontHeight(FontID font);
	int	GetFontBaseline(FontID font);
};


#endif // TEXT_HPP
