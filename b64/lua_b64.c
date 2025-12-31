/*
 * Copyright (c) 2024-2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <resolv.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

int luaopen_b64(lua_State *);

static int
l_b64_encode(lua_State *L)
{
	const u_char *data;
	char *buf;
	size_t datalen, buflen;
	int len;

	data = (const u_char *)luaL_checklstring(L, 1, &datalen);

	/* Base64 encodes 3 bytes into 4 characters. */
	buflen = 4 * (datalen + 2) / 3 + 1; /* datalen + 2 to round up */
	if ((buf = malloc(buflen)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	len = b64_ntop(data, datalen, buf, buflen);
	assert(len != -1); /* Only error cases are for buflen too small. */
	lua_pushlstring(L, buf, len);
	free(buf);
	return (1);
}

static int
l_b64_decode(lua_State *L)
{
	const char *encoded;
	u_char *buf;
	size_t enclen, buflen;
	int len;

	encoded = luaL_checklstring(L, 1, &enclen);

	/* Each char encodes 6 bits, so the data is 3/4 * enclen bytes long. */
	buflen = 3 * enclen / 4;
	if ((buf = malloc(buflen)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((len = b64_pton(encoded, buf, buflen)) == -1) {
		free(buf);
		return (fail(L, EINVAL));
	}
	lua_pushlstring(L, (char *)buf, len);
	free(buf);
	return (1);
}

static const struct luaL_Reg l_b64_funcs[] = {
	{"encode", l_b64_encode},
	{"decode", l_b64_decode},
	{NULL, NULL}
};

int
luaopen_b64(lua_State *L)
{
	luaL_newlib(L, l_b64_funcs);
	return (1);
}
