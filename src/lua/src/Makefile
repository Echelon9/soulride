# makefile for lua basic library

# modified by Thatcher Ulrich <tu@tulrich.com> 7/24/2000 to compile under Win32

TOP = ../..
include $(TOP)/config.mak

LUA= ..

include $(LUA)/config

OBJS=	\
	$(OBJ_DIR)/lapi.$(OBJ_EXT) \
	$(OBJ_DIR)/lauxlib.$(OBJ_EXT) \
	$(OBJ_DIR)/lbuffer.$(OBJ_EXT) \
	$(OBJ_DIR)/lbuiltin.$(OBJ_EXT) \
	$(OBJ_DIR)/ldo.$(OBJ_EXT) \
	$(OBJ_DIR)/lfunc.$(OBJ_EXT) \
	$(OBJ_DIR)/lgc.$(OBJ_EXT) \
	$(OBJ_DIR)/llex.$(OBJ_EXT) \
	$(OBJ_DIR)/lmem.$(OBJ_EXT) \
	$(OBJ_DIR)/lobject.$(OBJ_EXT) \
	$(OBJ_DIR)/lparser.$(OBJ_EXT) \
	$(OBJ_DIR)/lstate.$(OBJ_EXT) \
	$(OBJ_DIR)/lstring.$(OBJ_EXT) \
	$(OBJ_DIR)/ltable.$(OBJ_EXT) \
	$(OBJ_DIR)/ltm.$(OBJ_EXT) \
	$(OBJ_DIR)/lundump.$(OBJ_EXT) \
	$(OBJ_DIR)/lvm.$(OBJ_EXT) \
	$(OBJ_DIR)/lzio.$(OBJ_EXT)

SRCS=	\
	lapi.c \
	lauxlib.c \
	lbuffer.c \
	lbuiltin.c \
	ldo.c \
	lfunc.c \
	lgc.c \
	llex.c \
	lmem.c \
	lobject.c \
	lparser.c \
	lstate.c \
	lstring.c \
	ltable.c \
	ltm.c \
	lundump.c \
	lvm.c \
	lzio.c \
	lapi.h \
	lbuiltin.h \
	ldo.h \
	lfunc.h \
	lgc.h \
	llex.h \
	lmem.h \
	lobject.h \
	lopcodes.h \
	lparser.h \
	lstate.h \
	lstring.h \
	ltable.h \
	ltm.h \
	lundump.h \
	lvm.h \
	lzio.h

T= $(LUALIB)/$(OBJ_DIR)/lua.$(LIB_EXT)

all: $T

$T: makedir $(OBJS)
	-mkdir -p ../lib/$(OBJ_DIR)
	$(AR) $(LIB_OUT_FLAG)$@ $(OBJS)
#	$(RANLIB) $@

makedir:
	-mkdir -p $(OBJ_DIR)
	-mkdir -p $(LUALIB)/$(OBJ_DIR)

clean:
	rm -f $(OBJS) $T
	rm -rf $(OBJ_DIR)

co:
	co -q -f -M $(SRCS)

klean:	clean
	rm -f $(SRCS)
