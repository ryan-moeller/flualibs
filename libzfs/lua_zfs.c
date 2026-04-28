/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/zfs_context.h>
#include <sys/mnttab.h>
#include <sys/zfs_ioctl.h>
#include <errno.h>
#include <libnvpair.h>
#include <libzfs.h>

#include <lua.h>
#include <lauxlib.h>

#include "../libnvpair/lua_nvpair.h"
#include "utils.h"

#define	ZFS_HANDLE_METATABLE "zfs_handle_t"
#define	ZPOOL_HANDLE_METATABLE "zpool_handle_t"
#define	LIBZFS_HANDLE_METATABLE "libzfs_handle_t"
#define	ZFS_PROP_METATABLE "zfs_prop_t"

int luaopen_zfs(lua_State *);

static inline int
libzfsfail(lua_State *L, libzfs_handle_t *hdl, int error, const char *message)
{
	/*
	 * XXX: libzfs handles are not very ergonomic.  They are designed for
	 * single-use and handle errors in unpredictable ways.  This is one
	 * motivation for eventually replacing libzfs.
	 */
	if (libzfs_errno(hdl) == 0) {
		zfs_standard_error(hdl, error, message);
	}
	luaL_pushfail(L);
	lua_pushfstring(L, "%s: %s", libzfs_error_action(hdl),
	    libzfs_error_description(hdl));
	lua_pushinteger(L, libzfs_errno(hdl));
	return (3);
}

static inline int
zfsfail(lua_State *L, zfs_handle_t *zhp, int error, const char *message)
{
	return (libzfsfail(L, zfs_get_handle(zhp), error, message));
}

static inline int
zpoolfail(lua_State *L, zpool_handle_t *zhp, int error, const char *message)
{
	return (libzfsfail(L, zpool_get_handle(zhp), error, message));
}

static inline libzfs_handle_t *
checklibzfs(lua_State *L, int idx)
{
	return (checkcookie(L, idx, LIBZFS_HANDLE_METATABLE));
}

static inline zfs_handle_t *
checkzfs(lua_State *L, int idx)
{
	return (checkcookie(L, idx, ZFS_HANDLE_METATABLE));
}

static inline zpool_handle_t *
checkzpool(lua_State *L, int idx)
{
	return (checkcookie(L, idx, ZPOOL_HANDLE_METATABLE));
}

static inline void
pushprop(lua_State *L, zfs_prop_t prop)
{
	lua_newuserdatauv(L, 0, 1);
	lua_pushinteger(L, prop);
	lua_setiuservalue(L, -2, 1);
	luaL_setmetatable(L, ZFS_PROP_METATABLE);
}

static inline zfs_prop_t
checkprop(lua_State *L, int idx)
{
	zfs_prop_t prop;

	if (lua_isstring(L, idx)) {
		const char *propname = lua_tostring(L, idx);

		return (zfs_name_to_prop(propname));
	}
	if (lua_isinteger(L, idx)) {
		return (lua_tointeger(L, idx));
	}
	lua_getiuservalue(L, idx, 1);
	if (lua_isinteger(L, -1)) {
		prop = lua_tointeger(L, -1);
	} else {
		prop = ZPROP_INVAL;
	}
	lua_pop(L, 1);
	return (prop);
}

static int
l_zfs_name_to_prop(lua_State *L)
{
	const char *name;
	zfs_prop_t prop;

	name = luaL_checkstring(L, 1);

	if ((prop = zfs_name_to_prop(name)) == ZPROP_INVAL) {
		return (0);
	}
	lua_pushinteger(L, prop);
	return (1);
}

static int
l_libzfs_init(lua_State *L)
{
	libzfs_handle_t *hdl;

	if ((hdl = libzfs_init()) == NULL) {
		int error = errno;
		luaL_pushfail(L);
		lua_pushfstring(L, "libzfs_init: %s", libzfs_error_init(error));
		lua_pushinteger(L, error);
		return (3);
	}
	return (new(L, hdl, LIBZFS_HANDLE_METATABLE));
}

static int
l_libzfs_fini(lua_State *L)
{
	libzfs_handle_t *hdl;

	hdl = checkcookienull(L, 1, LIBZFS_HANDLE_METATABLE);

	if (hdl != NULL) {
		libzfs_fini(hdl);
		setcookie(L, 1, NULL);
	}
	return (0);
}

static int
l_libzfs_print_on_error(lua_State *L)
{
	libzfs_handle_t *hdl;
	boolean_t printerr;

	hdl = checklibzfs(L, 1);
	printerr = lua_toboolean(L, 2);

	libzfs_print_on_error(hdl, printerr);
	return (0);
}

/*
 * XXX: This function can only be called once after the handle has successfully
 * performed a command that allows logging.
 */
