/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <pwd.h>
#include <unistd.h>

#include <lua.h>

#define PWD_INITIAL_BUFSIZE getpagesize()

static inline void
pushpasswd(lua_State *L, struct passwd *pwd)
{
	lua_newtable(L);
#define SFIELD(name) ({ \
	lua_pushstring(L, pwd->pw_ ## name); \
	lua_setfield(L, -2, #name); \
})
	SFIELD(name);
	SFIELD(passwd);
	SFIELD(class);
	SFIELD(gecos);
	SFIELD(dir);
	SFIELD(shell);
#undef SFIELD
#define IFIELD(name) ({ \
	lua_pushinteger(L, pwd->pw_ ## name); \
	lua_setfield(L, -2, #name); \
})
	IFIELD(uid);
	IFIELD(gid);
	IFIELD(change);
	IFIELD(expire);
#undef IFIELD
}
