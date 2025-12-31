/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/eventfd.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define EVENTFD_METATABLE "eventfd"

int luaopen_sys_eventfd(lua_State *);

enum {
	FILEDESC = 1,
};

static inline void
checkeventfdnil(lua_State *L, int idx)
{
	int fd;

	luaL_checkudata(L, idx, EVENTFD_METATABLE);

	lua_getiuservalue(L, idx, FILEDESC);
}

static inline int
checkeventfd(lua_State *L, int idx)
{
	int fd;

	checkeventfdnil(L, idx);
	luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid eventfd (closed)");

	fd = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return (fd);
}

static int
l_eventfd(lua_State *L)
{
	unsigned int initval;
	int flags, fd;

	initval = luaL_checkinteger(L, 1);
	flags = luaL_optinteger(L, 2, 0);

	if ((fd = eventfd(initval, flags)) == -1) {
		return (fail(L, errno));
	}
	lua_newuserdatauv(L, 0, 1);
	lua_pushinteger(L, fd);
	lua_setiuservalue(L, -2, FILEDESC);
	luaL_setmetatable(L, EVENTFD_METATABLE);
	return (1);
}

static int
l_eventfd_close(lua_State *L)
{
	int fd;

	checkeventfdnil(L, 1);

	if (lua_isnil(L, -1)) {
		return (success(L));
	}
	fd = lua_tointeger(L, -1);
	if (close(fd) == -1) {
		return (fail(L, errno));
	}
	lua_pushnil(L);
	lua_setiuservalue(L, 1, FILEDESC);
	return (success(L));
}

static int
l_eventfd_fileno(lua_State *L)
{
	int fd;

	fd = checkeventfd(L, 1);

	lua_pushinteger(L, fd);
	return (1);
}

static int
l_eventfd_read(lua_State *L)
{
	eventfd_t value;
	int fd;

	fd = checkeventfd(L, 1);

	if (eventfd_read(fd, &value) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_eventfd_write(lua_State *L)
{
	eventfd_t value;
	int fd;

	fd = checkeventfd(L, 1);
	value = luaL_checkinteger(L, 2);

	if (eventfd_write(fd, value) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static const struct luaL_Reg l_eventfd_funcs[] = {
	{"eventfd", l_eventfd},
	{NULL, NULL}
};

static const struct luaL_Reg l_eventfd_meta[] = {
	{"__close", l_eventfd_close},
	{"__gc", l_eventfd_close},
	{"close", l_eventfd_close},
	{"fileno", l_eventfd_fileno},
	{"read", l_eventfd_read},
	{"write", l_eventfd_write},
	{NULL, NULL}
};

int
luaopen_sys_eventfd(lua_State *L)
{
	luaL_newmetatable(L, EVENTFD_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_eventfd_meta, 0);

	luaL_newlib(L, l_eventfd_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, EFD_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(CLOEXEC);
	DEFINE(NONBLOCK);
	DEFINE(SEMAPHORE);
#undef DEFINE
	return (1);
}
