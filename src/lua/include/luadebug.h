/*
** $Id: luadebug.h,v 1.1.1.1 2003/01/18 20:14:18 tulrich Exp $
** Debugging API
** See Copyright Notice in lua.h
*/


#ifndef luadebug_h
#define luadebug_h


#include "lua.h"

typedef lua_Object lua_Function;

typedef void (*lua_LHFunction) (int line);
typedef void (*lua_CHFunction) (lua_Function func, char *file, int line);

lua_Function lua_stackedfunction (int level);
void lua_funcinfo (lua_Object func, char **source, int *linedefined);
int lua_currentline (lua_Function func);
char *lua_getobjname (lua_Object o, char **name);

lua_Object lua_getlocal (lua_Function func, int local_number, char **name);
int lua_setlocal (lua_Function func, int local_number);

int lua_nups (lua_Function func);

lua_LHFunction lua_setlinehook (lua_LHFunction func);
lua_CHFunction lua_setcallhook (lua_CHFunction func);
int lua_setdebug (int debug);


#endif
