/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "lua_pwd.h"
#include "utils.h"

int luaopen_pwd(lua_State *);

static int
l_getpwent(lua_State *L)
{
	struct passwd pwd;
	struct passwd *result;
	char *buffer;
	size_t bufsize;
	int error;

	bufsize = PWD_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = getpwent_r(&pwd, buffer, bufsize, &result)) != 0) {
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
	pushpasswd(L, result);
	free(buffer);
	return (1);
}

static int
l_getpwnam(lua_State *L)
{
	struct passwd pwd;
	struct passwd *result;
	const char *name;
	char *buffer;
	size_t bufsize;
	int error;

	name = luaL_checkstring(L, 1);

	bufsize = PWD_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = getpwnam_r(name, &pwd, buffer, bufsize, &result)) != 0) {
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
	pushpasswd(L, result);
	free(buffer);
	return (1);
}

static int
l_getpwuid(lua_State *L)
{
	struct passwd pwd;
	struct passwd *result;
	char *buffer;
	size_t bufsize;
	uid_t uid;
	int error;

	uid = luaL_checkinteger(L, 1);

	bufsize = PWD_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = getpwuid_r(uid, &pwd, buffer, bufsize, &result)) != 0) {
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
	pushpasswd(L, result);
	free(buffer);
	return (1);
}

static int
l_setpassent(lua_State *L)
{
	int stayopen;

	luaL_checktype(L, 1, LUA_TBOOLEAN);
	stayopen = lua_toboolean(L, 1);

	lua_pushboolean(L, setpassent(stayopen));
	return (1);
}

static int
l_setpwent(lua_State *L __unused)
{
	setpwent();
	return (0);
}

static int
l_endpwent(lua_State *L __unused)
{
	endpwent();
	return (0);
}

static const struct luaL_Reg l_pwd_funcs[] = {
	{"getpwent", l_getpwent},
	{"getpwnam", l_getpwnam},
	{"getpwuid", l_getpwuid},
	{"setpassent", l_setpassent},
	{"setpwent", l_setpwent},
	{"endpwent", l_endpwent},
	{NULL, NULL}
};

int
luaopen_pwd(lua_State *L)
{
	luaL_newlib(L, l_pwd_funcs);
	return (1);
}
