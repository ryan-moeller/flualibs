/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/cnv.h>
#include <sys/dnv.h>
#include <sys/nv.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <strings.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

#define NVLIST_METATABLE "nvlist_t"
#define CONST_NVLIST_METATABLE "const nvlist_t"
#define CNVLIST_METATABLE "nvlist_t cookie"

#define NVLIST_TYPES(X) \
	X(bool, BOOL) \
	X(number, NUMBER) \
	X(string, STRING) \
	X(nvlist, NVLIST) \
	X(binary, BINARY) \
	X(bool_array, BOOL_ARRAY) \
	X(number_array, NUMBER_ARRAY) \
	X(string_array, STRING_ARRAY) \
	X(nvlist_array, NVLIST_ARRAY) \
	X(descriptor, DESCRIPTOR) \
	X(descriptor_array, DESCRIPTOR_ARRAY)

#define NVLIST_TYPES_NULL(X) \
	X(null, NULL) \
	NVLIST_TYPES(X)

int luaopen_nv(lua_State *);

static inline nvlist_t *
getnvlist(lua_State *L, int idx, const char *metatable)
{
	nvlist_t *nvl = NULL;

	if (lua_getmetatable(L, idx)) {
		luaL_getmetatable(L, metatable);
		if (lua_rawequal(L, -1, -2)) {
			nvl = getcookie(L, idx);
			lua_pop(L, 3);
		} else {
			lua_pop(L, 2);
		}
	}
	return (nvl);
}

static inline nvlist_t *
checknvlist(lua_State *L, int idx)
{
	nvlist_t *nvl;

	nvl = getnvlist(L, idx, NVLIST_METATABLE);
	luaL_argexpected(L, nvl != NULL, idx, NVLIST_METATABLE);
	luaL_argcheck(L, nvlist_error(nvl) == 0, idx, "nvlist has error");

	return (nvl);
}

static inline const nvlist_t *
checkconstnvlist(lua_State *L, int idx)
{
	const nvlist_t *nvl;

	nvl = getnvlist(L, idx, CONST_NVLIST_METATABLE);
	luaL_argexpected(L, nvl != NULL, idx, CONST_NVLIST_METATABLE);
	luaL_argcheck(L, nvlist_error(nvl) == 0, idx, "nvlist has error");

	return (nvl);
}

static inline nvlist_t *
getanynvlist(lua_State *L, int idx)
{
	nvlist_t *nvl = NULL;

	if (lua_getmetatable(L, idx)) {
		luaL_getmetatable(L, NVLIST_METATABLE);
		luaL_getmetatable(L, CONST_NVLIST_METATABLE);
		if (lua_rawequal(L, -1, -2) || lua_rawequal(L, -1, -3)) {
			nvl = getcookie(L, idx);
			lua_pop(L, 4);
		} else {
			lua_pop(L, 3);
		}
	}
	return (nvl);
}

static inline nvlist_t *
checkanynvlist(lua_State *L, int idx)
{
	nvlist_t *nvl;

	nvl = getanynvlist(L, idx);
	luaL_argexpected(L, nvl != NULL, idx, NVLIST_METATABLE);
	luaL_argcheck(L, nvlist_error(nvl) == 0, idx, "nvlist has error");

	return (nvl);
}

static inline const char *
checkname(lua_State *L, int idx, const nvlist_t *nvl, int type)
{
	const char *name;

	name = luaL_checkstring(L, idx);
	luaL_argcheck(L, nvlist_exists_type(nvl, name, type), idx,
	    "name not found");

	return (name);
}

static inline int
get_bool(lua_State *L, const nvlist_t *nvl)
{
	const char *name;

	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_BOOL);

		lua_pushboolean(L, nvlist_get_bool(nvl, name));
	} else {
		bool defval;

		name = luaL_checkstring(L, 2);
		luaL_checktype(L, 3, LUA_TBOOLEAN);
		defval = lua_toboolean(L, 3);

		lua_pushboolean(L, dnvlist_get_bool(nvl, name, defval));
	}
	return (1);
}

static inline int
get_number(lua_State *L, const nvlist_t *nvl)
{
	const char *name;

	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_NUMBER);

		lua_pushinteger(L, nvlist_get_number(nvl, name));
	} else {
		uint64_t defval;

		name = luaL_checkstring(L, 2);
		defval = luaL_checkinteger(L, 3);

		lua_pushinteger(L, dnvlist_get_number(nvl, name, defval));
	}
	return (1);
}

static inline int
get_string(lua_State *L, const nvlist_t *nvl)
{
	const char *name;

	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_STRING);

		lua_pushstring(L, nvlist_get_string(nvl, name));
	} else {
		const char *defval;

		name = luaL_checkstring(L, 2);
		defval = luaL_checkstring(L, 3);

		lua_pushstring(L, dnvlist_get_string(nvl, name, defval));
	}
	return (1);
}

static inline int
get_nvlist(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	const nvlist_t *value;

	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_NVLIST);

		value = nvlist_get_nvlist(nvl, name);
	} else {
		const nvlist_t *defval;

		name = luaL_checkstring(L, 2);
		defval = checkanynvlist(L, 3);

		value = dnvlist_get_nvlist(nvl, name, defval);
	}
	return (newref(L, 1, __DECONST(nvlist_t *, value),
	    CONST_NVLIST_METATABLE));
}

static inline int
get_descriptor(lua_State *L, const nvlist_t *nvl)
{
	const char *name;

	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_DESCRIPTOR);

		lua_pushinteger(L, nvlist_get_descriptor(nvl, name));
	} else {
		int defval;

		name = luaL_checkstring(L, 2);
		defval = luaL_checkinteger(L, 3);

		lua_pushinteger(L, dnvlist_get_descriptor(nvl, name, defval));
	}
	return (1);
}

