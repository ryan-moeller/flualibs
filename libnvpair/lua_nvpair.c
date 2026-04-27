/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <errno.h>
#include <stdlib.h>

/*
 * Solaris types not defined in system headers.
 */
typedef unsigned int uint_t;
typedef unsigned char uchar_t;
typedef enum { B_FALSE, B_TRUE } boolean_t;
typedef uint64_t hrtime_t;

#include <libnvpair.h>

#include <lua.h>
#include <lauxlib.h>

#include "lua_nvpair.h"
#include "utils.h"

int luaopen_nvpair(lua_State *);

#define NVPAIR_SCALAR_TYPES(X) \
	X(boolean_value, boolean_t,    BOOLEAN_VALUE, lua_toboolean    , lua_pushboolean) \
	X(byte,          uchar_t,      BYTE,          luaL_checkinteger, lua_pushinteger) \
	X(int8,          int8_t,       INT8,          luaL_checkinteger, lua_pushinteger) \
	X(uint8,         uint8_t,      UINT8,         luaL_checkinteger, lua_pushinteger) \
	X(int16,         int16_t,      INT16,         luaL_checkinteger, lua_pushinteger) \
	X(uint16,        uint16_t,     UINT16,        luaL_checkinteger, lua_pushinteger) \
	X(int32,         int32_t,      INT32,         luaL_checkinteger, lua_pushinteger) \
	X(uint32,        uint32_t,     UINT32,        luaL_checkinteger, lua_pushinteger) \
	X(int64,         int64_t,      INT64,         luaL_checkinteger, lua_pushinteger) \
	X(uint64,        uint64_t,     UINT64,        luaL_checkinteger, lua_pushinteger) \
	X(string,        const char *, STRING,        luaL_checkstring,  lua_pushstring ) \
	X(nvlist,        nvlist_t *,   NVLIST,        checknvlist,       pushnvlist     ) \
	X(hrtime,        hrtime_t,     HRTIME,        luaL_checkinteger, lua_pushinteger) \
	X(double,        double,       DOUBLE,        luaL_checknumber,  lua_pushnumber )

#define NVPAIR_ARRAY_LOOKUP_TYPES(X) \
	X(boolean, boolean_t, BOOLEAN, lua_toboolean,     lua_pushboolean) \
	X(byte,    uchar_t,   BYTE,    luaL_checkinteger, lua_pushinteger) \
	X(int8,    int8_t,    INT8,    luaL_checkinteger, lua_pushinteger) \
	X(uint8,   uint8_t,   UINT8,   luaL_checkinteger, lua_pushinteger) \
	X(int16,   int16_t,   INT16,   luaL_checkinteger, lua_pushinteger) \
	X(uint16,  uint16_t,  UINT16,  luaL_checkinteger, lua_pushinteger) \
	X(int32,   int32_t,   INT32,   luaL_checkinteger, lua_pushinteger) \
	X(uint32,  uint32_t,  UINT32,  luaL_checkinteger, lua_pushinteger) \
	X(int64,   int64_t,   INT64,   luaL_checkinteger, lua_pushinteger) \
	X(uint64,  uint64_t,  UINT64,  luaL_checkinteger, lua_pushinteger)

#define NVPAIR_ARRAY_TYPES(X) \
	NVPAIR_ARRAY_LOOKUP_TYPES(X) \
	X(string, const char *,     STRING, luaL_checkstring, lua_pushstring) \
	X(nvlist, const nvlist_t *, NVLIST, checknvlist,      pushnvlist    )

static int
l_nvpair_nvlist(lua_State *L)
{
	nvlist_t *nvl;
	uint_t nvflag;
	int error;

	nvflag = luaL_optinteger(L, 1, NV_UNIQUE_NAME);
	if ((error = nvlist_alloc(&nvl, nvflag, 0)) != 0) {
		return (fatal(L, "nvlist_alloc", error));
	}
	/* TODO: optional table initializer */
	pushnvlist(L, nvl);
	return (1);
}

