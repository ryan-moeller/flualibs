/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

int luaopen_pathconf(lua_State *);

static int
l_pc_pathconf(lua_State *L)
{
	const char *path;
	long value;
	int name;

	path = luaL_checkstring(L, 1);
	name = luaL_checkinteger(L, 2);

	errno = 0;
	if ((value = pathconf(path, name)) == -1) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_pc_lpathconf(lua_State *L)
{
	const char *path;
	long value;
	int name;

	path = luaL_checkstring(L, 1);
	name = luaL_checkinteger(L, 2);

	errno = 0;
	if ((value = lpathconf(path, name)) == -1) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_pc_fpathconf(lua_State *L)
{
	long value;
	int fd, name;

	if (lua_isinteger(L, 1)) {
		fd = lua_tointeger(L, 1);
	} else {
		luaL_Stream *s;

		s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
		if ((fd = fileno(s->f)) == -1) {
			return (fatal(L, "fileno", errno));
		}
	}
	name = luaL_checkinteger(L, 2);

	errno = 0;
	if ((value = fpathconf(fd, name)) == -1) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static const struct luaL_Reg l_pathconf_funcs[] = {
	{"pathconf", l_pc_pathconf},
	{"lpathconf", l_pc_lpathconf},
	{"fpathconf", l_pc_fpathconf},
	{NULL, NULL}
};

int
luaopen_pathconf(lua_State *L)
{
	luaL_newlib(L, l_pathconf_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, _PC_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(LINK_MAX);
	SETCONST(MAX_CANON);
	SETCONST(MAX_INPUT);
	SETCONST(NAME_MAX);
	SETCONST(PATH_MAX);
	SETCONST(PIPE_BUF);
	SETCONST(CHOWN_RESTRICTED);
	SETCONST(NO_TRUNC);
	SETCONST(VDISABLE);
	SETCONST(ASYNC_IO);
	SETCONST(PRIO_IO);
	SETCONST(SYNC_IO);
	SETCONST(ALLOC_SIZE_MIN);
	SETCONST(FILESIZEBITS);
	SETCONST(REC_INCR_XFER_SIZE);
	SETCONST(REC_MAX_XFER_SIZE);
	SETCONST(REC_MIN_XFER_SIZE);
	SETCONST(REC_XFER_ALIGN);
	SETCONST(SYMLINK_MAX);
	SETCONST(ACL_EXTENDED);
	SETCONST(ACL_PATH_MAX);
	SETCONST(CAP_PRESENT);
	SETCONST(INF_PRESENT);
	SETCONST(MAC_PRESENT);
	SETCONST(ACL_NFS4);
#ifdef _PC_DEALLOC_PRESENT
	SETCONST(DEALLOC_PRESENT);
#endif
#ifdef _PC_NAMEDATTR_ENABLED
	SETCONST(NAMEDATTR_ENABLED);
#endif
#ifdef _PC_HAS_NAMEDATTR
	SETCONST(HAS_NAMEDATTR);
#endif
#ifdef _PC_HAS_HIDDENSYSTEM
	SETCONST(HAS_HIDDENSYSTEM);
#endif
#ifdef _PC_CLONE_BLKSIZE
	SETCONST(CLONE_BLKSIZE);
#endif
#ifdef _PC_CASE_INSENSITIVE
	SETCONST(CASE_INSENSITIVE);
#endif
	SETCONST(MIN_HOLE_SIZE);
#undef SETCONST
	return (1);
}
