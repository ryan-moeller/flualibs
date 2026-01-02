/*
 * Copyright (c) 2025-2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/sctp.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/socket/lua_socket.h"
#include "sys/uio/lua_uio.h"
#include "utils.h"

#define CLOSER_METATABLE "netinet.sctp closer"

int luaopen_netinet_sctp(lua_State *);

static inline struct sockaddr *
checkaddrs(lua_State *L, int idx, size_t *num)
{
	struct sockaddr *addrs, *sa;
	size_t n, size, len;

	*num = 0;

	luaL_checktype(L, idx, LUA_TTABLE);
	if ((n = luaL_len(L, idx)) == 0) {
		luaL_argerror(L, 2, "must give at least one address");
	}

	size = n * offsetof(struct sockaddr, sa_data);
	for (size_t i = 1; i <= n; i++) {
		lua_geti(L, idx, i);
		if (lua_getfield(L, -1, "data") != LUA_TSTRING) {
			luaL_argerror(L, idx, "invalid sockaddr");
		}
		lua_tolstring(L, -1, &len);
		size += len;
		lua_pop(L, 2);
	}
	if ((addrs = malloc(size)) == NULL) {
		fatal(L, "malloc", ENOMEM);
	}
	sa = addrs;
	for (size_t i = 1; i <= n; i++) {
		lua_geti(L, idx, i);
		lua_getfield(L, -1, "family");
		if (!lua_isinteger(L, -1)) {
			free(addrs);
			luaL_argerror(L, idx, "invalid sockaddr");
		}
		sa->sa_family = lua_tointeger(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, -1, "data");
		memcpy(sa->sa_data, lua_tolstring(L, -1, &len), len);
		lua_pop(L, 1);
		sa->sa_len = offsetof(struct sockaddr, sa_data) + len;
		sa = (struct sockaddr *)((uintptr_t)sa + sa->sa_len);
		lua_pop(L, 1);
	}
	*num = n;
	return (addrs);
}

static inline void
pushaddrs(lua_State *L, const struct sockaddr *sa, size_t n)
{
	lua_createtable(L, n, 0);
	for (size_t i = 1; i <= n; i++) {
		pushaddr(L, sa);
		lua_rawseti(L, -2, i);
		sa = (const struct sockaddr *)((uintptr_t)sa + sa->sa_len);
	}
}

static int
l_invalid_sinfo_flag(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushboolean(L, INVALID_SINFO_FLAG(x));
	return (1);
}

static int
l_pr_sctp_policy(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushinteger(L, PR_SCTP_POLICY(x));
	return (1);
}

static int
l_pr_sctp_enabled(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushboolean(L, PR_SCTP_ENABLED(x));
	return (1);
}

static int
l_pr_sctp_ttl_enabled(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushboolean(L, PR_SCTP_TTL_ENABLED(x));
	return (1);
}

static int
l_pr_sctp_buf_enabled(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushboolean(L, PR_SCTP_BUF_ENABLED(x));
	return (1);
}

static int
l_pr_sctp_rtx_enabled(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushboolean(L, PR_SCTP_RTX_ENABLED(x));
	return (1);
}

static int
l_pr_sctp_invalid_policy(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushboolean(L, PR_SCTP_INVALID_POLICY(x));
	return (1);
}

static int
l_pr_sctp_valid_policy(lua_State *L)
{
	lua_Integer x;

	x = luaL_checkinteger(L, 1);

	lua_pushboolean(L, PR_SCTP_VALID_POLICY(x));
	return (1);
}

static int
l_sctp_peeloff(lua_State *L)
{
	sctp_assoc_t id;
	int s, new_s;

	s = checkfd(L, 1);
	id = luaL_checkinteger(L, 2);

	if ((new_s = sctp_peeloff(s, id)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, new_s);
	return (1);
}

static int
l_sctp_bindx(lua_State *L)
{
	struct sockaddr *addrs;
	size_t num;
	int s, type;

	s = checkfd(L, 1);
	addrs = checkaddrs(L, 2, &num);
	type = luaL_checkinteger(L, 3);

	if (sctp_bindx(s, addrs, num, type) == -1) {
		int error = errno;

		free(addrs);
		return (fail(L, error));
	}
	free(addrs);
	return (success(L));
}

static int
l_sctp_connectx(lua_State *L)
{
	struct sockaddr *addrs;
	size_t addrcnt;
	sctp_assoc_t id;
	int s;

	s = checkfd(L, 1);
	addrs = checkaddrs(L, 2, &addrcnt);
	luaL_argcheck(L, addrcnt > 0, 2, "must give at least one address");

	if (sctp_connectx(s, addrs, addrcnt, &id) == -1) {
		int error = errno;

		free(addrs);
		return (fail(L, error));
	}
	free(addrs);
	lua_pushinteger(L, id);
	return (1);
}

static int
l_sctp_getaddrlen(lua_State *L)
{
	sa_family_t family;
	int len;

	family = luaL_checkinteger(L, 1);

	if ((len = sctp_getaddrlen(family)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, len);
	return (1);
}

static int
l_sctp_getpaddrs(lua_State *L)
{
	struct sockaddr *addrs;
	sctp_assoc_t id;
	int s, naddrs;

	s = checkfd(L, 1);
	id = luaL_checkinteger(L, 2);

	if ((naddrs = sctp_getpaddrs(s, id, &addrs)) == -1) {
		return (fail(L, errno));
	}
	pushaddrs(L, addrs, naddrs);
	sctp_freepaddrs(addrs);
	return (1);
}

static int
l_sctp_getladdrs(lua_State *L)
{
	struct sockaddr *addrs;
	sctp_assoc_t id;
	int s, naddrs;

	s = checkfd(L, 1);
	id = luaL_checkinteger(L, 2);

	if ((naddrs = sctp_getladdrs(s, id, &addrs)) == -1) {
		return (fail(L, errno));
	}
	pushaddrs(L, addrs, naddrs);
	sctp_freeladdrs(addrs);
	return (1);
}

static int
l_sctp_getassocid(lua_State *L)
{
	struct sockaddr_storage ss;
	sctp_assoc_t id;
	int s;

	s = checkfd(L, 1);
	checkaddr(L, 2, &ss);

	if ((id = sctp_getassocid(s, (struct sockaddr *)&ss)) == 0) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, id);
	return (1);
}

static inline void
checksndinfo(lua_State *L, int idx, struct sctp_sndinfo *info)
{
#define FIELD(name) ({ \
	lua_getfield(L, -1, #name); \
	luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid sndinfo"); \
	info->snd_ ## name = lua_tointeger(L, -1); \
	lua_pop(L, 1); \
})
	FIELD(sid);
	FIELD(flags);
	FIELD(ppid);
	FIELD(context);
	FIELD(assoc_id);
#undef FIELD
}

static inline void
checkprinfo(lua_State *L, int idx, struct sctp_prinfo *info)
{
#define FIELD(name) ({ \
	lua_getfield(L, -1, #name); \
	luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid prinfo"); \
	info->pr_ ## name = lua_tointeger(L, -1); \
	lua_pop(L, 1); \
})
	FIELD(policy);
	FIELD(value);
#undef FIELD
}

static inline void
checkauthinfo(lua_State *L, int idx, struct sctp_authinfo *info)
{
#define FIELD(name) ({ \
	lua_getfield(L, -1, #name); \
	luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid authinfo"); \
	info->auth_ ## name = lua_tointeger(L, -1); \
	lua_pop(L, 1); \
})
	FIELD(keynumber);
#undef FIELD
}

static inline void
checksendvspa(lua_State *L, int idx, struct sctp_sendv_spa *info)
{
	info->sendv_flags = 0;
	if (lua_getfield(L, idx, "sndinfo") == LUA_TTABLE) {
		checksndinfo(L, idx, &info->sendv_sndinfo);
		info->sendv_flags |= SCTP_SEND_SNDINFO_VALID;
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "prinfo") == LUA_TTABLE) {
		checkprinfo(L, idx, &info->sendv_prinfo);
		info->sendv_flags |= SCTP_SEND_PRINFO_VALID;
	}
	lua_pop(L, 1);
	if (lua_getfield(L, idx, "authinfo") == LUA_TTABLE) {
		checkauthinfo(L, idx, &info->sendv_authinfo);
		info->sendv_flags |= SCTP_SEND_AUTHINFO_VALID;
	}
	lua_pop(L, 1);
}

enum {
	POINTER,
	LEN,
	FUNCTION,
};

static int
closer_close(lua_State *L)
{
	void (*closer)(void *, size_t);
	void *p;
	size_t len;

	luaL_checkudata(L, 1, CLOSER_METATABLE);

	lua_getiuservalue(L, 1, POINTER);
	p = lua_touserdata(L, -1);
	lua_getiuservalue(L, 1, LEN);
	len = lua_tointeger(L, -1);
	lua_getiuservalue(L, 1, FUNCTION);
	closer = lua_touserdata(L, -1);
	closer(p, len);
	return (0);
}

/*
 * Give a pointer a __close method and mark it to-be-closed.
 */