static int
l_nvpair_unpack(lua_State *L)
{
	nvlist_t *nvl;
	char *packed;
	size_t size;
	int error;

	packed = __DECONST(char *, luaL_checklstring(L, 1, &size));
	if ((error = nvlist_unpack(packed, size, &nvl, 0)) != 0) {
		return (fatal(L, "nvlist_unpack", error));
	}
	pushnvlist(L, nvl);
	return (1);
}

static int
l_nvlist_gc(lua_State *L)
{
	nvlist_t *nvl;

	nvl = checknvlist(L, 1);
	nvlist_free(nvl);
	setcookie(L, 1, NULL);
	return (0);
}

static inline void
pushvalue(lua_State *L, nvpair_t *nvp)
{
	int error;

	switch (nvpair_type(nvp)) {
	case DATA_TYPE_BOOLEAN:
		lua_pushboolean(L, true);
		break;
#define NVPAIR_VALUE(ftype, ctype, dtype, _lcheck, lpush) \
	case DATA_TYPE_##dtype: { \
		ctype value; \
\
		if ((error = nvpair_value_##ftype(nvp, &value)) != 0) { \
			fatal(L, "nvpair_value_"#ftype, error); \
		} \
		lpush(L, value); \
		break; \
	}
	NVPAIR_SCALAR_TYPES(NVPAIR_VALUE)
#undef NVPAIR_VALUE
#define NVPAIR_VALUE_ARRAY(ftype, ctype, dtype, _lcheck, lpush) \
	case DATA_TYPE_##dtype##_ARRAY: { \
		ctype *values; \
		uint_t len; \
\
		if ((error = nvpair_value_##ftype##_array(nvp, \
		    &values, &len)) != 0) { \
			fatal(L, "nvpair_value_"#ftype"_array", error); \
		} \
		lua_createtable(L, len, 0); \
		for (uint_t i = 0; i < len; i++) { \
			lpush(L, values[i]); \
			lua_rawseti(L, -2, i + 1); \
		} \
		break; \
	}
	NVPAIR_ARRAY_LOOKUP_TYPES(NVPAIR_VALUE_ARRAY)
	case DATA_TYPE_STRING_ARRAY: {
		const char **values;
		uint_t len;

		if ((error = nvpair_value_string_array(nvp, &values, &len))
		    != 0) {
			fatal(L, "nvpair_value_string_array", error);
		}
		lua_createtable(L, len, 0);
		for (uint_t i = 0; i < len; i++) {
			lua_pushstring(L, values[i]);
			lua_rawseti(L, -2, i + 1);
		}
		break;
	}
	case DATA_TYPE_NVLIST_ARRAY: {
		nvlist_t **values;
		uint_t len;

		if ((error = nvpair_value_nvlist_array(nvp, &values, &len))
		    != 0) {
			fatal(L, "nvpair_value_nvlist_array", error);
		}
		lua_createtable(L, len, 0);
		for (uint_t i = 0; i < len; i++) {
			/* XXX: ref protects the nvlist from gc */
			newref(L, 1, values[i], NVLIST_METATABLE);
			lua_rawseti(L, -2, i + 1);
		}
		break;
	}
#undef NVPAIR_VALUE_ARRAY
	default:
		luaL_error(L, "unexpected type");
	}
}

static int
l_nvlist_pairs_iter(lua_State *L)
{
	nvlist_t *nvl;
	nvpair_t *nvp, *next;
	int error;

	nvl = checknvlist(L, 1);
	nvp = lua_touserdata(L, 2);

	if (nvp == NULL) {
		return (0);
	}
	lua_pushlightuserdata(L, nvlist_next_nvpair(nvl, nvp));
	lua_pushstring(L, nvpair_name(nvp));
	pushvalue(L, nvp);
	lua_pushinteger(L, nvpair_type(nvp));
	return (4);
}

