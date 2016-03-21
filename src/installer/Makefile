# Makefile for self-executable installer packages.

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
include $(TOP)/config.mak


# Path to the makensis program.
MAKENSIS = "c:/Program Files/NSIS/makensis.exe"

# Path to a strip program that works on Windows .exe's.
#WIN_STRIP = "c:/MinGW/bin/strip.exe"
WIN_STRIP = strip


#
# Windows versions.
#

soulride_setup : stripped_exe copy_public_files
	perl gen_install.pl $(@)_$(VERSION_TAG).exe \
		Soul_Ride "http://www.soulride.com/products/soulride.html" "Soul Ride" \
		../../../soulride-private/ soulride.lst | $(MAKENSIS) -

soulride_setup_purplehills : stripped_exe copy_public_files
	perl gen_install.pl $(@)_$(VERSION_TAG).exe \
		Soul_Ride "http://www.soulride.com/products/soulride.html" "Soul Ride" \
		../../../soulride-private/ soulride.lst Purplehills | $(MAKENSIS) -

virtual_breckenridge : stripped_exe copy_public_files
	perl gen_install.pl $(@)_$(VERSION_TAG).exe \
		Breckenridge "http://www.breckenridge.com" "Virtual Breckenridge" \
		../../../soulride-private/ breckenridge.lst | $(MAKENSIS) -

virtual_stratton : stripped_exe copy_public_files
	perl gen_install.pl $(@)_$(VERSION_TAG).exe \
		Stratton "http://www.stratton.com" "Virtual Stratton" \
		../../../soulride-private/ stratton.lst | $(MAKENSIS) -

virtual_jay_peak : stripped_exe copy_public_files
	perl gen_install.pl $(@)_$(VERSION_TAG).exe \
		Jay_Peak "http://www.jaypeakresort.com" "Virtual Jay Peak" \
		../../../soulride-private/ jay_peak.lst | $(MAKENSIS) -


# Make sure we're packing a stripped version of the Release build of
# soulride.exe
stripped_exe:
	cp ../Release/soulride.exe ../../../soulride-private
	$(WIN_STRIP) ../../../soulride-private/soulride.exe

# Make sure to use most up-to-date misc files.
copy_public_files:
	cp ../../cdaindex.txt ../../../soulride-private/cdaindex.txt
	cp ../../readme.txt ../../../soulride-private/readme.txt


#
# Linux versions.
#

# soulride_setup : ...

virtual_breckenridge_linux :
	perl gen_linux_package.pl $(@)_$(VERSION_TAG).tar.gz Breckenridge breckenridge.lst

virtual_stratton_linux :
	perl gen_linux_package.pl $(@)_$(VERSION_TAG).tar.gz Stratton stratton.lst

virtual_jay_peak_linux :
	perl gen_linux_package.pl $(@)_$(VERSION_TAG).tar.gz Jay_Peak jay_peak.lst
