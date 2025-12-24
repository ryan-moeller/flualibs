/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <netinet/in.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "sys/socket/lua_socket.h"
#include "utils.h"

#define IN_ADDR_TESTS(X) \
	X(MULTICAST, multicast) \
	X(EXPERIMENTAL, experimental) \
	X(BADCLASS, badclass) \
	X(LINKLOCAL, linklocal) \
	X(LOOPBACK, loopback) \
	X(ZERONET, zeronet) \
	X(PRIVATE, private) \
	X(LOCAL_GROUP, local_group) \
	X(ANY_LOCAL, any_local) \

#define IN6_ADDR_TESTS(X) \
	X(UNSPECIFIED, unspecified) \
	X(LOOPBACK, loopback) \
	X(V4COMPAT, v4compat) \
	X(V4MAPPED, v4mapped) \
	X(LINKLOCAL, linklocal) \
	X(SITELOCAL, sitelocal) \
	X(MULTICAST, multicast) \
	X(MC_NODELOCAL, mc_nodelocal) \
	X(MC_LINKLOCAL, mc_linklocal) \
	X(MC_SITELOCAL, mc_sitelocal) \
	X(MC_ORGLOCAL, mc_orglocal) \
	X(MC_GLOBAL, mc_global) \

int luaopen_netinet_in(lua_State *);

static int
l_sockaddr_in(lua_State *L)
{
	struct sockaddr_in sin;
	struct sockaddr *addr;
	const char *data;
	size_t len;

	addr = (struct sockaddr *)&sin;

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "family");
	luaL_argcheck(L, lua_tointeger(L, -1) == AF_INET, 1,
	    "invalid sockaddr family");
	luaL_argcheck(L, lua_getfield(L, 1, "data") == LUA_TSTRING, 1,
	    "invalid sockaddr data");
	data = lua_tolstring(L, -1, &len);
	luaL_argcheck(L,
	    offsetof(struct sockaddr, sa_data) + len == sizeof(sin), 1,
	    "invalid sockaddr_in");

	memcpy(addr->sa_data, data, len);
	lua_newtable(L);
	lua_pushinteger(L, ntohs(sin.sin_port));
	lua_setfield(L, -2, "port");
	lua_pushinteger(L, ntohl(sin.sin_addr.s_addr));
	lua_setfield(L, -2, "addr");
	return (1);
}

static int
l_sockaddr_in6(lua_State *L)
{
	struct sockaddr_in6 sin;
	struct sockaddr *addr;
	const char *data;
	size_t len;

	addr = (struct sockaddr *)&sin;

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "family");
	luaL_argcheck(L, lua_tointeger(L, -1) == AF_INET6, 1,
	    "invalid sockaddr family");
	luaL_argcheck(L, lua_getfield(L, 1, "data") == LUA_TSTRING, 1,
	    "invalid sockaddr data");
	data = lua_tolstring(L, -1, &len);
	luaL_argcheck(L,
	    offsetof(struct sockaddr, sa_data) + len == sizeof(sin), 1,
	    "invalid sockaddr_in6");

	memcpy(addr->sa_data, data, len);
	lua_newtable(L);
	lua_pushinteger(L, ntohs(sin.sin6_port));
	lua_setfield(L, -2, "port");
	lua_pushinteger(L, ntohl(sin.sin6_flowinfo));
	lua_setfield(L, -2, "flowinfo");
	lua_pushlstring(L, (char *)&sin.sin6_addr, sizeof(sin.sin6_addr));
	lua_setfield(L, -2, "addr");
	lua_pushinteger(L, ntohl(sin.sin6_scope_id));
	lua_setfield(L, -2, "scope_id");
	return (1);
}

