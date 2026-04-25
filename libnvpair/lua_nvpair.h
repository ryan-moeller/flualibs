/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/param.h>
#include <sys/nvpair.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define NVLIST_METATABLE "nvlist_t*"
#define NVPAIR_METATABLE "nvpair_t*"

static inline void
pushnvlist(lua_State *L, nvlist_t *nvl)
{
	new(L, nvl, NVLIST_METATABLE);
}

static inline nvlist_t *
checknvlist(lua_State *L, int idx)
{
	return (checkcookie(L, idx, NVLIST_METATABLE));
}

static inline nvlist_t *
optnvlist(lua_State *L, int idx, nvlist_t *dflt)
{
	return (luaL_opt(L, checknvlist, idx, dflt));
}
