/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, Ryan Moeller <ryan-moeller@att.net>
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

#include <sys/types.h>
#include <sha.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

#define SHA_CTX_METATABLE "SHA_CTX"

int luaopen_md(lua_State *);

static int
l_sha1_init(lua_State *L)
{
	SHA_CTX *ctx;

	ctx = (SHA_CTX *)lua_newuserdata(L, sizeof (SHA_CTX));
	luaL_getmetatable(L, SHA_CTX_METATABLE);
	lua_setmetatable(L, -2);

	SHA1_Init(ctx);
	return (1);
}

static int
l_sha1_update(lua_State *L)
{
	SHA_CTX *ctx;
	const unsigned char *data;
	size_t len;

	ctx = (SHA_CTX *)luaL_checkudata(L, 1, SHA_CTX_METATABLE);
	luaL_argcheck(L, ctx != NULL, 1, "`ctx' expected");

	data = (const unsigned char *)luaL_checklstring(L, 2, &len);
	luaL_argcheck(L, data != NULL, 2, "`data' expected");

	SHA1_Update(ctx, data, len);

	return (0);
}

static int
l_sha1_final(lua_State *L)
{
	unsigned char digest[20];
	SHA_CTX *ctx;

	ctx = (SHA_CTX *)luaL_checkudata(L, 1, SHA_CTX_METATABLE);
	luaL_argcheck(L, ctx != NULL, 1, "`ctx' expected");

	SHA1_Final(digest, ctx);

	lua_pushlstring(L, (const char *)digest, sizeof digest);

	return (1);
}

static int
l_sha1_end(lua_State *L)
{
	char buf[41], *res;
	SHA_CTX *ctx;

	ctx = (SHA_CTX *)luaL_checkudata(L, 1, SHA_CTX_METATABLE);
	luaL_argcheck(L, ctx != NULL, 1, "`ctx' expected");

	res = SHA1_End(ctx, buf);

	lua_pushstring(L, res);

	return (1);
}

static const struct luaL_Reg l_md_funcs[] = {
	{"sha1_init", l_sha1_init},
	{NULL, NULL}
};

static const struct luaL_Reg l_sha_ctx_meta[] = {
	{"update", l_sha1_update},
	{"final", l_sha1_final},
	{"digest", l_sha1_end},
	{NULL, NULL}
};

int
luaopen_md(lua_State *L)
{
	lua_newtable(L);

	luaL_newmetatable(L, SHA_CTX_METATABLE);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	luaL_setfuncs(L, l_sha_ctx_meta, 0);

	luaL_setfuncs(L, l_md_funcs, 0);

	return (1);
}
