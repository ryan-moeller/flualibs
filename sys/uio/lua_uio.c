/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/uio.h>
#include <errno.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "lua_uio.h"
#include "utils.h"

int luaopen_sys_uio(lua_State *);

static int
l_readv(lua_State *L)
{
	struct iovec *iovs;
	size_t niov;
	ssize_t len;
	int fd;

	fd = checkfd(L, 1);
	iovs = checkriovecs(L, 2, &niov);

	if ((len = readv(fd, iovs, niov)) == -1) {
		int error = errno;

		freeriovecs(iovs, niov);
		return (fail(L, error));
	}
	pushriovecs(L, iovs, niov);
	lua_pushinteger(L, len);
	return (2);
}

static int
l_preadv(lua_State *L)
{
	struct iovec *iovs;
	size_t niov;
	off_t offset;
	ssize_t len;
	int fd;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 3);
	iovs = checkriovecs(L, 2, &niov);

	if ((len = preadv(fd, iovs, niov, offset)) == -1) {
		int error = errno;

		freeriovecs(iovs, niov);
		return (fail(L, error));
	}
	pushriovecs(L, iovs, niov);
	lua_pushinteger(L, len);
	return (2);
}

static int
l_writev(lua_State *L)
{
	struct iovec *iovs;
	size_t niov;
	ssize_t len;
	int fd;

	fd = checkfd(L, 1);
	iovs = checkwiovecs(L, 2, &niov);

	if ((len = writev(fd, iovs, niov)) == -1) {
		int error = errno;

		free(iovs);
		return (fail(L, error));
	}
	free(iovs);
	lua_pushinteger(L, len);
	return (1);
}

static int
l_pwritev(lua_State *L)
{
	struct iovec *iovs;
	size_t niov;
	off_t offset;
	ssize_t len;
	int fd;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 3);
	iovs = checkwiovecs(L, 2, &niov);

	if ((len = pwritev(fd, iovs, niov, offset)) == -1) {
		int error = errno;

		free(iovs);
		return (fail(L, error));
	}
	free(iovs);
	lua_pushinteger(L, len);
	return (1);
}

static const struct luaL_Reg l_uio_funcs[] = {
	{"readv", l_readv},
	{"preadv", l_preadv},
	{"writev", l_writev},
	{"pwritev", l_pwritev},
	{NULL, NULL}
};

int
luaopen_sys_uio(lua_State *L)
{
	luaL_newlib(L, l_uio_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, UIO_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(READ);
	DEFINE(WRITE);
	DEFINE(USERSPACE);
	DEFINE(SYSSPACE);
	DEFINE(NOCOPY);
#undef DEFINE
	return (1);
}
