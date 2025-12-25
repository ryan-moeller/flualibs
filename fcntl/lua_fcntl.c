/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <fcntl.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

int luaopen_fcntl(lua_State *);

static int
l_posix_fadvise(lua_State *L)
{
	off_t offset, len;
	int fd, advice, error;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 2);
	len = luaL_checkinteger(L, 3);
	advice = luaL_checkinteger(L, 4);

	if ((error = posix_fadvise(fd, offset, len, advice)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_posix_fallocate(lua_State *L)
{
	off_t offset, len;
	int fd, error;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 2);
	len = luaL_checkinteger(L, 3);

	if ((error = posix_fallocate(fd, offset, len)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

/* TODO: open, fcntl, flock, fspacectl, openat */

static const struct luaL_Reg l_fcntl_funcs[] = {
	{"posix_fadvise", l_posix_fadvise},
	{"posix_fallocate", l_posix_fallocate},
	{NULL, NULL}
};

int
luaopen_fcntl(lua_State *L)
{
	luaL_newlib(L, l_fcntl_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(O_RDONLY);
	DEFINE(O_WRONLY);
	DEFINE(O_RDWR);
	DEFINE(O_ACCMODE);
	DEFINE(O_NONBLOCK);
	DEFINE(O_APPEND);
	DEFINE(O_SHLOCK);
	DEFINE(O_EXLOCK);
	DEFINE(O_ASYNC);
	DEFINE(O_FSYNC);
	DEFINE(O_SYNC);
	DEFINE(O_NOFOLLOW);
	DEFINE(O_CREAT);
	DEFINE(O_TRUNC);
	DEFINE(O_EXCL);
	DEFINE(O_NOCTTY);
	DEFINE(O_DIRECT);
	DEFINE(O_DIRECTORY);
	DEFINE(O_EXEC);
	DEFINE(O_SEARCH);
	DEFINE(O_TTY_INIT);
	DEFINE(O_CLOEXEC);
	DEFINE(O_VERIFY);
	DEFINE(O_PATH);
	DEFINE(O_RESOLVE_BENEATH);
	DEFINE(O_DSYNC);
	DEFINE(O_EMPTY_PATH);
#ifdef O_NAMEDATTR
	DEFINE(O_NAMEDATTR);
#endif
#ifdef O_CLOFORK
	DEFINE(O_CLOFORK);
#endif

	DEFINE(AT_FDCWD);
	DEFINE(AT_EACCESS);
	DEFINE(AT_SYMLINK_NOFOLLOW);
	DEFINE(AT_SYMLINK_FOLLOW);
	DEFINE(AT_REMOVEDIR);
	DEFINE(AT_RESOLVE_BENEATH);
	DEFINE(AT_EMPTY_PATH);

	DEFINE(POSIX_FADV_NORMAL);
	DEFINE(POSIX_FADV_RANDOM);
	DEFINE(POSIX_FADV_SEQUENTIAL);
	DEFINE(POSIX_FADV_WILLNEED);
	DEFINE(POSIX_FADV_DONTNEED);
	DEFINE(POSIX_FADV_NOREUSE);

	DEFINE(FD_NONE);
#undef DEFINE
	return (1);
}
