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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
// avi_tools.hpp	-thatcher 12/30/2000 Copyright Slingshot Game Technology

// Header for routines to handle AVI files.


#ifndef AVI_TOOLS_HPP
#define AVI_TOOLS_HPP


#include "types.hpp"
#include "geometry.hpp"


namespace avi_tools {
	struct avi_stream {
		FILE*	fp;
		long	ChunkStack[10];
		int	ChunkSP;
		int	FrameCount;
		int	FrameChunkSize;
		
		void	open_video(FILE* fp, int width, int height, int frame_period_ms);
		void	write_video_frame(uint32* pixels, int width, int height);
		void	close();

		void	open_chunk(char* fourcc);
		void	close_chunk();
	};
};


#endif // AVI_TOOLS_HPP

