/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <grp.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>

#define GRP_INITIAL_BUFSIZE getpagesize()

static inline void
pushgroup(lua_State *L, struct group *grp)
{
	lua_newtable(L);
	lua_pushstring(L, grp->gr_name);
	lua_setfield(L, -2, "name");
	lua_pushstring(L, grp->gr_passwd);
	lua_setfield(L, -2, "passwd");
	lua_pushinteger(L, grp->gr_gid);
	lua_setfield(L, -2, "gid");
	lua_newtable(L);
	for (char **mem = grp->gr_mem; *mem != NULL; mem++) {
		lua_pushstring(L, *mem);
		lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	}
	lua_setfield(L, -2, "mem");
}
