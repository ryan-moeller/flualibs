/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/procdesc.h>
#include <errno.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define PROCDESC_METATABLE "procdesc"

int luaopen_sys_procdesc(lua_State *);

enum {
	FILEDESC = 1,
};

static inline void
checkprocdescnil(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PROCDESC_METATABLE);

	lua_getiuservalue(L, idx, FILEDESC);
}

static inline int
checkprocdesc(lua_State *L, int idx)
{
	int fd;

	checkprocdescnil(L, idx);
	luaL_argcheck(L, lua_isinteger(L, -1), idx, "invalid procdesc (closed)");

	fd = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return (fd);
}

static inline int
checkprocdescfd(lua_State *L, int idx)
{
	if (lua_isinteger(L, idx)) {
		return (lua_tointeger(L, idx));
	}
	return (checkprocdesc(L, idx));
}

static inline int
newprocdesc(lua_State *L, int fd)
{
	lua_newuserdatauv(L, 0, 1);
	lua_pushinteger(L, fd);
	lua_setiuservalue(L, -2, FILEDESC);
	luaL_setmetatable(L, PROCDESC_METATABLE);
	return (1);
}

static int
l_pdfork(lua_State *L)
{
	pid_t pid;
	int flags, fd;

	flags = luaL_optinteger(L, 1, 0);

	if ((pid = pdfork(&fd, flags)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, pid);
	newprocdesc(L, fd);
	return (2);
}

static int
l_pdgetpid(lua_State *L)
{
	pid_t pid;
	int fd;

	fd = checkprocdescfd(L, 1);

	if (pdgetpid(fd, &pid) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, pid);
	return (1);
}

static int
l_pdkill(lua_State *L)
{
	int fd, signum;

	fd = checkprocdescfd(L, 1);
	signum = luaL_checkinteger(L, 2);

	if (pdkill(fd, signum) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_procdesc_wrap(lua_State *L)
{
	int fd;

	fd = luaL_checkinteger(L, 1);

	return (newprocdesc(L, fd));
}

static int
l_procdesc_close(lua_State *L)
{
	int fd;

	checkprocdescnil(L, 1);

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
l_procdesc_fileno(lua_State *L)
{
	int fd;

	fd = checkprocdesc(L, 1);

	lua_pushinteger(L, fd);
	return (1);
}

static const struct luaL_Reg l_procdesc_funcs[] = {
	{"pdfork", l_pdfork},
	{"pdgetpid", l_pdgetpid},
	{"pdkill", l_pdkill},
	{"wrap", l_procdesc_wrap},
	{NULL, NULL}
};

static const struct luaL_Reg l_procdesc_meta[] = {
	{"__close", l_procdesc_close},
	{"__gc", l_procdesc_close},
	{"close", l_procdesc_close},
	{"fileno", l_procdesc_fileno},
	{"getpid", l_pdgetpid},
	{"kill", l_pdkill},
	{NULL, NULL}
};

int
luaopen_sys_procdesc(lua_State *L)
{
	luaL_newmetatable(L, PROCDESC_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_procdesc_meta, 0);

	luaL_newlib(L, l_procdesc_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, PD_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(DAEMON);
	DEFINE(CLOEXEC);
	DEFINE(ALLOWED_AT_FORK);
#undef DEFINE
	return (1);
}
