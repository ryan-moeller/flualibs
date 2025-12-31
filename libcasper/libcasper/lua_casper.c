/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/nv.h>
#include <libcasper.h>

#include <lua.h>
#include <lauxlib.h>

#include "libnv/lua_nv.h"
#include "lua_casper.h"
#include "utils.h"

int luaopen_casper(lua_State *);

static int
l_cap_init(lua_State *L)
{
	cap_channel_t *chan;

	if ((chan = cap_init()) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, chan, CAP_CHANNEL_METATABLE));
}

static int
l_cap_wrap(lua_State *L)
{
	cap_channel_t *chan;
	int sock, flags;

	/* TODO: Take a LUA_FILEHANDLE and invalidate it to take ownership. */
	sock = luaL_checkinteger(L, 1);
	flags = luaL_optinteger(L, 2, 0);

	if ((chan = cap_wrap(sock, flags)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, chan, CAP_CHANNEL_METATABLE));
}

static int
l_cap_close(lua_State *L)
{
	cap_channel_t *chan;

	if ((chan = checkcookienull(L, 1, CAP_CHANNEL_METATABLE)) == NULL) {
		return (0);
	}
	cap_close(chan);
	setcookie(L, 1, NULL);
	return (0);
}

static int
l_cap_unwrap(lua_State *L)
{
	cap_channel_t *chan;
	int fd, flags;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	setcookie(L, 1, NULL);
	flags = 0;
	lua_pushinteger(L, cap_unwrap(chan, &flags));
	lua_pushinteger(L, flags);
	return (2);
}

static int
l_cap_sock(lua_State *L)
{
	const cap_channel_t *chan;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	lua_pushinteger(L, cap_sock(chan));
	return (1);
}

static int
l_cap_clone(lua_State *L)
{
	const cap_channel_t *chan;
	cap_channel_t *clone;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	if ((clone = cap_clone(chan)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, clone, CAP_CHANNEL_METATABLE));
}

static int
l_cap_limit(lua_State *L)
{
	const cap_channel_t *chan;
	nvlist_t *limits;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	if (lua_isnoneornil(L, 2)) {
		if (cap_limit_get(chan, &limits) == -1) {
			return (fail(L, errno));
		}
		if (limits == NULL) {
			return (0);
		}
		return (new(L, limits, NVLIST_METATABLE));
	}
	limits = checknvlist(L, 2);

	setcookie(L, 2, NULL);
	if (cap_limit_set(chan, limits) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_send(lua_State *L)
{
	const cap_channel_t *chan;
	const nvlist_t *nvl;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	nvl = checkanynvlist(L, 2);

	if (cap_send_nvlist(chan, nvl) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_recv(lua_State *L)
{
	const cap_channel_t *chan;
	nvlist_t *nvl;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	if ((nvl = cap_recv_nvlist(chan)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, nvl, NVLIST_METATABLE));
}

static int
l_cap_xfer(lua_State *L)
{
	const cap_channel_t *chan;
	nvlist_t *tx, *rx;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	tx = checknvlist(L, 2);

	setcookie(L, 2, NULL);
	if ((rx = cap_xfer_nvlist(chan, tx)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, rx, NVLIST_METATABLE));
}

static int
l_cap_service_open(lua_State *L)
{
	const cap_channel_t *chan;
	cap_channel_t *serv;
	const char *name;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	name = luaL_checkstring(L, 2);

	if ((serv = cap_service_open(chan, name)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, serv, CAP_CHANNEL_METATABLE));
}

static const struct luaL_Reg l_casper_funcs[] = {
	{"init", l_cap_init},
	{"wrap", l_cap_wrap},
	{NULL, NULL}
};

static const struct luaL_Reg l_cap_channel_meta[] = {
	{"__close", l_cap_close},
	{"__gc", l_cap_close},
	{"close", l_cap_close},
	{"unwrap", l_cap_unwrap},
	{"sock", l_cap_sock},
	{"clone", l_cap_clone},
	{"limit", l_cap_limit},
	{"send", l_cap_send},
	{"recv", l_cap_recv},
	{"xfer", l_cap_xfer},
	{"service_open", l_cap_service_open},
	{NULL, NULL}
};

int
luaopen_casper(lua_State *L)
{
	/* Ensure the nv module is loaded so we can use its metatables. */
	lua_getglobal(L, "require");
	lua_pushstring(L, "nv");
	lua_call(L, 1, 0);

	luaL_newmetatable(L, CAP_CHANNEL_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_cap_channel_meta, 0);

	luaL_newlib(L, l_casper_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, CASPER_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(NO_UNIQ);
#undef DEFINE
	return (1);
}
