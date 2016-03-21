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
// ogl.hpp	-thatcher 3/3/1999 Copyright Slingshot Game Technology

// Largely derived from Ryan Haksi's opengl.h/.c from his Plasma library,
// see his copyright in ogl.cpp.
//
// Linux version of this file is largely just a wrapper for including the
// standard OpenGL headers.

#ifndef OGL_HPP
#define OGL_HPP


/* Important note: Do not include any of the gl includes yourself!
 * include only this file, it will load gl.h etc properly so that
 * the compiler wont think it is calling a staticly bound dll 
 */


#if defined(LINUX) || defined(MACOSX)

#	define APIENTRY
	typedef int	HGLRC;
	typedef int	HDC;
	inline HDC	GetDC(int winhandle) { return winhandle; }

#else // not Linux

	/* do this so that it wont declare everything as dllimport */
#	undef WINGDIAPI
#	define WINGDIAPI
	
#	ifndef _INC_WINDOWS
		#define APIENTRY __stdcall
		#define CALLBACK __stdcall
#	endif
	
#	ifndef WIN32
#		define WIN32
#		define OGL_HPP_FAKEWIN32_DEF
#	endif
	
#	ifndef _WCHAR_T_DEFINED
		typedef unsigned short wchar_t;
#		define _WCHAR_T_DEFINED
#	endif // _WCHAR_T_DEFINED
	
#	ifdef __GL_H__
#		error Do not manually include gl.h or glu.h
#	endif
#	ifdef __gl_h__
#		error Do not manually include gl.h or glu.h
#	endif
	
	//#include <GL/gl.h>
	//#include <GL/glu.h>
	
	// Optional extensions.
#	ifndef _INC_WINDOWS
		typedef void* HGLRC;
		typedef void* HDC;
		typedef unsigned int UINT;
		typedef int BOOL;
		class PIXELFORMATDESCRIPTOR;
#	endif

#endif // not LINUX

#ifdef MACOSX
#	include <OpenGL/gl.h>
#else
#	include <GL/gl.h>
#endif
	

extern "C" {
void	glLockArraysEXT(GLint first, GLsizei count);
void	glUnlockArraysEXT(void);
};


#define GL_TEXTURE_FILTER_CONTROL_EXT	0x8500
#define GL_TEXTURE_LOD_BIAS_EXT	0x8501

#define GL_TEXTURE_MIN_LOD_SGIS		0x813A
#define GL_TEXTURE_MAX_LOD_SGIS		0x813B
#define GL_TEXTURE_BASE_LEVEL_SGIS	0x813C
#define GL_TEXTURE_MAX_LEVEL_SGIS	0x813D

#define GL_CLAMP_TO_EDGE_EXT                0x812F


namespace OGL {
	int	Bind(const char *lib);
	void	BindExtensions();	// Call after getting a rendering context.
	void	Close();

#ifndef LINUX
	/* Use these instead of the wgl* functions */
	HGLRC	CreateContext(HDC hdc);
	bool	DeleteContext(HGLRC hglrc);
	bool	MakeCurrent(HDC hdc, HGLRC hglrc);
	bool	ShareLists(HGLRC rc1, HGLRC rc2);
//	bool	SwapLayerBuffers(HDC hdc, UINT fuPlanes);

	/* use these functions instead of the GDI ones */
	bool	SwapBuffers(HDC hdc);
	int	ChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR * ppfd);
	int	DescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR* ppfd);
	BOOL	SetPixelFormat(HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR * ppfd);
#endif // LINUX

	// Extension for controlling swap interval.
	bool	SwapIntervalEXT(int interval);

	// Returns true if glLock/UnlockArraysEXT() are implemented.
	// Otherwise they're no-ops, so it's safe to call them in any
	// case.
	bool	GetCVAEnabled();

	// Returns true if GL_CLAMP_TO_EDGE_EXT can be passed to glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_..., ...).
	bool	GetEdgeClampEnabled();

	// Returns true GL_TEXTURE_FILTER_CONTROL_EXT &
	// GL_TEXTURE_LOD_BIAS_EXT can be passed to glTexEnv*().
	bool	GetLODBiasEnabled();
};


