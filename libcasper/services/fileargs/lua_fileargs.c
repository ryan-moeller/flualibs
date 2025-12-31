/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifer: BSD-2-Clause
 */

#include <libcasper.h>
#include <casper/cap_fileargs.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/capsicum/lua_capsicum.h"
#include "sys/stat/lua_stat.h"
#include "libcasper/libcasper/lua_casper.h"
#include "libnv/lua_nv.h"
#include "utils.h"

#define FILEARGS_METATABLE "fileargs_t *"

int luaopen_casper_fileargs(lua_State *);

static int
l_fileargs_init(lua_State *L)
{
	fileargs_t *fa;
	cap_rights_t *rights;
	const char **argv;
	int argc, flags, operations;
	mode_t mode;

	luaL_checktype(L, 1, LUA_TTABLE);
	flags = luaL_checkinteger(L, 2);
	mode = luaL_checkinteger(L, 3);
	rights = luaL_checkudata(L, 4, CAP_RIGHTS_METATABLE);
	operations = luaL_checkinteger(L, 5);

	argc = luaL_len(L, 1);
	if ((argv = malloc(argc * sizeof(*argv))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < argc; i++) {
		lua_geti(L, 1, i + 1);
		argv[i] = lua_tostring(L, -1);
	}
	if ((fa = fileargs_init(argc, __DECONST(char **, argv), flags, mode,
	    rights, operations)) == NULL) {
		int error = errno;

		free(argv);
		return (fail(L, error));
	}
	free(argv);
	return (new(L, fa, FILEARGS_METATABLE));
}

static int
l_fileargs_cinit(lua_State *L)
{
	fileargs_t *fa;
	cap_channel_t *cas;
	cap_rights_t *rights;
	const char **argv;
	int argc, flags, operations;
	mode_t mode;

	cas = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);
	flags = luaL_checkinteger(L, 3);
	mode = luaL_checkinteger(L, 4);
	rights = luaL_checkudata(L, 5, CAP_RIGHTS_METATABLE);
	operations = luaL_checkinteger(L, 6);

	argc = luaL_len(L, 2);
	if ((argv = malloc(argc * sizeof(*argv))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < argc; i++) {
		lua_geti(L, 2, i + 1);
		argv[i] = lua_tostring(L, -1);
	}
	if ((fa = fileargs_cinit(cas, argc, __DECONST(char **, argv), flags,
	    mode, rights, operations)) == NULL) {
		int error = errno;

		free(argv);
		return (fail(L, error));
	}
	free(argv);
	return (new(L, fa, FILEARGS_METATABLE));
}

static int
l_fileargs_initnv(lua_State *L)
{
	fileargs_t *fa;
	nvlist_t *limits;

	if (lua_isnoneornil(L, 1)) {
		limits = NULL;
	} else {
		limits = checknvlist(L, 1);

		setcookie(L, 1, NULL);
	}

	if ((fa = fileargs_initnv(limits)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, fa, FILEARGS_METATABLE));
}

static int
l_fileargs_cinitnv(lua_State *L)
{
	fileargs_t *fa;
	cap_channel_t *cas;
	nvlist_t *limits;

	cas = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	if (lua_isnoneornil(L, 2)) {
		limits = NULL;
	} else {
		limits = checknvlist(L, 2);

		setcookie(L, 2, NULL);
	}

	if ((fa = fileargs_cinitnv(cas, limits)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, fa, FILEARGS_METATABLE));
}

static int
l_fileargs_wrap(lua_State *L)
{
	fileargs_t *fa;
	cap_channel_t *chan;
	int fdflags;

	chan = checkcookie(L, 1, CAP_CHANNEL_METATABLE);
	fdflags = luaL_checkinteger(L, 1);

	if ((fa = fileargs_wrap(chan, fdflags)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, fa, FILEARGS_METATABLE));
}

static int
l_fileargs_free(lua_State *L)
{
	fileargs_t *fa;

	fa = checkcookienull(L, 1, FILEARGS_METATABLE);

	fileargs_free(fa);
	return (0);
}

static int
l_fileargs_lstat(lua_State *L)
{
	struct stat sb;
	fileargs_t *fa;
	const char *name;

	fa = checkcookie(L, 1, FILEARGS_METATABLE);
	name = luaL_checkstring(L, 2);

	if (fileargs_lstat(fa, name, &sb) == -1) {
		return (fail(L, errno));
	}
	pushstat(L, &sb);
	return (1);
}

static int
l_fileargs_open(lua_State *L)
{
	fileargs_t *fa;
	const char *name;
	int fd;

	fa = checkcookie(L, 1, FILEARGS_METATABLE);
	name = luaL_checkstring(L, 2);

	if ((fd = fileargs_open(fa, name)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fd);
	return (1);
}

static int
closestream(lua_State *L)
{
	luaL_Stream *s;

	s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	return (luaL_fileresult(L, fclose(s->f) == 0, NULL));
}

static int
l_fileargs_fopen(lua_State *L)
{
	fileargs_t *fa;
	const char *name, *mode;
	luaL_Stream *s;

	fa = checkcookie(L, 1, FILEARGS_METATABLE);
	name = luaL_checkstring(L, 2);
	mode = luaL_checkstring(L, 3);

	s = lua_newuserdatauv(L, sizeof(*s), 0);
	if ((s->f = fileargs_fopen(fa, name, mode)) == NULL) {
		return (fail(L, errno));
	}
	s->closef = closestream;
	luaL_setmetatable(L, LUA_FILEHANDLE);
	return (1);
}

static int
l_fileargs_realpath(lua_State *L)
{
	char path[MAXPATHLEN];
	fileargs_t *fa;
	const char *pathname;

	fa = checkcookie(L, 1, FILEARGS_METATABLE);
	pathname = luaL_checkstring(L, 2);

	if (fileargs_realpath(fa, pathname, path) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, path);
	return (1);
}

static int
l_fileargs_unwrap(lua_State *L)
{
	fileargs_t *fa;
	cap_channel_t *chan;
	int fdflags;

	fa = checkcookie(L, 1, FILEARGS_METATABLE);

	setcookie(L, 1, NULL);
	chan = fileargs_unwrap(fa, &fdflags);
	new(L, chan, CAP_CHANNEL_METATABLE);
	lua_pushinteger(L, fdflags);
	return (2);
}

static const struct luaL_Reg l_fileargs_funcs[] = {
	{"init", l_fileargs_init},
	{"cinit", l_fileargs_cinit},
	{"initnv", l_fileargs_initnv},
	{"cinitnv", l_fileargs_cinitnv},
	{"wrap", l_fileargs_wrap},
	{NULL, NULL}
};

static const struct luaL_Reg l_fileargs_meta[] = {
	{"__gc", l_fileargs_free},
	{"lstat", l_fileargs_lstat},
	{"open", l_fileargs_open},
	{"fopen", l_fileargs_fopen},
	{"realpath", l_fileargs_realpath},
	{"unwrap", l_fileargs_unwrap},
	{NULL, NULL}
};

int
luaopen_casper_fileargs(lua_State *L)
{
	luaL_newmetatable(L, FILEARGS_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_fileargs_meta, 0);

	luaL_newlib(L, l_fileargs_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, FA_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(OPEN);
	DEFINE(LSTAT);
	DEFINE(REALPATH);
#undef DEFINE
	return (1);
}
