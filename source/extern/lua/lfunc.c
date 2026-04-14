/*
** $Id: lfunc.c,v 2.45.1.1 2017/04/19 17:39:34 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#define lfunc_c
#define LUA_CORE

#include "lprefix.h"


#include <stddef.h>

#include "lua.h"

#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"



CClosure *luaF_newCclosure (lua_State *L, int n) {
  GCObject *o = luaC_newobj(L, LUA_TCCL, sizeCclosure(n));
  CClosure *c = gco2ccl(o);
  c->nupvalues = cast_byte(n);
  return c;
}


LClosure *luaF_newLclosure (lua_State *L, int n) {
  GCObject *o = luaC_newobj(L, LUA_TLCL, sizeLclosure(n));
  LClosure *c = gco2lcl(o);
  c->p = NULL;
  c->nupvalues = cast_byte(n);
  while (n--) c->upvals[n] = NULL;
  return c;
}

/*
** fill a closure with new closed upvalues
*/
void luaF_initupvals (lua_State *L, LClosure *cl) {
  int i;
  for (i = 0; i < cl->nupvalues; i++) {
    UpVal *uv = luaM_new(L, UpVal);
    uv->refcount = 1;
    uv->v = &uv->u.value;  /* make it closed */
    setnilvalue(uv->v);
    cl->upvals[i] = uv;
  }
}


UpVal *luaF_findupval (lua_State *L, StkId level) {
  UpVal **pp = &L->openupval;
  UpVal *p;
  UpVal *uv;
  lua_assert(isintwups(L) || L->openupval == NULL);
  while (*pp != NULL && (p = *pp)->v >= level) {
    lua_assert(upisopen(p));
    if (p->v == level && !p->flags)  /* found a corresponding upvalue that is not a deferred value? */ {
      return p; /* return it */
    }
    pp = &p->u.open.next;
  }
  /* not found: create a new upvalue */
  uv = luaM_new(L, UpVal);
  uv->refcount = 0;
  uv->flags = 0;
  uv->u.open.next = *pp;  /* link it to list of open upvalues */
  uv->u.open.touched = 1;
  *pp = uv;
  uv->v = level;  /* current value lives in the stack */
  if (!isintwups(L)) {  /* thread not in list of threads with upvalues? */
    L->twups = G(L)->twups;  /* link it to the list */
    G(L)->twups = L;
  }
  return uv;
}

static void calldeferred(lua_State *L, void *ud) {
  UNUSED(ud);
  luaD_callnoyield(L, L->top - 2, 0);
}

/*
** Prepare deferred function plus its arguments for object 'obj' with
** error message 'err'. (This function assumes EXTRA_STACK.)
*/
static int preparetocall(lua_State *L, TValue *func, TValue *err) {
  StkId top = L->top;
  setobj2s(L, top, func);  /* will call deferred function */
  if (err) {
    setobj2s(L, top + 1, err); /* and error msg. as 1st argument */
  }
  else {
    setnilvalue(top + 1);
  }
  L->top = top + 2;  /* add function and arguments */
  return 1;
}

/*
** Prepare and call a deferred function. If status is OK, code is still
** inside the original protected call, and so any error will be handled
** there. Otherwise, a previous error already activated the original
** protected call, and so the call to the deferred method must be
** protected here. (A status == -1 behaves like a previous
** error, to also run the closing method in protected mode).
** If status is OK, the call to the deferred method will be pushed
** at the top of the stack. Otherwise, values are pushed after
** the 'level' of the upvalue containing deferred function, as everything after
** that won't be used again.
*/
static int calldeferredfunction(lua_State *L, StkId level, int status) {
  TValue *uv = level; /* value being closed */
  if (status == LUA_OK) {
    preparetocall(L, uv, NULL); /* something to call? */
    calldeferred(L, NULL);      /* call closing method */
  }
  else { /* must close the object in protected mode */
    ptrdiff_t oldtop;
    level++;                            /* space for error message */
    oldtop = savestack(L, level + 1);   /* top will be after that */
    luaD_seterrorobj(L, status, level); /* set error message */
    preparetocall(L, uv, level);
    int newstatus = luaD_pcall(L, calldeferred, NULL, oldtop, 0);
    if (newstatus != LUA_OK && status == CLOSEPROTECT) /* first error? */
      status = newstatus;                    /* this will be the new error */
    else {
      /* leave original error (or nil) on top */
      L->top = restorestack(L, oldtop);
    }
  }
  return status;
}

int luaF_close (lua_State *L, StkId level, int status) {
  UpVal *uv;
  while (L->openupval != NULL && (uv = L->openupval)->v >= level) {
    lua_assert(upisopen(uv));
    L->openupval = uv->u.open.next;  /* remove from 'open' list */
    if (uv->refcount == 0) {        /* no references? */
      UpVal uv1 = *uv;              /* copy the upvalue as we will free it below */
      luaM_free(L, uv);             /* free upvalue before invoking any deferred functions */
      if (status != NOCLOSINGMETH && uv1.flags && ttisfunction(uv1.v)) {
        ptrdiff_t levelrel = savestack(L, level);
        status = calldeferredfunction(L, uv1.v, status);
        level = restorestack(L, levelrel);
      }
    }
    else {
      setobj(L, &uv->u.value, uv->v);  /* move value to upvalue slot */
      uv->v = &uv->u.value;  /* now current value lives here */
      luaC_upvalbarrier(L, uv);
    }
  }
  return status;
}


Proto *luaF_newproto (lua_State *L) {
  GCObject *o = luaC_newobj(L, LUA_TPROTO, sizeof(Proto));
  Proto *f = gco2p(o);
  f->k = NULL;
  f->sizek = 0;
  f->p = NULL;
  f->sizep = 0;
  f->code = NULL;
  f->cache = NULL;
  f->sizecode = 0;
  f->lineinfo = NULL;
  f->sizelineinfo = 0;
  f->upvalues = NULL;
  f->sizeupvalues = 0;
  f->numparams = 0;
  f->is_vararg = 0;
  f->maxstacksize = 0;
  f->locvars = NULL;
  f->sizelocvars = 0;
  f->linedefined = 0;
  f->lastlinedefined = 0;
  f->source = NULL;
  return f;
}


void luaF_freeproto (lua_State *L, Proto *f) {
  luaM_freearray(L, f->code, f->sizecode);
  luaM_freearray(L, f->p, f->sizep);
  luaM_freearray(L, f->k, f->sizek);
  luaM_freearray(L, f->lineinfo, f->sizelineinfo);
  luaM_freearray(L, f->locvars, f->sizelocvars);
  luaM_freearray(L, f->upvalues, f->sizeupvalues);
  luaM_free(L, f);
}


/*
** Look for n-th local variable at line 'line' in function 'func'.
** Returns NULL if not found.
*/
const char *luaF_getlocalname (const Proto *f, int local_number, int pc) {
  int i;
  for (i = 0; i<f->sizelocvars && f->locvars[i].startpc <= pc; i++) {
    if (pc < f->locvars[i].endpc) {  /* is variable active? */
      local_number--;
      if (local_number == 0)
        return getstr(f->locvars[i].varname);
    }
  }
  return NULL;  /* not found */
}

