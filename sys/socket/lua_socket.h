/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

static inline void
pushaddr(lua_State *L, const struct sockaddr *addr)
{
	lua_createtable(L, 0, 2);
	lua_pushinteger(L, addr->sa_family);
	lua_setfield(L, -2, "family");
	lua_pushlstring(L, addr->sa_data,
	    addr->sa_len - offsetof(struct sockaddr, sa_data));
	lua_setfield(L, -2, "data");
}

static inline void
checkaddr(lua_State *L, int idx, struct sockaddr_storage *ss)
{
	struct sockaddr *addr = (struct sockaddr *)ss;
	const char *data;
	size_t datalen;

	memset(ss, 0, sizeof(*ss));
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "family");
	luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid address family");
	addr->sa_family = lua_tointeger(L, -1);
	lua_getfield(L, idx, "data");
	luaL_argcheck(L, lua_isstring(L, -1), idx, "invalid address data");
	data = lua_tolstring(L, -1, &datalen);
	memcpy(addr->sa_data, data, datalen);
	addr->sa_len = datalen + offsetof(struct sockaddr, sa_data);
	lua_pop(L, 2);
}
