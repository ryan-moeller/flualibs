/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

int luaopen_copy_file_range(lua_State *);

static inline int
checkfd(lua_State *L, int idx)
{
	luaL_Stream *s;
	int fd;

	if (lua_isinteger(L, idx)) {
		return (lua_tointeger(L, idx));
	}
	s = luaL_checkudata(L, idx, LUA_FILEHANDLE);

	if ((fd = fileno(s->f)) == -1) {
		return (fatal(L, "fileno", errno));
	}
	return (fd);
}

static int
l_copy_file_range(lua_State *L)
{
	off_t *inoffp, *outoffp;
	off_t inoff, outoff;
	size_t len;
	ssize_t copied;
	unsigned int flags;
	int infd, outfd;

	lua_remove(L, 1); /* func table from the __call metamethod */
	infd = checkfd(L, 1);
	if (lua_isnil(L, 2)) {
		inoffp = NULL;
	} else {
		inoff = luaL_checkinteger(L, 2);
		inoffp = &inoff;
	}
	outfd = checkfd(L, 3);
	if (lua_isnil(L, 4)) {
		outoffp = NULL;
	} else {
		outoff = luaL_checkinteger(L, 4);
		outoffp = &outoff;
	}
	len = luaL_checkinteger(L, 5);
	flags = luaL_checkinteger(L, 6);

	if ((copied = copy_file_range(infd, inoffp, outfd, outoffp, len, flags))
	    == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, copied);
	if (inoffp == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushinteger(L, inoff);
	}
	if (outoffp == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushinteger(L, outoff);
	}
	return (3);
}

int
luaopen_copy_file_range(lua_State *L)
{
	lua_newtable(L);
#ifdef COPY_FILE_RANGE_CLONE
	lua_pushinteger(L, COPY_FILE_RANGE_CLONE);
	lua_setfield(L, -2, "CLONE");
#endif
	lua_pushinteger(L, SSIZE_MAX);
	lua_setfield(L, -2, "SSIZE_MAX");
	/* Set up a metatable for the library so we can call it. */
	lua_newtable(L);
	lua_pushcfunction(L, l_copy_file_range);
	lua_setfield(L, -2, "__call");
	return (lua_setmetatable(L, -2));
}
