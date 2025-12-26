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

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lua_socket.h"
#include "luaerror.h"
#include "utils.h"

int luaopen_sys_socket(lua_State *);

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
	pushaddr(L, addr);
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
	pushaddr(L, name);
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
	pushaddr(L, name);
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
		int error = errno;

		free(optval);
		return (fail(L, error));
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
	pushaddr(L, from);
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
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(SOCK_STREAM);
	DEFINE(SOCK_DGRAM);
	DEFINE(SOCK_RAW);
	DEFINE(SOCK_RDM);
	DEFINE(SOCK_SEQPACKET);

	DEFINE(SOCK_CLOEXEC);
	DEFINE(SOCK_NONBLOCK);
#ifdef SOCK_CLOFORK
	DEFINE(SOCK_CLOFORK);
#endif

	DEFINE(SO_DEBUG);
	DEFINE(SO_ACCEPTCONN);
	DEFINE(SO_REUSEADDR);
	DEFINE(SO_KEEPALIVE);
	DEFINE(SO_DONTROUTE);
	DEFINE(SO_BROADCAST);
	DEFINE(SO_USELOOPBACK);
	DEFINE(SO_LINGER);
	DEFINE(SO_OOBINLINE);
	DEFINE(SO_REUSEPORT);
	DEFINE(SO_TIMESTAMP);
	DEFINE(SO_NOSIGPIPE);
	DEFINE(SO_ACCEPTFILTER);
	DEFINE(SO_BINTIME);
	DEFINE(SO_NO_OFFLOAD);
	DEFINE(SO_NO_DDP);
	DEFINE(SO_REUSEPORT_LB);
	DEFINE(SO_RERROR);

	DEFINE(SO_SNDBUF);
	DEFINE(SO_RCVBUF);
	DEFINE(SO_SNDLOWAT);
	DEFINE(SO_RCVLOWAT);
	DEFINE(SO_SNDTIMEO);
	DEFINE(SO_RCVTIMEO);
	DEFINE(SO_ERROR);
	DEFINE(SO_TYPE);
	DEFINE(SO_LABEL);
	DEFINE(SO_PEERLABEL);
	DEFINE(SO_LISTENQLIMIT);
	DEFINE(SO_LISTENQLEN);
	DEFINE(SO_LISTENINCQLEN);
#ifdef SO_FIB
	DEFINE(SO_FIB);
#endif
	DEFINE(SO_USER_COOKIE);
	DEFINE(SO_PROTOCOL);
	DEFINE(SO_TS_CLOCK);
	DEFINE(SO_MAX_PACING_RATE);
	DEFINE(SO_DOMAIN);
#ifdef SO_SPLICE
	DEFINE(SO_SPLICE);
#endif

	DEFINE(SO_TS_REALTIME_MICRO);
	DEFINE(SO_TS_BINTIME);
	DEFINE(SO_TS_REALTIME);
	DEFINE(SO_TS_MONOTONIC);
	DEFINE(SO_TS_DEFAULT);
	DEFINE(SO_TS_CLOCK_MAX);

	DEFINE(SO_VENDOR);

	DEFINE(SOL_SOCKET);

	DEFINE(AF_UNSPEC);
	DEFINE(AF_LOCAL);
	DEFINE(AF_UNIX);
	DEFINE(AF_INET);
	DEFINE(AF_IMPLINK);
	DEFINE(AF_PUP);
	DEFINE(AF_CHAOS);
	DEFINE(AF_NETBIOS);
	DEFINE(AF_ISO);
	DEFINE(AF_ECMA);
	DEFINE(AF_DATAKIT);
	DEFINE(AF_CCITT);
	DEFINE(AF_SNA);
	DEFINE(AF_DECnet);
	DEFINE(AF_LAT);
	DEFINE(AF_HYLINK);
	DEFINE(AF_APPLETALK);
	DEFINE(AF_ROUTE);
	DEFINE(AF_LINK);
	DEFINE(pseudo_AF_XTP);
	DEFINE(AF_COIP);
	DEFINE(AF_CNT);
	DEFINE(pseudo_AF_RTIP);
	DEFINE(AF_IPX);
	DEFINE(AF_SIP);
	DEFINE(pseudo_AF_PIP);
	DEFINE(AF_ISDN);
	DEFINE(AF_E164);
	DEFINE(pseudo_AF_KEY);
	DEFINE(AF_INET6);
	DEFINE(AF_NATM);
	DEFINE(AF_ATM);
	DEFINE(pseudo_AF_HDRCMPLT);
	DEFINE(AF_NETGRAPH);
	DEFINE(AF_SLOW);
	DEFINE(AF_SCLUSTER);
	DEFINE(AF_ARP);
	DEFINE(AF_BLUETOOTH);
	DEFINE(AF_IEEE80211);
	DEFINE(AF_NETLINK);
	DEFINE(AF_INET_SDP);
	DEFINE(AF_INET6_SDP);
	DEFINE(AF_HYPERV);
#ifdef AF_DIVERT
	DEFINE(AF_DIVERT);
#endif
#ifdef AF_IPFWLOG
	DEFINE(AF_IPFWLOG);
