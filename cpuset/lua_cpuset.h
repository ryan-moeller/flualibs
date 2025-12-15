/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef LUA_CPUSET_H
#define LUA_CPUSET_H

#include <sys/param.h>
#include <sys/cpuset.h>

#include <lua.h>
#include <lauxlib.h>

#define CPUSET_METATABLE "cpuset_t"

static inline cpuset_t *
newanoncpuset(lua_State *L)
{
	cpuset_t *set;

	set = lua_newuserdatauv(L, sizeof(*set), 0);
	luaL_setmetatable(L, CPUSET_METATABLE);
	return (set);
}

#endif
