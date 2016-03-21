# makefile for lua distribution (includes)

# modified by Thatcher Ulrich <tu@tulrich.com> 7/24/2000 to compile under Win32
# modified by Thatcher Ulrich 4/2003 to compile under Win32 with GNU make

LUA= ..

include $(LUA)/config

SRCS= lua.h lualib.h luadebug.h lauxlib.h

all:

clean:

co:
	co -q -f -M $(SRCS)

klean:	clean
	rm -f $(SRCS)
