/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <magic.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"
#include "utils.h"

#define MAGIC_METATABLE "magic_t"

int luaopen_magic(lua_State *);

static int
l_magic_open(lua_State *L)
{
	magic_t *cookiep;
	int flags;

	flags = luaL_optinteger(L, 1, MAGIC_NONE);

	cookiep = lua_newuserdatauv(L, sizeof(*cookiep), 0);
	luaL_setmetatable(L, MAGIC_METATABLE);

	*cookiep = magic_open(flags);
	if (*cookiep == NULL) {
		return (fail(L, errno));
	}
	return (1);
}

static int
l_magic_close(lua_State *L)
{
	magic_t *cookiep;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);

	magic_close(*cookiep);
	*cookiep = NULL;
	return (0);
}

static int
magicerr(lua_State *L, magic_t cookie)
{
	luaL_pushfail(L);
	lua_pushstring(L, magic_error(cookie));
	lua_pushinteger(L, magic_errno(cookie));
	return (3);
}

static int
l_magic_descriptor(lua_State *L)
{
	magic_t *cookiep;
	luaL_Stream *stream;
	const char *magic;
	int fd;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		fd = lua_tointeger(L, 2);
	} else {
		stream = luaL_checkudata(L, 2, LUA_FILEHANDLE);
		luaL_argcheck(L, stream->f != NULL, 2,
		    "invalid file handle (closed)");
		fd = fileno(stream->f);
	}
	luaL_argcheck(L, fd >= 0, 2, "invalid file descriptor");

	magic = magic_descriptor(*cookiep, fd);
	if (magic == NULL) {
		return (magicerr(L, *cookiep));
	}
	lua_pushstring(L, magic);
	return (1);
}

static int
l_magic_file(lua_State *L)
{
	magic_t *cookiep;
	const char *filename, *magic;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	filename = luaL_optstring(L, 2, NULL);

	magic = magic_file(*cookiep, filename);
	if (magic == NULL) {
		return (magicerr(L, *cookiep));
	}
	lua_pushstring(L, magic);
	return (1);
}

static int
l_magic_buffer(lua_State *L)
{
	magic_t *cookiep;
	const char *buffer, *magic;
	size_t length;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	buffer = luaL_checklstring(L, 2, &length);

	magic = magic_buffer(*cookiep, buffer, length);
	if (magic == NULL) {
		return (magicerr(L, *cookiep));
	}
	lua_pushstring(L, magic);
	return (1);
}

static int
l_magic_getflags(lua_State *L)
{
	magic_t *cookiep;
	int flags;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);

	flags = magic_getflags(*cookiep);
	if (flags == -1) {
		return (luaL_error(L, "magic_getflags failed"));
	}
	lua_pushinteger(L, flags);
	return (1);
}

static int
l_magic_setflags(lua_State *L)
{
	magic_t *cookiep;
	int flags;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	flags = luaL_checkinteger(L, 2);

	if (magic_setflags(*cookiep, flags) == -1) {
		return (luaL_error(L, "magic_setflags failed"));
	}
	return (0);
}

