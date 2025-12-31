/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <sha.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "luaerror.h"

#define SHA_CTX_METATABLE "SHA_CTX"

int luaopen_md(lua_State *);

static int
l_sha1_init(lua_State *L)
{
	SHA_CTX *ctx;

	ctx = lua_newuserdatauv(L, sizeof(SHA_CTX), 0);
	luaL_setmetatable(L, SHA_CTX_METATABLE);

	SHA1_Init(ctx);
	return (1);
}

static int
l_sha1_update(lua_State *L)
{
	SHA_CTX *ctx;
	const unsigned char *data;
	size_t len;

	ctx = luaL_checkudata(L, 1, SHA_CTX_METATABLE);
	data = (const unsigned char *)luaL_checklstring(L, 2, &len);

	SHA1_Update(ctx, data, len);
	return (0);
}

static int
l_sha1_final(lua_State *L)
{
	unsigned char digest[20];
	SHA_CTX *ctx;

	ctx = luaL_checkudata(L, 1, SHA_CTX_METATABLE);

	SHA1_Final(digest, ctx);
	lua_pushlstring(L, (const char *)digest, sizeof(digest));
	return (1);
}

static int
l_sha1_end(lua_State *L)
{
	char buf[41], *res;
	SHA_CTX *ctx;

	ctx = luaL_checkudata(L, 1, SHA_CTX_METATABLE);

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
	luaL_newmetatable(L, SHA_CTX_METATABLE);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	luaL_setfuncs(L, l_sha_ctx_meta, 0);

	luaL_newlib(L, l_md_funcs);

	return (1);
}
