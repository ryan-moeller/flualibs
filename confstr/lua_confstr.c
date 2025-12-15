/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

int luaopen_confstr(lua_State *);

static int
l_cs_confstr(lua_State *L)
{
	char *value;
	size_t len, newlen;
	int name;

	name = luaL_checkinteger(L, 1);

	value = NULL;
	len = 0;
	errno = 0;
	while ((newlen = confstr(name, value, len)) > len) {
		free(value);
		len = newlen * 2;
		if ((value = malloc(len)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	if (newlen == 0) {
		int error = errno;

		free(value);
		if (error == 0) {
			return (0);
		}
		return (fail(L, error));
	}
	lua_pushlstring(L, value, newlen);
	free(value);
	return (1);
}

static const struct luaL_Reg l_confstr_funcs[] = {
	{"confstr", l_cs_confstr},
	{NULL, NULL}
};

int
luaopen_confstr(lua_State *L)
{
	luaL_newlib(L, l_confstr_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, _CS_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(PATH);
	SETCONST(POSIX_V6_ILP32_OFF32_CFLAGS);
	SETCONST(POSIX_V6_ILP32_OFF32_LDFLAGS);
	SETCONST(POSIX_V6_ILP32_OFF32_LIBS);
	SETCONST(POSIX_V6_ILP32_OFFBIG_CFLAGS);
	SETCONST(POSIX_V6_ILP32_OFFBIG_LDFLAGS);
	SETCONST(POSIX_V6_ILP32_OFFBIG_LIBS);
	SETCONST(POSIX_V6_LP64_OFF64_CFLAGS);
	SETCONST(POSIX_V6_LP64_OFF64_LDFLAGS);
	SETCONST(POSIX_V6_LP64_OFF64_LIBS);
	SETCONST(POSIX_V6_LPBIG_OFFBIG_CFLAGS);
	SETCONST(POSIX_V6_LPBIG_OFFBIG_LDFLAGS);
	SETCONST(POSIX_V6_LPBIG_OFFBIG_LIBS);
	SETCONST(POSIX_V6_WIDTH_RESTRICTED_ENVS);
#undef SETCONST
	return (1);
}
