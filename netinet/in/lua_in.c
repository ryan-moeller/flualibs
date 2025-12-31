/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <netinet/in.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

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
		int error = errno;

		free(slist);
		return (fail(L, error));
	}
	free(slist);
	return (success(L));
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
		int error = errno;

		free(slist);
		return (fail(L, error));
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
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(IPPROTO_IP);
	DEFINE(IPPROTO_ICMP);
	DEFINE(IPPROTO_TCP);
	DEFINE(IPPROTO_UDP);

	DEFINE(INADDR_ANY);
	DEFINE(INADDR_BROADCAST);

	DEFINE(IPPROTO_IPV6);
	DEFINE(IPPROTO_RAW);
	DEFINE(INET_ADDRSTRLEN);

	DEFINE(IPPROTO_HOPOPTS);
	DEFINE(IPPROTO_IGMP);
	DEFINE(IPPROTO_GGP);
	DEFINE(IPPROTO_IPV4);
	DEFINE(IPPROTO_ST);
	DEFINE(IPPROTO_EGP);
	DEFINE(IPPROTO_PIGP);
	DEFINE(IPPROTO_RCCMON);
	DEFINE(IPPROTO_NVPII);
	DEFINE(IPPROTO_PUP);
	DEFINE(IPPROTO_ARGUS);
	DEFINE(IPPROTO_EMCON);
	DEFINE(IPPROTO_XNET);
	DEFINE(IPPROTO_CHAOS);
	DEFINE(IPPROTO_MUX);
	DEFINE(IPPROTO_MEAS);
	DEFINE(IPPROTO_HMP);
	DEFINE(IPPROTO_PRM);
	DEFINE(IPPROTO_IDP);
	DEFINE(IPPROTO_TRUNK1);
	DEFINE(IPPROTO_TRUNK2);
	DEFINE(IPPROTO_LEAF1);
	DEFINE(IPPROTO_LEAF2);
	DEFINE(IPPROTO_RDP);
	DEFINE(IPPROTO_IRTP);
	DEFINE(IPPROTO_TP);
	DEFINE(IPPROTO_BLT);
	DEFINE(IPPROTO_NSP);
	DEFINE(IPPROTO_INP);
	DEFINE(IPPROTO_DCCP);
	DEFINE(IPPROTO_3PC);
	DEFINE(IPPROTO_CMTP);
	DEFINE(IPPROTO_TPXX);
	DEFINE(IPPROTO_IL);
	DEFINE(IPPROTO_SDRP);
	DEFINE(IPPROTO_ROUTING);
	DEFINE(IPPROTO_FRAGMENT);
	DEFINE(IPPROTO_IDRP);
	DEFINE(IPPROTO_RSVP);
	DEFINE(IPPROTO_GRE);
	DEFINE(IPPROTO_MHRP);
	DEFINE(IPPROTO_BHA);
	DEFINE(IPPROTO_ESP);
	DEFINE(IPPROTO_AH);
	DEFINE(IPPROTO_INLSP);
	DEFINE(IPPROTO_SWIPE);
	DEFINE(IPPROTO_NHRP);
	DEFINE(IPPROTO_MOBILE);
	DEFINE(IPPROTO_TLSP);
	DEFINE(IPPROTO_SKIP);
	DEFINE(IPPROTO_ICMPV6);
	DEFINE(IPPROTO_NONE);
	DEFINE(IPPROTO_DSTOPTS);
	DEFINE(IPPROTO_AHIP);
	DEFINE(IPPROTO_CFTP);
	DEFINE(IPPROTO_HELLO);
	DEFINE(IPPROTO_SATEXPAK);
	DEFINE(IPPROTO_KRYPTOLAN);
	DEFINE(IPPROTO_RVD);
	DEFINE(IPPROTO_IPPC);
	DEFINE(IPPROTO_ADFS);
	DEFINE(IPPROTO_SATMON);
	DEFINE(IPPROTO_VISA);
	DEFINE(IPPROTO_IPCV);
	DEFINE(IPPROTO_CPNX);
	DEFINE(IPPROTO_CPHB);
	DEFINE(IPPROTO_WSN);
	DEFINE(IPPROTO_PVP);
	DEFINE(IPPROTO_BRSATMON);
	DEFINE(IPPROTO_ND);
	DEFINE(IPPROTO_WBMON);
	DEFINE(IPPROTO_WBEXPAK);
	DEFINE(IPPROTO_EON);
	DEFINE(IPPROTO_VMTP);
	DEFINE(IPPROTO_SVMTP);
	DEFINE(IPPROTO_VINES);
	DEFINE(IPPROTO_TTP);
	DEFINE(IPPROTO_IGP);
	DEFINE(IPPROTO_DGP);
	DEFINE(IPPROTO_TCF);
	DEFINE(IPPROTO_IGRP);
	DEFINE(IPPROTO_OSPFIGP);
	DEFINE(IPPROTO_SRPC);
	DEFINE(IPPROTO_LARP);
	DEFINE(IPPROTO_MTP);
	DEFINE(IPPROTO_AX25);
	DEFINE(IPPROTO_IPEIP);
	DEFINE(IPPROTO_MICP);
	DEFINE(IPPROTO_SCCSP);
	DEFINE(IPPROTO_ETHERIP);
	DEFINE(IPPROTO_ENCAP);
	DEFINE(IPPROTO_APES);
	DEFINE(IPPROTO_GMTP);
	DEFINE(IPPROTO_IPCOMP);
	DEFINE(IPPROTO_SCTP);
	DEFINE(IPPROTO_MH);
	DEFINE(IPPROTO_UDPLITE);
	DEFINE(IPPROTO_HIP);
	DEFINE(IPPROTO_HIP);
	DEFINE(IPPROTO_SHIM6);
	DEFINE(IPPROTO_PIM);
	DEFINE(IPPROTO_CARP);
	DEFINE(IPPROTO_PGM);
	DEFINE(IPPROTO_MPLS);
	DEFINE(IPPROTO_PFSYNC);
	DEFINE(IPPROTO_RESERVED_253);
	DEFINE(IPPROTO_RESERVED_254);
	DEFINE(IPPROTO_OLD_DIVERT);
	DEFINE(IPPROTO_MAX);
	DEFINE(IPPROTO_DIVERT);
	DEFINE(IPPROTO_SEND);
	DEFINE(IPPROTO_SPACER);

	DEFINE(IPPORT_RESERVED);
	DEFINE(IPPORT_EPHEMERALFIRST);
	DEFINE(IPPORT_EPHEMERALLAST);
	DEFINE(IPPORT_HIFIRSTAUTO);
	DEFINE(IPPORT_HILASTAUTO);
	DEFINE(IPPORT_RESERVEDSTART);
	DEFINE(IPPORT_MAX);

	DEFINE(IN_NETMASK_DEFAULT);

	DEFINE(INADDR_LOOPBACK);
	DEFINE(INADDR_NONE);
	DEFINE(INADDR_UNSPEC_GROUP);
	DEFINE(INADDR_ALLHOSTS_GROUP);
	DEFINE(INADDR_ALLRTRS_GROUP);
	DEFINE(INADDR_ALLRPTS_GROUP);
	DEFINE(INADDR_CARP_GROUP);
	DEFINE(INADDR_PFSYNC_GROUP);
	DEFINE(INADDR_ALLMDNS_GROUP);
	DEFINE(INADDR_MAX_LOCAL_GROUP);

	DEFINE(IN_RFC3021_MASK);

	DEFINE(IP_OPTIONS);
	DEFINE(IP_HDRINCL);
	DEFINE(IP_TOS);
	DEFINE(IP_TTL);
	DEFINE(IP_RECVOPTS);
	DEFINE(IP_RECVRETOPTS);
	DEFINE(IP_RECVDSTADDR);
	DEFINE(IP_SENDSRCADDR);
	DEFINE(IP_RETOPTS);
	DEFINE(IP_MULTICAST_IF);
	DEFINE(IP_MULTICAST_TTL);
	DEFINE(IP_MULTICAST_LOOP);
	DEFINE(IP_ADD_MEMBERSHIP);
	DEFINE(IP_DROP_MEMBERSHIP);
	DEFINE(IP_MULTICAST_VIF);
	DEFINE(IP_RSVP_ON);
	DEFINE(IP_RSVP_OFF);
	DEFINE(IP_RSVP_VIF_ON);
	DEFINE(IP_RSVP_VIF_OFF);
	DEFINE(IP_PORTRANGE);
	DEFINE(IP_RECVIF);
	DEFINE(IP_IPSEC_POLICY);
	DEFINE(IP_ONESBCAST);
	DEFINE(IP_BINDANY);
	DEFINE(IP_ORIGDSTADDR);
	DEFINE(IP_RECVORIGDSTADDR);
	/* Historical IP_FW_* and IP_DUMMYNET_* options 40-64 omitted. */
	DEFINE(IP_FW3);
	DEFINE(IP_DUMMYNET3);
	DEFINE(IP_RECVTTL);
	DEFINE(IP_MINTTL);
	DEFINE(IP_DONTFRAG);
	DEFINE(IP_RECVTOS);
	DEFINE(IP_ADD_SOURCE_MEMBERSHIP);
	DEFINE(IP_DROP_SOURCE_MEMBERSHIP);
	DEFINE(IP_BLOCK_SOURCE);
	DEFINE(IP_UNBLOCK_SOURCE);
	DEFINE(IP_MSFILTER);
	DEFINE(IP_VLAN_PCP);

	DEFINE(MCAST_JOIN_GROUP);
	DEFINE(MCAST_LEAVE_GROUP);
	DEFINE(MCAST_JOIN_SOURCE_GROUP);
	DEFINE(MCAST_LEAVE_SOURCE_GROUP);
	DEFINE(MCAST_BLOCK_SOURCE);
	DEFINE(MCAST_UNBLOCK_SOURCE);

	DEFINE(IP_FLOWID);
	DEFINE(IP_FLOWTYPE);
	DEFINE(IP_RSSBUCKETID);
	DEFINE(IP_RECVFLOWID);
	DEFINE(IP_RECVRSSBUCKETID);

	DEFINE(IP_DEFAULT_MULTICAST_TTL);
	DEFINE(IP_DEFAULT_MULTICAST_LOOP);

	DEFINE(IP_MAX_MEMBERSHIPS);

	DEFINE(IP_MAX_GROUP_SRC_FILTER);
	DEFINE(IP_MAX_SOCK_SRC_FILTER);
	DEFINE(IP_MAX_SOCK_MUTE_FILTER);

	DEFINE(MCAST_UNDEFINED);
	DEFINE(MCAST_INCLUDE);
	DEFINE(MCAST_EXCLUDE);

	DEFINE(IP_PORTRANGE_DEFAULT);
	DEFINE(IP_PORTRANGE_HIGH);
	DEFINE(IP_PORTRANGE_LOW);

	DEFINE(IPCTL_FORWARDING);
	DEFINE(IPCTL_SENDREDIRECTS);
	DEFINE(IPCTL_DEFTTL);
	DEFINE(IPCTL_SOURCEROUTE);
	DEFINE(IPCTL_DIRECTEDBROADCAST);
	DEFINE(IPCTL_INTRQMAXLEN);
	DEFINE(IPCTL_INTRQDROPS);
	DEFINE(IPCTL_STATS);
	DEFINE(IPCTL_ACCEPTSOURCEROUTE);
	DEFINE(IPCTL_FASTFORWARDING);
	DEFINE(IPCTL_GIF_TTL);
	DEFINE(IPCTL_INTRDQMAXLEN);
	DEFINE(IPCTL_INTRDQDROPS);

	DEFINE(IPV6PORT_RESERVED);
	DEFINE(IPV6PORT_ANONMIN);
	DEFINE(IPV6PORT_ANONMAX);
	DEFINE(IPV6PORT_RESERVEDMIN);
	DEFINE(IPV6PORT_RESERVEDMAX);

	DEFINE(INET6_ADDRSTRLEN);

	/* SIN6_LEN definition looks wrong? */

	DEFINE(IPV6_SOCKOPT_RESERVED1);
	DEFINE(IPV6_UNICAST_HOPS);
	DEFINE(IPV6_MULTICAST_IF);
	DEFINE(IPV6_MULTICAST_HOPS);
	DEFINE(IPV6_MULTICAST_LOOP);
	DEFINE(IPV6_JOIN_GROUP);
	DEFINE(IPV6_LEAVE_GROUP);
	DEFINE(IPV6_PORTRANGE);
	DEFINE(ICMP6_FILTER);
	DEFINE(IPV6_CHECKSUM);
	DEFINE(IPV6_V6ONLY);
	DEFINE(IPV6_BINDV6ONLY);
	DEFINE(IPV6_IPSEC_POLICY);
	/* Historical IPV6_FW_* options 30-34 omitted. */
	DEFINE(IPV6_RTHDRDSTOPTS);
	DEFINE(IPV6_RECVPKTINFO);
	DEFINE(IPV6_RECVHOPLIMIT);
	DEFINE(IPV6_RECVRTHDR);
	DEFINE(IPV6_RECVHOPOPTS);
	DEFINE(IPV6_RECVDSTOPTS);
	DEFINE(IPV6_USE_MIN_MTU);
	DEFINE(IPV6_RECVPATHMTU);
	DEFINE(IPV6_PATHMTU);
	DEFINE(IPV6_PKTINFO);
	DEFINE(IPV6_HOPLIMIT);
	DEFINE(IPV6_NEXTHOP);
	DEFINE(IPV6_HOPOPTS);
	DEFINE(IPV6_DSTOPTS);
	DEFINE(IPV6_RTHDR);
	DEFINE(IPV6_RECVTCLASS);
	DEFINE(IPV6_AUTOFLOWLABEL);
	DEFINE(IPV6_TCLASS);
	DEFINE(IPV6_DONTFRAG);
	DEFINE(IPV6_PREFER_TEMPADDR);
	DEFINE(IPV6_BINDANY);
