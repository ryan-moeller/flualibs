/*
 * Copyright (c) 2025-2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

int luaopen_time(lua_State *);

static int
l_clock_getcpuclockid(lua_State *L)
{
	pid_t pid;
	clockid_t clock_id;
	int error;

	pid = luaL_checkinteger(L, 1);

	if ((error = clock_getcpuclockid(pid, &clock_id)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, pid);
	return (1);
}

static int
l_clock_getres(lua_State *L)
{
	struct timespec t;
	clockid_t clock_id;
	int error;

	clock_id = luaL_checkinteger(L, 1);

	if ((error = clock_getres(clock_id, &t)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, t.tv_sec);
	lua_pushinteger(L, t.tv_nsec);
	return (2);
}

static int
l_clock_gettime(lua_State *L)
{
	struct timespec t;
	clockid_t clock_id;
	int error;

	clock_id = luaL_checkinteger(L, 1);

	if ((error = clock_gettime(clock_id, &t)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, t.tv_sec);
	lua_pushinteger(L, t.tv_nsec);
	return (2);
}

static int
l_clock_nanosleep(lua_State *L)
{
	struct timespec rqt, rmt;
	clockid_t clock_id;
	int flags, error;

	clock_id = luaL_checkinteger(L, 1);
	flags = luaL_checkinteger(L, 2);
	rqt.tv_sec = luaL_checkinteger(L, 3);
	rqt.tv_nsec = luaL_optinteger(L, 4, 0);

	if ((error = clock_nanosleep(clock_id, flags, &rqt, &rmt)) != 0) {
		if (error != EINTR) {
			return (fail(L, error));
		}
		lua_pushinteger(L, rmt.tv_sec);
		lua_pushinteger(L, rmt.tv_nsec);
		return (2);
	}
	return (0);
}

static int
l_clock_settime(lua_State *L)
{
	struct timespec t;
	clockid_t clock_id;
	int error;

	clock_id = luaL_checkinteger(L, 1);
	t.tv_sec = luaL_checkinteger(L, 2);
	t.tv_nsec = luaL_optinteger(L, 3, 0);

	if ((error = clock_settime(clock_id, &t)) != 0) {
		return (fail(L, error));
	}
	return (0);
}

static const struct luaL_Reg l_time_funcs[] = {
	{"clock_getcpuclockid", l_clock_getcpuclockid},
	{"clock_getres", l_clock_getres},
	{"clock_gettime", l_clock_gettime},
	{"clock_nanosleep", l_clock_nanosleep},
	{"clock_settime", l_clock_settime},
	{NULL, NULL}
};

int
luaopen_time(lua_State *L)
{
	luaL_newlib(L, l_time_funcs);

#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(CLOCK_REALTIME);
	DEFINE(CLOCK_REALTIME_PRECISE);
	DEFINE(CLOCK_REALTIME_FAST);
	DEFINE(CLOCK_REALTIME_COARSE);

	DEFINE(CLOCK_MONOTONIC);
	DEFINE(CLOCK_MONOTONIC_PRECISE);
	DEFINE(CLOCK_MONOTONIC_FAST);
	DEFINE(CLOCK_MONOTONIC_COARSE);
	DEFINE(CLOCK_BOOTTIME);

	DEFINE(CLOCK_UPTIME);
	DEFINE(CLOCK_UPTIME_PRECISE);
	DEFINE(CLOCK_UPTIME_FAST);

	DEFINE(CLOCK_VIRTUAL);

	DEFINE(CLOCK_PROF);

	DEFINE(CLOCK_SECOND);

	DEFINE(CLOCK_PROCESS_CPUTIME_ID);

	DEFINE(CLOCK_THREAD_CPUTIME_ID);

#ifdef CLOCK_TAI
	DEFINE(CLOCK_TAI);
#endif

	DEFINE(TIMER_ABSTIME);
#undef DEFINE
	return (1);
}
