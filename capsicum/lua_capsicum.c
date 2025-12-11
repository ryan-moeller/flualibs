/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ryan Moeller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* XXX: The capsicum API aborts on error. */

#include <sys/param.h>
#include <sys/capsicum.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"
#include "../utils.h"

#define CAP_RIGHTS_METATABLE "acl_t"

int luaopen_capsicum(lua_State *);

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
	luaL_setmetatable(L, CAP_RIGHTS_METATABLE);
	cap_rights_init(rights);
	for (int i = 1; i <= top; i++) {
		cap_rights_set(rights, luaL_checkinteger(L, i));
	}
	return (1);
}

static int
l_cap_rights_get(lua_State *L)
{
	cap_rights_t *rights;
	int fd;

	if (lua_isinteger(L, 1)) {
		fd = lua_tointeger(L, 1);
	} else {
		luaL_Stream *s;

		s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
		if ((fd = fileno(s->f)) == -1) {
			return (fatal(L, "fileno", errno));
		}
	}

	rights = lua_newuserdatauv(L, sizeof(*rights), 0);
	luaL_setmetatable(L, CAP_RIGHTS_METATABLE);

	if (cap_rights_get(fd, rights) == -1) {
		return (fail(L, errno));
	}
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
	if (lua_isinteger(L, 2)) {
		fd = lua_tointeger(L, 2);
	} else {
		luaL_Stream *s;

		s = luaL_checkudata(L, 2, LUA_FILEHANDLE);
		if ((fd = fileno(s->f)) == -1) {
			return (fatal(L, "fileno", errno));
		}
	}
	if (cap_rights_limit(fd, rights) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_cap_fcntls_limit(lua_State *L)
{
	uint32_t fcntlrights;
	int fd;

	if (lua_isinteger(L, 1)) {
		fd = lua_tointeger(L, 1);
	} else {
		luaL_Stream *s;

		s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
		if ((fd = fileno(s->f)) == -1) {
			return (fatal(L, "fileno", errno));
		}
	}
	fcntlrights = luaL_checkinteger(L, 2);

	if (cap_fcntls_limit(fd, fcntlrights) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_cap_fcntls_get(lua_State *L)
{
	uint32_t fcntlrights;
	int fd;

	if (lua_isinteger(L, 1)) {
		fd = lua_tointeger(L, 1);
	} else {
		luaL_Stream *s;

		s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
		if ((fd = fileno(s->f)) == -1) {
			return (fatal(L, "fileno", errno));
		}
	}

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

	if (lua_isinteger(L, 1)) {
		fd = lua_tointeger(L, 1);
	} else {
		luaL_Stream *s;

		s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
		if ((fd = fileno(s->f)) == -1) {
			return (fatal(L, "fileno", errno));
		}
	}
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
	lua_pushboolean(L, true);
	return (1);
}

static int
l_cap_ioctls_get(lua_State *L)
{
	cap_ioctl_t cmds[CAP_IOCTLS_LIMIT_MAX];
	ssize_t n;
	int fd;

	if (lua_isinteger(L, 1)) {
		fd = lua_tointeger(L, 1);
	} else {
		luaL_Stream *s;

		s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
		if ((fd = fileno(s->f)) == -1) {
			return (fatal(L, "fileno", errno));
		}
	}

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
luaopen_capsicum(lua_State *L)
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
#define SETCONST(ident) ({ \
	lua_pushinteger(L, CAP_##ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(READ);
	SETCONST(WRITE);
	SETCONST(SEEK_TELL);
	SETCONST(SEEK);
	SETCONST(PREAD);
	SETCONST(PWRITE);
	SETCONST(MMAP);
	SETCONST(MMAP_R);
	SETCONST(MMAP_W);
	SETCONST(MMAP_X);
	SETCONST(MMAP_RW);
	SETCONST(MMAP_RX);
	SETCONST(MMAP_WX);
	SETCONST(MMAP_RWX);
	SETCONST(CREATE);
	SETCONST(FEXECVE);
	SETCONST(FSYNC);
	SETCONST(FTRUNCATE);
	SETCONST(LOOKUP);
	SETCONST(FCHDIR);
	SETCONST(FCHFLAGS);
	SETCONST(CHFLAGSAT);
	SETCONST(FCHMOD);
	SETCONST(FCHMODAT);
	SETCONST(FCHOWN);
	SETCONST(FCHOWNAT);
	SETCONST(FCNTL);
	SETCONST(FLOCK);
	SETCONST(FPATHCONF);
	SETCONST(FSCK);
	SETCONST(FSTAT);
	SETCONST(FSTATAT);
	SETCONST(FSTATFS);
	SETCONST(FUTIMES);
	SETCONST(FUTIMESAT);
	SETCONST(LINKAT_TARGET);
	SETCONST(MKDIRAT);
	SETCONST(MKFIFOAT);
	SETCONST(MKNODAT);
	SETCONST(RENAMEAT_SOURCE);
	SETCONST(SYMLINKAT);
	SETCONST(UNLINKAT);
	SETCONST(ACCEPT);
	SETCONST(BIND);
	SETCONST(CONNECT);
	SETCONST(GETPEERNAME);
	SETCONST(GETSOCKNAME);
	SETCONST(GETSOCKOPT);
	SETCONST(LISTEN);
	SETCONST(PEELOFF);
	SETCONST(RECV);
	SETCONST(SEND);
	SETCONST(SETSOCKOPT);
	SETCONST(SHUTDOWN);
	SETCONST(BINDAT);
	SETCONST(CONNECTAT);
	SETCONST(LINKAT_SOURCE);
	SETCONST(RENAMEAT_TARGET);
#ifdef CAP_FCHROOT
	SETCONST(FCHROOT);
#endif
	SETCONST(SOCK_CLIENT);
	SETCONST(SOCK_SERVER);
	SETCONST(ALL0);
	SETCONST(UNUSED0_57);
	SETCONST(MAC_GET);
	SETCONST(MAC_SET);
	SETCONST(SEM_GETVALUE);
	SETCONST(SEM_POST);
	SETCONST(SEM_WAIT);
	SETCONST(EVENT);
	SETCONST(KQUEUE_EVENT);
	SETCONST(IOCTL);
	SETCONST(TTYHOOK);
	SETCONST(PDGETPID);
	SETCONST(PDWAIT);
	SETCONST(PDKILL);
	SETCONST(EXTATTR_DELETE);
	SETCONST(EXTATTR_GET);
	SETCONST(EXTATTR_LIST);
	SETCONST(EXTATTR_SET);
	SETCONST(ACL_CHECK);
	SETCONST(ACL_DELETE);
	SETCONST(ACL_GET);
	SETCONST(ACL_SET);
	SETCONST(KQUEUE_CHANGE);
	SETCONST(KQUEUE);
#ifdef CAP_INOTIFY_ADD
	SETCONST(INOTIFY_ADD);
#endif
#ifdef CAP_INOTIFY_RM
	SETCONST(INOTIFY_RM);
#endif
	SETCONST(ALL1);
	SETCONST(UNUSED1_22);
	SETCONST(UNUSED1_57);
	SETCONST(FCNTL_GETFL);
	SETCONST(FCNTL_SETFL);
	SETCONST(FCNTL_GETOWN);
	SETCONST(FCNTL_SETOWN);
	SETCONST(FCNTL_ALL);
	/* CAP_IOCTLS_ALL is useless in Lua */
#undef SETCONST
	return (1);
}
