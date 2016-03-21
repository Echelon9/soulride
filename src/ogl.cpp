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
// ogl.cpp	-thatcher 3/3/1999 Copyright Slingshot Game Technology

// Wrapper for OpenGL.  Allows dynamic linking of your desired OpenGL
// DLL.  Largely derived from Ryan Haksi's OpenGL.c from his Plasma
// library, see copyright below.
//
// Under Linux, this module doesn't try to do the dynamic linking; it
// only adds implementations of utility functions, and some wrappers
// for the windowing system setup stuff.  This module serves two
// purposes: 1) it provides some wrappers and initialization around a
// couple of handy OpenGL extensions, and 2) under Windows, it serves
// as a dynamic wrapper around the OpenGL DLL, so we can load a
// particular one on demand.  Under Linux, purpose 2 is basically
// pointless so all that stuff is just commented out.

/* 
 * OpenGL.c
 *
 * Part of the Plasma Game Server
 * written/copyright Ryan Haksi, 1998
 * 
 * If you use this file as part of a publically released project, please 
 * grant me the courtesy of a mention in the credits section somewhere.
 * Other than that opengl.h and opengl.c are yours to do with what you wish
 *
 * Current email address(es): cryogen@unix.infoserve.net
 * Current web page address: http://home.bc.rogers.wave.ca/borealis/ryan.html
 * 
 * If you add functions to this file, please email me the updated copy so that
 * I can maintain the most up to date copy.

 * Oh, and if anyone can figure out a way to shadow the wgl calls
 * transparently rather than the way I'm doing it now, lemme know. Its not
 * a big deal but things like this irk me.

 */


#ifdef LINUX
#include <SDL.h>
#else
#include <windows.h>
#endif // not LINUX

#include <stdlib.h>
#include <stdio.h>

#include "ogl.hpp"
#include "error.hpp"


#ifndef LINUX

