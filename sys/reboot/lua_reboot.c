/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/reboot.h>

#include <lua.h>

int
luaopen_sys_reboot(lua_State *L)
{
	lua_newtable(L);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, RB_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(AUTOBOOT);
	DEFINE(ASKNAME);
	DEFINE(SINGLE);
	DEFINE(NOSYNC);
	DEFINE(HALT);
	DEFINE(INITNAME);
	DEFINE(DFLTROOT);
	DEFINE(KDB);
	DEFINE(RDONLY);
	DEFINE(DUMP);
	DEFINE(MINIROOT);
	DEFINE(VERBOSE);
	DEFINE(SERIAL);
	DEFINE(CDROM);
	DEFINE(POWEROFF);
	DEFINE(GDB);
	DEFINE(MUTE);
	DEFINE(SELFTEST);
	DEFINE(RESERVED1);
	DEFINE(RESERVED2);
	DEFINE(PAUSE);
	DEFINE(REROOT);
	DEFINE(POWERCYCLE);
#ifdef RB_MUTEMSGS
	DEFINE(MUTEMSGS);
#endif
#ifdef RB_KEXEC
	DEFINE(KEXEC);
#endif
	DEFINE(PROBE);
	DEFINE(MULTIPLE);
	DEFINE(BOOTINFO);
#undef DEFINE
	return (1);
}
