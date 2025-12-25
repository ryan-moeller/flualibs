/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <fetch.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"

int luaopen_fetch(lua_State *);

static int
closestream(lua_State *L)
{
	luaL_Stream *stream;
	int res;

	stream = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	res = fclose(stream->f);
	return (luaL_fileresult(L, res == 0, NULL));;
}

static void
newstream(lua_State *L, FILE *f)
{
	luaL_Stream *stream;

	stream = lua_newuserdatauv(L, sizeof(*stream), 0);
	luaL_setmetatable(L, LUA_FILEHANDLE);
	stream->f = f;
	stream->closef = closestream;
}

static int
fetcherr(lua_State *L)
{
	luaL_pushfail(L);
	lua_pushstring(L, fetchLastErrString);
	lua_pushinteger(L, fetchLastErrCode);
	return (3);
}

static int
l_fetch_get(lua_State *L)
{
	const char *URL, *flags;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	f = fetchGetURL(URL, flags);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	return (1);
}

static int
l_fetch_put(lua_State *L)
{
	const char *URL, *flags;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	f = fetchPutURL(URL, flags);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	return (1);
}

static void
newurlstat(lua_State *L, struct url_stat *stat)
{
	lua_newtable(L);
	lua_pushstring(L, "size");
	lua_pushinteger(L, stat->size);
	lua_rawset(L, -3);
	lua_pushstring(L, "atime");
	lua_pushinteger(L, stat->atime);
	lua_rawset(L, -3);
	lua_pushstring(L, "mtime");
	lua_pushinteger(L, stat->mtime);
	lua_rawset(L, -3);
}

static int
l_fetch_xget(lua_State *L)
{
	struct url_stat stat;
	const char *URL, *flags;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	f = fetchXGetURL(URL, &stat, flags);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	newurlstat(L, &stat);
	return (2);
}

static int
l_fetch_stat(lua_State *L)
{
	struct url_stat stat;
	const char *URL, *flags;
	int res;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	res = fetchStatURL(URL, &stat, flags);
	if (res == -1) {
		return (fetcherr(L));
	}
	newurlstat(L, &stat);
	return (1);
}

static void
newentlist(lua_State *L, struct url_ent *ents)
{
	lua_newtable(L);
	for (int i = 0; ents[i].name[0] != '\0'; i++) {
		lua_newtable(L);
		lua_pushstring(L, "name");
		lua_pushstring(L, ents[i].name);
		lua_rawset(L, -3);
		lua_pushstring(L, "stat");
		newurlstat(L, &ents[i].stat);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, i + 1);
	}
}

static int
l_fetch_list(lua_State *L)
{
	const char *URL, *flags;
	struct url_ent *ents;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	ents = fetchListURL(URL, flags);
	if (ents == NULL) {
		return (fetcherr(L));
	}
	newentlist(L, ents);
	free(ents);
	return (1);
}

static int
l_fetch_request(lua_State *L)
{
	const char *URL, *method, *flags, *content_type, *body;
	struct url *u;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	method = luaL_checkstring(L, 2);
	flags = luaL_optstring(L, 3, NULL);
	content_type = luaL_optstring(L, 4, NULL);
	body = luaL_optstring(L, 5, NULL);
	u = fetchParseURL(URL);
	if (u == NULL) {
		return (fetcherr(L));
	}
	f = fetchReqHTTP(u, method, flags, content_type, body);
	fetchFreeURL(u);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	return (1);
}

static const struct luaL_Reg l_fetch_funcs[] = {
	{"get", l_fetch_get},
	{"put", l_fetch_put},
	{"xget", l_fetch_xget},
	{"stat", l_fetch_stat},
	{"list", l_fetch_list},
	{"request", l_fetch_request},
	{NULL, NULL}
};

int
luaopen_fetch(lua_State *L)
{
	luaL_newlib(L, l_fetch_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, FETCH_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(ABORT);
	DEFINE(AUTH);
	DEFINE(DOWN);
	DEFINE(EXISTS);
	DEFINE(FULL);
	DEFINE(INFO);
	DEFINE(MEMORY);
	DEFINE(MOVED);
	DEFINE(NETWORK);
	DEFINE(OK);
	DEFINE(PROTO);
	DEFINE(RESOLV);
	DEFINE(SERVER);
	DEFINE(TEMP);
	DEFINE(TIMEOUT);
	DEFINE(UNAVAIL);
	DEFINE(UNKNOWN);
	DEFINE(URL);
#undef DEFINE
	return (1);
}
