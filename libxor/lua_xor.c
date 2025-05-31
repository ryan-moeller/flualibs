/*-
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

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
	luaL_argcheck(L, luaL_len(L, 2) == keylen, 2, "`key' with length 4 expected");

	for (int i = 1; i <= keylen; ++i) {
		if (lua_rawgeti(L, 2, i) != LUA_TNUMBER) {
			luaL_error(L, "`key[%d]' is not a number", i);
		}
		key[i-1] = lua_tonumber(L, -1);
	}
	lua_pop(L, keylen);

	output = (char *)malloc(len);
	if (output == NULL) {
		luaL_error(L, "malloc failed: %s", strerror(errno));
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
