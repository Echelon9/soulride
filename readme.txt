Soul Ride
=========

v1.5 2004 June 3

by Slingshot Game Technology, Inc.

Copyright 2003 All rights reserved.

http://www.soulride.com


About
-----

Soul Ride is a single-player PC snowboarding game which emphasizes
purity, realism and extreme backcountry terrain.  The runs are drawn
from geological survey data of real-world mountains.  The action is
based on a meticulous simulation of snowboarding physics.  The
gameplay is straight out of the old-school arcade.  And every move
the player makes is recorded on a virtual VCR.


Platform Requirements
---------------------

* Intel Pentium II 300 or faster
* Windows 9x, ME, NT, 2000, XP or Linux
* 3D hardware accelerator with OpenGL 1.1 driver (successfully tested
  with Voodoo 1, Voodoo 2, Voodoo 3, i810, RIVA 128, RIVA TNT,
  GeForce, Rage 128, Radeon)
* 64 MB RAM
* 20 MB disk space
* Joystick, gamepad or Slingshot Catapult Controller


Installation (Windows)
----------------------

Run "setup.exe" from the CD-ROM to start the installer.  To uninstall,
use the "Uninstall Soul Ride" icon in the Soul Ride program group, or
use the "Add/Remove Programs" icon in the Windows Control Panel.

To run the game, go to "Start | Programs | Soul Ride" and click on
the "Soul Ride" icon.

For users of 3dfx Voodoo1 and Voodoo2 cards: The full OpenGL drivers
for Voodoo1 and Voodoo2 cards work well with Soul Ride, but you must
explicitly select 3dfx support.  Select "Display Options" when the
program starts up, and then select "3dfx stand-alone driver" before
continuing.  Users of Voodoo3 cards and above should be able to run
the game normally.


Installation (Linux)
--------------------

Change to the directory where you want to install (for example,
"/usr/games").  Unpack the distribution with the command:

  # tar -xzvf virtual_jay_peak_linux_1_1a.tar.gz

(Replace the file name with the specific archive you have downloaded.)
This will create a subdirectory named "soulride", containing the game
program and the required data.  To run the game, change into the
soulride directory, and use the command:

  # ./start_Jay_Peak.sh

If you downloaded a different mountain, look for a script named
"start_<Mountain Name>.sh" and run it instead.

See readme-linux.txt for more info on starting the game.


Instructions
------------

Soul Ride was designed with a joystick or Catapult in mind, but it
also works with the keyboard or mouse.

+---------------+-------------------+-------------------+--------------------------+
|   function    | joystick/gamepad  |     keyboard      |          mouse           |
+===============+===================+===================+==========================+
| steer         | left/right stick  | left/right arrows | move left/right          |
+---------------+-------------------+-------------------+--------------------------+
| jump          | button 1          | CTRL              | button 1 (left)          |
+---------------+-------------------+-------------------+--------------------------+
| grab (in air) | button 2 + steer  | SHIFT + steer     | button 2 (right) + steer |
+---------------+-------------------+-------------------+--------------------------+
| flip          |                         jump + up/down                           |
+---------------+------------------------------------------------------------------+
| back/cancel   | button 4          | ESC               |                          |
+---------------+-------------------+-------------------+--------------------------+
| select        | button 1          | ENTER             | button 1 (left)          |
+---------------+-------------------+-------------------+--------------------------+
| music info    |                     F2                                           |
+---------------+------------------------------------------------------------------+
| last cd track |                     F3                                           |
+---------------+------------------------------------------------------------------+
| next cd track |                     F4                                           |
+---------------+------------------------------------------------------------------+


Notes:

* All of the tricks are derived from those controls. The more
  impressive and higher-scoring flips, spins and air are based on
  timing, technique and terrain, so keep practicing!

* With a joystick or mouse, you control the boarder's steering by
  pressing the stick left or right. The steering is proportional, and
  if you press all the way left or right, the boarder will
  skid. Partway to either side, there is a sweet spot which makes the
  boarder carve with the minimum of skidding.  Using the keyboard, you
  can adjust the amount of steer by tapping the arrow keys on and off
  instead of holding them down.


Configuration
-------------

The Soul Ride engine uses the Lua language (version 3.2, see
http://www.lua.org) for configuration of various options.  There is a
partial list of options below.  You can set options on the command
line, or in a configuration file, or interactively through a command
console.

To set options on the command-line, start the program like so:

> soulride option1=value1 option2=value2 option3=value3 ...

To set options via a configuration file, edit the file "config.txt" in
the main soulride directory.  This file is a list of variable
assignments, like:

option1="value1"
option2="value2"
...

You can add additional options here, that will be set automatically
each time you start Soul Ride.

Finally, you can adjust options while the game is running using the
console.  To activate the console, press the ` key (that's the single
backward quote key).  A dark overlay should pop down, and you can
type:

> option1=value1

to change options.  Press ` again to make the console go away.  The
console language is Lua 3.2, so you can type valid Lua expressions as
well.

Options are case-sensitive.

Some options:

Music          | Boolean, default "1".  Set to 0 to disable cd-audio, or 1 to enable it.
JoystickPort   | 
DirectInput    | (Windows only) Boolean, default "1".  Use DirectInput to read the joystick.  Should work either way.
EnableJets     | Boolean, default "0".  Set to 1 to enable jet-power on button 2.


Troubleshooting
---------------

Graphics problems:

Problem: Very low frame rates and/or washed out dithered colors: This can
happen if OpenGL defaults to software mode instead of using your 3D
hardware accelerator.

Suggestions:

* For some cards, you must make sure you have your desktop set to
16-bit color depth.

* The game is known to work well with 3dfx Voodoo, NVIDIA RIVA and
GeForce, and ATI Radeon based video cards.  Other types of 3D
accelerators may or may not work so well.

* Make sure you have your card's latest OpenGL driver installed.  If
that sounds like gobbledegook to you, you should run GLSetup.
GLSetup can be found on the game CD; locate the icon labeled
"GLSetup" and double-click it to go through the driver installation
process.


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


Featuring music by Sinkhole, The Sadies, New Sweet Breath, Waco
Brothers, Scroat Belly, Bender, Neko Case & Her Boyfriends, Split Lip
Rayfield, and Alejandro Escovedo.

Those bands and more can be found at:

Bloodshot Records -- www.bloodshotrecords.com
Ringing Ear Records -- www.tulrich.com/rer
Mint Records -- www.mintrecs.com



History
-------

1.5 -- Localization tweaks, fix some bugs in VCR mouse handling

1.4 -- Polish localization, better UTF-8 handling, misc

1.3x -- multiplayer work-in-progress from Bjorn Leffler, Italian
  localization

1.3 -- German localization, misc cleanups and fixes
