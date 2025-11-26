/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

#define MQ_METATABLE "mq"

int luaopen_mq(lua_State *);

static int
l_mq_open(lua_State *L)
{
	const char *name;
	int oflag;
	mqd_t mq, *mqp;

	mqp = lua_newuserdatauv(L, sizeof(*mqp), 0);
	luaL_setmetatable(L, MQ_METATABLE);
	*mqp = (mqd_t)-1;

	name = luaL_checkstring(L, 1);
	oflag = luaL_checkinteger(L, 2);
	if (oflag & O_CREAT) {
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

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
	mq = *mqp;
	*mqp = (mqd_t)-1;
	if (mq_close(mq) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_mq_unlink(lua_State *L)
{
	const char *name;

	name = luaL_checkstring(L, 1);

	if (mq_unlink(name) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_mq_getattr(lua_State *L)
{
	mqd_t *mqp;
	struct mq_attr attr;

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);

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

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
	attr.mq_flags = luaL_checkinteger(L, 2);

	if (mq_setattr(*mqp, &attr, NULL) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_mq_send(lua_State *L)
{
	mqd_t *mqp;
	const char *msg;
	size_t len;
	unsigned prio;

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
	msg = luaL_checklstring(L, 2, &len);
	prio = luaL_checkinteger(L, 3);

	if (mq_send(*mqp, msg, len, prio) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_mq_receive(lua_State *L)
{
	mqd_t *mqp;
	char *buf;
	size_t buflen;
	ssize_t len;
	unsigned prio;

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
	buflen = luaL_checkinteger(L, 2);
	buf = malloc(buflen);
	if (buf == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	prio = 0;

	len = mq_receive(*mqp, buf, buflen, &prio);
	if (len == -1) {
		free(buf);
		return (fail(L, errno));
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

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
	msg = luaL_checklstring(L, 2, &len);
	prio = luaL_checkinteger(L, 3);
	abs_timeout.tv_sec = luaL_checkinteger(L, 4);
	abs_timeout.tv_nsec = luaL_optinteger(L, 5, 0);

	if (mq_timedsend(*mqp, msg, len, prio, &abs_timeout) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
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

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
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
		free(buf);
		return (fail(L, errno));
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

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
	fd = mq_getfd_np(*mqp);
	lua_pushinteger(L, fd);
	return (1);
}

static int
l_mq_gc(lua_State *L)
{
	mqd_t mq, *mqp;

	mqp = luaL_checkudata(L, 1, MQ_METATABLE);
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

static const struct luaL_Reg l_mq_funcs[] = {
	{"open", l_mq_open},
	{"unlink", l_mq_unlink},
	{NULL, NULL}
};

static const struct luaL_Reg l_mq_meta[] = {
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
luaopen_mq(lua_State *L)
{
	luaL_newmetatable(L, MQ_METATABLE);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, l_mq_gc);
	lua_setfield(L, -2, "__gc");

	luaL_setfuncs(L, l_mq_meta, 0);

	luaL_newlib(L, l_mq_funcs);

#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(O_RDONLY);
	SETCONST(O_WRONLY);
	SETCONST(O_RDWR);
	SETCONST(O_CREAT);
	SETCONST(O_EXCL);
	SETCONST(O_NONBLOCK);
#undef SETCONST
	return (1);
}
