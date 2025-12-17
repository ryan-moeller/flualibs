/*
 * Copyright (c) 2023-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/module.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

int luaopen_sys_module(lua_State *);

static int
l_modnext(lua_State *L)
{
	int modid, nextid;

	modid = luaL_optinteger(L, 1, 0);

	if ((nextid = modnext(modid)) == -1) {
		return (fatal(L, "modnext", errno));
	}
	if (nextid == 0) {
		return (0);
	}
	lua_pushinteger(L, nextid);
	return (1);
}

static int
l_modfnext(lua_State *L)
{
	int modid, nextid;

	modid = luaL_checkinteger(L, 1);

	if ((nextid = modfnext(modid)) == -1) {
		return (fatal(L, "modfnext", errno));
	}
	if (nextid == 0) {
		return (0);
	}
	lua_pushinteger(L, nextid);
	return (1);
}

static int
l_modstat(lua_State *L)
{
	struct module_stat stat;
	int modid;

	modid = luaL_checkinteger(L, 1);

	stat.version = sizeof(stat);
	if (modstat(modid, &stat) == -1) {
		return (fail(L, errno));
	}
	lua_newtable(L);
	lua_pushstring(L, stat.name);
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, stat.refs);
	lua_setfield(L, -2, "refs");
	lua_pushinteger(L, stat.id);
	lua_setfield(L, -2, "id");
	lua_pushinteger(L, stat.data.intval);
	lua_setfield(L, -2, "data");
	return (1);
}

static int
l_modfind(lua_State *L)
{
	const char *modname;
	int modid;

	modname = luaL_checkstring(L, 1);

	if ((modid = modfind(modname)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, modid);
	return (1);
}

static const struct luaL_Reg l_module_funcs[] = {
	{"modnext", l_modnext},
	{"modfnext", l_modfnext},
	{"modstat", l_modstat},
	{"modfind", l_modfind},
	{NULL, NULL}
};

int
luaopen_sys_module(lua_State *L)
{
	luaL_newlib(L, l_module_funcs);
	return (1);
}
