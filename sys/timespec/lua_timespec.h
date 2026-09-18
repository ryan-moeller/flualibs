/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/timespec.h>

#include <lua.h>
#include <lauxlib.h>

static inline void
checktimespec(lua_State *L, int idx, struct timespec *t)
{
	int isnum;

	if (lua_istable(L, idx)) {
		lua_rawgeti(L, idx, 1);
		t->tv_sec = lua_tointegerx(L, -1, &isnum);
		if (isnum) {
			lua_rawgeti(L, idx, 2);
			t->tv_nsec = (uint64_t)lua_tointegerx(L, -1, &isnum);
		}
		lua_pop(L, isnum ? 2 : 1);
	} else {
		t->tv_sec = (time_t)lua_tointegerx(L, idx, &isnum);
		t->tv_nsec = 0;
	}
	luaL_argcheck(L, isnum, idx,
	    "expected integer or pair of integers {sec, nsec}");
}

static inline void
pushtimespec(lua_State *L, struct timespec *t)
{
	lua_createtable(L, 2, 0);
	lua_pushinteger(L, t->tv_sec);
	lua_rawseti(L, -2, 1);
	lua_pushinteger(L, t->tv_nsec);
	lua_rawseti(L, -2, 2);
}

static inline void
checkitimerspec(lua_State *L, int idx, struct itimerspec *it)
{
	int isnum;

	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "value");
	checktimespec(L, -1, &it->it_value);
	lua_getfield(L, idx, "interval");
	checktimespec(L, -1, &it->it_interval);
	lua_pop(L, 2);
}

static inline void
pushitimerspec(lua_State *L, struct itimerspec *it)
{
	lua_createtable(L, 2, 0);
	pushtimespec(L, &it->it_value);
	lua_setfield(L, -2, "value");
	pushtimespec(L, &it->it_interval);
	lua_setfield(L, -2, "interval");
}
