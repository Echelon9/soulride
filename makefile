# Product makefile for Soul Ride.

CPPSOURCES=$(wildcard src/*.cpp src/*.hpp src/gamegui/*.cpp src/gamegui/*.hpp)

ifeq "$(debug)" "1"
OBJ_DIR = Debug
else
OBJ_DIR = Release
endif

UNAME_S := $(shell uname -s)

## all         : (default) compiles and copies the executable to project dir
all:
ifeq ($(UNAME_S),Darwin)
	cd src/ && $(MAKE) -f Makefile.macosx macosx_shared
else
	$(MAKE) -C src $@
endif

## clean       : remove build artifacts
clean:
	$(MAKE) -C src $@
	cd src/test && $(MAKE) clean || true

PRODUCTS = \
	soulride_setup \
	virtual_breckenridge \
	virtual_breckenridge_linux \
	virtual_stratton \
	virtual_stratton_linux \
	virtual_jay_peak \
	virtual_jay_peak_linux

$(PRODUCTS): all
	$(MAKE) -C src/installer $@

## test        : run the test suite
test:
	cd src/test && $(MAKE) test

## help        : print this help message and exit
help: makefile
	@sed -n 's/^##//p' $<

## installdeps : install dependencies on Debian-based system
installdeps:
	apt-get install build-essential libsdl2-dev libsdl2-mixer-dev

## doxygen     : generate documentation of the C++ code
doxygen: docs/doxygen/html/index.html

docs/doxygen/html/index.html: $(CPPSOURCES)
	doxygen

## cppcheck    : run static analysis on C++ source code
cppcheck: $(CPPSOURCES)
	cppcheck $(CPPSOURCES) --enable=all --platform=unix64 \
	--std=c++11 --inline-suppr --quiet --force \
	$(addprefix -I,$(INCLUDE_DIRS)) \
	-I/usr/include -I/usr/include/linux