#ifndef LINUX


//#define OGL_INLINE
#ifdef OGL_INLINE


// Function pointers to gl calls.
extern void		(APIENTRY *fnglAlphaFunc)(GLenum func, GLclampf ref);
extern void		(APIENTRY *fnglBegin)(GLenum mode);
extern void		(APIENTRY *fnglBindTexture)(GLenum target, GLuint texture);
extern void		(APIENTRY *fnglBlendFunc)(GLenum sfactor, GLenum dfactor);
extern void		(APIENTRY *fnglClear)(GLbitfield mask);
extern void		(APIENTRY *fnglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void		(APIENTRY *fnglClearDepth)(GLclampd depth);
extern void		(APIENTRY *fnglColor3f)(GLfloat red, GLfloat green, GLfloat blue);
extern void		(APIENTRY *fnglColor3fv)(const GLfloat *v);
extern void		(APIENTRY *fnglColor3ub)(GLubyte r, GLubyte g, GLubyte b);
extern void		(APIENTRY *fnglColor3ubv)(const GLubyte *v);
extern void		(APIENTRY *fnglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void		(APIENTRY *fnglColor4fv)(const GLfloat *v);
extern void		(APIENTRY *fnglCullFace)(GLenum mode);
extern void		(APIENTRY *fnglDeleteTextures)(GLsizei n, const GLuint * textures); 
extern void		(APIENTRY *fnglDepthFunc)(GLenum func);
extern void		(APIENTRY *fnglDisable)(GLenum cap);
extern void		(APIENTRY *fnglDrawPixels)(GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid* data);
extern void		(APIENTRY *fnglEnable)(GLenum cap);
extern void		(APIENTRY *fnglEnd)(void);
extern void		(APIENTRY *fnglFinish)(void);
extern void		(APIENTRY *fnglFlush)(void);
extern void		(APIENTRY *fnglFrontFace)(GLenum mode);
extern void		(APIENTRY *fnglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void		(APIENTRY *fnglGenTextures)(GLsizei n, GLuint* textures);
extern GLenum		(APIENTRY *fnglGetError)(void);
extern void		(APIENTRY *fnglGetFloatv)(GLenum pname, GLfloat *params);
extern void		(APIENTRY *fnglGetIntegerv)(GLenum pname, GLint *params);
extern const GLubyte * (APIENTRY *fnglGetString)(GLenum name);
extern void		(APIENTRY *fnglHint)(GLenum target, GLenum mode);
extern GLboolean	(APIENTRY *fnglIsEnabled)(GLenum cap);
extern void		(APIENTRY *fnglLoadIdentity)(void);
extern void		(APIENTRY *fnglLoadMatrixf)(const GLfloat *m);
extern void		(APIENTRY *fnglMatrixMode)(GLenum mode);
extern void		(APIENTRY *fnglMultMatrixf)(const GLfloat *m);
extern void		(APIENTRY *fnglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void		(APIENTRY *fnglPixelStoref)(GLenum pname, GLfloat param);
extern void		(APIENTRY *fnglPixelStorei)(GLenum pname, GLint param);
extern void		(APIENTRY *fnglPixelTransferf)(GLenum pname, GLfloat param);
extern void		(APIENTRY *fnglPolygonOffset)(GLfloat factor, GLfloat units);
extern void		(APIENTRY *fnglPopMatrix)(void);
extern void		(APIENTRY *fnglPushMatrix)(void);
extern void		(APIENTRY *fnglRasterPos2f)(GLfloat x, GLfloat y);
extern void		(APIENTRY *fnglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRY *fnglScalef)(GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRY *fnglShadeModel)(GLenum mode);
extern void		(APIENTRY *fnglTexCoord2f)(GLfloat s, GLfloat t);
extern void		(APIENTRY *fnglTexCoord2fv)(const GLfloat *v);
extern void		(APIENTRY *fnglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
extern void		(APIENTRY *fnglTexCoord3fv)(const GLfloat *v);
extern void		(APIENTRY *fnglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
extern void		(APIENTRY *fnglTexEnvi)(GLenum target, GLenum pname, GLint param);
extern void		(APIENTRY *fnglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void		(APIENTRY *fnglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
extern void		(APIENTRY *fnglTexParameteri)(GLenum target, GLenum pname, GLint param);
extern void		(APIENTRY *fnglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels); 
extern void		(APIENTRY *fnglTranslatef)(GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRY *fnglVertex2f)(GLfloat x, GLfloat y);
extern void		(APIENTRY *fnglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRY *fnglVertex3fv)(const GLfloat *v);
extern void		(APIENTRY *fnglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void		(APIENTRY *fnglVertex4fv)(const GLfloat* v);
extern void		(APIENTRY *fnglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);


inline void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) {
	(*fnglAlphaFunc)(func,ref);
}

inline void APIENTRY glBegin(GLenum mode) {
	(*fnglBegin)(mode);
}

inline void APIENTRY glBindTexture(GLenum target, GLuint texture) {
	(*fnglBindTexture)(target,texture);
}


inline void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor) {
	(*fnglBlendFunc)(sfactor,dfactor);
}


inline void APIENTRY glClear(GLbitfield mask) {
	(*fnglClear)(mask);
}


inline void APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	(*fnglClearColor)(red,green,blue,alpha);
}


inline void APIENTRY glClearDepth (GLclampd depth) {
	(*fnglClearDepth)(depth);
}


inline void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	(*fnglColor3f)(red,green,blue);
}


inline void APIENTRY glColor3fv(const GLfloat *v) {
	(*fnglColor3fv)(v);
}


inline void APIENTRY glColor3ub(const GLubyte r, const GLubyte g, const GLubyte b) {
	(*fnglColor3ub)(r, g, b);
}


inline void APIENTRY glColor3ubv(const GLubyte *v) {
	(*fnglColor3ubv)(v);
}


inline void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	(*fnglColor4f)(red,green,blue,alpha);
}


inline void APIENTRY glColor4fv(const GLfloat *v) {
	(*fnglColor4fv)(v);
}


inline void APIENTRY glCullFace(GLenum mode) {
	(*fnglCullFace)(mode);
}


inline void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures) {
	(*fnglDeleteTextures)(n, textures);
}


inline void APIENTRY glDepthFunc(GLenum func) {
	(*fnglDepthFunc)(func);
}


inline void APIENTRY glDisable(GLenum cap) {
	(*fnglDisable)(cap);
}


inline void APIENTRY glDrawPixels(GLsizei w, GLsizei h, GLenum format, GLenum type, const GLvoid* pixels) {
	(*fnglDrawPixels)(w, h, format, type, pixels);
}


inline void APIENTRY glEnable(GLenum cap) {
	(*fnglEnable)(cap);
}


inline void APIENTRY glEnd(void) {
	(*fnglEnd)();
}

inline void APIENTRY glFinish(void) {
	(*fnglFinish)();
}


inline void APIENTRY glFlush(void) {
	(*fnglFlush)();
}


inline void APIENTRY glFrontFace(GLenum mode) {
	(*fnglFrontFace)(mode);
}


inline void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	(*fnglFrustum)(left,right,bottom,top,zNear,zFar);
}


inline void APIENTRY glGenTextures(GLsizei n, GLuint* textures) {
	(*fnglGenTextures)(n, textures);
}


inline GLenum APIENTRY glGetError(void) {
	return (*fnglGetError)();
}


inline void APIENTRY glGetFloatv(GLenum pname, GLfloat *params) {
	(*fnglGetFloatv)(pname, params);
}


inline void APIENTRY glGetIntegerv(GLenum pname, GLint *params) {
	(*fnglGetIntegerv)(pname,params);
}


inline const GLubyte * APIENTRY glGetString(GLenum name) {
	return (*fnglGetString)(name);
}


inline void APIENTRY glHint(GLenum target, GLenum mode) {
	(*fnglHint)(target,mode);
}


inline GLboolean APIENTRY glIsEnabled(GLenum cap) {
	return (*fnglIsEnabled)(cap);
}


inline void APIENTRY glLoadIdentity(void) {
	(*fnglLoadIdentity)();
}


inline void APIENTRY glLoadMatrixf(const GLfloat *m) {
	(*fnglLoadMatrixf)(m);
}


inline void APIENTRY glMatrixMode(GLenum mode) {
	(*fnglMatrixMode)(mode);
}


inline void APIENTRY glMultMatrixf(const GLfloat *m) {
	(*fnglMultMatrixf)(m);
}


inline void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	(*fnglOrtho)(left,right,bottom,top,zNear,zFar);
}


inline void APIENTRY glPixelStoref (GLenum pname, GLfloat param) {
	(*fnglPixelStorei)(pname, param);
}


inline void APIENTRY glPixelStorei (GLenum pname, GLint param) {
	(*fnglPixelStorei)(pname, param);
}


inline void APIENTRY glPixelTransferf (GLenum pname, GLfloat param) {
	(*fnglPixelTransferf)(pname, param);
}


inline void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units) {
	(*fnglPolygonOffset)(factor,units);
}


