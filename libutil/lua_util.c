/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libutil.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

int luaopen_util(lua_State *);

static int
l_expand_number(lua_State *L)
{
	const char *buf;
	int64_t num;

	buf = luaL_checkstring(L, 1);

	if (expand_number(buf, &num) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, num);
	return (1);
}

static int
l_expand_unsigned(lua_State *L)
{
	const char *buf;
	uint64_t num;

	buf = luaL_checkstring(L, 1);

	if (expand_unsigned(buf, &num) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, num);
	return (1);
}

static int
l_humanize_number(lua_State *L)
{
	luaL_Buffer b;
	const char *suffix;
	char *buf;
	int64_t number;
	size_t width, buflen;
	int scale, flags, res;

	width = luaL_checkinteger(L, 1);
	number = luaL_checkinteger(L, 2);
	suffix = luaL_checkstring(L, 3);
	scale = luaL_checkinteger(L, 4);
	flags = luaL_checkinteger(L, 5);

	buflen = width + 1; /* extra byte for termination */
	buf = luaL_buffinitsize(L, &b, buflen);
	res = humanize_number(buf, buflen, number, suffix, scale, flags);
	if (res == -1) {
		return (0);
	}
	if ((scale & HN_GETSCALE) != 0) {
		lua_pushinteger(L, res);
	} else {
		luaL_pushresultsize(&b, res);
	}
	return (1);
}

static const struct luaL_Reg l_util_funcs[] = {
	{"expand_number", l_expand_number},
	{"expand_unsigned", l_expand_unsigned},
	{"humanize_number", l_humanize_number},
	{NULL, NULL}
};

int
luaopen_util(lua_State *L)
{
	luaL_newlib(L, l_util_funcs);

#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(HN_DECIMAL);
	DEFINE(HN_NOSPACE);
	DEFINE(HN_B);
	DEFINE(HN_DIVISOR_1000);
	DEFINE(HN_IEC_PREFIXES);
	DEFINE(HN_GETSCALE);
	DEFINE(HN_AUTOSCALE);
#undef DEFINE
	return (1);
}
