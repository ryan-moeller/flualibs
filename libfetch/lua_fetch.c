/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ryan Moeller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <fetch.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

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
	stream->f = f;
	stream->closef = closestream;
	luaL_setmetatable(L, LUA_FILEHANDLE);
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
	flags = luaL_checkstring(L, 3);
	content_type = luaL_checkstring(L, 4);
	body = luaL_checkstring(L, 5);
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
#define SETCONST(ident) ({ \
	lua_pushinteger(L, FETCH_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(ABORT);
	SETCONST(AUTH);
	SETCONST(DOWN);
	SETCONST(EXISTS);
	SETCONST(FULL);
	SETCONST(INFO);
	SETCONST(MEMORY);
	SETCONST(MOVED);
	SETCONST(NETWORK);
	SETCONST(OK);
	SETCONST(PROTO);
	SETCONST(RESOLV);
	SETCONST(SERVER);
	SETCONST(TEMP);
	SETCONST(TIMEOUT);
	SETCONST(UNAVAIL);
	SETCONST(UNKNOWN);
	SETCONST(URL);
#undef SETCONST
	return (1);
}
