# Product makefile for Soul Ride.


ifeq "$(debug)" "1"
OBJ_DIR = Debug
else
OBJ_DIR = Release
endif


# Default target just recompiles and copies the executable etc to
# project dir.
all:
	$(MAKE) -C src $@
	cp src/$(OBJ_DIR)/soulride$(EXE_EXT) .
	cp src/$(OBJ_DIR)/soulride$(EXE_EXT) ../soulride-private
	cp readme.txt ../soulride-private
	cp readme-*.txt ../soulride-private
	cp cdaindex.txt ../soulride-private

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
