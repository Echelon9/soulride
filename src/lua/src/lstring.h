/*
** $Id: lstring.h,v 1.1.1.1 2003/01/18 20:14:27 tulrich Exp $
** String table (keep all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#ifndef lstring_h
#define lstring_h


#include "lobject.h"


void luaS_init (void);
TaggedString *luaS_createudata (void *udata, int tag);
TaggedString *luaS_collector (void);
void luaS_free (TaggedString *l);
TaggedString *luaS_newlstr (char *str, long l);
TaggedString *luaS_new (char *str);
TaggedString *luaS_newfixedstring (char *str);
void luaS_rawsetglobal (TaggedString *ts, TObject *newval);
char *luaS_travsymbol (int (*fn)(TObject *));
int luaS_globaldefined (char *name);
TaggedString *luaS_collectudata (void);
void luaS_freeall (void);


#endif
