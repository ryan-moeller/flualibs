/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifer: BSD-2-Clause
 */

#include <libcasper.h>
#include <casper/cap_syslog.h>

#include <lua.h>
#include <lauxlib.h>

#include "libcasper/libcasper/lua_casper.h"
#include "utils.h"

int luaopen_casper_syslog(lua_State *);

static int
l_cap_openlog(lua_State *L)
{
	cap_channel_t *chan;
	const char *ident;
	int logopt, facility;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	lua_pop(L, 1);
	ident = luaL_checkstring(L, 2);
	logopt = luaL_checkinteger(L, 3);
	facility = luaL_optinteger(L, 4, 0);

	cap_openlog(chan, ident, logopt, facility);
	return (0);
}

static int
l_cap_setlogmask(lua_State *L)
{
	cap_channel_t *chan;
	int maskpri;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	maskpri = luaL_checkinteger(L, 2);

	lua_pushinteger(L, cap_setlogmask(chan, maskpri));
	return (1);
}

static int
l_cap_syslog(lua_State *L)
{
	cap_channel_t *chan;
	const char *message;
	int priority;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	priority = luaL_checkinteger(L, 2);
	message = luaL_checkstring(L, 3);

	cap_syslog(chan, priority, "%s", message);
	return (0);
}

static int
l_cap_closelog(lua_State *L)
{
	cap_channel_t *chan;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);

	cap_closelog(chan);
	return (0);
}

static const struct luaL_Reg l_syslog_funcs[] = {
	{"openlog", l_cap_openlog},
	{"setlogmask", l_cap_setlogmask},
	{"syslog", l_cap_syslog},
	{"closelog", l_cap_closelog},
	{NULL, NULL}
};

int
luaopen_casper_syslog(lua_State *L)
{
	luaL_newlib(L, l_syslog_funcs);
	return (1);
}
