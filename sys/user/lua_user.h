/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/user.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define KINFO_PROC_METATABLE "struct kinfo_proc *"

static inline void
pushtimeval(lua_State *L, struct timeval *tv)
{
	lua_createtable(L, 0, 2);
#define FIELD(name) ({ \
	lua_pushinteger(L, tv->tv_ ## name); \
	lua_setfield(L, -2, #name); \
})
	FIELD(sec);
	FIELD(usec);
#undef FIELD
}

static inline void
pushrusage(lua_State *L, struct rusage *ru)
{
	lua_newtable(L);
#define TVFIELD(name) ({ \
	pushtimeval(L, &ru->ru_ ## name); \
	lua_setfield(L, -2, #name); \
})
#define INTFIELD(name) ({ \
	lua_pushinteger(L, ru->ru_ ## name); \
	lua_setfield(L, -2, #name); \
})
	TVFIELD(utime);
	TVFIELD(stime);
	INTFIELD(maxrss);
	INTFIELD(ixrss);
	INTFIELD(idrss);
	INTFIELD(isrss);
	INTFIELD(minflt);
	INTFIELD(majflt);
	INTFIELD(nswap);
	INTFIELD(inblock);
	INTFIELD(oublock);
	INTFIELD(msgsnd);
	INTFIELD(msgrcv);
	INTFIELD(nsignals);
	INTFIELD(nvcsw);
	INTFIELD(nivcsw);
#undef TVFIELD
#undef INTFIELD
}
