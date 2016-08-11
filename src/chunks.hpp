/*
    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.

    This file is part of The Soul Ride Engine, see http://soulride.com

    The Soul Ride Engine is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    The Soul Ride Engine is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
// chunks.hpp	-thatcher 6/4/1998 Copyright Slingshot

// Definition of chunk types, for parsing Soul Ride input files.  The
// basic idea is that the file is composed of a series of chunks, each
// of which may contain other chunks.  Each chunk has a type field which
// selects how the data should be interpreted, as well as a skip-count
// so that the file-reader can skip the chunk if it doesn't understand
// the type (or if it's just not interested).


namespace Chunks {


enum ChunkIDType {
	TERRAIN = 0,
	OBJECTLIST,
	// GEOMETRY,
	// SCRIPTS
	
	CHUNKCOUNT
};


};	// End namespace Chunks.
