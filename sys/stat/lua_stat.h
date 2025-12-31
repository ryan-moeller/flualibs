/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/param.h>
#include <sys/stat.h>

#include <lua.h>

static inline void
pushstat(lua_State *L, struct stat *sb)
{
	lua_newtable(L);
#define INTFIELD(name) ({ \
	lua_pushinteger(L, sb->st_##name); \
	lua_setfield(L, -2, #name); \
})
	INTFIELD(dev);
	INTFIELD(ino);
	INTFIELD(nlink);
	INTFIELD(mode);
#ifdef SFBSD_NAMEDATTR
	INTFIELD(bsdflags);
#endif
	INTFIELD(uid);
	INTFIELD(gid);
	INTFIELD(rdev);
	INTFIELD(size);
	INTFIELD(blocks);
	INTFIELD(blksize);
	INTFIELD(flags);
	INTFIELD(gen);
#if __FreeBSD_version > 1402501
	INTFIELD(filerev);
#endif
#undef INTFIELD
#define TIMEFIELD(name) ({ \
	lua_createtable(L, 0, 2); \
	lua_pushinteger(L, sb->st_##name.tv_sec); \
	lua_setfield(L, -2, "sec"); \
	lua_pushinteger(L, sb->st_##name.tv_nsec); \
	lua_setfield(L, -2, "nsec"); \
	lua_setfield(L, -2, #name); \
})
	TIMEFIELD(atim);
	TIMEFIELD(mtim);
	TIMEFIELD(ctim);
	TIMEFIELD(birthtim);
#undef TIMEFIELD
}
