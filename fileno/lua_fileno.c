/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

int luaopen_fileno(lua_State *);

static int
l_fileno(lua_State *L)
{
	luaL_Stream *stream;
	int fd;

	stream = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	lua_assert(stream->f != NULL);
	fd = fileno(stream->f);
	lua_pushinteger(L, fd);
	return (1);
}

int
luaopen_fileno(lua_State *L)
{
	luaL_getmetatable(L, LUA_FILEHANDLE);
	lua_getfield(L, -1, "__index");
	lua_pushcfunction(L, l_fileno);
	lua_setfield(L, -2, "fileno");
	return (0);
}
