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
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"
#include "utils.h"

int luaopen_freebsd_sys_socket(lua_State *);

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

static const struct luaL_Reg l_socket_funcs[] = {
	{"sendfile", l_sendfile},
	{NULL, NULL}
};

int
luaopen_freebsd_sys_socket(lua_State *L)
{
	luaL_newlib(L, l_socket_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(SF_NODISKIO);
	SETCONST(SF_NOCACHE);
	SETCONST(SF_USER_READAHEAD);
#undef SETCONST
	return (1);
}