static int
l_magic_check(lua_State *L)
{
	magic_t *cookiep;
	const char *filename;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	filename = luaL_optstring(L, 2, NULL);

	if (magic_check(*cookiep, filename) == -1) {
		return (magicerr(L, *cookiep));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_magic_compile(lua_State *L)
{
	magic_t *cookiep;
	const char *filename;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	filename = luaL_optstring(L, 2, NULL);

	if (magic_compile(*cookiep, filename) == -1) {
		return (magicerr(L, *cookiep));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_magic_list(lua_State *L)
{
	magic_t *cookiep;
	const char *filename;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	filename = luaL_optstring(L, 2, NULL);

	if (magic_list(*cookiep, filename) == -1) {
		return (magicerr(L, *cookiep));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_magic_load(lua_State *L)
{
	magic_t *cookiep;
	const char *filename;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	filename = luaL_optstring(L, 2, NULL);

	if (magic_load(*cookiep, filename) == -1) {
		return (magicerr(L, *cookiep));
	}
	lua_pushboolean(L, true);
	return (1);
}

/* magic_load_buffers seems unlikely to be useful in Lua */

static int
l_magic_getparam(lua_State *L)
{
	magic_t *cookiep;
	size_t limit;
	int param;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	param = luaL_checkinteger(L, 2);

	if (magic_getparam(*cookiep, param, &limit) == -1) {
		return (luaL_error(L, "magic_getparam failed"));
	}
	lua_pushinteger(L, limit);
	return (1);
}

static int
l_magic_setparam(lua_State *L)
{
	magic_t *cookiep;
	size_t limit;
	int param;

	cookiep = luaL_checkudata(L, 1, MAGIC_METATABLE);
	param = luaL_checkinteger(L, 2);
	limit = luaL_checkinteger(L, 3);

	if (magic_setparam(*cookiep, param, &limit) == -1) {
		return (luaL_error(L, "magic_setparam failed"));
	}
	return (0);
}

static int
l_magic_getpath(lua_State *L)
{
	const char *magicfile, *path;
	int action;

	magicfile = luaL_optstring(L, 1, NULL);
	action = luaL_optinteger(L, 2, 0);

	path = magic_getpath(magicfile, action);
	lua_pushstring(L, path);
	return (1);
}

static const struct luaL_Reg l_magic_funcs[] = {
	{"open", l_magic_open},
	{"getpath", l_magic_getpath},
	{NULL, NULL}
};

static const struct luaL_Reg l_magic_meta[] = {
	{"__close", l_magic_close},
	{"__gc", l_magic_close},
	{"close", l_magic_close},
	{"descriptor", l_magic_descriptor},
	{"file", l_magic_file},
	{"buffer", l_magic_buffer},
	{"getflags", l_magic_getflags},
	{"setflags", l_magic_setflags},
	{"check", l_magic_check},
	{"compile", l_magic_compile},
	{"list", l_magic_list},
	{"load", l_magic_load},
	{"getparam", l_magic_getparam},
	{"setparam", l_magic_setparam},
	{NULL, NULL}
};

int
luaopen_magic(lua_State *L)
{
	luaL_newmetatable(L, MAGIC_METATABLE);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_setfuncs(L, l_magic_meta, 0);

	luaL_newlib(L, l_magic_funcs);

#define SETCONST(ident) ({ \
	lua_pushinteger(L, MAGIC_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(NONE);
	SETCONST(DEBUG);
	SETCONST(SYMLINK);
	SETCONST(COMPRESS);
	SETCONST(DEVICES);
	SETCONST(MIME_TYPE);
	SETCONST(CONTINUE);
	SETCONST(CHECK);
	SETCONST(PRESERVE_ATIME);
	SETCONST(RAW);
	SETCONST(ERROR);
	SETCONST(MIME_ENCODING);
	SETCONST(MIME);
	SETCONST(APPLE);
	SETCONST(EXTENSION);
	SETCONST(COMPRESS_TRANSP);
	SETCONST(NO_COMPRESS_FORK);
	SETCONST(NODESC);

	SETCONST(NO_CHECK_COMPRESS);
	SETCONST(NO_CHECK_TAR);
	SETCONST(NO_CHECK_SOFT);
	SETCONST(NO_CHECK_APPTYPE);
	SETCONST(NO_CHECK_ELF);
	SETCONST(NO_CHECK_TEXT);
	SETCONST(NO_CHECK_CDF);
	SETCONST(NO_CHECK_CSV);
	SETCONST(NO_CHECK_TOKENS);
	SETCONST(NO_CHECK_ENCODING);
	SETCONST(NO_CHECK_JSON);
	SETCONST(NO_CHECK_SIMH);
	SETCONST(NO_CHECK_BUILTIN);

	SETCONST(VERSION);

	SETCONST(PARAM_INDIR_MAX);
	SETCONST(PARAM_NAME_MAX);
	SETCONST(PARAM_ELF_PHNUM_MAX);
	SETCONST(PARAM_ELF_SHNUM_MAX);
	SETCONST(PARAM_ELF_NOTES_MAX);
	SETCONST(PARAM_REGEX_MAX);
	SETCONST(PARAM_BYTES_MAX);
	SETCONST(PARAM_ENCODING_MAX);
	SETCONST(PARAM_ELF_SHSIZE_MAX);
	SETCONST(PARAM_MAGWARN_MAX);
#undef SETCONST
	return (1);
}
