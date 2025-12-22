/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <netdb.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "sys/socket/lua_socket.h"

int luaopen_netdb(lua_State *);

static inline void
checkai(lua_State *L, int idx, struct addrinfo *ai)
{
	memset(ai, 0, sizeof(*ai));
	luaL_checktype(L, idx, LUA_TTABLE);
	if (lua_getfield(L, idx, "flags") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid flags");
		ai->ai_flags = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "family") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid family");
		ai->ai_family = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "socktype") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid socktype");
		ai->ai_socktype = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "protocol") != LUA_TNIL) {
		luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid protocol");
		ai->ai_protocol = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);
	/* Other fields are not relevant to getaddrinfo. */
}

static inline void
pushai(lua_State *L, const struct addrinfo *ai)
{
	lua_newtable(L);
	lua_pushinteger(L, ai->ai_flags);
	lua_setfield(L, -2, "flags");
	lua_pushinteger(L, ai->ai_family);
	lua_setfield(L, -2, "family");
	lua_pushinteger(L, ai->ai_socktype);
	lua_setfield(L, -2, "socktype");
	lua_pushinteger(L, ai->ai_protocol);
	lua_setfield(L, -2, "protocol");
	pushaddr(L, ai->ai_addr);
	lua_setfield(L, -2, "addr");
}

static inline int
gai_fail(lua_State *L, int error)
{
	luaL_pushfail(L);
	lua_pushstring(L, gai_strerror(error));
	lua_pushinteger(L, error);
	return (3);
}

static int
l_getaddrinfo(lua_State *L)
{
	struct addrinfo hints, *res;
	const struct addrinfo *hintsp;
	const char *hostname, *servname;
	int error;

	hostname = luaL_optstring(L, 1, NULL);
	servname = luaL_optstring(L, 2, NULL);
	if (lua_istable(L, 3)) {
		hintsp = &hints;
		checkai(L, 3, &hints);
	} else {
		hintsp = NULL;
	}

	if ((error = getaddrinfo(hostname, servname, hintsp, &res)) != 0) {
		return (gai_fail(L, error));
	}
	lua_newtable(L);
	for (const struct addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
		pushai(L, ai);
		lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	}
	freeaddrinfo(res);
	return (1);
}

static int
l_getnameinfo(lua_State *L)
{
	char host[NI_MAXHOST], serv[NI_MAXSERV];
	struct sockaddr_storage ss;
	const struct sockaddr *sa;
	int flags, error;

	sa = (const struct sockaddr *)&ss;

	checkaddr(L, 1, &ss);
	flags = luaL_optinteger(L, 2, 0);

	if ((error = getnameinfo(sa, sa->sa_len, host, sizeof(host), serv,
	    sizeof(serv), flags)) != 0) {
		return (gai_fail(L, error));
	}
	lua_pushstring(L, host);
	lua_pushstring(L, serv);
	return (2);
}

static const struct luaL_Reg l_netdb_funcs[] = {
	{"getaddrinfo", l_getaddrinfo},
	{"getnameinfo", l_getnameinfo},
	{NULL, NULL}
};

int
luaopen_netdb(lua_State *L)
{
	luaL_newlib(L, l_netdb_funcs);
#define SETCONST(ident) ({ \
	lua_pushliteral(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(_PATH_HEQUIV);
	SETCONST(_PATH_HOSTS);
	SETCONST(_PATH_NETWORKS);
	SETCONST(_PATH_PROTOCOLS);
	SETCONST(_PATH_SERVICES);
	SETCONST(_PATH_SERVICES_DB);
#undef SETCONST
	{
		const char c = SCOPE_DELIMITER;

		lua_pushlstring(L, &c, 1);
		lua_setfield(L, -2, "SCOPE_DELIMITER");
	}
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(IPPORT_RESERVED);

	SETCONST(NETDB_INTERNAL);
	SETCONST(NETDB_SUCCESS);
	SETCONST(HOST_NOT_FOUND);
	SETCONST(TRY_AGAIN);
	SETCONST(NO_RECOVERY);
	SETCONST(NO_DATA);
	SETCONST(NO_ADDRESS);

	SETCONST(EAI_ADDRFAMILY);
	SETCONST(EAI_AGAIN);
	SETCONST(EAI_BADFLAGS);
	SETCONST(EAI_FAIL);
	SETCONST(EAI_FAMILY);
	SETCONST(EAI_MEMORY);
	SETCONST(EAI_NODATA);
	SETCONST(EAI_NONAME);
	SETCONST(EAI_SERVICE);
	SETCONST(EAI_SOCKTYPE);
	SETCONST(EAI_SYSTEM);
	SETCONST(EAI_BADHINTS);
	SETCONST(EAI_PROTOCOL);
	SETCONST(EAI_OVERFLOW);
	SETCONST(EAI_MAX);

	SETCONST(AI_PASSIVE);
	SETCONST(AI_CANONNAME);
	SETCONST(AI_NUMERICHOST);
	SETCONST(AI_NUMERICSERV);
	SETCONST(AI_MASK);
	SETCONST(AI_ALL);
	SETCONST(AI_V4MAPPED_CFG);
	SETCONST(AI_ADDRCONFIG);
	SETCONST(AI_V4MAPPED);
	SETCONST(AI_DEFAULT);

	SETCONST(NI_MAXHOST);
	SETCONST(NI_MAXSERV);

	SETCONST(NI_NOFQDN);
	SETCONST(NI_NUMERICHOST);
	SETCONST(NI_NAMEREQD);
	SETCONST(NI_NUMERICSERV);
	SETCONST(NI_DGRAM);
	SETCONST(NI_NUMERICSCOPE);
#undef SETCONST
	return (1);
}
