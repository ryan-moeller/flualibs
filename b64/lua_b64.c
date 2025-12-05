/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <resolv.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

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