#endif
	DEFINE(AF_MAX);

	DEFINE(AF_VENDOR00);
	DEFINE(AF_VENDOR01);
	/* AF_HYPERV */
	DEFINE(AF_VENDOR03);
	DEFINE(AF_VENDOR04);
	DEFINE(AF_VENDOR05);
	DEFINE(AF_VENDOR06);
	DEFINE(AF_VENDOR07);
	DEFINE(AF_VENDOR08);
	DEFINE(AF_VENDOR09);
	DEFINE(AF_VENDOR10);
	DEFINE(AF_VENDOR11);
	DEFINE(AF_VENDOR12);
	DEFINE(AF_VENDOR13);
	DEFINE(AF_VENDOR14);
	DEFINE(AF_VENDOR15);
	DEFINE(AF_VENDOR16);
	DEFINE(AF_VENDOR17);
	DEFINE(AF_VENDOR18);
	DEFINE(AF_VENDOR19);
	DEFINE(AF_VENDOR20);
	DEFINE(AF_VENDOR21);
	DEFINE(AF_VENDOR22);
	DEFINE(AF_VENDOR23);
	DEFINE(AF_VENDOR24);
	DEFINE(AF_VENDOR25);
	DEFINE(AF_VENDOR26);
	DEFINE(AF_VENDOR27);
	DEFINE(AF_VENDOR28);
	DEFINE(AF_VENDOR29);
	DEFINE(AF_VENDOR30);
	DEFINE(AF_VENDOR31);
	DEFINE(AF_VENDOR32);
	DEFINE(AF_VENDOR33);
	DEFINE(AF_VENDOR34);
	DEFINE(AF_VENDOR35);
	DEFINE(AF_VENDOR36);
	DEFINE(AF_VENDOR37);
	DEFINE(AF_VENDOR38);
	DEFINE(AF_VENDOR39);
	DEFINE(AF_VENDOR40);
	DEFINE(AF_VENDOR41);
	DEFINE(AF_VENDOR42);
	DEFINE(AF_VENDOR43);
	DEFINE(AF_VENDOR44);
	DEFINE(AF_VENDOR45);
	DEFINE(AF_VENDOR46);
	DEFINE(AF_VENDOR47);

	DEFINE(SOCK_MAXADDRLEN);

	DEFINE(PF_UNSPEC);
	DEFINE(PF_LOCAL);
	DEFINE(PF_UNIX);
	DEFINE(PF_INET);
	DEFINE(PF_IMPLINK);
	DEFINE(PF_PUP);
	DEFINE(PF_CHAOS);
	DEFINE(PF_NETBIOS);
	DEFINE(PF_ISO);
	DEFINE(PF_ECMA);
	DEFINE(PF_DATAKIT);
	DEFINE(PF_CCITT);
	DEFINE(PF_SNA);
	DEFINE(PF_DECnet);
	DEFINE(PF_LAT);
	DEFINE(PF_HYLINK);
	DEFINE(PF_APPLETALK);
	DEFINE(PF_ROUTE);
	DEFINE(PF_LINK);
	DEFINE(PF_XTP);
	DEFINE(PF_COIP);
	DEFINE(PF_CNT);
	DEFINE(PF_RTIP);
	DEFINE(PF_IPX);
	DEFINE(PF_SIP);
	DEFINE(PF_PIP);
	DEFINE(PF_ISDN);
	DEFINE(PF_KEY);
	DEFINE(PF_INET6);
	DEFINE(PF_NATM);
	DEFINE(PF_ATM);
	DEFINE(PF_NETGRAPH);
	DEFINE(PF_SLOW);
	DEFINE(PF_SCLUSTER);
	DEFINE(PF_ARP);
	DEFINE(PF_BLUETOOTH);
	DEFINE(PF_IEEE80211);
	DEFINE(PF_NETLINK);
	DEFINE(PF_INET_SDP);
	DEFINE(PF_INET6_SDP);
#ifdef PF_HYPERV
	DEFINE(PF_HYPERV);
#endif
#ifdef PF_DIVERT
	DEFINE(PF_DIVERT);
#endif
#ifdef PF_IPFWLOG
	DEFINE(PF_IPFWLOG);
#endif
	DEFINE(PF_MAX);

	DEFINE(NET_RT_DUMP);
	DEFINE(NET_RT_FLAGS);
	DEFINE(NET_RT_IFLIST);
	DEFINE(NET_RT_IFMALIST);
	DEFINE(NET_RT_IFLISTL);
	DEFINE(NET_RT_NHOP);
	DEFINE(NET_RT_NHGRP);

	DEFINE(SOMAXCONN);

	DEFINE(MSG_OOB);
	DEFINE(MSG_PEEK);
	DEFINE(MSG_DONTROUTE);
	DEFINE(MSG_EOR);
	DEFINE(MSG_TRUNC);
	DEFINE(MSG_CTRUNC);
	DEFINE(MSG_WAITALL);
	DEFINE(MSG_DONTWAIT);
	DEFINE(MSG_EOF);
	DEFINE(MSG_NOTIFICATION);
	DEFINE(MSG_NBIO);
	DEFINE(MSG_COMPAT);
	DEFINE(MSG_NOSIGNAL);
	DEFINE(MSG_CMSG_CLOEXEC);
	DEFINE(MSG_WAITFORONE);
#ifdef MSG_CMSG_CLOFORK
	DEFINE(MSG_CMSG_CLOFORK);
#endif

	DEFINE(CMGROUP_MAX);

	DEFINE(SCM_RIGHTS);
	DEFINE(SCM_TIMESTAMP);
	DEFINE(SCM_CREDS);
	DEFINE(SCM_BINTIME);
	DEFINE(SCM_REALTIME);
	DEFINE(SCM_MONOTONIC);
	DEFINE(SCM_TIME_INFO);
	DEFINE(SCM_CREDS2);

	DEFINE(ST_INFO_HW);
	DEFINE(ST_INFO_HW_HPREC);

	DEFINE(SHUT_RD);
	DEFINE(SHUT_WR);
	DEFINE(SHUT_RDWR);

	DEFINE(SF_NODISKIO);
	DEFINE(SF_NOCACHE);
	DEFINE(SF_USER_READAHEAD);
#undef DEFINE
	return (1);
}
