/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifer: BSD-2-Clause
 */

#include <libcasper.h>
#include <casper/cap_netdb.h>

#include <lua.h>
#include <lauxlib.h>

#include "libcasper/libcasper/lua_casper.h"
#include "netdb/lua_netdb.h"
#include "utils.h"

int luaopen_casper_netdb(lua_State *);

static int
l_cap_getprotobyname(lua_State *L)
{
	cap_channel_t *capnetdb;
	const char *name;
	struct protoent *ent;

	capnetdb = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	name = luaL_checkstring(L, 2);

	if ((ent = cap_getprotobyname(capnetdb, name)) == NULL) {
		/* XXX: no reliable error info from cap_netdb, assume ENOENT */
		return (0);
	}
	pushprotoent(L, ent);
	return (1);
}

static const struct luaL_Reg l_netdb_funcs[] = {
	{"getprotobyname", l_cap_getprotobyname},
	{NULL, NULL}
};

int
luaopen_casper_netdb(lua_State *L)
{
	luaL_newlib(L, l_netdb_funcs);
	return (1);
}
