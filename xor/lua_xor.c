/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"
#include "utils.h"

int luaopen_xor(lua_State *);

static int
l_xor_apply(lua_State *L)
{
	const int keylen = 4;
	unsigned char key[keylen];
	const char *input;
	size_t len;
	char *output;

	input = luaL_checklstring(L, 1, &len);

	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_argcheck(L, luaL_len(L, 2) == keylen, 2,
	    "`key' with length 4 expected");

	for (int i = 1; i <= keylen; ++i) {
		if (lua_rawgeti(L, 2, i) != LUA_TNUMBER) {
			return (luaL_error(L, "`key[%d]' is not a number", i));
		}
		key[i-1] = lua_tonumber(L, -1);
	}
	lua_pop(L, keylen);

	output = malloc(len);
	if (output == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}

	for (size_t i = 0; i < len; ++i) {
		output[i] = input[i] ^ key[i % keylen];
	}

	lua_pushlstring(L, output, len);

	free(output);
	
	return (1);
}

static const struct luaL_Reg l_xor_funcs[] = {
	{"apply", l_xor_apply},
	{NULL, NULL}
};

int
luaopen_xor(lua_State *L)
{
	luaL_newlib(L, l_xor_funcs);
	return (1);
}
