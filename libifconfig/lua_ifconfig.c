/*
 * Copyright (c) 2020-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define _WANT_FREEBSD_BITSET

#include <sys/param.h>
#include <sys/bitset.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_bridgevar.h>
#include <net/if_dl.h>
#include <net/ieee8023ad_lacp.h>
#include <net/if_lagg.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/ip_carp.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#include <assert.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysdecode.h>
#include <sysexits.h>

#include <libifconfig.h>
#include <libifconfig_sfp.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/socket/lua_socket.h"

#define IFCONFIG_HANDLE_META "ifconfig_handle_t"
#define STRUCT_IFADDRS_META "struct ifaddrs *"
#define SFP_DUMP_META "struct ifconfig_sfp_dump"

#define IFFBITS       \
    "\020"            \
    "\001UP"          \
    "\002BROADCAST"   \
    "\003DEBUG"       \
    "\004LOOPBACK"    \
    "\005POINTOPOINT" \
    "\007RUNNING"     \
    "\010NOARP"       \
    "\011PROMISC"     \
    "\012ALLMULTI"    \
    "\013OACTIVE"     \
    "\014SIMPLEX"     \
    "\015LINK0"       \
    "\016LINK1"       \
    "\017LINK2"       \
    "\020MULTICAST"   \
    "\022PPROMISC"    \
    "\023MONITOR"     \
    "\024STATICARP"

#define IFCAPBITS        \
    "\020"               \
    "\001RXCSUM"         \
    "\002TXCSUM"         \
    "\003NETCONS"        \
    "\004VLAN_MTU"       \
    "\005VLAN_HWTAGGING" \
    "\006JUMBO_MTU"      \
    "\007POLLING"        \
    "\010VLAN_HWCSUM"    \
    "\011TSO4"           \
    "\012TSO6"           \
    "\013LRO"            \
    "\014WOL_UCAST"      \
    "\015WOL_MCAST"      \
    "\016WOL_MAGIC"      \
    "\017TOE4"           \
    "\020TOE6"           \
    "\021VLAN_HWFILTER"  \
    "\023VLAN_HWTSO"     \
    "\024LINKSTATE"      \
    "\025NETMAP"         \
    "\026RXCSUM_IPV6"    \
    "\027TXCSUM_IPV6"    \
    "\031TXRTLMT"        \
    "\032HWRXTSTMP"      \
    "\033NOMAP"          \
    "\034TXTLS4"         \
    "\035TXTLS6"

#define IN6BITS      \
    "\020"           \
    "\001anycast"    \
    "\002tenative"   \
    "\003duplicated" \
    "\004detached"   \
    "\005deprecated" \
    "\007nodad"      \
    "\010autoconf"   \
    "\011temporary"  \
    "\012prefer_source"

#define ND6BITS            \
    "\020"                 \
    "\001nud"              \
    "\002accept_rtadv"     \
    "\003prefer_source"    \
    "\004ifdisabled"       \
    "\005dont_set_ifroute" \
    "\006auto_linklocal"   \
    "\007no_radr"          \
    "\010no_prefer_iface"  \
    "\011no_dad"           \
    "\012ipv6_only"        \
    "\013ipv6_only"

#define LAGGHASHBITS \
    "\020"           \
    "\001l2"         \
    "\002l3"         \
    "\003l4"

static const char *carp_states[] = { CARP_STATES };
static const struct lagg_protos lagg_protos[] = LAGG_PROTOS;
static const char *stp_states[] = { STP_STATES };
static const char *stp_protos[] = { STP_PROTOS };
static const char *stp_roles[] = { STP_ROLES };

int luaopen_ifconfig(lua_State *);

/*
 * Create an array of the set flags in v, using the kernel %b format bits.
 */
static void
push_flags_array(lua_State *L, unsigned v, const char *bits)
{
	int i, len;

	lua_newtable(L);

	/* Skip numeric format. */
	++bits;

	while ((i = *bits++) != '\0') {
		if ((v & (1 << (i - 1))) != 0) {
			for (len = 0; bits[len] > 32; ++len);
			lua_pushlstring(L, bits, len);
			lua_rawseti(L, -2, i);
			bits += len;
		} else {
			for (; *bits > 32; ++bits);
		}
	}
}

/*
 * Decode an array of the flags to set and return as an int.
 */
static int
pop_flags_array(lua_State *L, int index, const char *bits)
{
	int v = 0;

	/* Skip numeric format. */
	++bits;

	lua_pushnil(L); /* first key */
	while (lua_next(L, index) != 0) {
		const char *flag, *flags = bits;
		int i, len;

		luaL_checkstring(L, index + 2);

		flag = lua_tostring(L, index + 2);
		lua_pop(L, 1);
		while ((i = *flags++) != '\0') {
			for (len = 0; flags[len] > 32; ++len);
			if (strncmp(flag, flags, len) == 0) {
				v |= 1 << (i - 1);
				break;
			}
			flags += len;
		}
	}

	return (v);
}

/*
 * Push a table describing media information.
 */
static void
push_media_info(lua_State *L, ifmedia_t media)
{
	const char *name, **opts;

	lua_newtable(L);

	name = ifconfig_media_get_type(media);
	if (name != NULL) {
		lua_pushstring(L, name);
		lua_setfield(L, -2, "type");
	}

	name = ifconfig_media_get_subtype(media);
	if (name != NULL) {
		lua_pushstring(L, name);
		lua_setfield(L, -2, "subtype");
	}

	name = ifconfig_media_get_mode(media);
	if (name != NULL) {
		lua_pushstring(L, name);
		lua_setfield(L, -2, "mode");
	}

	lua_newtable(L);
	opts = ifconfig_media_get_options(media);
	if (opts != NULL) {
		for (size_t i = 0; opts[i] != NULL; ++i) {
			lua_pushstring(L, opts[i]);
			lua_rawseti(L, -2, i);
		}
		free(opts);
	}
	lua_setfield(L, -2, "options");
}

/*
 * Push a table describing an AF_INET address or nil if error.
 */
static void
push_inet4_addrinfo(lua_State *L, ifconfig_handle_t *h, struct ifaddrs *ifa)
{
	char addr_buf[NI_MAXHOST];
	struct ifconfig_inet_addr addr;

	if (ifconfig_inet_get_addrinfo(h, ifa->ifa_name, ifa, &addr) != 0) {
		lua_pushnil(L);
		return;
	}

	lua_newtable(L);

	/* TODO: can this just be inet_ntoa? */
	inet_ntop(AF_INET, &addr.sin->sin_addr, addr_buf, sizeof(addr_buf));
	lua_pushstring(L, addr_buf);
	lua_setfield(L, -2, "inet");

	if (addr.dst != NULL) {
		lua_pushstring(L, inet_ntoa_r(addr.dst->sin_addr, addr_buf,
		    sizeof(addr_buf)));
		lua_setfield(L, -2, "destination");
	}

	lua_pushstring(L, inet_ntoa_r(addr.netmask->sin_addr, addr_buf,
	    sizeof(addr_buf)));
	lua_setfield(L, -2, "netmask");

	if (addr.broadcast != NULL) {
		lua_pushstring(L, inet_ntoa_r(addr.broadcast->sin_addr,
		    addr_buf, sizeof(addr_buf)));
		lua_setfield(L, -2, "broadcast");
	}

	if (addr.vhid != 0) {
		lua_pushinteger(L, addr.vhid);
		lua_setfield(L, -2, "vhid");
	}
}

