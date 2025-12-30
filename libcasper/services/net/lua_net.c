/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifer: BSD-2-Clause
 */

#include <libcasper.h>
#include <casper/cap_net.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/socket/lua_socket.h"
#include "netdb/lua_netdb.h"
#include "libcasper/libcasper/lua_casper.h"
#include "utils.h"

#define CAP_NET_LIMIT_METATABLE "cap_net_limit_t *"

int luaopen_casper_net(lua_State *);

static int
l_cap_bind(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *addr;
	cap_channel_t *chan;
	int s;

	addr = (const struct sockaddr *)&ss;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	s = checkfd(L, 2);
	checkaddr(L, 3, &ss);

	if (cap_bind(chan, s, addr, addr->sa_len) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_connect(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *addr;
	cap_channel_t *chan;
	int s;

	addr = (const struct sockaddr *)&ss;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	s = checkfd(L, 2);
	checkaddr(L, 3, &ss);

	if (cap_connect(chan, s, addr, addr->sa_len) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_getaddrinfo(lua_State *L)
{
	struct addrinfo hints, *res;
	const struct addrinfo *hintsp;
	const char *hostname, *servname;
	cap_channel_t *chan;
	int error;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	hostname = luaL_optstring(L, 2, NULL);
	servname = luaL_optstring(L, 3, NULL);
	if (lua_istable(L, 4)) {
		hintsp = &hints;
		checkai(L, 4, &hints);
	} else {
		hintsp = NULL;
	}

	if ((error = cap_getaddrinfo(chan, hostname, servname, hintsp, &res))
	    != 0) {
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
l_cap_getnameinfo(lua_State *L)
{
	char host[NI_MAXHOST], serv[NI_MAXSERV];
	struct sockaddr_storage ss;
	const struct sockaddr *sa;
	cap_channel_t *chan;
	int flags, error;

	sa = (const struct sockaddr *)&ss;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	checkaddr(L, 2, &ss);
	flags = luaL_optinteger(L, 3, 0);

	if ((error = cap_getnameinfo(chan, sa, sa->sa_len, host, sizeof(host),
	    serv, sizeof(serv), flags)) != 0) {
		return (gai_fail(L, error));
	}
	lua_pushstring(L, host);
	lua_pushstring(L, serv);
	return (2);
}

static int
l_cap_net_limit_init(lua_State *L)
{
	cap_channel_t *chan;
	cap_net_limit_t *limit;
	uint64_t mode;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	mode = luaL_checkinteger(L, 2);

	if ((limit = cap_net_limit_init(chan, mode)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, limit, CAP_NET_LIMIT_METATABLE));
}

static int
l_cap_net_free(lua_State *L)
{
	cap_net_limit_t *limit;

	limit = checkcookienull(L, 1, CAP_NET_LIMIT_METATABLE);

	cap_net_free(limit);
	return (0);
}

static int
l_cap_net_limit_addr2name(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *sa;
	cap_net_limit_t *limit;

	sa = (const struct sockaddr *)&ss;

	limit = checkcookie(L, 1, CAP_NET_LIMIT_METATABLE);
	checkaddr(L, 2, &ss);

	cap_net_limit_addr2name(limit, sa, sa->sa_len);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_net_limit_addr2name_family(lua_State *L)
{
	cap_net_limit_t *limit;
	int *family;
	size_t n;

	limit = checkcookie(L, 1, CAP_NET_LIMIT_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);

	n = luaL_len(L, 2);
	if ((family = malloc(n * sizeof(*family))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (size_t i = 0; i < n; i++) {
		lua_geti(L, 2, i + 1);
		family[i] = lua_tointeger(L, -1);
	}
	lua_pop(L, n);
	cap_net_limit_addr2name_family(limit, family, n);
	free(family);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_net_limit_bind(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *sa;
	cap_net_limit_t *limit;

	sa = (const struct sockaddr *)&ss;

	limit = checkcookie(L, 1, CAP_NET_LIMIT_METATABLE);
	checkaddr(L, 2, &ss);

	cap_net_limit_bind(limit, sa, sa->sa_len);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_net_limit_connect(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *sa;
	cap_net_limit_t *limit;

	sa = (const struct sockaddr *)&ss;

	limit = checkcookie(L, 1, CAP_NET_LIMIT_METATABLE);
	checkaddr(L, 2, &ss);

	cap_net_limit_connect(limit, sa, sa->sa_len);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_net_limit_name2addr(lua_State *L)
{
	cap_net_limit_t *limit;
	const char *name, *serv;

	limit = checkcookie(L, 1, CAP_NET_LIMIT_METATABLE);
	name = luaL_optstring(L, 2, NULL);
	serv = luaL_optstring(L, 3, NULL);

	cap_net_limit_name2addr(limit, name, serv);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_net_limit_name2addr_family(lua_State *L)
{
	cap_net_limit_t *limit;
	int *family;
	size_t n;

	limit = checkcookie(L, 1, CAP_NET_LIMIT_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);

	n = luaL_len(L, 2);
	if ((family = malloc(n * sizeof(*family))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (size_t i = 0; i < n; i++) {
		lua_geti(L, 2, i + 1);
		family[i] = lua_tointeger(L, -1);
	}
	lua_pop(L, n);
	cap_net_limit_name2addr_family(limit, family, n);
	free(family);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_net_limit(lua_State *L)
{
	cap_net_limit_t *limit;

	limit = checkcookie(L, 1, CAP_NET_LIMIT_METATABLE);

	setcookie(L, 1, NULL);
	if (cap_net_limit(limit) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static const struct luaL_Reg l_net_funcs[] = {
	{"bind", l_cap_bind},
	{"connect", l_cap_connect},
	{"getaddrinfo", l_cap_getaddrinfo},
	{"getnameinfo", l_cap_getnameinfo},
	{"limit_init", l_cap_net_limit_init},
	{NULL, NULL}
};

static const struct luaL_Reg l_net_limit_meta[] = {
	{"__gc", l_cap_net_free},
	{"addr2name", l_cap_net_limit_addr2name},
	{"addr2name_family", l_cap_net_limit_addr2name_family},
	{"bind", l_cap_net_limit_bind},
	{"connect", l_cap_net_limit_connect},
	{"name2addr", l_cap_net_limit_name2addr},
	{"name2addr_family", l_cap_net_limit_name2addr_family},
	{"limit", l_cap_net_limit},
	{NULL, NULL}
};

int
luaopen_casper_net(lua_State *L)
{
	luaL_newmetatable(L, CAP_NET_LIMIT_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_net_limit_meta, 0);

	luaL_newlib(L, l_net_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, CAPNET_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(ADDR2NAME);
	DEFINE(NAME2ADDR);
	DEFINE(CONNECT);
	DEFINE(BIND);
	DEFINE(CONNECTDNS);
#undef DEFINE
	return (1);
}
