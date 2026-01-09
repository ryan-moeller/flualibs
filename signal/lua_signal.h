/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/signal.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

static inline void
checksigevent(lua_State *L, int idx, struct sigevent *sig)
{
	memset(sig, 0, sizeof(*sig));

	luaL_checktype(L, idx, LUA_TTABLE);
#define INTFIELD(name) ({ \
	lua_getfield(L, idx, #name); \
	sig->sigev_ ## name = luaL_checkinteger(L, -1); \
	lua_pop(L, 1); \
})
	INTFIELD(notify);
	INTFIELD(notify_kqueue);
	INTFIELD(notify_kevent_flags);
	INTFIELD(notify_thread_id);
	/* TODO: notify_function and notify_attributes */
	INTFIELD(signo);
	if (lua_getfield(L, idx, "value") == LUA_TLIGHTUSERDATA) {
		sig->sigev_value.sival_ptr = lua_touserdata(L, -1);
	} else {
		sig->sigev_value.sival_int = luaL_checkinteger(L, -1);
	}
	lua_pop(L, 1);
#undef INTFIELD
}

static inline void
pushsigevent(lua_State *L, struct sigevent *sig)
{
	lua_newtable(L);
#define INTFIELD(name) ({ \
	lua_pushinteger(L, sig->sigev_ ## name); \
	lua_setfield(L, -2, #name); \
})
	INTFIELD(notify);
	/* XXX: these fields are aliases */
	INTFIELD(notify_kqueue);
	INTFIELD(notify_kevent_flags);
	INTFIELD(notify_thread_id);
	/* TODO: notify_function and notify_attributes */
	INTFIELD(signo);
	/* XXX: this field is a union */
	lua_createtable(L, 0, 2);
	lua_pushinteger(L, sig->sigev_value.sival_int);
	lua_setfield(L, -2, "int");
	lua_pushlightuserdata(L, sig->sigev_value.sival_ptr);
	lua_setfield(L, -2, "ptr");
	lua_setfield(L, -2, "value");
#undef INTFIELD
}
