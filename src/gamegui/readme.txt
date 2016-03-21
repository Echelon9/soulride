///////////////////////////////////////////////////////
//
//  File readme.txt
//
//  GameGUI info
//
//  Author: Mike Linkovich
//
//  Start Date: Feb 10 2000
//
///////////////////////////////


See gamegui.h for class function documentation.
See winmain.cpp for example usage (create, load, play, destroy)

gamegui.exe plays back the test.ggm GameGUI Movie file.  It relies on your default OpenGL (opengl32.dll) driver residing in the Windows directory.


-------------------------------

Compiling

No makefile is included, it's probably easier just to add all the .cpp and .h source files to your project, to whichever directory you choose, and add it to your include path.

The only include directive reqired by your app will be #include "gamegui.h".

OpenGL headers are expected to reside in the directory "gl\".  The only library dependencies (so far) are in opengl32.lib

The STL is used throughout the implementation code to manage data structures.  For now, since movie data is static, <vector> and <algo> are the only components used.  In order to re-compile the source, you will need to be able to compile those STL components.  Your application however is shielded from STL dependencies.  [this may change in future...]

Be sure to add the STL include path to the project.  No use is made of <string> or any other types that overwrite the standard C library, so the order of inclusion shouldn't matter.

The STL source used is the STLport (adapted SGI STL) version 3.01.  It is available online from: http://www.metabyte.com/~fbp/stl/

Some basic types are (cautiously) defined that may already be supported by your compiler.  They are all in the file "gg_types.h".  They are all bracketed by #ifndef/#endif pairs to try to avoid redefinition.


-------------------------------

Conventions

The library name is GameGUI.  This is the root object.  From this you can load and create GG_Movie and GG_Player objects.  Typically the application will use the GG_Player object to control playback.  All objects are C++ virtual interfaces.

Most functions return error codes in the event of failure.  Error codes are defined in "gamegui.h"

Except for a few simple types mentioned above, and the GameGUI object, all other types are prefaced by GG_.  Eg:  GG_Movie, GG_Player.

Internally, member variables are prefaced by m_ to distinguish from temp and parameter variables.


-------------------------------

Memory management

All objects accessible to the application are created via appropriate create functions.  Since they are only interface classes, don't try to use constructors.  Objects are managed by reference counting.  To free an object, use its unRef() function.  To the appliation, this will generally mean doing something like the following:

>
GameGUICreate(&rc, flags, &gameGuiObject);
gameGuiObject->createMovie( "movie.ggm", &movie );
gameGuiObject->createPlayer( movie, &player );
...
player->unRef();  player = null;
movie->unRef();  movie = null;
gameGuiObject->unRef();  gameGuiObject = null;
>

Because of reference counting, it is safe to do the following:

>
gameGuiObject->createMovie( "movie.ggm", &movie );
gameGuiObject->createPlayer( movie, &player );
movie->unRef();  movie = null;
player->play(1);
>

The player will call movie->addRef() when it is created, and release a reference when it is destroyed.

As usual with reference counting, do not use an object after you call unRef().  But you can use addRef/unRef to manage your app's use of GameGUI resources.

Note that when the reference count of the GameGUI object drops to 0, or when it is deleted directly, it will stop and destroy all players, movies and resources used by its instance.  Any pointers remaining in the app will be invalid.

Note that delete may be used *only* with the GameGUI object, destroying it regardless of its reference count.

Multiple players may be created from one movie.  This is probably not going to be used too often, but it is an option.  Each new player created from the same movie will increment the movie's reference count.

-------------------------------

Movie files

Movie files are text files that describe specific types of "actors" and path/filenames all of the resource files.  They also define event scripts containing lists of events.  The file format is documented in the example movie file "example.ggm".

File dependencies are loaded relative to the directory of the root movie file.


-------------------------------

Mike Linkovich
Feb 2000
