Virtual Stratton
================

v1.1 16 Oct 2001

by Slingshot Game Technology, Inc.

Copyright 2001 All rights reserved.

http://www.soulride.com


About
-----

Virtual Stratton is a simulation of snowboarding at the Stratton
Mountain resort in Vermont.  The terrain is based on geological
survey data of the mountain and surrounding area, with the addition
of signature trails and buildings.  The software is based on the
extreme backcountry snowboarding game Soul Ride.

When you're done exploring our meticulous recreation of Stratton
Mountain, browse to http://www.stratton.com to see about visiting the
real thing!


Platform Requirements
---------------------

* Intel Pentium II 300 or faster
* Windows 9x, ME, NT4, 2000
* 3D hardware accelerator with OpenGL 1.1 driver (successfully tested
  with 3dfx Voodoo series, Intel i810, NVIDIA RIVA 128, RIVA TNT,
  GeForce 256, ATI Rage 128, ATI Radeon)
* 64 MB RAM
* 20 MB disk space
* Joystick, gamepad, mouse or Slingshot Catapult Controller


Installation
------------

Double-click the "virtual_stratton.exe" file after downloading it.
This will start the installer.  Press "OK" to use the default
installation location.  It's OK to install over a previous version of
Virtual Stratton or Soul Ride.

To uninstall, use the "Uninstall Virtual Stratton" icon in the
Virtual Stratton program group, or use the "Add/Remove Programs" icon
in the Windows Control Panel.

To run the game, go to "Start | Programs | Virtual Stratton" and click on
the "Virtual Stratton" icon.

For users of 3dfx Voodoo1 and Voodoo2 cards: The full OpenGL drivers
for Voodoo1 and Voodoo2 cards work well with Soul Ride, but you must
explicitly select 3dfx support.  Select "Display Options" when the
program starts up, and then select "3dfx stand-alone driver" before
continuing.  Users of Voodoo3 cards and above should be able to run
the game normally.


Owners of Soul Ride
-------------------

If you already own the retail version of Soul Ride, or decide to
purchase it later, it's OK to install Virtual Stratton and Soul Ride
in the same folder.  If you own Soul Ride, you can access the
additional Stratton terrain directly from Soul Ride, using the Change
Mountain option from the main menu.


Instructions
------------

Have fun and explore!  For best results, use a joystick or analog
gamepad, or the Slingshot Catapult snowboarding controller.  You can
also use the keyboard, but it's more difficult to make smooth turns.

NEW with version 1.1: you can steer with the mouse when running
fullscreen!

The basic controls are: left and right to steer the rider, button 1
or <Ctrl> to crouch (release to jump), up and down to attempt a flip
while jumping, and button 2 or <Shift> in combination with left or
right to do a grab while in the air.  The simulation is based on a
detailed physics model, so the limits of what you can do are up to
the terrain, your skill, and the laws of physics.


Troubleshooting
---------------

Graphics problems:

Problem: Very low frame rates and/or washed out dithered colors: This can
happen if OpenGL defaults to software mode instead of using your 3D
hardware accelerator.

Suggestions:

* For some cards, you must make sure you have your desktop set to
16-bit color depth.

* Make sure you have your card's latest OpenGL driver installed.  If
that sounds like gobbledegook to you, you should visit
http://www.GLSetup.com on the web and follow the instructions there.

* Not all video cards are capable of supporting Virtual Stratton's 3D
graphics.  The key factor is whether the video card has a working
OpenGL driver.  Visit http://www.GLSetup.com for the latest driver
information.  Many laptops and some older desktops just don't have
the necessary silicon, and in those cases there's nothing we can do.


Problem: I get an error message having something to do with
DirectInput or DirectSound.

Suggestion: you can avoid the use of DirectInput by running with the
option "DirectInput=0".  The control functionality with or without
DirectInput should be the same, but this option may help work around
configuration problems.  You can disable sound completely by running
with the option "Sound=0".


Other problems: visit www.soulride.com for more info.


Credits
-------

Programming: Thatcher Ulrich
Art, Design, more Programming: Mike Linkovich
Biz, Design: Mark Culliton
Biz, Design: Rob Podoloff
Concept: John Lewis
More Work: Peter Lehman, Dawna Paton, Bryan Lewis

Thanks to:

Waldemar Celes, Roberto Ierusalimschy and Luiz Henrique de Figueiredo
at TeCGraf, PUC-Rio for the Lua scripting language.
www.GLSetup.com for OpenGL support.
Ryan Haksi for OpenGL wrapper code.
US Geological Survey for data and SDTS++.
Andi Dobbs for snowboarding tips.
Jonathan Blow, Seumas McNally, Eliot Shepard, Ben Discoe, Sean T Barrett,
Jeff Lander, Karl Ulrich, Kent Quirk, Geoff Howland for good advice.
Nullsoft (http://www.nullsoft.com) for the excellent NSIS installer.
