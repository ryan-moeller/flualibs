/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <dirent.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

#define DIR_METATABLE "DIR"

int luaopen_dirent(lua_State *);

static int
l_opendir(lua_State *L)
{
	const char *path;
	DIR *dirp;

	path = luaL_checkstring(L, 1);

	if ((dirp = opendir(path)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, dirp, DIR_METATABLE));
}

static int
l_fdopendir(lua_State *L)
{
	DIR *dirp;
	int fd;

	fd = luaL_checkinteger(L, 1);

	if ((dirp = fdopendir(fd)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, dirp, DIR_METATABLE));
}

static int
l_closedir(lua_State *L)
{
	DIR *dirp;

	dirp = checkcookie(L, 1, DIR_METATABLE);

	if (closedir(dirp) == -1) {
		return (fatal(L, "closedir", errno));
	}
	setcookie(L, 1, NULL);
	return (0);
}

static int
l_dir_gc(lua_State *L)
{
	DIR *dirp;

	dirp = checkcookienull(L, 1, DIR_METATABLE);

	if (dirp != NULL && closedir(dirp) == -1) {
		return (fatal(L, "closedir", errno));
	}
	return (0);
}

static int
l_readdir(lua_State *L)
{
	DIR *dirp;
	struct dirent *ent;

	dirp = checkcookie(L, 1, DIR_METATABLE);

	errno = 0;
	if ((ent = readdir(dirp)) == NULL) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_createtable(L, 0, 3);
	lua_pushinteger(L, ent->d_fileno);
	lua_setfield(L, -2, "fileno");
	/* d_off and d_reclen aren't useful here */
	lua_pushinteger(L, ent->d_type);
	lua_setfield(L, -2, "type");
	lua_pushlstring(L, ent->d_name, ent->d_namlen);
	lua_setfield(L, -2, "name");
	return (1);
}

static int
l_telldir(lua_State *L)
{
	DIR *dirp;
	long loc;

	dirp = checkcookie(L, 1, DIR_METATABLE);

	if ((loc = telldir(dirp)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, loc);
	return (1);
}

static int
l_seekdir(lua_State *L)
{
	DIR *dirp;
	long loc;

	dirp = checkcookie(L, 1, DIR_METATABLE);
	loc = luaL_checkinteger(L, 2);

	seekdir(dirp, loc);
	return (0);
}

static int
l_rewinddir(lua_State *L)
{
	DIR *dirp;

	dirp = checkcookie(L, 1, DIR_METATABLE);

	rewinddir(dirp);
	return (0);
}

static int
l_dirfd(lua_State *L)
{
	DIR *dirp;
	int fd;

	dirp = checkcookie(L, 1, DIR_METATABLE);

	lua_pushinteger(L, dirfd(dirp));
	return (1);
}

static int
l_iftodt(lua_State *L)
{
	lua_pushinteger(L, IFTODT(luaL_checkinteger(L, 1)));
	return (1);
}

static int
l_dttoif(lua_State *L)
{
	lua_pushinteger(L, DTTOIF(luaL_checkinteger(L, 1)));
	return (1);
}

static const struct luaL_Reg l_dirent_funcs[] = {
	{"opendir", l_opendir},
	{"fdopendir", l_fdopendir},
	/* scandir family seems a bit too C-specific */
	{"iftodt", l_iftodt},
	{"dttoif", l_dttoif},
	{NULL, NULL}
};

static const struct luaL_Reg l_dir_meta[] = {
	{"__close", l_closedir},
	{"__gc", l_dir_gc},
	{"read", l_readdir},
	{"tell", l_telldir},
	{"seek", l_seekdir},
	{"rewind", l_rewinddir},
	{"fd", l_dirfd},
	{NULL, NULL}
};

int
luaopen_dirent(lua_State *L)
{
	luaL_newmetatable(L, DIR_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_dir_meta, 0);

	luaL_newlib(L, l_dirent_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(DT_UNKNOWN);
	SETCONST(DT_FIFO);
	SETCONST(DT_CHR);
	SETCONST(DT_DIR);
	SETCONST(DT_BLK);
	SETCONST(DT_REG);
	SETCONST(DT_LNK);
	SETCONST(DT_SOCK);
	SETCONST(DT_WHT);
#undef SETCONST
	return (1);
}
