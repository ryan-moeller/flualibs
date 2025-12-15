/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ryan Moeller
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

#include <sys/param.h>
#include <errno.h>
#include <pathconv.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../utils.h"

int luaopen_pathconv(lua_State *);

static int
l_abs2rel(lua_State *L)
{
	char result[MAXPATHLEN];
	const char *path, *base;

	path = luaL_checkstring(L, 1);
	base = luaL_checkstring(L, 2);

	if (abs2rel(path, base, result, sizeof(result)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_rel2abs(lua_State *L)
{
	char result[MAXPATHLEN];
	const char *path, *base;

	path = luaL_checkstring(L, 1);
	base = luaL_checkstring(L, 2);

	if (rel2abs(path, base, result, sizeof(result)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, result);
	return (1);
}

static const struct luaL_Reg l_pathconv_funcs[] = {
	{"abs2rel", l_abs2rel},
	{"rel2abs", l_rel2abs},
	{NULL, NULL}
};

int
luaopen_pathconv(lua_State *L)
{
	luaL_newlib(L, l_pathconv_funcs);
	return (1);
}
