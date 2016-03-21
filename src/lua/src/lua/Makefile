# makefile for lua interpreter

# modified by Thatcher Ulrich <tu@tulrich.com> 7/24/2000 to compile under Win32
# modified again 4/2003 to work with GNU make

TOP = ../../..
include $(TOP)/config.mak

LUA= ../..

include $(LUA)/config

EXTRA_DEFS= $(POSIX)
CFLAGS += $(EXTRA_DEFS)

OBJS= $(OBJ_DIR)/lua.$(OBJ_EXT)
SRCS= lua.c

T= $(BIN)/$(OBJ_DIR)/lua$(EXE_EXT)

all: $T

$T: makedirs $(OBJS) $(LUALIB)/$(OBJ_DIR)/lua.$(LIB_EXT) $(LUALIB)/$(OBJ_DIR)/lualib.$(LIB_EXT)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LUALIB)/$(OBJ_DIR)/lua.lib $(LUALIB)/$(OBJ_DIR)/lualib.lib

makedirs:
	-mkdir -p $(OBJ_DIR)
	-mkdir -p $(BIN)/$(OBJ_DIR)

$(LUALIB)/$(OBJ_DIR)/lua.$(LIB_EXT):
	cd ..
	$(MAKE)
	cd lua

$(LUALIB)/$(OBJ_DIR)/lualib.$(LIB_EXT):
	cd ../lib
	$(MAKE)
	cd ../lua

clean:
	rm -f $(OBJS) $T
	rm -rf $(OBJ_DIR)

co:
	co -q -f -M $(SRCS)

klean:	clean
	rm -f $(SRCS)