static int
l_nvlist_pairs(lua_State *L)
{
	nvlist_t *nvl;

	nvl = checknvlist(L, 1);

	lua_pushcfunction(L, l_nvlist_pairs_iter);
	lua_pushvalue(L, 1);
	lua_pushlightuserdata(L, nvlist_next_nvpair(nvl, NULL));
	return (3);
}

static int
l_nvlist_size(lua_State *L)
{
	nvlist_t *nvl;
	size_t size;
	int encoding, error;

	nvl = checknvlist(L, 1);
	encoding = luaL_optinteger(L, 2, NV_ENCODE_NATIVE);
	if ((error = nvlist_size(nvl, &size, encoding)) != 0) {
		return (fatal(L, "nvlist_size", error));
	}
	lua_pushinteger(L, size);
	return (1);
}

static int
l_nvlist_pack(lua_State *L)
{
	nvlist_t *nvl;
	char *packed;
	size_t size;
	int encoding, error;

	nvl = checknvlist(L, 1);
	encoding = luaL_optinteger(L, 2, NV_ENCODE_NATIVE);
	packed = NULL;
	if ((error = nvlist_pack(nvl, &packed, &size, encoding, 0)) != 0) {
		return (fatal(L, "nvlist_pack", error));
	}
	lua_pushlstring(L, packed, size);
	free(packed);
	return (1);
}

static int
l_nvlist_dup(lua_State *L)
{
	nvlist_t *nvl, *dupnvl;
	int error;

 	nvl = checknvlist(L, 1);
	if ((error = nvlist_dup(nvl, &dupnvl, 0)) != 0) {
		return (fatal(L, "nvlist_dup", error));
	}
	pushnvlist(L, dupnvl);
	return (1);
}

static int
l_nvlist_merge(lua_State *L)
{
	nvlist_t *nvla, *nvlb;
	int error;

	nvla = checknvlist(L, 1);
	nvlb = checknvlist(L, 2);
	if ((error = nvlist_merge(nvla, nvlb, 0)) != 0) {
		return (fatal(L, "nvlist_merge", error));
	}
	return (0);
}

static int
l_nvlist_nvflag(lua_State *L)
{
	nvlist_t *nvl;

	nvl = checknvlist(L, 1);
	lua_pushinteger(L, nvlist_nvflag(nvl));
	return (1);
}

static int
l_nvlist_add_boolean(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int error;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	if ((error = nvlist_add_boolean(nvl, name)) != 0) {
		return (fatal(L, "nvlist_add_boolean", error));
	}
	return (0);
}

