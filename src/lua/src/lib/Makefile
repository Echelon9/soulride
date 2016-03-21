# makefile for lua standard library

# modified by Thatcher Ulrich <tu@tulrich.com> 7/24/2000 to compile under Win32
# modified again 4/2003 to work with GNU make

TOP = ../../..
include $(TOP)/config.mak

LUA= ../..

include $(LUA)/config

# actually only used in liolib.c
EXTRA_DEFS= $(POPEN)
CFLAGS += $(EXTRA_DEFS)

OBJS= \
	$(OBJ_DIR)/linit.$(OBJ_EXT) \
	$(OBJ_DIR)/ldblib.$(OBJ_EXT) \
	$(OBJ_DIR)/liolib.$(OBJ_EXT) \
	$(OBJ_DIR)/lmathlib.$(OBJ_EXT) \
	$(OBJ_DIR)/lstrlib.$(OBJ_EXT)

SRCS= linit.c ldblib.c liolib.c lmathlib.c lstrlib.c

T= $(LUALIB)/$(OBJ_DIR)/lualib.$(LIB_EXT)

all: $T

$T: makedirs $(OBJS)
	$(AR) $(LIB_OUT_FLAG)$@ $(OBJS)
#	$(RANLIB) $@

makedirs:
	-mkdir -p $(OBJ_DIR)
	-mkdir -p $(LUALIB)/$(OBJ_DIR)

clean:
	rm -f $(OBJS) $T
	rm -rf $(OBJ_DIR)

co:
	co -q -f -M $(SRCS)

klean:	clean
	rm -f $(SRCS)
