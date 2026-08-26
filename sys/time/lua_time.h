/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/time.h>

#include <lua.h>
#include <lauxlib.h>

static inline void
checkbintime(lua_State *L, int idx, struct bintime *t)
{
	int isnum;

	if (lua_istable(L, idx)) {
		lua_rawgeti(L, idx, 1);
		t->sec = lua_tointegerx(L, -1, &isnum);
		if (isnum) {
			lua_rawgeti(L, idx, 2);
			t->frac = (uint64_t)lua_tointegerx(L, -1, &isnum);
		}
		lua_pop(L, isnum ? 2 : 1);
	} else {
		t->sec = (time_t)lua_tointegerx(L, idx, &isnum);
		t->frac = 0;
	}
	luaL_argcheck(L, isnum, idx,
	    "expected integer or pair of integers {sec, frac}");
}

static inline void
pushbintime(lua_State *L, struct bintime *t)
{
	lua_createtable(L, 2, 0);
	lua_pushinteger(L, t->sec);
	lua_rawseti(L, -2, 1);
	lua_pushinteger(L, t->frac);
	lua_rawseti(L, -2, 2);
}
