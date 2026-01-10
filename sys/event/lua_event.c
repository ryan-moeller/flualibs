/*
 * Copyright (c) 2024-2026 Ryan Moeller
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
	int *kqp;
	u_int flags;

	flags = luaL_optinteger(L, 1, 0);

	kqp = lua_newuserdatauv(L, sizeof(int), 0);
	if ((*kqp = kqueuex(flags)) == -1) {
		return (fail(L, errno));
	}
	luaL_setmetatable(L, KQUEUE_METATABLE);
	return (1);
}

static int
l_wrap(lua_State *L)
{
	int *kqp, kq;

	kq = luaL_checkinteger(L, 1);

	kqp = lua_newuserdatauv(L, sizeof(int), 0);
	*kqp = kq;
	luaL_setmetatable(L, KQUEUE_METATABLE);
	return (1);
}

static int
l_kevent(lua_State *L)
{
	int *kqp;
	struct kevent *changelist, event;
	int nchanges;

	kqp = luaL_checkudata(L, 1, KQUEUE_METATABLE);

	if (lua_isnoneornil(L, 2)) {
		changelist = NULL;
		nchanges = 0;
	} else {
		luaL_argcheck(L, lua_type(L, 2) == LUA_TTABLE, 2,
		    "`changelist' expected");
		nchanges = luaL_len(L, 2);
		if ((changelist = malloc(nchanges * sizeof(struct kevent)))
		    == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
		for (size_t i = 1; i <= nchanges; ++i) {
			uintptr_t ident;
			short filter;
			u_short flags;
			u_int fflags;
			int64_t data;
			void *udata;

			luaL_argcheck(L, lua_geti(L, 2, i) == LUA_TTABLE, 2,
			    "`changelist' invalid");

			/* Accept "cookie" as an alias for "ident" field. */
			if (lua_getfield(L, -1, "cookie")
			    == LUA_TLIGHTUSERDATA) {
				/* Some event sources put a pointer in ident. */
				ident = (uintptr_t)lua_touserdata(L, -1);
				lua_pop(L, 1);
			} else if (lua_getfield(L, -2, "ident")
			    == LUA_TNUMBER) {
				ident = lua_tointeger(L, -1);
				lua_pop(L, 2);
			} else {
				return (luaL_argerror(L, 2,
				    "`changelist' invalid ident"));
			}

#define INTFIELD(name) ({ \
			luaL_argcheck(L, \
			    lua_getfield(L, -1, #name) == LUA_TNUMBER, 2, \
			    "`changelist' invalid " #name); \
			name = lua_tointeger(L, -1); \
			lua_pop(L, 1); \
})
			INTFIELD(filter);
			INTFIELD(flags);
#undef INTFIELD
#define OPTINTFIELD(name) ({ \
			if (lua_getfield(L, -1, #name) == LUA_TNIL) { \
				name = 0; \
			} else if (lua_isinteger(L, -1)) { \
				name = lua_tointeger(L, -1); \
			} else { \
				return (luaL_argerror(L, 2, \
				    "`changelist' invalid " #name)); \
			} \
			lua_pop(L, 1); \
})
			OPTINTFIELD(fflags);
			OPTINTFIELD(data);
#undef OPTINTFIELD
			switch (lua_getfield(L, -1, "udata")) {
			case LUA_TNIL:
				udata = NULL;
				break;
			case LUA_TTHREAD:
				/*
				 * XXX: This pointer has to outlive the event in
				 * the kqueue, but the kernel doesn't tell us
				 * when the event is removed.  Dealing with this
				 * is up to the user...
				 */
				udata = lua_tothread(L, -1);
				break;
			default:
				return (luaL_argerror(L, 2,
				    "`changelist' invalid udata"));
			}
			lua_pop(L, 1);

			EV_SET(&changelist[i-1], ident, filter, flags, fflags,
			    data, udata);
			lua_pop(L, 1);
		}
	}

	/* TODO: Getting more than one event at a time would be nice. */
	switch (kevent(*kqp, changelist, nchanges, &event, 1, NULL)) {
	case 1:
		break;
	case -1:
		return (fail(L, errno));
	default:
		return (luaL_error(L, "kevent failed spectacularly"));
	}

	lua_newtable(L);
