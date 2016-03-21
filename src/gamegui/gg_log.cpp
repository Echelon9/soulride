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
///////////////////////////////////////////////////////
//
//  gg_log.cpp
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


#include "gg_log.h"
#include <stdio.h>


static char GG_LogFileName[] = "gamegui.log";
static int GG_LogStarted = 0;


void GG_logStr( char *buf )
{
#ifndef NDEBUG
  FILE *fp;

  if( GG_LogStarted == 0 )
  {
    fp = fopen( GG_LogFileName, "w" );
    GG_LogStarted = 1;
  }
  else
    fp = fopen( GG_LogFileName, "a" );

  if( fp != NULL )
  {
    fprintf( fp, buf );
    fprintf( fp, "\n" );
    fclose(fp);
  }
#endif // not NDEBUG
}
