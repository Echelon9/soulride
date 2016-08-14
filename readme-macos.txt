--------------------------------------------
        Notes on the macOS port
--------------------------------------------

The macOS port is very similar to the Linux version. It is possible to only use X11 instead of Cocoa, which is how I started. This port uses the native Cocoa/OpenGL Frameworks. It also requires SDL2 and SDL2_mixer to be installed or distributed with the game (see below).

The Cocoa version behaves like any macOS application. There is a clickable icon in the Finder and an initial options window.

Joysticks are supported through SDL 2.0. I believe most HID compliant USB joysticks will work.

--------------------------------------------
           Development comments
--------------------------------------------

To build SoulRide, you need to install the Developer Tools distributed by Apple (gcc/g++, make, etc.) I'm using the latest version (gcc 4.0)

To build, run "make -f Makefile.macosx SoulRide.app" in the src directory. In a terminal of course. This creates the "SoulRide.app" application bundle, which can be doubleclicked in finder.

SoulRide links against the SDL2 and SDL2_mixer frameworks. It is possible to distribute the frameworks with the application by putting them in Soul_Ride.app/Contents/Frameworks. A framework is a bundle of headers and shared libraries. To install them system-wide, put SDL2.framework and SDL2_mixer.framework in /Library/Frameworks.

To get the debug messages written to stdout/stderr, start the game using Soul_Ride.app/Contents/MacOS/Soul_Ride from a terminal.

Where are the highscore/player files ?
They are in stored in the users Document folder (usually /Users/$USER/Documents/SoulRide/PlayerData)

Porting to MacOS 9 should be a piece of cake (just recompile), but I'm not going to do that.

Bjorn Leffler
6 Nov 2005
