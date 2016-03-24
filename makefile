# Product makefile for Soul Ride.

CPPSOURCES=$(wildcard src/*.cpp src/*.hpp src/gamegui/*.cpp src/gamegui/*.hpp)

ifeq "$(debug)" "1"
OBJ_DIR = Debug
else
OBJ_DIR = Release
endif


# Default target just recompiles and copies the executable etc to
# project dir.
all:
	$(MAKE) -C src $@

clean:
	$(MAKE) -C src $@

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

installdeps:
	apt-get install build-essential libsdl1.2-dev libsdl-mixer1.2-dev

# Generate documentation of the C++ code
doxygen: docs/doxygen/html/index.html

docs/doxygen/html/index.html: $(CPPSOURCES)
	doxygen
