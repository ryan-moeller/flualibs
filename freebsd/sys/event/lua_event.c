/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <sys/event.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"
#include "utils.h"

#define KQUEUE_METATABLE "kqueue"

int luaopen_freebsd_sys_event(lua_State *);

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
	lua_pushboolean(L, true);
	return (1);
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
luaopen_freebsd_sys_event(lua_State *L)
{
	luaL_newmetatable(L, KQUEUE_METATABLE);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, l_gc);
	lua_setfield(L, -2, "__gc");

	luaL_setfuncs(L, l_kqueue_meta, 0);

	luaL_newlib(L, l_kqueue_funcs);

#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(EV_ADD);
	SETCONST(EV_ENABLE);
	SETCONST(EV_DISABLE);
	SETCONST(EV_DISPATCH);
	SETCONST(EV_DELETE);
	SETCONST(EV_RECEIPT);
	SETCONST(EV_ONESHOT);
	SETCONST(EV_CLEAR);
	SETCONST(EV_EOF);
	SETCONST(EV_ERROR);
#ifdef EV_KEEPUDATA
	SETCONST(EV_KEEPUDATA);
#endif
	SETCONST(EVFILT_READ);
	SETCONST(EVFILT_WRITE);
	SETCONST(EVFILT_EMPTY);
	SETCONST(EVFILT_AIO);
	SETCONST(EVFILT_VNODE);
	SETCONST(EVFILT_PROC);
	SETCONST(EVFILT_PROCDESC);
	SETCONST(EVFILT_SIGNAL);
	SETCONST(EVFILT_TIMER);
	SETCONST(EVFILT_USER);
	SETCONST(NOTE_FFNOP);
	SETCONST(NOTE_FFAND);
	SETCONST(NOTE_FFOR);
	SETCONST(NOTE_FFCOPY);
	SETCONST(NOTE_FFCTRLMASK);
	SETCONST(NOTE_FFLAGSMASK);
	SETCONST(NOTE_TRIGGER);
	SETCONST(NOTE_LOWAT);
	SETCONST(NOTE_FILE_POLL);
	SETCONST(NOTE_DELETE);
	SETCONST(NOTE_WRITE);
	SETCONST(NOTE_EXTEND);
	SETCONST(NOTE_ATTRIB);
	SETCONST(NOTE_LINK);
	SETCONST(NOTE_RENAME);
	SETCONST(NOTE_REVOKE);
	SETCONST(NOTE_OPEN);
	SETCONST(NOTE_CLOSE);
	SETCONST(NOTE_CLOSE_WRITE);
	SETCONST(NOTE_READ);
	SETCONST(NOTE_EXIT);
	SETCONST(NOTE_FORK);
	SETCONST(NOTE_EXEC);
	SETCONST(NOTE_PCTRLMASK);
	SETCONST(NOTE_PDATAMASK);
	SETCONST(NOTE_TRACK);
	SETCONST(NOTE_TRACKERR);
	SETCONST(NOTE_CHILD);
	SETCONST(NOTE_SECONDS);
	SETCONST(NOTE_MSECONDS);
	SETCONST(NOTE_USECONDS);
	SETCONST(NOTE_NSECONDS);
	SETCONST(NOTE_ABSTIME);
#undef SETCONST
	return (1);
}
