/*
 * Copyright (c) 2025-2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

#include "luaerror.h"
#include "utils.h"

#define LIBBE_METATABLE "libbe_handle_t *"

int luaopen_be(lua_State *);

/* XXX: From be_impl.h, using because error handling in libbe is incomplete. */
int set_error(libbe_handle_t *, be_error_t);

static inline int
befail(lua_State *L, libbe_handle_t *hdl)
{
	luaL_pushfail(L);
	lua_pushstring(L, libbe_error_description(hdl));
	lua_pushinteger(L, libbe_errno(hdl));
	return (3);
}

static inline int
befailwith(lua_State *L, libbe_handle_t *hdl, be_error_t error)
{
	/*
	 * Some functions return an error without setting it.  The only way we
	 * can get error descriptions from libbe is if the error is set in the
	 * handle.  By coincidence, the internal set_error() function is
	 * externally visible.  Hopefully the former problem gets fixed before
	 * the latter.
	 */
	set_error(hdl, error);
	return (befail(L, hdl));
}

static int
l_be_init(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *root;

	root = lua_tostring(L, 1);
	hdl = libbe_init(root);
	if (hdl == NULL) {
		/* XXX: No error info is provided by libbe. */
		luaL_error(L, "libbe_init");
	}
	return (new(L, hdl, LIBBE_METATABLE));
}

static int
l_be_close(lua_State *L)
{
	libbe_handle_t *hdl;

	hdl = checkcookienull(L, 1, LIBBE_METATABLE);
	if (hdl == NULL) {
		return (0);
	}
	libbe_close(hdl);
	setcookie(L, 1, NULL);
	return (0);
}

static int
l_be_active_name(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	name = be_active_name(hdl);
	lua_pushstring(L, name);
	return (1);
}

static int
l_be_active_path(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *path;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	path = be_active_path(hdl);
	lua_pushstring(L, path);
	return (1);
}

static int
l_be_nextboot_name(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	name = be_nextboot_name(hdl);
	lua_pushstring(L, name);
	return (1);
}

static int
l_be_nextboot_path(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *path;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	path = be_nextboot_path(hdl);
	lua_pushstring(L, path);
	return (1);
}

