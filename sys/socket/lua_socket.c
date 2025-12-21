/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"
#include "utils.h"

int luaopen_sys_socket(lua_State *);

static inline void
pushaddr(lua_State *L, const struct sockaddr_storage *ss)
{
	const struct sockaddr *addr = (const struct sockaddr *)ss;

	lua_createtable(L, 0, 2);
	lua_pushinteger(L, addr->sa_family);
	lua_setfield(L, -2, "family");
	lua_pushlstring(L, addr->sa_data,
	    addr->sa_len - offsetof(struct sockaddr, sa_data));
	lua_setfield(L, -2, "data");
}

static inline void
checkaddr(lua_State *L, int idx, struct sockaddr_storage *ss)
{
	struct sockaddr *addr = (struct sockaddr *)ss;
	const char *data;
	size_t datalen;

	memset(ss, 0, sizeof(*ss));
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "family");
	luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid address family");
	addr->sa_family = lua_tointeger(L, -1);
	lua_getfield(L, idx, "data");
	luaL_argcheck(L, lua_isstring(L, -1), idx, "invalid address data");
	data = lua_tolstring(L, -1, &datalen);
	memcpy(addr->sa_data, data, datalen);
	addr->sa_len = datalen + offsetof(struct sockaddr, sa_data);
	lua_pop(L, 2);
}

static int
l_accept(lua_State *L)
{
	struct sockaddr_storage ss;
	struct sockaddr *addr;
	socklen_t addrlen;
	int s, new_s;

	addr = (struct sockaddr *)&ss;
	addrlen = sizeof(ss);

	s = checkfd(L, 1);
	if (lua_isinteger(L, 2)) {
		int flags;

		flags = lua_tointeger(L, 2);

		new_s = accept4(s, addr, &addrlen, flags);
	} else {
		new_s = accept(s, addr, &addrlen);
	}
	if (new_s == -1) {
		return (fail(L, errno));
	}
	assert(ss.ss_len == addrlen);
	lua_pushinteger(L, new_s);
	pushaddr(L, &ss);
	return (2);
}

