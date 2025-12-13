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