#define NVLIST_ADD(ftype, ctype, _dtype, lcheck, _lpush) \
static int \
l_nvlist_add_##ftype(lua_State *L) \
{ \
	nvlist_t *nvl; \
	const char *name; \
	ctype value; \
	int error; \
\
	nvl = checknvlist(L, 1); \
	name = luaL_checkstring(L, 2); \
	value = lcheck(L, 3); \
	if ((error = nvlist_add_##ftype(nvl, name, value)) != 0) { \
		return (fatal(L, "nvlist_add_"#ftype, error)); \
	} \
	return (0); \
}
NVPAIR_SCALAR_TYPES(NVLIST_ADD)
#undef NVLIST_ADD

#define NVLIST_ADD_ARRAY(ftype, ctype, _dtype, lcheck, _lpush) \
static int \
l_nvlist_add_##ftype##_array(lua_State *L) \
{ \
	nvlist_t *nvl; \
	const char *name; \
	ctype *values; \
	uint_t len; \
	int error; \
\
	nvl = checknvlist(L, 1); \
	name = luaL_checkstring(L, 2); \
	luaL_checktype(L, 3, LUA_TTABLE); \
	len = luaL_len(L, 3); \
\
	values = malloc(sizeof (ctype) * len); \
	if (values == NULL) return (fatal(L, "malloc", ENOMEM)); \
	for (uint_t i = 0; i < len; i++) { \
		lua_geti(L, 3, i + 1); \
		values[i] = lcheck(L, -1); \
	} \
	error = nvlist_add_##ftype##_array(nvl, name, values, len); \
	free(values); \
	if (error != 0) { \
		return (fatal(L, "nvlist_add_"#ftype"_array", error)); \
	} \
	return (0); \
}
NVPAIR_ARRAY_TYPES(NVLIST_ADD_ARRAY)
#undef NVLIST_ADD_ARRAY

static int
l_nvlist_remove(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	data_type_t type;
	int error;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	if ((type = luaL_optinteger(L, 3, -1)) == -1) {
		if ((error = nvlist_remove_all(nvl, name)) != 0) {
			return (fatal(L, "nvlist_remove_all", error));
		}
		return (0);
	}
	if ((error = nvlist_remove(nvl, name, type)) != 0) {
		return (fatal(L, "nvlist_remove", error));
	}
	return (0);
}

static int
l_nvlist_lookup(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	nvpair_t *nvp;
	int error;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);

	if ((error = nvlist_lookup_nvpair(nvl, name, &nvp)) != 0) {
		return (0);
	}
	pushvalue(L, nvp);
	lua_pushinteger(L, nvpair_type(nvp));
	return (2);
}

static int
l_nvlist_lookup_boolean(lua_State *L)
{
	nvlist_t *nvl;
	const char *name;
	int error;

	nvl = checknvlist(L, 1);
	name = luaL_checkstring(L, 2);
	if ((error = nvlist_lookup_boolean(nvl, name)) != 0) {
		return (fatal(L, "nvlist_lookup_boolean", error));
	}
	lua_pushboolean(L, B_TRUE);
	return (1);
}

#define NVLIST_LOOKUP(ftype, ctype, _dtype, _lcheck, lpush) \
static int \
l_nvlist_lookup_##ftype(lua_State *L) \
{ \
	nvlist_t *nvl; \
	const char *name; \
	ctype value; \
	int error; \
\
	nvl = checknvlist(L, 1); \
	name = luaL_checkstring(L, 2); \
	if ((error = nvlist_lookup_##ftype(nvl, name, &value)) != 0) { \
		return (fatal(L, "nvlist_lookup_"#ftype, error)); \
	} \
	lpush(L, value); \
	return (1); \
}
NVPAIR_SCALAR_TYPES(NVLIST_LOOKUP)
#undef NVLIST_LOOKUP

#define NVLIST_LOOKUP_ARRAY(ftype, ctype, _dtype, _lcheck, lpush) \
static int \
l_nvlist_lookup_##ftype##_array(lua_State *L) \
{ \
	nvlist_t *nvl; \
	const char *name; \
	ctype *values; \
	uint_t len; \
	int error; \
\
	nvl = checknvlist(L, 1); \
	name = luaL_checkstring(L, 2); \
	if ((error = nvlist_lookup_##ftype##_array(nvl, name, &values, &len)) \
	    != 0) { \
		return (fatal(L, "nvlist_lookup_"#ftype"_array", error)); \
	} \
	lua_createtable(L, len, 0); \
	for (uint_t i = 0; i < len; i++) { \
		lpush(L, values[i]); \
		lua_rawseti(L, -2, i + 1); \
	} \
	return (1); \
}
NVPAIR_ARRAY_LOOKUP_TYPES(NVLIST_LOOKUP_ARRAY)
#undef NVLIST_LOOKUP_ARRAY

static const struct luaL_Reg l_nvpair_funcs[] = {
	{"nvlist", l_nvpair_nvlist},
	{"unpack", l_nvpair_unpack},
	{NULL, NULL}
};

static const struct luaL_Reg l_nvlist_meta[] = {
	{"__gc", l_nvlist_gc},
	{"__pairs", l_nvlist_pairs},
	{"size", l_nvlist_size},
	{"pack", l_nvlist_pack},
	{"dup", l_nvlist_dup},
	{"merge", l_nvlist_merge},
	{"nvflag", l_nvlist_nvflag},
	{"add_boolean", l_nvlist_add_boolean},
#define NVLIST_ADD(ftype, _ctype, _dtype, _lcheck, _lpush) \
	{"add_"#ftype, l_nvlist_add_##ftype},
	NVPAIR_SCALAR_TYPES(NVLIST_ADD)
#undef NVLIST_ADD
#define NVLIST_ADD_ARRAY(ftype, _ctype, _dtype, _lcheck, _lpush) \
	{"add_"#ftype"_array", l_nvlist_add_##ftype##_array},
	NVPAIR_ARRAY_TYPES(NVLIST_ADD_ARRAY)
#undef NVLIST_ADD_ARRAY
	{"remove", l_nvlist_remove},
	{"lookup", l_nvlist_lookup},
	{"lookup_boolean", l_nvlist_lookup_boolean},
#define NVLIST_LOOKUP(ftype, _ctype, _dtype, _lcheck, _lpush) \
	{"lookup_"#ftype, l_nvlist_lookup_##ftype},
	NVPAIR_SCALAR_TYPES(NVLIST_LOOKUP)
#undef NVLIST_LOOKUP
#define NVLIST_LOOKUP_ARRAY(ftype, _ctype, _dtype, _lcheck, _lpush) \
	{"lookup_"#ftype"_array", l_nvlist_lookup_##ftype##_array},
	NVPAIR_ARRAY_LOOKUP_TYPES(NVLIST_LOOKUP_ARRAY)
#undef NVLIST_LOOKUP_ARRAY
	{NULL, NULL}
};

int
luaopen_nvpair(lua_State *L)
{
	luaL_newmetatable(L, NVLIST_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_nvlist_meta, 0);

	luaL_newlib(L, l_nvpair_funcs);

#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(NV_ENCODE_NATIVE);
	DEFINE(NV_ENCODE_XDR);
	DEFINE(NV_UNIQUE_NAME);
	DEFINE(NV_UNIQUE_NAME_TYPE);

	DEFINE(DATA_TYPE_DONTCARE);
	DEFINE(DATA_TYPE_UNKNOWN);
	DEFINE(DATA_TYPE_BOOLEAN);
	DEFINE(DATA_TYPE_BYTE);
	DEFINE(DATA_TYPE_INT16);
	DEFINE(DATA_TYPE_UINT16);
	DEFINE(DATA_TYPE_INT32);
	DEFINE(DATA_TYPE_UINT32);
	DEFINE(DATA_TYPE_INT64);
	DEFINE(DATA_TYPE_UINT64);
	DEFINE(DATA_TYPE_STRING);
	DEFINE(DATA_TYPE_BYTE_ARRAY);
	DEFINE(DATA_TYPE_INT16_ARRAY);
	DEFINE(DATA_TYPE_UINT16_ARRAY);
	DEFINE(DATA_TYPE_INT32_ARRAY);
	DEFINE(DATA_TYPE_UINT32_ARRAY);
	DEFINE(DATA_TYPE_INT64_ARRAY);
	DEFINE(DATA_TYPE_UINT64_ARRAY);
	DEFINE(DATA_TYPE_STRING_ARRAY);
	DEFINE(DATA_TYPE_HRTIME);
	DEFINE(DATA_TYPE_NVLIST);
	DEFINE(DATA_TYPE_NVLIST_ARRAY);
	DEFINE(DATA_TYPE_BOOLEAN_VALUE);
	DEFINE(DATA_TYPE_INT8);
	DEFINE(DATA_TYPE_UINT8);
	DEFINE(DATA_TYPE_BOOLEAN_ARRAY);
	DEFINE(DATA_TYPE_INT8_ARRAY);
	DEFINE(DATA_TYPE_UINT8_ARRAY);
	DEFINE(DATA_TYPE_DOUBLE);
#undef DEFINE
	return (1);
}
