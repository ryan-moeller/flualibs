/*-
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
#include <sys/socket.h>
#include <sys/uio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

int luaopen_sendfile(lua_State *);

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
			    malloc(hdtr->hdr_cnt * sizeof *hdtr->headers);
			if (hdtr->headers == NULL) {
				return (luaL_error(L, "malloc: %s",
				    strerror(errno)));
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
			    malloc(hdtr->trl_cnt * sizeof *hdtr->trailers);
			if (hdtr->trailers == NULL) {
				free(hdtr->headers);
				return (luaL_error(L, "malloc: %s",
				    strerror(errno)));
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
	int fd, s, flags, readahead, error;

	lua_remove(L, 1); /* func table from the __call metamethod */
	fd = luaL_checkinteger(L, 1);
	s =  luaL_checkinteger(L, 2);
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
	error = sendfile(fd, s, offset, nbytes, hdtrp, &sbytes,
	    SF_FLAGS(readahead, flags));
	if (error == -1) {
		error = errno;
		luaL_pushfail(L);
		lua_pushfstring(L, "sendfile: %s", strerror(error));
		lua_pushinteger(L, error);
		return (3);
	}
	lua_pushinteger(L, sbytes);
	return (1);
}

int
luaopen_sendfile(lua_State *L)
{
	lua_newtable(L);
	/* Set up the library in a metatable so we can call it. */
	lua_createtable(L, 0, 5);
	lua_pushcfunction(L, l_sendfile);
	lua_setfield(L, -2, "__call");
#define SETCONST(ident) ({ \
	lua_pushinteger(L, SF_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(NODISKIO);
	SETCONST(NOCACHE);
	SETCONST(USER_READAHEAD);
#undef SETCONST
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	return (lua_setmetatable(L, -2));
}