static inline int
get_binary(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	const void *value;
	size_t size;

	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_BINARY);

		value = nvlist_get_binary(nvl, name, &size);
	} else {
		const void *defval;
		size_t defsize;

		name = luaL_checkstring(L, 2);
		defval = luaL_checklstring(L, 3, &defsize);

		value = dnvlist_get_binary(nvl, name, &size, defval, defsize);
	}
	lua_pushlstring(L, value, size);
	return (1);
}

static inline int
get_bool_array(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	const bool *value;
	size_t nitems;

	name = checkname(L, 2, nvl, NV_TYPE_BOOL_ARRAY);

	value = nvlist_get_bool_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushboolean(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static inline int
get_number_array(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	const uint64_t *value;
	size_t nitems;

	name = checkname(L, 2, nvl, NV_TYPE_NUMBER_ARRAY);

	value = nvlist_get_number_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushinteger(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static inline int
get_string_array(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	const char * const *value;
	size_t nitems;

	name = checkname(L, 2, nvl, NV_TYPE_STRING_ARRAY);

	value = nvlist_get_string_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushstring(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static inline int
get_nvlist_array(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	const nvlist_t * const *value;
	size_t nitems;

	name = checkname(L, 2, nvl, NV_TYPE_NVLIST_ARRAY);

	value = nvlist_get_nvlist_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		newref(L, 1, __DECONST(nvlist_t *, value[i]),
		    CONST_NVLIST_METATABLE);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static inline int
get_descriptor_array(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	const int *value;
	size_t nitems;

	name = checkname(L, 2, nvl, NV_TYPE_DESCRIPTOR_ARRAY);

	value = nvlist_get_descriptor_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushinteger(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static inline int
next(lua_State *L, const nvlist_t *nvl)
{
	const char *name;
	void *cookie;
	int type;

	if (lua_isnoneornil(L, 2)) {
		cookie = NULL;
		new(L, cookie, CNVLIST_METATABLE);
		if (lua_gettop(L) > 2) {
			lua_replace(L, 2);
		}
	} else {
		cookie = checkcookie(L, 2, CNVLIST_METATABLE);
	}
	luaL_checktype(L, 3, LUA_TNONE);

	if ((name = nvlist_next(nvl, &type, &cookie)) == NULL) {
		return (0);
	}
	setref(L, -1, 1);
	lua_pushstring(L, name);
	lua_pushinteger(L, type);
	return (3);
}

static inline int
get_parent(lua_State *L, const nvlist_t *nvl)
{
	const nvlist_t *parent;
	void *cookie;

	if ((parent = nvlist_get_parent(nvl, &cookie)) == NULL) {
		return (0);
	}
	getref(L, 1);
	newref(L, -1, __DECONST(nvlist_t *, parent), CONST_NVLIST_METATABLE);
	newref(L, 1, cookie, CNVLIST_METATABLE);
	return (2);
}

static inline int
get_array_next(lua_State *L, const nvlist_t *nvl)
{
	const nvlist_t *next;

	if ((next = nvlist_get_array_next(nvl)) == NULL) {
		return (0);
	}
	getref(L, 1);
	return (newref(L, -1, __DECONST(nvlist_t *, next),
	    CONST_NVLIST_METATABLE));
}

static inline int
get_pararr(lua_State *L, const nvlist_t *nvl)
{
	const nvlist_t *parent;
	void *cookie;

	if ((parent = nvlist_get_pararr(nvl, &cookie)) == NULL) {
		return (0);
	}
	getref(L, 1);
	newref(L, -1, __DECONST(nvlist_t *, parent), CONST_NVLIST_METATABLE);
	if (cookie == NULL) {
		return (1);
	}
	newref(L, 1, cookie, CNVLIST_METATABLE);
	return (2);
}

static int
l_nv_create(lua_State *L)
{
	nvlist_t *nvl;
	int flags;

	flags = luaL_optinteger(L, 1, 0);

	return (new(L, nvlist_create(flags), NVLIST_METATABLE));
}

static int
l_nv_recv(lua_State *L)
{
	nvlist_t *nvl;
	int sock, flags;

	sock = luaL_checkinteger(L, 1);
	flags = luaL_optinteger(L, 2, 0);

	if ((nvl = nvlist_recv(sock, flags)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, nvl, NVLIST_METATABLE));
}

static int
l_nv_unpack(lua_State *L)
{
	nvlist_t *nvl;
	const void *p;
	size_t len;
	int flags;

	p = luaL_checklstring(L, 1, &len);
	flags = luaL_optinteger(L, 2, 0);

	if ((nvl = nvlist_unpack(p, len, flags)) == NULL) {
		   return (fail(L, errno));
	}
	return (new(L, nvl, NVLIST_METATABLE));
}

static int
l_nvlist_gc(lua_State *L)
{
	nvlist_t *nvl;

	nvl = checkcookienull(L, 1, NVLIST_METATABLE);

	nvlist_destroy(nvl);
	return (0);
}

static int
l_nvlist_error(lua_State *L)
{
	const nvlist_t *nvl;
	int error;

	nvl = checkcookienull(L, 1, NVLIST_METATABLE);

	lua_pushinteger(L, nvlist_error(nvl));
	return (1);
}

static int
l_nvlist_set_error(lua_State *L)
{
	nvlist_t *nvl;
	int error;

	nvl = checkcookienull(L, 1, NVLIST_METATABLE);
	if ((error = luaL_checkinteger(L, 2)) == 0) {
		char msg[NL_TEXTMAX];

		strerror_r(EINVAL, msg, sizeof(msg));
		return (luaL_argerror(L, 2, msg));
	}

	nvlist_set_error(nvl, error);
	return (0);
}

static int
l_nvlist_empty(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checknvlist(L, 1);

	lua_pushboolean(L, nvlist_empty(nvl));
	return (1);
}

static int
l_nvlist_flags(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checknvlist(L, 1);

	lua_pushinteger(L, nvlist_flags(nvl));
	return (1);
}

static int
l_nvlist_in_array(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checkcookie(L, 1, NVLIST_METATABLE);

	lua_pushboolean(L, nvlist_in_array(nvl));
	return (1);
}

static int
l_nvlist_clone(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checknvlist(L, 1);

	return (new(L, nvlist_clone(nvl), NVLIST_METATABLE));
}

static int
l_nvlist_dump(lua_State *L)
{
	const nvlist_t *nvl;
	int fd;

	nvl = checknvlist(L, 1);
	fd = luaL_checkinteger(L, 2);

	nvlist_dump(nvl, fd);
	return (0);
}

static int
l_nvlist_fdump(lua_State *L)
{
	const nvlist_t *nvl;
	luaL_Stream *s;

	nvl = checknvlist(L, 1);
	s = luaL_checkudata(L, 2, LUA_FILEHANDLE);

	nvlist_fdump(nvl, s->f);
	return (0);
}

static int
l_nvlist_size(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checkcookie(L, 1, NVLIST_METATABLE);

	lua_pushinteger(L, nvlist_size(nvl));
	return (1);
}

static int
l_nvlist_pack(lua_State *L)
{
	const nvlist_t *nvl;
	void *p;
	size_t len;

	nvl = checknvlist(L, 1);

	if ((p = nvlist_pack(nvl, &len)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushlstring(L, p, len);
	free(p);
	return (1);
}

static int
l_nvlist_send(lua_State *L)
{
	const nvlist_t *nvl;
	int sock;

	nvl = checknvlist(L, 1);
	sock = luaL_checkinteger(L, 2);

	if (nvlist_send(sock, nvl) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_nvlist_xfer(lua_State *L)
{
	nvlist_t *tosend, *recvd;
	int sock, flags;

	tosend = checknvlist(L, 1);
	sock = luaL_checkinteger(L, 2);
	flags = luaL_optinteger(L, 3, 0);

	setcookie(L, 1, NULL);
	if ((recvd = nvlist_xfer(sock, tosend, flags)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, recvd, NVLIST_METATABLE));
}

static int
l_nvlist_exists(lua_State *L)
{
	const nvlist_t *nvl;
	const char *name;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);

	lua_pushboolean(L, nvlist_exists(nvl, name));
	return (1);
}

static int
l_nvlist_exists_type(lua_State *L)
{
	const nvlist_t *nvl;
	const char *name;
	int type;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	type = luaL_checkinteger(L, 3);

	lua_pushboolean(L, nvlist_exists_type(nvl, name, type));
	return (1);
}

#define NVLIST_EXISTS(type, _) \
static int \
l_nvlist_exists_##type(lua_State *L) \
{ \
	const nvlist_t *nvl; \
	const char *name; \
\
	nvl = checknvlist(L, 1); \
	name = luaL_checkstring(L, 2); \
\
	lua_pushboolean(L, nvlist_exists_##type(nvl, name)); \
	return (1); \
}

NVLIST_TYPES_NULL(NVLIST_EXISTS)

#undef NVLIST_EXISTS

static int
l_nvlist_add_null(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);

	nvlist_add_null(nvl, name);
	return (0);
}

static int
l_nvlist_add_bool(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	bool value;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TBOOLEAN);
	value = lua_toboolean(L, 3);

	nvlist_add_bool(nvl, name, value);
	return (0);
}

static int
l_nvlist_add_number(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	uint64_t value;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	value = luaL_checkinteger(L, 3);

	nvlist_add_number(nvl, name, value);
	return (0);
}

static int
l_nvlist_add_string(lua_State *L)
{
	nvlist_t *nvl;
	const char *name, *value;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	value = luaL_checkstring(L, 3);

	nvlist_add_string(nvl, name, value);
	return (0);
}

static int
l_nvlist_add_nvlist(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	const nvlist_t *value;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	value = checkanynvlist(L, 3);

	nvlist_add_nvlist(nvl, name, value);
	return (0);
}

static int
l_nvlist_add_descriptor(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int value;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	value = luaL_checkinteger(L, 3);

	nvlist_add_descriptor(nvl, name, value);
	return (0);
}

static int
l_nvlist_add_binary(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	const void *value;
	size_t len;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	value = luaL_checklstring(L, 3, &len);

	nvlist_add_binary(nvl, name, value, len);
	return (0);
}

static int
l_nvlist_add_bool_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	bool *value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	luaL_argcheck(L, ((nitems = luaL_len(L, 3)) > 0), 3, "cannot be empty");

	if ((value = malloc(nitems * sizeof(*value))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < nitems; i++) {
		lua_geti(L, 3, i + 1);
		value[i] = lua_toboolean(L, -1);
		lua_pop(L, 1);
	}
	nvlist_move_bool_array(nvl, name, value, nitems);
	return (0);
}

static int
l_nvlist_add_number_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	uint64_t *value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	luaL_argcheck(L, ((nitems = luaL_len(L, 3)) > 0), 3, "cannot be empty");

	if ((value = malloc(nitems * sizeof(*value))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < nitems; i++) {
		lua_geti(L, 3, i + 1);
		value[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}
	nvlist_move_number_array(nvl, name, value, nitems);
	return (0);
}

static int
l_nvlist_add_string_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	const char **value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	luaL_argcheck(L, ((nitems = luaL_len(L, 3)) > 0), 3, "cannot be empty");

	if ((value = malloc(nitems * sizeof(*value))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < nitems; i++) {
		lua_geti(L, 3, i + 1);
		value[i] = lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	nvlist_add_string_array(nvl, name, value, nitems);
	free(value);
	return (0);
}

static int
l_nvlist_add_nvlist_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	const nvlist_t **value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	luaL_argcheck(L, ((nitems = luaL_len(L, 3)) > 0), 3, "cannot be empty");

	if ((value = malloc(nitems * sizeof(*value))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < nitems; i++) {
		lua_geti(L, 3, i + 1);
		if ((value[i] = getanynvlist(L, -1)) == NULL) {
			free(value);
			return (luaL_argerror(L, 3, "nvlists expected"));
		}
		if (nvlist_error(value[i]) != 0) {
			free(value);
			return (luaL_argerror(L, 3, "nvlist has error"));
		}
		lua_pop(L, 2);
	}
	nvlist_add_nvlist_array(nvl, name, value, nitems);
	free(value);
	return (0);
}

static int
l_nvlist_add_descriptor_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int *value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	luaL_argcheck(L, ((nitems = luaL_len(L, 3)) > 0), 3, "cannot be empty");

	if ((value = malloc(nitems * sizeof(*value))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < nitems; i++) {
		lua_geti(L, 3, i + 1);
		value[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}
	nvlist_add_descriptor_array(nvl, name, value, nitems);
	free(value);
	return (0);
}

static int
l_nvlist_move_descriptor(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int value;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	value = luaL_checkinteger(L, 3);

	nvlist_move_descriptor(nvl, name, value);
	return (0);
}

static int
l_nvlist_move_nvlist(lua_State *L)
{
	nvlist_t *nvl, *value;
	const char *name;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	value = checknvlist(L, 3);

	setcookie(L, 3, NULL);
	nvlist_move_nvlist(nvl, name, value);
	return (0);
}

static int
l_nvlist_move_descriptor_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int *value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	luaL_argcheck(L, ((nitems = luaL_len(L, 3)) > 0), 3, "cannot be empty");

	if ((value = malloc(nitems * sizeof(*value))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < nitems; i++) {
		lua_geti(L, 3, i + 1);
		value[i] = lua_tointeger(L, -1);
		lua_pop(L, 1);
	}
	nvlist_move_descriptor_array(nvl, name, value, nitems);
	return (0);
}

static int
l_nvlist_move_nvlist_array(lua_State *L)
{
	nvlist_t *nvl, **value;
	const char *name;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	luaL_argcheck(L, ((nitems = luaL_len(L, 3)) > 0), 3, "cannot be empty");

	if ((value = malloc(nitems * sizeof(*value))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < nitems; i++) {
		lua_geti(L, 3, i + 1);
		if ((value[i] = getnvlist(L, -1, NVLIST_METATABLE)) == NULL) {
			while (i--) {
				nvlist_destroy(value[i]);
			}
			free(value);
			return (luaL_argerror(L, 3, "nvlists expected"));
		}
		if (nvlist_error(value[i]) != 0) {
			while (i--) {
				nvlist_destroy(value[i]);
			}
			free(value);
			return (luaL_argerror(L, 3, "nvlist has error"));
		}
		setcookie(L, -1, NULL);
		lua_pop(L, 1);
	}
	nvlist_move_nvlist_array(nvl, name, value, nitems);
	return (0);
}

#define NVLIST_GET(type, _) \
static int \
l_nvlist_get_##type(lua_State *L) \
{ \
	return (get_##type(L, checknvlist(L, 1))); \
}

NVLIST_TYPES(NVLIST_GET)

#undef NVLIST_GET

static int
l_nvlist_take_bool(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;

	nvl = checknvlist(L, 1);
	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_BOOL);

		lua_pushboolean(L, nvlist_take_bool(nvl, name));
	} else {
		bool defval;

		name = luaL_checkstring(L, 2);
		luaL_checktype(L, 3, LUA_TBOOLEAN);
		defval = lua_toboolean(L, 3);

		lua_pushboolean(L, dnvlist_take_bool(nvl, name, defval));
	}
	return (1);
}

static int
l_nvlist_take_number(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;

	nvl = checknvlist(L, 1);
	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_NUMBER);

		lua_pushinteger(L, nvlist_take_number(nvl, name));
	} else {
		uint64_t defval;

		name = luaL_checkstring(L, 2);
		defval = luaL_checkinteger(L, 3);

		lua_pushinteger(L, dnvlist_take_number(nvl, name, defval));
	}
	return (1);
}

static int
l_nvlist_take_string(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	char *value, *defval;

	nvl = checknvlist(L, 1);
	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_STRING);
		defval = NULL;

		value = nvlist_take_string(nvl, name);
	} else {
		name = luaL_checkstring(L, 2);
		defval = __DECONST(char *, luaL_checkstring(L, 3));

		value = dnvlist_take_string(nvl, name, defval);
	}
	lua_pushstring(L, value);
	if (value != defval) {
		free(value);
	}
	return (1);
}

static int
l_nvlist_take_nvlist(lua_State *L)
{
	nvlist_t *nvl, *value;
	const char *name;

	nvl = checknvlist(L, 1);
	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_NVLIST);

		value = nvlist_take_nvlist(nvl, name);
	} else {
		nvlist_t *defval;

		name = luaL_checkstring(L, 2);
		defval = checkanynvlist(L, 3);
		luaL_checktype(L, 4, LUA_TNONE);

		value = dnvlist_take_nvlist(nvl, name, defval);
		if (value == defval) {
			return (1);
		}
	}
	return (new(L, value, NVLIST_METATABLE));
}

static int
l_nvlist_take_descriptor(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;

	nvl = checknvlist(L, 1);
	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_DESCRIPTOR);

		lua_pushinteger(L, nvlist_take_descriptor(nvl, name));
	} else {
		int defval;

		name = luaL_checkstring(L, 2);
		defval = luaL_checkinteger(L, 3);

		lua_pushinteger(L, dnvlist_take_descriptor(nvl, name, defval));
	}
	return (1);
}

static int
l_nvlist_take_binary(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	void *value, *defval;
	size_t size;

	nvl = checknvlist(L, 1);
	if (lua_isnoneornil(L, 3)) {
		name = checkname(L, 2, nvl, NV_TYPE_BINARY);
		defval = NULL;

		value = nvlist_take_binary(nvl, name, &size);
	} else {
		size_t defsize;

		name = luaL_checkstring(L, 2);
		defval = __DECONST(void *, luaL_checklstring(L, 3, &defsize));

		value = dnvlist_take_binary(nvl, name, &size, defval, defsize);
	}
	lua_pushlstring(L, value, size);
	if (value != defval) {
		free(value);
	}
	return (1);
}

static int
l_nvlist_take_bool_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	bool *value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_BOOL_ARRAY);

	value = nvlist_take_bool_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushboolean(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

static int
l_nvlist_take_number_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	uint64_t *value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_NUMBER_ARRAY);

	value = nvlist_take_number_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushinteger(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

static int
l_nvlist_take_string_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	char **value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_STRING_ARRAY);

	value = nvlist_take_string_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushstring(L, value[i]);
		lua_rawseti(L, -2, i + 1);
		free(value[i]);
	}
	free(value);
	return (1);
}

static int
l_nvlist_take_nvlist_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	nvlist_t **value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_NVLIST_ARRAY);

	value = nvlist_take_nvlist_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		new(L, value[i], NVLIST_METATABLE);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

static int
l_nvlist_take_descriptor_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int *value;
	size_t nitems;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_DESCRIPTOR_ARRAY);

	value = nvlist_take_descriptor_array(nvl, name, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushinteger(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

static int
l_nvlist_append_bool_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_BOOL_ARRAY);
	luaL_argexpected(L, lua_isboolean(L, 3), 3, "boolean");

	nvlist_append_bool_array(nvl, name, lua_toboolean(L, 3));
	return (0);
}

static int
l_nvlist_append_number_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	uint64_t value;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_NUMBER_ARRAY);
	value = luaL_checkinteger(L, 3);

	nvlist_append_number_array(nvl, name, value);
	return (0);
}

