/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023-2025 Ryan Moeller <ryan-moeller@att.net>
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
#include <sys/linker.h>
#include <sys/module.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

int luaopen_kldstat(lua_State *);

static int
l_kldstat_iter_next(lua_State *L)
{
	struct kld_file_stat stat;
	int nextid;

	nextid = luaL_checkinteger(L, lua_upvalueindex(1));
	if (nextid == 0) {
		return (0);
	}
	stat.version = sizeof (stat);
	if (kldstat(nextid, &stat) == -1) {
		luaL_error(L, "cannot kldstat %d: %s", nextid, strerror(errno));
	}
	nextid = kldnext(nextid);
	lua_pushinteger(L, nextid);
	lua_replace(L, lua_upvalueindex(1));

	lua_newtable(L);
	lua_pushstring(L, stat.name);
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, stat.refs);
	lua_setfield(L, -2, "refs");
	lua_pushinteger(L, stat.id);
	lua_setfield(L, -2, "id");
	{
		char *s = NULL;
		if (asprintf(&s, "%p", stat.address) == -1) {
			luaL_error(L, "cannot allocate for address");
		}
		lua_pushstring(L, s);
		free(s);
	}
	lua_setfield(L, -2, "address");
	lua_pushinteger(L, stat.size);
	lua_setfield(L, -2, "size");
	lua_pushstring(L, stat.pathname);
	lua_setfield(L, -2, "pathname");
	return (1);
}

static int
l_kldstat(lua_State *L)
{

	lua_pushinteger(L, kldnext(0));
	lua_pushcclosure(L, l_kldstat_iter_next, 1);
	return (1);
}

static int
l_modstat_next(lua_State *L)
{
	struct module_stat stat;
	int nextid;

	nextid = luaL_checkinteger(L, lua_upvalueindex(1));
	if (nextid == 0) {
		return (0);
	}
	stat.version = sizeof (stat);
	if (modstat(nextid, &stat) == -1) {
		luaL_error(L, "cannot modstat %d: %s", nextid, strerror(errno));
	}
	nextid = modfnext(nextid);
	lua_pushinteger(L, nextid);
	lua_replace(L, lua_upvalueindex(1));

	lua_newtable(L);
	lua_pushstring(L, stat.name);
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, stat.refs);
	lua_setfield(L, -2, "refs");
	lua_pushinteger(L, stat.id);
	lua_setfield(L, -2, "id");
	lua_pushinteger(L, stat.data.intval);
	lua_setfield(L, -2, "data");
	return (1);
}

static int
l_modstat(lua_State *L)
{
	int fileid;

	fileid = luaL_checkinteger(L, 1);
	lua_pushinteger(L, kldfirstmod(fileid));
	lua_pushcclosure(L, l_modstat_next, 1);
	return (1);
}

static const struct luaL_Reg l_kldstat_funcs[] = {
	/* Iterate kernel module files, yielding stats for each. */
	{"kldstat", l_kldstat},
	/* Iterate modules in a file given fileid, yielding stats for each. */
	{"modstat", l_modstat},
	{NULL, NULL}
};

int
luaopen_kldstat(lua_State *L)
{
	luaL_newlib(L, l_kldstat_funcs);
	return (1);
}
