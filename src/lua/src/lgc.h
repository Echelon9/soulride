/*
** $Id: lgc.h,v 1.1.1.1 2003/01/18 20:14:22 tulrich Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/

#ifndef lgc_h
#define lgc_h


#include "lobject.h"


void luaC_checkGC (void);
TObject* luaC_getref (int ref);
int luaC_ref (TObject *o, int lock);
void luaC_hashcallIM (Hash *l);
void luaC_strcallIM (TaggedString *l);


#endif
