/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <grp.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "lua_grp.h"
#include "utils.h"

int luaopen_grp(lua_State *);

static int
l_getgrent(lua_State *L)
{
	struct group grp;
	struct group *result;
	char *buffer;
	size_t bufsize;
	int error;

	bufsize = GRP_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = getgrent_r(&grp, buffer, bufsize, &result)) != 0) {
		free(buffer);
		if (error == ERANGE) {
			bufsize *= 2;
			goto retry;
		}
		return (fail(L, error));
	}
	if (result == NULL) {
		free(buffer);
		return (0);
	}
	pushgroup(L, result);
	free(buffer);
	return (1);
}

static int
l_getgrnam(lua_State *L)
{
	struct group grp;
	struct group *result;
	const char *name;
	char *buffer;
	size_t bufsize;
	int error;

	name = luaL_checkstring(L, 1);

	bufsize = GRP_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = getgrnam_r(name, &grp, buffer, bufsize, &result)) != 0) {
		free(buffer);
		if (error == ERANGE) {
			bufsize *= 2;
			goto retry;
		}
		return (fail(L, error));
	}
	if (result == NULL) {
		free(buffer);
		return (0);
	}
	pushgroup(L, result);
	free(buffer);
	return (1);
}

static int
l_getgrgid(lua_State *L)
{
	struct group grp;
	struct group *result;
	char *buffer;
	size_t bufsize;
	gid_t gid;
	int error;

	gid = luaL_checkinteger(L, 1);

	bufsize = GRP_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = getgrgid_r(gid, &grp, buffer, bufsize, &result)) != 0) {
		free(buffer);
		if (error == ERANGE) {
			bufsize *= 2;
			goto retry;
		}
		return (fail(L, error));
	}
	if (result == NULL) {
		free(buffer);
		return (0);
	}
	pushgroup(L, result);
	free(buffer);
	return (1);
}

static int
l_setgroupent(lua_State *L)
{
	int stayopen;

	luaL_checktype(L, 1, LUA_TBOOLEAN);
	stayopen = lua_toboolean(L, 1);

	lua_pushboolean(L, setgroupent(stayopen));
	return (1);
}

static int
l_setgrent(lua_State *L __unused)
{
	setgrent();
	return (0);
}

static int
l_endgrent(lua_State *L __unused)
{
	endgrent();
	return (0);
}

static const struct luaL_Reg l_grp_funcs[] = {
	{"getgrent", l_getgrent},
	{"getgrnam", l_getgrnam},
	{"getgrgid", l_getgrgid},
	{"setgroupent", l_setgroupent},
	{"setgrent", l_setgrent},
	{"endgrent", l_endgrent},
	{NULL, NULL}
};

int
luaopen_grp(lua_State *L)
{
	luaL_newlib(L, l_grp_funcs);
	return (1);
}
