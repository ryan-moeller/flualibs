/*
 * Copyright (c) 2025-2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "luaerror.h"

static inline int
success(lua_State *L)
{
	lua_pushinteger(L, true);
	return (1);
}

static inline int
fail(lua_State *L, int error)
{
	char msg[NL_TEXTMAX];

	strerror_r(error, msg, sizeof(msg));
	luaL_pushfail(L);
	lua_pushstring(L, msg);
	lua_pushinteger(L, error);
	return (3);
}

static inline int __dead2
fatal(lua_State *L, const char *source, int error)
{
	char msg[NL_TEXTMAX];

	strerror_r(error, msg, sizeof(msg));
	luaL_error(L, "%s: %s", source, msg);
}

enum wrapperuv {
	COOKIE = 1,
	REF = 2,
};

static inline void
setcookie(lua_State *L, int idx, void *cookie)
{
	idx = lua_absindex(L, idx);
	lua_pushlightuserdata(L, cookie);
	lua_setiuservalue(L, idx, COOKIE);
}

static inline int
getcookie(lua_State *L, int idx)
{
	return (lua_getiuservalue(L, idx, COOKIE));
}

static inline void
setref(lua_State *L, int idx, int ref)
{
	idx = lua_absindex(L, idx);
	lua_pushvalue(L, ref);
	lua_setiuservalue(L, idx, REF);
}

static inline int
getref(lua_State *L, int idx)
{
	return (lua_getiuservalue(L, idx, REF));
}

static inline int
new(lua_State *L, void *cookie, const char *metatable)
{
	lua_newuserdatauv(L, 0, 1);
	luaL_setmetatable(L, metatable);

	setcookie(L, -1, cookie);
	return (1);
}

/* pointer into another object (at idx) with a ref to keep it alive */
static inline int
newref(lua_State *L, int idx, void *cookie, const char *metatable)
{
	idx = lua_absindex(L, idx);
	lua_newuserdatauv(L, 0, 2);
	luaL_setmetatable(L, metatable);

	setcookie(L, -1, cookie);
	setref(L, -1, idx);
	return (1);
}

static inline void *
checklightuserdata(lua_State *L, int idx)
{
	luaL_checktype(L, idx, LUA_TLIGHTUSERDATA);

	return (lua_touserdata(L, idx));
}

static inline void
checkcookieuv(lua_State *L, int idx, const char *metatable)
{
	luaL_checkudata(L, idx, metatable);
	luaL_argcheck(L, getcookie(L, idx) == LUA_TLIGHTUSERDATA, idx,
	    "invalid cookie");
}

static inline void *
checkcookienull(lua_State *L, int idx, const char *metatable)
{
	void *cookie;

	checkcookieuv(L, idx, metatable);

	cookie = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return (cookie);
}

static inline void *
checkcookie(lua_State *L, int idx, const char *metatable)
{
	void *cookie;

	cookie = checkcookienull(L, idx, metatable);
	luaL_argcheck(L, cookie != NULL, idx, "cookie expired");
	return (cookie);
}

static inline int
checkfd(lua_State *L, int idx)
{
	luaL_Stream *s;
	int fd;

	if (lua_isinteger(L, idx)) {
		return (lua_tointeger(L, idx));
	}
	s = luaL_checkudata(L, idx, LUA_FILEHANDLE);
	luaL_argcheck(L, s->f != NULL, idx, "invalid file handle (closed)");

	if ((fd = fileno(s->f)) == -1) {
		fatal(L, "fileno", errno);
	}
	return (fd);
}

static inline void
tpush(lua_State *L, int idx)
{
	int len;

	len = luaL_len(L, idx);
	lua_rawseti(L, idx, len + 1);
}

static inline void
tpop(lua_State *L, int idx)
{
	int len;

	idx = lua_absindex(L, idx);
	len = luaL_len(L, idx);
	lua_rawgeti(L, idx, len);
	lua_pushnil(L);
	lua_rawseti(L, idx, len);
}

static inline void
tpack(lua_State *L, int n)
{
	int idx;

	lua_createtable(L, n, 0);
	if (n > 0) {
		idx = lua_gettop(L) - n;
		lua_insert(L, idx);
		for (int i = 1; i <= n; i++) {
			lua_pushvalue(L, idx + i);
			lua_rawseti(L, idx, i);
		}
		lua_settop(L, idx);
	}
}

static inline int
tunpack(lua_State *L, int idx)
{
	int len;

	idx = lua_absindex(L, idx);
	len = luaL_len(L, idx);
	for (int i = 1; i <= len; i++) {
		lua_rawgeti(L, idx, i);
	}
	lua_remove(L, idx);
	return (len);
}
