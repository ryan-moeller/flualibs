/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <netinet/ip.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int luaopen_netinet_ip(lua_State *);

static int
l_ipopt_copied(lua_State *L)
{
	lua_Integer o;

	o = luaL_checkinteger(L, 1);

	lua_pushboolean(L, IPOPT_COPIED(o) != 0);
	return (1);
}

static int
l_ipopt_class(lua_State *L)
{
	lua_Integer o;

	o = luaL_checkinteger(L, 1);

	lua_pushinteger(L, IPOPT_CLASS(o));
	return (1);
}

static int
l_ipopt_number(lua_State *L)
{
	lua_Integer o;

	o = luaL_checkinteger(L, 1);

	lua_pushinteger(L, IPOPT_NUMBER(o));
	return (1);
}

static const struct luaL_Reg l_ip_funcs[] = {
	{"ipopt_copied", l_ipopt_copied},
	{"ipopt_class", l_ipopt_class},
	{"ipopt_number", l_ipopt_number},
	{NULL, NULL}
};

int
luaopen_netinet_ip(lua_State *L)
{
	luaL_newlib(L, l_ip_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(IPVERSION);

	DEFINE(IP_MAXPACKET);

	DEFINE(IPTOS_LOWDELAY);
	DEFINE(IPTOS_THROUGHPUT);
	DEFINE(IPTOS_RELIABILITY);
	DEFINE(IPTOS_MINCOST);

	/* Deprecated IP precedence definitions omitted. */

	DEFINE(IPTOS_DSCP_OFFSET);

	DEFINE(IPTOS_DSCP_CS0);
	DEFINE(IPTOS_DSCP_CS1);
	DEFINE(IPTOS_DSCP_AF11);
	DEFINE(IPTOS_DSCP_AF12);
	DEFINE(IPTOS_DSCP_AF13);
	DEFINE(IPTOS_DSCP_CS2);
	DEFINE(IPTOS_DSCP_AF21);
	DEFINE(IPTOS_DSCP_AF22);
	DEFINE(IPTOS_DSCP_AF23);
	DEFINE(IPTOS_DSCP_CS3);
	DEFINE(IPTOS_DSCP_AF31);
	DEFINE(IPTOS_DSCP_AF32);
	DEFINE(IPTOS_DSCP_AF33);
	DEFINE(IPTOS_DSCP_CS4);
	DEFINE(IPTOS_DSCP_AF41);
	DEFINE(IPTOS_DSCP_AF42);
	DEFINE(IPTOS_DSCP_AF43);
	DEFINE(IPTOS_DSCP_CS5);
	DEFINE(IPTOS_DSCP_VA);
	DEFINE(IPTOS_DSCP_EF);
	DEFINE(IPTOS_DSCP_CS6);
	DEFINE(IPTOS_DSCP_CS7);

	DEFINE(IPTOS_ECN_NOTECT);
	DEFINE(IPTOS_ECN_ECT1);
	DEFINE(IPTOS_ECN_ECT0);
	DEFINE(IPTOS_ECN_CE);
	DEFINE(IPTOS_ECN_MASK);

	DEFINE(IPOPT_CONTROL);
	DEFINE(IPOPT_RESERVED1);
	DEFINE(IPOPT_DEBMEAS);
	DEFINE(IPOPT_RESERVED2);

	DEFINE(IPOPT_EOL);
	DEFINE(IPOPT_NOP);

	DEFINE(IPOPT_RR);
	DEFINE(IPOPT_TS);
	DEFINE(IPOPT_SECURITY);
	DEFINE(IPOPT_LSRR);
	DEFINE(IPOPT_ESO);
	DEFINE(IPOPT_CIPSO);
	DEFINE(IPOPT_SATID);
	DEFINE(IPOPT_SSRR);
	DEFINE(IPOPT_RA);

	DEFINE(IPOPT_OPTVAL);
	DEFINE(IPOPT_OLEN);
	DEFINE(IPOPT_OFFSET);
	DEFINE(IPOPT_MINOFF);

	DEFINE(IPOPT_TS_TSONLY);
	DEFINE(IPOPT_TS_TSANDADDR);
	DEFINE(IPOPT_TS_PRESPEC);

	DEFINE(IPOPT_SECUR_UNCLASS);
	DEFINE(IPOPT_SECUR_CONFID);
	DEFINE(IPOPT_SECUR_EFTO);
	DEFINE(IPOPT_SECUR_MMMM);
	DEFINE(IPOPT_SECUR_RESTR);
	DEFINE(IPOPT_SECUR_SECRET);
	DEFINE(IPOPT_SECUR_TOPSECRET);

	DEFINE(MAXTTL);
	DEFINE(IPDEFTTL);
	DEFINE(IPTTLDEC);
	DEFINE(IP_MSS);
#undef DEFINE
	return (1);
}