static int
l_zpool_log_history(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *message;
	int error;

	hdl = checklibzfs(L, 1);
	message = luaL_checkstring(L, 2);

	if ((error = zpool_log_history(hdl, message)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_libzfs_errno(lua_State *L)
{
	libzfs_handle_t *hdl;

	hdl = checklibzfs(L, 1);

	lua_pushinteger(L, libzfs_errno(hdl));
	return (1);
}

static int
l_libzfs_error_action(lua_State *L)
{
	libzfs_handle_t *hdl;

	hdl = checklibzfs(L, 1);

	lua_pushstring(L, libzfs_error_action(hdl));
	return (1);
}

static int
l_libzfs_error_description(lua_State *L)
{
	libzfs_handle_t *hdl;

	hdl = checklibzfs(L, 1);

	lua_pushstring(L, libzfs_error_description(hdl));
	return (1);
}

static int
l_libzfs_mnttab_cache(lua_State *L)
{
	libzfs_handle_t *hdl;
	boolean_t enable;

	hdl = checklibzfs(L, 1);
	enable = lua_toboolean(L, 2);

	libzfs_mnttab_cache(hdl, enable);
	return (0);
}

static int
l_libzfs_mnttab_find(lua_State *L)
{
	struct mnttab entry;
	libzfs_handle_t *hdl;
	const char *fsname;
	int error;

	hdl = checklibzfs(L, 1);
	fsname = luaL_checkstring(L, 2);

	if ((error = libzfs_mnttab_find(hdl, fsname, &entry)) != 0) {
		return (libzfsfail(L, hdl, error, "libzfs_mnttab_find"));
	}
	lua_newtable(L);
	lua_pushstring(L, entry.mnt_special);
	lua_setfield(L, -2, "special");
	lua_pushstring(L, entry.mnt_mountp);
	lua_setfield(L, -2, "mountp");
	lua_pushstring(L, entry.mnt_fstype);
	lua_setfield(L, -2, "fstype");
	lua_pushstring(L, entry.mnt_mntopts);
	lua_setfield(L, -2, "mntopts");
	return (1);
}

static int
l_libzfs_mnttab_add(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *special, *mountp, *mntopts;

	hdl = checklibzfs(L, 1);
	special = luaL_checkstring(L, 2);
	mountp = luaL_checkstring(L, 3);
	mntopts = luaL_checkstring(L, 4);

	libzfs_mnttab_add(hdl, special, mountp, mntopts);
	return (0);
}

static int
l_libzfs_mnttab_remove(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *fsname;

	hdl = checklibzfs(L, 1);
	fsname = luaL_checkstring(L, 2);

	libzfs_mnttab_remove(hdl, fsname);
	return (0);
}

static int
l_zpool_open(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *pool;
	zpool_handle_t *zhp;

	hdl = checklibzfs(L, 1);
	pool = luaL_checkstring(L, 2);

	if ((zhp = zpool_open(hdl, pool)) == NULL) {
		return (libzfsfail(L, hdl, libzfs_errno(hdl), "zpool_open"));
	}
	return (new(L, zhp, ZPOOL_HANDLE_METATABLE));
}

static int
l_zpool_open_canfail(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *pool;
	zpool_handle_t *zhp;

	hdl = checklibzfs(L, 1);
	pool = luaL_checkstring(L, 2);

	if ((zhp = zpool_open_canfail(hdl, pool)) == NULL) {
		return (libzfsfail(L, hdl, libzfs_errno(hdl),
		    "zpool_open_canfail"));
	}
	return (new(L, zhp, ZPOOL_HANDLE_METATABLE));
}

static int
zpool_iter_cb(zpool_handle_t *zhp, void *data)
{
	lua_State *L = data;
	int nargs = lua_gettop(L) - 1;
	int error;

	/* Copy the function. */
	lua_pushvalue(L, -nargs); /* hdl, cb, ..., cb */
	new(L, zhp, ZPOOL_HANDLE_METATABLE); /* hdl, cb, ..., cb, zhp */
	/* Copy any additional args. */
	for (int i = 1; i < nargs; i++) {
		lua_pushvalue(L, -nargs);
	}
	if ((error = lua_pcall(L, nargs, 0, 0)) != LUA_OK) {
		luaL_pushfail(L);
		lua_insert(L, -2);
		return (-1);
	}
	return (0);
}

static int
l_zpool_iter(lua_State *L)
{
	libzfs_handle_t *hdl;
	int top = lua_gettop(L);
	int error;

	hdl = checklibzfs(L, 1);
	luaL_argcheck(L, lua_isfunction(L, 2), 2, "callback function required");

	if ((error = zpool_iter(hdl, zpool_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		luaL_pushfail(L);
		lua_pushliteral(L, "zpool_iter failed");
		return (2);
	}
	return (success(L));
}

static int
l_zpool_create(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *poolname;
	nvlist_t *nvroot, *props, *fsprops;
	int error;

	hdl = checklibzfs(L, 1);
	poolname = luaL_checkstring(L, 2);
	nvroot = checknvlist(L, 3);
	props = optnvlist(L, 4, NULL);
	fsprops = optnvlist(L, 5, NULL);

	if ((error = zpool_create(hdl, poolname, nvroot, props, fsprops))
	    != 0) {
		return (libzfsfail(L, hdl, error, "zpool_create"));
	}
	return (success(L));
}

static int
l_zfs_open(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *path;
	zfs_handle_t *zhp;
	int types;

	hdl = checklibzfs(L, 1);
	path = luaL_checkstring(L, 2);
	types = luaL_checkinteger(L, 3);

	if ((zhp = zfs_open(hdl, path, types)) == NULL) {
		return (libzfsfail(L, hdl, libzfs_errno(hdl), "zfs_open"));
	}
	return (new(L, zhp, ZFS_HANDLE_METATABLE));
}

static int
l_zfs_valid_proplist(lua_State *L)
{
	libzfs_handle_t *hdl;
	zfs_handle_t *zhp;
	zpool_handle_t *pool_hdl;
	nvlist_t *props, *ret;
	const char *errbuf;
	zfs_type_t type;
	boolean_t zoned, key_params_ok;

	hdl = checklibzfs(L, 1);
	type = luaL_checkinteger(L, 2);
	props = checknvlist(L, 3);
	zoned = lua_toboolean(L, 4);
	zhp = luaL_opt(L, checkzfs, 5, NULL);
	pool_hdl = luaL_opt(L, checkzpool, 6, NULL);
	key_params_ok = lua_toboolean(L, 7);
	errbuf = luaL_checkstring(L, 8);

	if ((ret = zfs_valid_proplist(hdl, type, props, zoned, zhp, pool_hdl,
	    key_params_ok, errbuf)) == NULL) {
		return (libzfsfail(L, hdl, libzfs_errno(hdl),
		    "zfs_valid_proplist"));
	}
	pushnvlist(L, ret);
	return (1);
}

static int
l_zfs_close(lua_State *L)
{
	zfs_handle_t *zhp;

	zhp = checkcookienull(L, 1, ZFS_HANDLE_METATABLE);

	if (zhp != NULL) {
		zfs_close(zhp);
		setcookie(L, 1, NULL);
	}
	return (0);
}

static int
l_zfs_handle_dup(lua_State *L)
{
	zfs_handle_t *zhp, *dupzhp;

	zhp = checkzfs(L, 1);

	if ((dupzhp = zfs_handle_dup(zhp)) == NULL) {
		return (zfsfail(L, zhp, libzfs_errno(zfs_get_handle(zhp)),
		    "zfs_handle_dup"));
	}
	return (new(L, dupzhp, ZFS_HANDLE_METATABLE));
}

static int
l_zfs_get_type(lua_State *L)
{
	zfs_handle_t *zhp;

	zhp = checkzfs(L, 1);

	lua_pushinteger(L, zfs_get_type(zhp));
	return (1);
}

static int
l_zfs_get_underlying_type(lua_State *L)
{
	zfs_handle_t *zhp;

	zhp = checkzfs(L, 1);

	lua_pushinteger(L, zfs_get_underlying_type(zhp));
	return (1);
}

static int
l_zfs_get_name(lua_State *L)
{
	zfs_handle_t *zhp;

	zhp = checkzfs(L, 1);

	lua_pushstring(L, zfs_get_name(zhp));
	return (1);
}

#if 0
static int
l_zfs_get_pool_handle(lua_State *L)
{
	zfs_handle_t *zhp;
	zpool_handle_t *pool_hdl;

	zhp = checkzfs(L, 1);

	if ((pool_hdl = zfs_get_pool_handle(zhp)) == NULL) {
		return (zfsfail(L, zhp, libzfs_errno(zfs_get_handle(zhp)),
		    "zfs_get_pool_handle"));
	}
	/* TODO: need a refcounted handle */
	return (new(L, pool_hdl, ZPOOL_HANDLE_METATABLE));
}
#endif

static int
l_zfs_get_pool_name(lua_State *L)
{
	zfs_handle_t *zhp;

	zhp = checkzfs(L, 1);

	lua_pushstring(L, zfs_get_pool_name(zhp));
	return (1);
}

static int
l_zfs_prop_set(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *propname, *propval;
	int error;

	zhp = checkzfs(L, 1);
	propname = luaL_checkstring(L, 2);
	propval = luaL_checkstring(L, 3);

	if ((error = zfs_prop_set(zhp, propname, propval)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_set"));
	}
	return (success(L));
}

static int
l_zfs_prop_set_list(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *props;
	int error;

	zhp = checkzfs(L, 1);
	props = checknvlist(L, 2);

	if ((error = zfs_prop_set_list(zhp, props)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_set_list"));
	}
	return (success(L));
}

static int
l_zfs_prop_set_list_flags(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *props;
	int flags, error;

	zhp = checkzfs(L, 1);
	props = checknvlist(L, 2);
	flags = luaL_optinteger(L, 3, 0);

	if ((error = zfs_prop_set_list_flags(zhp, props, flags)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_set_list_flags"));
	}
	return (success(L));
}

static int
l_zfs_prop_get(lua_State *L)
{
	char property[ZFS_MAXPROPLEN];
	char sourcename[ZFS_MAX_DATASET_NAME_LEN];
	zfs_handle_t *zhp;
	zfs_prop_t prop;
	zprop_source_t source;
	boolean_t literal;
	int error;

	property[0] = '\0';
	sourcename[0] = '\0';

	zhp = checkzfs(L, 1);
	prop = checkprop(L, 2);
	literal = lua_toboolean(L, 3);

	if ((error = zfs_prop_get(zhp, prop, property, sizeof(property),
	    &source, sourcename, sizeof(sourcename), literal)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_get"));
	}
	lua_pushstring(L, property);
	lua_pushinteger(L, source);
	if (*sourcename == '\0') {
		return (2);
	}
	lua_pushstring(L, sourcename);
	return (3);
}

static int
l_zfs_prop_get_recvd(lua_State *L)
{
	char property[ZFS_MAXPROPLEN];
	zfs_handle_t *zhp;
	const char *propname;
	boolean_t literal;
	int error;

	property[0] = '\0';

	zhp = checkzfs(L, 1);
	propname = luaL_checkstring(L, 2);
	literal = lua_toboolean(L, 3);

	if ((error = zfs_prop_get_recvd(zhp, propname, property,
	    sizeof(property), literal)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_get_recvd"));
	}
	lua_pushstring(L, property);
	return (1);
}

static int
l_zfs_prop_get_numeric(lua_State *L)
{
	char sourcename[ZFS_MAX_DATASET_NAME_LEN];
	zfs_handle_t *zhp;
	zfs_prop_t prop;
	uint64_t value;
	zprop_source_t source;
	int error;

	zhp = checkzfs(L, 1);
	prop = checkprop(L, 2);

	if ((error = zfs_prop_get_numeric(zhp, prop, &value, &source,
	    sourcename, sizeof(sourcename))) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_get_numeric"));
	}
	lua_pushinteger(L, value);
	lua_pushinteger(L, source);
	if (*sourcename == '\0') {
		return (2);
	}
	lua_pushstring(L, sourcename);
	return (3);
}

static int
l_zfs_prop_get_userquota_int(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *propname;
	uint64_t value;
	int error;

	zhp = checkzfs(L, 1);
	propname = luaL_checkstring(L, 2);

	if ((error = zfs_prop_get_userquota_int(zhp, propname, &value)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_get_userquota_int"));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_zfs_prop_get_userquota(lua_State *L)
{
	char value[ZFS_MAXPROPLEN];
	zfs_handle_t *zhp;
	const char *propname;
	boolean_t literal;
	int error;

	zhp = checkzfs(L, 1);
	propname = luaL_checkstring(L, 2);
	literal = lua_toboolean(L, 3);

	if ((error = zfs_prop_get_userquota(zhp, propname, value, sizeof(value),
	    literal)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_get_userquota"));
	}
	lua_pushstring(L, value);
	return (1);
}

static int
l_zfs_prop_get_written_int(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *propname;
	uint64_t value;
	int error;

	zhp = checkzfs(L, 1);
	propname = luaL_checkstring(L, 2);

	if ((error = zfs_prop_get_written_int(zhp, propname, &value)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_get_written_int"));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_zfs_prop_get_written(lua_State *L)
{
	char value[ZFS_MAXPROPLEN];
	zfs_handle_t *zhp;
	const char *propname;
	boolean_t literal;
	int error;

	zhp = checkzfs(L, 1);
	propname = luaL_checkstring(L, 2);
	literal = lua_toboolean(L, 3);

	if ((error = zfs_prop_get_written(zhp, propname, value, sizeof(value),
	    literal)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_get_written"));
	}
	lua_pushstring(L, value);
	return (1);
}

static int
l_getprop_uint64(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *source;
	zfs_prop_t prop;

	zhp = checkzfs(L, 1);
	prop = checkprop(L, 2);

	lua_pushinteger(L, getprop_uint64(zhp, prop, &source));
	lua_pushstring(L, source);
	return (2);
}

static int
l_zfs_prop_get_int(lua_State *L)
{
	zfs_handle_t *zhp;
	zfs_prop_t prop;

	zhp = checkzfs(L, 1);
	prop = checkprop(L, 2);

	lua_pushinteger(L, zfs_prop_get_int(zhp, prop));
	return (1);
}

static int
l_zfs_prop_inherit(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *propname;
	boolean_t received;
	int error;

	zhp = checkzfs(L, 1);
	propname = luaL_checkstring(L, 2);
	received = lua_toboolean(L, 3);

	if ((error = zfs_prop_inherit(zhp, propname, received)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_prop_inherit"));
	}
	return (success(L));
}

static int
l_zfs_get_all_props(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *props;
	int error;

	zhp = checkzfs(L, 1);

	/* XXX: Need to dup to have sane lifetime. */
	if ((error = nvlist_dup(zfs_get_all_props(zhp), &props, 0)) != 0) {
		return (fatal(L, "nvlist_dup", error));
	}
	pushnvlist(L, props);
	return (1);
}

static int
l_zfs_get_user_props(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *props;
	int error;

	zhp = checkzfs(L, 1);

	/* XXX: Need to dup to have sane lifetime. */
	if ((error = nvlist_dup(zfs_get_user_props(zhp), &props, 0)) != 0) {
		return (fatal(L, "nvlist_dup", error));
	}
	pushnvlist(L, props);
	return (1);
}

static int
l_zfs_get_recvd_props(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *props;
	int error;

	zhp = checkzfs(L, 1);

	if ((props = zfs_get_recvd_props(zhp)) == NULL) {
		return (zfsfail(L, zhp, libzfs_errno(zfs_get_handle(zhp)),
		    "zfs_get_recvd_props"));
	}
	/* XXX: Need to dup to have sane lifetime. */
	if ((error = nvlist_dup(props, &props, 0)) != 0) {
		return (fatal(L, "nvlist_dup", error));
	}
	pushnvlist(L, props);
	return (1);
}

static int
l_zfs_get_clones_nvl(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *clones;

	zhp = checkzfs(L, 1);

	if ((clones = zfs_get_clones_nvl(zhp)) == NULL) {
		/* XXX: We don't get useful error info back. */
		return (zfsfail(L, zhp, EZFS_UNKNOWN, "zfs_get_clones_nvl"));
	}
	pushnvlist(L, clones);
	return (1);
}

static int
l_zpool_close(lua_State *L)
{
	zpool_handle_t *zhp;

	zhp = checkcookienull(L, 1, ZPOOL_HANDLE_METATABLE);

	if (zhp != NULL) {
		zpool_close(zhp);
		setcookie(L, 1, NULL);
	}
	return (0);
}

static int
l_zpool_get_name(lua_State *L)
{
	zpool_handle_t *zhp;

	zhp = checkzpool(L, 1);

	lua_pushstring(L, zpool_get_name(zhp));
	return (1);
}

static int
l_zpool_get_state(lua_State *L)
{
	zpool_handle_t *zhp;

	zhp = checkzpool(L, 1);

	lua_pushinteger(L, zpool_get_state(zhp));
	return (1);
}

static int
l_zfs_prop_default_string(lua_State *L)
{
	const char *string;
	zfs_prop_t prop;

	prop = checkprop(L, 1);

	if ((string = zfs_prop_default_string(prop)) == NULL) {
		return (0);
	}
	lua_pushstring(L, string);
	return (1);
}

static int
l_zfs_prop_default_numeric(lua_State *L)
{
	zfs_prop_t prop;

	prop = checkprop(L, 1);

	lua_pushinteger(L, zfs_prop_default_numeric(prop));
	return (1);
}

static int
l_zfs_prop_column_name(lua_State *L)
{
	zfs_prop_t prop;

	prop = checkprop(L, 1);

	lua_pushstring(L, zfs_prop_column_name(prop));
	return (1);
}

static int
l_zfs_prop_align_right(lua_State *L)
{
	zfs_prop_t prop;

	prop = checkprop(L, 1);

	lua_pushboolean(L, zfs_prop_align_right(prop));
	return (1);
}

static int
l_zfs_prop_to_name(lua_State *L)
{
	zfs_prop_t prop;

	prop = checkprop(L, 1);

	lua_pushstring(L, zfs_prop_to_name(prop));
	return (1);
}

static int
l_zfs_prop_values(lua_State *L)
{
	const char *values;
	zfs_prop_t prop;

	prop = checkprop(L, 1);

	if ((values = zfs_prop_values(prop)) == NULL) {
		return (0);
	}
	lua_pushstring(L, values);
	return (1);
}

static int
l_zfs_prop_is_string(lua_State *L)
{
	zfs_prop_t prop;

	prop = checkprop(L, 1);

	lua_pushboolean(L, zfs_prop_is_string(prop));
	return (1);
}

static const struct luaL_Reg l_zfs_funcs[] = {
	{"init", l_libzfs_init},
	{"prop", l_zfs_name_to_prop},
	{NULL, NULL}
};

static const struct luaL_Reg l_zfs_meta[] = {
	{"__close", l_zfs_close},
	{"__gc", l_zfs_close},
	{"close", l_zfs_close},
	{"handle_dup", l_zfs_handle_dup},
	{"get_type", l_zfs_get_type},
	{"get_underlying_type", l_zfs_get_underlying_type},
	{"get_name", l_zfs_get_name},
#if 0
	{"get_pool_handle", l_zfs_get_pool_handle},
#endif
	{"get_pool_name", l_zfs_get_pool_name},
	{"prop_set", l_zfs_prop_set},
	{"prop_set_list", l_zfs_prop_set_list},
	{"prop_set_list_flags", l_zfs_prop_set_list_flags},
	{"prop_get", l_zfs_prop_get},
	{"prop_get_recvd", l_zfs_prop_get_recvd},
	{"prop_get_numeric", l_zfs_prop_get_numeric},
	{"prop_get_userquota_int", l_zfs_prop_get_userquota_int},
	{"prop_get_userquota", l_zfs_prop_get_userquota},
	{"prop_get_written_int", l_zfs_prop_get_written_int},
	{"prop_get_written", l_zfs_prop_get_written},
#if 0 /* XXX: not implemented in libzfs */
	{"prop_get_feature", l_zfs_prop_get_feature},
#endif
	{"getprop_uint64", l_getprop_uint64},
	{"prop_get_int", l_zfs_prop_get_int},
	{"prop_inherit", l_zfs_prop_inherit},
	{"get_all_props", l_zfs_get_all_props},
	{"get_user_props", l_zfs_get_user_props},
	{"get_recvd_props", l_zfs_get_recvd_props},
	{"get_clones_nvl", l_zfs_get_clones_nvl},
	/* TODO: so much more */
	{NULL, NULL}
};

static const struct luaL_Reg l_zpool_meta[] = {
	{"__close", l_zpool_close},
	{"__gc", l_zpool_close},
	{"close", l_zpool_close},
	{"get_name", l_zpool_get_name},
	{"get_state", l_zpool_get_state},
	/* TODO: lots more */
	{NULL, NULL}
};

static const struct luaL_Reg l_libzfs_meta[] = {
	{"__close", l_libzfs_fini},
	{"__gc", l_libzfs_fini},
	{"fini", l_libzfs_fini},
	{"print_on_error", l_libzfs_print_on_error},
	{"log_history", l_zpool_log_history},
	{"errno", l_libzfs_errno},
	{"error_action", l_libzfs_error_action},
	{"error_description", l_libzfs_error_description},
	{"mnttab_cache", l_libzfs_mnttab_cache},
	{"mnttab_find", l_libzfs_mnttab_find},
	{"mnttab_add", l_libzfs_mnttab_add},
	{"mnttab_remove", l_libzfs_mnttab_remove},
	{"zpool_open", l_zpool_open},
	{"zpool_open_canfail", l_zpool_open_canfail},
	{"zpool_iter", l_zpool_iter},
	{"zpool_create", l_zpool_create},
	/* TODO: lots more stuff */
	{"zfs_open", l_zfs_open},
	{"valid_proplist", l_zfs_valid_proplist},
	{NULL, NULL}
};

static const struct luaL_Reg l_prop_meta[] = {
	{"default_string", l_zfs_prop_default_string},
	{"default_numeric", l_zfs_prop_default_numeric},
	{"column_name", l_zfs_prop_column_name},
	{"align_right", l_zfs_prop_align_right},
	{"to_name", l_zfs_prop_to_name},
	{"values", l_zfs_prop_values},
	{"is_string", l_zfs_prop_is_string},
	{NULL, NULL}
};

int
luaopen_zfs(lua_State *L)
{
	lua_getglobal(L, "require");
	lua_pushstring(L, "nvpair");
	lua_call(L, 1, 0);

	luaL_newmetatable(L, ZFS_HANDLE_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_zfs_meta, 0);

	luaL_newmetatable(L, ZPOOL_HANDLE_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_zpool_meta, 0);

	luaL_newmetatable(L, LIBZFS_HANDLE_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_libzfs_meta, 0);

	luaL_newmetatable(L, ZFS_PROP_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_prop_meta, 0);

	luaL_newlib(L, l_zfs_funcs);

#define	DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(SPA_MINDEVSIZE);
	DEFINE(ZFS_MAXPROPLEN);
	DEFINE(ZPOOL_MAXPROPLEN);

	DEFINE(EZFS_SUCCESS);
	DEFINE(EZFS_NOMEM);
	DEFINE(EZFS_BADPROP);
	DEFINE(EZFS_PROPREADONLY);
	DEFINE(EZFS_PROPTYPE);
	DEFINE(EZFS_PROPNONINHERIT);
	DEFINE(EZFS_PROPSPACE);
	DEFINE(EZFS_BADTYPE);
	DEFINE(EZFS_BUSY);
	DEFINE(EZFS_EXISTS);
	DEFINE(EZFS_NOENT);
	DEFINE(EZFS_BADSTREAM);
	DEFINE(EZFS_DSREADONLY);
	DEFINE(EZFS_VOLTOOBIG);
	DEFINE(EZFS_INVALIDNAME);
	DEFINE(EZFS_BADRESTORE);
	DEFINE(EZFS_BADBACKUP);
	DEFINE(EZFS_BADTARGET);
	DEFINE(EZFS_NODEVICE);
	DEFINE(EZFS_BADDEV);
	DEFINE(EZFS_NOREPLICAS);
	DEFINE(EZFS_RESILVERING);
	DEFINE(EZFS_BADVERSION);
	DEFINE(EZFS_POOLUNAVAIL);
	DEFINE(EZFS_DEVOVERFLOW);
	DEFINE(EZFS_BADPATH);
	DEFINE(EZFS_CROSSTARGET);
	DEFINE(EZFS_ZONED);
	DEFINE(EZFS_MOUNTFAILED);
	DEFINE(EZFS_UMOUNTFAILED);
	DEFINE(EZFS_UNSHARENFSFAILED);
	DEFINE(EZFS_SHARENFSFAILED);
	DEFINE(EZFS_PERM);
	DEFINE(EZFS_NOSPC);
	DEFINE(EZFS_FAULT);
	DEFINE(EZFS_IO);
	DEFINE(EZFS_INTR);
	DEFINE(EZFS_ISSPARE);
	DEFINE(EZFS_INVALCONFIG);
	DEFINE(EZFS_RECURSIVE);
	DEFINE(EZFS_NOHISTORY);
	DEFINE(EZFS_POOLPROPS);
	DEFINE(EZFS_POOL_NOTSUP);
	DEFINE(EZFS_POOL_INVALARG);
	DEFINE(EZFS_NAMETOOLONG);
	DEFINE(EZFS_OPENFAILED);
	DEFINE(EZFS_NOCAP);
	DEFINE(EZFS_LABELFAILED);
	DEFINE(EZFS_BADWHO);
	DEFINE(EZFS_BADPERM);
	DEFINE(EZFS_BADPERMSET);
	DEFINE(EZFS_NODELEGATION);
	DEFINE(EZFS_UNSHARESMBFAILED);
	DEFINE(EZFS_SHARESMBFAILED);
	DEFINE(EZFS_BADCACHE);
	DEFINE(EZFS_ISL2CACHE);
	DEFINE(EZFS_VDEVNOTSUP);
	DEFINE(EZFS_NOTSUP);
	DEFINE(EZFS_ACTIVE_SPARE);
	DEFINE(EZFS_UNPLAYED_LOGS);
	DEFINE(EZFS_REFTAG_RELE);
	DEFINE(EZFS_REFTAG_HOLD);
	DEFINE(EZFS_TAGTOOLONG);
	DEFINE(EZFS_PIPEFAILED);
	DEFINE(EZFS_THREADCREATEFAILED);
	DEFINE(EZFS_POSTSPLIT_ONLINE);
	DEFINE(EZFS_SCRUBBING);
	DEFINE(EZFS_ERRORSCRUBBING);
	DEFINE(EZFS_ERRORSCRUB_PAUSED);
	DEFINE(EZFS_NO_SCRUB);
	DEFINE(EZFS_DIFF);
	DEFINE(EZFS_DIFFDATA);
	DEFINE(EZFS_POOLREADONLY);
	DEFINE(EZFS_SCRUB_PAUSED);
	DEFINE(EZFS_SCRUB_PAUSED_TO_CANCEL);
	DEFINE(EZFS_ACTIVE_POOL);
	DEFINE(EZFS_CRYPTOFAILED);
	DEFINE(EZFS_NO_PENDING);
	DEFINE(EZFS_CHECKPOINT_EXISTS);
	DEFINE(EZFS_DISCARDING_CHECKPOINT);
	DEFINE(EZFS_NO_CHECKPOINT);
	DEFINE(EZFS_DEVRM_IN_PROGRESS);
	DEFINE(EZFS_VDEV_TOO_BIG);
	DEFINE(EZFS_IOC_NOTSUPPORTED);
	DEFINE(EZFS_TOOMANY);
	DEFINE(EZFS_INITIALIZING);
	DEFINE(EZFS_NO_INITIALIZE);
	DEFINE(EZFS_WRONG_PARENT);
	DEFINE(EZFS_TRIMMING);
	DEFINE(EZFS_NO_TRIM);
	DEFINE(EZFS_TRIM_NOTSUP);
	DEFINE(EZFS_NO_RESILVER_DEFER);
	DEFINE(EZFS_EXPORT_IN_PROGRESS);
	DEFINE(EZFS_REBUILDING);
	DEFINE(EZFS_VDEV_NOTSUP);
	DEFINE(EZFS_NOT_USER_NAMESPACE);
	DEFINE(EZFS_CKSUM);
	DEFINE(EZFS_RESUME_EXISTS);
	DEFINE(EZFS_SHAREFAILED);
	DEFINE(EZFS_RAIDZ_EXPAND_IN_PROGRESS);
	DEFINE(EZFS_ASHIFT_MISMATCH);
#if __FreeBSD_version > 1600015
	DEFINE(EZFS_NO_USER_NS_SUPPORT);
#endif
	DEFINE(EZFS_UNKNOWN);

	DEFINE(ZPOOL_STATUS_CORRUPT_CACHE);
	DEFINE(ZPOOL_STATUS_MISSING_DEV_R);
	DEFINE(ZPOOL_STATUS_MISSING_DEV_NR);
	DEFINE(ZPOOL_STATUS_CORRUPT_LABEL_R);
	DEFINE(ZPOOL_STATUS_CORRUPT_LABEL_NR);
	DEFINE(ZPOOL_STATUS_BAD_GUID_SUM);
	DEFINE(ZPOOL_STATUS_CORRUPT_POOL);
	DEFINE(ZPOOL_STATUS_CORRUPT_DATA);
	DEFINE(ZPOOL_STATUS_FAILING_DEV);
	DEFINE(ZPOOL_STATUS_VERSION_NEWER);
	DEFINE(ZPOOL_STATUS_HOSTID_MISMATCH);
	DEFINE(ZPOOL_STATUS_HOSTID_ACTIVE);
	DEFINE(ZPOOL_STATUS_HOSTID_REQUIRED);
	DEFINE(ZPOOL_STATUS_IO_FAILURE_WAIT);
	DEFINE(ZPOOL_STATUS_IO_FAILURE_CONTINUE);
	DEFINE(ZPOOL_STATUS_IO_FAILURE_MMP);
	DEFINE(ZPOOL_STATUS_BAD_LOG);
	DEFINE(ZPOOL_STATUS_ERRATA);
	DEFINE(ZPOOL_STATUS_UNSUP_FEAT_READ);
	DEFINE(ZPOOL_STATUS_UNSUP_FEAT_WRITE);
	DEFINE(ZPOOL_STATUS_FAULTED_DEV_R);
	DEFINE(ZPOOL_STATUS_FAULTED_DEV_NR);
	DEFINE(ZPOOL_STATUS_VERSION_OLDER);
	DEFINE(ZPOOL_STATUS_FEAT_DISABLED);
	DEFINE(ZPOOL_STATUS_RESILVERING);
	DEFINE(ZPOOL_STATUS_OFFLINE_DEV);
	DEFINE(ZPOOL_STATUS_REMOVED_DEV);
	DEFINE(ZPOOL_STATUS_REBUILDING);
	DEFINE(ZPOOL_STATUS_REBUILD_SCRUB);
	DEFINE(ZPOOL_STATUS_NON_NATIVE_ASHIFT);
	DEFINE(ZPOOL_STATUS_COMPATIBILITY_ERR);
	DEFINE(ZPOOL_STATUS_INCOMPATIBLE_FEAT);
	DEFINE(ZPOOL_STATUS_OK);

	DEFINE(ZFS_TYPE_INVALID);
	DEFINE(ZFS_TYPE_FILESYSTEM);
	DEFINE(ZFS_TYPE_SNAPSHOT);
	DEFINE(ZFS_TYPE_VOLUME);
	DEFINE(ZFS_TYPE_POOL);
	DEFINE(ZFS_TYPE_BOOKMARK);
	DEFINE(ZFS_TYPE_VDEV);
	DEFINE(ZFS_TYPE_DATASET);

	DEFINE(ZPROP_CONT);
	DEFINE(ZPROP_INVAL);
	DEFINE(ZPROP_USERPROP);
	DEFINE(ZFS_PROP_TYPE);
	DEFINE(ZFS_PROP_CREATION);
	DEFINE(ZFS_PROP_USED);
	DEFINE(ZFS_PROP_AVAILABLE);
	DEFINE(ZFS_PROP_REFERENCED);
	DEFINE(ZFS_PROP_COMPRESSRATIO);
	DEFINE(ZFS_PROP_MOUNTED);
	DEFINE(ZFS_PROP_ORIGIN);
	DEFINE(ZFS_PROP_QUOTA);
	DEFINE(ZFS_PROP_RESERVATION);
	DEFINE(ZFS_PROP_VOLSIZE);
	DEFINE(ZFS_PROP_VOLBLOCKSIZE);
	DEFINE(ZFS_PROP_RECORDSIZE);
	DEFINE(ZFS_PROP_MOUNTPOINT);
	DEFINE(ZFS_PROP_SHARENFS);
	DEFINE(ZFS_PROP_CHECKSUM);
	DEFINE(ZFS_PROP_COMPRESSION);
	DEFINE(ZFS_PROP_ATIME);
	DEFINE(ZFS_PROP_DEVICES);
	DEFINE(ZFS_PROP_EXEC);
	DEFINE(ZFS_PROP_SETUID);
	DEFINE(ZFS_PROP_READONLY);
	DEFINE(ZFS_PROP_ZONED);
	DEFINE(ZFS_PROP_SNAPDIR);
	DEFINE(ZFS_PROP_ACLMODE);
	DEFINE(ZFS_PROP_ACLINHERIT);
	DEFINE(ZFS_PROP_CREATETXG);
	DEFINE(ZFS_PROP_NAME);
	DEFINE(ZFS_PROP_CANMOUNT);
	DEFINE(ZFS_PROP_ISCSIOPTIONS);
	DEFINE(ZFS_PROP_XATTR);
	DEFINE(ZFS_PROP_NUMCLONES);
	DEFINE(ZFS_PROP_COPIES);
	DEFINE(ZFS_PROP_VERSION);
	DEFINE(ZFS_PROP_UTF8ONLY);
	DEFINE(ZFS_PROP_NORMALIZE);
	DEFINE(ZFS_PROP_CASE);
	DEFINE(ZFS_PROP_VSCAN);
	DEFINE(ZFS_PROP_NBMAND);
	DEFINE(ZFS_PROP_SHARESMB);
	DEFINE(ZFS_PROP_REFQUOTA);
	DEFINE(ZFS_PROP_REFRESERVATION);
	DEFINE(ZFS_PROP_GUID);
	DEFINE(ZFS_PROP_PRIMARYCACHE);
	DEFINE(ZFS_PROP_SECONDARYCACHE);
	DEFINE(ZFS_PROP_USEDSNAP);
	DEFINE(ZFS_PROP_USEDDS);
	DEFINE(ZFS_PROP_USEDCHILD);
	DEFINE(ZFS_PROP_USEDREFRESERV);
	DEFINE(ZFS_PROP_USERACCOUNTING);
	DEFINE(ZFS_PROP_STMF_SHAREINFO);
	DEFINE(ZFS_PROP_DEFER_DESTROY);
	DEFINE(ZFS_PROP_USERREFS);
	DEFINE(ZFS_PROP_LOGBIAS);
	DEFINE(ZFS_PROP_UNIQUE);
	DEFINE(ZFS_PROP_OBJSETID);
	DEFINE(ZFS_PROP_DEDUP);
	DEFINE(ZFS_PROP_MLSLABEL);
	DEFINE(ZFS_PROP_SYNC);
	DEFINE(ZFS_PROP_DNODESIZE);
	DEFINE(ZFS_PROP_REFRATIO);
	DEFINE(ZFS_PROP_WRITTEN);
	DEFINE(ZFS_PROP_CLONES);
	DEFINE(ZFS_PROP_LOGICALUSED);
	DEFINE(ZFS_PROP_LOGICALREFERENCED);
	DEFINE(ZFS_PROP_INCONSISTENT);
	DEFINE(ZFS_PROP_VOLMODE);
	DEFINE(ZFS_PROP_FILESYSTEM_LIMIT);
	DEFINE(ZFS_PROP_SNAPSHOT_LIMIT);
	DEFINE(ZFS_PROP_FILESYSTEM_COUNT);
	DEFINE(ZFS_PROP_SNAPSHOT_COUNT);
	DEFINE(ZFS_PROP_SNAPDEV);
	DEFINE(ZFS_PROP_ACLTYPE);
	DEFINE(ZFS_PROP_SELINUX_CONTEXT);
	DEFINE(ZFS_PROP_SELINUX_FSCONTEXT);
	DEFINE(ZFS_PROP_SELINUX_DEFCONTEXT);
	DEFINE(ZFS_PROP_SELINUX_ROOTCONTEXT);
	DEFINE(ZFS_PROP_RELATIME);
	DEFINE(ZFS_PROP_REDUNDANT_METADATA);
	DEFINE(ZFS_PROP_OVERLAY);
	DEFINE(ZFS_PROP_PREV_SNAP);
	DEFINE(ZFS_PROP_RECEIVE_RESUME_TOKEN);
	DEFINE(ZFS_PROP_ENCRYPTION);
	DEFINE(ZFS_PROP_KEYLOCATION);
	DEFINE(ZFS_PROP_KEYFORMAT);
	DEFINE(ZFS_PROP_PBKDF2_SALT);
	DEFINE(ZFS_PROP_PBKDF2_ITERS);
	DEFINE(ZFS_PROP_ENCRYPTION_ROOT);
	DEFINE(ZFS_PROP_KEY_GUID);
	DEFINE(ZFS_PROP_KEYSTATUS);
	DEFINE(ZFS_PROP_REMAPTXG);
	DEFINE(ZFS_PROP_SPECIAL_SMALL_BLOCKS);
	DEFINE(ZFS_PROP_IVSET_GUID);
	DEFINE(ZFS_PROP_REDACTED);
	DEFINE(ZFS_PROP_REDACT_SNAPS);
	DEFINE(ZFS_PROP_SNAPSHOTS_CHANGED);
	DEFINE(ZFS_PROP_PREFETCH);
#if __FreeBSD_version > 1500003
	DEFINE(ZFS_PROP_VOLTHREADING);
#endif
#if __FreeBSD_version > 1500025
	DEFINE(ZFS_PROP_DIRECT);
#endif
#if __FreeBSD_version > 1500029
	DEFINE(ZFS_PROP_LONGNAME);
#endif
#if __FreeBSD_version > 1500039
	DEFINE(ZFS_PROP_DEFAULTUSERQUOTA);
	DEFINE(ZFS_PROP_DEFAULTGROUPQUOTA);
	DEFINE(ZFS_PROP_DEFAULTPROJECTQUOTA);
	DEFINE(ZFS_PROP_DEFAULTUSEROBJQUOTA);
	DEFINE(ZFS_PROP_DEFAULTGROUPOBJQUOTA);
	DEFINE(ZFS_PROP_DEFAULTPROJECTOBJQUOTA);
#endif
#if __FreeBSD_version > 1600015
	DEFINE(ZFS_PROP_SNAPSHOTS_CHANGED_NSECS);
#endif
	DEFINE(ZFS_NUM_PROPS);

	DEFINE(ZFS_PROP_USERUSED);
	DEFINE(ZFS_PROP_USERQUOTA);
	DEFINE(ZFS_PROP_GROUPUSED);
	DEFINE(ZFS_PROP_GROUPQUOTA);
	DEFINE(ZFS_PROP_USEROBJUSED);
	DEFINE(ZFS_PROP_USEROBJQUOTA);
	DEFINE(ZFS_PROP_GROUPOBJUSED);
	DEFINE(ZFS_PROP_GROUPOBJQUOTA);
	DEFINE(ZFS_PROP_PROJECTUSED);
	DEFINE(ZFS_PROP_PROJECTQUOTA);
	DEFINE(ZFS_PROP_PROJECTOBJUSED);
	DEFINE(ZFS_PROP_PROJECTOBJQUOTA);
	DEFINE(ZFS_NUM_USERQUOTA_PROPS);

	DEFINE(ZPOOL_PROP_INVAL);
	DEFINE(ZPOOL_PROP_NAME);
	DEFINE(ZPOOL_PROP_SIZE);
	DEFINE(ZPOOL_PROP_CAPACITY);
	DEFINE(ZPOOL_PROP_ALTROOT);
	DEFINE(ZPOOL_PROP_HEALTH);
	DEFINE(ZPOOL_PROP_GUID);
	DEFINE(ZPOOL_PROP_VERSION);
	DEFINE(ZPOOL_PROP_BOOTFS);
	DEFINE(ZPOOL_PROP_DELEGATION);
	DEFINE(ZPOOL_PROP_AUTOREPLACE);
	DEFINE(ZPOOL_PROP_CACHEFILE);
	DEFINE(ZPOOL_PROP_FAILUREMODE);
	DEFINE(ZPOOL_PROP_LISTSNAPS);
	DEFINE(ZPOOL_PROP_AUTOEXPAND);
	DEFINE(ZPOOL_PROP_DEDUPDITTO);
	DEFINE(ZPOOL_PROP_DEDUPRATIO);
	DEFINE(ZPOOL_PROP_FREE);
	DEFINE(ZPOOL_PROP_ALLOCATED);
	DEFINE(ZPOOL_PROP_READONLY);
	DEFINE(ZPOOL_PROP_ASHIFT);
	DEFINE(ZPOOL_PROP_COMMENT);
	DEFINE(ZPOOL_PROP_EXPANDSZ);
	DEFINE(ZPOOL_PROP_FREEING);
	DEFINE(ZPOOL_PROP_FRAGMENTATION);
	DEFINE(ZPOOL_PROP_LEAKED);
	DEFINE(ZPOOL_PROP_MAXBLOCKSIZE);
	DEFINE(ZPOOL_PROP_TNAME);
	DEFINE(ZPOOL_PROP_MAXDNODESIZE);
	DEFINE(ZPOOL_PROP_MULTIHOST);
	DEFINE(ZPOOL_PROP_CHECKPOINT);
	DEFINE(ZPOOL_PROP_LOAD_GUID);
	DEFINE(ZPOOL_PROP_AUTOTRIM);
	DEFINE(ZPOOL_PROP_COMPATIBILITY);
	DEFINE(ZPOOL_PROP_BCLONEUSED);
	DEFINE(ZPOOL_PROP_BCLONESAVED);
	DEFINE(ZPOOL_PROP_BCLONERATIO);
#if __FreeBSD_version > 1500023
	DEFINE(ZPOOL_PROP_DEDUP_TABLE_SIZE);
	DEFINE(ZPOOL_PROP_DEDUP_TABLE_QUOTA);
	DEFINE(ZPOOL_PROP_DEDUPCACHED);
#endif
#if __FreeBSD_version > 1500029
	DEFINE(ZPOOL_PROP_LAST_SCRUBBED_TXG);
#endif
#if __FreeBSD_version > 1600013
	DEFINE(ZPOOL_PROP_DEDUPUSED);
	DEFINE(ZPOOL_PROP_DEDUPSAVED);
	DEFINE(ZPOOL_PROP_AVAILABLE);
	DEFINE(ZPOOL_PROP_USABLE);
	DEFINE(ZPOOL_PROP_USED);
	DEFINE(ZPOOL_PROP_NORMAL_SIZE);
	DEFINE(ZPOOL_PROP_NORMAL_CAPACITY);
	DEFINE(ZPOOL_PROP_NORMAL_FREE);
	DEFINE(ZPOOL_PROP_NORMAL_ALLOCATED);
	DEFINE(ZPOOL_PROP_NORMAL_AVAILABLE);
	DEFINE(ZPOOL_PROP_NORMAL_USABLE);
	DEFINE(ZPOOL_PROP_NORMAL_USED);
	DEFINE(ZPOOL_PROP_NORMAL_EXPANDSZ);
	DEFINE(ZPOOL_PROP_NORMAL_FRAGMENTATION);
	DEFINE(ZPOOL_PROP_SPECIAL_SIZE);
	DEFINE(ZPOOL_PROP_SPECIAL_CAPACITY);
	DEFINE(ZPOOL_PROP_SPECIAL_FREE);
	DEFINE(ZPOOL_PROP_SPECIAL_ALLOCATED);
	DEFINE(ZPOOL_PROP_SPECIAL_AVAILABLE);
	DEFINE(ZPOOL_PROP_SPECIAL_USABLE);
	DEFINE(ZPOOL_PROP_SPECIAL_USED);
	DEFINE(ZPOOL_PROP_SPECIAL_EXPANDSZ);
	DEFINE(ZPOOL_PROP_SPECIAL_FRAGMENTATION);
	DEFINE(ZPOOL_PROP_DEDUP_SIZE);
	DEFINE(ZPOOL_PROP_DEDUP_CAPACITY);
	DEFINE(ZPOOL_PROP_DEDUP_FREE);
	DEFINE(ZPOOL_PROP_DEDUP_ALLOCATED);
	DEFINE(ZPOOL_PROP_DEDUP_AVAILABLE);
	DEFINE(ZPOOL_PROP_DEDUP_USABLE);
	DEFINE(ZPOOL_PROP_DEDUP_USED);
	DEFINE(ZPOOL_PROP_DEDUP_EXPANDSZ);
	DEFINE(ZPOOL_PROP_DEDUP_FRAGMENTATION);
	DEFINE(ZPOOL_PROP_LOG_SIZE);
	DEFINE(ZPOOL_PROP_LOG_CAPACITY);
	DEFINE(ZPOOL_PROP_LOG_FREE);
	DEFINE(ZPOOL_PROP_LOG_ALLOCATED);
	DEFINE(ZPOOL_PROP_LOG_AVAILABLE);
	DEFINE(ZPOOL_PROP_LOG_USABLE);
	DEFINE(ZPOOL_PROP_LOG_USED);
	DEFINE(ZPOOL_PROP_LOG_EXPANDSZ);
	DEFINE(ZPOOL_PROP_LOG_FRAGMENTATION);
	DEFINE(ZPOOL_PROP_ELOG_SIZE);
	DEFINE(ZPOOL_PROP_ELOG_CAPACITY);
	DEFINE(ZPOOL_PROP_ELOG_FREE);
	DEFINE(ZPOOL_PROP_ELOG_ALLOCATED);
	DEFINE(ZPOOL_PROP_ELOG_AVAILABLE);
	DEFINE(ZPOOL_PROP_ELOG_USABLE);
	DEFINE(ZPOOL_PROP_ELOG_USED);
	DEFINE(ZPOOL_PROP_ELOG_EXPANDSZ);
	DEFINE(ZPOOL_PROP_ELOG_FRAGMENTATION);
	DEFINE(ZPOOL_PROP_SELOG_SIZE);
	DEFINE(ZPOOL_PROP_SELOG_CAPACITY);
	DEFINE(ZPOOL_PROP_SELOG_FREE);
	DEFINE(ZPOOL_PROP_SELOG_ALLOCATED);
	DEFINE(ZPOOL_PROP_SELOG_AVAILABLE);
	DEFINE(ZPOOL_PROP_SELOG_USABLE);
	DEFINE(ZPOOL_PROP_SELOG_USED);
	DEFINE(ZPOOL_PROP_SELOG_EXPANDSZ);
	DEFINE(ZPOOL_PROP_SELOG_FRAGMENTATION);
#endif
	DEFINE(ZPOOL_NUM_PROPS);

#if __FreeBSD_version > 1600013
	DEFINE(ZPOOL_MC_PROP_SIZE);
	DEFINE(ZPOOL_MC_PROP_CAPACITY);
	DEFINE(ZPOOL_MC_PROP_FREE);
	DEFINE(ZPOOL_MC_PROP_ALLOCATED);
	DEFINE(ZPOOL_MC_PROP_AVAILABLE);
	DEFINE(ZPOOL_MC_PROP_USABLE);
	DEFINE(ZPOOL_MC_PROP_USED);
	DEFINE(ZPOOL_MC_PROP_EXPANDSZ);
	DEFINE(ZPOOL_MC_PROP_FRAGMENTATION);
	DEFINE(ZPOOL_NUM_MC_PROPS);

	DEFINE(ZPOOL_MC_PROPS_NORMAL);
	DEFINE(ZPOOL_MC_PROPS_SPECIAL);
	DEFINE(ZPOOL_MC_PROPS_DEDUP);
	DEFINE(ZPOOL_MC_PROPS_LOG);
	DEFINE(ZPOOL_MC_PROPS_ELOG);
	DEFINE(ZPOOL_MC_PROPS_SELOG);
#endif

	DEFINE(ZPROP_MAX_COMMENT);
	DEFINE(ZPROP_BOOLEAN_NA);

	DEFINE(ZPROP_SRC_NONE);
	DEFINE(ZPROP_SRC_DEFAULT);
	DEFINE(ZPROP_SRC_TEMPORARY);
	DEFINE(ZPROP_SRC_LOCAL);
	DEFINE(ZPROP_SRC_INHERITED);
	DEFINE(ZPROP_SRC_RECEIVED);

	DEFINE(ZPROP_SRC_ALL);

	DEFINE(ZPROP_ERR_NOCLEAR);
	DEFINE(ZPROP_ERR_NORESTORE);

	DEFINE(ZFS_MAX_DATASET_NAME_LEN);
#undef DEFINE

#define	DEFINE(ident) ({ \
	lua_pushstring(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(ZPROP_VALUE);
	DEFINE(ZPROP_SOURCE);
#undef DEFINE
	return (1);
}