static int
l_nvlist_append_string_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	const char *value;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_STRING_ARRAY);
	value = luaL_checkstring(L, 3);

	nvlist_append_string_array(nvl, name, value);
	return (0);
}

static int
l_nvlist_append_nvlist_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	const nvlist_t *value;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_NVLIST_ARRAY);
	value = checkanynvlist(L, 3);

	nvlist_append_nvlist_array(nvl, name, value);
	return (0);
}

static int
l_nvlist_append_descriptor_array(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int value;

	nvl = checknvlist(L, 1);
	name = checkname(L, 2, nvl, NV_TYPE_DESCRIPTOR_ARRAY);
	value = luaL_checkinteger(L, 3);

	nvlist_append_descriptor_array(nvl, name, value);
	return (0);
}

static int
l_nvlist_free(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;

	nvl = checkcookie(L, 1, NVLIST_METATABLE);
	name = luaL_checkstring(L, 2);
	luaL_argcheck(L, nvlist_exists(nvl, name), 2, "name not found");

	nvlist_free(nvl, name);
	return (0);
}

static int
l_nvlist_free_type(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int type;

	nvl = checknvlist(L, 1);
	type = luaL_checkinteger(L, 3);
	name = checkname(L, 2, nvl, type);

	nvlist_free_type(nvl, name, type);
	return (0);
}

