/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <sys/event.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>

#include "luaerror.h"
#include "utils.h"

#define KQUEUE_METATABLE "kqueue"

int luaopen_sys_event(lua_State *);

static int
l_kqueue(lua_State *L)
{
	int kq, *kqp;

	kqp = lua_newuserdatauv(L, sizeof(int), 0);
	luaL_setmetatable(L, KQUEUE_METATABLE);
	*kqp = -1;

	kq = kqueuex(KQUEUE_CLOEXEC);
	if (kq == -1) {
		return (fail(L, errno));
	}
	*kqp = kq;

	return (1);
}

static int
l_kevent(lua_State *L)
{
	int *kqp;
	struct kevent *changelist, event;
	int nchanges, ret;

	kqp = luaL_checkudata(L, 1, KQUEUE_METATABLE);

	if (lua_gettop(L) == 1 || lua_type(L, 2) == LUA_TNIL) {
		changelist = NULL;
		nchanges = 0;
	} else {
		luaL_argcheck(L, lua_type(L, 2) == LUA_TTABLE, 2,
		    "`changelist' expected");
		nchanges = luaL_len(L, 2);
		changelist = malloc(nchanges * sizeof(struct kevent));
		for (size_t i = 1; i <= nchanges; ++i) {
			uintptr_t ident;
			short filter;
			u_short flags;
			u_int fflags;
			int64_t data;
			void *udata;
			int type;

			type = lua_geti(L, 2, i);
			luaL_argcheck(L, type == LUA_TTABLE, 2,
			    "`changelist' invalid");

			type = lua_getfield(L, -1, "ident");
			luaL_argcheck(L, type == LUA_TNUMBER, 2,
			    "`changelist' invalid ident");
			ident = lua_tointeger(L, -1);
			lua_pop(L, 1);

			type = lua_getfield(L, -1, "filter");
			luaL_argcheck(L, type == LUA_TNUMBER, 2,
			    "`changelist' invalid filter");
			filter = lua_tointeger(L, -1);
			lua_pop(L, 1);

			type = lua_getfield(L, -1, "flags");
			luaL_argcheck(L, type == LUA_TNUMBER, 2,
			    "`changelist' invalid flags");
			flags = lua_tointeger(L, -1);
			lua_pop(L, 1);

			type = lua_getfield(L, -1, "fflags");
			luaL_argcheck(L,
			    type == LUA_TNIL || type == LUA_TNUMBER, 2,
			    "`changelist' invalid fflags");
			fflags = luaL_optinteger(L, -1, 0);
			lua_pop(L, 1);

			type = lua_getfield(L, -1, "data");
			luaL_argcheck(L,
			    type == LUA_TNIL || type == LUA_TNUMBER, 2,
			    "`changelist' invalid data");
			data = luaL_optinteger(L, -1, 0);
			lua_pop(L, 1);

			type = lua_getfield(L, -1, "udata");
			if (type == LUA_TNIL) {
				udata = NULL;
			} else {
				luaL_argcheck(L, type == LUA_TTHREAD, 2,
				    "`changelist' invalid udata");
				/*
				 * XXX: This pointer has to outlive the event in
				 * the kqueue, but the kernel doesn't tell us
				 * when the event is removed.  Dealing with this
				 * is up to the user...
				 */
				udata = lua_tothread(L, -1);
			}
			lua_pop(L, 1);

			EV_SET(&changelist[i-1], ident, filter, flags, fflags,
			    data, udata);
			lua_pop(L, 1);
		}
	}

	ret = kevent(*kqp, changelist, nchanges, &event, 1, NULL);
	if (ret == -1) {
		return (fail(L, errno));
	} else if (ret != 1) {
		return (luaL_error(L, "kevent failed spectacularly"));
	}

	lua_newtable(L);

	lua_pushstring(L, "ident");
	lua_pushinteger(L, event.ident);
	lua_rawset(L, -3);

	lua_pushstring(L, "filter");
	lua_pushinteger(L, event.filter);
	lua_rawset(L, -3);

	lua_pushstring(L, "flags");
	lua_pushinteger(L, event.flags);
	lua_rawset(L, -3);

	lua_pushstring(L, "fflags");
	lua_pushinteger(L, event.fflags);
	lua_rawset(L, -3);

	lua_pushstring(L, "data");
	lua_pushinteger(L, event.data);
	lua_rawset(L, -3);

	if (event.udata != NULL) {
		lua_pushstring(L, "udata");
		lua_pushthread(event.udata);
		lua_xmove(event.udata, L, 1);
		lua_rawset(L, -3);
	}

	return (1);
}

