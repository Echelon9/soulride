# makefile for lua compiler

# modified by Thatcher Ulrich <tu@tulrich.com> 7/24/2000 to compile under Win32
# modified again to work w/ GNU make

TOP = ../../..
include $(TOP)/config.mak

LUA= ../..
include $(LUA)/config

INCS= -I$(INC) $(EXTRA_INCS) -I..
CFLAGS += $(DEBUGFLAGS) $(WARN) $(INCS) $(DEFS)

OBJS= \
	$(OBJ_DIR)/dump.$(OBJ_EXT) \
	$(OBJ_DIR)/luac.$(OBJ_EXT) \
	$(OBJ_DIR)/opcode.$(OBJ_EXT) \
	$(OBJ_DIR)/opt.$(OBJ_EXT) \
	$(OBJ_DIR)/print.$(OBJ_EXT) \
	$(OBJ_DIR)/stubs.$(OBJ_EXT) \
	$(OBJ_DIR)/test.$(OBJ_EXT)

SRCS= dump.c luac.c opcode.c opt.c print.c stubs.c test.c luac.h opcode.h

T= $(BIN)/$(OBJ_DIR)/luac$(EXE_EXT)

all: $T

$T: makedirs $(OBJS) $(LUALIB)/$(OBJ_DIR)/lua.$(LIB_EXT)
	$(CC) -o $@ $(OBJS) $(LUALIB)/$(OBJ_DIR)/lua.$(LIB_EXT) $(LDFLAGS)

makedirs:
	-mkdir -p $(OBJ_DIR)
	-mkdir -p $(BIN)/$(OBJ_DIR)

$(LUALIB)/$(OBJ_DIR)/lua.$(LIB_EXT):
	cd ..; $(MAKE)

clean:
	rm -f $(OBJS) $T
	rm -rf $(OBJ_DIR)

co:
	co -q -f -M $(SRCS)

klean:	clean
	rm -f $(SRCS)
