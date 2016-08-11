#    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.
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
#    along with Foobar; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Soul Ride unified (multi-platform) makefile.

TOP = .

# Set compiler, various include dirs, options, etc.
include $(TOP)/config.mak




# If we're using SDL, and static=1, then link differently.
ifeq "$(USE_SDL)" "1"
ifeq "$(static)" "1"
proj=$(OBJ_DIR)/soulride-static$(EXE_EXT)
else
proj=$(OBJ_DIR)/soulride$(EXE_EXT)
endif
else
proj=$(OBJ_DIR)/soulride$(EXE_EXT)
endif


WINDOWS_OBJ_LIST =				\
	$(OBJ_DIR)/winmain.$(OBJ_EXT)		\
	$(OBJ_DIR)/exceptionhandler.$(OBJ_EXT)	\
	$(OBJ_DIR)/crashtest.$(OBJ_EXT)		\
	$(OBJ_DIR)/winsound.$(OBJ_EXT)

LINUX_OBJ_LIST =				\
	$(OBJ_DIR)/linuxmain.$(OBJ_EXT)		\
	$(OBJ_DIR)/linuxsound.$(OBJ_EXT)

GENERIC_OBJ_LIST =				\
	$(OBJ_DIR)/oglrender.$(OBJ_EXT)		\
	$(OBJ_DIR)/ogl.$(OBJ_EXT)		\
	$(OBJ_DIR)/timer.$(OBJ_EXT)		\
	$(OBJ_DIR)/input.$(OBJ_EXT)		\
	$(OBJ_DIR)/psdread.$(OBJ_EXT)		\
	$(OBJ_DIR)/avi_tools.$(OBJ_EXT)		\
						\
	$(OBJ_DIR)/attractmode.$(OBJ_EXT)	\
	$(OBJ_DIR)/boarder.$(OBJ_EXT)		\
	$(OBJ_DIR)/boarderctl.$(OBJ_EXT)	\
	$(OBJ_DIR)/camera.$(OBJ_EXT)		\
	$(OBJ_DIR)/clip.$(OBJ_EXT)		\
	$(OBJ_DIR)/collide.$(OBJ_EXT)		\
	$(OBJ_DIR)/config.$(OBJ_EXT)		\
	$(OBJ_DIR)/console.$(OBJ_EXT)		\
	$(OBJ_DIR)/finish.$(OBJ_EXT)		\
	$(OBJ_DIR)/game.$(OBJ_EXT)		\
	$(OBJ_DIR)/gameloop.$(OBJ_EXT)		\
	$(OBJ_DIR)/geometry.$(OBJ_EXT)		\
	$(OBJ_DIR)/gstamp.$(OBJ_EXT)		\
	$(OBJ_DIR)/highscore.$(OBJ_EXT)		\
	$(OBJ_DIR)/imageoverlay.$(OBJ_EXT)	\
	$(OBJ_DIR)/lift.$(OBJ_EXT)		\
	$(OBJ_DIR)/main.$(OBJ_EXT)		\
	$(OBJ_DIR)/mdynamic.$(OBJ_EXT)		\
	$(OBJ_DIR)/mesh.$(OBJ_EXT)		\
	$(OBJ_DIR)/model.$(OBJ_EXT)		\
	$(OBJ_DIR)/multiplayer.$(OBJ_EXT)	\
	$(OBJ_DIR)/music.$(OBJ_EXT)		\
	$(OBJ_DIR)/orientedstatic.$(OBJ_EXT)	\
	$(OBJ_DIR)/overlay.$(OBJ_EXT)		\
	$(OBJ_DIR)/particle.$(OBJ_EXT)		\
	$(OBJ_DIR)/path.$(OBJ_EXT)		\
	$(OBJ_DIR)/perftest.$(OBJ_EXT)		\
	$(OBJ_DIR)/player.$(OBJ_EXT)		\
	$(OBJ_DIR)/polygonregion.$(OBJ_EXT)	\
	$(OBJ_DIR)/recording.$(OBJ_EXT)		\
	$(OBJ_DIR)/sbox.$(OBJ_EXT)		\
	$(OBJ_DIR)/screenshot.$(OBJ_EXT)	\
	$(OBJ_DIR)/scylinder.$(OBJ_EXT)		\
	$(OBJ_DIR)/staticlod.$(OBJ_EXT)		\
	$(OBJ_DIR)/stickman.$(OBJ_EXT)		\
	$(OBJ_DIR)/surface.$(OBJ_EXT)		\
	$(OBJ_DIR)/surfaceshader.$(OBJ_EXT)	\
	$(OBJ_DIR)/terrainmesh.$(OBJ_EXT)	\
	$(OBJ_DIR)/terrainmodel.$(OBJ_EXT)	\
	$(OBJ_DIR)/text.$(OBJ_EXT)		\
	$(OBJ_DIR)/tree.$(OBJ_EXT)		\
	$(OBJ_DIR)/ui.$(OBJ_EXT)		\
	$(OBJ_DIR)/uihelidrop.$(OBJ_EXT)	\
	$(OBJ_DIR)/uimainmenu.$(OBJ_EXT)	\
	$(OBJ_DIR)/uioptions.$(OBJ_EXT)		\
	$(OBJ_DIR)/uiplayback.$(OBJ_EXT)	\
	$(OBJ_DIR)/uiplayer.$(OBJ_EXT)		\
	$(OBJ_DIR)/usercamera.$(OBJ_EXT)	\
	$(OBJ_DIR)/utility.$(OBJ_EXT)		\
	$(OBJ_DIR)/view.$(OBJ_EXT)		\
	$(OBJ_DIR)/weather.$(OBJ_EXT)