/*
 * Push a table describing an AF_INET6 address or nil if error.
 */
static void
push_inet6_addrinfo(lua_State *L, ifconfig_handle_t *h, struct ifaddrs *ifa)
{
	char addr_buf[NI_MAXHOST];
	struct ifconfig_inet6_addr addr;
	struct timespec now;

	if (ifconfig_inet6_get_addrinfo(h, ifa->ifa_name, ifa, &addr) != 0) {
		lua_pushnil(L);
		return;
	}

	lua_newtable(L);

	if (getnameinfo((struct sockaddr *)addr.sin6, addr.sin6->sin6_len,
	    addr_buf, sizeof(addr_buf), NULL, 0, NI_NUMERICHOST) != 0)
		/* TODO: can this just be inet_ntoa? */
		inet_ntop(AF_INET6, &addr.sin6->sin6_addr, addr_buf,
		    sizeof(addr_buf));
	lua_pushstring(L, addr_buf);
	lua_setfield(L, -2, "inet6");

	if (addr.dstin6 != NULL) {
		inet_ntop(AF_INET6, addr.dstin6, addr_buf, sizeof(addr_buf));
		lua_pushstring(L, addr_buf);
		lua_setfield(L, -2, "destination");
	}

	lua_pushinteger(L, addr.prefixlen);
	lua_setfield(L, -2, "prefixlen");

	if (addr.sin6->sin6_scope_id != 0) {
		lua_pushinteger(L, addr.sin6->sin6_scope_id);
		lua_setfield(L, -2, "scopeid");
	}

	push_flags_array(L, addr.flags, IN6BITS);
	lua_setfield(L, -2, "flags");

	clock_gettime(CLOCK_MONOTONIC_FAST, &now);
	if (addr.lifetime.ia6t_preferred || addr.lifetime.ia6t_expire) {
		lua_pushinteger(L, MAX(0l,
		    addr.lifetime.ia6t_preferred - now.tv_sec));
		lua_setfield(L, -2, "preferred_lifetime");

		lua_pushinteger(L, MAX(0l,
		    addr.lifetime.ia6t_expire - now.tv_sec));
		lua_setfield(L, -2, "valid_lifetime");
	}

	if (addr.vhid != 0) {
		lua_pushinteger(L, addr.vhid);
		lua_setfield(L, -2, "vhid");
	}
}

/*
 * Push a table describing an AF_LINK address or nil if error.
 */
static void
push_link_addrinfo(lua_State *L, ifconfig_handle_t *h __unused,
    struct ifaddrs *ifa)
{
	char addr_buf[NI_MAXHOST];
	union {
		struct sockaddr *a;
		struct sockaddr_dl *dl;
	} s;
	int n;

	s.a = ifa->ifa_addr;
	if (s.dl == NULL || s.dl->sdl_alen <= 0) {
		lua_pushnil(L);
		return;
	}

	lua_newtable(L);

	switch (s.dl->sdl_type) {
	case IFT_ETHER:
	case IFT_L2VLAN:
	case IFT_BRIDGE:
		if (s.dl->sdl_alen != ETHER_ADDR_LEN)
			break;
		ether_ntoa_r((struct ether_addr *)LLADDR(s.dl), addr_buf);
		lua_pushstring(L, addr_buf);
		lua_setfield(L, -2, "ether");
		break;
	default:
		n = s.dl->sdl_nlen > 0 ? s.dl->sdl_nlen + 1 : 0;
		lua_pushstring(L, link_ntoa(s.dl) + n); /* FIXME: link_ntoa_r */
		lua_setfield(L, -2, "lladdr");
		break;
	}
}

/*
 * Push a table describing an AF_LOCAL address.
 */
static void
push_local_addrinfo(lua_State *L, ifconfig_handle_t *h __unused,
    struct ifaddrs *ifa)
{
	union {
		struct sockaddr *a;
		struct sockaddr_un *un;
	} s;

	lua_newtable(L);

	s.a = ifa->ifa_addr;
	if (strlen(s.un->sun_path) == 0)
		lua_pushstring(L, "-");
	else
		lua_pushstring(L, s.un->sun_path);
	lua_setfield(L, -2, "path");
}

/*
 * Push a table describing SFP channel power.
 */
static void
push_channel_power(lua_State  *L, uint16_t power)
{

	lua_newtable(L);

	lua_pushinteger(L, power);
	lua_setfield(L, -2, "value");

	lua_pushnumber(L, power_mW(power));
	lua_setfield(L, -2, "mW");

	lua_pushnumber(L, power_dBm(power));
	lua_setfield(L, -2, "dBm");
}

/*
 * Push a table describing SFP channel bias current.
 */
static void
push_channel_bias(lua_State  *L, uint16_t bias)
{

	lua_newtable(L);

	lua_pushinteger(L, bias);
	lua_setfield(L, -2, "raw");

	lua_pushnumber(L, bias_mA(bias));
	lua_setfield(L, -2, "mA");
}

/*
 * Check the stack for an ifconfig handle userdata at the given index.
 */
static ifconfig_handle_t *
l_ifconfig_checkhandle(lua_State *L, int index)
{
	ifconfig_handle_t *h, **hp;

	hp = luaL_checkudata(L, index, IFCONFIG_HANDLE_META);
	assert(hp != NULL);
	h = *hp;
	luaL_argcheck(L, h != NULL, index, "invalid ifconfig handle");
	return (h);
}

/*
 * Check the stack for an ifaddrs userdata at the given index.
 */
static struct ifaddrs *
l_ifconfig_checkifaddrs(lua_State *L, int index)
{
	struct ifaddrs *ifa, **ifap;

	ifap = luaL_checkudata(L, index, STRUCT_IFADDRS_META);
	assert(ifap != NULL);
	ifa = *ifap;
	luaL_argcheck(L, ifa != NULL, index, "invalid ifaddr");
	return (ifa);
}

/*
 * Invoke a callback function on the stack with the handle and accumulator
 * from the stack and the given ifaddrs, leaving the handle, callback, and
 * accumulator result on the stack after the call.
 *
 * This is used by the ifconfig_foreach_* functions to invoke a lua callback.
 */
