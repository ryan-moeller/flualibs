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
#include "lua_netdb.h"

int luaopen_netdb(lua_State *);

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
#define DEFINE(ident) ({ \
	lua_pushliteral(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(_PATH_HEQUIV);
	DEFINE(_PATH_HOSTS);
	DEFINE(_PATH_NETWORKS);
	DEFINE(_PATH_PROTOCOLS);
	DEFINE(_PATH_SERVICES);
	DEFINE(_PATH_SERVICES_DB);
#undef DEFINE
	{
		const char c = SCOPE_DELIMITER;

		lua_pushlstring(L, &c, 1);
		lua_setfield(L, -2, "SCOPE_DELIMITER");
	}
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(IPPORT_RESERVED);

	DEFINE(NETDB_INTERNAL);
	DEFINE(NETDB_SUCCESS);
	DEFINE(HOST_NOT_FOUND);
	DEFINE(TRY_AGAIN);
	DEFINE(NO_RECOVERY);
	DEFINE(NO_DATA);
	DEFINE(NO_ADDRESS);

	DEFINE(EAI_ADDRFAMILY);
	DEFINE(EAI_AGAIN);
	DEFINE(EAI_BADFLAGS);
	DEFINE(EAI_FAIL);
	DEFINE(EAI_FAMILY);
	DEFINE(EAI_MEMORY);
	DEFINE(EAI_NODATA);
	DEFINE(EAI_NONAME);
	DEFINE(EAI_SERVICE);
	DEFINE(EAI_SOCKTYPE);
	DEFINE(EAI_SYSTEM);
	DEFINE(EAI_BADHINTS);
	DEFINE(EAI_PROTOCOL);
	DEFINE(EAI_OVERFLOW);
	DEFINE(EAI_MAX);

	DEFINE(AI_PASSIVE);
	DEFINE(AI_CANONNAME);
	DEFINE(AI_NUMERICHOST);
	DEFINE(AI_NUMERICSERV);
	DEFINE(AI_MASK);
	DEFINE(AI_ALL);
	DEFINE(AI_V4MAPPED_CFG);
	DEFINE(AI_ADDRCONFIG);
	DEFINE(AI_V4MAPPED);
	DEFINE(AI_DEFAULT);

	DEFINE(NI_MAXHOST);
	DEFINE(NI_MAXSERV);

	DEFINE(NI_NOFQDN);
	DEFINE(NI_NUMERICHOST);
	DEFINE(NI_NAMEREQD);
	DEFINE(NI_NUMERICSERV);
	DEFINE(NI_DGRAM);
	DEFINE(NI_NUMERICSCOPE);
#undef DEFINE
	return (1);
}
