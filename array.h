/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

enum arrayuv {
	ARRAY_COUNT = 3,
	ARRAY_CLEANUP = 4,
	ARRAY_NUV = 4,
};

static inline void
setarray(lua_State *L, void *data, size_t count, const char *metatable,
    lua_CFunction cleanup)
{
	setcookie(L, -1, data);
	lua_pushinteger(L, count);
	lua_setiuservalue(L, -2, ARRAY_COUNT);
	if (cleanup != NULL) {
		lua_pushcfunction(L, cleanup);
		lua_setiuservalue(L, -2, ARRAY_CLEANUP);
	}
	luaL_setmetatable(L, metatable);
}

/* Elements live in external memory; cleanup, if given, releases it. */
static inline void
newarrayext(lua_State *L, void *data, size_t count, const char *metatable,
    lua_CFunction cleanup)
{
	lua_newuserdatauv(L, 0, ARRAY_NUV);
	setarray(L, data, count, metatable, cleanup);
}

/* Elements live in the userdata. */
static inline void *
newarray(lua_State *L, size_t count, size_t size, const char *metatable)
{
	void *data;

	data = lua_newuserdatauv(L, count * size, ARRAY_NUV);
	setarray(L, data, count, metatable, NULL);
	return (data);
}

static inline lua_Integer
arraycount(lua_State *L, int idx)
{
	lua_Integer count;

	lua_getiuservalue(L, idx, ARRAY_COUNT);
	count = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return (count);
}

/* Upvalues: element size, element metatable name, array metatable name. */
static inline int
l_array_index(lua_State *L)
{
	char *array;
	lua_Integer i;
	size_t size;

	array = checkcookienull(L, 1, lua_tostring(L, lua_upvalueindex(3)));
	i = luaL_checkinteger(L, 2);
	size = lua_tointeger(L, lua_upvalueindex(1));

	if (i < 1 || i > arraycount(L, 1)) {
		return (0);
	}
	return (newref(L, 1, array + (i - 1) * size,
	    lua_tostring(L, lua_upvalueindex(2))));
}

static inline int
l_array_len(lua_State *L)
{
	checkcookieuv(L, 1, lua_tostring(L, lua_upvalueindex(3)));
	lua_pop(L, 1);

	lua_pushinteger(L, arraycount(L, 1));
	return (1);
}

static inline int
l_array_gc(lua_State *L)
{
	luaL_checkudata(L, 1, lua_tostring(L, lua_upvalueindex(3)));

	/* Clear first so the cleanup function only ever runs once. */
	if (lua_getiuservalue(L, 1, ARRAY_CLEANUP) != LUA_TNIL) {
		lua_pushnil(L);
		lua_setiuservalue(L, 1, ARRAY_CLEANUP);
		lua_pushvalue(L, 1);
		lua_call(L, 1, 0);
	}
	return (0);
}

/*
 * Register an array type, named name, whose elements are size bytes each and
 * are exposed as objects with the elemname metatable.
 */
static inline void
newarraymeta(lua_State *L, const char *name, size_t size, const char *elemname)
{
	static const struct luaL_Reg meta[] = {
		{"__index", l_array_index},
		{"__len", l_array_len},
		{"__gc", l_array_gc},
		{NULL, NULL}
	};

	luaL_newmetatable(L, name);
	lua_pushinteger(L, size);
	lua_pushstring(L, elemname);
	lua_pushstring(L, name);
	luaL_setfuncs(L, meta, 3);
	lua_pop(L, 1);
}
