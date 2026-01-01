/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/_iovec.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

static inline void
freeriovecs(struct iovec *iovs, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		free(iovs[i].iov_base);
	}
	free(iovs);
}

static inline struct iovec *
checkriovecs(lua_State *L, int idx, size_t *niov)
{
	struct iovec *iovs;
	size_t n;
	int error;

	*niov = 0;

	luaL_checktype(L, idx, LUA_TTABLE);

	n = luaL_len(L, idx);
	if ((iovs = calloc(n, sizeof(*iovs))) == NULL) {
		fatal(L, "calloc", ENOMEM);
	}
	error = 0;
	for (size_t i = 0; i < n; i++) {
		struct iovec *iov = &iovs[i];

		lua_geti(L, idx, i + 1);
		if (!lua_isinteger(L, -1)) {
			error = EINVAL;
			goto fail;
		}
		iov->iov_len = lua_tointeger(L, -1);
		if ((iov->iov_base = malloc(iov->iov_len)) == NULL) {
			error = ENOMEM;
			goto fail;
		}
	}
	*niov = n;
	return (iovs);
fail:
	freeriovecs(iovs, n);
	if (error == EINVAL) {
		luaL_argerror(L, idx, "expected iovec buffer lengths");
	}
	fatal(L, "malloc", ENOMEM);
}

static inline void
pushriovecs(lua_State *L, struct iovec *iovs, size_t n)
{
	lua_createtable(L, n, 0);
	for (size_t i = 0; i < n; i++) {
		lua_pushlstring(L, iovs[i].iov_base, iovs[i].iov_len);
		lua_rawseti(L, -2, i + 1);
	}
	freeriovecs(iovs, n);
}

static inline struct iovec *
checkwiovecs(lua_State *L, int idx, size_t *niov)
{
	struct iovec *iovs;
	size_t n;
	int error;

	*niov = 0;

	luaL_checktype(L, idx, LUA_TTABLE);

	n = luaL_len(L, idx);
	if ((iovs = calloc(n, sizeof(*iovs))) == NULL) {
		fatal(L, "calloc", ENOMEM);
	}
	error = 0;
	for (size_t i = 0; i < n; i++) {
		struct iovec *iov = &iovs[i];
		const char *p;

		if (lua_geti(L, idx, i + 1) != LUA_TSTRING) {
			free(iovs);
			luaL_argerror(L, idx, "expected strings");
		}
		p = lua_tolstring(L, -1, &iov->iov_len);
		iov->iov_base = __DECONST(char *, p);
	}
	*niov = n;
	return (iovs);
}
