/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ryan Moeller <ryan-moeller@att.net>
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

/*
 * Solaris types not defined in system headers.
 */
typedef unsigned int uint_t;
typedef unsigned char uchar_t;
typedef enum { B_FALSE, B_TRUE } boolean_t;
typedef uint64_t hrtime_t;

#include <be.h>
#include <libnvpair.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

#define LIBBE_METATABLE "libbe_handle_t *"

int luaopen_be(lua_State *);

static int
l_be_init(lua_State *L)
{
	libbe_handle_t *hdl, **hdlp;
	const char *root;

	root = lua_tostring(L, 1);
	hdl = libbe_init(root);
	if (hdl == NULL) {
		luaL_error(L, "libbe_init");
	}
	hdlp = (libbe_handle_t **)lua_newuserdata(L, sizeof *hdlp);
	luaL_setmetatable(L, LIBBE_METATABLE);
	*hdlp = hdl;
	return (1);
}

static int
l_be_close(lua_State *L)
{
	libbe_handle_t **hdlp;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	if (*hdlp == NULL) {
		return (0);
	}
	libbe_close(*hdlp);
	*hdlp = NULL;
	return (0);
}

static int
l_be_active_name(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	name = be_active_name(*hdlp);
	lua_pushstring(L, name);
	return (1);
}

static int
l_be_active_path(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *path;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	path = be_active_path(*hdlp);
	lua_pushstring(L, path);
	return (1);
}

static int
l_be_nextboot_name(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	name = be_nextboot_name(*hdlp);
	lua_pushstring(L, name);
	return (1);
}

static int
l_be_nextboot_path(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *path;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	path = be_nextboot_path(*hdlp);
	lua_pushstring(L, path);
	return (1);
}

static int
l_be_root_path(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *path;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	path = be_root_path(*hdlp);
	lua_pushstring(L, path);
	return (1);
}

