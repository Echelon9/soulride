You must have 3D hardware, with a working OpenGL driver (e.g. Mesa,
NVidia's proprietary driver, etc).  You must also have SDL installed
(http://www.libsdl.org) to run the "soulride" executable, although the
"soulride-static" executable does not require SDL to be installed.

The executables "soulride" and "soulride-static" are the same, except
that "soulride" is dynamically linked against SDL, SDL_mixer and the
C++ standard library, while "soulride-static" is statically linked
against those libraries.

Try "soulride" first, and if that doesn't work, try again with
"soulride-static".

Command-line options:

  Syntax: "soulride option1=value1 option2=value2 ..."

  * Fullscreen -- 0 for windowed, 1 for fullscreen

  * DefaultMountain -- give the name of the mountain, e.g. "Jay_Peak",
    "Breckenridge", "Stratton".  These names come from the directory
    name under data/ where the mountain-specific data files are
    stored.

  * OGLModeIndex -- these are the options:
    	0: 320, 240, 16
	1: 640, 480, 16
	2: 800, 600, 16
	3: 1024, 768, 16
	4: 1280, 1024, 16
	5: 320, 240, 32
	6: 640, 480, 32
	7: 800, 600, 32
	8: 1024, 768, 32
	9: 1280, 1024, 32

Thanks to the following people for Linux advice or assistance with
testing on Linux:

Andreas Umbach
Sean Middleditch
Ruediger Arp
Chunky Kibbles
hursh
zakk
