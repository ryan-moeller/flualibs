/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* XXX: lfs doesn't expose st_flags */

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

int luaopen_chflags(lua_State *);

static int
l_chflags(lua_State *L)
{
	const char *path;
	unsigned long flags;

	path = luaL_checkstring(L, 1);
	flags = luaL_checkinteger(L, 2);

	if (chflags(path, flags) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_lchflags(lua_State *L)
{
	const char *path;
	unsigned long flags;

	path = luaL_checkstring(L, 1);
	flags = luaL_checkinteger(L, 2);

	if (lchflags(path, flags) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_fchflags(lua_State *L)
{
	unsigned long flags;
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
	flags = luaL_checkinteger(L, 2);

	if (fchflags(fd, flags) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_chflagsat(lua_State *L)
{
	const char *path;
	unsigned long flags;
	int fd, atflag;

	fd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	flags = luaL_checkinteger(L, 3);
	atflag = luaL_checkinteger(L, 4);

	if (chflagsat(fd, path, flags, atflag) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_fflagstostr(lua_State *L)
{
	char *str;
	u_long flags;

	flags = luaL_checkinteger(L, 1);

	if ((str = fflagstostr(flags)) == NULL) {
		return (fail(L, ENOMEM));
	}
	lua_pushstring(L, str);
	free(str);
	return (1);
}

static int
l_strtofflags(lua_State *L)
{
	const char *s;
	char *str;
	u_long set, clr;

	s = luaL_checkstring(L, 1);

	if ((str = strdup(s)) == NULL) {
		return (fatal(L, "strdup", ENOMEM));
	}
	if (strtofflags(&str, &set, &clr) != 0) {
		luaL_pushfail(L);
		lua_pushstring(L, str);
		free(str);
		return (2);
	}
	free(str);
	lua_pushinteger(L, set);
	lua_pushinteger(L, clr);
	return (2);
}

static const struct luaL_Reg l_chflags_funcs[] = {
	{"chflags", l_chflags},
	{"lchflags", l_lchflags},
	{"fchflags", l_fchflags},
	{"chflagsat", l_chflagsat},
	{"fflagstostr", l_fflagstostr},
	{"strtofflags", l_strtofflags},
	{NULL, NULL}
};

int
luaopen_chflags(lua_State *L)
{
	luaL_newlib(L, l_chflags_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(AT_SYMLINK_FOLLOW);
	SETCONST(AT_RESOLVE_BENEATH);
	SETCONST(AT_EMPTY_PATH);
	SETCONST(AT_FDCWD);
	SETCONST(UF_SETTABLE);
	SETCONST(UF_NODUMP);
	SETCONST(UF_IMMUTABLE);
	SETCONST(UF_APPEND);
	SETCONST(UF_OPAQUE);
	SETCONST(UF_NOUNLINK);
	SETCONST(UF_SYSTEM);
	SETCONST(UF_SPARSE);
	SETCONST(UF_OFFLINE);
	SETCONST(UF_REPARSE);
	SETCONST(UF_ARCHIVE);
	SETCONST(UF_READONLY);
	SETCONST(UF_HIDDEN);
	SETCONST(SF_SETTABLE);
	SETCONST(SF_ARCHIVED);
	SETCONST(SF_IMMUTABLE);
	SETCONST(SF_APPEND);
	SETCONST(SF_NOUNLINK);
	SETCONST(SF_SNAPSHOT);
#undef SETCONST
	return (1);
}