#ifdef IPV6_BINDMULTI /* removed */
	DEFINE(IPV6_BINDMULTI);
#endif
#ifdef IPV6_RSS_LISTEN_BUCKET /* removed */
	DEFINE(IPV6_RSS_LISTEN_BUCKET);
#endif
	DEFINE(IPV6_FLOWID);
	DEFINE(IPV6_FLOWTYPE);
	DEFINE(IPV6_RSSBUCKETID);
	DEFINE(IPV6_RECVFLOWID);
	DEFINE(IPV6_RECVRSSBUCKETID);
	DEFINE(IPV6_ORIGDSTADDR);
	DEFINE(IPV6_RECVORIGDSTADDR);
	DEFINE(IPV6_VLAN_PCP);

	DEFINE(IPV6_RTHDR_LOOSE);
	DEFINE(IPV6_RTHDR_STRICT);
	DEFINE(IPV6_RTHDR_TYPE_0);

	DEFINE(IPV6_DEFAULT_MULTICAST_HOPS);
	DEFINE(IPV6_DEFAULT_MULTICAST_LOOP);

	DEFINE(IPV6_MAX_MEMBERSHIPS);
	DEFINE(IPV6_MAX_GROUP_SRC_FILTER);
	DEFINE(IPV6_MAX_SOCK_SRC_FILTER);

	DEFINE(IPV6_PORTRANGE_DEFAULT);
	DEFINE(IPV6_PORTRANGE_HIGH);
	DEFINE(IPV6_PORTRANGE_LOW);

	DEFINE(IPV6PROTO_MAXID);
	DEFINE(IPV6CTL_FORWARDING);
	DEFINE(IPV6CTL_SENDREDIRECTS);
	DEFINE(IPV6CTL_DEFHLIM);
	DEFINE(IPV6CTL_FORWSRCRT);
	DEFINE(IPV6CTL_STATS);
	DEFINE(IPV6CTL_MRTSTATS);
	DEFINE(IPV6CTL_MRTPROTO);
	DEFINE(IPV6CTL_MAXFRAGPACKETS);
	DEFINE(IPV6CTL_SOURCECHECK);
	DEFINE(IPV6CTL_SOURCECHECK_LOGINT);
	DEFINE(IPV6CTL_ACCEPT_RTADV);
	DEFINE(IPV6CTL_LOG_INTERVAL);
	DEFINE(IPV6CTL_HDRNESTLIMIT);
	DEFINE(IPV6CTL_DAD_COUNT);
	DEFINE(IPV6CTL_AUTO_FLOWLABEL);
	DEFINE(IPV6CTL_DEFMCASTHLIM);
	DEFINE(IPV6CTL_GIF_HLIM);
	DEFINE(IPV6CTL_KAME_VERSION);
	DEFINE(IPV6CTL_USE_DEPRECATED);
	DEFINE(IPV6CTL_RR_PRUNE);
	DEFINE(IPV6CTL_V6ONLY);
