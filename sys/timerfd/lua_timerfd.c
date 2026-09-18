/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/timerfd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/timespec/lua_timespec.h"
#include "utils.h"

#define TIMERFD_METATABLE "timer fd"

int luaopen_sys_timerfd(lua_State *);

enum {
	FILEDESC = 1,
};

static inline void
checktimerfdnil(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, TIMERFD_METATABLE);

	lua_getiuservalue(L, idx, FILEDESC);
}

static inline int
checktimerfd(lua_State *L, int idx)
{
	int fd;

	checktimerfdnil(L, idx);
	luaL_argcheck(L, lua_isinteger(L, -1), idx,
	    "invalid timer fd (closed)");

	fd = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return (fd);
}

static inline int
newtimerfd(lua_State *L, int fd)
{
	lua_newuserdatauv(L, 0, 1);
	lua_pushinteger(L, fd);
	lua_setiuservalue(L, -2, FILEDESC);
	luaL_setmetatable(L, TIMERFD_METATABLE);
	return (1);
}

static int
l_timerfd_create(lua_State *L)
{
	int clockid, flags, fd;

	clockid = luaL_checkinteger(L, 1);
	flags = luaL_optinteger(L, 2, 0);

	if ((fd = timerfd_create(clockid, flags)) == -1) {
		return (fail(L, errno));
	}
	return (newtimerfd(L, fd));
}

static int
l_timerfd_close(lua_State *L)
{
	int fd;

	checktimerfdnil(L, 1);

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
l_timerfd_fileno(lua_State *L)
{
	int fd;

	fd = checktimerfd(L, 1);

	lua_pushinteger(L, fd);
	return (1);
}

static int
l_timerfd_read(lua_State *L)
{
	uint64_t counter;
	ssize_t len;
	int fd;

	fd = checktimerfd(L, 1);

	if ((len = read(fd, &counter, sizeof(counter))) == -1) {
		return (fail(L, errno));
	}
	lua_assert(len == sizeof(counter));
	lua_pushinteger(L, counter);
	return (1);
}

static int
l_timerfd_ioctl(lua_State *L)
{
	int fd, cmd, val;

	fd = checktimerfd(L, 1);
	cmd = luaL_checkinteger(L, 2);
	val = luaL_checkinteger(L, 3);

	if (ioctl(fd, cmd, &val) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_timerfd_gettime(lua_State *L)
{
	struct itimerspec curr_value;
	int fd;

	fd = checktimerfd(L, 1);

	if (timerfd_gettime(fd, &curr_value) == -1) {
		return (fail(L, errno));
	}
	pushitimerspec(L, &curr_value);
	return (1);
}

static int
l_timerfd_settime(lua_State *L)
{
	struct itimerspec new_value, old_value;
	int fd, flags;

	fd = checktimerfd(L, 1);
	flags = luaL_checkinteger(L, 2);
	checkitimerspec(L, 3, &new_value);

	if (timerfd_settime(fd, flags, &new_value, &old_value) == -1) {
		return (fail(L, errno));
	}
	pushitimerspec(L, &old_value);
	return (1);
}

static int
l_timerfd_wrap(lua_State *L)
{
	int fd;

	fd = luaL_checkinteger(L, 1);

	return (newtimerfd(L, fd));
}

static const struct luaL_Reg l_timerfd_funcs[] = {
	{"create", l_timerfd_create},
	{"wrap", l_timerfd_wrap},
	{NULL, NULL}
};

static const struct luaL_Reg l_timerfd_meta[] = {
	{"__close", l_timerfd_close},
	{"__gc", l_timerfd_close},
	{"ioctl", l_timerfd_ioctl},
	{"gettime", l_timerfd_gettime},
	{"settime", l_timerfd_settime},
	{"close", l_timerfd_close},
	{"fileno", l_timerfd_fileno},
	{"read", l_timerfd_read},
	{NULL, NULL}
};

int
luaopen_sys_timerfd(lua_State *L)
{
	luaL_newmetatable(L, TIMERFD_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_timerfd_meta, 0);

	luaL_newlib(L, l_timerfd_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, TFD_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(NONBLOCK);
	DEFINE(CLOEXEC);
#undef DEFINE
	return (1);
}
