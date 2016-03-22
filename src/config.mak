#    Copyright 2003 Thatcher Ulrich
#
#    This file is part of The Soul Ride Engine, see http://soulride.com
#
#    The Soul Ride Engine is free software; you can redistribute it
#    and/or modify it under the terms of the GNU General Public License
#    as published by the Free Software Foundation; either version 2 of
#    the License, or (at your option) any later version.
#
#    The Soul Ride Engine is distributed in the hope that it will be
#    useful, but WITHOUT ANY WARRANTY; without even the implied
#    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with The Soul Ride Engine; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# make configuration for soulride (adapted from tu-testbed/config)


# Generic options
VERSION_TAG = 1_5
VERSION_STRING = \"1.5\"


# If you haven't set SOULRIDE_COMPILER, then take a guess at its value
# based on the OSTYPE env variable.
#
# If you want to use a particular compiler; for example gcc-3.0, then
# set the environment variable SOULRIDE_COMPILER to the compiler
# name (or pass it on the make command line, like "make SOULRIDE_COMPILER=gcc")
ifndef SOULRIDE_COMPILER
	ifndef $(OSTYPE)
		OSTYPE = $(shell uname)
	endif
	OSTYPE := $(patsubst Linux, linux, $(OSTYPE))

	ifneq (,$(findstring linux, $(OSTYPE)))
		SOULRIDE_COMPILER = gcc
	else
		SOULRIDE_COMPILER = msvc
	endif
endif


ifeq ($(SOULRIDE_COMPILER), msvc)

#
# MSVC/Windows options
#


# Uncomment this option if you're saddled with an old version of
# MSVCRT.  Newer versions of MSVCRT's malloc perform about the same as
# dlmalloc.
#USE_DL_MALLOC_FLAG := -DUSE_DL_MALLOC

# MSVC
PLATFORM_WINDOWS := 1
USE_SDL := 0
CC := cl /nologo
AR := lib /NOLOGO
#LINK := link /nologo
CC_OUT_FLAG := /Fo
LIB_OUT_FLAG := /OUT:
LIB_EXT := lib
LIB_PRE :=
OBJ_EXT := obj
EXE_EXT := .exe
DLL_EXT := dll
SDL_DIR := $(TOP)/../SDL-1.2.5
SDL_INCLUDE := -I "$(SDL_DIR)/include"
SDL_LIBS := winmm.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
ifeq "$(USE_SDL)" "1"
MEDIA_LIBS := $(SDL_DIR)/SDL.lib
else
MEDIA_LIBS := dinput.lib dsound.lib
endif
LIBS := $(MEDIA_LIBS) gdi32.lib oldnames.lib $(SDL_LIBS)

ifeq ($(DEBUG),1)
	# msvc debug flags
	CC_DEBUG_FLAGS := -Zi -Od
	LDFLAGS := -Zi /link -DEBUG /INCREMENTAL:NO /NODEFAULTLIB:libc.lib libcmtd.lib
	MAKE_DLL := cl /nologo /LDd
else
	# msvc non-debug flags
	CC_DEBUG_FLAGS := -Ox -DNDEBUG=1
	LDFLAGS := /link -DEBUG -DEBUGTYPE:BOTH /INCREMENTAL:NO /FIXED:NO /PDB:NONE /RELEASE /NODEFAULTLIB:libc.lib libcmt.lib oldnames.lib
	MAKE_DLL := cl /nologo /LD
endif

CFLAGS := $(CFLAGS) $(CC_DEBUG_FLAGS) /MT -GR -GX -DWIN32=1 $(USE_DL_MALLOC_FLAG) -W3 $(SDL_INCLUDE) -DVERSION_STRING=$(VERSION_STRING)

LIBPNG_DIR := $(TOP)/../libpng
LIBPNG_INCLUDE := -I$(LIBPNG_DIR)
LIBPNG := $(LIBPNG_DIR)/libpng.$(LIB_EXT)

ZLIB_DIR := $(TOP)/../zlib
ZLIB_INCLUDE := -I$(ZLIB_DIR)
ZLIB := $(ZLIB_DIR)/zlib.$(LIB_EXT)

JPEGLIB_DIR := $(TOP)/../jpeg-6b
JPEGLIB_INCLUDE := -I$(JPEGLIB_DIR)
JPEGLIB := $(JPEGLIB_DIR)/libjpeg.$(LIB_EXT)


else

# GCC
USE_SDL := 1
CC := $(SOULRIDE_COMPILER)
AR := ar -r
CC_OUT_FLAG := -o
LIB_OUT_FLAG :=
LIB_PRE := lib
LIB_EXT := a
OBJ_EXT := o
EXE_EXT :=
DLL_EXT := so
LIBS := -lSDL_mixer -lSDL -lGL -lm -lpthread -lstdc++
DLL_LIBS := -ldl
SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs)

ifeq ($(DEBUG), 1)
	# gcc debug flags
	CC_DEBUG_FLAGS := -g
	LDFLAGS := -g
else
	# gcc non-debug flags
	CC_DEBUG_FLAGS := -O3 -DNDEBUG=1 -ffast-math -fexpensive-optimizations -fomit-frame-pointer
endif

CFLAGS := $(CFLAGS) $(CC_DEBUG_FLAGS) $(SDL_CFLAGS) -fpic -Wall -fexceptions -DLINUX=1 -DSDL=1 -DVERSION_STRING=$(VERSION_STRING)
LDFLAGS := -lGL # -lGLU
MAKE_DLL := gcc -shared

# On Unix-like machines, these libraries are usually installed in
# standard locations.

#LIBPNG_DIR := $(TOP)/../libpng
#LIBPNG_INCLUDE := $(LIBPNG_DIR)
LIBPNG := -lpng

#ZLIB_DIR := $(TOP)/../zlib
ZLIB_INCLUDE :=
ZLIB := -lz

#JPEGLIB_DIR := $(TOP)/../jpeg-6b
JPEGLIB_INCLUDE :=
JPEGLIB := -ljpeg


endif # GCC


# %.$(OBJ_EXT): %.cpp
# 	$(CC) -c $< $(CFLAGS)

# %.$(OBJ_EXT): %.c
# 	$(CC) -c $< $(CFLAGS)

ENGINE := $(TOP)/engine/engine.$(LIB_EXT)
ENGINE_INCLUDE := -I$(TOP)


ifeq "$(DEBUG)" "1"
OBJ_DIR=Debug
else
OBJ_DIR=Release
endif

$(OBJ_DIR)/%.$(OBJ_EXT): %.cpp
	$(CC) -c $< $(CFLAGS) $(CC_OUT_FLAG)$@

$(OBJ_DIR)/%.$(OBJ_EXT): %.c
	$(CC) -c $< $(CFLAGS) $(CC_OUT_FLAG)$@


# Local Variables:
# mode: Makefile
# End:
