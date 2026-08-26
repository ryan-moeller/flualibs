/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/devicestat.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define DEVSTAT_METATABLE "struct devstat *"

static inline struct devstat *
checkdevstat(lua_State *L, int idx)
{
	return (checkcookie(L, idx, DEVSTAT_METATABLE));
}
