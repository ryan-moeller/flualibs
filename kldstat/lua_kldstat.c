/*
 * Copyright (c) 2023-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/linker.h>
#include <sys/module.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

int luaopen_kldstat(lua_State *);

static int
l_kldstat_iter_next(lua_State *L)
{
	struct kld_file_stat stat;
	int nextid;

	nextid = luaL_checkinteger(L, lua_upvalueindex(1));
	if (nextid == 0) {
		return (0);
	}
	stat.version = sizeof(stat);
	if (kldstat(nextid, &stat) == -1) {
		return (fatal(L, "kldstat", errno));
	}
	nextid = kldnext(nextid);
	lua_pushinteger(L, nextid);
	lua_replace(L, lua_upvalueindex(1));

	lua_newtable(L);
	lua_pushstring(L, stat.name);
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, stat.refs);
	lua_setfield(L, -2, "refs");
	lua_pushinteger(L, stat.id);
	lua_setfield(L, -2, "id");
	{
		char *s = NULL;
		if (asprintf(&s, "%p", stat.address) == -1) {
			return (fatal(L, "asprintf", errno));
		}
		lua_pushstring(L, s);
		free(s);
	}
	lua_setfield(L, -2, "address");
	lua_pushinteger(L, stat.size);
	lua_setfield(L, -2, "size");
	lua_pushstring(L, stat.pathname);
	lua_setfield(L, -2, "pathname");
	return (1);
}

static int
l_kldstat(lua_State *L)
{

	lua_pushinteger(L, kldnext(0));
	lua_pushcclosure(L, l_kldstat_iter_next, 1);
	return (1);
}

static int
l_modstat_next(lua_State *L)
{
	struct module_stat stat;
	int nextid;

	nextid = luaL_checkinteger(L, lua_upvalueindex(1));
	if (nextid == 0) {
		return (0);
	}
	stat.version = sizeof(stat);
	if (modstat(nextid, &stat) == -1) {
		return (fatal(L, "modstat", errno));
	}
	nextid = modfnext(nextid);
	lua_pushinteger(L, nextid);
	lua_replace(L, lua_upvalueindex(1));

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
l_modstat(lua_State *L)
{
	int fileid;

	fileid = luaL_checkinteger(L, 1);
	lua_pushinteger(L, kldfirstmod(fileid));
	lua_pushcclosure(L, l_modstat_next, 1);
	return (1);
}

static const struct luaL_Reg l_kldstat_funcs[] = {
	/* Iterate kernel module files, yielding stats for each. */
	{"kldstat", l_kldstat},
	/* Iterate modules in a file given fileid, yielding stats for each. */
	{"modstat", l_modstat},
	{NULL, NULL}
};

int
luaopen_kldstat(lua_State *L)
{
	luaL_newlib(L, l_kldstat_funcs);
	return (1);
}