inline void APIENTRY glPopMatrix(void) {
	(*fnglPopMatrix)();
}


inline void APIENTRY glPushMatrix(void) {
	(*fnglPushMatrix)();
}


inline void APIENTRY glRasterPos2f(GLfloat x, GLfloat y) {
	(*fnglRasterPos2f)(x,y);
}


inline void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	(*fnglRotatef)(angle,x,y,z);
}


inline void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglScalef)(x,y,z);
}


inline void APIENTRY glShadeModel (GLenum mode) {
	(*fnglShadeModel)(mode);
}


inline void APIENTRY glTexCoord2f(GLfloat s, GLfloat t) {
	(*fnglTexCoord2f)(s,t);
}


inline void APIENTRY glTexCoord2fv(const GLfloat *v) {
	(*fnglTexCoord2fv)(v);
}


inline void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {
	(*fnglTexCoord3f)(s,t,r);
}


inline void APIENTRY glTexCoord3fv(const GLfloat *v) {
	(*fnglTexCoord3fv)(v);
}


inline void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
	(*fnglTexEnvf)(target,pname,param);
}


inline void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param) {
	(*fnglTexEnvi)(target,pname,param);
}


inline void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexImage2D)(target,level,internalformat,width,height,border,format,type,pixels);
}


inline void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
	(*fnglTexParameterf)(target,pname,param);
}


inline void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param) {
	(*fnglTexParameteri)(target,pname,param);
}


inline void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
	(*fnglTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels);
}


inline void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglTranslatef)(x,y,z);
}


inline void APIENTRY glVertex2f(GLfloat x, GLfloat y) {
	(*fnglVertex2f)(x,y);
}


inline void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	(*fnglVertex3f)(x,y,z);
}


inline void APIENTRY glVertex3fv(const GLfloat *v) {
	(*fnglVertex3fv)(v);
}


inline void APIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	(*fnglVertex4f)(x,y,z,w);
}


inline void APIENTRY glVertex4fv(const GLfloat* v) {
	(*fnglVertex4fv)(v);
}


inline void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	(*fnglViewport)(x,y,width,height);
}



#endif // OGL_INLINE


#ifndef _INC_WINDOWS
#undef OGL_HPP_FAKEWIN32_DEFS
#undef APIENTRY
#undef CALLBACK
#endif

#if defined(OGL_HPP_FAKEWIN32_DEF)
#undef WIN32
#endif


/* restore normal meaning of WINGDIAPI */
#undef WINGDIAPI
#define WINGDIAPI __declspec(dllimport)


#endif // not LINUX


#endif	// OGL_HPP
