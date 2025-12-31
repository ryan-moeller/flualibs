/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <capsicum_helpers.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/capsicum/lua_capsicum.h"
#include "luaerror.h"
#include "utils.h"

int luaopen_capsicum_helpers(lua_State *);

static int
l_caph_enter(lua_State *L)
{
	if (caph_enter() == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_enter_casper(lua_State *L)
{
	if (caph_enter_casper() == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_rights_limit(lua_State *L)
{
	cap_rights_t *rights;
	int fd;

	fd = checkfd(L, 1);
	rights = luaL_checkudata(L, 2, CAP_RIGHTS_METATABLE);

	if (caph_rights_limit(fd, rights) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_ioctls_limit(lua_State *L)
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

	for (int i = 2; i <= top; i++) {
		cmds[i - 2] = luaL_checkinteger(L, i);
	}
	if (caph_ioctls_limit(fd, cmds, nitems(cmds)) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_fcntls_limit(lua_State *L)
{
	uint32_t fcntlrights;
	int fd;

	fd = checkfd(L, 1);
	fcntlrights = luaL_checkinteger(L, 2);

	if (caph_fcntls_limit(fd, fcntlrights) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_limit_stream(lua_State *L)
{
	int fd, flags;

	fd = checkfd(L, 1);
	flags = luaL_checkinteger(L, 2);

	if (caph_limit_stream(fd, flags) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_limit_stdin(lua_State *L)
{
	if (caph_limit_stdin() == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_limit_stderr(lua_State *L)
{
	if (caph_limit_stderr() == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_limit_stdout(lua_State *L)
{
	if (caph_limit_stdout() == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_limit_stdio(lua_State *L)
{
	if (caph_limit_stdio() == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_caph_stream_rights(lua_State *L)
{
	cap_rights_t *rights;
	int flags;

	rights = luaL_checkudata(L, 1, CAP_RIGHTS_METATABLE);
	flags = luaL_checkinteger(L, 2);

	caph_stream_rights(rights, flags);
	lua_pushvalue(L, 1);
	return (1);
}

static int
l_caph_cache_tzdata(lua_State *L __unused)
{
	caph_cache_tzdata();
	return (0);
}

static int
l_caph_cache_catpages(lua_State *L __unused)
{
	caph_cache_catpages();
	return (0);
}

static const struct luaL_Reg l_caph_funcs[] = {
	{"enter", l_caph_enter},
	{"enter_casper", l_caph_enter_casper},
	{"rights_limit", l_caph_rights_limit},
	{"ioctls_limit", l_caph_ioctls_limit},
	{"fcntls_limit", l_caph_fcntls_limit},
	{"limit_stream", l_caph_limit_stream},
	{"limit_stdin", l_caph_limit_stdin},
	{"limit_stderr", l_caph_limit_stderr},
	{"limit_stdout", l_caph_limit_stdout},
	{"limit_stdio", l_caph_limit_stdio},
	{"stream_rights", l_caph_stream_rights},
	{"cache_tzdata", l_caph_cache_tzdata},
	{"cache_catpages", l_caph_cache_catpages},
	{NULL, NULL}
};

int
luaopen_capsicum_helpers(lua_State *L)
{
	luaL_newlib(L, l_caph_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, CAPH_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(IGNORE_EBADF);
	DEFINE(READ);
	DEFINE(WRITE);
	DEFINE(LOOKUP);
#undef DEFINE
	return (1);
}