static int
l_bind(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *addr;
	int s;

	addr = (const struct sockaddr *)&ss;

	s = checkfd(L, 1);
	checkaddr(L, 2, &ss);

	if (bind(s, addr, addr->sa_len) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_connect(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *addr;
	int s;

	addr = (const struct sockaddr *)&ss;

	s = checkfd(L, 1);
	checkaddr(L, 2, &ss);

	if (connect(s, addr, addr->sa_len) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_bindat(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *addr;
	int fd, s;

	addr = (const struct sockaddr *)&ss;

	fd = luaL_checkinteger(L, 1);
	s = checkfd(L, 2);
	checkaddr(L, 3, &ss);

	if (bindat(fd, s, addr, addr->sa_len) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_connectat(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *addr;
	int fd, s;

	addr = (const struct sockaddr *)&ss;

	fd = luaL_checkinteger(L, 1);
	s = checkfd(L, 2);
	checkaddr(L, 3, &ss);

	if (connectat(fd, s, addr, addr->sa_len) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_getpeername(lua_State *L)
{
	struct sockaddr_storage ss;
	struct sockaddr *name;
	socklen_t namelen;
	int s;

	name = (struct sockaddr *)&ss;
	namelen = sizeof(ss);

	s = checkfd(L, 1);

	if (getpeername(s, name, &namelen) == -1) {
		return (fail(L, errno));
	}
	pushaddr(L, &ss);
	return (1);
}

static int
l_getsockname(lua_State *L)
{
	struct sockaddr_storage ss;
	struct sockaddr *name;
	socklen_t namelen;
	int s;

	name = (struct sockaddr *)&ss;
	namelen = sizeof(ss);

	s = checkfd(L, 1);

	if (getsockname(s, name, &namelen) == -1) {
		return (fail(L, errno));
	}
	pushaddr(L, &ss);
	return (1);
}

static int
l_getsockopt(lua_State *L)
{
	void *optval;
	socklen_t optlen;
	int s, level, optname;

	s = checkfd(L, 1);
	level = luaL_checkinteger(L, 2);
	optname = luaL_checkinteger(L, 3);
	optlen = luaL_checkinteger(L, 4);

	if ((optval = malloc(optlen)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if (getsockopt(s, level, optname, optval, &optlen) == -1) {
		free(optval);
		return (fail(L, errno));
	}
	lua_pushlstring(L, optval, optlen);
	free(optval);
	return (1);
}

static int
l_listen(lua_State *L)
{
	int s, backlog;

	s = checkfd(L, 1);
	backlog = luaL_optinteger(L, 2, -1);

	if (listen(s, backlog) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_recv(lua_State *L)
{
	luaL_Buffer b;
	char *p;
	size_t n;
	ssize_t len;
	int s, flags;

	s = checkfd(L, 1);
	n = luaL_checkinteger(L, 2);
	flags = luaL_optinteger(L, 3, 0);

	p = luaL_buffinitsize(L, &b, n);
	if ((len = recv(s, p, n, flags)) == -1) {
		return (fail(L, errno));
	}
	luaL_pushresultsize(&b, len);
	return (1);
}

static int
l_recvfrom(lua_State *L)
{
	luaL_Buffer b;
	struct sockaddr_storage ss;
	struct sockaddr *from;
	char *p;
	socklen_t fromlen;
	size_t n;
	ssize_t len;
	int s, flags;

	from = (struct sockaddr *)&ss;
	fromlen = sizeof(ss);

	s = checkfd(L, 1);
	n = luaL_checkinteger(L, 2);
	flags = luaL_optinteger(L, 3, 0);

	p = luaL_buffinitsize(L, &b, n);
	if ((len = recvfrom(s, p, n, flags, from, &fromlen)) == -1) {
		return (fail(L, errno));
	}
	assert(ss.ss_len == fromlen);
	luaL_pushresultsize(&b, len);
	pushaddr(L, &ss);
	return (1);
}

static int
l_recvmsg(lua_State *L)
{
	return (luaL_error(L, "TODO"));
}

static int
l_recvmmsg(lua_State *L)
{
	return (luaL_error(L, "TODO"));
}

static int
l_send(lua_State *L)
{
	const char *p;
	size_t len;
	ssize_t n;
	int s, flags;

	s = checkfd(L, 1);
	p = luaL_checklstring(L, 2, &len);
	flags = luaL_optinteger(L, 3, 0);

	if ((n = send(s, p, len, flags)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, n);
	return (1);
}

static int
l_sendto(lua_State *L)
{
	struct sockaddr_storage ss;
	const struct sockaddr *to;
	const char *p;
	size_t len;
	ssize_t n;
	int s, flags;

	to = (const struct sockaddr *)&ss;

	s = checkfd(L, 1);
	p = luaL_checklstring(L, 2, &len);
	if (lua_isinteger(L, 3)) {
		flags = lua_tointeger(L, 3);
		checkaddr(L, 4, &ss);
	} else {
		flags = 0;
		checkaddr(L, 3, &ss);
	}

	if ((n = sendto(s, p, len, flags, to, to->sa_len)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, n);
	return (1);
}

static int
l_sendmsg(lua_State *L)
{
	return (luaL_error(L, "TODO"));
}

static int
get_hdtr(lua_State *L, int arg, struct sf_hdtr *hdtr)
{
	if (lua_getfield(L, arg, "headers") == LUA_TTABLE) {
		int t = lua_gettop(L);

		hdtr->hdr_cnt = luaL_len(L, t);
		if (hdtr->hdr_cnt == 0) {
			hdtr->headers = NULL;
		} else {
			hdtr->headers =
			    malloc(hdtr->hdr_cnt * sizeof(*hdtr->headers));
			if (hdtr->headers == NULL) {
				return (fatal(L, "malloc", ENOMEM));
			}
		}
		for (int i = 0; i < hdtr->hdr_cnt; i++) {
			if (lua_geti(L, t, i + 1) != LUA_TSTRING) {
				free(hdtr->headers);
				return (luaL_error(L,
				    "headers must be strings"));
			}
			hdtr->headers[i].iov_base = __DECONST(char *,
			    luaL_tolstring(L, -1, &hdtr->headers[i].iov_len));
			assert(hdtr->headers[i].iov_base != NULL);
		}
	} else {
		hdtr->headers = NULL;
		hdtr->hdr_cnt = 0;
	}
	if (lua_getfield(L, arg, "trailers") == LUA_TTABLE) {
		int t = lua_gettop(L);

		hdtr->trl_cnt = luaL_len(L, t);
		if (hdtr->trl_cnt == 0) {
			hdtr->trailers = NULL;
		} else {
			hdtr->trailers =
			    malloc(hdtr->trl_cnt * sizeof(*hdtr->trailers));
			if (hdtr->trailers == NULL) {
				free(hdtr->headers);
				return (fatal(L, "malloc", ENOMEM));
			}
		}
		for (int i = 0; i < hdtr->trl_cnt; i++) {
			if (lua_geti(L, t, i + 1) != LUA_TSTRING) {
				free(hdtr->trailers);
				free(hdtr->headers);
				return (luaL_error(L,
				    "trailers must be strings"));
			}
			hdtr->trailers[i].iov_base = __DECONST(char *,
			    luaL_tolstring(L, -1, &hdtr->trailers[i].iov_len));
			assert(hdtr->trailers[i].iov_base != NULL);
		}
	} else {
		hdtr->trailers = NULL;
		hdtr->trl_cnt = 0;
	}
	return (0);
}

static int
l_sendfile(lua_State *L)
{
	struct sf_hdtr hdtr, *hdtrp;
	off_t offset, sbytes;
	size_t nbytes;
	int fd, s, flags, readahead;

	fd = checkfd(L, 1);
	s = checkfd(L, 2);
	offset = luaL_checkinteger(L, 3);
	nbytes = luaL_checkinteger(L, 4);
	if (lua_istable(L, 5)) {
		int res;

		hdtrp = &hdtr;
		res = get_hdtr(L, 5, hdtrp);
		assert(res == 0);
		flags = luaL_optinteger(L, 6, 0);
		readahead = luaL_optinteger(L, 7, 0);
	} else {
		hdtrp = NULL;
		flags = luaL_optinteger(L, 5, 0);
		readahead = luaL_optinteger(L, 6, 0);
	}
	if (sendfile(fd, s, offset, nbytes, hdtrp, &sbytes,
	    SF_FLAGS(readahead, flags)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, sbytes);
	return (1);
}

static int
l_sendmmsg(lua_State *L)
{
	return (luaL_error(L, "TODO"));
}

static int
l_setfib(lua_State *L)
{
	int fib;

	fib = luaL_checkinteger(L, 1);

	if (setfib(fib) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_setsockopt(lua_State *L)
{
	const void *optval;
	size_t optlen;
	int s, level, optname;

	s = checkfd(L, 1);
	level = luaL_checkinteger(L, 2);
	optname = luaL_checkinteger(L, 3);
	optval = luaL_checklstring(L, 4, &optlen);

	if (setsockopt(s, level, optname, optval, optlen) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_shutdown(lua_State *L)
{
	int s, how;

	s = checkfd(L, 1);
	how = luaL_checkinteger(L, 2);

	if (shutdown(s, how) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_sockatmark(lua_State *L)
{
	int s, atmark;

	s = checkfd(L, 1);

	if ((atmark = sockatmark(s)) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, atmark);
	return (1);
}

static int
l_socket(lua_State *L)
{
	int domain, type, protocol, s;

	domain = luaL_checkinteger(L, 1);
	type = luaL_checkinteger(L, 2);
	protocol = luaL_checkinteger(L, 3);

	if ((s = socket(domain, type, protocol)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, s);
	return (1);
}

static int
l_socketpair(lua_State *L)
{
	int sv[2];
	int domain, type, protocol;

	domain = luaL_checkinteger(L, 1);
	type = luaL_checkinteger(L, 2);
	protocol = luaL_checkinteger(L, 3);

	if (socketpair(domain, type, protocol, sv) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, sv[0]);
	lua_pushinteger(L, sv[1]);
	return (2);
}

static const struct luaL_Reg l_socket_funcs[] = {
	{"accept", l_accept},
	{"bind", l_bind},
	{"connect", l_connect},
	{"bindat", l_bindat},
	{"connectat", l_connectat},
	{"getpeername", l_getpeername},
	{"getsockname", l_getsockname},
	{"getsockopt", l_getsockopt},
	{"listen", l_listen},
	{"recv", l_recv},
	{"recvfrom", l_recvfrom},
	{"recvmsg", l_recvmsg},
	{"recvmmsg", l_recvmmsg},
	{"send", l_send},
	{"sendto", l_sendto},
	{"sendmsg", l_sendmsg},
	{"sendfile", l_sendfile},
	{"sendmmsg", l_sendmmsg},
	{"setfib", l_setfib},
	{"setsockopt", l_setsockopt},
	{"shutdown", l_shutdown},
	{"sockatmark", l_sockatmark},
	{"socket", l_socket},
	{"socketpair", l_socketpair},
	{NULL, NULL}
};

int
luaopen_sys_socket(lua_State *L)
{
	luaL_newlib(L, l_socket_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(SOCK_STREAM);
	SETCONST(SOCK_DGRAM);
	SETCONST(SOCK_RAW);
	SETCONST(SOCK_RDM);
	SETCONST(SOCK_SEQPACKET);

	SETCONST(SOCK_CLOEXEC);
	SETCONST(SOCK_NONBLOCK);
#ifdef SOCK_CLOFORK
	SETCONST(SOCK_CLOFORK);
#endif

	SETCONST(SO_DEBUG);
	SETCONST(SO_ACCEPTCONN);
	SETCONST(SO_REUSEADDR);
	SETCONST(SO_KEEPALIVE);
	SETCONST(SO_DONTROUTE);
	SETCONST(SO_BROADCAST);
	SETCONST(SO_USELOOPBACK);
	SETCONST(SO_LINGER);
	SETCONST(SO_OOBINLINE);
	SETCONST(SO_REUSEPORT);
	SETCONST(SO_TIMESTAMP);
	SETCONST(SO_NOSIGPIPE);
	SETCONST(SO_ACCEPTFILTER);
	SETCONST(SO_BINTIME);
	SETCONST(SO_NO_OFFLOAD);
	SETCONST(SO_NO_DDP);
	SETCONST(SO_REUSEPORT_LB);
	SETCONST(SO_RERROR);

	SETCONST(SO_SNDBUF);
	SETCONST(SO_RCVBUF);
	SETCONST(SO_SNDLOWAT);
	SETCONST(SO_RCVLOWAT);
	SETCONST(SO_SNDTIMEO);
	SETCONST(SO_RCVTIMEO);
	SETCONST(SO_ERROR);
	SETCONST(SO_TYPE);
	SETCONST(SO_LABEL);
	SETCONST(SO_PEERLABEL);
	SETCONST(SO_LISTENQLIMIT);
	SETCONST(SO_LISTENQLEN);
	SETCONST(SO_LISTENINCQLEN);
#ifdef SO_FIB
	SETCONST(SO_FIB);
#endif
	SETCONST(SO_USER_COOKIE);
	SETCONST(SO_PROTOCOL);
	SETCONST(SO_TS_CLOCK);
	SETCONST(SO_MAX_PACING_RATE);
	SETCONST(SO_DOMAIN);
#ifdef SO_SPLICE
	SETCONST(SO_SPLICE);
#endif

	SETCONST(SO_TS_REALTIME_MICRO);
	SETCONST(SO_TS_BINTIME);
	SETCONST(SO_TS_REALTIME);
	SETCONST(SO_TS_MONOTONIC);
	SETCONST(SO_TS_DEFAULT);
	SETCONST(SO_TS_CLOCK_MAX);

	SETCONST(SO_VENDOR);

	SETCONST(SOL_SOCKET);

	SETCONST(AF_UNSPEC);
	SETCONST(AF_LOCAL);
	SETCONST(AF_UNIX);
	SETCONST(AF_INET);
	SETCONST(AF_IMPLINK);
	SETCONST(AF_PUP);
	SETCONST(AF_CHAOS);
	SETCONST(AF_NETBIOS);
	SETCONST(AF_ISO);
	SETCONST(AF_ECMA);
	SETCONST(AF_DATAKIT);
	SETCONST(AF_CCITT);
	SETCONST(AF_SNA);
	SETCONST(AF_DECnet);
	SETCONST(AF_LAT);
	SETCONST(AF_HYLINK);
	SETCONST(AF_APPLETALK);
	SETCONST(AF_ROUTE);
	SETCONST(AF_LINK);
	SETCONST(pseudo_AF_XTP);
	SETCONST(AF_COIP);
	SETCONST(AF_CNT);
	SETCONST(pseudo_AF_RTIP);
	SETCONST(AF_IPX);
	SETCONST(AF_SIP);
	SETCONST(pseudo_AF_PIP);
	SETCONST(AF_ISDN);
	SETCONST(AF_E164);
	SETCONST(pseudo_AF_KEY);
	SETCONST(AF_INET6);
	SETCONST(AF_NATM);
	SETCONST(AF_ATM);
	SETCONST(pseudo_AF_HDRCMPLT);
	SETCONST(AF_NETGRAPH);
	SETCONST(AF_SLOW);
	SETCONST(AF_SCLUSTER);
	SETCONST(AF_ARP);
	SETCONST(AF_BLUETOOTH);
	SETCONST(AF_IEEE80211);
	SETCONST(AF_NETLINK);
	SETCONST(AF_INET_SDP);
	SETCONST(AF_INET6_SDP);
	SETCONST(AF_HYPERV);
#ifdef AF_DIVERT
	SETCONST(AF_DIVERT);
#endif
#ifdef AF_IPFWLOG
	SETCONST(AF_IPFWLOG);
#endif
	SETCONST(AF_MAX);

	SETCONST(AF_VENDOR00);
	SETCONST(AF_VENDOR01);
	/* AF_HYPERV */
	SETCONST(AF_VENDOR03);
	SETCONST(AF_VENDOR04);
	SETCONST(AF_VENDOR05);
	SETCONST(AF_VENDOR06);
	SETCONST(AF_VENDOR07);
	SETCONST(AF_VENDOR08);
	SETCONST(AF_VENDOR09);
	SETCONST(AF_VENDOR10);
	SETCONST(AF_VENDOR11);
	SETCONST(AF_VENDOR12);
	SETCONST(AF_VENDOR13);
	SETCONST(AF_VENDOR14);
	SETCONST(AF_VENDOR15);
	SETCONST(AF_VENDOR16);
	SETCONST(AF_VENDOR17);
	SETCONST(AF_VENDOR18);
	SETCONST(AF_VENDOR19);
	SETCONST(AF_VENDOR20);
	SETCONST(AF_VENDOR21);
	SETCONST(AF_VENDOR22);
	SETCONST(AF_VENDOR23);
	SETCONST(AF_VENDOR24);
	SETCONST(AF_VENDOR25);
	SETCONST(AF_VENDOR26);
	SETCONST(AF_VENDOR27);
	SETCONST(AF_VENDOR28);
	SETCONST(AF_VENDOR29);
	SETCONST(AF_VENDOR30);
	SETCONST(AF_VENDOR31);
	SETCONST(AF_VENDOR32);
	SETCONST(AF_VENDOR33);
	SETCONST(AF_VENDOR34);
	SETCONST(AF_VENDOR35);
	SETCONST(AF_VENDOR36);
	SETCONST(AF_VENDOR37);
	SETCONST(AF_VENDOR38);
	SETCONST(AF_VENDOR39);
	SETCONST(AF_VENDOR40);
	SETCONST(AF_VENDOR41);
	SETCONST(AF_VENDOR42);
	SETCONST(AF_VENDOR43);
	SETCONST(AF_VENDOR44);
	SETCONST(AF_VENDOR45);
	SETCONST(AF_VENDOR46);
	SETCONST(AF_VENDOR47);

	SETCONST(SOCK_MAXADDRLEN);

	SETCONST(PF_UNSPEC);
	SETCONST(PF_LOCAL);
	SETCONST(PF_UNIX);
	SETCONST(PF_INET);
	SETCONST(PF_IMPLINK);
	SETCONST(PF_PUP);
	SETCONST(PF_CHAOS);
	SETCONST(PF_NETBIOS);
	SETCONST(PF_ISO);
	SETCONST(PF_ECMA);
	SETCONST(PF_DATAKIT);
	SETCONST(PF_CCITT);
	SETCONST(PF_SNA);
	SETCONST(PF_DECnet);
	SETCONST(PF_LAT);
	SETCONST(PF_HYLINK);
	SETCONST(PF_APPLETALK);
	SETCONST(PF_ROUTE);
	SETCONST(PF_LINK);
	SETCONST(PF_XTP);
	SETCONST(PF_COIP);
	SETCONST(PF_CNT);
	SETCONST(PF_RTIP);
	SETCONST(PF_IPX);
	SETCONST(PF_SIP);
	SETCONST(PF_PIP);
	SETCONST(PF_ISDN);
	SETCONST(PF_KEY);
	SETCONST(PF_INET6);
	SETCONST(PF_NATM);
	SETCONST(PF_ATM);
	SETCONST(PF_NETGRAPH);
	SETCONST(PF_SLOW);
	SETCONST(PF_SCLUSTER);
	SETCONST(PF_ARP);
	SETCONST(PF_BLUETOOTH);
	SETCONST(PF_IEEE80211);
	SETCONST(PF_NETLINK);
	SETCONST(PF_INET_SDP);
	SETCONST(PF_INET6_SDP);
#ifdef PF_HYPERV
	SETCONST(PF_HYPERV);
#endif
#ifdef PF_DIVERT
	SETCONST(PF_DIVERT);
#endif
#ifdef PF_IPFWLOG
	SETCONST(PF_IPFWLOG);
#endif
	SETCONST(PF_MAX);

	SETCONST(NET_RT_DUMP);
	SETCONST(NET_RT_FLAGS);
	SETCONST(NET_RT_IFLIST);
	SETCONST(NET_RT_IFMALIST);
	SETCONST(NET_RT_IFLISTL);
	SETCONST(NET_RT_NHOP);
	SETCONST(NET_RT_NHGRP);

	SETCONST(SOMAXCONN);

	SETCONST(MSG_OOB);
	SETCONST(MSG_PEEK);
	SETCONST(MSG_DONTROUTE);
	SETCONST(MSG_EOR);
	SETCONST(MSG_TRUNC);
	SETCONST(MSG_CTRUNC);
	SETCONST(MSG_WAITALL);
	SETCONST(MSG_DONTWAIT);
	SETCONST(MSG_EOF);
	SETCONST(MSG_NOTIFICATION);
	SETCONST(MSG_NBIO);
	SETCONST(MSG_COMPAT);
	SETCONST(MSG_NOSIGNAL);
	SETCONST(MSG_CMSG_CLOEXEC);
	SETCONST(MSG_WAITFORONE);
#ifdef MSG_CMSG_CLOFORK
	SETCONST(MSG_CMSG_CLOFORK);
#endif

	SETCONST(CMGROUP_MAX);

	SETCONST(SCM_RIGHTS);
	SETCONST(SCM_TIMESTAMP);
	SETCONST(SCM_CREDS);
	SETCONST(SCM_BINTIME);
	SETCONST(SCM_REALTIME);
	SETCONST(SCM_MONOTONIC);
	SETCONST(SCM_TIME_INFO);
	SETCONST(SCM_CREDS2);

	SETCONST(ST_INFO_HW);
	SETCONST(ST_INFO_HW_HPREC);

	SETCONST(SHUT_RD);
	SETCONST(SHUT_WR);
	SETCONST(SHUT_RDWR);

	SETCONST(SF_NODISKIO);
	SETCONST(SF_NOCACHE);
	SETCONST(SF_USER_READAHEAD);
#undef SETCONST
	return (1);
}
