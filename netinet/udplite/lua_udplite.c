/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <netinet/udplite.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int luaopen_netinet_udplite(lua_State *);

int
luaopen_netinet_udplite(lua_State *L)
{
	lua_newtable(L);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, UDPLITE_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(SEND_CSCOV);
	DEFINE(RECV_CSCOV);
#undef DEFINE
	return (1);
}