void		(APIENTRY *fnglAlphaFunc)(GLenum func, GLclampf ref);
void		(APIENTRY *fnglBegin)(GLenum mode);
void		(APIENTRY *fnglBindTexture)(GLenum target, GLuint texture);
void		(APIENTRY *fnglBlendFunc)(GLenum sfactor, GLenum dfactor);
void		(APIENTRY *fnglClear)(GLbitfield mask);
void		(APIENTRY *fnglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void		(APIENTRY *fnglClearDepth)(GLclampd depth);
void		(APIENTRY *fnglColor3f)(GLfloat red, GLfloat green, GLfloat blue);
void		(APIENTRY *fnglColor3fv)(const GLfloat *v);
void		(APIENTRY *fnglColor3ub)(GLubyte r, GLubyte g, GLubyte b);
void		(APIENTRY *fnglColor3ubv)(const GLubyte *v);
void		(APIENTRY *fnglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void		(APIENTRY *fnglColor4fv)(const GLfloat *v);
void		(APIENTRY *fnglColor4ub)(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
void		(APIENTRY *fnglCullFace)(GLenum mode);
void		(APIENTRY *fnglDeleteTextures)(GLsizei n, const GLuint * textures); 
void		(APIENTRY *fnglDepthFunc)(GLenum func);
void		(APIENTRY *fnglDepthMask)(GLboolean flag);
void		(APIENTRY *fnglDisable)(GLenum cap);
void		(APIENTRY *fnglDisableClientState)(GLenum array);
void		(APIENTRY *fnglDrawArrays)(GLenum mode, GLint first, GLsizei count);
void		(APIENTRY *fnglDrawBuffer)(GLenum mode);
void		(APIENTRY *fnglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
void		(APIENTRY *fnglDrawPixels)(GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid* data);
void		(APIENTRY *fnglEnable)(GLenum cap);
void		(APIENTRY *fnglEnableClientState)(GLenum array);
void		(APIENTRY *fnglEnd)(void);
void		(APIENTRY *fnglFinish)(void);
void		(APIENTRY *fnglFlush)(void);
void		(APIENTRY *fnglFogf)(GLenum pname, GLfloat param);
void		(APIENTRY *fnglFogfv)(GLenum pname, const GLfloat* params);
void		(APIENTRY *fnglFogi)(GLenum pname, GLint param);
void		(APIENTRY *fnglFrontFace)(GLenum mode);
void		(APIENTRY *fnglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void		(APIENTRY *fnglGenTextures)(GLsizei n, GLuint* textures);
GLenum		(APIENTRY *fnglGetError)(void);
void		(APIENTRY *fnglGetFloatv)(GLenum pname, GLfloat *params);
void		(APIENTRY *fnglGetIntegerv)(GLenum pname, GLint *params);
const GLubyte * (APIENTRY *fnglGetString)(GLenum name);
void		(APIENTRY *fnglHint)(GLenum target, GLenum mode);
GLboolean	(APIENTRY *fnglIsEnabled)(GLenum cap);
void		(APIENTRY *fnglLoadIdentity)(void);
void		(APIENTRY *fnglLoadMatrixf)(const GLfloat *m);
void		(APIENTRY *fnglMatrixMode)(GLenum mode);
void		(APIENTRY *fnglMultMatrixf)(const GLfloat *m);
void		(APIENTRY *fnglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void		(APIENTRY *fnglPixelStoref)(GLenum pname, GLfloat param);
void		(APIENTRY *fnglPixelStorei)(GLenum pname, GLint param);
void		(APIENTRY *fnglPixelTransferf)(GLenum pname, GLfloat param);
void		(APIENTRY *fnglPolygonMode)(GLenum face, GLenum mode);
void		(APIENTRY *fnglPolygonOffset)(GLfloat factor, GLfloat units);
void		(APIENTRY *fnglPopMatrix)(void);
void		(APIENTRY *fnglPushMatrix)(void);
void		(APIENTRY *fnglRasterPos2f)(GLfloat x, GLfloat y);
void		(APIENTRY *fnglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
void		(APIENTRY *fnglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRY *fnglScalef)(GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRY *fnglShadeModel)(GLenum mode);
void		(APIENTRY *fnglTexCoord2f)(GLfloat s, GLfloat t);
void		(APIENTRY *fnglTexCoord2fv)(const GLfloat *v);
void		(APIENTRY *fnglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
void		(APIENTRY *fnglTexCoord3fv)(const GLfloat *v);
void		(APIENTRY *fnglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr);
void		(APIENTRY *fnglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
void		(APIENTRY *fnglTexEnvi)(GLenum target, GLenum pname, GLint param);
void		(APIENTRY *fnglTexGenfv)(GLenum, GLenum, const GLfloat*);
void		(APIENTRY *fnglTexGeni)(GLenum, GLenum, GLint);
void		(APIENTRY *fnglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void		(APIENTRY *fnglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
void		(APIENTRY *fnglTexParameteri)(GLenum target, GLenum pname, GLint param);
void		(APIENTRY *fnglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels); 
void		(APIENTRY *fnglTranslatef)(GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRY *fnglVertex2f)(GLfloat x, GLfloat y);
void		(APIENTRY *fnglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRY *fnglVertex3fv)(const GLfloat *v);
void		(APIENTRY *fnglVertex3i)(GLint x, GLint y, GLint z);
void		(APIENTRY *fnglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void		(APIENTRY *fnglVertex4fv)(const GLfloat* v);
void		(APIENTRY *fnglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr);
void		(APIENTRY *fnglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

#endif // not LINUX


// Optional extensions.
extern "C" {
void		(APIENTRY *fnglLockArraysEXT)(GLint first, GLsizei count) = NULL;
void		(APIENTRY *fnglUnlockArraysEXT)() = NULL;
};


namespace OGL {
;


#ifndef LINUX
HINSTANCE	OpenglInst = NULL;
bool	OpenglBypassGDI = false;
#endif // LINUX

bool	CVAEnabled = false;
bool	LODBiasEnabled = false;
bool	EdgeClampEnabled = false;


bool	LoadExtension(void** func, const char* name)
// Attempts to load a function pointer to the given named extension.  On
// success, assigns the function pointer value to *func and returns true.
// Returns false on failure.
{
#ifndef LINUX
	if (!OpenglInst) return false;

	PROC	(APIENTRY *fnwglGetProcAddress)(LPCSTR lpszProc);
	*((void**)&fnwglGetProcAddress) = (void*)GetProcAddress(OpenglInst, "wglGetProcAddress");
	if (!fnwglGetProcAddress) return false;
 
	void*	p = (*fnwglGetProcAddress)(name);
	if (p) {
		*func = p;
		return true;
	} else {
		return false;
	}
#else

	void*	p = SDL_GL_GetProcAddress(name);
	if (p) {
		*func = p;
		return true;
	} else {
		return false;
	}
#endif
}


int Bind(const char *lib)
// Given the DLL name of the desired OpenGL driver, bind it to our
// OpenGL function calls.
// Returns 0 on success, -1 on failure.
{
#ifndef LINUX

	// This is the unspeakably lame thing you have to do to get rid of the 3Dfx splash logo.
	_putenv("FX_GLIDE_NO_SPLASH=0");

	// Load DLL. 
	OpenglInst = LoadLibrary(lib); 
	if (OpenglInst == NULL) {
		return -1;
	}

	/* load all the function pointers */
	if (!(*((void**)&fnglAlphaFunc)	= (void*)GetProcAddress(OpenglInst, "glAlphaFunc"))) return -1;
	if (!(*((void**)&fnglBegin)		= (void*)GetProcAddress(OpenglInst, "glBegin"))) return -1;
	if (!(*((void**)&fnglBindTexture)	= (void*)GetProcAddress(OpenglInst, "glBindTexture"))) return -1;
	if (!(*((void**)&fnglBlendFunc)	= (void*)GetProcAddress(OpenglInst, "glBlendFunc"))) return -1;
	if (!(*((void**)&fnglClear)		= (void*)GetProcAddress(OpenglInst, "glClear"))) return -1;
	if (!(*((void**)&fnglClearColor)	= (void*)GetProcAddress(OpenglInst, "glClearColor"))) return -1;
	if (!(*((void**)&fnglClearDepth)	= (void*)GetProcAddress(OpenglInst, "glClearDepth"))) return -1;
	if (!(*((void**)&fnglColor3f)	= (void*)GetProcAddress(OpenglInst, "glColor3f"))) return -1;
	if (!(*((void**)&fnglColor3fv)	= (void*)GetProcAddress(OpenglInst, "glColor3fv"))) return -1;
	if (!(*((void**)&fnglColor3ub)	= (void*)GetProcAddress(OpenglInst, "glColor3ub"))) return -1;
	if (!(*((void**)&fnglColor3ubv)	= (void*)GetProcAddress(OpenglInst, "glColor3ubv"))) return -1;
	if (!(*((void**)&fnglColor4f)	= (void*)GetProcAddress(OpenglInst, "glColor4f"))) return -1;
	if (!(*((void**)&fnglColor4fv)	= (void*)GetProcAddress(OpenglInst, "glColor4fv"))) return -1;
	if (!(*((void**)&fnglColor4ub)	= (void*)GetProcAddress(OpenglInst, "glColor4ub"))) return -1;
	if (!(*((void**)&fnglCullFace)	= (void*)GetProcAddress(OpenglInst, "glCullFace"))) return -1;
	if (!(*((void**)&fnglDeleteTextures)	= (void*)GetProcAddress(OpenglInst, "glDeleteTextures"))) return -1;
	if (!(*((void**)&fnglDepthFunc)	= (void*)GetProcAddress(OpenglInst, "glDepthFunc"))) return -1;
	if (!(*((void**)&fnglDepthMask)	= (void*)GetProcAddress(OpenglInst, "glDepthMask"))) return -1;
	if (!(*((void**)&fnglDisable)	= (void*)GetProcAddress(OpenglInst, "glDisable"))) return -1;
	if (!(*((void**)&fnglDisableClientState)	= (void*)GetProcAddress(OpenglInst, "glDisableClientState"))) return -1;
	if (!(*((void**)&fnglDrawArrays)	= (void*)GetProcAddress(OpenglInst, "glDrawArrays"))) return -1;
	if (!(*((void**)&fnglDrawBuffer)	= (void*)GetProcAddress(OpenglInst, "glDrawBuffer"))) return -1;
	if (!(*((void**)&fnglDrawElements)	= (void*)GetProcAddress(OpenglInst, "glDrawElements"))) return -1;
	if (!(*((void**)&fnglDrawPixels)	= (void*)GetProcAddress(OpenglInst, "glDrawPixels"))) return -1;
	if (!(*((void**)&fnglEnable)	= (void*)GetProcAddress(OpenglInst, "glEnable"))) return -1;
	if (!(*((void**)&fnglEnableClientState)	= (void*)GetProcAddress(OpenglInst, "glEnableClientState"))) return -1;
	if (!(*((void**)&fnglEnd)		= (void*)GetProcAddress(OpenglInst, "glEnd"))) return -1;
	if (!(*((void**)&fnglFinish)	= (void*)GetProcAddress(OpenglInst, "glFinish"))) return -1;
	if (!(*((void**)&fnglFlush)	= (void*)GetProcAddress(OpenglInst, "glFlush"))) return -1;
	if (!(*((void**)&fnglFogf)	= (void*)GetProcAddress(OpenglInst, "glFogf"))) return -1;
	if (!(*((void**)&fnglFogfv)	= (void*)GetProcAddress(OpenglInst, "glFogfv"))) return -1;
	if (!(*((void**)&fnglFogi)	= (void*)GetProcAddress(OpenglInst, "glFogi"))) return -1;
	if (!(*((void**)&fnglFrontFace)	= (void*)GetProcAddress(OpenglInst, "glFrontFace"))) return -1;
	if (!(*((void**)&fnglFrustum)	= (void*)GetProcAddress(OpenglInst, "glFrustum"))) return -1;
	if (!(*((void**)&fnglGenTextures)	= (void*)GetProcAddress(OpenglInst, "glGenTextures"))) return -1;
	if (!(*((void**)&fnglGetError)	= (void*)GetProcAddress(OpenglInst, "glGetError"))) return -1;
	if (!(*((void**)&fnglGetFloatv)	= (void*)GetProcAddress(OpenglInst, "glGetFloatv"))) return -1;
	if (!(*((void**)&fnglGetIntegerv)	= (void*)GetProcAddress(OpenglInst, "glGetIntegerv"))) return -1;
	if (!(*((void**)&fnglGetString)	= (void*)GetProcAddress(OpenglInst, "glGetString"))) return -1;
	if (!(*((void**)&fnglHint)		= (void*)GetProcAddress(OpenglInst, "glHint"))) return -1;
	if (!(*((void**)&fnglIsEnabled)	= (void*)GetProcAddress(OpenglInst, "glIsEnabled"))) return -1;
	if (!(*((void**)&fnglLoadIdentity)	= (void*)GetProcAddress(OpenglInst, "glLoadIdentity"))) return -1;
	if (!(*((void**)&fnglLoadMatrixf)	= (void*)GetProcAddress(OpenglInst, "glLoadMatrixf"))) return -1;
	if (!(*((void**)&fnglMatrixMode)	= (void*)GetProcAddress(OpenglInst, "glMatrixMode"))) return -1;
	if (!(*((void**)&fnglMultMatrixf)	= (void*)GetProcAddress(OpenglInst, "glMultMatrixf"))) return -1;
	if (!(*((void**)&fnglOrtho)		= (void*)GetProcAddress(OpenglInst, "glOrtho"))) return -1;
	if (!(*((void**)&fnglPixelStoref)	= (void*)GetProcAddress(OpenglInst, "glPixelStoref"))) return -1;
	if (!(*((void**)&fnglPixelStorei)	= (void*)GetProcAddress(OpenglInst, "glPixelStorei"))) return -1;
	if (!(*((void**)&fnglPixelTransferf)	= (void*)GetProcAddress(OpenglInst, "glPixelTransferf"))) return -1;
	if (!(*((void**)&fnglPolygonMode) = (void*)GetProcAddress(OpenglInst, "glPolygonMode"))) return -1;
	if (!(*((void**)&fnglPolygonOffset) = (void*)GetProcAddress(OpenglInst, "glPolygonOffset"))) return -1;
	if (!(*((void**)&fnglPopMatrix)	= (void*)GetProcAddress(OpenglInst, "glPopMatrix"))) return -1;
	if (!(*((void**)&fnglPushMatrix)	= (void*)GetProcAddress(OpenglInst, "glPushMatrix"))) return -1;
	if (!(*((void**)&fnglRasterPos2f)	= (void*)GetProcAddress(OpenglInst, "glRasterPos2f"))) return -1;
	if (!(*((void**)&fnglReadPixels)	= (void*)GetProcAddress(OpenglInst, "glReadPixels"))) return -1;
	if (!(*((void**)&fnglRotatef)	= (void*)GetProcAddress(OpenglInst, "glRotatef"))) return -1;
	if (!(*((void**)&fnglScalef)	= (void*)GetProcAddress(OpenglInst, "glScalef"))) return -1;
	if (!(*((void**)&fnglShadeModel)	= (void*)GetProcAddress(OpenglInst, "glShadeModel"))) return -1;
	if (!(*((void**)&fnglTexCoord2f)	= (void*)GetProcAddress(OpenglInst, "glTexCoord2f"))) return -1;
	if (!(*((void**)&fnglTexCoord2fv)	= (void*)GetProcAddress(OpenglInst, "glTexCoord2fv"))) return -1;
	if (!(*((void**)&fnglTexCoord3f)	= (void*)GetProcAddress(OpenglInst, "glTexCoord3f"))) return -1;
	if (!(*((void**)&fnglTexCoord3fv)	= (void*)GetProcAddress(OpenglInst, "glTexCoord3fv"))) return -1;
	if (!(*((void**)&fnglTexCoordPointer)	= (void*)GetProcAddress(OpenglInst, "glTexCoordPointer"))) return -1;
	if (!(*((void**)&fnglTexEnvf)	= (void*)GetProcAddress(OpenglInst, "glTexEnvf"))) return -1;
	if (!(*((void**)&fnglTexEnvi)	= (void*)GetProcAddress(OpenglInst, "glTexEnvi"))) return -1;
	if (!(*((void**)&fnglTexGenfv)	= (void*)GetProcAddress(OpenglInst, "glTexGenfv"))) return -1;
	if (!(*((void**)&fnglTexGeni)	= (void*)GetProcAddress(OpenglInst, "glTexGeni"))) return -1;
	if (!(*((void**)&fnglTexImage2D)	= (void*)GetProcAddress(OpenglInst, "glTexImage2D"))) return -1;
	if (!(*((void**)&fnglTexParameterf) = (void*)GetProcAddress(OpenglInst, "glTexParameterf"))) return -1;
	if (!(*((void**)&fnglTexParameteri) = (void*)GetProcAddress(OpenglInst, "glTexParameteri"))) return -1;
	if (!(*((void**)&fnglTexSubImage2D) = (void*)GetProcAddress(OpenglInst, "glTexSubImage2D"))) return -1;
	if (!(*((void**)&fnglTranslatef)	= (void*)GetProcAddress(OpenglInst, "glTranslatef"))) return -1;
	if (!(*((void**)&fnglVertex2f)	= (void*)GetProcAddress(OpenglInst, "glVertex2f"))) return -1;
	if (!(*((void**)&fnglVertex3f)	= (void*)GetProcAddress(OpenglInst, "glVertex3f"))) return -1;
	if (!(*((void**)&fnglVertex3fv)	= (void*)GetProcAddress(OpenglInst, "glVertex3fv"))) return -1;
	if (!(*((void**)&fnglVertex3i)	= (void*)GetProcAddress(OpenglInst, "glVertex3i"))) return -1;
	if (!(*((void**)&fnglVertex4f)	= (void*)GetProcAddress(OpenglInst, "glVertex4f"))) return -1;
	if (!(*((void**)&fnglVertex4fv)	= (void*)GetProcAddress(OpenglInst, "glVertex4fv"))) return -1;
	if (!(*((void**)&fnglVertexPointer)	= (void*)GetProcAddress(OpenglInst, "glVertexPointer"))) return -1;
	if (!(*((void**)&fnglViewport)	= (void*)GetProcAddress(OpenglInst, "glViewport"))) return -1;

	/* 
	 * The 3dfxgl.dll minidriver bypasses the GDI entirely
	 * I do a proper failover, but the GDI failover call for SwapBuffers is
	 * REALLY bloody slow, so best to avoid it alltogether. (when using winglide
	 * for debugging I was going from 8 fps to 5fps, thats 37.5%!!!!)
	 */
	if (strcmp(lib, "opengl32.dll")) OpenglBypassGDI = true;

	/* 
	 * The Matrox D3D wrapper fails completely unless the GDI is bypassed?
	 * but glGetString doesnt work until after MakeCurrentContext, how do I detect?
	 */
	/* Cant do anything here, couldnt get the call to work in MakeCurrentContext */

	/* going to try remembering the results of the pixel format calls */

#endif // not LINUX

	return 0;
}


bool	CheckExtension(const char* extension)
// Some extension checking code snipped from glut.
{
	static const char*	extensions = NULL;
	const char*	start;
	char*	where;
	char*	terminator;
	bool	supported;
	
	// Extension names should not have spaces
	where = strchr(extension, ' ');
	if (where || *extension == '\0') return false;
	
	// Grab extensions (but only once)
	if (!extensions) extensions = (const char*)glGetString(GL_EXTENSIONS);
	
	// Look for extension
	start = extensions;
	supported = false;
	while (!supported) {
		// Does extension SEEM to be supported?
		where = strstr((const char*)start, extension);
		if (!where) break;

		// Ok, extension SEEMS to be supported
		supported = true;

		// Check for space before extension
		supported &= (where == start) || (where[-1] == ' ');

		// Check for space after extension
		terminator = where + strlen(extension);
		supported &= (*terminator == '\0') || (*terminator == ' ');

		// Next search starts at current terminator
		start = terminator;
	}

	return supported;
}


void	BindExtensions()
// Binds any available extensions that we know about.
{
	// Load optional extensions.

	{
		// Compiled vertex arrays.
#if ENABLE_CVA	// may break ATI's drivers!
		CVAEnabled = true;
		if (LoadExtension(((void**)&fnglLockArraysEXT), "glLockArraysEXT") == false) CVAEnabled = false;
		if (LoadExtension(((void**)&fnglUnlockArraysEXT), "glUnlockArraysEXT") == false) CVAEnabled = false;
//		fnglUnlockArraysEXT = (void (*)()) SDL_GL_GetProcAddress("glUnlockArraysEXT");
#endif // ENABLE_CVA

		// Texture LOD bias.
		LODBiasEnabled = CheckExtension("GL_SGIS_texture_lod");

		// Texture edge clamp.
		EdgeClampEnabled = CheckExtension("GL_SGIS_texture_edge_clamp") |
				   CheckExtension("GL_EXT_texture_edge_clamp");
	}

//	printf("glUnlockArraysEXT = %x\n", fnglUnlockArraysEXT);//xxxxxx
}


bool	GetLODBiasEnabled()
// Returns true if the GL_EXT_texture_lod_bias extension is available.
{
	return LODBiasEnabled;
}


bool	GetCVAEnabled()
// Returns true if the compiled vertex array extension is available.
{
	return CVAEnabled;
}


bool	GetEdgeClampEnabled()
// Returns true if GL_EXT_texture_edge_clamp or GL_SGIS_texture_edge_clamp extensions
// are valid.  If so, then GL_CLAMP_TO_EDGE_EXT is valid.
{
	return EdgeClampEnabled;
}


void Unbind()
// Free the OpenGL DLL.  For completeness, should set all the function
// pointers to NULL.  Maybe later.
{
#ifndef LINUX
	FreeLibrary(OpenglInst); 
	OpenglInst = NULL; 
#endif // LINUX
}


#ifdef LINUX


bool	SwapIntervalEXT(int interval)
{
	return true;
}


#else // not LINUX


/******************************************************************************/
/* replaced WGL* GDI functions */

/*
 * These cant? be fixed like the above functions becuase they are declared in gdi.h
 * and for some reason the compiler will insist on declaring them as EXPORTED
 * dll functions... weird
 */

// thatcher: not sure what he's talking about above.  Does it refer to an earlier
// version where his functions had the same names as the wgl* functions?


HGLRC	CreateContext(HDC hdc)
{
	HGLRC (APIENTRY *fnwglCreateContext)(HDC hdc);

	if (!OpenglInst) return NULL;
	*((void**)&fnwglCreateContext) = (void*)GetProcAddress(OpenglInst, "wglCreateContext");
	if (!fnwglCreateContext) return NULL;
	return (*fnwglCreateContext)(hdc);
}


bool	DeleteContext(HGLRC hglrc)
{
	BOOL (APIENTRY *fnwglDeleteContext)(HGLRC hglrc);

	if (!OpenglInst) return FALSE;
	*((void**)&fnwglDeleteContext) = (void*)GetProcAddress(OpenglInst, "wglDeleteContext");
	if (!fnwglDeleteContext) return FALSE;
	return (*fnwglDeleteContext)(hglrc) ? true : false;
}


bool	MakeCurrent(HDC hdc, HGLRC hglrc)
{
	BOOL (APIENTRY *fnwglMakeCurrent)(HDC hdc, HGLRC hglrc);

	if (!OpenglInst) return FALSE;
	*((void**)&fnwglMakeCurrent) = (void*)GetProcAddress(OpenglInst, "wglMakeCurrent");
	if (!fnwglMakeCurrent) return FALSE;
	return (*fnwglMakeCurrent)(hdc, hglrc) ? true : false;
}


bool	ShareLists(HGLRC rc1, HGLRC rc2)
{
	BOOL (APIENTRY *fnwglShareLists)(HGLRC rc1, HGLRC rc2);

	if (!OpenglInst) return FALSE;
	*((void**)&fnwglShareLists) = (void*)GetProcAddress(OpenglInst, "wglShareLists");
	if (!fnwglShareLists) return FALSE;
	return (*fnwglShareLists)(rc1, rc2) ? true : false;
}


bool	SwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
	BOOL (APIENTRY *fnwglSwapLayerBuffers)(HDC hdc, UINT fuPlanes);

	if (!OpenglInst) return FALSE;
	*((void**)&fnwglSwapLayerBuffers) = (void*)GetProcAddress(OpenglInst, "wglSwapLayerBuffers");
	if (!fnwglSwapLayerBuffers) return FALSE;
	return (*fnwglSwapLayerBuffers)(hdc, fuPlanes) ? true : false;
}


bool	SwapBuffers(HDC hdc)
{
	BOOL (APIENTRY *fnSwapBuffers)(HDC hdc);

	if (!OpenglBypassGDI) {
		// Try GDI version first
		int retValue;
		retValue = ::SwapBuffers(hdc);
		if (retValue) return retValue ? true : false;
	}
	if (!OpenglInst) return FALSE;
	*((void**)&fnSwapBuffers) = (void*)GetProcAddress(OpenglInst, "wglSwapBuffers");
	if (!fnSwapBuffers) return FALSE;
	return (*fnSwapBuffers)(hdc) ? true : false;
}


int	ChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
	int (APIENTRY *fnChoosePixelFormat)(HDC hdc, CONST PIXELFORMATDESCRIPTOR * ppfd);

	if (!OpenglBypassGDI) {
		// Try GDI version first
		int retValue;
		retValue = ::ChoosePixelFormat(hdc, ppfd);
		if (retValue) return retValue;
	}
	// Bypass GDI entirely
	if (!OpenglInst) return 0;
	*((void**)&fnChoosePixelFormat) = (void*)GetProcAddress(OpenglInst, "wglChoosePixelFormat");
	if (!fnChoosePixelFormat) return 0;
	OpenglBypassGDI = true;
	return (*fnChoosePixelFormat)(hdc, ppfd); 
}


int	DescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
	int (APIENTRY *fnDescribePixelFormat)(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);

	if (!OpenglBypassGDI) {
		// Try GDI version first
		int retValue;
		retValue = ::DescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
		if (retValue) return retValue;
	}
	// Bypass GDI entirely
	if (!OpenglInst) return FALSE;
	*((void**)&fnDescribePixelFormat) = (void*)GetProcAddress(OpenglInst, "wglDescribePixelFormat");
	if (!fnDescribePixelFormat) return FALSE;
	OpenglBypassGDI = true;
	return (*fnDescribePixelFormat)(hdc, iPixelFormat, nBytes, ppfd);
}


BOOL	SetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd)
{
	BOOL (APIENTRY *fnSetPixelFormat)(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR * ppfd);

	if (!OpenglBypassGDI) {
		// Try GDI version first
		int retValue;
		retValue = ::SetPixelFormat(hdc, iPixelFormat, ppfd);
		if (retValue) return retValue;
	}
	// Bypass GDI entirely
	if (!OpenglInst) return FALSE;
	*((void**)&fnSetPixelFormat) = (void*)GetProcAddress(OpenglInst, "wglSetPixelFormat");
	if (!fnSetPixelFormat) return FALSE;
	OpenglBypassGDI = true;
	return (*fnSetPixelFormat)(hdc, iPixelFormat, ppfd);
}


bool	SwapIntervalEXT(int interval)
{
	BOOL (APIENTRY *fnwglSwapIntervalEXT)(int);

	if (!OpenglInst) return FALSE;
	if (LoadExtension((void**) &fnwglSwapIntervalEXT, "wglSwapIntervalEXT") == false) return FALSE;
	return (*fnwglSwapIntervalEXT)(interval) ? true : false;
}


#endif // not LINUX


}; // end namespace OGL
;


#ifndef LINUX


//
// gl* wrappers
//


#ifndef OGL_INLINE


void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) {
	(*fnglAlphaFunc)(func,ref);
}

void APIENTRY glBegin(GLenum mode) {
	(*fnglBegin)(mode);
}

void APIENTRY glBindTexture(GLenum target, GLuint texture) {
	(*fnglBindTexture)(target,texture);
}


void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor) {
	(*fnglBlendFunc)(sfactor,dfactor);
}


void APIENTRY glClear(GLbitfield mask) {
	(*fnglClear)(mask);
}


void	APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	(*fnglClearColor)(red,green,blue,alpha);
}


void	APIENTRY glClearDepth (GLclampd depth) {
	(*fnglClearDepth)(depth);
}


void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	(*fnglColor3f)(red,green,blue);
}


void APIENTRY glColor3fv(const GLfloat *v) {
	(*fnglColor3fv)(v);
}


void APIENTRY glColor3ub(const GLubyte r, const GLubyte g, const GLubyte b) {
	(*fnglColor3ub)(r, g, b);
}


void APIENTRY glColor3ubv(const GLubyte *v) {
	(*fnglColor3ubv)(v);
}


void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	(*fnglColor4f)(red,green,blue,alpha);
}


void APIENTRY glColor4fv(const GLfloat *v) {
	(*fnglColor4fv)(v);
}


void APIENTRY glColor4ub(const GLubyte r, const GLubyte g, const GLubyte b, const GLubyte a) {
	(*fnglColor4ub)(r, g, b, a);
}


void APIENTRY glCullFace(GLenum mode) {
	(*fnglCullFace)(mode);
}


void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures) {
	(*fnglDeleteTextures)(n, textures);
}


void APIENTRY glDepthFunc(GLenum func) {
	(*fnglDepthFunc)(func);
}


void APIENTRY glDepthMask(GLboolean flag) {
	(*fnglDepthMask)(flag);
}


void APIENTRY glDisable(GLenum cap) {
	(*fnglDisable)(cap);
}


void APIENTRY glDisableClientState(GLenum cap) {
	(*fnglDisableClientState)(cap);
}


void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {
	(*fnglDrawArrays)(mode, first, count);
}


void APIENTRY glDrawBuffer(GLenum mode) {
	(*fnglDrawBuffer)(mode);
}


void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
	(*fnglDrawElements)(mode, count, type, indices);
}


void APIENTRY glDrawPixels(GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid* pixels) {
	(*fnglDrawPixels)(w, h, format, type, pixels);
}


void APIENTRY glEnable(GLenum cap) {
	(*fnglEnable)(cap);
}


void APIENTRY glEnableClientState(GLenum cap) {
	(*fnglEnableClientState)(cap);
}


void APIENTRY glEnd(void) {
	(*fnglEnd)();
}

void APIENTRY glFinish(void) {
	(*fnglFinish)();
}


void APIENTRY glFlush(void) {
	(*fnglFlush)();
}


void APIENTRY glFogf(GLenum pname, GLfloat param) {
	(*fnglFogf)(pname, param);
}

void APIENTRY glFogfv(GLenum pname, const GLfloat* params) {
	(*fnglFogfv)(pname, params);
}

void APIENTRY glFogi(GLenum pname, GLint param) {
	(*fnglFogi)(pname, param);
}

void APIENTRY glFrontFace(GLenum mode) {
	(*fnglFrontFace)(mode);
}


void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	(*fnglFrustum)(left,right,bottom,top,zNear,zFar);
}


void APIENTRY glGenTextures(GLsizei n, GLuint* textures) {
	(*fnglGenTextures)(n, textures);
}


GLenum APIENTRY glGetError(void) {
	return (*fnglGetError)();
}


void APIENTRY glGetFloatv(GLenum pname, GLfloat *params) {
	(*fnglGetFloatv)(pname, params);
}


void APIENTRY glGetIntegerv(GLenum pname, GLint *params) {
	(*fnglGetIntegerv)(pname,params);
}


const GLubyte * APIENTRY glGetString(GLenum name) {
	return (*fnglGetString)(name);
}


void APIENTRY glHint(GLenum target, GLenum mode) {
	(*fnglHint)(target,mode);
}


GLboolean APIENTRY glIsEnabled(GLenum cap) {
	return (*fnglIsEnabled)(cap);
}


void APIENTRY glLoadIdentity(void) {
	(*fnglLoadIdentity)();
}


void APIENTRY glLoadMatrixf(const GLfloat *m) {
	(*fnglLoadMatrixf)(m);
}


void APIENTRY glMatrixMode(GLenum mode) {
	(*fnglMatrixMode)(mode);
}


void APIENTRY glMultMatrixf(const GLfloat *m) {
	(*fnglMultMatrixf)(m);
}


void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	(*fnglOrtho)(left,right,bottom,top,zNear,zFar);
}


