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
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(UDP_ENCAP);
	DEFINE(UDP_VENDOR);
	DEFINE(UDP_ENCAP_ESPINUDP_NON_IKE);
	DEFINE(UDP_ENCAP_ESPINUDP);
	DEFINE(UDP_ENCAP_ESPINUDP_PORT);
	DEFINE(UDP_ENCAP_ESPINUDP_MAXFRAGLEN);
#undef DEFINE
	return (1);
}
