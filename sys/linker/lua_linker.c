/*
 * Copyright (c) 2023-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/linker.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

int luaopen_sys_linker(lua_State *);

static int
l_kldload(lua_State *L)
{
	const char *file;
	int fileid;

	file = luaL_checkstring(L, 1);

	if ((fileid = kldload(file)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fileid);
	return (1);
}

static int
l_kldunload(lua_State *L)
{
	int fileid, flags;

	fileid = luaL_checkinteger(L, 1);
	flags = luaL_optinteger(L, 2, LINKER_UNLOAD_NORMAL);

	if (kldunloadf(fileid, flags) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_kldfind(lua_State *L)
{
	const char *file;
	int fileid;

	file = luaL_checkstring(L, 1);

	if ((fileid = kldfind(file)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fileid);
	return (1);
}

static int
l_kldnext(lua_State *L)
{
	int fileid, nextid;

	fileid = luaL_optinteger(L, 1, 0);

	if ((nextid = kldnext(fileid)) == -1) {
		return (fatal(L, "kldnext", errno));
	}
	if (nextid == 0) {
		return (0);
	}
	lua_pushinteger(L, nextid);
	return (1);
}

static int
l_kldstat(lua_State *L)
{
	struct kld_file_stat stat;
	int fileid;

	fileid = luaL_checkinteger(L, 1);

	stat.version = sizeof(stat);
	if (kldstat(fileid, &stat) == -1) {
		return (fail(L, errno));
	}
	lua_newtable(L);
	lua_pushstring(L, stat.name);
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, stat.refs);
	lua_setfield(L, -2, "refs");
	lua_pushinteger(L, stat.id);
	lua_setfield(L, -2, "id");
	lua_pushlightuserdata(L, stat.address);
	lua_setfield(L, -2, "address");
	lua_pushinteger(L, stat.size);
	lua_setfield(L, -2, "size");
	lua_pushstring(L, stat.pathname);
	lua_setfield(L, -2, "pathname");
	return (1);
}

static int
l_kldfirstmod(lua_State *L)
{
	int fileid, modid;

	fileid = luaL_checkinteger(L, 1);

	if ((modid = kldfirstmod(fileid)) == -1) {
		return (fail(L, errno));
	}
	if (modid == 0) {
		 return (0);
	}
	lua_pushinteger(L, modid);
	return (1);
}

/* TODO: kldsym */

static const struct luaL_Reg l_linker_funcs[] = {
	{"kldload", l_kldload},
	{"kldunload", l_kldunload},
	{"kldfind", l_kldfind},
	{"kldnext", l_kldnext},
	{"kldstat", l_kldstat},
	{"kldfirstmod", l_kldfirstmod},
	/*{"kldsym", l_kldsym},*/
	{NULL, NULL}
};

int
luaopen_sys_linker(lua_State *L)
{
	luaL_newlib(L, l_linker_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, LINKER_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(UNLOAD_NORMAL);
	DEFINE(UNLOAD_FORCE);
#undef DEFINE
	return (1);
}
