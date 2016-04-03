# Soul Ride #

[![Build Status](https://travis-ci.org/Echelon9/soulride.svg?branch=master)](https://travis-ci.org/Echelon9/soulride)

Welcome to **Soul Ride**, a GPL physics-based snowboarding simulator, with an
advanced terrain engine designed for rendering real-world locations!

Originally released as a proprietary game by Slingshot Game Technology, Inc.
the engine source code has since been relicensed under the GPLv2.

## About ##

Soul Ride is a single-player PC snowboarding game which emphasizes
purity, realism and extreme backcountry terrain.  The runs are drawn
from geological survey data of real-world mountains.  The action is
based on a meticulous simulation of snowboarding physics.  The
gameplay is straight out of the old-school arcade.  And every move
the player makes is recorded on a virtual VCR.

## Platform ##

Soul Ride has been ported to a variety of platforms:

  * Linux (current primary development platform)
  * OS X
  * Windows

## Development ##

### Linux ###

On Debian-based distributions, install all necessary dependencies with:
```
sudo make installdeps
```
Building from source:
```
make
make DEBUG=1 # for debug build
```

### OS X ###

TBD

### Windows ###

TBD

## Unit Tests ##

Unit tests can be run by:
```
make test
```
Further unit test coverage would be appreciated!

## Further Information ##

[readme.txt](readme.txt)
[readme-linux.txt](readme-linux.txt)
[readme-macosx.txt](readme-macosx.txt)

## License ##

Copyright 2003 Slingshot Game Technology, Inc.

The source code is made available to you under the terms of the GNU General
Public License version 2 as published by the Free Software Foundation. For more
information, see [COPYING](COPYING).

The game assets (maps) are not.
