/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <errno.h>
#include <pathconv.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

int luaopen_pathconv(lua_State *);

static int
l_abs2rel(lua_State *L)
{
	char result[MAXPATHLEN];
	const char *path, *base;

	path = luaL_checkstring(L, 1);
	base = luaL_checkstring(L, 2);

	if (abs2rel(path, base, result, sizeof(result)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_rel2abs(lua_State *L)
{
	char result[MAXPATHLEN];
	const char *path, *base;

	path = luaL_checkstring(L, 1);
	base = luaL_checkstring(L, 2);

	if (rel2abs(path, base, result, sizeof(result)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, result);
	return (1);
}

static const struct luaL_Reg l_pathconv_funcs[] = {
	{"abs2rel", l_abs2rel},
	{"rel2abs", l_rel2abs},
	{NULL, NULL}
};

int
luaopen_pathconv(lua_State *L)
{
	luaL_newlib(L, l_pathconv_funcs);
	return (1);
}