#define IN_ADDR_TEST(upper, lower) \
static int \
l_in_##lower(lua_State *L) \
{ \
	in_addr_t i; \
\
	i = luaL_checkinteger(L, 1); \
\
	lua_pushboolean(L, IN_##upper(i)); \
	return (1); \
}

IN_ADDR_TESTS(IN_ADDR_TEST)

#undef IN_ADDR_TEST

static inline const struct in6_addr *
checkin6addr(lua_State *L, int idx)
{
	const struct in6_addr *a;
	size_t len;

	a = (const struct in6_addr *)luaL_checklstring(L, idx, &len);
	luaL_argcheck(L, len == sizeof(*a), idx, "invalid in6_addr");

	return (a);
}

static int
l_in6_are_addr_equal(lua_State *L)
{
	const struct in6_addr *a, *b;

	a = checkin6addr(L, 1);
	b = checkin6addr(L, 2);

	lua_pushboolean(L, IN6_ARE_ADDR_EQUAL(a, b));
	return (1);
}

#define IN6_ADDR_TEST(upper, lower) \
static int \
l_in6_is_addr_##lower(lua_State *L) \
{ \
	const struct in6_addr *a; \
\
	a = checkin6addr(L, 1); \
\
	lua_pushboolean(L, IN6_IS_ADDR_##upper(a)); \
	return (1); \
}

IN6_ADDR_TESTS(IN6_ADDR_TEST)

#undef IN6_ADDR_TEST

static int
l_setsourcefilter(lua_State *L)
{
	struct sockaddr_storage gs, *slist;
	struct sockaddr *group;
	uint32_t interface, fmode, numsrc;
	int s;

	group = (struct sockaddr *)&gs;

	s = checkfd(L, 1);
	interface = luaL_checkinteger(L, 2);
	checkaddr(L, 3, &gs);
	fmode = luaL_checkinteger(L, 4);
	if (lua_isnoneornil(L, 5)) {
		numsrc = 0;
	} else {
		luaL_checktype(L, 5, LUA_TTABLE);
		numsrc = luaL_len(L, 5);
	}

	if (numsrc > 0) {
		if ((slist = malloc(numsrc * sizeof(*slist))) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	} else {
		slist = NULL;
	}
	for (uint32_t i = 0; i < numsrc; i++) {
		struct sockaddr *sa = (struct sockaddr *)&slist[i];
		size_t datalen;
		int idx;

		lua_geti(L, 5, i + 1);
		idx = lua_gettop(L);
		if (!lua_istable(L, idx)) {
			goto bad;
		}
		lua_getfield(L, idx, "family");
		if (!lua_isinteger(L, -1)) {
			goto bad;
		}
		sa->sa_family = lua_tointeger(L, -1);
		if (lua_getfield(L, idx, "data") != LUA_TSTRING) {
			goto bad;
		}
		memcpy(sa->sa_data, lua_tolstring(L, -1, &datalen), datalen);
		sa->sa_len = datalen + offsetof(struct sockaddr, sa_data);
	}
	lua_pop(L, numsrc);
	if (setsourcefilter(s, interface, group, group->sa_len, fmode, numsrc,
	    slist) == -1) {
		free(slist);
		return (fail(L, errno));
	}
	free(slist);
	lua_pushboolean(L, true);
	return (1);
bad:
	free(slist);
	return (luaL_argerror(L, 5, "invalid sockaddr"));
}

static int
l_getsourcefilter(lua_State *L)
{
	struct sockaddr_storage gs, *slist;
	struct sockaddr *group;
	uint32_t interface, fmode, slistlen, numsrc;
	int s;

	group = (struct sockaddr *)&gs;

	s = checkfd(L, 1);
	interface = luaL_checkinteger(L, 2);
	checkaddr(L, 3, &gs);

	slistlen = 0;
	/* Get an initial size. */
	if (getsourcefilter(s, interface, group, group->sa_len, &fmode,
	    &slistlen, NULL) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fmode);
	lua_createtable(L, slistlen, 0);
	if (slistlen == 0) {
		return (2);
	}
grew:
	slistlen *= 2; /* Anticipate potential growth. */
	if ((slist = malloc(slistlen * sizeof(*slist))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	numsrc = slistlen;
	if (getsourcefilter(s, interface, group, group->sa_len, &fmode, &numsrc,
	    slist) == -1) {
		free(slist);
		return (fail(L, errno));
	}
	if (numsrc > slistlen) {
		free(slist);
		slistlen = numsrc;
		goto grew;
	}
	for (uint32_t i = 0; i < numsrc; i++) {
		pushaddr(L, (const struct sockaddr *)&slist[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(slist);
	return (2);
}

static int
l_inet6_opts_ext(lua_State *L)
{
	void *ext;
	lua_Integer n;
	socklen_t extlen;
	int offset;

	luaL_checktype(L, 1, LUA_TTABLE);
	n = luaL_len(L, 1);

	offset = inet6_opt_init(NULL, 0);
	assert(offset != -1);
	for (lua_Integer i = 1; i <= n; i++) {
		size_t len;
		uint8_t type, align;

		if (lua_geti(L, 1, i) != LUA_TTABLE) {
			goto bad;
		}
		lua_getfield(L, -1, "type");
		if (!lua_isinteger(L, -1)) {
			goto bad;
		}
		type = lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, -1, "align");
		if (!lua_isinteger(L, -1)) {
			goto bad;
		}
		align = lua_tointeger(L, -1);
		lua_pop(L, 1);
		if (lua_getfield(L, -1, "value") != LUA_TSTRING) {
			goto bad;
		}
		lua_tolstring(L, -1, &len);
		lua_pop(L, 1);
		if ((offset = inet6_opt_append(NULL, 0, offset, type, len,
		    align, NULL)) == -1) {
			goto bad;
		}
		lua_pop(L, 1);
	}
	offset = inet6_opt_finish(NULL, 0, offset);
	assert(offset != -1);
	extlen = offset;
	if ((ext = malloc(extlen)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	offset = inet6_opt_init(ext, extlen);
	assert(offset != -1);
	for (lua_Integer i = 1; i <= n; i++) {
		const void *value;
		void *databuf;
		size_t len;
		int voff;
		uint8_t type, align;

		lua_geti(L, 1, i);
		lua_getfield(L, -1, "type");
		type = lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, -1, "align");
		align = lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, -1, "value");
		value = lua_tolstring(L, -1, &len);
		lua_pop(L, 1);
		offset = inet6_opt_append(ext, extlen, offset, type, len, align,
		    &databuf);
		assert(offset != -1);
		voff = inet6_opt_set_val(databuf, 0, __DECONST(void *, value),
		    len);
		assert(voff != -1);
		lua_pop(L, 1);
	}
	offset = inet6_opt_finish(ext, extlen, offset);
	assert(offset == extlen);
	lua_pushlstring(L, ext, extlen);
	free(ext);
	return (1);
bad:
	return (luaL_argerror(L, 1, "invalid opts"));
}

static int
l_inet6_ext_opts(lua_State *L)
{
	void *ext, *databuf;
	size_t extlen;
	int offset;
	socklen_t datalen;
	uint8_t type;

	ext = __DECONST(void *, luaL_checklstring(L, 1, &extlen));

	lua_newtable(L);
	offset = 0;
	while ((offset = inet6_opt_next(ext, extlen, offset, &type, &datalen,
	    &databuf)) != -1) {
		lua_createtable(L, 0, 2);
		lua_pushlstring(L, databuf, datalen);
		lua_setfield(L, -2, "value");
		lua_pushinteger(L, type);
		lua_setfield(L, -2, "type");
		lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	}
	return (1);
}

static int
l_inet6_rth_ext(lua_State *L)
{
	void *ext;
	socklen_t space;
	int type, segments;

	type = luaL_checkinteger(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	segments = luaL_len(L, 2);

	if ((space = inet6_rth_space(type, segments)) == 0) {
		return (fail(L, EINVAL));
	}
	if ((ext = malloc(space)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (lua_Integer i = 1; i <= segments; i++) {
		const struct in6_addr *addr;
		size_t len;

		if (lua_geti(L, 2, i) != LUA_TSTRING) {
			goto bad;
		}
		addr = (const struct in6_addr *)lua_tolstring(L, -1, &len);
		if (len != sizeof(*addr)) {
			goto bad;
		}
		if (inet6_rth_add(ext, addr) == -1) {
			goto bad;
		}
		lua_pop(L, 1);
	}
	lua_pushlstring(L, ext, space);
	return (1);
bad:
	free(ext);
	return (luaL_argerror(L, 2, "invalid segments"));
}

static int
l_inet6_rth_reverse(lua_State *L)
{
	const void *src;
	void *dst;
	size_t len;

	src = luaL_checklstring(L, 1, &len);

	if ((dst = malloc(len)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if (inet6_rth_reverse(src, dst) == -1) {
		free(dst);
		return (fail(L, EINVAL));
	}
	lua_pushlstring(L, dst, len);
	free(dst);
	return (1);
}

static int
l_inet6_ext_rth(lua_State *L)
{
	const void *ext;
	int segments;

	ext = luaL_checkstring(L, 1);

	if ((segments = inet6_rth_segments(ext)) == -1) {
		return (fail(L, EINVAL));
	}
	lua_createtable(L, segments, 0);
	for (int i = 0; i < segments; i++) {
		const struct in6_addr *addr;

		if ((addr = inet6_rth_getaddr(ext, i)) == NULL) {
			return (fail(L, EINVAL));
		}
		lua_pushlstring(L, (const char *)addr, sizeof(*addr));
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static const struct luaL_Reg l_in_funcs[] = {
	{"sockaddr_in", l_sockaddr_in},
	{"sockaddr_in6", l_sockaddr_in6},
#define IN_ADDR_TEST(_, name) {"in_"#name, l_in_##name},
	IN_ADDR_TESTS(IN_ADDR_TEST)
#undef IN_ADDR_TEST
	{"in6_are_addr_equal", l_in6_are_addr_equal},
#define IN6_ADDR_TEST(_, name) {"in6_is_addr_"#name, l_in6_is_addr_##name},
	IN6_ADDR_TESTS(IN6_ADDR_TEST)
#undef IN6_ADDR_TEST
	{"setsourcefilter", l_setsourcefilter},
	{"getsourcefilter", l_getsourcefilter},
	{"inet6_opts_ext", l_inet6_opts_ext},
	{"inet6_ext_opts", l_inet6_ext_opts},
	{"inet6_rth_ext", l_inet6_rth_ext},
	{"inet6_rth_reverse", l_inet6_rth_reverse},
	{"inet6_ext_rth", l_inet6_ext_rth},
	{NULL, NULL}
};

int
luaopen_netinet_in(lua_State *L)
{
	luaL_newlib(L, l_in_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(IPPROTO_IP);
	SETCONST(IPPROTO_ICMP);
	SETCONST(IPPROTO_TCP);
	SETCONST(IPPROTO_UDP);

	SETCONST(INADDR_ANY);
	SETCONST(INADDR_BROADCAST);

	SETCONST(IPPROTO_IPV6);
	SETCONST(IPPROTO_RAW);
	SETCONST(INET_ADDRSTRLEN);

	SETCONST(IPPROTO_HOPOPTS);
	SETCONST(IPPROTO_IGMP);
	SETCONST(IPPROTO_GGP);
	SETCONST(IPPROTO_IPV4);
	SETCONST(IPPROTO_ST);
	SETCONST(IPPROTO_EGP);
	SETCONST(IPPROTO_PIGP);
	SETCONST(IPPROTO_RCCMON);
	SETCONST(IPPROTO_NVPII);
	SETCONST(IPPROTO_PUP);
	SETCONST(IPPROTO_ARGUS);
	SETCONST(IPPROTO_EMCON);
	SETCONST(IPPROTO_XNET);
	SETCONST(IPPROTO_CHAOS);
	SETCONST(IPPROTO_MUX);
	SETCONST(IPPROTO_MEAS);
	SETCONST(IPPROTO_HMP);
	SETCONST(IPPROTO_PRM);
	SETCONST(IPPROTO_IDP);
	SETCONST(IPPROTO_TRUNK1);
	SETCONST(IPPROTO_TRUNK2);
	SETCONST(IPPROTO_LEAF1);
	SETCONST(IPPROTO_LEAF2);
	SETCONST(IPPROTO_RDP);
	SETCONST(IPPROTO_IRTP);
	SETCONST(IPPROTO_TP);
	SETCONST(IPPROTO_BLT);
	SETCONST(IPPROTO_NSP);
	SETCONST(IPPROTO_INP);
	SETCONST(IPPROTO_DCCP);
	SETCONST(IPPROTO_3PC);
	SETCONST(IPPROTO_CMTP);
	SETCONST(IPPROTO_TPXX);
	SETCONST(IPPROTO_IL);
	SETCONST(IPPROTO_SDRP);
	SETCONST(IPPROTO_ROUTING);
	SETCONST(IPPROTO_FRAGMENT);
	SETCONST(IPPROTO_IDRP);
	SETCONST(IPPROTO_RSVP);
	SETCONST(IPPROTO_GRE);
	SETCONST(IPPROTO_MHRP);
	SETCONST(IPPROTO_BHA);
	SETCONST(IPPROTO_ESP);
	SETCONST(IPPROTO_AH);
	SETCONST(IPPROTO_INLSP);
	SETCONST(IPPROTO_SWIPE);
	SETCONST(IPPROTO_NHRP);
	SETCONST(IPPROTO_MOBILE);
	SETCONST(IPPROTO_TLSP);
	SETCONST(IPPROTO_SKIP);
	SETCONST(IPPROTO_ICMPV6);
	SETCONST(IPPROTO_NONE);
	SETCONST(IPPROTO_DSTOPTS);
	SETCONST(IPPROTO_AHIP);
	SETCONST(IPPROTO_CFTP);
	SETCONST(IPPROTO_HELLO);
	SETCONST(IPPROTO_SATEXPAK);
	SETCONST(IPPROTO_KRYPTOLAN);
	SETCONST(IPPROTO_RVD);
	SETCONST(IPPROTO_IPPC);
	SETCONST(IPPROTO_ADFS);
	SETCONST(IPPROTO_SATMON);
	SETCONST(IPPROTO_VISA);
	SETCONST(IPPROTO_IPCV);
	SETCONST(IPPROTO_CPNX);
	SETCONST(IPPROTO_CPHB);
	SETCONST(IPPROTO_WSN);
	SETCONST(IPPROTO_PVP);
	SETCONST(IPPROTO_BRSATMON);
	SETCONST(IPPROTO_ND);
	SETCONST(IPPROTO_WBMON);
	SETCONST(IPPROTO_WBEXPAK);
	SETCONST(IPPROTO_EON);
	SETCONST(IPPROTO_VMTP);
	SETCONST(IPPROTO_SVMTP);
	SETCONST(IPPROTO_VINES);
	SETCONST(IPPROTO_TTP);
	SETCONST(IPPROTO_IGP);
	SETCONST(IPPROTO_DGP);
	SETCONST(IPPROTO_TCF);
	SETCONST(IPPROTO_IGRP);
	SETCONST(IPPROTO_OSPFIGP);
	SETCONST(IPPROTO_SRPC);
	SETCONST(IPPROTO_LARP);
	SETCONST(IPPROTO_MTP);
	SETCONST(IPPROTO_AX25);
	SETCONST(IPPROTO_IPEIP);
	SETCONST(IPPROTO_MICP);
	SETCONST(IPPROTO_SCCSP);
	SETCONST(IPPROTO_ETHERIP);
	SETCONST(IPPROTO_ENCAP);
	SETCONST(IPPROTO_APES);
	SETCONST(IPPROTO_GMTP);
	SETCONST(IPPROTO_IPCOMP);
	SETCONST(IPPROTO_SCTP);
	SETCONST(IPPROTO_MH);
	SETCONST(IPPROTO_UDPLITE);
	SETCONST(IPPROTO_HIP);
	SETCONST(IPPROTO_HIP);
	SETCONST(IPPROTO_SHIM6);
	SETCONST(IPPROTO_PIM);
	SETCONST(IPPROTO_CARP);
	SETCONST(IPPROTO_PGM);
	SETCONST(IPPROTO_MPLS);
	SETCONST(IPPROTO_PFSYNC);
	SETCONST(IPPROTO_RESERVED_253);
	SETCONST(IPPROTO_RESERVED_254);
	SETCONST(IPPROTO_OLD_DIVERT);
	SETCONST(IPPROTO_MAX);
	SETCONST(IPPROTO_DIVERT);
	SETCONST(IPPROTO_SEND);
	SETCONST(IPPROTO_SPACER);

	SETCONST(IPPORT_RESERVED);
	SETCONST(IPPORT_EPHEMERALFIRST);
	SETCONST(IPPORT_EPHEMERALLAST);
	SETCONST(IPPORT_HIFIRSTAUTO);
	SETCONST(IPPORT_HILASTAUTO);
	SETCONST(IPPORT_RESERVEDSTART);
	SETCONST(IPPORT_MAX);

	SETCONST(IN_NETMASK_DEFAULT);

	SETCONST(INADDR_LOOPBACK);
	SETCONST(INADDR_NONE);
	SETCONST(INADDR_UNSPEC_GROUP);
	SETCONST(INADDR_ALLHOSTS_GROUP);
	SETCONST(INADDR_ALLRTRS_GROUP);
	SETCONST(INADDR_ALLRPTS_GROUP);
	SETCONST(INADDR_CARP_GROUP);
	SETCONST(INADDR_PFSYNC_GROUP);
	SETCONST(INADDR_ALLMDNS_GROUP);
	SETCONST(INADDR_MAX_LOCAL_GROUP);

	SETCONST(IN_RFC3021_MASK);

	SETCONST(IP_OPTIONS);
	SETCONST(IP_HDRINCL);
	SETCONST(IP_TOS);
	SETCONST(IP_TTL);
	SETCONST(IP_RECVOPTS);
	SETCONST(IP_RECVRETOPTS);
	SETCONST(IP_RECVDSTADDR);
	SETCONST(IP_SENDSRCADDR);
	SETCONST(IP_RETOPTS);
	SETCONST(IP_MULTICAST_IF);
	SETCONST(IP_MULTICAST_TTL);
	SETCONST(IP_MULTICAST_LOOP);
	SETCONST(IP_ADD_MEMBERSHIP);
	SETCONST(IP_DROP_MEMBERSHIP);
	SETCONST(IP_MULTICAST_VIF);
	SETCONST(IP_RSVP_ON);
	SETCONST(IP_RSVP_OFF);
	SETCONST(IP_RSVP_VIF_ON);
	SETCONST(IP_RSVP_VIF_OFF);
	SETCONST(IP_PORTRANGE);
	SETCONST(IP_RECVIF);
	SETCONST(IP_IPSEC_POLICY);
	SETCONST(IP_ONESBCAST);
	SETCONST(IP_BINDANY);
	SETCONST(IP_ORIGDSTADDR);
	SETCONST(IP_RECVORIGDSTADDR);
	/* Historical IP_FW_* and IP_DUMMYNET_* options 40-64 omitted. */
	SETCONST(IP_FW3);
	SETCONST(IP_DUMMYNET3);
	SETCONST(IP_RECVTTL);
	SETCONST(IP_MINTTL);
	SETCONST(IP_DONTFRAG);
	SETCONST(IP_RECVTOS);
	SETCONST(IP_ADD_SOURCE_MEMBERSHIP);
	SETCONST(IP_DROP_SOURCE_MEMBERSHIP);
	SETCONST(IP_BLOCK_SOURCE);
	SETCONST(IP_UNBLOCK_SOURCE);
	SETCONST(IP_MSFILTER);
	SETCONST(IP_VLAN_PCP);

	SETCONST(MCAST_JOIN_GROUP);
	SETCONST(MCAST_LEAVE_GROUP);
	SETCONST(MCAST_JOIN_SOURCE_GROUP);
	SETCONST(MCAST_LEAVE_SOURCE_GROUP);
	SETCONST(MCAST_BLOCK_SOURCE);
	SETCONST(MCAST_UNBLOCK_SOURCE);

	SETCONST(IP_FLOWID);
	SETCONST(IP_FLOWTYPE);
	SETCONST(IP_RSSBUCKETID);
	SETCONST(IP_RECVFLOWID);
	SETCONST(IP_RECVRSSBUCKETID);

	SETCONST(IP_DEFAULT_MULTICAST_TTL);
	SETCONST(IP_DEFAULT_MULTICAST_LOOP);

	SETCONST(IP_MAX_MEMBERSHIPS);

	SETCONST(IP_MAX_GROUP_SRC_FILTER);
	SETCONST(IP_MAX_SOCK_SRC_FILTER);
	SETCONST(IP_MAX_SOCK_MUTE_FILTER);

	SETCONST(MCAST_UNDEFINED);
	SETCONST(MCAST_INCLUDE);
	SETCONST(MCAST_EXCLUDE);

	SETCONST(IP_PORTRANGE_DEFAULT);
	SETCONST(IP_PORTRANGE_HIGH);
	SETCONST(IP_PORTRANGE_LOW);

	SETCONST(IPCTL_FORWARDING);
	SETCONST(IPCTL_SENDREDIRECTS);
	SETCONST(IPCTL_DEFTTL);
	SETCONST(IPCTL_SOURCEROUTE);
	SETCONST(IPCTL_DIRECTEDBROADCAST);
	SETCONST(IPCTL_INTRQMAXLEN);
	SETCONST(IPCTL_INTRQDROPS);
	SETCONST(IPCTL_STATS);
	SETCONST(IPCTL_ACCEPTSOURCEROUTE);
	SETCONST(IPCTL_FASTFORWARDING);
	SETCONST(IPCTL_GIF_TTL);
	SETCONST(IPCTL_INTRDQMAXLEN);
	SETCONST(IPCTL_INTRDQDROPS);

	SETCONST(IPV6PORT_RESERVED);
	SETCONST(IPV6PORT_ANONMIN);
	SETCONST(IPV6PORT_ANONMAX);
	SETCONST(IPV6PORT_RESERVEDMIN);
	SETCONST(IPV6PORT_RESERVEDMAX);

	SETCONST(INET6_ADDRSTRLEN);

	/* SIN6_LEN definition looks wrong? */

	SETCONST(IPV6_SOCKOPT_RESERVED1);
	SETCONST(IPV6_UNICAST_HOPS);
	SETCONST(IPV6_MULTICAST_IF);
	SETCONST(IPV6_MULTICAST_HOPS);
	SETCONST(IPV6_MULTICAST_LOOP);
	SETCONST(IPV6_JOIN_GROUP);
	SETCONST(IPV6_LEAVE_GROUP);
	SETCONST(IPV6_PORTRANGE);
	SETCONST(ICMP6_FILTER);
	SETCONST(IPV6_CHECKSUM);
	SETCONST(IPV6_V6ONLY);
	SETCONST(IPV6_BINDV6ONLY);
	SETCONST(IPV6_IPSEC_POLICY);
	/* Historical IPV6_FW_* options 30-34 omitted. */
	SETCONST(IPV6_RTHDRDSTOPTS);
	SETCONST(IPV6_RECVPKTINFO);
	SETCONST(IPV6_RECVHOPLIMIT);
	SETCONST(IPV6_RECVRTHDR);
	SETCONST(IPV6_RECVHOPOPTS);
	SETCONST(IPV6_RECVDSTOPTS);
	SETCONST(IPV6_USE_MIN_MTU);
	SETCONST(IPV6_RECVPATHMTU);
	SETCONST(IPV6_PATHMTU);
	SETCONST(IPV6_PKTINFO);
	SETCONST(IPV6_HOPLIMIT);
	SETCONST(IPV6_NEXTHOP);
	SETCONST(IPV6_HOPOPTS);
	SETCONST(IPV6_DSTOPTS);
	SETCONST(IPV6_RTHDR);
	SETCONST(IPV6_RECVTCLASS);
	SETCONST(IPV6_AUTOFLOWLABEL);
	SETCONST(IPV6_TCLASS);
	SETCONST(IPV6_DONTFRAG);
	SETCONST(IPV6_PREFER_TEMPADDR);
	SETCONST(IPV6_BINDANY);
#ifdef IPV6_BINDMULTI /* removed */
	SETCONST(IPV6_BINDMULTI);
#endif
#ifdef IPV6_RSS_LISTEN_BUCKET /* removed */
	SETCONST(IPV6_RSS_LISTEN_BUCKET);
#endif
	SETCONST(IPV6_FLOWID);
	SETCONST(IPV6_FLOWTYPE);
	SETCONST(IPV6_RSSBUCKETID);
	SETCONST(IPV6_RECVFLOWID);
	SETCONST(IPV6_RECVRSSBUCKETID);
	SETCONST(IPV6_ORIGDSTADDR);
	SETCONST(IPV6_RECVORIGDSTADDR);
	SETCONST(IPV6_VLAN_PCP);

	SETCONST(IPV6_RTHDR_LOOSE);
	SETCONST(IPV6_RTHDR_STRICT);
	SETCONST(IPV6_RTHDR_TYPE_0);

	SETCONST(IPV6_DEFAULT_MULTICAST_HOPS);
	SETCONST(IPV6_DEFAULT_MULTICAST_LOOP);

	SETCONST(IPV6_MAX_MEMBERSHIPS);
	SETCONST(IPV6_MAX_GROUP_SRC_FILTER);
	SETCONST(IPV6_MAX_SOCK_SRC_FILTER);

	SETCONST(IPV6_PORTRANGE_DEFAULT);
	SETCONST(IPV6_PORTRANGE_HIGH);
	SETCONST(IPV6_PORTRANGE_LOW);

	SETCONST(IPV6PROTO_MAXID);
	SETCONST(IPV6CTL_FORWARDING);
	SETCONST(IPV6CTL_SENDREDIRECTS);
	SETCONST(IPV6CTL_DEFHLIM);
	SETCONST(IPV6CTL_FORWSRCRT);
	SETCONST(IPV6CTL_STATS);
	SETCONST(IPV6CTL_MRTSTATS);
	SETCONST(IPV6CTL_MRTPROTO);
	SETCONST(IPV6CTL_MAXFRAGPACKETS);
	SETCONST(IPV6CTL_SOURCECHECK);
	SETCONST(IPV6CTL_SOURCECHECK_LOGINT);
	SETCONST(IPV6CTL_ACCEPT_RTADV);
	SETCONST(IPV6CTL_LOG_INTERVAL);
	SETCONST(IPV6CTL_HDRNESTLIMIT);
	SETCONST(IPV6CTL_DAD_COUNT);
	SETCONST(IPV6CTL_AUTO_FLOWLABEL);
	SETCONST(IPV6CTL_DEFMCASTHLIM);
	SETCONST(IPV6CTL_GIF_HLIM);
	SETCONST(IPV6CTL_KAME_VERSION);
	SETCONST(IPV6CTL_USE_DEPRECATED);
	SETCONST(IPV6CTL_RR_PRUNE);
	SETCONST(IPV6CTL_V6ONLY);
#ifdef IPV6CTL_STABLEADDR_NETIFSRC
	SETCONST(IPV6CTL_STABLEADDR_NETIFSRC);
#endif
#ifdef IPV6CTL_STABLEADDR_MAXRETRIES
	SETCONST(IPV6CTL_STABLEADDR_MAXRETRIES);
#endif
	SETCONST(IPV6CTL_USETEMPADDR);
	SETCONST(IPV6CTL_TEMPPLTIME);
	SETCONST(IPV6CTL_TEMPVLTIME);
	SETCONST(IPV6CTL_AUTO_LINKLOCAL);
	SETCONST(IPV6CTL_RIP6STATS);
	SETCONST(IPV6CTL_PREFER_TEMPADDR);
	SETCONST(IPV6CTL_ADDRCTLPOLICY);
	SETCONST(IPV6CTL_USE_DEFAULTZONE);
#ifdef IPV6CTL_USESTABLEADDR
	SETCONST(IPV6CTL_USESTABLEADDR);
#endif
	SETCONST(IPV6CTL_MAXFRAGS);
	SETCONST(IPV6CTL_MCAST_PMTU);
	SETCONST(IPV6CTL_STEALTH);
	SETCONST(ICMPV6CTL_ND6_ONLINKNSRFC4861);
	SETCONST(IPV6CTL_NO_RADR);
	SETCONST(IPV6CTL_NORBIT_RAIF);
	SETCONST(IPV6CTL_RFC6204W3);
	SETCONST(IPV6CTL_INTRQMAXLEN);
	SETCONST(IPV6CTL_INTRDQMAXLEN);
	SETCONST(IPV6CTL_MAXFRAGSPERPACKET);
	SETCONST(IPV6CTL_MAXFRAGBUCKETSIZE);
	SETCONST(IPV6CTL_MAXID);

	/* M_* definitions aren't actually user visible */
#undef SETCONST
#define SETCONST(ident, INIT) ({ \
	const struct in6_addr ident = INIT; \
	lua_pushlstring(L, (const char *)&ident, sizeof(ident)); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(in6addr_any, IN6ADDR_ANY_INIT);
	SETCONST(in6addr_loopback, IN6ADDR_LOOPBACK_INIT);
	SETCONST(in6addr_nodelocal_allnodes, IN6ADDR_NODELOCAL_ALLNODES_INIT);
	SETCONST(in6add4_intfacelocal_allnodes,
	    IN6ADDR_INTFACELOCAL_ALLNODES_INIT);
	SETCONST(in6addr_linklocal_allnodes, IN6ADDR_LINKLOCAL_ALLNODES_INIT);
	SETCONST(in6addr_linklocal_allrouters,
	    IN6ADDR_LINKLOCAL_ALLROUTERS_INIT);
	SETCONST(in6addr_linklocal_allv2routers,
	    IN6ADDR_LINKLOCAL_ALLV2ROUTERS_INIT);
#undef SETCONST
	return (1);
}