#define INTFIELD(name) ({ \
	lua_pushinteger(L, event.name); \
	lua_setfield(L, -2, #name); \
})
	INTFIELD(ident);
	/* Some event sources put a pointer in ident (e.g. AIO). */
	lua_pushlightuserdata(L, (void *)event.ident);
	lua_setfield(L, -2, "cookie");
	INTFIELD(filter);
	INTFIELD(flags);
	INTFIELD(fflags);
	INTFIELD(data);
	/* FIXME: coroutine must be from same thread state */
	if (event.udata != NULL) {
		lua_pushstring(L, "udata");
		lua_pushthread(event.udata);
		lua_xmove(event.udata, L, 1);
		lua_rawset(L, -3);
	}
#undef INTFIELD
	return (1);
}

static int
l_close(struct lua_State *L)
{
	int *kqp, kq;

	kqp = luaL_checkudata(L, 1, KQUEUE_METATABLE);
	if ((kq = *kqp) == -1) {
		/* Already closed.  Return nothing to be safe for <close>. */
		return (0);
	}
	if (close(kq) == -1) {
		return (fail(L, errno));
	}
	*kqp = -1;
	return (success(L));
}

static int
l_fileno(lua_State *L)
{
	int *kqp, kq;

	kqp = luaL_checkudata(L, 1, KQUEUE_METATABLE);
	if ((kq = *kqp) == -1) {
		return (0);
	}
	lua_pushinteger(L, kq);
	return (1);
}

static int
l_gc(struct lua_State *L)
{
	int *kqp, kq;

	kqp = luaL_checkudata(L, 1, KQUEUE_METATABLE);
	if ((kq = *kqp) == -1) {
		return (0);
	}
	close(kq);
	*kqp = -1;
	return (0);
}


static const struct luaL_Reg l_kqueue_funcs[] = {
	{"kqueue", l_kqueue},
	{"wrap", l_wrap},
	{NULL, NULL}
};

static const struct  luaL_Reg l_kqueue_meta[] = {
	{"__close", l_close},
	{"__gc", l_gc},
	{"close", l_close},
	{"kevent", l_kevent},
	{"fileno", l_fileno},
	{NULL, NULL}
};

int
luaopen_sys_event(lua_State *L)
{
	luaL_newmetatable(L, KQUEUE_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_kqueue_meta, 0);

	luaL_newlib(L, l_kqueue_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(EV_ADD);
	DEFINE(EV_DELETE);
	DEFINE(EV_ENABLE);
	DEFINE(EV_DISABLE);
	DEFINE(EV_FORCEONESHOT);
#ifdef EV_KEEPUDATA
	DEFINE(EV_KEEPUDATA);
#endif

	DEFINE(EV_ONESHOT);
	DEFINE(EV_CLEAR);
	DEFINE(EV_RECEIPT);
	DEFINE(EV_DISPATCH);

	DEFINE(EV_EOF);
	DEFINE(EV_ERROR);

	DEFINE(EVFILT_READ);
	DEFINE(EVFILT_WRITE);
	DEFINE(EVFILT_AIO);
	DEFINE(EVFILT_VNODE);
	DEFINE(EVFILT_PROC);
	DEFINE(EVFILT_SIGNAL);
	DEFINE(EVFILT_TIMER);
	DEFINE(EVFILT_PROCDESC);
	DEFINE(EVFILT_FS);
	DEFINE(EVFILT_LIO);
	DEFINE(EVFILT_USER);
	DEFINE(EVFILT_SENDFILE);
	DEFINE(EVFILT_EMPTY);
#ifdef EVFILT_JAIL
	DEFINE(EVFILT_JAIL);
#endif
#ifdef EVFILT_JAILDESC
	DEFINE(EVFILT_JAILDESC);
#endif

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
#ifdef NOTE_JAIL_CHILD
	DEFINE(NOTE_JAIL_CHILD);
#endif
#ifdef NOTE_JAIL_SET
	DEFINE(NOTE_JAIL_SET);
#endif
#ifdef NOTE_JAIL_ATTACH
	DEFINE(NOTE_JAIL_ATTACH);
#endif
#ifdef NOTE_JAIL_REMOVE
	DEFINE(NOTE_JAIL_REMOVE);
#endif
#ifdef NOTE_JAIL_MULTI
	DEFINE(NOTE_JAIL_MULTI);
#endif
#ifdef NOTE_JAIL_CTRLMASK
	DEFINE(NOTE_JAIL_CTRLMASK);
#endif
	DEFINE(NOTE_SECONDS);
	DEFINE(NOTE_MSECONDS);
	DEFINE(NOTE_USECONDS);
	DEFINE(NOTE_NSECONDS);
	DEFINE(NOTE_ABSTIME);

	DEFINE(KQUEUE_CLOEXEC);
#ifdef KQUEUE_CPONFORK
	DEFINE(KQUEUE_CPONFORK);
#endif
#undef DEFINE
	return (1);
}
