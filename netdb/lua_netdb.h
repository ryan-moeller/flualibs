/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <netdb.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/socket/lua_socket.h"

static inline int
gai_fail(lua_State *L, int error)
{
	luaL_pushfail(L);
	lua_pushstring(L, gai_strerror(error));
	lua_pushinteger(L, error);
	return (3);
}

static inline void
checkai(lua_State *L, int idx, struct addrinfo *ai)
{
	memset(ai, 0, sizeof(*ai));
	luaL_checktype(L, idx, LUA_TTABLE);
	if (lua_getfield(L, idx, "flags") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid flags");
		ai->ai_flags = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "family") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid family");
		ai->ai_family = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "socktype") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid socktype");
		ai->ai_socktype = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "protocol") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid protocol");
		ai->ai_protocol = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	/* Other fields are not relevant to getaddrinfo. */
}

static inline void
pushai(lua_State *L, const struct addrinfo *ai)
{
	lua_newtable(L);
	lua_pushinteger(L, ai->ai_flags);
	lua_setfield(L, -2, "flags");
	lua_pushinteger(L, ai->ai_family);
	lua_setfield(L, -2, "family");
	lua_pushinteger(L, ai->ai_socktype);
	lua_setfield(L, -2, "socktype");
	lua_pushinteger(L, ai->ai_protocol);
	lua_setfield(L, -2, "protocol");
	pushaddr(L, ai->ai_addr);
	lua_setfield(L, -2, "addr");
}
