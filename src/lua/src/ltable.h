/*
** $Id: ltable.h,v 1.1.1.1 2003/01/18 20:14:27 tulrich Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#ifndef ltable_h
#define ltable_h

#include "lobject.h"


#define node(t,i)       (&(t)->node[i])
#define ref(n)		(&(n)->ref)
#define val(n)		(&(n)->val)
#define nhash(t)	((t)->nhash)

#define luaH_get(t,ref)	(val(luaH_present((t), (ref))))
#define luaH_move(t,from,to)	(luaH_setint(t, to, luaH_getint(t, from)))

Hash *luaH_new (int nhash);
void luaH_free (Hash *frees);
Node *luaH_present (Hash *t, TObject *key);
void luaH_set (Hash *t, TObject *ref, TObject *val);
int luaH_pos (Hash *t, TObject *r);
void luaH_setint (Hash *t, int ref, TObject *val);
TObject *luaH_getint (Hash *t, int ref);


#endif
