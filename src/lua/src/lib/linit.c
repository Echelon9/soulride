/*
** $Id: linit.c,v 1.1.1.1 2003/01/18 20:14:29 tulrich Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in lua.h
*/

#include "lua.h"
#include "lualib.h"


void lua_userinit (void) {
  lua_iolibopen();
  lua_strlibopen();
  lua_mathlibopen();
  lua_dblibopen();
}

