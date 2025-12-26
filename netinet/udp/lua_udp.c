/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/udp.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int luaopen_netinet_udp(lua_State *);

int
luaopen_netinet_udp(lua_State *L)
{
	lua_newtable(L);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, UDP_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(ENCAP);
	DEFINE(VENDOR);
	DEFINE(ENCAP_ESPINUDP_NON_IKE);
	DEFINE(ENCAP_ESPINUDP);
	DEFINE(ENCAP_ESPINUDP_PORT);
	DEFINE(ENCAP_ESPINUDP_MAXFRAGLEN);
#undef DEFINE
	return (1);
}
