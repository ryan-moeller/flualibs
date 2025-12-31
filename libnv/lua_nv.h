/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/nv.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define NVLIST_METATABLE "nvlist_t"
#define CONST_NVLIST_METATABLE "const nvlist_t"

static inline nvlist_t *
getnvlist(lua_State *L, int idx, const char *metatable)
{
	nvlist_t *nvl = NULL;

	if (lua_getmetatable(L, idx)) {
		luaL_getmetatable(L, metatable);
		if (lua_rawequal(L, -1, -2)) {
			nvl = getcookie(L, idx);
			lua_pop(L, 3);
		} else {
			lua_pop(L, 2);
		}
	}
	return (nvl);
}

static inline nvlist_t *
checknvlist(lua_State *L, int idx)
{
	nvlist_t *nvl;

	nvl = getnvlist(L, idx, NVLIST_METATABLE);
	luaL_argexpected(L, nvl != NULL, idx, NVLIST_METATABLE);
	luaL_argcheck(L, nvlist_error(nvl) == 0, idx, "nvlist has error");

	return (nvl);
}

static inline const nvlist_t *
checkconstnvlist(lua_State *L, int idx)
{
	const nvlist_t *nvl;

	nvl = getnvlist(L, idx, CONST_NVLIST_METATABLE);
	luaL_argexpected(L, nvl != NULL, idx, CONST_NVLIST_METATABLE);
	luaL_argcheck(L, nvlist_error(nvl) == 0, idx, "nvlist has error");

	return (nvl);
}

static inline nvlist_t *
getanynvlist(lua_State *L, int idx)
{
	nvlist_t *nvl = NULL;

	if (lua_getmetatable(L, idx)) {
		luaL_getmetatable(L, NVLIST_METATABLE);
		luaL_getmetatable(L, CONST_NVLIST_METATABLE);
		if (lua_rawequal(L, -1, -2) || lua_rawequal(L, -1, -3)) {
			nvl = getcookie(L, idx);
			lua_pop(L, 4);
		} else {
			lua_pop(L, 3);
		}
	}
	return (nvl);
}

static inline nvlist_t *
checkanynvlist(lua_State *L, int idx)
{
	nvlist_t *nvl;

	nvl = getanynvlist(L, idx);
	luaL_argexpected(L, nvl != NULL, idx, NVLIST_METATABLE);
	luaL_argcheck(L, nvlist_error(nvl) == 0, idx, "nvlist has error");

	return (nvl);
}