static inline void
closer(lua_State *L, void *p, size_t len, void (*closer)(void *, size_t))
{
	lua_newuserdatauv(L, 0, 3);
	lua_pushlightuserdata(L, p);
	lua_setiuservalue(L, -2, POINTER);
	lua_pushinteger(L, len);
	lua_setiuservalue(L, -2, LEN);
	lua_pushlightuserdata(L, closer);
	lua_setiuservalue(L, -2, FUNCTION);
	luaL_setmetatable(L, CLOSER_METATABLE);
	lua_toclose(L, -1);
}

static void
free_closer(void *p, size_t len __unused)
{
	free(p);
}

static int
l_sctp_sendv(lua_State *L)
{
	struct sctp_sendv_spa info;
	struct sockaddr *addrs;
	struct iovec *iov;
	void *infop;
	size_t addrcnt, iovcnt;
	ssize_t len;
	int s, flags;
	unsigned int infotype;
	socklen_t infolen;

	s = checkfd(L, 1);
	iov = checkwiovecs(L, 2, &iovcnt);
	closer(L, iov, iovcnt, free_closer);
	addrs = checkaddrs(L, 3, &addrcnt);
	closer(L, addrs, addrcnt, free_closer);
	checksendvspa(L, 4, &info);
	flags = luaL_optinteger(L, 5, 0);

	if (info.sendv_flags == 0) {
		infop = NULL;
		infolen = 0;
		infotype = SCTP_SENDV_NOINFO;
	} else {
		infop = &info;
		infolen = sizeof(info);
		infotype = SCTP_SENDV_SPA;
	}
	if ((len = sctp_sendv(s, iov, iovcnt, addrs, addrcnt, infop, infolen,
	    infotype, flags)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, len);
	return (1);
}

static inline void
pushrcvinfo(lua_State *L, struct sctp_rcvinfo *info)
{
	lua_newtable(L);
#define FIELD(name) ({ \
	lua_pushinteger(L, info->rcv_ ## name); \
	lua_setfield(L, -2, #name); \
})
	FIELD(sid);
	FIELD(ssn);
	FIELD(flags);
	FIELD(ppid);
	FIELD(tsn);
	FIELD(cumtsn);
	FIELD(context);
	FIELD(assoc_id);
#undef FIELD
}

static inline void
pushnxtinfo(lua_State *L, struct sctp_nxtinfo *info)
{
	lua_newtable(L);
#define FIELD(name) ({ \
	lua_pushinteger(L, info->nxt_ ## name); \
	lua_setfield(L, -2, #name); \
})
	FIELD(sid);
	FIELD(flags);
	FIELD(ppid);
	FIELD(length);
	FIELD(assoc_id);
#undef FIELD
}

union sctp_recvvinfo {
	struct sctp_recvv_rn rn;
	struct sctp_rcvinfo rcv;
	struct sctp_nxtinfo nxt;
};

static inline void
pushrecvvinfo(lua_State *L, union sctp_recvvinfo *info, unsigned int infotype)
{
	lua_newtable(L);
	switch (infotype) {
	case SCTP_RECVV_NOINFO:
		break;
	case SCTP_RECVV_NXTINFO:
		pushnxtinfo(L, &info->nxt);
		lua_setfield(L, -2, "nxtinfo");
		break;
	case SCTP_RECVV_RCVINFO:
		pushrcvinfo(L, &info->rcv);
		lua_setfield(L, -2, "rcvinfo");
		break;
	case SCTP_RECVV_RN:
		pushnxtinfo(L, &info->nxt);
		lua_setfield(L, -2, "nxtinfo");
		pushrcvinfo(L, &info->rcv);
		lua_setfield(L, -2, "rcvinfo");
		break;
	default:
		lua_pushliteral(L, "unknown infotype");
		lua_setfield(L, -2, "error");
		break;
	}
}

static void
freeriovecs_closer(void *p, size_t len)
{
	freeriovecs(p, len);
}

static int
l_sctp_recvv(lua_State *L)
{
	union sctp_recvvinfo info;
	struct sockaddr_storage ss;
	struct sockaddr *from;
	struct iovec *iov;
	size_t iovlen;
	ssize_t len;
	unsigned int infotype;
	int s, flags;
	socklen_t fromlen, infolen;

	from = (struct sockaddr *)&ss;
	infolen = sizeof(info);

	s = checkfd(L, 1);
	iov = checkriovecs(L, 2, &iovlen);
	closer(L, iov, iovlen, freeriovecs_closer);
	checkaddr(L, 3, &ss);

	fromlen = from->sa_len;
	if ((len = sctp_recvv(s, iov, iovlen, from, &fromlen, &info, &infolen,
	    &infotype, &flags)) == -1) {
		return (fail(L, errno));
	}
	pushriovecs(L, iov, iovlen);
	lua_pushinteger(L, len);
	pushaddr(L, from);
	pushrecvvinfo(L, &info, infotype);
	lua_pushinteger(L, flags);
	return (5);
}

static const struct luaL_Reg l_sctp_funcs[] = {
	{"invalid_sinfo_flag", l_invalid_sinfo_flag},
	{"pr_sctp_policy", l_pr_sctp_policy},
	{"pr_sctp_enabled", l_pr_sctp_enabled},
	{"pr_sctp_ttl_enabled", l_pr_sctp_ttl_enabled},
	{"pr_sctp_buf_enabled", l_pr_sctp_buf_enabled},
	{"pr_sctp_rtx_enabled", l_pr_sctp_rtx_enabled},
	{"pr_sctp_invalid_policy", l_pr_sctp_invalid_policy},
	{"pr_sctp_valid_policy", l_pr_sctp_valid_policy},
#define APIREG(name) {#name, l_sctp_ ## name}
	APIREG(peeloff),
	APIREG(bindx),
	APIREG(connectx),
	APIREG(getaddrlen),
	APIREG(getpaddrs),
	APIREG(getladdrs),
	/* sctp_opt_info omitted, use getsockopt directly */
	APIREG(getassocid),
	APIREG(sendv),
	APIREG(recvv),
	/* deprecated send* & recv* APIs omitted */
#undef APIREG
	{NULL, NULL}
};

int
luaopen_netinet_sctp(lua_State *L)
{
	/* A small convenience for giving C pointers a __close metamethod. */
	luaL_newmetatable(L, CLOSER_METATABLE);
	lua_pushcfunction(L, closer_close);
	lua_setfield(L, -2, "__close");

	luaL_newlib(L, l_sctp_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, SCTP_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(RTOINFO);
	DEFINE(ASSOCINFO);
	DEFINE(INITMSG);
	DEFINE(NODELAY);
	DEFINE(AUTOCLOSE);
	DEFINE(SET_PEER_PRIMARY_ADDR);
	DEFINE(PRIMARY_ADDR);
	DEFINE(ADAPTATION_LAYER);
	DEFINE(DISABLE_FRAGMENTS);
	DEFINE(PEER_ADDR_PARAMS);
	DEFINE(DEFAULT_SEND_PARAM);
	DEFINE(I_WANT_MAPPED_V4_ADDR);
	DEFINE(MAXSEG);
	DEFINE(DELAYED_SACK);
	DEFINE(FRAGMENT_INTERLEAVE);
	DEFINE(PARTIAL_DELIVERY_POINT);
	DEFINE(AUTH_CHUNK);
	DEFINE(AUTH_KEY);
	DEFINE(HMAC_IDENT);
	DEFINE(AUTH_ACTIVE_KEY);
	DEFINE(AUTH_DELETE_KEY);
	DEFINE(USE_EXT_RCVINFO);
	DEFINE(AUTO_ASCONF);
	DEFINE(MAX_BURST);
	DEFINE(CONTEXT);
	DEFINE(EXPLICIT_EOR);
	DEFINE(REUSE_PORT);
	DEFINE(AUTH_DEACTIVATE_KEY);
	DEFINE(EVENT);
	DEFINE(RECVRCVINFO);
	DEFINE(RECVNXTINFO);
	DEFINE(DEFAULT_SNDINFO);
	DEFINE(DEFAULT_PRINFO);
	DEFINE(PEER_ADDR_THLDS);
	DEFINE(REMOTE_UDP_ENCAPS_PORT);
	DEFINE(ECN_SUPPORTED);
	DEFINE(PR_SUPPORTED);
	DEFINE(AUTH_SUPPORTED);
	DEFINE(ASCONF_SUPPORTED);
	DEFINE(RECONFIG_SUPPORTED);
	DEFINE(NRSACK_SUPPORTED);
	DEFINE(PKTDROP_SUPPORTED);
	DEFINE(MAX_CWND);
#ifdef SCTP_ACCEPT_ZERO_CHECKSUM
	DEFINE(ACCEPT_ZERO_CHECKSUM);
#endif
	DEFINE(STATUS);
	DEFINE(GET_PEER_ADDR_INFO);
	DEFINE(PEER_AUTH_CHUNKS);
	DEFINE(LOCAL_AUTH_CHUNKS);
	DEFINE(GET_ASSOC_NUMBER);
	DEFINE(GET_ASSOC_ID_LIST);
	DEFINE(TIMEOUTS);
	DEFINE(PR_STREAM_STATUS);
	DEFINE(PR_ASSOC_STATUS);
	DEFINE(ENABLE_STREAM_RESET);
	DEFINE(RESET_STREAMS);
	DEFINE(RESET_ASSOC);
	DEFINE(ADD_STREAMS);
	DEFINE(ENABLE_RESET_STREAM_REQ);
	DEFINE(ENABLE_RESET_ASSOC_REQ);
	DEFINE(ENABLE_CHANGE_ASSOC_REQ);
	DEFINE(ENABLE_VALUE_MASK);
	DEFINE(STREAM_RESET_INCOMING);
	DEFINE(STREAM_RESET_OUTGOING);
	DEFINE(SET_DEBUG_LEVEL);
	DEFINE(CLR_STAT_LOG);
	DEFINE(CMT_ON_OFF);
	DEFINE(CMT_USE_DAC);
	DEFINE(PLUGGABLE_CC);
	DEFINE(STREAM_SCHEDULER);
	DEFINE(STREAM_SCHEDULER_VALUE);
	DEFINE(CC_OPTION);
	DEFINE(INTERLEAVING_SUPPORTED);
	DEFINE(GET_SNDBUF_USE);
	DEFINE(GET_STAT_LOG);
	DEFINE(PCB_STATUS);
	DEFINE(GET_NONCE_VALUES);
	DEFINE(SET_DYNAMIC_PRIMARY);

	DEFINE(VRF_ID);
	DEFINE(ADD_VRF_ID);
	DEFINE(GET_VRF_IDS);
	DEFINE(GET_ASOC_VRF);
	DEFINE(DEL_VRF_ID);
	DEFINE(GET_PACKET_LOG);

	DEFINE(CC_RFC2581);
	DEFINE(CC_HSTCP);
	DEFINE(CC_HTCP);
	DEFINE(CC_RTCC);
	DEFINE(CC_OPT_RTCC_SETMODE);
	DEFINE(CC_OPT_USE_DCCC_ECN);
	DEFINE(CC_OPT_STEADY_STEP);

	DEFINE(CMT_OFF);
	DEFINE(CMT_BASE);
	DEFINE(CMT_RPV1);
	DEFINE(CMT_RPV2);
	DEFINE(CMT_MPTCP);
	DEFINE(CMT_MAX);

	DEFINE(SS_DEFAULT);
	DEFINE(SS_RR);
	DEFINE(SS_RR_PKT);
	DEFINE(SS_PRIO);
	DEFINE(SS_FB);
	DEFINE(SS_FCFS);

	DEFINE(FRAG_LEVEL_0);
	DEFINE(FRAG_LEVEL_1);
	DEFINE(FRAG_LEVEL_2);

	DEFINE(CLOSED);
	DEFINE(BOUND);
	DEFINE(LISTEN);
	DEFINE(COOKIE_WAIT);
	DEFINE(COOKIE_ECHOED);
	DEFINE(ESTABLISHED);
	DEFINE(SHUTDOWN_SENT);
	DEFINE(SHUTDOWN_RECEIVED);
	DEFINE(SHUTDOWN_ACK_SENT);
	DEFINE(SHUTDOWN_PENDING);

	DEFINE(CAUSE_NO_ERROR);
	DEFINE(CAUSE_INVALID_STREAM);
	DEFINE(CAUSE_MISSING_PARAM);
	DEFINE(CAUSE_STALE_COOKIE);
	DEFINE(CAUSE_OUT_OF_RESC);
	DEFINE(CAUSE_UNRESOLVABLE_ADDR);
	DEFINE(CAUSE_UNRECOG_CHUNK);
	DEFINE(CAUSE_INVALID_PARAM);
	DEFINE(CAUSE_UNRECOG_PARAM);
	DEFINE(CAUSE_NO_USER_DATA);
	DEFINE(CAUSE_COOKIE_IN_SHUTDOWN);
	DEFINE(CAUSE_RESTART_W_NEWADDR);
	DEFINE(CAUSE_USER_INITIATED_ABT);
	DEFINE(CAUSE_PROTOCOL_VIOLATION);
	DEFINE(CAUSE_DELETING_LAST_ADDR);
	DEFINE(CAUSE_RESOURCE_SHORTAGE);
	DEFINE(CAUSE_DELETING_SRC_ADDR);
	DEFINE(CAUSE_ILLEGAL_ASCONF_ACK);
	DEFINE(CAUSE_REQUEST_REFUSED);
	DEFINE(CAUSE_NAT_COLLIDING_STATE);
	DEFINE(CAUSE_NAT_MISSING_STATE);
	DEFINE(CAUSE_UNSUPPORTED_HMACID);

	/* Chunk type definitions omitted, don't implement a firewall in Lua. */

	/* XXX: Are these chunk flags useful? */
	DEFINE(HAD_NO_TCB);

	DEFINE(FROM_MIDDLE_BOX);
	DEFINE(BADCRC);
	DEFINE(PACKET_TRUNCATED);

	DEFINE(CWR_REDUCE_OVERRIDE);
	DEFINE(CWR_IN_SAME_WINDOW);

	DEFINE(SAT_NETWORK_MIN);
	DEFINE(SAT_NETWORK_BURST_INCR);

	DEFINE(DATA_FRAG_MASK);
	DEFINE(DATA_MIDDLE_FRAG);
	DEFINE(DATA_LAST_FRAG);
	DEFINE(DATA_FIRST_FRAG);
	DEFINE(DATA_NOT_FRAG);
	DEFINE(DATA_UNORDERED);
	DEFINE(DATA_SACK_IMMEDIATELY);

	DEFINE(SACK_NONCE_SUM);
	DEFINE(SACK_CMT_DAC);

	/* XXX: Are these PCB flags useful? */
	DEFINE(PCB_FLAGS_UDPTYPE);
	DEFINE(PCB_FLAGS_TCPTYPE);
	DEFINE(PCB_FLAGS_BOUNDALL);
	DEFINE(PCB_FLAGS_ACCEPTING);
	DEFINE(PCB_FLAGS_UNBOUND);
	DEFINE(PCB_FLAGS_SND_ITERATOR_UP);
	DEFINE(PCB_FLAGS_CLOSE_IP);
	DEFINE(PCB_FLAGS_WAS_CONNECTED);
	DEFINE(PCB_FLAGS_WAS_ABORTED);
	DEFINE(PCB_FLAGS_CONNECTED);
	DEFINE(PCB_FLAGS_IN_TCPPOOL);
	DEFINE(PCB_FLAGS_DONT_WAKE);
	DEFINE(PCB_FLAGS_WAKEOUTPUT);
	DEFINE(PCB_FLAGS_WAKEINPUT);
	DEFINE(PCB_FLAGS_BOUND_V6);
	DEFINE(PCB_FLAGS_BLOCKING_IO);
	DEFINE(PCB_FLAGS_SOCKET_GONE);
	DEFINE(PCB_FLAGS_SOCKET_ALLGONE);
	DEFINE(PCB_FLAGS_SOCKET_CANT_READ);
	DEFINE(PCB_COPY_FLAGS);
	DEFINE(PCB_FLAGS_DO_NOT_PMTUD);
	DEFINE(PCB_FLAGS_DONOT_HEARTBEAT);
	DEFINE(PCB_FLAGS_FRAG_INTERLEAVE);
	DEFINE(PCB_FLAGS_INTERLEAVE_STRMS);
	DEFINE(PCB_FLAGS_DO_ASCONF);
	DEFINE(PCB_FLAGS_AUTO_ASCONF);
	DEFINE(PCB_FLAGS_NODELAY);
	DEFINE(PCB_FLAGS_AUTOCLOSE);
	DEFINE(PCB_FLAGS_RECVASSOCEVNT);
	DEFINE(PCB_FLAGS_RECVPADDREVNT);
	DEFINE(PCB_FLAGS_RECVPEERERR);
	DEFINE(PCB_FLAGS_RECVSHUTDOWNEVNT);
	DEFINE(PCB_FLAGS_ADAPTATIONEVNT);
	DEFINE(PCB_FLAGS_PDAPIEVNT);
	DEFINE(PCB_FLAGS_AUTHEVNT);
	DEFINE(PCB_FLAGS_STREAM_RESETEVNT);
	DEFINE(PCB_FLAGS_NO_FRAGMENT);
	DEFINE(PCB_FLAGS_EXPLICIT_EOR);
	DEFINE(PCB_FLAGS_NEEDS_MAPPED_V4);
	DEFINE(PCB_FLAGS_MULTIPLE_ASCONFS);
	DEFINE(PCB_FLAGS_PORTREUSE);
	DEFINE(PCB_FLAGS_DRYEVNT);
	DEFINE(PCB_FLAGS_RECVRCVINFO);
	DEFINE(PCB_FLAGS_RECVNXTINFO);
	DEFINE(PCB_FLAGS_ASSOC_RESETEVNT);
	DEFINE(PCB_FLAGS_STREAM_CHANGEEVNT);
	DEFINE(PCB_FLAGS_RECVNSENDFAILEVNT);

	DEFINE(MOBILITY_BASE);
	DEFINE(MOBILITY_FASTHANDOFF);
	DEFINE(MOBILITY_PRIM_DELETED);

	DEFINE(SMALLEST_PMTU);
	DEFINE(LARGEST_PMTU);

	/* sctp_uio.h */
	DEFINE(FUTURE_ASSOC);
	DEFINE(CURRENT_ASSOC);
	DEFINE(ALL_ASSOC);
	DEFINE(INIT);
	DEFINE(SNDRCV);
	DEFINE(EXTRCV);
	DEFINE(SNDINFO);
	DEFINE(RCVINFO);
	DEFINE(NXTINFO);
	DEFINE(PRINFO);
	DEFINE(AUTHINFO);
	DEFINE(DSTADDRV4);
	DEFINE(DSTADDRV6);
	DEFINE(ALIGN_RESV_PAD);
	DEFINE(ALIGN_RESV_PAD_SHORT);
	DEFINE(NO_NEXT_MSG);
	DEFINE(NEXT_MSG_AVAIL);
	DEFINE(NEXT_MSG_ISCOMPLETE);
	DEFINE(NEXT_MSG_IS_UNORDERED);
	DEFINE(NEXT_MSG_IS_NOTIFICATION);
	DEFINE(RECVV_NOINFO);
	DEFINE(RECVV_RCVINFO);
	DEFINE(RECVV_NXTINFO);
	DEFINE(RECVV_RN);
	DEFINE(SENDV_NOINFO);
	DEFINE(SENDV_SNDINFO);
	DEFINE(SENDV_PRINFO);
	DEFINE(SENDV_AUTHINFO);
	DEFINE(SENDV_SPA);
	DEFINE(SEND_SNDINFO_VALID);
	DEFINE(SEND_PRINFO_VALID);
	DEFINE(SEND_AUTHINFO_VALID);
	DEFINE(NOTIFICATION);
	DEFINE(COMPLETE);
	DEFINE(EOF);
	DEFINE(ABORT);
	DEFINE(UNORDERED);
	DEFINE(ADDR_OVER);
	DEFINE(SENDALL);
	DEFINE(EOR);
	DEFINE(SACK_IMMEDIATELY);
	DEFINE(PR_SCTP_NONE);
	DEFINE(PR_SCTP_TTL);
	DEFINE(PR_SCTP_PRIO);
	DEFINE(PR_SCTP_BUF);
	DEFINE(PR_SCTP_RTX);
	DEFINE(PR_SCTP_MAX);
	DEFINE(PR_SCTP_ALL);
	DEFINE(COMM_UP);
	DEFINE(COMM_LOST);
	DEFINE(RESTART);
	DEFINE(SHUTDOWN_COMP);
	DEFINE(CANT_STR_ASSOC);
	DEFINE(ASSOC_SUPPORTS_PR);
	DEFINE(ASSOC_SUPPORTS_AUTH);
	DEFINE(ASSOC_SUPPORTS_ASCONF);
	DEFINE(ASSOC_SUPPORTS_MULTIBUF);
	DEFINE(ASSOC_SUPPORTS_RE_CONFIG);
	DEFINE(ASSOC_SUPPORTS_INTERLEAVING);
	DEFINE(ASSOC_SUPPORTS_MAX);
	DEFINE(ADDR_AVAILABLE);
	DEFINE(ADDR_UNREACHABLE);
	DEFINE(ADDR_REMOVED);
	DEFINE(ADDR_ADDED);
	DEFINE(ADDR_MADE_PRIM);
	DEFINE(ADDR_CONFIRMED);
	DEFINE(ACTIVE);
	DEFINE(INACTIVE);
	DEFINE(UNCONFIRMED);
	DEFINE(DATA_UNSENT);
	DEFINE(DATA_SENT);
	DEFINE(PARTIAL_DELIVERY_ABORTED);
	DEFINE(AUTH_NEW_KEY);
	DEFINE(AUTH_NO_AUTH);
	DEFINE(AUTH_FREE_KEY);
	DEFINE(STREAM_RESET_INCOMING_SSN);
	DEFINE(STREAM_RESET_OUTGOING_SSN);
	DEFINE(STREAM_RESET_DENIED);
	DEFINE(STREAM_RESET_FAILED);
	DEFINE(ASSOC_RESET_DENIED);
	DEFINE(ASSOC_RESET_FAILED);
	DEFINE(STREAM_CHANGE_DENIED);
	DEFINE(STREAM_CHANGE_FAILED);
	DEFINE(ASSOC_CHANGE);
	DEFINE(PEER_ADDR_CHANGE);
	DEFINE(REMOTE_ERROR);
	DEFINE(SEND_FAILED);
	DEFINE(SHUTDOWN_EVENT);
	DEFINE(ADAPTATION_INDICATION);
	DEFINE(PARTIAL_DELIVERY_EVENT);
	DEFINE(AUTHENTICATION_EVENT);
	DEFINE(STREAM_RESET_EVENT);
	DEFINE(SENDER_DRY_EVENT);
	DEFINE(NOTIFICATIONS_STOPPED_EVENT);
	DEFINE(ASSOC_RESET_EVENT);
	DEFINE(STREAM_CHANGE_EVENT);
	DEFINE(SEND_FAILED_EVENT);
	DEFINE(AUTH_HMAC_ID_RSVD);
	DEFINE(AUTH_HMAC_ID_SHA1);
	DEFINE(AUTH_HMAC_ID_SHA256);
	DEFINE(MAX_EXPLICT_STR_RESET); /* XXX: typo? */
#ifdef SCTP_EDMID_NONE
	DEFINE(EDMID_NONE);
#endif
#ifdef SCTP_EDMID_LOWER_LAYER_DTLS
	DEFINE(EDMID_LOWER_LAYER_DTLS);
#endif
	DEFINE(MAX_LOGGING_SIZE);
	DEFINE(TRACE_PARAMS);
	/* /sctp_uio.h */

	DEFINE(PACKET_LOG_SIZE);

	DEFINE(MAX_SACK_DELAY);
	DEFINE(MAX_HB_INTERVAL);
	DEFINE(MIN_COOKIE_LIFE);
	DEFINE(MAX_COOKIE_LIFE);

	DEFINE(BLK_LOGGING_ENABLE);
	DEFINE(CWND_MONITOR_ENABLE);
	DEFINE(CWND_LOGGING_ENABLE);
	DEFINE(FLIGHT_LOGGING_ENABLE);
	DEFINE(FR_LOGGING_ENABLE);
	DEFINE(LOCK_LOGGING_ENABLE);
	DEFINE(MAP_LOGGING_ENABLE);
	DEFINE(MBCNT_LOGGING_ENABLE);
	DEFINE(MBUF_LOGGING_ENABLE);
	DEFINE(NAGLE_LOGGING_ENABLE);
	DEFINE(RECV_RWND_LOGGING_ENABLE);
	DEFINE(RTTVAR_LOGGING_ENABLE);
	DEFINE(SACK_LOGGING_ENABLE);
	DEFINE(SACK_RWND_LOGGING_ENABLE);
	DEFINE(SB_LOGGING_ENABLE);
	DEFINE(STR_LOGGING_ENABLE);
	DEFINE(WAKE_LOGGING_ENABLE);
	DEFINE(LOG_MAXBURST_ENABLE);
	DEFINE(LOG_RWND_ENABLE);
	DEFINE(LOG_SACK_ARRIVALS_ENABLE);
	DEFINE(LTRACE_CHUNK_ENABLE);
	DEFINE(LTRACE_ERROR_ENABLE);
	DEFINE(LAST_PACKET_TRACING);
	DEFINE(THRESHOLD_LOGGING);
	DEFINE(LOG_AT_SEND_2_SCTP);
	DEFINE(LOG_AT_SEND_2_OUTQ);
	DEFINE(LOG_TRY_ADVANCE);
#undef DEFINE
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	/* sctp_uio.h */
	DEFINE(SPP_HB_ENABLE);
	DEFINE(SPP_HB_DISABLE);
	DEFINE(SPP_HB_DEMAND);
	DEFINE(SPP_PMTUD_ENABLE);
	DEFINE(SPP_PMTUD_DISABLE);
	DEFINE(SPP_HB_TIME_IS_ZERO);
	DEFINE(SPP_IPV6_FLOWLABEL);
	DEFINE(SPP_DSCP);
	DEFINE(SPP_IPV4_TOS);
	/* /sctp_uio.h */
#undef DEFINE
	return (1);
}
