/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <resolv.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"

int luaopen_b64(lua_State *);

static int
l_base64_encode(lua_State *L)
{
	const u_char *data;
	size_t len, strlen;
	char *str;
	int res;

	data = (const u_char *)luaL_checklstring(L, 1, &len);

	/* Base64 encodes 3 bytes into 4 characters. */
	strlen = 4 * (len + 2) / 3 + 1; /* len + 2 to round up */
	str = malloc(strlen);
	if (str == NULL)
		return (luaL_error(L, "malloc failed"));

	res = b64_ntop(data, len, str, strlen);
	if (res == -1) {
		free(str);
		return (luaL_error(L, "b64_ntop failed"));
	}
	lua_pushlstring(L, str, res);
	free(str);
	return (1);
}

static int
l_base64_decode(lua_State *L)
{
	const char *str;
	size_t len;
	u_char *data;
	int res;

	str = luaL_checklstring(L, 1, &len);

	/* Each char represents 6 bits, so the data is 3/4 * len bytes long. */
	len = 3 * len / 4;
	data = malloc(len);
	if (data == NULL)
		return (luaL_error(L, "malloc failed"));

	res = b64_pton(str, data, len);
	if (res == -1) {
		free(data);
		return (luaL_error(L, "b64_pton failed"));
	}
	lua_pushlstring(L, (char *)data, res);
	free(data);
	return (1);
}

static const struct luaL_Reg l_b64_funcs[] = {
	{"encode", l_base64_encode},
	{"decode", l_base64_decode},
	{NULL, NULL}
};

int
luaopen_b64(lua_State *L)
{
	luaL_newlib(L, l_b64_funcs);
	return (1);
}