static void
foreach_cb(ifconfig_handle_t *h __unused, struct ifaddrs *ifa, void *udata)
{
	lua_State *L = udata;
	struct ifaddrs **ifap;

	/* Stack: h,cb,acc */

	/* Make copies of the callback and handle positioned for the call. */
	lua_pushvalue(L, 1); /* -> h,cb,acc,h */
	lua_pushvalue(L, 2); /* -> h,cb,acc,h,cb */
	lua_insert(L, 3); /* -> h,cb,cb,acc,h */
	lua_insert(L, 4); /* -> h,cb,cb,h,acc */

	/* Push the ifa userdata. */
	ifap = lua_newuserdatauv(L, sizeof(*ifap), 0); /* -> h,cb,cb,h,acc,ifa */
	luaL_setmetatable(L, STRUCT_IFADDRS_META);
	*ifap = ifa;

	/* Reposition the accumulator to be last so it can be optional. */
	lua_insert(L, 5); /* -> h,cb,cb,h,ifa,acc */

	/* Invoke the callback. */
	lua_call(L, 3, 1); /* -> h,cb,acc */
}

/** Explicit close method
 */
static int
l_ifconfig_handle_close(lua_State *L)
{
	ifconfig_handle_t *h, **hp;

	hp = luaL_checkudata(L, 1, IFCONFIG_HANDLE_META);
	assert(hp != NULL);
	h = *hp;
	if (h != NULL) {
		ifconfig_close(h);
		*hp = NULL;
	}
	return (0);
}

/** Get error info
 * @return	errtype, errno, ioctlname?
 */
static int
l_ifconfig_handle_error(lua_State *L)
{
	ifconfig_handle_t *h;
	ifconfig_errtype errtype;

	h = l_ifconfig_checkhandle(L, 1);

	errtype = ifconfig_err_errtype(h);
	switch (errtype) {
	case OK:
		lua_pushnil(L);
		return (1);
	case OTHER:
		lua_pushstring(L, "OTHER");
		break;
	case IOCTL:
		lua_pushstring(L, "IOCTL");
		break;
	case SOCKET:
		lua_pushstring(L, "SOCKET");
		break;
	default:
		lua_pushstring(L, "<unknown>");
		break;
	}

	lua_pushinteger(L, ifconfig_err_errno(h));

	if (errtype == IOCTL) {
		unsigned long req = ifconfig_err_ioctlreq(h);
		lua_pushstring(L, sysdecode_ioctlname(req));
		return (3);
	}

	return (2);
}

/** Higher-order fold iterator over interfaces
 * @param cb	A callback function (handle, iface, udata) -> udata
 * @param udata	An initial accumulator value (may be nil)
 * @return	The final accumulator value returned by cb
 */
static int
l_ifconfig_foreach_iface(lua_State *L)
{
	const int MAXARGS = 3;
	ifconfig_handle_t *h;
	int n;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	/* Ensure the correct number of args.  Extras are discarded. */
	n = lua_gettop(L);
	if (n == (MAXARGS - 1))
		lua_pushnil(L);
	else if (n > MAXARGS)
		lua_pop(L, n - MAXARGS);

	if (ifconfig_foreach_iface(h, foreach_cb, L) != 0)
		return luaL_error(L, "iteration failed");

	return (1);
}

/** Higher-order fold iterator over interface addresses
 * @param iface	A STRUCT_IFADDRS_META userdata value from foreach_iface
 * @param cb	A callback function (handle, ifaddr, udata) -> udata
 * @param udata	An initial accumulator value (may be nil)
 * @return	The final accumulator value returned by cb
 */
static int
l_ifconfig_foreach_ifaddr(lua_State *L)
{
	const int MAXARGS = 4;
	ifconfig_handle_t *h;
	struct ifaddrs *ifa;
	int n;

	h = l_ifconfig_checkhandle(L, 1);
	ifa = l_ifconfig_checkifaddrs(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	/* Ensure the correct number of args.  Extras are discarded. */
	n = lua_gettop(L);
	if (n == (MAXARGS - 1))
		lua_pushnil(L);
	else if (n > MAXARGS)
		lua_pop(L, n - MAXARGS);
	lua_remove(L, 2);

	ifconfig_foreach_ifaddr(h, ifa, foreach_cb, L);

	return (1);
}

/** Get interface description
 * @param name	The name of an interface
 * @return	Description string if set, otherwise nil
 */
static int
l_ifconfig_get_description(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	char *desc;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_description(h, name, &desc) != 0)
		lua_pushnil(L);
	else
		lua_pushstring(L, desc);

	return (1);
}

/** Set interface description
 * @param name	The name of an interface
 * @param desc	Description string to set
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_description(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name, *desc;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checkstring(L, 3);

	name = lua_tostring(L, 2);
	desc = lua_tostring(L, 3);
	if (ifconfig_set_description(h, name, desc) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Unset interface description
 * @param name	The name of an interface
 * @return	true if success, false if error
 */
static int
l_ifconfig_unset_description(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_unset_description(h, name) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Set interface name
 * @param name	The name of an interface
 * @param newname	A new name for the interface
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_name(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name, *newname;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checkstring(L, 3);

	name = lua_tostring(L, 2);
	newname = lua_tostring(L, 3);
	if (ifconfig_set_name(h, name, newname) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Get original interface name
 * @param name	The name of an interface
 * @return	Original interface name or nil if error
 */
static int
l_ifconfig_get_orig_name(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	char *orig_name;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_orig_name(h, name, &orig_name) != 0)
		lua_pushnil(L);
	else
		lua_pushstring(L, orig_name);

	return (1);
}

#if 0 /* setfib not implemented in libifconfig? */
/** Set interface fib
 * @param name	The name of an interface
 * @param fib	A fib number to use
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_fib(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	ptrdiff_t fib;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checkinteger(L, 3);

	name = lua_tostring(L, 2);
	fib = lua_tointeger(L, 3);
	luaL_argcheck(L, INT_MAX < fib || fib < INT_MIN, 3, "fib out of range");
	if (ifconfig_set_fib(h, name, fib) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}
#endif

/** Get interface fib
 * @param name	The name of an interface
 * @return	fib of the interface or nil if error
 */
static int
l_ifconfig_get_fib(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	int fib;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_fib(h, name, &fib) != 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, fib);

	return (1);
}

/** Set interface mtu
 * @param name	The name of an interface
 * @param mtu	A mtu to use
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_mtu(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	ptrdiff_t mtu;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checkinteger(L, 3);

	name = lua_tostring(L, 2);
	mtu = lua_tointeger(L, 3);
	luaL_argcheck(L, INT_MIN <= mtu && mtu <= INT_MAX, 3, "mtu out of range");
	if (ifconfig_set_mtu(h, name, mtu) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Get interface mtu
 * @param name	The name of an interface
 * @return	mtu of the interface or nil if error
 */
static int
l_ifconfig_get_mtu(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	int mtu;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_mtu(h, name, &mtu) != 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, mtu);

	return (1);
}

/** Get interface nd6 info
 * @param name	The name of an interface
 * @return	An list of the enabled nd6 options or nil if error
 */
static int
l_ifconfig_get_nd6(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct in6_ndireq nd;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_nd6(h, name, &nd) != 0)
		lua_pushnil(L);
	else
		push_flags_array(L, nd.ndi.flags, ND6BITS);

	return (1);
}

/** Set interface metric
 * @param name	The name of an interface
 * @param metric	A metric to use
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_metric(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	ptrdiff_t metric;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checkinteger(L, 3);

	name = lua_tostring(L, 2);
	metric = lua_tointeger(L, 3);
	luaL_argcheck(L, INT_MAX < metric || metric < INT_MIN, 3,
	    "metric out of range");
	if (ifconfig_set_metric(h, name, metric) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Get interface metric
 * @param name	The name of an interface
 * @return	metric of the interface or nil if error
 */
static int
l_ifconfig_get_metric(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	int metric;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_metric(h, name, &metric) != 0)
		lua_pushnil(L);
	else
		lua_pushinteger(L, metric);

	return (1);
}

/** Set interface capabilities
 * @param name	The name of an interface
 * @param caps	An array of the capabilities to be enabled on the interface
 *              (capabilities not listed will be disabled)
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_capabilities(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	int capability;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);

	name = lua_tostring(L, 2);
	capability = pop_flags_array(L, 3, IFCAPBITS);
	if (ifconfig_set_capability(h, name, capability) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Get interface capabilities
 * @param name	The name of an interface
 * @return	A table of the enabled/supported capabilites or nil if error
 */
static int
l_ifconfig_get_capabilities(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifconfig_capabilities capability;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_capability(h, name, &capability) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	lua_pushstring(L, "enabled");
	push_flags_array(L, capability.curcap, IFCAPBITS);
	lua_rawset(L, 3);

	lua_pushstring(L, "supported");
	push_flags_array(L, capability.reqcap, IFCAPBITS);
	lua_rawset(L, 3);

	return (1);
}

/** Get interface groups list
 * @param name	The name of an interface
 * @return	A table of the groups containing the interface or nil if error
 */
static int
l_ifconfig_get_groups(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifgroupreq ifgr;
	struct ifg_req *ifg;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_groups(h, name, &ifgr) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	for (ifg = ifgr.ifgr_groups;
	    ifg != NULL && ifgr.ifgr_len >= sizeof(*ifg);
	    ++ifg, ifgr.ifgr_len -= sizeof(*ifg)) {
		if (strcmp(ifg->ifgrq_group, "all") == 0)
			continue;
		lua_pushstring(L, ifg->ifgrq_group);
		lua_rawseti(L, 3, lua_rawlen(L, 3) + 1);
	}

	return (1);
}

/** Get interface status
 * @param name	The name of an interface
 * @return	Interface status as a string or nil if error
 */
static int
l_ifconfig_get_status(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifstat status;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_get_ifstatus(h, name, &status) != 0)
		lua_pushnil(L);
	else
		lua_pushstring(L, status.ascii);

	return (1);
}

/** Get interface media info
 * @param name	The name of an interface
 * @return	A table describing the interface media or nil if error
 */
static int
l_ifconfig_get_media(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name, *status;
	struct ifmediareq *ifmr;
	struct ifdownreason ifdr;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_media_get_mediareq(h, name, &ifmr) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	push_media_info(L, ifmr->ifm_current);
	lua_setfield(L, 3, "current");

	push_media_info(L, ifmr->ifm_active);
	lua_setfield(L, 3, "active");

	lua_newtable(L);
	for (int i = 0; i < ifmr->ifm_count; ++i) {
		push_media_info(L, ifmr->ifm_ulist[i]);
		lua_rawseti(L, 4, lua_rawlen(L, 4) + 1);
	}
	lua_setfield(L, 3, "supported");

	status = ifconfig_media_get_status(ifmr);
	lua_pushstring(L, status);
	lua_setfield(L, 3, "status");

	if (strcmp(status, "no carrier") == 0 &&
	    ifconfig_media_get_downreason(h, name, &ifdr) == 0) {
		switch (ifdr.ifdr_reason) {
		case IFDR_REASON_MSG:
			lua_pushstring(L, ifdr.ifdr_msg);
			lua_setfield(L, 3, "down_reason");
			break;
		case IFDR_REASON_VENDOR:
			lua_pushinteger(L, ifdr.ifdr_vendor);
			lua_setfield(L, 3, "down_reason");
			break;
		default:
			break;
		}
	}

	free(ifmr);

	return (1);
}

#if __FreeBSD_version > 1400083
/*
 * Push a string describing an AF_INET address.
 */
static void
push_inet4_addr(lua_State *L, struct in_addr *addr)
{
	char addr_buf[INET_ADDRSTRLEN];

	(void)inet_ntop(AF_INET, addr, addr_buf, sizeof(addr_buf));
	lua_pushstring(L, addr_buf);
}

/*
 * Push a string describing an AF_INET6 address.
 */
static void
push_inet6_addr(lua_State *L, struct in6_addr *addr)
{
	char addr_buf[INET6_ADDRSTRLEN];

	(void)inet_ntop(AF_INET6, addr, addr_buf, sizeof(addr_buf));
	lua_pushstring(L, addr_buf);
}
#endif

/*
 * Push a table describing a struct ifconfig_carp.
 */
static void
push_carp(lua_State *L,
#if __FreeBSD_version > 1400083
    struct ifconfig_carp *carpr
#else
    struct carpreq *carpr
#endif
    )
{
	lua_newtable(L);

	lua_pushstring(L, carp_states[carpr->carpr_state]);
	lua_setfield(L, -2, "state");

	lua_pushinteger(L, carpr->carpr_vhid);
	lua_setfield(L, -2, "vhid");

	lua_pushinteger(L, carpr->carpr_advbase);
	lua_setfield(L, -2, "advbase");

	lua_pushinteger(L, carpr->carpr_advskew);
	lua_setfield(L, -2, "advskew");

#if __FreeBSD_version > 1400083
	lua_pushlstring(L, (const char *)carpr->carpr_key, CARP_KEY_LEN);
	lua_setfield(L, -2, "key");

	push_inet4_addr(L, &carpr->carpr_addr);
	lua_setfield(L, -2, "addr");

	push_inet6_addr(L, &carpr->carpr_addr6);
	lua_setfield(L, -2, "addr6");
#endif

#if __FreeBSD_version > 1500018
	lua_pushinteger(L, carpr->carpr_version);
	lua_setfield(L, -2, "version");

	lua_pushinteger(L, carpr->carpr_vrrp_prio);
	lua_setfield(L, -2, "vrrp_prio");

	lua_pushinteger(L, carpr->carpr_vrrp_adv_inter);
	lua_setfield(L, -2, "vrrp_adv_inter");
#endif
}

/** Get interface carp info
 * @param name	The name of an interface
 * @return	A table describing the interface carp vhids or nil if error
 */
static int
l_ifconfig_get_carp(lua_State *L)
{
#if __FreeBSD_version > 1400083
	struct ifconfig_carp carpr[CARP_MAXVHID];
#else
	struct carpreq carpr[CARP_MAXVHID];
#endif
	ifconfig_handle_t *h;
	const char *name;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_carp_get_info(h, name, carpr, nitems(carpr)) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	for (int i = 0; i < carpr[0].carpr_count; ++i) {
		push_carp(L, &carpr[i]);
		lua_rawseti(L, 3, lua_rawlen(L, 3) + 1);
	}

	return (1);
}

#if __FreeBSD_version > 1400083
/** Get interface carp info for specific vhid
 * @param name	The name of an interface
 * @param vhid	The vhid to describe
 * @return	A table describing the interface carp vhid or nil if error
 */
static int
l_ifconfig_get_carp_vhid(lua_State *L)
{
	struct ifconfig_carp carpr;
	ifconfig_handle_t *h;
	const char *name;
	uint32_t vhid;

	h = l_ifconfig_checkhandle(L, 1);
	name = luaL_checkstring(L, 2);
	vhid = luaL_checkinteger(L, 3);

	if (ifconfig_carp_get_vhid(h, name, &carpr, vhid) != 0) {
		lua_pushnil(L);
		return (1);
	}

	push_carp(L, &carpr);

	return (1);
}

/** Set interface carp info
 * @param name	The name of a carp interface
 * @param info	A table describing the carp configuration to set
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_carp_info(lua_State *L)
{
	struct ifconfig_carp carpr;
	struct addrinfo hints, *res;
	ifconfig_handle_t *h;
	const char *name, *state, *key, *addr;
	size_t len;

	h = l_ifconfig_checkhandle(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);

	lua_getfield(L, 3, "vhid");
	carpr.carpr_vhid = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 3, "state");
	state = lua_tostring(L, -1);
	if (state == NULL)
		return luaL_error(L, "invalid state");
	for (int i = 0; i < nitems(carp_states); i++) {
		if (strcmp(state, carp_states[i]) == 0) {
			carpr.carpr_state = i;
			state = NULL;
			break;
		}
	}
	if (state != NULL)
		return luaL_error(L, "unknown state %s", state);
	lua_pop(L, 1);

	lua_getfield(L, 3, "advbase");
	carpr.carpr_advbase = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 3, "advskew");
	carpr.carpr_advskew = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 3, "key");
	key = lua_tolstring(L, -1, &len);
	if (key == NULL || len != sizeof(carpr.carpr_key))
		return luaL_error(L, "invalid key");
	memcpy(carpr.carpr_key, key, sizeof(carpr.carpr_key));
	lua_pop(L, 1);

	lua_getfield(L, 3, "addr");
	addr = lua_tostring(L, -1);
	if (addr == NULL)
		return luaL_error(L, "invalid addr");
	carpr.carpr_addr.s_addr = inet_addr(addr);
	lua_pop(L, 1);

	lua_getfield(L, 3, "addr6");
	addr = lua_tostring(L, -1);
	if (addr == NULL)
		return luaL_error(L, "invalid addr6");
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_NUMERICHOST;
	if (getaddrinfo(addr, NULL, &hints, &res) == 1)
		return luaL_error(L, "invalid addr6");
	memcpy(&carpr.carpr_addr6,
	    &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr,
	    sizeof(carpr.carpr_addr6));
	lua_pop(L, 1);

#if __FreeBSD_version > 1500018
	lua_getfield(L, 3, "version");
	carpr.carpr_version = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 3, "vrrp_prio");
	carpr.carpr_vrrp_prio = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 3, "vrrp_adv_inter");
	carpr.carpr_vrrp_adv_inter = lua_tointeger(L, -1);
	lua_pop(L, 1);
#endif

	if (ifconfig_carp_set_info(h, name, &carpr) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}
#endif

/** Get address info
 * @param addr	An ifaddrs userdata object
 * @return	Table describing the address or nil if error
 */
static int
l_ifconfig_addr_info(lua_State *L)
{
	ifconfig_handle_t *h;
	struct ifaddrs *ifa;

	h = l_ifconfig_checkhandle(L, 1);
	ifa = l_ifconfig_checkifaddrs(L, 2);

	switch (ifa->ifa_addr->sa_family) {
	case AF_INET:
		push_inet4_addrinfo(L, h, ifa);
		break;
	case AF_INET6:
		push_inet6_addrinfo(L, h, ifa);
		break;
	case AF_LINK:
		push_link_addrinfo(L, h, ifa);
		break;
	case AF_LOCAL:
		push_local_addrinfo(L, h, ifa);
		break;
	case AF_UNSPEC:
	default:
		lua_pushnil(L);
		break;
	}

	return (1);
}

/** Get bridge interface status
 * @param name	The name of a bridge interface
 * @return	Table describing the status of the bridge or nil if error
 */
static int
l_ifconfig_get_bridge_status(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifconfig_bridge_status *bridge;
	struct ifbropreq *params;
	struct ifbreq *member;
	uint8_t lladdr[ETHER_ADDR_LEN];
	char addr_buf[NI_MAXHOST];
	uint16_t bprio;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_bridge_get_bridge_status(h, name, &bridge) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	lua_pushinteger(L, bridge->cache_size);
	lua_setfield(L, -2, "address_cache_size");

	lua_pushinteger(L, bridge->cache_lifetime);
	lua_setfield(L, -2, "address_cache_lifetime");

	params = bridge->params;

	lua_pushinteger(L, params->ifbop_priority);
	lua_setfield(L, -2, "priority");

	if (params->ifbop_protocol < nitems(stp_protos))
		lua_pushstring(L, stp_protos[params->ifbop_protocol]);
	else
		lua_pushinteger(L, params->ifbop_protocol);
	lua_setfield(L, -2, "protocol");

	lua_pushinteger(L, params->ifbop_hellotime);
	lua_setfield(L, -2, "hello_time");

	lua_pushinteger(L, params->ifbop_fwddelay);
	lua_setfield(L, -2, "forward_delay");

	lua_pushinteger(L, params->ifbop_holdcount);
	lua_setfield(L, -2, "hold_count");

	lua_pushinteger(L, params->ifbop_maxage);
	lua_setfield(L, -2, "max_age");

	PV2ID(params->ifbop_bridgeid, bprio, lladdr);
	lua_pushstring(L, ether_ntoa_r((struct ether_addr *)lladdr, addr_buf));
	lua_setfield(L, -2, "id");

	PV2ID(params->ifbop_designated_root, bprio, lladdr);
	lua_pushstring(L, ether_ntoa_r((struct ether_addr *)lladdr, addr_buf));
	lua_setfield(L, -2, "root_id");

	lua_pushinteger(L, bprio);
	lua_setfield(L, -2, "root_priority");

	lua_pushinteger(L, params->ifbop_root_path_cost);
	lua_setfield(L, -2, "root_path_cost");

	lua_pushinteger(L, params->ifbop_root_port & 0xfff);
	lua_setfield(L, -2, "root_port");

	lua_newtable(L);

	for (size_t i = 0; i < bridge->members_count; ++i) {
		member = bridge->members + i;

		lua_newtable(L);

		push_flags_array(L, member->ifbr_ifsflags, IFBIFBITS);
		lua_setfield(L, -2, "flags");

		lua_pushinteger(L, member->ifbr_addrmax);
		lua_setfield(L, -2, "ifmaxaddr");

		lua_pushinteger(L, member->ifbr_portno);
		lua_setfield(L, -2, "port");

		lua_pushinteger(L, member->ifbr_priority);
		lua_setfield(L, -2, "priority");

		lua_pushinteger(L, member->ifbr_path_cost);
		lua_setfield(L, -2, "path_cost");

		if (member->ifbr_ifsflags & IFBIF_STP) {
			if (member->ifbr_proto < nitems(stp_protos))
				lua_pushstring(L,
				    stp_protos[member->ifbr_proto]);
			else
				lua_pushinteger(L, member->ifbr_proto);
			lua_setfield(L, -2, "protocol");

			if (member->ifbr_role < nitems(stp_roles))
				lua_pushstring(L,
				    stp_roles[member->ifbr_role]);
			else
				lua_pushinteger(L, member->ifbr_role);
			lua_setfield(L, -2, "role");

			if (member->ifbr_state < nitems(stp_states))
				lua_pushstring(L,
				    stp_states[member->ifbr_state]);
			else
				lua_pushinteger(L, member->ifbr_state);
			lua_setfield(L, -2, "state");
		}

#if __FreeBSD_version > 1500051
		{
			ifbvlan_set_t *vlans = bridge->member_vlans + i;
			size_t bit;

			lua_createtable(L, 0, BIT_COUNT(BRVLAN_SETSIZE, vlans));
			BIT_FOREACH_ISSET(BRVLAN_SETSIZE, bit, vlans) {
				lua_pushboolean(L, true);
				lua_rawseti(L, -2, bit);
			}
			lua_setfield(L, -2, "vlans");
		}
#endif

		lua_setfield(L, -2, member->ifbr_ifsname);
	}
	lua_setfield(L, -2, "members");

#if __FreeBSD_version > 1500056
	push_flags_array(L, bridge->flags, IFBRFBITS);
	lua_setfield(L, -2, "flags");

	if (bridge->defpvid) {
		lua_pushinteger(L, bridge->defpvid);
		lua_setfield(L, -2, "default_pvid");
	}
#endif

	ifconfig_bridge_free_bridge_status(bridge);

	return (1);
}

/** Get lagg interface status
 * @param name	The name of a lagg interface
 * @return	Table describing the status of the lagg or nil if error
 */
static int
l_ifconfig_get_lagg_status(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifconfig_lagg_status *lagg_status;
	const char *protoname = "<unknown>";

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_lagg_get_lagg_status(h, name, &lagg_status) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	for (size_t i = 0; i < nitems(lagg_protos); ++i) {
		 if (lagg_status->ra->ra_proto == lagg_protos[i].lpr_proto) {
			protoname = lagg_protos[i].lpr_name;
			break;
		 }
	}
	lua_pushstring(L, protoname);
	lua_setfield(L, -2, "laggproto");

	push_flags_array(L, lagg_status->rf->rf_flags & LAGG_F_HASHMASK,
	    LAGGHASHBITS);
	lua_setfield(L, -2, "lagghash");

	lua_newtable(L);

	push_flags_array(L, lagg_status->ro->ro_opts, LAGG_OPT_BITS);
	lua_setfield(L, -2, "flags");

	lua_pushinteger(L, lagg_status->ro->ro_flowid_shift);
	lua_setfield(L, -2, "flowid_shift");

	if (lagg_status->ra->ra_proto == LAGG_PROTO_ROUNDROBIN) {
		lua_pushinteger(L, lagg_status->ro->ro_bkt);
		lua_setfield(L, -2, "rr_limit");
	}

	lua_setfield(L, -2, "options");

	lua_newtable(L);

	lua_pushinteger(L, lagg_status->ro->ro_active);
	lua_setfield(L, -2, "active");

	lua_pushinteger(L, lagg_status->ro->ro_flapping);
	lua_setfield(L, -2, "flapping");

	lua_setfield(L, -2, "stats");

	lua_newtable(L);

	for (int i = 0; i < lagg_status->ra->ra_ports; ++i) {
		struct lagg_reqport *lagg_port = &lagg_status->ra->ra_port[i];

		lua_newtable(L);

		push_flags_array(L, lagg_port->rp_flags, LAGG_PORT_BITS);
		lua_setfield(L, -2, "flags");

		if (lagg_status->ra->ra_proto == LAGG_PROTO_LACP) {
			struct lacp_opreq *lacp_port = (struct lacp_opreq *)
			    &lagg_port->rp_lacpreq;

			push_flags_array(L, lacp_port->actor_state,
			    LACP_STATE_BITS);
			lua_setfield(L, -2, "lacp_state");
		}

		lua_setfield(L, -2, lagg_port->rp_portname);
	}

	lua_setfield(L, -2, "ports");

	ifconfig_lagg_free_lagg_status(lagg_status);

	return (1);
}

/** Get the parent laggdev of a laggport interface
 * @param name	The name of a laggport interface
 * @return	Parent laggdev name or nil if error
 */
static int
l_ifconfig_get_laggport_laggdev(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct lagg_reqport lagg_port;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_lagg_get_laggport_status(h, name, &lagg_port) != 0)
		lua_pushnil(L);
	else
		lua_pushstring(L, lagg_port.rp_ifname);

	return (1);
}

/** Destroy an interface
 * @param name	The name of an interface
 * @return	true if success, false if error
 */
static int
l_ifconfig_destroy(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_destroy_interface(h, name) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Create an interface
 * @param name	The name of an interface
 * @return	Created interface name or nil if error
 */
static int
l_ifconfig_create(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	char *ifname;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_create_interface(h, name, &ifname) != 0)
		lua_pushnil(L);
	else
		lua_pushstring(L, ifname);

	return (1);
}

/** Create a vlan interface
 * @param name	The name of a vlan interface to create
 * @param vlandev	The name of an interface to attach to
 * @param vlantag	A vlan number to use
 * @return	The name of the created interface or nil if error
 */
static int
l_ifconfig_create_vlan(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name, *vlandev;
	char *ifname;
	ptrdiff_t vlantag;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checkstring(L, 3);
	luaL_checkinteger(L, 4);

	name = lua_tostring(L, 2);
	vlandev = lua_tostring(L, 3);
	vlantag = lua_tointeger(L, 4);
	luaL_argcheck(L, 0 <= vlantag && vlantag <= UINT16_MAX, 4,
	    "vlantag out of range");
	if (ifconfig_create_interface_vlan(h, name, &ifname, vlandev, vlantag) != 0)
		lua_pushnil(L);
	else
		lua_pushstring(L, ifname);

	return (1);
}