#define NVLIST_FREE(type, T) \
static int \
l_nvlist_free_##type(lua_State *L) \
{ \
	nvlist_t *nvl; \
	const char *name; \
\
	nvl = checknvlist(L, 1); \
	name = checkname(L, 2, nvl, NV_TYPE_##T); \
\
	nvlist_free_##type(nvl, name); \
	return (0); \
}

NVLIST_TYPES_NULL(NVLIST_FREE)

#undef NVLIST_FREE

static int
l_nvlist_next(lua_State *L)
{
	return (next(L, checknvlist(L, 1)));
}

static int
l_nvlist_get_parent(lua_State *L)
{
	return (get_parent(L, checknvlist(L, 1)));
}

static int
l_nvlist_get_array_next(lua_State *L)
{
	return (get_array_next(L, checknvlist(L, 1)));
}

static int
l_nvlist_get_pararr(lua_State *L)
{
	return (get_pararr(L, checknvlist(L, 1)));
}

static int
l_const_nvlist_error(lua_State *L)
{
	const nvlist_t *nvl;
	int error;

	nvl = checkcookienull(L, 1, CONST_NVLIST_METATABLE);

	lua_pushinteger(L, nvlist_error(nvl));
	return (1);
}

static int
l_const_nvlist_empty(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checkconstnvlist(L, 1);

	lua_pushboolean(L, nvlist_empty(nvl));
	return (1);
}

static int
l_const_nvlist_flags(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checkconstnvlist(L, 1);

	lua_pushinteger(L, nvlist_flags(nvl));
	return (1);
}

static int
l_const_nvlist_in_array(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checkcookie(L, 1, CONST_NVLIST_METATABLE);

	lua_pushboolean(L, nvlist_in_array(nvl));
	return (1);
}

static int
l_const_nvlist_clone(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checkconstnvlist(L, 1);

	return (new(L, nvlist_clone(nvl), NVLIST_METATABLE));
}

static int
l_const_nvlist_dump(lua_State *L)
{
	const nvlist_t *nvl;
	int fd;

	nvl = checkconstnvlist(L, 1);
	fd = luaL_checkinteger(L, 2);

	nvlist_dump(nvl, fd);
	return (0);
}

static int
l_const_nvlist_fdump(lua_State *L)
{
	const nvlist_t *nvl;
	luaL_Stream *s;

	nvl = checkconstnvlist(L, 1);
	s = luaL_checkudata(L, 2, LUA_FILEHANDLE);

	nvlist_fdump(nvl, s->f);
	return (0);
}

static int
l_const_nvlist_size(lua_State *L)
{
	const nvlist_t *nvl;

	nvl = checkcookie(L, 1, CONST_NVLIST_METATABLE);

	lua_pushinteger(L, nvlist_size(nvl));
	return (1);
}

static int
l_const_nvlist_pack(lua_State *L)
{
	const nvlist_t *nvl;
	void *p;
	size_t len;

	nvl = checkconstnvlist(L, 1);

	if ((p = nvlist_pack(nvl, &len)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushlstring(L, p, len);
	free(p);
	return (1);
}

static int
l_const_nvlist_send(lua_State *L)
{
	const nvlist_t *nvl;
	int sock;

	nvl = checkconstnvlist(L, 1);
	sock = luaL_checkinteger(L, 2);

	if (nvlist_send(sock, nvl) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_const_nvlist_exists(lua_State *L)
{
	const nvlist_t *nvl;
	const char *name;

	nvl = checkconstnvlist(L, 1);
	name = luaL_checkstring(L, 2);

	lua_pushboolean(L, nvlist_exists(nvl, name));
	return (1);
}

static int
l_const_nvlist_exists_type(lua_State *L)
{
	const nvlist_t *nvl;
	const char *name;
	int type;

	nvl = checkconstnvlist(L, 1);
	name = luaL_checkstring(L, 2);
	type = luaL_checkinteger(L, 3);

	lua_pushboolean(L, nvlist_exists_type(nvl, name, type));
	return (1);
}

#define NVLIST_EXISTS(type, _) \
static int \
l_const_nvlist_exists_##type(lua_State *L) \
{ \
	const nvlist_t *nvl; \
	const char *name; \
\
	nvl = checkconstnvlist(L, 1); \
	name = luaL_checkstring(L, 2); \
\
	lua_pushboolean(L, nvlist_exists_##type(nvl, name)); \
	return (1); \
}

#define NVLIST_EXISTS(type, _) \
static int \
l_const_nvlist_exists_##type(lua_State *L) \
{ \
	const nvlist_t *nvl; \
	const char *name; \
\
	nvl = checkconstnvlist(L, 1); \
	name = luaL_checkstring(L, 2); \
\
	lua_pushboolean(L, nvlist_exists_##type(nvl, name)); \
	return (1); \
}

NVLIST_TYPES_NULL(NVLIST_EXISTS)

#undef NVLIST_EXISTS

#define NVLIST_GET(type, _) \
static int \
l_const_nvlist_get_##type(lua_State *L) \
{ \
	return (get_##type(L, checkconstnvlist(L, 1))); \
}

NVLIST_TYPES(NVLIST_GET)

#undef NVLIST_GET

static int
l_const_nvlist_next(lua_State *L)
{
	return (next(L, checkconstnvlist(L, 1)));
}

static int
l_const_nvlist_get_parent(lua_State *L)
{
	return (get_parent(L, checkconstnvlist(L, 1)));
}

static int
l_const_nvlist_get_array_next(lua_State *L)
{
	return (get_array_next(L, checkconstnvlist(L, 1)));
}

static int
l_const_nvlist_get_pararr(lua_State *L)
{
	return (get_pararr(L, checkconstnvlist(L, 1)));
}

static int
l_cnvlist_name(lua_State *L)
{
	const void *cookie;

	cookie = checkcookie(L, 1, CNVLIST_METATABLE);

	lua_pushstring(L, cnvlist_name(cookie));
	return (1);
}

static int
l_cnvlist_type(lua_State *L)
{
	const void *cookie;

	cookie = checkcookie(L, 1, CNVLIST_METATABLE);

	lua_pushinteger(L, cnvlist_type(cookie));
	return (1);
}

static inline void *
checkcookietype(lua_State *L, int idx, int type)
{
	void *cookie;

	cookie = checkcookie(L, idx, CNVLIST_METATABLE);
	lua_pop(L, 1);
	luaL_argcheck(L, cnvlist_type(cookie) == type, idx, "wrong type");

	return (cookie);
}

static int
l_cnvlist_get_bool(lua_State *L)
{
	const void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_BOOL);

	lua_pushboolean(L, cnvlist_get_bool(cookie));
	return (1);
}

static int
l_cnvlist_get_number(lua_State *L)
{
	const void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_NUMBER);

	lua_pushinteger(L, cnvlist_get_number(cookie));
	return (1);
}

static int
l_cnvlist_get_string(lua_State *L)
{
	const void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_STRING);

	lua_pushstring(L, cnvlist_get_string(cookie));
	return (1);
}

static int
l_cnvlist_get_nvlist(lua_State *L)
{
	const void *cookie;
	const nvlist_t *value;

	cookie = checkcookietype(L, 1, NV_TYPE_NVLIST);

	value = cnvlist_get_nvlist(cookie);
	return (newref(L, 1, __DECONST(nvlist_t *, value),
	    CONST_NVLIST_METATABLE));
}

static int
l_cnvlist_get_binary(lua_State *L)
{
	const void *cookie, *value;
	size_t size;

	cookie = checkcookietype(L, 1, NV_TYPE_BINARY);

	value = cnvlist_get_binary(cookie, &size);
	lua_pushlstring(L, value, size);
	return (1);
}

static int
l_cnvlist_get_bool_array(lua_State *L)
{
	const void *cookie;
	const bool *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_BOOL_ARRAY);

	value = cnvlist_get_bool_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushboolean(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_cnvlist_get_number_array(lua_State *L)
{
	const void *cookie;
	const uint64_t *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_NUMBER_ARRAY);

	value = cnvlist_get_number_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushnumber(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_cnvlist_get_string_array(lua_State *L)
{
	const void *cookie;
	const char * const *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_STRING_ARRAY);

	value = cnvlist_get_string_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushstring(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_cnvlist_get_nvlist_array(lua_State *L)
{
	const void *cookie;
	const nvlist_t * const *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_NVLIST_ARRAY);

	value = cnvlist_get_nvlist_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		newref(L, 1, __DECONST(nvlist_t *, value[i]),
		    CONST_NVLIST_METATABLE);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_cnvlist_get_descriptor(lua_State *L)
{
	const void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_DESCRIPTOR);

	lua_pushinteger(L, cnvlist_get_descriptor(cookie));
	return (1);
}

static int
l_cnvlist_get_descriptor_array(lua_State *L)
{
	const void *cookie;
	const int *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_DESCRIPTOR_ARRAY);

	value = cnvlist_get_descriptor_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushinteger(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_cnvlist_take_bool(lua_State *L)
{
	void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_BOOL);

	lua_pushboolean(L, cnvlist_take_bool(cookie));
	return (1);
}

static int
l_cnvlist_take_number(lua_State *L)
{
	void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_NUMBER);

	lua_pushinteger(L, cnvlist_take_number(cookie));
	return (1);
}

static int
l_cnvlist_take_string(lua_State *L)
{
	void *cookie;
	char *value;

	cookie = checkcookietype(L, 1, NV_TYPE_STRING);

	value = cnvlist_take_string(cookie);
	lua_pushstring(L, value);
	free(value);
	return (1);
}

static int
l_cnvlist_take_nvlist(lua_State *L)
{
	void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_NVLIST);

	return (new(L, cnvlist_take_nvlist(cookie), NVLIST_METATABLE));
}

static int
l_cnvlist_take_binary(lua_State *L)
{
	void *cookie, *value;
	size_t size;

	cookie = checkcookietype(L, 1, NV_TYPE_BINARY);

	value = cnvlist_take_binary(cookie, &size);
	lua_pushlstring(L, value, size);
	free(value);
	return (1);
}

static int
l_cnvlist_take_bool_array(lua_State *L)
{
	void *cookie;
	bool *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_BOOL_ARRAY);

	value = cnvlist_take_bool_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushboolean(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

static int
l_cnvlist_take_number_array(lua_State *L)
{
	void *cookie;
	uint64_t *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_NUMBER_ARRAY);

	value = cnvlist_take_number_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushinteger(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

static int
l_cnvlist_take_string_array(lua_State *L)
{
	void *cookie;
	char **value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_STRING_ARRAY);

	value = cnvlist_take_string_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushstring(L, value[i]);
		lua_rawseti(L, -2, i + 1);
		free(value[i]);
	}
	free(value);
	return (1);
}

static int
l_cnvlist_take_nvlist_array(lua_State *L)
{
	void *cookie;
	nvlist_t **value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_NVLIST_ARRAY);

	value = cnvlist_take_nvlist_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		new(L, value[i], NVLIST_METATABLE);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

static int
l_cnvlist_take_descriptor(lua_State *L)
{
	void *cookie;

	cookie = checkcookietype(L, 1, NV_TYPE_DESCRIPTOR);

	lua_pushinteger(L, cnvlist_take_descriptor(cookie));
	return (1);
}

static int
l_cnvlist_take_descriptor_array(lua_State *L)
{
	void *cookie;
	int *value;
	size_t nitems;

	cookie = checkcookietype(L, 1, NV_TYPE_DESCRIPTOR_ARRAY);

	value = cnvlist_take_descriptor_array(cookie, &nitems);
	lua_createtable(L, nitems, 0);
	for (int i = 0; i < nitems; i++) {
		lua_pushinteger(L, value[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(value);
	return (1);
}

#define CNVLIST_FREE(type, T) \
static int \
l_cnvlist_free_##type(lua_State *L) \
{ \
	void *cookie; \
\
	cookie = checkcookietype(L, 1, NV_TYPE_##T); \
\
	cnvlist_free_##type(cookie); \
	return (0); \
}

NVLIST_TYPES(CNVLIST_FREE)

#undef CNVLIST_FREE

static const struct luaL_Reg l_nv_funcs[] = {
	{"create", l_nv_create},
	{"recv", l_nv_recv},
	{"unpack", l_nv_unpack},
	{NULL, NULL}
};

static const struct luaL_Reg l_nvlist_meta[] = {
	{"__gc", l_nvlist_gc},
	{"error", l_nvlist_error},
	{"set_error", l_nvlist_set_error},
	{"empty", l_nvlist_empty},
	{"flags", l_nvlist_flags},
	{"in_array", l_nvlist_in_array},
	{"clone", l_nvlist_clone},
	{"dump", l_nvlist_dump},
	{"fdump", l_nvlist_fdump},
	{"size", l_nvlist_size},
	{"pack", l_nvlist_pack},
	{"send", l_nvlist_send},
	{"xfer", l_nvlist_xfer},
	{"exists", l_nvlist_exists},
#define NVLIST_EXISTS(type, _) {"exists_"#type, l_nvlist_exists_##type},
	NVLIST_TYPES_NULL(NVLIST_EXISTS)
#undef NVLIST_EXISTS
#define NVLIST_ADD(type, _) {"add_"#type, l_nvlist_add_##type},
	NVLIST_TYPES_NULL(NVLIST_ADD)
#undef NVLIST_ADD
	{"move_nvlist", l_nvlist_move_nvlist},
	{"move_descriptor", l_nvlist_move_descriptor},
	{"move_nvlist_array", l_nvlist_move_nvlist_array},
	{"move_descriptor_array", l_nvlist_move_descriptor_array},
#define NVLIST_GET(type, _) {"get_"#type, l_nvlist_get_##type},
	NVLIST_TYPES(NVLIST_GET)
#undef NVLIST_GET
#define NVLIST_TAKE(type, _) {"take_"#type, l_nvlist_take_##type},
	NVLIST_TYPES(NVLIST_TAKE)
#undef NVLIST_TAKE
	{"append_bool_array", l_nvlist_append_bool_array},
	{"append_number_array", l_nvlist_append_number_array},
	{"append_string_array", l_nvlist_append_string_array},
	{"append_nvlist_array", l_nvlist_append_nvlist_array},
	{"append_descriptor_array", l_nvlist_append_descriptor_array},
	{"free", l_nvlist_free},
	{"free_type", l_nvlist_free_type},
#define NVLIST_FREE(type, _) {"free_"#type, l_nvlist_free_##type},
	NVLIST_TYPES_NULL(NVLIST_FREE)
#undef NVLIST_FREE
	{"next", l_nvlist_next},
	{"get_parent", l_nvlist_get_parent},
	{"get_array_next", l_nvlist_get_array_next},
	{"get_pararr", l_nvlist_get_pararr},
	{NULL, NULL}
};

static const struct luaL_Reg l_const_nvlist_meta[] = {
	{"error", l_const_nvlist_error},
	{"empty", l_const_nvlist_empty},
	{"flags", l_const_nvlist_flags},
	{"in_array", l_const_nvlist_in_array},
	{"clone", l_const_nvlist_clone},
	{"dump", l_const_nvlist_dump},
	{"fdump", l_const_nvlist_fdump},
	{"size", l_const_nvlist_size},
	{"pack", l_const_nvlist_pack},
	{"send", l_const_nvlist_send},
	{"exists", l_const_nvlist_exists},
#define NVLIST_EXISTS(type, _) {"exists_"#type, l_const_nvlist_exists_##type},
	NVLIST_TYPES_NULL(NVLIST_EXISTS)
#undef NVLIST_EXISTS
#define NVLIST_GET(type, _) {"get_"#type, l_const_nvlist_get_##type},
	NVLIST_TYPES(NVLIST_GET)
#undef NVLIST_GET
	{"next", l_const_nvlist_next},
	{"get_parent", l_const_nvlist_get_parent},
	{"get_array_next", l_const_nvlist_get_array_next},
	{"get_pararr", l_const_nvlist_get_pararr},
	{NULL, NULL}
};

static const struct luaL_Reg l_cnvlist_meta[] = {
	{"name", l_cnvlist_name},
	{"type", l_cnvlist_type},
#define CNVLIST_GET(type, _) {"get_"#type, l_cnvlist_get_##type},
	NVLIST_TYPES(CNVLIST_GET)
#undef CNVLIST_GET
#define CNVLIST_TAKE(type, _) {"take_"#type, l_cnvlist_take_##type},
	NVLIST_TYPES(CNVLIST_TAKE)
#undef CNVLIST_TAKE
#define CNVLIST_FREE(type, _) {"free_"#type, l_cnvlist_free_##type},
	NVLIST_TYPES(CNVLIST_FREE)
#undef CNVLIST_FREE
	{NULL, NULL}
};

int
luaopen_nv(lua_State *L)
{
	luaL_newmetatable(L, NVLIST_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_nvlist_meta, 0);

	luaL_newmetatable(L, CONST_NVLIST_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_const_nvlist_meta, 0);

	luaL_newmetatable(L, CNVLIST_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_cnvlist_meta, 0);

	luaL_newlib(L, l_nv_funcs);

#define DEFINE(ident) ({ \
	lua_pushinteger(L, NV_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(NAME_MAX);
	DEFINE(TYPE_NONE);
	DEFINE(TYPE_BOOL);
	DEFINE(TYPE_NUMBER);
	DEFINE(TYPE_STRING);
	DEFINE(TYPE_NVLIST);
	DEFINE(TYPE_DESCRIPTOR);
	DEFINE(TYPE_BINARY);
	DEFINE(TYPE_BOOL_ARRAY);
	DEFINE(TYPE_NUMBER_ARRAY);
	DEFINE(TYPE_STRING_ARRAY);
	DEFINE(TYPE_NVLIST_ARRAY);
	DEFINE(TYPE_DESCRIPTOR_ARRAY);
	DEFINE(FLAG_IGNORE_CASE);
	DEFINE(FLAG_NO_UNIQUE);
#undef DEFINE
	return (1);
}
