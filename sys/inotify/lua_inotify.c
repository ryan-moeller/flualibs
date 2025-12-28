/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/inotify.h>
#include <errno.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

#define INOTIFY_METATABLE "inotify fd"

int luaopen_sys_inotify(lua_State *);

enum {
	FILEDESC = 1,
};

static inline void
checkinotifynil(lua_State *L, int idx)
{
	int fd;

	luaL_checkudata(L, idx, INOTIFY_METATABLE);

	lua_getiuservalue(L, idx, FILEDESC);
}

static inline int
checkinotify(lua_State *L, int idx)
{
	int fd;

	checkinotifynil(L, idx);
	luaL_argcheck(L, lua_isinteger(L, -1), idx,
	    "invalid inotify fd (closed)");

	fd = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return (fd);
}

static int
l_inotify_init(lua_State *L)
{
	int flags, fd;

	flags = luaL_optinteger(L, 1, 0);

	if ((fd = inotify_init1(flags)) == -1) {
		return (fail(L, errno));
	}
	lua_newuserdatauv(L, 0, 1);
	lua_pushinteger(L, fd);
	lua_setiuservalue(L, -2, FILEDESC);
	luaL_setmetatable(L, INOTIFY_METATABLE);
	return (1);
}

static int
l_inotify_close(lua_State *L)
{
	int fd;

	checkinotifynil(L, 1);

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
l_inotify_fileno(lua_State *L)
{
	int fd;

	fd = checkinotify(L, 1);

	lua_pushinteger(L, fd);
	return (1);
}

static inline const struct inotify_event *
pushevent(lua_State *L, const struct inotify_event *ie, ssize_t *resid)
{
	size_t len = sizeof(*ie) + ie->len;

	lua_newtable(L);
	lua_pushinteger(L, ie->wd);
	lua_setfield(L, -2, "wd");
	lua_pushinteger(L, ie->mask);
	lua_setfield(L, -2, "mask");
	if (ie->len > 0) {
		lua_pushstring(L, ie->name);
		lua_setfield(L, -2, "name");
	}
	*resid -= len;
	return ((const struct inotify_event *)((uintptr_t)ie + len));
}

static int
l_inotify_read(lua_State *L)
{
	struct {
		struct inotify_event ie;
		char namebuf[NAME_MAX + 1];
	} event;
	const struct inotify_event *ie = &event.ie;
	ssize_t len;
	int fd;

	fd = checkinotify(L, 1);
	/* TODO: support specifying read buffer size? */

	if ((len = read(fd, &event, sizeof(event))) == -1) {
		return (fail(L, errno));
	}
	/* We may receive multiple events due to the excess size of namebuf. */
	lua_newtable(L);
	for (lua_Integer i = 1; len > 0; i++) {
		ie = pushevent(L, ie, &len);
		lua_rawseti(L, -2, i);
	}
	return (1);
}

static int
l_inotify_add_watch(lua_State *L)
{
	const char *pathname;
	uint32_t mask;
	int fd, wd;

	fd = checkinotify(L, 1);
	pathname = luaL_checkstring(L, 2);
	mask = luaL_checkinteger(L, 3);

	if ((wd = inotify_add_watch(fd, pathname, mask)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, wd);
	return (1);
}

static int
l_inotify_add_watch_at(lua_State *L)
{
	const char *pathname;
	uint32_t mask;
	int fd, dfd, wd;

	fd = checkinotify(L, 1);
	dfd = luaL_checkinteger(L, 2);
	pathname = luaL_checkstring(L, 3);
	mask = luaL_checkinteger(L, 4);

	if ((wd = inotify_add_watch_at(fd, dfd, pathname, mask)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, wd);
	return (1);
}

static int
l_inotify_rm_watch(lua_State *L)
{
	uint32_t wd;
	int fd;

	fd = checkinotify(L, 1);
	wd = luaL_checkinteger(L, 2);

	if (inotify_rm_watch(fd, wd) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static const struct luaL_Reg l_inotify_funcs[] = {
	{"init", l_inotify_init},
	{NULL, NULL}
};

static const struct luaL_Reg l_inotify_meta[] = {
	{"__close", l_inotify_close},
	{"__gc", l_inotify_close},
	{"add_watch", l_inotify_add_watch},
	{"add_watch_at", l_inotify_add_watch_at},
	{"rm_watch", l_inotify_rm_watch},
	{"close", l_inotify_close},
	{"fileno", l_inotify_fileno},
	{"read", l_inotify_read},
	{NULL, NULL}
};

int
luaopen_sys_inotify(lua_State *L)
{
	luaL_newmetatable(L, INOTIFY_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_inotify_meta, 0);

	luaL_newlib(L, l_inotify_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, IN_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(NONBLOCK);
	DEFINE(CLOEXEC);
	DEFINE(ACCESS);
	DEFINE(MODIFY);
	DEFINE(ATTRIB);
	DEFINE(CLOSE_WRITE);
	DEFINE(CLOSE_NOWRITE);
	DEFINE(CLOSE);
	DEFINE(OPEN);
	DEFINE(MOVED_FROM);
	DEFINE(MOVED_TO);
	DEFINE(MOVE);
	DEFINE(CREATE);
	DEFINE(DELETE);
	DEFINE(DELETE_SELF);
	DEFINE(MOVE_SELF);
	DEFINE(ALL_EVENTS);
	DEFINE(ONLYDIR);
	DEFINE(DONT_FOLLOW);
	DEFINE(EXCL_UNLINK);
	DEFINE(MASK_CREATE);
	DEFINE(MASK_ADD);
	DEFINE(ONESHOT);
	DEFINE(UNMOUNT);
	DEFINE(Q_OVERFLOW);
	DEFINE(IGNORED);
	DEFINE(ISDIR);
#undef DEFINE
	return (1);
}
