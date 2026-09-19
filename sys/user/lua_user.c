/*
 * Copyright (c) 2023-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/user.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/user/lua_user.h"
#include "utils.h"

int luaopen_sys_user(lua_State *);

static int
l_kinfo_proc_index(lua_State *L)
{
	struct kinfo_proc *p;
	const char *field;

	p = checkcookie(L, 1, KINFO_PROC_METATABLE);
	field = luaL_checkstring(L, 2);

#define PTRFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		lua_pushinteger(L, (uintptr_t)p->ki_ ## name); \
		return (1); \
	} \
})
#define INTFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		lua_pushinteger(L, p->ki_ ## name); \
		return (1); \
	} \
})
#define INTSFIELD(name, len) ({ \
	if (strcmp(field, #name) == 0) { \
		lua_createtable(L, len, 0); \
		for (int i = 0; i < len; i++) { \
			lua_pushinteger(L, p->ki_ ## name[i]); \
			lua_rawseti(L, -2, i + 1); \
		} \
		return (1); \
	} \
})
#define STRFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		lua_pushstring(L, p->ki_ ## name); \
		return (1); \
	} \
})
#define SIGSETFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		lua_pushlstring(L, (char *)&p->ki_ ## name, sizeof(sigset_t)); \
		return (1); \
	} \
})
#define TVFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		pushtimeval(L, &p->ki_ ## name); \
		return (1); \
	} \
})
#define RUFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		pushrusage(L, &p->ki_ ## name); \
		return (1); \
	} \
})
	INTFIELD(structsize);
	INTFIELD(layout);
	PTRFIELD(args);
	PTRFIELD(paddr);
	PTRFIELD(addr);
	PTRFIELD(tracep);
	PTRFIELD(textvp);
	PTRFIELD(fd);
	PTRFIELD(vmspace);
	PTRFIELD(wchan);
	INTFIELD(pid);
	INTFIELD(ppid);
	INTFIELD(pgid);
	INTFIELD(tpgid);
	INTFIELD(sid);
	INTFIELD(tsid);
	INTFIELD(jobc);
	INTFIELD(tdev_freebsd11);
	SIGSETFIELD(siglist);
	SIGSETFIELD(sigmask);
	SIGSETFIELD(sigignore);
	SIGSETFIELD(sigcatch);
	INTFIELD(uid);
	INTFIELD(ruid);
	INTFIELD(svuid);
	INTFIELD(rgid);
	INTFIELD(svgid);
	INTFIELD(ngroups);
	INTSFIELD(groups, KI_NGROUPS);
	INTFIELD(size);
	INTFIELD(rssize);
	INTFIELD(swrss);
	INTFIELD(tsize);
	INTFIELD(dsize);
	INTFIELD(ssize);
	INTFIELD(xstat);
	INTFIELD(acflag);
	INTFIELD(pctcpu);
	INTFIELD(estcpu);
	INTFIELD(slptime);
	INTFIELD(swtime);
	INTFIELD(cow);
	INTFIELD(runtime);
	TVFIELD(start);
	TVFIELD(childtime);
	INTFIELD(flag);
	INTFIELD(kiflag);
	INTFIELD(traceflag);
	INTFIELD(stat);
	INTFIELD(nice);
	INTFIELD(lock);
	INTFIELD(rqindex);
	INTFIELD(oncpu_old);
	INTFIELD(lastcpu_old);
	STRFIELD(tdname);
	STRFIELD(wmesg);
	STRFIELD(login);
	STRFIELD(lockname);
	STRFIELD(comm);
	STRFIELD(emul);
	STRFIELD(loginclass);
	STRFIELD(moretdname);
	INTFIELD(tdev);
	INTFIELD(oncpu);
	INTFIELD(lastcpu);
	INTFIELD(tracer);
	INTFIELD(flag2);
	INTFIELD(fibnum);
	INTFIELD(cr_flags);
	INTFIELD(jid);
	INTFIELD(numthreads);
	INTFIELD(tid);
	if (strcmp(field, "pri") == 0) {
		lua_createtable(L, 0, 4);
#define FIELD(name) ({ \
		lua_pushinteger(L, p->ki_pri.pri_ ## name); \
		lua_setfield(L, -2, #name); \
})
		FIELD(class);
		FIELD(level);
		FIELD(native);
		FIELD(user);
#undef FIELD
		return (1);
	}
	RUFIELD(rusage);
	RUFIELD(rusage_ch);
	PTRFIELD(pcb);
	PTRFIELD(kstack);
	PTRFIELD(udata);
	PTRFIELD(tdaddr);
	PTRFIELD(pd);
#if __FreeBSD_version > 1500044
	PTRFIELD(uerrmsg);
#endif
	INTFIELD(sflag);
	INTFIELD(tdflags);
#undef PTRFIELD
#undef INTFIELD
#undef INTSFIELD
#undef STRFIELD
#undef SIGSETFIELD
#undef TVFIELD
	/* No field name match. */
	return (0);
}

static const struct luaL_Reg l_kinfo_proc_meta[] = {
	{"__index", l_kinfo_proc_index},
	{NULL, NULL}
};

int
luaopen_sys_user(lua_State *L)
{
	luaL_newmetatable(L, KINFO_PROC_METATABLE);
	luaL_setfuncs(L, l_kinfo_proc_meta, 0);
	return (0);
}
