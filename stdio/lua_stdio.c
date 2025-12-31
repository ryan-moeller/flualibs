/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>

int luaopen_stdio(lua_State *);

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

static const struct luaL_Reg l_stdio_funcs[] = {
	{"fileno", l_fileno},
	{NULL, NULL}
};

int
luaopen_stdio(lua_State *L)
{
	luaL_getmetatable(L, LUA_FILEHANDLE);
	lua_getfield(L, -1, "__index");
	lua_pushcfunction(L, l_fileno);
	lua_setfield(L, -2, "fileno");

	luaL_newlib(L, l_stdio_funcs);
	return (1);
}
