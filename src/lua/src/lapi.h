/*
** $Id: lapi.h,v 1.1.1.1 2003/01/18 20:14:19 tulrich Exp $
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#ifndef lapi_h
#define lapi_h


#include "lua.h"
#include "lobject.h"


TObject *luaA_Address (lua_Object o);
void luaA_pushobject (TObject *o);
void luaA_packresults (void);
int luaA_passresults (void);
TaggedString *luaA_nextvar (TaggedString *g);
int luaA_next (Hash *t, int i);

#endif