static int
l_be_root_path(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *path;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	path = be_root_path(hdl);
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

			(void)nvpair_value_byte(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_INT16: {
			int16_t value;

			(void)nvpair_value_int16(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT16: {
			uint16_t value;

			(void)nvpair_value_uint16(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_INT32: {
			int32_t value;

			(void)nvpair_value_int32(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT32: {
			uint32_t value;

			(void)nvpair_value_uint32(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_INT64: {
			int64_t value;

			(void)nvpair_value_int64(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT64: {
			uint64_t value;

			(void)nvpair_value_uint64(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_STRING: {
			const char *value;

			(void)nvpair_value_string(nvp, &value);
			lua_pushstring(L, value);
			break;
		}
		case DATA_TYPE_BYTE_ARRAY: {
			uchar_t *value;
			uint_t nelem;

			(void)nvpair_value_byte_array(nvp, &value, &nelem);
			lua_pushlstring(L, (char *)value, nelem);
			break;
		}
		case DATA_TYPE_INT16_ARRAY: {
			int16_t *value;
			uint_t nelem;

			(void)nvpair_value_int16_array(nvp, &value, &nelem);
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

			(void)nvpair_value_uint16_array(nvp, &value, &nelem);
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

			(void)nvpair_value_int32_array(nvp, &value, &nelem);
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

			(void)nvpair_value_uint32_array(nvp, &value, &nelem);
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

			(void)nvpair_value_int64_array(nvp, &value, &nelem);
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

			(void)nvpair_value_uint64_array(nvp, &value, &nelem);
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

			(void)nvpair_value_string_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushstring(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_HRTIME: {
			hrtime_t value;

			(void)nvpair_value_hrtime(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_NVLIST: {
			nvlist_t *value;

			(void)nvpair_value_nvlist(nvp, &value);
			push_nvlist(L, value);
			break;
		}
		case DATA_TYPE_NVLIST_ARRAY: {
			nvlist_t **value;
			uint_t nelem;

			(void)nvpair_value_nvlist_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				push_nvlist(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_BOOLEAN_VALUE: {
			boolean_t value;

			(void)nvpair_value_boolean_value(nvp, &value);
			lua_pushboolean(L, value);
			break;
		}
		case DATA_TYPE_INT8: {
			int8_t value;

			(void)nvpair_value_int8(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_UINT8: {
			uint8_t value;

			(void)nvpair_value_uint8(nvp, &value);
			lua_pushinteger(L, value);
			break;
		}
		case DATA_TYPE_BOOLEAN_ARRAY: {
			boolean_t *value;
			uint_t nelem;

			(void)nvpair_value_boolean_array(nvp, &value, &nelem);
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

			(void)nvpair_value_int8_array(nvp, &value, &nelem);
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

			(void)nvpair_value_uint8_array(nvp, &value, &nelem);
			lua_newtable(L);
			for (uint_t i = 0; i < nelem; i++) {
				lua_pushinteger(L, value[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break;
		}
		case DATA_TYPE_DOUBLE: {
			double value;

			(void)nvpair_value_double(nvp, &value);
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
	libbe_handle_t *hdl;
	nvlist_t *props;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	if ((error = be_prop_list_alloc(&props)) != 0) {
		return (fatal(L, "be_prop_list_alloc", error));
	}
	if ((error = be_get_bootenv_props(hdl, props)) != 0) {
		be_prop_list_free(props);
		return (befailwith(L, hdl, error));
	}
	push_nvlist(L, props);
	be_prop_list_free(props);
	return (1);
}

static int
l_be_get_dataset_props(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	nvlist_t *props;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if ((error = be_prop_list_alloc(&props)) != 0) {
		return (fatal(L, "be_prop_list_alloc", error));
	}
	if ((error = be_get_dataset_props(hdl, name, props)) != 0) {
		be_prop_list_free(props);
		return (befailwith(L, hdl, error));
	}
	push_nvlist(L, props);
	be_prop_list_free(props);
	return (1);
}

static int
l_be_get_dataset_snapshots(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	nvlist_t *snaps;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if ((error = be_prop_list_alloc(&snaps)) != 0) {
		return (fatal(L, "be_prop_list_alloc", error));
	}
	if ((error = be_get_dataset_snapshots(hdl, name, snaps)) != 0) {
		be_prop_list_free(snaps);
		return (befailwith(L, hdl, error));
	}
	push_nvlist(L, snaps);
	be_prop_list_free(snaps);
	return (1);
}

static int
l_be_activate(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	bool temp;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	temp = lua_toboolean(L, 3);

	if (be_activate(hdl, name, temp) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_deactivate(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	bool temp;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	temp = lua_toboolean(L, 3);

	if (be_deactivate(hdl, name, temp) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_is_auto_snapshot_name(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	bool isauto;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	isauto = be_is_auto_snapshot_name(hdl, name);
	lua_pushboolean(L, isauto);
	return (1);
}

static int
l_be_create(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if (be_create(hdl, name) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_create_depth(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name, *snap;
	int depth;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	snap = luaL_checkstring(L, 3);
	depth = luaL_checkinteger(L, 4);

	if (be_create_depth(hdl, name, snap, depth) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_create_from_existing(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name, *existing;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	existing = luaL_checkstring(L, 3);

	if (be_create_from_existing(hdl, name, existing) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_create_from_existing_snap(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name, *snap;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	snap = luaL_checkstring(L, 3);

	if (be_create_from_existing_snap(hdl, name, snap) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_snapshot(lua_State *L)
{
	char result[BE_MAXPATHLEN];
	libbe_handle_t *hdl;
	const char *source, *snap;
	bool recursive;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	source = luaL_checkstring(L, 2);
	snap = lua_tostring(L, 3);
	recursive = lua_toboolean(L, 4);

	if (be_snapshot(hdl, source, snap, recursive, result) != 0) {
		return (befail(L, hdl));
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_be_rename(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *oldname, *newname;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	oldname = luaL_checkstring(L, 2);
	newname = luaL_checkstring(L, 3);

	if (be_rename(hdl, oldname, newname) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_destroy(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	int options;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	options = luaL_checkinteger(L, 3);

	if (be_destroy(hdl, name, options) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_mount(lua_State *L)
{
	char result[BE_MAXPATHLEN];
	libbe_handle_t *hdl;
	const char *name, *mountpoint;
	int options;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	mountpoint = lua_tostring(L, 3);
	options = luaL_checkinteger(L, 4);

	if (be_mount(hdl, name, mountpoint, options, result) != 0) {
		return (befail(L, hdl));
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_be_unmount(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	int options;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	options = luaL_checkinteger(L, 3);

	if (be_unmount(hdl, name, options) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_mounted_at(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *path;
	nvlist_t *details;
	bool get_details;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	path = luaL_checkstring(L, 2);
	get_details = lua_toboolean(L, 3);

	if (!get_details) {
		error = be_mounted_at(hdl, path, NULL);
		if (error != 0 && error != 1) {
			return (befail(L, hdl));
		}
		lua_pushboolean(L, error == 0);
		return (1);
	}
	if ((error = be_prop_list_alloc(&details)) != 0) {
		return (fatal(L, "be_pro_list_alloc", error));
	}
	error = be_mounted_at(hdl, path, details);
	if (error != 0 && error != 1) {
		return (befail(L, hdl));
	}
	lua_pushboolean(L, error == 0);
	push_nvlist(L, details);
	be_prop_list_free(details);
	return (2);
}

static int
l_be_errno(lua_State *L)
{
	libbe_handle_t *hdl;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	lua_pushinteger(L, libbe_errno(hdl));
	return (1);
}

static int
l_be_error_description(lua_State *L)
{
	libbe_handle_t *hdl;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);

	lua_pushstring(L, libbe_error_description(hdl));
	return (1);
}

static int
l_be_print_on_error(lua_State *L)
{
	libbe_handle_t *hdl;
	bool value;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	value = lua_toboolean(L, 2);

	libbe_print_on_error(hdl, value);
	return (0);
}

static int
l_be_root_concat(lua_State *L)
{
	char result[BE_MAXPATHLEN];
	libbe_handle_t *hdl;
	const char *name;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if ((error = be_root_concat(hdl, name, result)) != 0) {
		return (befailwith(L, hdl, error));
	}
	lua_pushstring(L, result);
	return (1);
}

static int
l_be_validate_name(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	if ((error = be_validate_name(hdl, name)) != 0) {
		return (befailwith(L, hdl, error));
	}
	return (success(L));
}

static int
l_be_validate_snap(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *snap;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	snap = luaL_checkstring(L, 2);

	if ((error = be_validate_snap(hdl, snap)) != 0) {
		return (befailwith(L, hdl, error));
	}
	return (success(L));
}

static int
l_be_exists(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	int error;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);

	error = be_exists(hdl, name);
	if (error != 0 && error != 1) {
		return (befailwith(L, hdl, error));
	}
	lua_pushboolean(L, error == 0);
	return (1);
}

static int
l_be_export(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	int fd;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	fd = checkfd(L, 3);

	if (be_export(hdl, name, fd) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_import(lua_State *L)
{
	libbe_handle_t *hdl;
	const char *name;
	int fd;

	hdl = checkcookie(L, 1, LIBBE_METATABLE);
	name = luaL_checkstring(L, 2);
	fd = checkfd(L, 3);

	if (be_import(hdl, name, fd) != 0) {
		return (befail(L, hdl));
	}
	return (success(L));
}

static int
l_be_nicenum(lua_State *L)
{
	char result[32];
	uint64_t num;

	num = luaL_checkinteger(L, 1);

	be_nicenum(num, result, sizeof(result));
	lua_pushstring(L, result);
	return (1);
}

static const struct luaL_Reg l_be_funcs[] = {
	{"init", l_be_init},
	{"nicenum", l_be_nicenum},
	{NULL, NULL}
};

static const struct luaL_Reg l_be_meta[] = {
	{"__close", l_be_close},
	{"__gc", l_be_close},
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
	luaL_setfuncs(L, l_be_meta, 0);

	luaL_newlib(L, l_be_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, BE_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(MAXPATHLEN);
	DEFINE(ERR_SUCCESS);
	DEFINE(ERR_INVALIDNAME);
	DEFINE(ERR_EXISTS);
	DEFINE(ERR_NOENT);
	DEFINE(ERR_PERMS);
	DEFINE(ERR_DESTROYACT);
	DEFINE(ERR_DESTROYMNT);
	DEFINE(ERR_BADPATH);
	DEFINE(ERR_PATHBUSY);
	DEFINE(ERR_NOORIGIN);
	DEFINE(ERR_MOUNTED);
	DEFINE(ERR_NOMOUNT);
	DEFINE(ERR_ZFSOPEN);
	DEFINE(ERR_ZFSCLONE);
	DEFINE(ERR_IO);
	DEFINE(ERR_NOPOOL);
	DEFINE(ERR_NOMEM);
	DEFINE(ERR_UNKNOWN);
	DEFINE(ERR_INVORIGIN);
	DEFINE(ERR_HASCLONES);
	DEFINE(DESTROY_FORCE);
	DEFINE(DESTROY_ORIGIN);
	DEFINE(DESTROY_AUTOORIGIN);
	DEFINE(MNT_FORCE);
	DEFINE(MNT_DEEP);
#undef DEFINE
	return (1);
}
