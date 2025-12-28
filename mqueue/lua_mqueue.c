/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

#define MQD_METATABLE "mqd_t"

int luaopen_mqueue(lua_State *);

static int
l_mq_open(lua_State *L)
{
	const char *name;
	int oflag;
	mqd_t mq, *mqp;

	mqp = lua_newuserdatauv(L, sizeof(*mqp), 0);
	luaL_setmetatable(L, MQD_METATABLE);
	*mqp = (mqd_t)-1;

	name = luaL_checkstring(L, 1);
	oflag = luaL_checkinteger(L, 2);
	if ((oflag & O_CREAT) != 0) {
		mode_t mode;

		mode = luaL_checkinteger(L, 3);

		if (lua_gettop(L) == 5) {
			struct mq_attr attr;

			lua_pushstring(L, "maxmsg");
			lua_gettable(L, 4);
			attr.mq_maxmsg = (long)lua_tonumber(L, -1);
			lua_pop(L, -1);

			lua_pushstring(L, "msgsize");
			lua_gettable(L, 4);
			attr.mq_msgsize = (long)lua_tonumber(L, -1);
			lua_pop(L, -1);

			mq = mq_open(name, oflag, mode, &attr);
		} else {
			mq = mq_open(name, oflag, mode, NULL);
		}
	} else {
		mq = mq_open(name, oflag);
	}
	if (mq == (mqd_t)-1) {
		return (fail(L, errno));
	}
	*mqp = mq;
	return (1);
}

static int
l_mq_close(lua_State *L)
{
	mqd_t mq, *mqp;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	mq = *mqp;
	*mqp = (mqd_t)-1;
	if (mq_close(mq) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mq_unlink(lua_State *L)
{
	const char *name;

	name = luaL_checkstring(L, 1);

	if (mq_unlink(name) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mq_getattr(lua_State *L)
{
	mqd_t *mqp;
	struct mq_attr attr;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);

	if (mq_getattr(*mqp, &attr) == -1) {
		return (fail(L, errno));
	}

	lua_newtable(L);
	lua_pushinteger(L, attr.mq_flags);
	lua_setfield(L, -2, "flags");
	lua_pushinteger(L, attr.mq_maxmsg);
	lua_setfield(L, -2, "maxmsg");
	lua_pushinteger(L, attr.mq_msgsize);
	lua_setfield(L, -2, "msgsize");
	lua_pushinteger(L, attr.mq_curmsgs);
	lua_setfield(L, -2, "curmsgs");
	return (1);
}

static int
l_mq_setattr(lua_State *L)
{
	mqd_t *mqp;
	struct mq_attr attr;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	attr.mq_flags = luaL_checkinteger(L, 2);

	if (mq_setattr(*mqp, &attr, NULL) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mq_send(lua_State *L)
{
	mqd_t *mqp;
	const char *msg;
	size_t len;
	unsigned prio;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	msg = luaL_checklstring(L, 2, &len);
	prio = luaL_checkinteger(L, 3);

	if (mq_send(*mqp, msg, len, prio) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mq_receive(lua_State *L)
{
	mqd_t *mqp;
	char *buf;
	size_t buflen;
	ssize_t len;
	unsigned prio;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	buflen = luaL_checkinteger(L, 2);
	buf = malloc(buflen);
	if (buf == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	prio = 0;

	len = mq_receive(*mqp, buf, buflen, &prio);
	if (len == -1) {
		int error = errno;

		free(buf);
		return (fail(L, error));
	}
	lua_pushlstring(L, buf, len);
	lua_pushinteger(L, prio);
	return (2);
}

static int
l_mq_timedsend(lua_State *L)
{
	struct timespec abs_timeout;
	mqd_t *mqp;
	const char *msg;
	size_t len;
	unsigned prio;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	msg = luaL_checklstring(L, 2, &len);
	prio = luaL_checkinteger(L, 3);
	abs_timeout.tv_sec = luaL_checkinteger(L, 4);
	abs_timeout.tv_nsec = luaL_optinteger(L, 5, 0);

	if (mq_timedsend(*mqp, msg, len, prio, &abs_timeout) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mq_timedreceive(lua_State *L)
{
	struct timespec abs_timeout;
	mqd_t *mqp;
	char *buf;
	size_t buflen;
	ssize_t len;
	unsigned prio;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	buflen = luaL_checkinteger(L, 2);
	abs_timeout.tv_sec = luaL_checkinteger(L, 3);
	abs_timeout.tv_nsec = luaL_optinteger(L, 4, 0);

	buf = malloc(buflen);
	if (buf == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	prio = 0;

	len = mq_timedreceive(*mqp, buf, buflen, &prio, &abs_timeout);
	if (len == -1) {
		int error = errno;

		free(buf);
		return (fail(L, error));
	}
	lua_pushlstring(L, buf, len);
	lua_pushinteger(L, prio);
	return (2);
}

static int
l_mq_getfd_np(lua_State *L)
{
	mqd_t *mqp;
	int fd;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	fd = mq_getfd_np(*mqp);
	lua_pushinteger(L, fd);
	return (1);
}

static int
l_mq_gc(lua_State *L)
{
	mqd_t mq, *mqp;

	mqp = luaL_checkudata(L, 1, MQD_METATABLE);
	if (mqp == NULL) {
		return (0);
	}
	mq = *mqp;
	if (mq == (mqd_t)-1) {
		return (0);
	}
	*mqp = (mqd_t)-1;
	if (mq_close(mq) == -1) {
		return (fail(L, errno));
	}
	return (0);
}

static const struct luaL_Reg l_mqueue_funcs[] = {
	{"open", l_mq_open},
	{"unlink", l_mq_unlink},
	{NULL, NULL}
};

static const struct luaL_Reg l_mqd_meta[] = {
	{"__close", l_mq_gc},
	{"__gc", l_mq_gc},
	{"close", l_mq_close},
	{"getattr", l_mq_getattr},
	{"setattr", l_mq_setattr},
	{"send", l_mq_send},
	{"receive", l_mq_receive},
	{"timedsend", l_mq_timedsend},
	{"timedreceive", l_mq_timedreceive},
	{"getfd", l_mq_getfd_np},
	{NULL, NULL}
};

int
luaopen_mqueue(lua_State *L)
{
	luaL_newmetatable(L, MQD_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_mqd_meta, 0);

	luaL_newlib(L, l_mqueue_funcs);
	return (1);
}
