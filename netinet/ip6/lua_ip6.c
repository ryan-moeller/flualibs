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
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(IPV6_VERSION);
	DEFINE(IPV6_VERSION_MASK);

	DEFINE(IPV6_FLOWINFO_MASK);
	DEFINE(IPV6_FLOWLABEL_MASK);
	DEFINE(IPV6_ECN_MASK);
	DEFINE(IPV6_FLOWLABEL_LEN);

	DEFINE(IP6OPT_PAD1);
	DEFINE(IP6OPT_PADN);
	DEFINE(IP6OPT_JUMBO);
	DEFINE(IP6OPT_NSAP_ADDR);
	DEFINE(IP6OPT_TUNNEL_LIMIT);
	DEFINE(IP6OPT_ROUTER_ALERT);
	DEFINE(IP6OPT_RTALERT);
	DEFINE(IP6OPT_RTALERT_LEN);
	DEFINE(IP6OPT_RTALERT_MLD);
	DEFINE(IP6OPT_RTALERT_RSVP);
	DEFINE(IP6OPT_RTALERT_ACTNET);
	DEFINE(IP6OPT_MINLEN);
	DEFINE(IP6OPT_EID);
	DEFINE(IP6OPT_TYPE_SKIP);
	DEFINE(IP6OPT_TYPE_DISCARD);
	DEFINE(IP6OPT_TYPE_FORCEICMP);
	DEFINE(IP6OPT_TYPE_ICMP);
	DEFINE(IP6OPT_MUTABLE);
	DEFINE(IP6OPT_JUMBO_LEN);

	DEFINE(IP6_ALERT_MLD);
	DEFINE(IP6_ALERT_RSVP);
	DEFINE(IP6_ALERT_AN);

	DEFINE(IP6F_OFF_MASK);
	DEFINE(IP6F_RESERVED_MASK);
	DEFINE(IP6F_MORE_FRAG);

	DEFINE(IPV6_MAXHLIM);
	DEFINE(IPV6_DEFHLIM);
	DEFINE(IPV6_DEFFRAGTTL);
	DEFINE(IPV6_HLIMDEC);
	DEFINE(IPV6_MMTU);
	DEFINE(IPV6_MAXPACKET);
	DEFINE(IPV6_MAXOPTHDR);
#undef DEFINE
	return (1);
}
