# makefile for lua hierarchy

# modified by Thatcher Ulrich <tu@tulrich.com> 7/24/2000 to compile under Win32
# modified by Thatcher Ulrich 4/2003 to compile under Win32 with GNU make

all:
	-mkdir bin
	$(MAKE) -C include $@
	$(MAKE) -C src $@
	$(MAKE) -C src/luac $@
	$(MAKE) -C src/lib $@
	$(MAKE) -C src/lua $@

clean:
	$(MAKE) -C include $@
	$(MAKE) -C src $@
	$(MAKE) -C src/luac $@
	$(MAKE) -C src/lib $@
	$(MAKE) -C src/lua $@

strip:
	strip bin/lua*
