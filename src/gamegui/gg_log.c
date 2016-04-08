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


void
GG_logStr( const char *buf )
{
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
}
