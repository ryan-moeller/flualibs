/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip6.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int luaopen_netinet_ip6(lua_State *);

static int
l_ip6opt_type(lua_State *L)
{
	lua_Integer o;

	o = luaL_checkinteger(L, 1);

	lua_pushinteger(L, IP6OPT_TYPE(o));
	return (1);
}

static int
l_ipv6_traffic_class(lua_State *L)
{
	struct ip6_hdr *ip6;
	size_t len;

	ip6 = (struct ip6_hdr *)luaL_checklstring(L, 1, &len);
	luaL_argcheck(L, len == sizeof(*ip6), 1, "invalid ip6_hdr");

	lua_pushinteger(L, IPV6_TRAFFIC_CLASS(ip6));
	return (1);
}

static int
l_ipv6_dscp(lua_State *L)
{
	struct ip6_hdr *ip6;
	size_t len;

	ip6 = (struct ip6_hdr *)luaL_checklstring(L, 1, &len);
	luaL_argcheck(L, len == sizeof(*ip6), 1, "invalid ip6_hdr");

	lua_pushinteger(L, IPV6_DSCP(ip6));
	return (1);
}

static int
l_ipv6_ecn(lua_State *L)
{
	struct ip6_hdr *ip6;
	size_t len;

	ip6 = (struct ip6_hdr *)luaL_checklstring(L, 1, &len);
	luaL_argcheck(L, len == sizeof(*ip6), 1, "invalid ip6_hdr");

	lua_pushinteger(L, IPV6_ECN(ip6));
	return (1);
}

static const struct luaL_Reg l_ip6_funcs[] = {
	{"ip6opt_type", l_ip6opt_type},
	{"ipv6_traffic_class", l_ipv6_traffic_class},
	{"ipv6_dscp", l_ipv6_dscp},
	{"ipv6_ecn", l_ipv6_ecn},
	{NULL, NULL}
};

int
luaopen_netinet_ip6(lua_State *L)
{
	luaL_newlib(L, l_ip6_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(IPV6_VERSION);
	SETCONST(IPV6_VERSION_MASK);

	SETCONST(IPV6_FLOWINFO_MASK);
	SETCONST(IPV6_FLOWLABEL_MASK);
	SETCONST(IPV6_ECN_MASK);
	SETCONST(IPV6_FLOWLABEL_LEN);

	SETCONST(IP6OPT_PAD1);
	SETCONST(IP6OPT_PADN);
	SETCONST(IP6OPT_JUMBO);
	SETCONST(IP6OPT_NSAP_ADDR);
	SETCONST(IP6OPT_TUNNEL_LIMIT);
	SETCONST(IP6OPT_ROUTER_ALERT);
	SETCONST(IP6OPT_RTALERT);
	SETCONST(IP6OPT_RTALERT_LEN);
	SETCONST(IP6OPT_RTALERT_MLD);
	SETCONST(IP6OPT_RTALERT_RSVP);
	SETCONST(IP6OPT_RTALERT_ACTNET);
	SETCONST(IP6OPT_MINLEN);
	SETCONST(IP6OPT_EID);
	SETCONST(IP6OPT_TYPE_SKIP);
	SETCONST(IP6OPT_TYPE_DISCARD);
	SETCONST(IP6OPT_TYPE_FORCEICMP);
	SETCONST(IP6OPT_TYPE_ICMP);
	SETCONST(IP6OPT_MUTABLE);
	SETCONST(IP6OPT_JUMBO_LEN);

	SETCONST(IP6_ALERT_MLD);
	SETCONST(IP6_ALERT_RSVP);
	SETCONST(IP6_ALERT_AN);

	SETCONST(IP6F_OFF_MASK);
	SETCONST(IP6F_RESERVED_MASK);
	SETCONST(IP6F_MORE_FRAG);

	SETCONST(IPV6_MAXHLIM);
	SETCONST(IPV6_DEFHLIM);
	SETCONST(IPV6_DEFFRAGTTL);
	SETCONST(IPV6_HLIMDEC);
	SETCONST(IPV6_MMTU);
	SETCONST(IPV6_MAXPACKET);
	SETCONST(IPV6_MAXOPTHDR);
#undef SETCONST
	return (1);
}
