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
///////////////////////////////////////////////////////
//
//  gg_log.h
//
//  GameGUI error logging util
//
//  First call to GG_LOG() will open a new text
//  file.  Additional calls will add to it.
//
//  Author: Mike Linkovich
//
//  Start Date: Feb 15 2000
//
///////////////////////////////


#ifndef _GAMEGUI_LOG_INCLUDED
#define _GAMEGUI_LOG_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

void GG_logStr( const char *buf );


//  To control logging via a compiler switch, use
//  the GG_LOG() macro instead of GG_logStr directly.
//  Use #define GAMEGUI_NOLOG to disable logging,

#ifndef GAMEGUI_NOLOG
#define GG_LOG(X) GG_logStr(X)
#else
#define GG_LOG(X)
#endif


#ifdef __cplusplus
};
#endif


#endif  // _GAMEGUI_LOG_INCLUDED