ifeq "$(PLATFORM_WINDOWS)" "1"
OBJ_LIST = $(WINDOWS_OBJ_LIST) $(GENERIC_OBJ_LIST)
else
OBJ_LIST = $(LINUX_OBJ_LIST) $(GENERIC_OBJ_LIST)
endif


all: mkdirs $(proj)


#.cpp.o :
#	$(cc) $(cdebug) $(cflags) -o $*.o $(version_switches) $*.cpp


gamegui = gamegui/$(OBJ_DIR)/gamegui.$(LIB_EXT)
$(gamegui): FORCE
	$(MAKE) -C gamegui


lua = lua/lib/$(OBJ_DIR)/lua.$(LIB_EXT)
$(lua): FORCE
	$(MAKE) -C lua/src

lualib = lua/lib/$(OBJ_DIR)/lualib.$(LIB_EXT)
$(lualib): FORCE
	$(MAKE) -C lua/src/lib


ifeq "$(SOULRIDE_COMPILER)" "gcc"
ifeq "$USE_SDL" "1"
ifeq "$(static)" "1"
stdlibs := /usr/lib/libstdc++-libc6.2-2.a.3
SDL_MIXER_LIBS := /usr/lib/libSDL_mixer.a /usr/lib/libsmpeg.a /usr/lib/libvorbisfile.a /usr/lib/libvorbis.a /usr/lib/libogg.a
SDL_LDFLAGS := $(SDL_MIXER_LIBS) $(shell sdl-config --static-libs)
SDL_LDFLAGS := $(patsubst -lSDL, /usr/lib/libSDL.a, $(SDL_LDFLAGS))
else
stdlibs := -lstdc++
SDL_LDFLAGS := -lSDL_mixer $(shell sdl-config --libs)
endif
endif
endif

ifneq "$USE_SDL" "1"
SDL_LDFLAGS := 
endif


ifeq "$(PLATFORM_WINDOWS)" "1"
PROJ_RES = $(OBJ_DIR)/soulride.res
# possible locations for MFC includes
MFC_INCLUDE = -i "c:\Program Files\Microsoft Visual Studio\VC98\MFC\Include" \
	-i "c:\Program Files\Microsoft Visual Studio .NET 2003\vc7\atlmfc\include"
$(PROJ_RES): soulride.rc
	rc $(MFC_INCLUDE) -r -fo $@ soulride.rc
endif


$(proj): $(OBJ_LIST) $(PROJ_RES) $(lua) $(lualib) $(gamegui)
	mkdir -p $(OBJ_DIR)
	$(CC) -o $(proj) \
		$(OBJ_LIST) \
		$(LDFLAGS) $(PROJ_RES) $(gamegui) $(lua) $(lualib) \
		$(LIBS)	\
		$(SDL_LDFLAGS)
	cp $(proj) ..

mkdirs:
	mkdir -p $(OBJ_DIR)

clean:
	-rm $(OBJ_DIR)/*.$(OBJ_EXT) $(proj)
	-rm $(OBJ_DIR)/*.map
	-rm $(OBJ_DIR)/*.res
	$(MAKE) -C gamegui $@
	$(MAKE) -C lua $@


FORCE:
