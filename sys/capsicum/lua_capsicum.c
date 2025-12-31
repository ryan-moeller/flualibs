/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* XXX: The capsicum API aborts on error. */

#include <sys/param.h>
#include <sys/capsicum.h>
#include <errno.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lua_capsicum.h"
#include "luaerror.h"
#include "utils.h"

int luaopen_sys_capsicum(lua_State *);

static int
l_cap_enter(lua_State *L)
{
	if (cap_enter() == -1) {
		return (fatal(L, "cap_enter", errno));
	}
	return (0);
}

static int
l_cap_sandboxed(lua_State *L)
{
	lua_pushboolean(L, cap_sandboxed());
	return (1);
}

static int
l_cap_rights_new(lua_State *L)
{
	cap_rights_t *rights;
	int top;

	top = lua_gettop(L);

	rights = lua_newuserdatauv(L, sizeof(*rights), 0);
	cap_rights_init(rights);
	for (int i = 1; i <= top; i++) {
		cap_rights_set(rights, luaL_checkinteger(L, i));
	}
	luaL_setmetatable(L, CAP_RIGHTS_METATABLE);
	return (1);
}

static int
l_cap_rights_get(lua_State *L)
{
	cap_rights_t *rights;
	int fd;

	fd = checkfd(L, 1);

	rights = lua_newuserdatauv(L, sizeof(*rights), 0);
	if (cap_rights_get(fd, rights) == -1) {
		return (fail(L, errno));
	}
	luaL_setmetatable(L, CAP_RIGHTS_METATABLE);
	return (1);
}

static int
l_cap_rights_init(lua_State *L)
{
	cap_rights_t *rights;
	int top;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	top = lua_gettop(L);

	cap_rights_init(rights);
	for (int i = 2; i <= top; i++) {
		cap_rights_set(rights, luaL_checkinteger(L, i));
	}
	return (1);
}

static int
l_cap_rights_set(lua_State *L)
{
	cap_rights_t *rights;
	int top;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	top = lua_gettop(L);

	for (int i = 2; i <= top; i++) {
		cap_rights_set(rights, luaL_checkinteger(L, i));
	}
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_rights_clear(lua_State *L)
{
	cap_rights_t *rights;
	int top;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	top = lua_gettop(L);

	for (int i = 2; i <= top; i++) {
		cap_rights_clear(rights, luaL_checkinteger(L, i));
	}
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_rights_is_set(lua_State *L)
{
	cap_rights_t *rights;
	int top;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	top = lua_gettop(L);

	for (int i = 2; i <= top; i++) {
		if (!cap_rights_is_set(rights, luaL_checkinteger(L, i))) {
			lua_pushboolean(L, false);
			return (1);
		}
	}
	lua_pushboolean(L, true);
	return (1);
}

#if __FreeBSD_version > 1500006
static int
l_cap_rights_is_empty(lua_State *L)
{
	cap_rights_t *rights;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);

	lua_pushboolean(L, cap_rights_is_empty(rights));
	return (1);
}
#endif

static int
l_cap_rights_is_valid(lua_State *L)
{
	cap_rights_t *rights;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);

	lua_pushboolean(L, cap_rights_is_valid(rights));
	return (1);
}

