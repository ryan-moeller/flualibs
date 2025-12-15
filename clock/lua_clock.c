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

#include "../utils.h"

int luaopen_clock(lua_State *);

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

static const struct luaL_Reg l_clock_funcs[] = {
	{"getcpuclockid", l_clock_getcpuclockid},
	{"getres", l_clock_getres},
	{"gettime", l_clock_gettime},
	{"nanosleep", l_clock_nanosleep},
	{"settime", l_clock_settime},
	{NULL, NULL}
};

int
luaopen_clock(lua_State *L)
{
	luaL_newlib(L, l_clock_funcs);

#define SETCONST(ident) ({ \
	lua_pushinteger(L, CLOCK_##ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(REALTIME);
	SETCONST(REALTIME_PRECISE);
	SETCONST(REALTIME_FAST);
	SETCONST(REALTIME_COARSE);

	SETCONST(MONOTONIC);
	SETCONST(MONOTONIC_PRECISE);
	SETCONST(MONOTONIC_FAST);
	SETCONST(MONOTONIC_COARSE);
	SETCONST(BOOTTIME);

	SETCONST(UPTIME);
	SETCONST(UPTIME_PRECISE);
	SETCONST(UPTIME_FAST);

	SETCONST(VIRTUAL);

	SETCONST(PROF);

	SETCONST(SECOND);

	SETCONST(PROCESS_CPUTIME_ID);

	SETCONST(THREAD_CPUTIME_ID);

#ifdef CLOCK_TAI
	SETCONST(TAI);
#endif
#undef SETCONST
	lua_pushinteger(L, TIMER_ABSTIME);
	lua_setfield(L, -2, "TIMER_ABSTIME");
	return (1);
}
