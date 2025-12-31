/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <readpassphrase.h>
#include <stdlib.h>
#include <strings.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

int luaopen_readpassphrase(lua_State *);

static int
l_readpassphrase(lua_State *L)
{
	const char *prompt;
	char *buf;
	size_t buflen;
	int flags;

	prompt = luaL_checkstring(L, 1);
	buflen = luaL_checkinteger(L, 2);
	flags = luaL_optinteger(L, 3, 0);

	if ((buf = malloc(buflen)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if (readpassphrase(prompt, buf, buflen, flags) == NULL) {
		int error = errno;

		explicit_bzero(buf, buflen);
		free(buf);
		return (fail(L, error));
	}
	lua_pushstring(L, buf);
	explicit_bzero(buf, buflen);
	free(buf);
	return (1);
}

static int
l_explicit_bzero(lua_State *L)
{
	const char *s;
	size_t len;

	s = luaL_checklstring(L, 1, &len);

	explicit_bzero(__DECONST(char *, s), len);
	return (0);
}

static const struct luaL_Reg l_rpp_funcs[] = {
	{"readpassphrase", l_readpassphrase},
	{"explicit_bzero", l_explicit_bzero},
	{NULL, NULL}
};

int
luaopen_readpassphrase(lua_State *L)
{
	luaL_newlib(L, l_rpp_funcs);

#define DEFINE(ident) ({ \
	lua_pushinteger(L, RPP_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(ECHO_OFF);
	DEFINE(ECHO_ON);
	DEFINE(REQUIRE_TTY);
	DEFINE(FORCELOWER);
	DEFINE(FORCEUPPER);
	DEFINE(SEVENBIT);
	DEFINE(STDIN);
#undef DEFINE
	return (1);
}
