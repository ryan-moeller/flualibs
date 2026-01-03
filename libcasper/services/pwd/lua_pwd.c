/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>

#include <libcasper.h>
#include <casper/cap_pwd.h>

#include <lua.h>
#include <lauxlib.h>

#include "libcasper/libcasper/lua_casper.h"
#include "pwd/lua_pwd.h"
#include "utils.h"

int luaopen_casper_pwd(lua_State *);

static int
l_cap_getpwent(lua_State *L)
{
	struct passwd pwd;
	struct passwd *result;
	cap_channel_t *chan;
	char *buffer;
	size_t bufsize;
	int error;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	bufsize = PWD_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = cap_getpwent_r(chan, &pwd, buffer, bufsize, &result))
	    != 0) {
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
l_cap_getpwnam(lua_State *L)
{
	struct passwd pwd;
	struct passwd *result;
	cap_channel_t *chan;
	const char *name;
	char *buffer;
	size_t bufsize;
	int error;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	name = luaL_checkstring(L, 2);

	bufsize = PWD_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = cap_getpwnam_r(chan, name, &pwd, buffer, bufsize, &result))
	    != 0) {
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
l_cap_getpwuid(lua_State *L)
{
	struct passwd pwd;
	struct passwd *result;
	cap_channel_t *chan;
	char *buffer;
	size_t bufsize;
	uid_t uid;
	int error;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	uid = luaL_checkinteger(L, 2);

	bufsize = PWD_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = cap_getpwuid_r(chan, uid, &pwd, buffer, bufsize, &result))
	    != 0) {
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
l_cap_setpassent(lua_State *L)
{
	cap_channel_t *chan;
	int stayopen;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	luaL_checktype(L, 2, LUA_TBOOLEAN);
	stayopen = lua_toboolean(L, 2);

	if (cap_setpassent(chan, stayopen) == 0) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_setpwent(lua_State *L)
{
	cap_channel_t *chan;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	cap_setpwent(chan);
	return (0);
}

static int
l_cap_endpwent(lua_State *L)
{
	cap_channel_t *chan;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	cap_endpwent(chan);
	return (0);
}

static int
l_cap_pwd_limit_cmds(lua_State *L)
{
	cap_channel_t *chan;
	const char **cmds;
	size_t ncmds;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);

	ncmds = luaL_len(L, 2);
	if ((cmds = malloc(ncmds * sizeof(*cmds))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (size_t i = 0; i < ncmds; i++) {
		if (lua_geti(L, 2, i + 1) != LUA_TSTRING) {
			free(cmds);
			return (luaL_argerror(L, 2, "expected strings"));
		}
		cmds[i] = lua_tostring(L, -1);
	}
	if (cap_pwd_limit_cmds(chan, cmds, ncmds) == -1) {
		int error = errno;

		free(cmds);
		return (fail(L, error));
	}
	free(cmds);
	return (success(L));
}

static int
l_cap_pwd_limit_fields(lua_State *L)
{
	cap_channel_t *chan;
	const char **fields;
	size_t nfields;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);

	nfields = luaL_len(L, 2);
	if ((fields = malloc(nfields * sizeof(*fields))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (size_t i = 0; i < nfields; i++) {
		if (lua_geti(L, 2, i + 1) != LUA_TSTRING) {
			free(fields);
			return (luaL_argerror(L, 2, "expected strings"));
		}
		fields[i] = lua_tostring(L, -1);
	}
	if (cap_pwd_limit_fields(chan, fields, nfields) == -1) {
		int error = errno;

		free(fields);
		return (fail(L, error));
	}
	free(fields);
	return (success(L));
}

static int
l_cap_pwd_limit_users(lua_State *L)
{
	cap_channel_t *chan;
	const char **users;
	uid_t *uids;
	size_t n, nusers, nuids;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);

	/* Make both arrays big enough for the whole list to keep it simple. */
	n = luaL_len(L, 2);
	if ((users = malloc(n * sizeof(*users))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((uids = malloc(n * sizeof(*uids))) == NULL) {
		free(users);
		return (fatal(L, "malloc", ENOMEM));
	}
	for (size_t i = nusers = nuids = 0; i < n; i++) {
		if (lua_geti(L, 2, i + 1) == LUA_TSTRING) {
			users[nusers++] = lua_tostring(L, -1);
		} else if (lua_isinteger(L, -1)) {
			uids[nuids++] = lua_tointeger(L, -1);
		} else {
			free(uids);
			free(users);
			return (luaL_argerror(L, 2,
			    "expected strings or integers"));
		}
	}
	if (cap_pwd_limit_users(chan, users, nusers, uids, nuids) == -1) {
		int error = errno;

		free(uids);
		free(users);
		return (fail(L, error));
	}
	free(uids);
	free(users);
	return (success(L));
}

static const struct luaL_Reg l_pwd_funcs[] = {
	{"getpwent", l_cap_getpwent},
	{"getpwnam", l_cap_getpwnam},
	{"getpwuid", l_cap_getpwuid},
	{"setpassent", l_cap_setpassent},
	{"setpwent", l_cap_setpwent},
	{"endpwent", l_cap_endpwent},
	{"limit_cmds", l_cap_pwd_limit_cmds},
	{"limit_fields", l_cap_pwd_limit_fields},
	{"limit_users", l_cap_pwd_limit_users},
	{NULL, NULL}
};

int
luaopen_casper_pwd(lua_State *L)
{
	luaL_newlib(L, l_pwd_funcs);
	return (1);
}