#ifdef IPV6CTL_STABLEADDR_NETIFSRC
	DEFINE(IPV6CTL_STABLEADDR_NETIFSRC);
#endif
#ifdef IPV6CTL_STABLEADDR_MAXRETRIES
	DEFINE(IPV6CTL_STABLEADDR_MAXRETRIES);
#endif
	DEFINE(IPV6CTL_USETEMPADDR);
	DEFINE(IPV6CTL_TEMPPLTIME);
	DEFINE(IPV6CTL_TEMPVLTIME);
	DEFINE(IPV6CTL_AUTO_LINKLOCAL);
	DEFINE(IPV6CTL_RIP6STATS);
	DEFINE(IPV6CTL_PREFER_TEMPADDR);
	DEFINE(IPV6CTL_ADDRCTLPOLICY);
	DEFINE(IPV6CTL_USE_DEFAULTZONE);
#ifdef IPV6CTL_USESTABLEADDR
	DEFINE(IPV6CTL_USESTABLEADDR);
#endif
	DEFINE(IPV6CTL_MAXFRAGS);
	DEFINE(IPV6CTL_MCAST_PMTU);
	DEFINE(IPV6CTL_STEALTH);
	DEFINE(ICMPV6CTL_ND6_ONLINKNSRFC4861);
	DEFINE(IPV6CTL_NO_RADR);
	DEFINE(IPV6CTL_NORBIT_RAIF);
	DEFINE(IPV6CTL_RFC6204W3);
	DEFINE(IPV6CTL_INTRQMAXLEN);
	DEFINE(IPV6CTL_INTRDQMAXLEN);
	DEFINE(IPV6CTL_MAXFRAGSPERPACKET);
	DEFINE(IPV6CTL_MAXFRAGBUCKETSIZE);
	DEFINE(IPV6CTL_MAXID);

	/* M_* definitions aren't actually user visible */
#undef DEFINE
#define DEFINE(ident, INIT) ({ \
	const struct in6_addr ident = INIT; \
	lua_pushlstring(L, (const char *)&ident, sizeof(ident)); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(in6addr_any, IN6ADDR_ANY_INIT);
	DEFINE(in6addr_loopback, IN6ADDR_LOOPBACK_INIT);
	DEFINE(in6addr_nodelocal_allnodes, IN6ADDR_NODELOCAL_ALLNODES_INIT);
	DEFINE(in6add4_intfacelocal_allnodes,
	    IN6ADDR_INTFACELOCAL_ALLNODES_INIT);
	DEFINE(in6addr_linklocal_allnodes, IN6ADDR_LINKLOCAL_ALLNODES_INIT);
	DEFINE(in6addr_linklocal_allrouters,
	    IN6ADDR_LINKLOCAL_ALLROUTERS_INIT);
	DEFINE(in6addr_linklocal_allv2routers,
	    IN6ADDR_LINKLOCAL_ALLV2ROUTERS_INIT);
#undef DEFINE
	return (1);
}
