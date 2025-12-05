/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ryan Moeller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <limits.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "luaerror.h"

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

static inline void *
getcookie(lua_State *L, int idx)
{
	void *cookie;

	lua_getiuservalue(L, idx, COOKIE);
	return (lua_touserdata(L, -1));
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

static inline void *
checkcookienull(lua_State *L, int idx, const char *metatable)
{
	void *cookie;

	luaL_checkudata(L, idx, metatable);

	return (getcookie(L, idx));
}

static inline void *
checkcookie(lua_State *L, int idx, const char *metatable)
{
	void *cookie;

	cookie = checkcookienull(L, idx, metatable);
	luaL_argcheck(L, cookie != NULL, idx, "cookie expired");
	return (cookie);
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