void	APIENTRY glPixelStoref (GLenum pname, GLfloat param) {
	(*fnglPixelStorei)(pname, int(param));
}


void	APIENTRY glPixelStorei (GLenum pname, GLint param) {
	(*fnglPixelStorei)(pname, param);
}


void	APIENTRY glPixelTransferf (GLenum pname, GLfloat param) {
	(*fnglPixelTransferf)(pname, param);
}


void APIENTRY glPolygonMode(GLenum face, GLenum mode) {
	(*fnglPolygonMode)(face, mode);
}


void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units) {
	(*fnglPolygonOffset)(factor,units);
}


void APIENTRY glPopMatrix(void) {
	(*fnglPopMatrix)();
}


void APIENTRY glPushMatrix(void) {
	(*fnglPushMatrix)();
}


void APIENTRY glRasterPos2f(GLfloat x, GLfloat y) {
	(*fnglRasterPos2f)(x,y);
}

void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels) {
	(*fnglReadPixels)(x, y, width, height, format, type, pixels);
}

void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	(*fnglRotatef)(angle,x,y,z);
}


void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglScalef)(x,y,z);
}


void	APIENTRY glShadeModel (GLenum mode) {
	(*fnglShadeModel)(mode);
}


void APIENTRY glTexCoord2f(GLfloat s, GLfloat t) {
	(*fnglTexCoord2f)(s,t);
}


