/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <grp.h>
#include <stdlib.h>

#include <libcasper.h>
#include <casper/cap_grp.h>

#include <lua.h>
#include <lauxlib.h>

#include "grp/lua_grp.h"
#include "libcasper/libcasper/lua_casper.h"
#include "utils.h"

int luaopen_casper_grp(lua_State *);

static int
l_cap_getgrent(lua_State *L)
{
	struct group grp;
	struct group *result;
	cap_channel_t *chan;
	char *buffer;
	size_t bufsize;
	int error;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	bufsize = GRP_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = cap_getgrent_r(chan, &grp, buffer, bufsize, &result))
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
	pushgroup(L, result);
	free(buffer);
	return (1);
}

static int
l_cap_getgrnam(lua_State *L)
{
	struct group grp;
	struct group *result;
	cap_channel_t *chan;
	const char *name;
	char *buffer;
	size_t bufsize;
	int error;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	name = luaL_checkstring(L, 2);

	bufsize = GRP_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = cap_getgrnam_r(chan, name, &grp, buffer, bufsize, &result))
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
	pushgroup(L, result);
	free(buffer);
	return (1);
}

static int
l_cap_getgrgid(lua_State *L)
{
	struct group grp;
	struct group *result;
	cap_channel_t *chan;
	char *buffer;
	size_t bufsize;
	gid_t gid;
	int error;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	gid = luaL_checkinteger(L, 2);

	bufsize = GRP_INITIAL_BUFSIZE;
retry:
	if ((buffer = malloc(bufsize)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((error = cap_getgrgid_r(chan, gid, &grp, buffer, bufsize, &result))
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
	pushgroup(L, result);
	free(buffer);
	return (1);
}

static int
l_cap_setgroupent(lua_State *L)
{
	cap_channel_t *chan;
	int stayopen;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	luaL_checktype(L, 2, LUA_TBOOLEAN);
	stayopen = lua_toboolean(L, 2);

	if (cap_setgroupent(chan, stayopen) == 0) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_setgrent(lua_State *L)
{
	cap_channel_t *chan;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	if (cap_setgrent(chan) == 0) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_endgrent(lua_State *L)
{
	cap_channel_t *chan;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	cap_endgrent(chan);
	return (0);
}

static int
l_cap_grp_limit_cmds(lua_State *L)
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
	if (cap_grp_limit_cmds(chan, cmds, ncmds) == -1) {
		int error = errno;

		free(cmds);
		return (fail(L, error));
	}
	free(cmds);
	return (success(L));
}

static int
l_cap_grp_limit_fields(lua_State *L)
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
	if (cap_grp_limit_fields(chan, fields, nfields) == -1) {
		int error = errno;

		free(fields);
		return (fail(L, error));
	}
	free(fields);
	return (success(L));
}

static int
l_cap_grp_limit_groups(lua_State *L)
{
	cap_channel_t *chan;
	const char **groups;
	gid_t *gids;
	size_t n, ngroups, ngids;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);

	/* Make both arrays big enough for the whole list to keep it simple. */
	n = luaL_len(L, 2);
	if ((groups = malloc(n * sizeof(*groups))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((gids = malloc(n * sizeof(*gids))) == NULL) {
		free(groups);
		return (fatal(L, "malloc", ENOMEM));
	}
	for (size_t i = ngroups = ngids = 0; i < n; i++) {
		if (lua_geti(L, 2, i + 1) == LUA_TSTRING) {
			groups[ngroups++] = lua_tostring(L, -1);
		} else if (lua_isinteger(L, -1)) {
			gids[ngids++] = lua_tointeger(L, -1);
		} else {
			free(gids);
			free(groups);
			return (luaL_argerror(L, 2,
			    "expected strings or integers"));
		}
	}
	if (cap_grp_limit_groups(chan, groups, ngroups, gids, ngids) == -1) {
		int error = errno;

		free(gids);
		free(groups);
		return (fail(L, error));
	}
	free(gids);
	free(groups);
	return (success(L));
}

static const struct luaL_Reg l_grp_funcs[] = {
	{"getgrent", l_cap_getgrent},
	{"getgrnam", l_cap_getgrnam},
	{"getgrgid", l_cap_getgrgid},
	{"setgroupent", l_cap_setgroupent},
	{"setgrent", l_cap_setgrent},
	{"endgrent", l_cap_endgrent},
	{"limit_cmds", l_cap_grp_limit_cmds},
	{"limit_fields", l_cap_grp_limit_fields},
	{"limit_groups", l_cap_grp_limit_groups},
	{NULL, NULL}
};

int
luaopen_casper_grp(lua_State *L)
{
	luaL_newlib(L, l_grp_funcs);
	return (1);
}