/** Set vlandev and vlantag for a vlan interface
 * @param name	The name of a vlan interface
 * @param vlandev	The name of the interface to attach to
 * @param vlantag	A vlan number to use
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_vlantag(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name, *vlandev;
	ptrdiff_t vlantag;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);
	luaL_checkstring(L, 3);
	luaL_checkinteger(L, 4);

	name = lua_tostring(L, 2);
	vlandev = lua_tostring(L, 3);
	vlantag = lua_tointeger(L, 4);
	luaL_argcheck(L, 0 <= vlantag && vlantag <= UINT16_MAX, 4,
	    "vlantag out of range");
	if (ifconfig_set_vlantag(h, name, vlandev, vlantag) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);

	return (1);
}

/** Get a list of all the interface cloners available on the system
 * @return	a list of the names of known interface cloners or nil if error
 */
static int
l_ifconfig_list_cloners(lua_State *L)
{
	ifconfig_handle_t *h;
	char *cloners;
	size_t cloners_count;

	h = l_ifconfig_checkhandle(L, 1);

	if (ifconfig_list_cloners(h, &cloners, &cloners_count) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	for (size_t i = 0; i < cloners_count; ++i) {
		const char *name = cloners + i * IFNAMSIZ;

		lua_pushstring(L, name);
		lua_rawseti(L, -2, i);
	}

	free(cloners);
	return (1);
}

#if __FreeBSD_version > 1500062
/** Set an interface up or down
 * @param name	The name of an interface
 * @param up	true to set the interface up, false to set it down
 * @return	true if success, false if error
 */
static int
l_ifconfig_set_up(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	bool up;

	h = l_ifconfig_checkhandle(L, 1);
	name = luaL_checkstring(L, 2);
	up = lua_toboolean(L, 3);
	if (ifconfig_set_up(h, name, up) != 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);
	return (1);
}
#endif

/** Get SFP module information
 * @param name	The name of an interface
 * @return	Table describing available info about an SFP module if present,
 *              or nil if no module present or an error occurred
 */
static int
l_ifconfig_get_sfp_info(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifconfig_sfp_info sfp;
	struct ifconfig_sfp_info_strings strings;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_sfp_get_sfp_info(h, name, &sfp) != 0) {
		lua_pushnil(L);
		return (1);
	}
	ifconfig_sfp_get_sfp_info_strings(&sfp, &strings);

	lua_newtable(L);

#define $(enu) \
	if (sfp.sfp_##enu != 0) { \
		lua_newtable(L); \
		lua_pushstring(L, ifconfig_enum_sfp_##enu##_description()); \
		lua_setfield(L, -2, "description"); \
		lua_pushstring(L, strings.sfp_##enu); \
		lua_setfield(L, -2, "string"); \
		lua_pushinteger(L, sfp.sfp_##enu); \
		lua_setfield(L, -2, "value"); \
		lua_setfield(L, -2, "sfp_"#enu); \
	}
	$(id)
	$(conn)
	$(eth_10g)
	$(eth)
	$(fc_len)
	$(cab_tech)
	$(fc_media)
	$(eth_1040g)
	$(eth_ext)
	$(rev)
#undef $

	return (1);
}

/** Get SFP module vendor info
 * @param name	The name of an interface
 * @return	Table of vendor info or nil if error
 */
static int
l_ifconfig_get_sfp_vendor_info(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifconfig_sfp_vendor_info vendor;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_sfp_get_sfp_vendor_info(h, name, &vendor) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	lua_pushstring(L, vendor.name);
	lua_setfield(L, -2, "name");

	lua_pushstring(L, vendor.pn);
	lua_setfield(L, -2, "part_number");

	lua_pushstring(L, vendor.sn);
	lua_setfield(L, -2, "serial_number");

	lua_pushstring(L, vendor.date);
	lua_setfield(L, -2, "date");

	return (1);
}

/** Get SFP module status
 * @param name	The name of an interface
 * @return	Table describing module status if present
 * 		or nil if no module or an error occurs
 */
static int
l_ifconfig_get_sfp_status(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifconfig_sfp_status status;
	size_t channels = 1;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	name = lua_tostring(L, 2);
	if (ifconfig_sfp_get_sfp_status(h, name, &status) != 0) {
		lua_pushnil(L);
		return (1);
	}

	lua_newtable(L);

	if (-40.0 <= status.temp && status.temp <= 125.0) {
		lua_pushnumber(L, status.temp);
		lua_setfield(L, -2, "temperature");
	}

	lua_pushnumber(L, status.voltage);
	lua_setfield(L, -2, "voltage");

	if (status.bitrate != 0) {
		lua_pushnumber(L, status.bitrate);
		lua_setfield(L, -2, "bitrate");

		channels = 4;
	}

	lua_newtable(L);

	for (size_t chan = 0; chan < channels; ++chan) {
		lua_newtable(L);

		push_channel_power(L, status.channel[chan].rx);
		lua_setfield(L, -2, "rx_power");

		push_channel_bias(L, status.channel[chan].tx);
		lua_setfield(L, -2, "tx_bias");

		lua_rawseti(L, -2, chan + 1);
	}

	lua_setfield(L, -2, "channels");

	return (1);
}

/** Get SFP module I2C dump
 * @param name	The name of an interface
 * @return	A userdata blob with the dump or nil if error
 */
static int
l_ifconfig_get_sfp_dump(lua_State *L)
{
	ifconfig_handle_t *h;
	const char *name;
	struct ifconfig_sfp_dump *dump;

	h = l_ifconfig_checkhandle(L, 1);
	luaL_checkstring(L, 2);

	dump = lua_newuserdatauv(L, sizeof(*dump), 0);
	luaL_setmetatable(L, SFP_DUMP_META);

	name = lua_tostring(L, 2);
	if (ifconfig_sfp_get_sfp_dump(h, name, dump) != 0)
		lua_pushnil(L);

	return (1);
}

static const struct luaL_Reg l_ifconfig_handle[] = {
	{"close", l_ifconfig_handle_close},
	{"error", l_ifconfig_handle_error},
	{"foreach_iface", l_ifconfig_foreach_iface},
	{"foreach_ifaddr", l_ifconfig_foreach_ifaddr},
	{"get_description", l_ifconfig_get_description},
	{"set_description", l_ifconfig_set_description},
	{"unset_description", l_ifconfig_unset_description},
	{"set_name", l_ifconfig_set_name},
	{"get_orig_name", l_ifconfig_get_orig_name},
#if 0 /* setfib not implemented in libifconfig? */
	{"set_fib", l_ifconfig_set_fib},
#endif
	{"get_fib", l_ifconfig_get_fib},
	{"set_mtu", l_ifconfig_set_mtu},
	{"get_mtu", l_ifconfig_get_mtu},
	{"get_nd6", l_ifconfig_get_nd6},
	{"set_metric", l_ifconfig_set_metric},
	{"get_metric", l_ifconfig_get_metric},
	{"set_capabilities", l_ifconfig_set_capabilities},
	{"get_capabilities", l_ifconfig_get_capabilities},
	{"get_groups", l_ifconfig_get_groups},
	{"get_status", l_ifconfig_get_status},
	{"get_media", l_ifconfig_get_media},
	{"get_carp", l_ifconfig_get_carp},
	{"addr_info", l_ifconfig_addr_info},
	{"get_bridge_status", l_ifconfig_get_bridge_status},
	{"get_lagg_status", l_ifconfig_get_lagg_status},
	{"get_laggport_laggdev", l_ifconfig_get_laggport_laggdev},
	{"destroy", l_ifconfig_destroy},
	{"create", l_ifconfig_create},
	{"create_vlan", l_ifconfig_create_vlan},
	{"set_vlantag", l_ifconfig_set_vlantag},
	{"list_cloners", l_ifconfig_list_cloners},
#if __FreeBSD_version > 1500062
	{"set_up", l_ifconfig_set_up},
#endif
	{"get_sfp_info", l_ifconfig_get_sfp_info},
	{"get_sfp_vendor_info", l_ifconfig_get_sfp_vendor_info},
	{"get_sfp_status", l_ifconfig_get_sfp_status},
	{"get_sfp_dump", l_ifconfig_get_sfp_dump},
	{NULL, NULL}
};

static void
l_ifconfig_handle_meta(lua_State *L)
{
	luaL_newmetatable(L, IFCONFIG_HANDLE_META);

	/* metatable.__index = metatable */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	/* Automatically close handle when garbage collected. */
	lua_pushcfunction(L, l_ifconfig_handle_close);
	lua_setfield(L, -2, "__gc");

	luaL_setfuncs(L, l_ifconfig_handle, 0);

	lua_pop(L, 1);
}

/** Get interface name
 * @return	Name of the interface as a string
 */
static int
l_struct_ifaddrs_name(lua_State *L)
{
	struct ifaddrs *ifa;

	ifa = l_ifconfig_checkifaddrs(L, 1);

	lua_pushstring(L, ifa->ifa_name);
	return (1);
}

/** Get interface flags
 * @return	List of flags set on this interface
 */
static int
l_struct_ifaddrs_flags(lua_State *L)
{
	struct ifaddrs *ifa;

	ifa = l_ifconfig_checkifaddrs(L, 1);

	push_flags_array(L, ifa->ifa_flags, IFFBITS);
	return (1);
}

/** Get interface address
 * @return	Table describing a sockaddr
 */
static int
l_struct_ifaddrs_addr(lua_State *L)
{
	const struct ifaddrs *ifa;

	ifa = l_ifconfig_checkifaddrs(L, 1);

	pushaddr(L, ifa->ifa_addr);
	return (1);
}

/** Get interface netmask
 * @return	Table describing a sockaddr
 */
static int
l_struct_ifaddrs_netmask(lua_State *L)
{
	const struct ifaddrs *ifa;

	ifa = l_ifconfig_checkifaddrs(L, 1);

	pushaddr(L, ifa->ifa_netmask);
	return (1);
}

/** Get interface destination address
 * @return	Table describing a sockaddr
 */
static int
l_struct_ifaddrs_dstaddr(lua_State *L)
{
	const struct ifaddrs *ifa;

	ifa = l_ifconfig_checkifaddrs(L, 1);

	pushaddr(L, ifa->ifa_dstaddr);
	return (1);
}

static const struct luaL_Reg l_struct_ifaddrs[] = {
	{"name", l_struct_ifaddrs_name},
	{"flags", l_struct_ifaddrs_flags},
	{"addr", l_struct_ifaddrs_addr},
	{"netmask", l_struct_ifaddrs_netmask},
	{"dstaddr", l_struct_ifaddrs_dstaddr},
	{NULL, NULL}
};

static void
l_struct_ifaddrs_meta(lua_State *L)
{
	luaL_newmetatable(L, STRUCT_IFADDRS_META);

	/* metatable.__index = metatable */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_setfuncs(L, l_struct_ifaddrs, 0);

	lua_pop(L, 1);
}

static size_t
hex_dump(char *dst, const uint8_t *src, size_t len)
{
	const size_t LINE_BYTES	= 16;
	char *p = dst;

	for (size_t i = 0; i < len; ++i) {
		char sep;
		if (i == (len - 1))
			sep = '\n';
		else if (((i + 1) % LINE_BYTES) != 0)
			sep = ' ';
		else
			sep = '\n';
		p += sprintf(p, "%02X%c", src[i], sep);
	}

	return (p - dst);
}

static int
l_sfp_dump_tostring(lua_State *L)
{
	struct ifconfig_sfp_dump *dump;
	char *p, *buf;

	dump = luaL_checkudata(L, 1, SFP_DUMP_META);
	assert(dump != NULL);

	p = buf = malloc(SFF_DUMP_SIZE * 4); /* should be sufficiently large */
	assert(p != NULL);
	memset(p, 0, sizeof(buf));

	switch (ifconfig_sfp_dump_region_count(dump)) {
	case 2:
		p += sprintf(p, "SFF8436 DUMP (0xA0 0..81 range):\n");
		p += hex_dump(p, dump->data + QSFP_DUMP0_START,
		    QSFP_DUMP0_SIZE);
		p += sprintf(p, "SFF8436 DUMP (0xA0 128..255 range):\n");
		p += hex_dump(p, dump->data + QSFP_DUMP1_START,
		    QSFP_DUMP1_SIZE);
		break;
	case 1:
		p += sprintf(p, "SFF8472 DUMP (0xA0 0..127 range):\n");
		p += hex_dump(p, dump->data + SFP_DUMP_START, SFP_DUMP_SIZE);
		break;
	default:
		p += sprintf(p, "UNIDENTIFIED DUMP (0xA0 0..255 range):\n");
		p += hex_dump(p, dump->data, sizeof(dump->data));
		break;
	}

	lua_pushstring(L, buf);
	free(buf);
	return (1);
}

static const struct luaL_Reg l_sfp_dump[] = {
	{"__tostring", l_sfp_dump_tostring},
	{NULL, NULL}
};

static void
l_sfp_dump_meta(lua_State *L)
{
	luaL_newmetatable(L, SFP_DUMP_META);

	/* metatable.__index = metatable */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_setfuncs(L, l_sfp_dump, 0);

	lua_pop(L, 1);
}

static int
l_ifconfig_open(lua_State *L)
{
	ifconfig_handle_t *h, **hp;

	if ((h = ifconfig_open()) == NULL) {
		lua_pushnil(L);
		return (1);
	}

	hp = lua_newuserdatauv(L, sizeof(*hp), 0);
	luaL_setmetatable(L, IFCONFIG_HANDLE_META);
	*hp = h;
	return (1);
}

static const struct luaL_Reg l_ifconfig[] = {
	{"open", l_ifconfig_open},
	{NULL, NULL}
};

int
luaopen_ifconfig(lua_State *L)
{
	l_ifconfig_handle_meta(L);
	l_struct_ifaddrs_meta(L);
	l_sfp_dump_meta(L);

	lua_newtable(L);
	luaL_setfuncs(L, l_ifconfig, 0);
	return (1);
}
