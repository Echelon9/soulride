# GameGUI makefile.	-thatcher 2/17/2000 Copyright Slingshot Game Technology.

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
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


TOP = ..

# Set compiler, various include dirs, options, etc.
include $(TOP)/config.mak

ifeq "$(DEBUG)" "1"
OBJ_DIR=Debug
else
OBJ_DIR=Release
endif

$(OBJ_DIR)/%.$(OBJ_EXT): %.cpp
	$(CC) -c $< $(CFLAGS) $(CC_OUT_FLAG)$@

$(OBJ_DIR)/%.$(OBJ_EXT): %.c
	$(CC) -c $< $(CFLAGS) $(CC_OUT_FLAG)$@


proj=$(OBJ_DIR)/gamegui.$(LIB_EXT)

OBJ_LIST = \
	$(OBJ_DIR)/gamegui.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_actor.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_audio.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_event.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_file.$(OBJ_EXT)		\
	$(OBJ_DIR)/gg_font_impl.$(OBJ_EXT)		\
	$(OBJ_DIR)/gg_log.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_movie_impl.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_player_impl.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_psdread.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_resource.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_string.$(OBJ_EXT)	\
	$(OBJ_DIR)/gg_script.$(OBJ_EXT)	\


VECEDIT_OBJ_LIST = \
	$(OBJ_DIR)/vecedit_winmain.$(OBJ_EXT)	\
	$(OBJ_DIR)/veceditor.$(OBJ_EXT)	\
	veceditor.res


all: mkdirs $(proj)




$(OBJ_DIR)/ggplayer$(EXE_EXT): $(proj) $(OBJ_DIR)/winmain.$(OBJ_EXT)
	$(CC) $(LDFLAGS) -o $(OBJ_DIR)/ggplayer$(EXE_EXT) \
		$(OBJ_DIR)/winmain.$(OBJ_EXT) $(proj) \
		opengl32.lib winmm.lib dsound.lib dxguid.lib ole32.lib

$(OBJ_DIR)/veceditor$(EXE_EXT): $(VECEDIT_OBJ_LIST) $(proj)
	$(CC) \
		$(VECEDIT_OBJ_LIST) $(proj) \
		$(LDFLAGS) -out:$(OBJ_DIR)/veceditor$(EXE_EXT) \
		$(LIBS) opengl32.lib winmm.lib dsound.lib ole32.lib

$(OBJ_DIR)/fonttest$(EXE_EXT): $(proj) $(OBJ_DIR)/winmain_fonttest.$(OBJ_EXT)
	$(CC) \
		$(OBJ_DIR)/winmain_fonttest.$(OBJ_EXT) $(proj) \
		$(guiflags) $(LDFLAGS) -out:$(OBJ_DIR)/fonttest$(EXE_EXT) -map:$(OBJ_DIR)/fonttest.map \
		$(LIBS) opengl32.lib winmm.lib dsound.lib dxguid.lib ole32.lib


$(proj): $(OBJ_LIST)
	$(AR) $(LIB_OUT_FLAG)$(proj) $(OBJ_LIST)


mkdirs:
	mkdir -p $(OBJ_DIR)

# Rules for cleaning out those old files
clean:
	rm -rf Release
	rm -rf Debug