static int
l_close(struct lua_State *L)
{
	int *kqp, kq;

	kqp = luaL_checkudata(L, 1, KQUEUE_METATABLE);
	kq = *kqp;
	luaL_argcheck(L, kq != -1, 1, "`kq' already closed");
	if (close(kq) == -1) {
		return (fail(L, errno));
	}
	*kqp = -1;
	return (success(L));
}

static int
l_gc(struct lua_State *L)
{
	int kq, *kqp;

	kqp = luaL_checkudata(L, 1, KQUEUE_METATABLE);
	kq = *kqp;
	if (kq == -1) {
		return (0);
	}
	close(kq);
	*kqp = -1;
	return (0);
}


static const struct luaL_Reg l_kqueue_funcs[] = {
	{"kqueue", l_kqueue},
	{NULL, NULL}
};

static const struct  luaL_Reg l_kqueue_meta[] = {
	{"kevent", l_kevent},
	{"close", l_close},
	{NULL, NULL}
};

int
luaopen_sys_event(lua_State *L)
{
	luaL_newmetatable(L, KQUEUE_METATABLE);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, l_gc);
	lua_setfield(L, -2, "__gc");

	luaL_setfuncs(L, l_kqueue_meta, 0);

	luaL_newlib(L, l_kqueue_funcs);

#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(EV_ADD);
	DEFINE(EV_ENABLE);
	DEFINE(EV_DISABLE);
	DEFINE(EV_DISPATCH);
	DEFINE(EV_DELETE);
	DEFINE(EV_RECEIPT);
	DEFINE(EV_ONESHOT);
	DEFINE(EV_CLEAR);
	DEFINE(EV_EOF);
	DEFINE(EV_ERROR);
#ifdef EV_KEEPUDATA
	DEFINE(EV_KEEPUDATA);
#endif
	DEFINE(EVFILT_READ);
	DEFINE(EVFILT_WRITE);
	DEFINE(EVFILT_EMPTY);
	DEFINE(EVFILT_AIO);
	DEFINE(EVFILT_VNODE);
	DEFINE(EVFILT_PROC);
	DEFINE(EVFILT_PROCDESC);
	DEFINE(EVFILT_SIGNAL);
	DEFINE(EVFILT_TIMER);
	DEFINE(EVFILT_USER);
	DEFINE(NOTE_FFNOP);
	DEFINE(NOTE_FFAND);
	DEFINE(NOTE_FFOR);
	DEFINE(NOTE_FFCOPY);
	DEFINE(NOTE_FFCTRLMASK);
	DEFINE(NOTE_FFLAGSMASK);
	DEFINE(NOTE_TRIGGER);
	DEFINE(NOTE_LOWAT);
	DEFINE(NOTE_FILE_POLL);
	DEFINE(NOTE_DELETE);
	DEFINE(NOTE_WRITE);
	DEFINE(NOTE_EXTEND);
	DEFINE(NOTE_ATTRIB);
	DEFINE(NOTE_LINK);
	DEFINE(NOTE_RENAME);
	DEFINE(NOTE_REVOKE);
	DEFINE(NOTE_OPEN);
	DEFINE(NOTE_CLOSE);
	DEFINE(NOTE_CLOSE_WRITE);
	DEFINE(NOTE_READ);
	DEFINE(NOTE_EXIT);
	DEFINE(NOTE_FORK);
	DEFINE(NOTE_EXEC);
	DEFINE(NOTE_PCTRLMASK);
	DEFINE(NOTE_PDATAMASK);
	DEFINE(NOTE_TRACK);
	DEFINE(NOTE_TRACKERR);
	DEFINE(NOTE_CHILD);
	DEFINE(NOTE_SECONDS);
	DEFINE(NOTE_MSECONDS);
	DEFINE(NOTE_USECONDS);
	DEFINE(NOTE_NSECONDS);
	DEFINE(NOTE_ABSTIME);
#undef DEFINE
	return (1);
}