static void
push_nvlist(lua_State *L, nvlist_t *nvl)
{
	nvpair_t *nvp = NULL;

	lua_newtable(L);
	while ((nvp = nvlist_next_nvpair(nvl, nvp)) != NULL) {
		lua_pushstring(L, nvpair_name(nvp));
		switch (nvpair_type(nvp)) {
		case DATA_TYPE_BOOLEAN:
			lua_pushboolean(L, true);
			break;
		case DATA_TYPE_BYTE: {
			uchar_t value;

			(void) nvpair_value_byte(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_INT16: {
			int16_t value;

			(void) nvpair_value_int16(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT16: {
			uint16_t value;

			(void) nvpair_value_uint16(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_INT32: {
			int32_t value;

			(void) nvpair_value_int32(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT32: {
			uint32_t value;

			(void) nvpair_value_uint32(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_INT64: {
			int64_t value;

			(void) nvpair_value_int64(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT64: {
			uint64_t value;

			(void) nvpair_value_uint64(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_STRING: {
			const char *value;

			(void) nvpair_value_string(nvp, &value);
			lua_pushstring(L, value);
			break;
		}
		case DATA_TYPE_BYTE_ARRAY: {
			uchar_t *value;
			uint_t nelem;

			(void) nvpair_value_byte_array(nvp, &value, &nelem);
			lua_pushlstring(L, (char *)value, nelem);
			break;
		}
		case DATA_TYPE_INT16_ARRAY: {
			int16_t *value;
			uint_t nelem;

			(void) nvpair_value_int16_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_UINT16_ARRAY: {
			uint16_t *value;
			uint_t nelem;

			(void) nvpair_value_uint16_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_INT32_ARRAY: {
			int32_t *value;
			uint_t nelem;

			(void) nvpair_value_int32_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_UINT32_ARRAY: {
			uint32_t *value;
			uint_t nelem;

			(void) nvpair_value_uint32_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_INT64_ARRAY: {
			int64_t *value;
			uint_t nelem;

			(void) nvpair_value_int64_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_UINT64_ARRAY: {
			uint64_t *value;
			uint_t nelem;

			(void) nvpair_value_uint64_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_STRING_ARRAY: {
			const char **value;
			uint_t nelem;

			(void) nvpair_value_string_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushstring(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_HRTIME: {
			hrtime_t value;

			(void) nvpair_value_hrtime(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_NVLIST: {
			nvlist_t *value;

			(void) nvpair_value_nvlist(nvp, &value);
			push_nvlist(L, value);
			break;
		}
		case DATA_TYPE_NVLIST_ARRAY: {
			nvlist_t **value;
			uint_t nelem;

			(void) nvpair_value_nvlist_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				push_nvlist(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_BOOLEAN_VALUE: {
			boolean_t value;

			(void) nvpair_value_boolean_value(nvp, &value);
			lua_pushboolean(L, value);
			break;
		}
		case DATA_TYPE_INT8: {
			int8_t value;

			(void) nvpair_value_int8(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT8: {
			uint8_t value;

			(void) nvpair_value_uint8(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_BOOLEAN_ARRAY: {
			boolean_t *value;
			uint_t nelem;

			(void) nvpair_value_boolean_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushboolean(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_INT8_ARRAY: {
			int8_t *value;
			uint_t nelem;

			(void) nvpair_value_int8_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_UINT8_ARRAY: {
			uint8_t *value;
			uint_t nelem;

			(void) nvpair_value_uint8_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_DOUBLE: {
			double value;

			(void) nvpair_value_double(nvp, &value);
			lua_pushnumber(L, value);
			break;
		}
		default:
			lua_pushnil(L);
		}
		lua_rawset(L, -3);
	}
}

static int
l_be_get_bootenv_props(lua_State *L)
{
	libbe_handle_t **hdlp;
	nvlist_t *props;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	if (be_prop_list_alloc(&props) != 0) {
		luaL_error(L, "be_prop_list_alloc");
	}
	if (be_get_bootenv_props(*hdlp, props) != 0) {
		be_prop_list_free(props);
		luaL_error(L, "be_get_bootenv_props");
	}
	push_nvlist(L, props);
	be_prop_list_free(props);
	return (1);
}

static int
l_be_get_dataset_props(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	nvlist_t *props;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if (be_prop_list_alloc(&props) != 0) {
		luaL_error(L, "be_prop_list_alloc");
	}
	if (be_get_dataset_props(*hdlp, name, props) != 0) {
		be_prop_list_free(props);
		luaL_error(L, "be_get_dataset_props");
	}
	push_nvlist(L, props);
	be_prop_list_free(props);
	return (1);
}

static int
l_be_get_dataset_snapshots(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	nvlist_t *snaps;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if (be_prop_list_alloc(&snaps) != 0) {
		luaL_error(L, "be_prop_list_alloc");
	}
	if (be_get_dataset_snapshots(*hdlp, name, snaps) != 0) {
		be_prop_list_free(snaps);
		luaL_error(L, "be_get_dataset_props");
	}
	push_nvlist(L, snaps);
	be_prop_list_free(snaps);
	return (1);
}

static int
l_be_activate(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	bool temp;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	temp = lua_toboolean(L, 3);

	if (be_activate(*hdlp, name, temp) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_activate");
	}
	return (0);
}

static int
l_be_deactivate(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	bool temp;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	temp = lua_toboolean(L, 3);

	if (be_deactivate(*hdlp, name, temp) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_deactivate");
	}
	return (0);
}

static int
l_be_is_auto_snapshot_name(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	bool isauto;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	isauto = be_is_auto_snapshot_name(*hdlp, name);
	lua_pushboolean(L, isauto);
	return (1);
}

static int
l_be_create(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if (be_create(*hdlp, name) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_create");
	}
	return (0);
}

static int
l_be_create_depth(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name, *snap;
	int depth;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	snap = luaL_checkstring(L, 3);
	depth = luaL_checkinteger(L, 4);

	if (be_create_depth(*hdlp, name, snap, depth) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_create_depth");
	}
	return (0);
}

static int
l_be_create_from_existing(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name, *existing;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	existing = luaL_checkstring(L, 3);

	if (be_create_from_existing(*hdlp, name, existing) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_create_from_existing");
	}
	return (0);
}

static int
l_be_create_from_existing_snap(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name, *snap;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	snap = luaL_checkstring(L, 3);

	if (be_create_from_existing_snap(*hdlp, name, snap) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_create_from_existing_snap");
	}
	return (0);
}

static int
l_be_snapshot(lua_State *L)
{
	char result[BE_MAXPATHLEN];
	libbe_handle_t **hdlp;
	const char *source, *snap;
	bool recursive;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	source = luaL_checkstring(L, 2);
	snap = lua_tostring(L, 3);
	recursive = lua_toboolean(L, 4);

	if (be_snapshot(*hdlp, source, snap, recursive, result) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_snapshot");
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_be_rename(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *oldname, *newname;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	oldname = luaL_checkstring(L, 2);
	newname = luaL_checkstring(L, 3);

	if (be_rename(*hdlp, oldname, newname) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_rename");
	}
	return (0);
}

static int
l_be_destroy(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	int options;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	options = luaL_checkinteger(L, 3);

	if (be_destroy(*hdlp, name, options) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_destroy");
	}
	return (0);
}

static int
l_be_mount(lua_State *L)
{
	char result[BE_MAXPATHLEN];
	libbe_handle_t **hdlp;
	const char *name, *mountpoint;
	int options;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	mountpoint = lua_tostring(L, 3);
	options = luaL_checkinteger(L, 4);

	if (be_mount(*hdlp, name, mountpoint, options, result) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_mount");
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_be_unmount(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	int options;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	options = luaL_checkinteger(L, 3);

	if (be_unmount(*hdlp, name, options) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_unmount");
	}
	return (0);
}

static int
l_be_mounted_at(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *path;
	nvlist_t *details;
	bool get_details;
	int err;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	path = luaL_checkstring(L, 2);
	get_details = lua_toboolean(L, 3);

	if (!get_details) {
		err = be_mounted_at(*hdlp, path, NULL);
		if (err != 0 && err != 1) {
			/* TODO: error details */
			luaL_error(L, "be_mounted_at");
		}
		lua_pushboolean(L, err == 0);
		return (1);
	}
	if (be_prop_list_alloc(&details) != 0) {
		luaL_error(L, "be_prop_list_alloc");
	}
	err = be_mounted_at(*hdlp, path, details);
	if (err != 0 && err != 1) {
		/* TODO: error details */
		luaL_error(L, "be_mounted_at");
	}
	lua_pushboolean(L, err == 0);
	push_nvlist(L, details);
	be_prop_list_free(details);
	return (2);
}

static int
l_be_errno(lua_State *L)
{
	libbe_handle_t **hdlp;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	lua_pushinteger(L, libbe_errno(*hdlp));
	return (1);
}

static int
l_be_error_description(lua_State *L)
{
	libbe_handle_t **hdlp;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);

	lua_pushstring(L, libbe_error_description(*hdlp));
	return (1);
}

static int
l_be_print_on_error(lua_State *L)
{
	libbe_handle_t **hdlp;
	bool value;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	value = lua_toboolean(L, 2);

	libbe_print_on_error(*hdlp, value);
	return (0);
}

static int
l_be_root_concat(lua_State *L)
{
	char result[BE_MAXPATHLEN];
	libbe_handle_t **hdlp;
	const char *name;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if (be_root_concat(*hdlp, name, result) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_root_concat");
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_be_validate_name(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	int err;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	err = be_validate_name(*hdlp, name);
	lua_pushinteger(L, err);
	return (1);
}

static int
l_be_validate_snap(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *snap;
	int err;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	snap = luaL_checkstring(L, 2);

	err = be_validate_snap(*hdlp, snap);
	lua_pushinteger(L, err);
	return (1);
}

static int
l_be_exists(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	int err;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	err = be_exists(*hdlp, name);
	if (err != 0 && err != 1) {
		/* TODO: error details */
		luaL_error(L, "be_exists");
	}
	lua_pushboolean(L, err == 0);
	return (1);
}

static int
l_be_export(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	int fd;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	fd = luaL_checkinteger(L, 3);

	if (be_export(*hdlp, name, fd) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_export");
	}
	return (0);
}

static int
l_be_import(lua_State *L)
{
	libbe_handle_t **hdlp;
	const char *name;
	int fd;

	hdlp = (libbe_handle_t **)luaL_checkudata(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	fd = luaL_checkinteger(L, 3);

	if (be_import(*hdlp, name, fd) != 0) {
		/* TODO: error details */
		luaL_error(L, "be_import");
	}
	return (0);
}

static int
l_be_nicenum(lua_State *L)
{
	char result[32];
	uint64_t num;

	num = luaL_checkinteger(L, 1);

	be_nicenum(num, result, sizeof result);
	lua_pushstring(L, result);
	return (1);
}

static const struct luaL_Reg l_be_funcs[] = {
	{"init", l_be_init},
	{"nicenum", l_be_nicenum},
	{NULL, NULL}
};

static const struct luaL_Reg l_be_meta[] = {
	{"close", l_be_close},
	{"active_name", l_be_active_name},
	{"active_path", l_be_active_path},
	{"nextboot_name", l_be_nextboot_name},
	{"nextboot_path", l_be_nextboot_path},
	{"root_path", l_be_root_path},
	{"get_bootenv_props", l_be_get_bootenv_props},
	{"get_dataset_props", l_be_get_dataset_props},
	{"get_dataset_snapshots", l_be_get_dataset_snapshots},
	{"activate", l_be_activate},
	{"deactivate", l_be_deactivate},
	{"is_auto_snapshot_name", l_be_is_auto_snapshot_name},
	{"create", l_be_create},
	{"create_depth", l_be_create_depth},
	{"create_from_existing", l_be_create_from_existing},
	{"create_from_existing_snap", l_be_create_from_existing_snap},
	{"snapshot", l_be_snapshot},
	{"rename", l_be_rename},
	{"destroy", l_be_destroy},
	{"mount", l_be_mount},
	{"unmount", l_be_unmount},
	{"mounted_at", l_be_mounted_at},
	{"errno", l_be_errno},
	{"error_description", l_be_error_description},
	{"print_on_error", l_be_print_on_error},
	{"root_concat", l_be_root_concat},
	{"validate_name", l_be_validate_name},
	{"validate_snap", l_be_validate_snap},
	{"exists", l_be_exists},
	{"export", l_be_export},
	{"import", l_be_import},
	{NULL, NULL}
};

int
luaopen_be(lua_State *L)
{
	luaL_newmetatable(L, LIBBE_METATABLE);

	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, l_be_close);
	lua_setfield(L, -2, "__gc");

	luaL_setfuncs(L, l_be_meta, 0);

	luaL_newlib(L, l_be_funcs);

#define SETCONST(ident) ({ \
	lua_pushinteger(L, BE_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(MAXPATHLEN);
	SETCONST(ERR_SUCCESS);
	SETCONST(ERR_INVALIDNAME);
	SETCONST(ERR_EXISTS);
	SETCONST(ERR_NOENT);
	SETCONST(ERR_PERMS);
	SETCONST(ERR_DESTROYACT);
	SETCONST(ERR_DESTROYMNT);
	SETCONST(ERR_BADPATH);
	SETCONST(ERR_PATHBUSY);
	SETCONST(ERR_NOORIGIN);
	SETCONST(ERR_MOUNTED);
	SETCONST(ERR_NOMOUNT);
	SETCONST(ERR_ZFSOPEN);
	SETCONST(ERR_ZFSCLONE);
	SETCONST(ERR_IO);
	SETCONST(ERR_NOPOOL);
	SETCONST(ERR_NOMEM);
	SETCONST(ERR_UNKNOWN);
	SETCONST(ERR_INVORIGIN);
	SETCONST(ERR_HASCLONES);
	SETCONST(DESTROY_FORCE);
	SETCONST(DESTROY_ORIGIN);
	SETCONST(DESTROY_AUTOORIGIN);
	SETCONST(MNT_FORCE);
	SETCONST(MNT_DEEP);
#undef SETCONST
	return (1);
}
