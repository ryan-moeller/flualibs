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

int luaopen_freebsd_fcntl(lua_State *);

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
luaopen_freebsd_fcntl(lua_State *L)
{
	luaL_newlib(L, l_fcntl_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(O_RDONLY);
	SETCONST(O_WRONLY);
	SETCONST(O_RDWR);
	SETCONST(O_ACCMODE);
	SETCONST(O_NONBLOCK);
	SETCONST(O_APPEND);
	SETCONST(O_SHLOCK);
	SETCONST(O_EXLOCK);
	SETCONST(O_ASYNC);
	SETCONST(O_FSYNC);
	SETCONST(O_SYNC);
	SETCONST(O_NOFOLLOW);
	SETCONST(O_CREAT);
	SETCONST(O_TRUNC);
	SETCONST(O_EXCL);
	SETCONST(O_NOCTTY);
	SETCONST(O_DIRECT);
	SETCONST(O_DIRECTORY);
	SETCONST(O_EXEC);
	SETCONST(O_SEARCH);
	SETCONST(O_TTY_INIT);
	SETCONST(O_CLOEXEC);
	SETCONST(O_VERIFY);
	SETCONST(O_PATH);
	SETCONST(O_RESOLVE_BENEATH);
	SETCONST(O_DSYNC);
	SETCONST(O_EMPTY_PATH);
#ifdef O_NAMEDATTR
	SETCONST(O_NAMEDATTR);
#endif
#ifdef O_CLOFORK
	SETCONST(O_CLOFORK);
#endif

	SETCONST(AT_FDCWD);
	SETCONST(AT_EACCESS);
	SETCONST(AT_SYMLINK_NOFOLLOW);
	SETCONST(AT_SYMLINK_FOLLOW);
	SETCONST(AT_REMOVEDIR);
	SETCONST(AT_RESOLVE_BENEATH);
	SETCONST(AT_EMPTY_PATH);

	SETCONST(POSIX_FADV_NORMAL);
	SETCONST(POSIX_FADV_RANDOM);
	SETCONST(POSIX_FADV_SEQUENTIAL);
	SETCONST(POSIX_FADV_WILLNEED);
	SETCONST(POSIX_FADV_DONTNEED);
	SETCONST(POSIX_FADV_NOREUSE);

	SETCONST(FD_NONE);
#undef SETCONST
	return (1);
}
