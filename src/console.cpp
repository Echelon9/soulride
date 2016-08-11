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
// console.cpp	-thatcher 6/27/2000 Copyright Slingshot Game Technology

// Code for a simple command-line interface to lua.


#include <stdarg.h>

#include "utility.hpp"
#include "config.hpp"
#include "error.hpp"
#include "gameloop.hpp"
#include "render.hpp"
#include "view.hpp"
#include "ogl.hpp"
#include "text.hpp"
#include "lua.hpp"
extern "C" {
#include "lua/include/lauxlib.h"
}
#include "console.hpp"


namespace Console {
;


bool	Active = false;


const int	ConsoleWidth = 80;
const int	ConsoleMaxLines = 20;
char	Buffer[ConsoleMaxLines][ConsoleWidth];
int	ActiveLine = 0;

const int	CommandLines = 10;
int	ActiveCommand = 0;
int	HistPointer = 0;
char	Command[CommandLines][ConsoleWidth];
int	CursorX = 0;


char*	DebugWatch = NULL;


#ifndef MAXPRINT
#define MAXPRINT	40  /* arbitrary limit */
#endif

static void lua_console_print ()
// Homemade replacement for the Lua 'print' function, which
// uses the console logging facilities instead of stdout.
{
	lua_Object args[MAXPRINT];
	lua_Object obj;
	int n = 0;
	int i;
	while ((obj = lua_getparam(n+1)) != LUA_NOOBJECT) {
		luaL_arg_check(n < MAXPRINT, n+1, "too many arguments");
		args[n++] = obj;
	}
	for (i=0; i<n; i++) {
		lua_pushobject(args[i]);
		if (lua_call("tostring"))
			lua_error("error in `tostring' called by `print'");
		obj = lua_getresult(1);
		if (!lua_isstring(obj))
			lua_error("`tostring' must return a string to `print'");
		if (i>0) fputs("\t", stdout);
		Log(lua_getstring(obj));
	}
	Log("\n");//xxxx
}


static void	lua_console_alert()
// Send error message to console.
{
	Log(luaL_check_string(1));
}


static void	DebugWatch_lua()
// Lua hook to set a watch expression.
{
	const char*	expr = lua_getstring(lua_getparam(1));

	if (DebugWatch) {
		delete [] DebugWatch;
		DebugWatch = NULL;
	}

	if (expr) {
		DebugWatch = new char[strlen(expr)+1];
		strcpy(DebugWatch, expr);
	}
}


void	Open()
// Initialize the console.
{
	int	i;
	for (i = 0; i < ConsoleMaxLines; i++) {
		Buffer[i][0] = 0;
	}
	ActiveLine = 0;
	
	for (i = 0; i < CommandLines; i++) {
		Command[i][0] = 0;
	}
	ActiveCommand = 0;
	CursorX = 0;

	// Register a replacement 'print' and "_ALERT" for Lua, to direct
	// messages to the console instead of stdout/stderr.
	lua_register("print", lua_console_print);
	lua_register("_ALERT", lua_console_alert);

	Log("Soul Ride console");
	Config::SetIntValue("ConsoleHeight", 10);

	lua_register("debug_watch", DebugWatch_lua);
}


void	Close()
// Shut down the console.
{
	if (DebugWatch) delete [] DebugWatch;
}


bool	IsActive()
// Returns true if the console is active.
{
	return Active;
}


void	Update(const UpdateState& u)
// Process input.
{
	char	buf[80];
	Input::GetAlphaInput(buf, 80);

	char* p = buf;

	while (*p) {
		int	c = (unsigned char) *p;

		if (c == '`') {
			Active = !Active;
			p++;
			continue;
		}
		
		if (Active) {
			if (c == 13) {
				// Execute the current command line.
				char*	command = Command[ActiveCommand];
				
				ActiveLine = (ActiveLine + 1) % ConsoleMaxLines;
				ActiveCommand = (ActiveCommand + 1) % CommandLines;

				lua_dostring(command);

				// Set up for next command.
				Buffer[ActiveLine][0] = 0;
				Command[ActiveCommand][0] = 0;
				CursorX = 0;
				HistPointer = 0;

				strcpy(Buffer[ActiveLine], "> ");
				
			} else if (c == 8) { // backspace.
				if (CursorX > 0) {
					strcpy(Command[ActiveCommand] + CursorX-1, Command[ActiveCommand] + CursorX);
					CursorX--;
				}
				
			} else if (c == 4 || c == 138) {	// ctl-D or DEL
				if (CursorX < (int) strlen(Command[ActiveCommand])) {
					strcpy(Command[ActiveCommand] + CursorX, Command[ActiveCommand] + CursorX + 1);
				}
				
			} else if (c == 16 || c == 136 || c == 137) {	// ctl-P or up or pgup
				if (HistPointer < CommandLines) {
					HistPointer++;
					int	i = ActiveCommand - HistPointer;
					if (i < 0) i += CommandLines;

					// Copy command.
					strcpy(Command[ActiveCommand], Command[i]);
					CursorX = strlen(Command[ActiveCommand]);
				}

			} else if (c == 14 || c == 130 || c == 131) {	// ctl-N or dn or pgdn
				if (HistPointer > 0) {
					HistPointer--;
					int	i = ActiveCommand - HistPointer;
					if (i < 0) i += CommandLines;

					// Copy command.
					strcpy(Command[ActiveCommand], Command[i]);
					CursorX = strlen(Command[ActiveCommand]);
				} else {
					Command[ActiveCommand][0] = 0;
					CursorX = 0;
				}

			} else if (c == 6 || c == 134) {	// ctl-F or right
				if (CursorX < (int) strlen(Command[ActiveCommand])) CursorX++;
				
			} else if (c == 2 || c == 132) {	// ctl-B or left
				if (CursorX > 0) CursorX--;

			} else if (c == 1 || c == 135) {	// ctl-A or home
				CursorX = 0;

			} else if (c == 5 || c == 129) {	// ctl-E or end
				CursorX = strlen(Command[ActiveCommand]);
				
			} else if (CursorX < ConsoleWidth - 1 && c >= 32 && c <= 126) {
				// Add the character to the current command line.
				char*	l = Command[ActiveCommand];
				int	len = strlen(l);
				if (CursorX < len) {
					// Shift remainder of line to the right.
					int	rem = len - CursorX;
					if (len >= ConsoleWidth - 1) rem = (ConsoleWidth-1) - CursorX;
					char*	p = l + (CursorX + rem);
					while (rem) {
						*p = *(p-1);
						p--;
						rem--;
					}
					l[ConsoleWidth-1] = 0;
				} else {
					l[CursorX + 1] = 0;
				}
				l[CursorX] = c;
				CursorX++;
				if (CursorX > ConsoleWidth-1) CursorX = ConsoleWidth - 1;
			}

			strcpy(Buffer[ActiveLine], "> ");
			strncat(Buffer[ActiveLine], Command[ActiveCommand], ConsoleWidth-2);
			Buffer[ActiveLine][ConsoleWidth-1] = 0;
		}
		
		p++;
	}
}


void	Log(const char* buf)
// Logs the given string to the console buffer.
{
	strncpy(Buffer[ActiveLine], buf, ConsoleWidth-1);
	Buffer[ActiveLine][ConsoleWidth-1] = 0;

	ActiveLine = (ActiveLine + 1) % ConsoleMaxLines;
	strcpy(Buffer[ActiveLine], "> ");	// prompt.
}


#ifdef WIN32
#define vsnprintf _vsnprintf
#endif // WIN32


void	Printf(const char* fmt, ...)
// printf-style logging into the console buffer.
{
//	strncpy(Buffer[ActiveLine], buf, ConsoleWidth-1);
	va_list	ap;
	va_start(ap, fmt);
	vsnprintf(Buffer[ActiveLine], ConsoleWidth-1, fmt, ap);
	va_end(ap);
	Buffer[ActiveLine][ConsoleWidth-1] = 0;

	ActiveLine = (ActiveLine + 1) % ConsoleMaxLines;
	strcpy(Buffer[ActiveLine], "> ");	// prompt.
}


void	Render(const ViewState& s)
// Draw the console.
{
	if ((!Active) && DebugWatch) {
		// Show the value of some lua expression.
		lua_beginblock();

		if (lua_dostring(DebugWatch) != 0) {
//			// Error.  Eliminate the watch.
//			delete [] DebugWatch;
//			DebugWatch = NULL;
		} else {
			lua_Object	result = lua_getresult(1);
			const char*	msg = NULL;
			if (result != LUA_NOOBJECT) msg = lua_getstring(result);
			if (msg == NULL) msg = "nil";
			Text::DrawString(10, 20, Text::FIXEDSYS, Text::ALIGN_LEFT, msg);
		}
		lua_endblock();
	}
	
	if (Active) {
		int	height = iclamp(1, Config::GetIntValue("ConsoleHeight"), ConsoleMaxLines);
		
		// Draw background.
		::Render::SetTexture(NULL);
		::Render::DisableAlphaTest();
		::Render::DisableLightmapBlend();
		::Render::CommitRenderState();
		
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glScalef(2.0f / 640, -2.0f / 480.0f, 1);
		glTranslatef(-640 / 2.0f, -480 / 2.0f, 0);
		
		glColor4f(0, 0, 0, 0.5f);
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		float	y = float(Text::GetFontHeight(Text::FIXEDSYS) * height + 6);
		glVertex2f(0, y);
		glVertex2f(640, y);
		glVertex2f(640, 0);

		glEnd();

		// Restore OpenGL state.
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		// Draw text.
		int	i;
		for (i = 0; i < height; i++) {
			int	line = (ActiveLine - i);
			if (line < 0) line += ConsoleMaxLines;

			float	y = float(height - i) * Text::GetFontHeight(Text::FIXEDSYS);
			Text::DrawString(0, (int) y, Text::FIXEDSYS, Text::ALIGN_LEFT, Buffer[line], 0xFFFFFFFF);

			if (i == 0) {
				// Draw cursor.
				float	x = float(Text::GetWidth(Text::FIXEDSYS, Buffer[line])
								  - Text::GetWidth(Text::FIXEDSYS, Command[ActiveCommand] + CursorX));
				Text::DrawString((int) x, (int) y, Text::FIXEDSYS, Text::ALIGN_LEFT, "_", 0xFFFFFFFF);
			}
		}
//		Text::DrawMultiLineString();
	}
}


}	// end namespace Console.