void APIENTRY glTexCoord2fv(const GLfloat *v) {
	(*fnglTexCoord2fv)(v);
}


void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {
	(*fnglTexCoord3f)(s,t,r);
}


void APIENTRY glTexCoord3fv(const GLfloat *v) {
	(*fnglTexCoord3fv)(v);
}


void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr) {
	(*fnglTexCoordPointer)(size, type, stride, ptr);
}


void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
	(*fnglTexEnvf)(target,pname,param);
}


void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param) {
	(*fnglTexEnvi)(target,pname,param);
}


void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat* param) {
	(*fnglTexGenfv)(coord,pname,param);
}


void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param) {
	(*fnglTexGeni)(coord,pname,param);
}


void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexImage2D)(target,level,internalformat,width,height,border,format,type,pixels);
}


void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
	(*fnglTexParameterf)(target,pname,param);
}


void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param) {
	(*fnglTexParameteri)(target,pname,param);
}


void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}


void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglTranslatef)(x,y,z);
}


void APIENTRY glVertex2f(GLfloat x, GLfloat y) {
	(*fnglVertex2f)(x,y);
}


void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglVertex3f)(x,y,z);
}


void APIENTRY glVertex3fv(const GLfloat *v) {
	(*fnglVertex3fv)(v);
}


void APIENTRY glVertex3i(GLint x, GLint y, GLint z) {
	(*fnglVertex3i)(x,y,z);
}


void APIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	(*fnglVertex4f)(x,y,z,w);
}


void APIENTRY glVertex4fv(const GLfloat* v) {
	(*fnglVertex4fv)(v);
}


void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* ptr) {
	(*fnglVertexPointer)(size, type, stride, ptr);
}


void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	(*fnglViewport)(x,y,width,height);
}


#endif // not OGL_INLINE

#endif // not LINUX


//
// Optional extensions.
//


#ifndef LINUX

void	glLockArraysEXT(GLint first, GLsizei count) {
#if ENABLE_CVA	// may break ATI!
	if (fnglLockArraysEXT) {
		(*fnglLockArraysEXT)(first, count);
	}
#endif // ENABLE_CVA
}


void	glUnlockArraysEXT() {
#if ENABLE_CVA	// may break ATI!
	if (fnglUnlockArraysEXT) {
		(*fnglUnlockArraysEXT)();
	}
#endif // ENABLE_CVA
}

#endif // not LINUX