static int
l_cap_rights_merge(lua_State *L)
{
	cap_rights_t *dst, *src;

	dst = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	src = luaL_checkudata(L, 2, CAP_RIGHTS_METATABLE);

	cap_rights_merge(dst, src);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_rights_remove(lua_State *L)
{
	cap_rights_t *dst, *src;

	dst = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	src = luaL_checkudata(L, 2, CAP_RIGHTS_METATABLE);

	cap_rights_remove(dst, src);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_cap_rights_contains(lua_State *L)
{
	cap_rights_t *big, *little;

	big = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	little = luaL_checkudata(L, 2, CAP_RIGHTS_METATABLE);

	lua_pushboolean(L, cap_rights_contains(big, little));
	return (1);
}

static int
l_cap_rights_limit(lua_State *L)
{
	cap_rights_t *rights;
	int fd;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	fd = checkfd(L, 2);

	if (cap_rights_limit(fd, rights) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_fcntls_limit(lua_State *L)
{
	uint32_t fcntlrights;
	int fd;

	fd = checkfd(L, 1);
	fcntlrights = luaL_checkinteger(L, 2);

	if (cap_fcntls_limit(fd, fcntlrights) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_fcntls_get(lua_State *L)
{
	uint32_t fcntlrights;
	int fd;

	fd = checkfd(L, 1);

	if (cap_fcntls_get(fd, &fcntlrights) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fcntlrights);
	return (1);
}

#define CAP_IOCTLS_LIMIT_MAX 256

static int
l_cap_ioctls_limit(lua_State *L)
{
	cap_ioctl_t cmds[CAP_IOCTLS_LIMIT_MAX];
	lua_Integer n;
	int fd, top;

	fd = checkfd(L, 1);
	top = lua_gettop(L);
	if (top - 1 > CAP_IOCTLS_LIMIT_MAX) {
		return (luaL_error(L, "too many cmds (max %d)",
		    CAP_IOCTLS_LIMIT_MAX));
	}

	for (int i = 2; i < top; i++) {
		cmds[i - 2] = luaL_checkinteger(L, i);
	}
	if (cap_ioctls_limit(fd, cmds, nitems(cmds)) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_cap_ioctls_get(lua_State *L)
{
	cap_ioctl_t cmds[CAP_IOCTLS_LIMIT_MAX];
	ssize_t n;
	int fd;

	fd = checkfd(L, 1);

	if ((n = cap_ioctls_get(fd, cmds, nitems(cmds))) == -1) {
		return (fail(L, errno));
	}
	if (n == CAP_IOCTLS_ALL) {
		lua_pushboolean(L, false);
		return (1);
	}
	lua_createtable(L, n, 0);
	for (lua_Integer i = 0; i < n; i++) {
		lua_pushinteger(L, cmds[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static const struct luaL_Reg l_capsicum_funcs[] = {
	{"enter", l_cap_enter},
	{"sandboxed", l_cap_sandboxed},
	{NULL, NULL}
};

static const struct luaL_Reg l_cap_rights_funcs[] = {
	{"new", l_cap_rights_new},
	{"get", l_cap_rights_get},
	{NULL, NULL}
};

static const struct luaL_Reg l_cap_rights_meta[] = {
	{"init", l_cap_rights_init},
	{"set", l_cap_rights_set},
	{"clear", l_cap_rights_clear},
	{"is_set", l_cap_rights_is_set},
#if __FreeBSD_version > 1500006
	{"is_empty", l_cap_rights_is_empty},
#endif
	{"is_valid", l_cap_rights_is_valid},
	{"merge", l_cap_rights_merge},
	{"remove", l_cap_rights_remove},
	{"contains", l_cap_rights_contains},
	{"limit", l_cap_rights_limit},
	{NULL, NULL}
};

static const struct luaL_Reg l_cap_fcntls_funcs[] = {
	{"limit", l_cap_fcntls_limit},
	{"get", l_cap_fcntls_get},
	{NULL, NULL}
};

static const struct luaL_Reg l_cap_ioctls_funcs[] = {
	{"limit", l_cap_ioctls_limit},
	{"get", l_cap_ioctls_get},
	{NULL, NULL}
};

int
luaopen_sys_capsicum(lua_State *L)
{
	luaL_newmetatable(L, CAP_RIGHTS_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_cap_rights_meta, 0);

	luaL_newlib(L, l_capsicum_funcs);
	luaL_newlib(L, l_cap_rights_funcs);
	lua_setfield(L, -2, "rights");
	luaL_newlib(L, l_cap_fcntls_funcs);
	lua_setfield(L, -2, "fcntls");
	luaL_newlib(L, l_cap_ioctls_funcs);
	lua_setfield(L, -2, "ioctls");
#define DEFINE(ident) ({ \
	lua_pushinteger(L, CAP_##ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(READ);
	DEFINE(WRITE);
	DEFINE(SEEK_TELL);
	DEFINE(SEEK);
	DEFINE(PREAD);
	DEFINE(PWRITE);
	DEFINE(MMAP);
	DEFINE(MMAP_R);
	DEFINE(MMAP_W);
	DEFINE(MMAP_X);
	DEFINE(MMAP_RW);
	DEFINE(MMAP_RX);
	DEFINE(MMAP_WX);
	DEFINE(MMAP_RWX);
	DEFINE(CREATE);
	DEFINE(FEXECVE);
	DEFINE(FSYNC);
	DEFINE(FTRUNCATE);
	DEFINE(LOOKUP);
	DEFINE(FCHDIR);
	DEFINE(FCHFLAGS);
	DEFINE(CHFLAGSAT);
	DEFINE(FCHMOD);
	DEFINE(FCHMODAT);
	DEFINE(FCHOWN);
	DEFINE(FCHOWNAT);
	DEFINE(FCNTL);
	DEFINE(FLOCK);
	DEFINE(FPATHCONF);
	DEFINE(FSCK);
	DEFINE(FSTAT);
	DEFINE(FSTATAT);
	DEFINE(FSTATFS);
	DEFINE(FUTIMES);
	DEFINE(FUTIMESAT);
	DEFINE(LINKAT_TARGET);
	DEFINE(MKDIRAT);
	DEFINE(MKFIFOAT);
	DEFINE(MKNODAT);
	DEFINE(RENAMEAT_SOURCE);
	DEFINE(SYMLINKAT);
	DEFINE(UNLINKAT);
	DEFINE(ACCEPT);
	DEFINE(BIND);
	DEFINE(CONNECT);
	DEFINE(GETPEERNAME);
	DEFINE(GETSOCKNAME);
	DEFINE(GETSOCKOPT);
	DEFINE(LISTEN);
	DEFINE(PEELOFF);
	DEFINE(RECV);
	DEFINE(SEND);
	DEFINE(SETSOCKOPT);
	DEFINE(SHUTDOWN);
	DEFINE(BINDAT);
	DEFINE(CONNECTAT);
	DEFINE(LINKAT_SOURCE);
	DEFINE(RENAMEAT_TARGET);
#ifdef CAP_FCHROOT
	DEFINE(FCHROOT);
#endif
	DEFINE(SOCK_CLIENT);
	DEFINE(SOCK_SERVER);
	DEFINE(ALL0);
	DEFINE(UNUSED0_57);
	DEFINE(MAC_GET);
	DEFINE(MAC_SET);
	DEFINE(SEM_GETVALUE);
	DEFINE(SEM_POST);
	DEFINE(SEM_WAIT);
	DEFINE(EVENT);
	DEFINE(KQUEUE_EVENT);
	DEFINE(IOCTL);
	DEFINE(TTYHOOK);
	DEFINE(PDGETPID);
	DEFINE(PDWAIT);
	DEFINE(PDKILL);
	DEFINE(EXTATTR_DELETE);
	DEFINE(EXTATTR_GET);
	DEFINE(EXTATTR_LIST);
	DEFINE(EXTATTR_SET);
	DEFINE(ACL_CHECK);
	DEFINE(ACL_DELETE);
	DEFINE(ACL_GET);
	DEFINE(ACL_SET);
	DEFINE(KQUEUE_CHANGE);
	DEFINE(KQUEUE);
#ifdef CAP_INOTIFY_ADD
	DEFINE(INOTIFY_ADD);
#endif
#ifdef CAP_INOTIFY_RM
	DEFINE(INOTIFY_RM);
#endif
	DEFINE(ALL1);
	DEFINE(UNUSED1_22);
	DEFINE(UNUSED1_57);
	DEFINE(FCNTL_GETFL);
	DEFINE(FCNTL_SETFL);
	DEFINE(FCNTL_GETOWN);
	DEFINE(FCNTL_SETOWN);
	DEFINE(FCNTL_ALL);
	/* CAP_IOCTLS_ALL is useless in Lua */
#undef DEFINE
	return (1);
}
