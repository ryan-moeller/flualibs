/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

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
	{"getcpuclockid", l_clock_getcpuclockid},
	{"getres", l_clock_getres},
	{"gettime", l_clock_gettime},
	{"nanosleep", l_clock_nanosleep},
	{"settime", l_clock_settime},
	{NULL, NULL}
};

int
luaopen_time(lua_State *L)
{
	luaL_newlib(L, l_time_funcs);

#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(CLOCK_REALTIME);
	SETCONST(CLOCK_REALTIME_PRECISE);
	SETCONST(CLOCK_REALTIME_FAST);
	SETCONST(CLOCK_REALTIME_COARSE);

	SETCONST(CLOCK_MONOTONIC);
	SETCONST(CLOCK_MONOTONIC_PRECISE);
	SETCONST(CLOCK_MONOTONIC_FAST);
	SETCONST(CLOCK_MONOTONIC_COARSE);
	SETCONST(CLOCK_BOOTTIME);

	SETCONST(CLOCK_UPTIME);
	SETCONST(CLOCK_UPTIME_PRECISE);
	SETCONST(CLOCK_UPTIME_FAST);

	SETCONST(CLOCK_VIRTUAL);

	SETCONST(CLOCK_PROF);

	SETCONST(CLOCK_SECOND);

	SETCONST(CLOCK_PROCESS_CPUTIME_ID);

	SETCONST(CLOCK_THREAD_CPUTIME_ID);

#ifdef CLOCK_TAI
	SETCONST(CLOCK_TAI);
#endif

	SETCONST(TIMER_ABSTIME);
#undef SETCONST
	return (1);
}
