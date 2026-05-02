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
		return (libzfsfail(L, hdl, EZFS_UNKNOWN, "zpool_open"));
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
		return (libzfsfail(L, hdl, EZFS_UNKNOWN, "zpool_open_canfail"));
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
l_zpool_label_disk(lua_State *L)
{
	libzfs_handle_t *hdl;
	zpool_handle_t *zhp;
	const char *name;
	int error;

	hdl = checklibzfs(L, 1);
	zhp = luaL_opt(L, checkzpool, 2, NULL);
	name = luaL_checkstring(L, 3);

	if ((error = zpool_label_disk(hdl, zhp, name)) != 0) {
		return (libzfsfail(L, hdl, error, "zpool_label_disk"));
	}
	return (success(L));
}

static int
l_zpool_prepare_and_label_disk(lua_State *L)
{
	libzfs_handle_t *hdl;
	zpool_handle_t *zhp;
	const char *name, *prepare_str;
	nvlist_t *vdev_nv;
	char **lines;
	int nlines, error;

	lines = NULL;

	hdl = checklibzfs(L, 1);
	zhp = luaL_opt(L, checkzpool, 2, NULL);
	name = luaL_checkstring(L, 3);
	vdev_nv = checknvlist(L, 4);
	prepare_str = luaL_optstring(L, 5, NULL);

	if ((error = zpool_prepare_and_label_disk(hdl, zhp, name, vdev_nv,
	    prepare_str, &lines, &nlines)) != 0) {
		ASSERT0P(lines);
		ASSERT0(nlines);
		return (libzfsfail(L, hdl, error,
		    "zpool_prepare_and_label_disk"));
	}
	ASSERT3P(lines, !=, NULL);
	lua_createtable(L, nlines, 0);
	for (int i = 0; i < nlines; i++) {
		lua_pushstring(L, lines[i]);
		lua_rawseti(L, -2, i + 1);
	}
	libzfs_free_str_array(lines, nlines);
	return (1);
}

static int
l_zpool_import(lua_State *L)
{
	libzfs_handle_t *hdl;
	nvlist_t *config;
	const char *newname, *altroot;
	int error;

	hdl = checklibzfs(L, 1);
	config = checknvlist(L, 2);
	newname = luaL_optstring(L, 3, NULL);
	altroot = luaL_optstring(L, 4, NULL);

	if ((error = zpool_import(hdl, config, newname,
	    __DECONST(char *, altroot))) != 0) {
		return (libzfsfail(L, hdl, error, "zpool_import"));
	}
	return (success(L));
}

static int
l_zpool_import_props(lua_State *L)
{
	libzfs_handle_t *hdl;
	nvlist_t *config, *props;
	const char *newname;
	int flags, error;

	hdl = checklibzfs(L, 1);
	config = checknvlist(L, 2);
	newname = luaL_optstring(L, 3, NULL);
	props = optnvlist(L, 4, NULL);
	flags = luaL_checkinteger(L, 5);

	if ((error = zpool_import_props(hdl, config, newname, props, flags))
	    != 0) {
		return (libzfsfail(L, hdl, error, "zpool_import_props"));
	}
	return (success(L));
}

static int
l_zpool_vdev_name(lua_State *L)
{
	libzfs_handle_t *hdl;
	zpool_handle_t *zhp;
	nvlist_t *nvl;
	char *name;
	int name_flags;

	hdl = checklibzfs(L, 1);
	zhp = luaL_opt(L, checkzpool, 2, NULL);
	nvl = checknvlist(L, 3);
	name_flags = luaL_optinteger(L, 4, 0);

	if ((name = zpool_vdev_name(hdl, zhp, nvl, name_flags)) == NULL) {
		return (libzfsfail(L, hdl, ENOMEM, "zpool_vdev_name"));
	}
	lua_pushstring(L, name);
	free(name);
	return (1);
}

static int
l_zpool_events_next(lua_State *L)
{
	libzfs_handle_t *hdl;
	nvlist_t *nvl;
	unsigned flags;
	int dropped, zevent_fd, error;

	hdl = checklibzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	zevent_fd = checkfd(L, 3);

	if ((error = zpool_events_next(hdl, &nvl, &dropped, flags, zevent_fd))
	    != 0) {
		return (libzfsfail(L, hdl, error, "zpool_events_next"));
	}
	pushnvlist(L, nvl);
	lua_pushinteger(L, dropped);
	return (2);
}

static int
l_zpool_events_clear(lua_State *L)
{
	libzfs_handle_t *hdl;
	int count, error;

	hdl = checklibzfs(L, 1);

	if ((error = zpool_events_clear(hdl, &count)) != 0) {
		return (libzfsfail(L, hdl, error, "zpool_events_clear"));
	}
	lua_pushinteger(L, count);
	return (1);
}

static int
l_zpool_events_seek(lua_State *L)
{
	libzfs_handle_t *hdl;
	uint64_t eid;
	int zevent_fd, error;

	hdl = checklibzfs(L, 1);
	eid = luaL_checkinteger(L, 2);
	zevent_fd = checkfd(L, 3);

	if ((error = zpool_events_seek(hdl, eid, zevent_fd)) != 0) {
		return (libzfsfail(L, hdl, error, "zpool_events_seek"));
	}
	return (success(L));
}

static int
l_zpool_explain_recover(lua_State *L)
{
#if __FreeBSD_version > 1500023
	char buf[PAGE_SIZE/2];
#endif
	libzfs_handle_t *hdl;
	const char *name;
	int reason;
	nvlist_t *config;

	hdl = checklibzfs(L, 1);
	name = luaL_checkstring(L, 2);
	reason = luaL_checkinteger(L, 3);
	config = checknvlist(L, 4);

#if __FreeBSD_version > 1500023
	zpool_explain_recover(hdl, name, reason, config, buf, sizeof(buf));
	lua_pushstring(L, buf);
	return (1);
#else
	zpool_explain_recover(hdl, name, reason, config);
	return (0);
#endif
}

static int
zfs_iter_cb(zfs_handle_t *zhp, void *arg)
{
	lua_State *L = arg;
	int nargs = lua_gettop(L) - 1;
	int error;

	/* Copy the function. */
	lua_pushvalue(L, -nargs); /* hdl, cb, ..., cb */
	new(L, zhp, ZFS_HANDLE_METATABLE); /* hdl, cb, ..., cb, zhp */
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
l_zfs_foreach_mountpoint(lua_State *L)
{
	luaL_Buffer b;
	libzfs_handle_t *hdl;
	zfs_handle_t **handles;
	size_t num_handles;
	uint_t nthr;
	int ref, top;

	hdl = checklibzfs(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	num_handles = luaL_len(L, 2);
	nthr = luaL_checkinteger(L, 3);
	luaL_argcheck(L, lua_isfunction(L, 4), 4, "callback function required");

	/* Stash a reference to the handles table to protect it from GC. */
	handles = lua_newuserdatauv(L, num_handles * sizeof(*handles), 1);
	for (int i = 0; i < num_handles; i++) {
		/* XXX */
		lua_rawgeti(L, 2, i + 1);
		handles[i] = checkzfs(L, -1);
	}
	lua_pushvalue(L, 2);
	lua_setiuservalue(L, -2, 1);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);

	/* Massage the stack to fit zfs_iter_cb. */
	lua_pop(L, num_handles);
	lua_remove(L, 2);
	lua_remove(L, 3);

	top = lua_gettop(L);
	zfs_foreach_mountpoint(hdl, handles, num_handles, zfs_iter_cb, L, nthr);
	luaL_unref(L, LUA_REGISTRYINDEX, ref);
	return (lua_gettop(L) - top);
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
		return (libzfsfail(L, hdl, EZFS_UNKNOWN, "zfs_open"));
	}
	return (new(L, zhp, ZFS_HANDLE_METATABLE));
}

static int
l_zfs_iter_root(lua_State *L)
{
	libzfs_handle_t *hdl;
	int top = lua_gettop(L);
	int error;

	hdl = checklibzfs(L, 1);
	luaL_argcheck(L, lua_isfunction(L, 2), 2, "callback function required");

	if ((error = zfs_iter_root(hdl, zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_root failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_crypto_create(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *parent_name;
	nvlist_t *props, *pool_props;
	uint8_t *wkeydata;
	uint_t wkeylen;
	boolean_t stdin_available;
	int error;

	hdl = checklibzfs(L, 1);
	parent_name = luaL_optstring(L, 2, NULL);
	props = checknvlist(L, 3);
	pool_props = checknvlist(L, 4);
	stdin_available = lua_toboolean(L, 5);

	if ((error = zfs_crypto_create(hdl, __DECONST(char *, parent_name),
	    props, pool_props, stdin_available, &wkeydata, &wkeylen)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_crypto_create"));
	}
	lua_pushlstring(L, (char *)wkeydata, wkeylen);
	return (1);
}

static int
l_zfs_crypto_clone_check(lua_State *L)
{
	libzfs_handle_t *hdl;
	zfs_handle_t *origin_zhp;
	nvlist_t *props;
	int error;

	hdl = checklibzfs(L, 1);
	origin_zhp = checkzfs(L, 2);
	props = checknvlist(L, 3);

	if ((error = zfs_crypto_clone_check(hdl, origin_zhp, NULL, props))
	    != 0) {
		return (libzfsfail(L, hdl, error, "zfs_crypto_clone_check"));
	}
	return (success(L));
}

static int
l_zfs_crypto_attempt_load_keys(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *fsname;
	int error;

	hdl = checklibzfs(L, 1);
	fsname = luaL_checkstring(L, 2);

	if ((error = zfs_crypto_attempt_load_keys(hdl, fsname)) != 0) {
		return (libzfsfail(L, hdl, error,
		    "zfs_crypto_attempt_load_keys"));
	}
	return (success(L));
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
		return (libzfsfail(L, hdl, EZFS_UNKNOWN, "zfs_valid_proplist"));
	}
	pushnvlist(L, ret);
	return (1);
}

static int
l_zfs_path_to_zhandle(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *path;
	zfs_type_t type;
	zfs_handle_t *zhp;

	hdl = checklibzfs(L, 1);
	path = luaL_checkstring(L, 2);
	type = luaL_checkinteger(L, 3);

	if ((zhp = zfs_path_to_zhandle(hdl, path, type)) == NULL) {
		return (libzfsfail(L, hdl, EZFS_UNKNOWN,
		    "zfs_path_to_zhandle"));
	}
	return (new(L, zhp, ZFS_HANDLE_METATABLE));
}

static int
l_zfs_dataset_exists(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *path;
	zfs_type_t type;

	hdl = checklibzfs(L, 1);
	path = luaL_checkstring(L, 2);
	type = luaL_checkinteger(L, 3);

	lua_pushboolean(L, zfs_dataset_exists(hdl, path, type));
	return (1);
}

static int
l_zfs_create(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *path;
	zfs_type_t type;
	nvlist_t *props;
	int error;

	hdl = checklibzfs(L, 1);
	path = luaL_checkstring(L, 2);
	type = luaL_checkinteger(L, 3);
	props = optnvlist(L, 4, NULL);

	if ((error = zfs_create(hdl, path, type, props)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_create"));
	}
	return (success(L));
}

static int
l_zfs_create_ancestors(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *path;
	int error;

	hdl = checklibzfs(L, 1);
	path = luaL_checkstring(L, 2);

	if ((error = zfs_create_ancestors(hdl, path)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_create_ancestors"));
	}
	return (success(L));
}

#if __FreeBSD_version > 1600013
static int
l_zfs_create_ancestors_props(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *path;
	nvlist_t *props;
	int error;

	hdl = checklibzfs(L, 1);
	path = luaL_checkstring(L, 2);
	props = optnvlist(L, 3, 0);

	if ((error = zfs_create_ancestors_props(hdl, path, props)) != 0) {
		return (libzfsfail(L, hdl, error,
		    "zfs_create_ancestors_props"));
	}
	return (success(L));
}
#endif

static int
l_zfs_destroy_snaps_nvl(lua_State *L)
{
	libzfs_handle_t *hdl;
	nvlist_t *snaps;
	boolean_t defer;
	int error;

	hdl = checklibzfs(L, 1);
	snaps = checknvlist(L, 2);
	defer = lua_toboolean(L, 3);

	if ((error = zfs_destroy_snaps_nvl(hdl, snaps, defer)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_destroy_snaps_nvl"));
	}
	return (success(L));
}

static int
l_zfs_snapshot(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *path;
	nvlist_t *props;
	boolean_t recursive;
	int error;

	hdl = checklibzfs(L, 1);
	path = luaL_checkstring(L, 2);
	recursive = lua_toboolean(L, 3);
	props = optnvlist(L, 4, NULL);

	if ((error = zfs_snapshot(hdl, path, recursive, props)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_snapshot"));
	}
	return (success(L));
}

static int
l_zfs_snapshot_nvl(lua_State *L)
{
	libzfs_handle_t *hdl;
	nvlist_t *snaps, *props;
	int error;

	hdl = checklibzfs(L, 1);
	snaps = checknvlist(L, 2);
	props = optnvlist(L, 3, NULL);

	if ((error = zfs_snapshot_nvl(hdl, snaps, props)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_snapshot_nvl"));
	}
	return (success(L));
}

static inline void
checksendflags(lua_State *L, int idx, sendflags_t *flags)
{
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "verbosity");
	flags->verbosity = lua_tointeger(L, -1);
	lua_pop(L, 1);
#define	FLAG(name) ({ \
	lua_getfield(L, idx, #name); \
	flags->name = lua_toboolean(L, -1); \
	lua_pop(L, 1); \
})
	FLAG(replicate);
	FLAG(skipmissing);
	FLAG(doall);
	FLAG(fromorigin);
	FLAG(pad);
	FLAG(props);
	FLAG(dryrun);
	FLAG(parsable);
	FLAG(progress);
	FLAG(progressastitle);
	FLAG(largeblock);
	FLAG(embed_data);
	FLAG(compress);
	FLAG(raw);
	FLAG(backup);
	FLAG(holds);
	FLAG(saved);
#if __FreeBSD_version > 1600013
	FLAG(no_preserve_encryption);
#endif
#undef FLAG
}

static int
l_zfs_send_resume(lua_State *L)
{
	libzfs_handle_t *hdl;
	sendflags_t flags;
	const char *resume_token;
	int outfd, error;

	memset(&flags, 0, sizeof(flags));

	hdl = checklibzfs(L, 1);
	checksendflags(L, 2, &flags);
	outfd = checkfd(L, 3);
	resume_token = luaL_checkstring(L, 4);

	if ((error = zfs_send_resume(hdl, &flags, outfd, resume_token)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_send_resume"));
	}
	return (success(L));
}

static int
l_zfs_send_resume_token_to_nvlist(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *resume_token;
	nvlist_t *nvl;

	hdl = checklibzfs(L, 1);
	resume_token = luaL_checkstring(L, 2);

	if ((nvl = zfs_send_resume_token_to_nvlist(hdl, resume_token))
	    == NULL) {
		return (libzfsfail(L, hdl, EINVAL,
		    "zfs_send_resume_token_to_nvlist"));
	}
	pushnvlist(L, nvl);
	return (1);
}

static inline void
checkrecvflags(lua_State *L, int idx, recvflags_t *flags)
{
	luaL_checktype(L, idx, LUA_TTABLE);
#define	FLAG(name) ({ \
	lua_getfield(L, idx, #name); \
	flags->name = lua_toboolean(L, -1); \
	lua_pop(L, 1); \
})
	FLAG(verbose);
	FLAG(isprefix);
	FLAG(istail);
	FLAG(dryrun);
	FLAG(force);
	FLAG(canmountoff);
	FLAG(resumable);
	FLAG(byteswap);
	FLAG(nomount);
	FLAG(holds);
	FLAG(skipholds);
	FLAG(domount);
	FLAG(forceunmount);
	FLAG(heal);
#undef FLAG
}

static int
l_zfs_receive(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *tosnap;
	nvlist_t *props;
	recvflags_t flags;
	int infd, error;

	memset(&flags, 0, sizeof(flags));

	hdl = checklibzfs(L, 1);
	tosnap = luaL_checkstring(L, 2);
	props = optnvlist(L, 3, NULL);
	checkrecvflags(L, 4, &flags);
	infd = checkfd(L, 5);

	if ((error = zfs_receive(hdl, tosnap, props, &flags, infd, NULL))
	    != 0) {
		return (libzfsfail(L, hdl, error, "zfs_receive"));
	}
	return (success(L));
}

static int
l_is_mounted(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *special;
	char *where;
	boolean_t mounted;

	hdl = checklibzfs(L, 1);
	special = luaL_checkstring(L, 2);

	mounted = is_mounted(hdl, special, &where);
	lua_pushboolean(L, mounted);
	if (mounted) {
		lua_pushstring(L, where);
		return (2);
	}
	return (1);
}

static int
l_zfs_nicestrtonum(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *value;
	uint64_t num;
	int error;

	hdl = checklibzfs(L, 1);
	value = luaL_checkstring(L, 2);

	if ((error = zfs_nicestrtonum(hdl, value, &num)) != 0) {
		return (libzfsfail(L, hdl, error, "zfs_nicestrtonum"));
	}
	lua_pushinteger(L, num);
	return (1);
}

static int
l_zpool_in_use(lua_State *L)
{
	libzfs_handle_t *hdl;
	char *namestr;
	pool_state_t state;
	boolean_t inuse;
	int fd, error;

	hdl = checklibzfs(L, 1);
	fd = checkfd(L, 2);

	if ((error = zpool_in_use(hdl, fd, &state, &namestr, &inuse)) != 0) {
		return (libzfsfail(L, hdl, error, "zpool_in_use"));
	}
	lua_pushboolean(L, inuse);
	lua_pushstring(L, namestr);
	lua_pushinteger(L, state);
	return (3);
}

static int
l_zpool_nextboot(lua_State *L)
{
	libzfs_handle_t *hdl;
	const char *command;
	uint64_t pool_guid, vdev_guid;
	int error;

	hdl = checklibzfs(L, 1);
	pool_guid = luaL_checkinteger(L, 2);
	vdev_guid = luaL_checkinteger(L, 3);
	command = luaL_checkstring(L, 4);

	if ((error = zpool_nextboot(hdl, pool_guid, vdev_guid, command)) != 0) {
		return (libzfsfail(L, hdl, error, "zpool_nextboot"));
	}
	return (success(L));
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
		return (zfsfail(L, zhp, EZFS_UNKNOWN, "zfs_handle_dup"));
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
		return (zfsfail(L, zhp, EZFS_UNKNOWN, "zfs_get_pool_handle"));
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
		return (zfsfail(L, zhp, EZFS_UNKNOWN, "zfs_get_recvd_props"));
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
l_zfs_iter_children(lua_State *L)
{
	zfs_handle_t *zhp;
	int top = lua_gettop(L);
	int error;

	zhp = checkzfs(L, 1);
	luaL_argcheck(L, lua_isfunction(L, 2), 2, "callback function required");

	if ((error = zfs_iter_children(zhp, zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_children failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_dependents(lua_State *L)
{
	zfs_handle_t *zhp;
	boolean_t allowrecursion;
	int top, error;

	zhp = checkzfs(L, 1);
	allowrecursion = lua_toboolean(L, 2);
	luaL_argcheck(L, lua_isfunction(L, 3), 3, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	top = lua_gettop(L);

	if ((error = zfs_iter_dependents(zhp, allowrecursion, zfs_iter_cb, L))
	    != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_dependents failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_filesystems(lua_State *L)
{
	zfs_handle_t *zhp;
	int top = lua_gettop(L);
	int error;

	zhp = checkzfs(L, 1);
	luaL_argcheck(L, lua_isfunction(L, 2), 2, "callback function required");

	if ((error = zfs_iter_filesystems(zhp, zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_filesystems failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_snapshots(lua_State *L)
{
	zfs_handle_t *zhp;
	uint64_t min_txg, max_txg;
	boolean_t simple;
	int top, error;

	zhp = checkzfs(L, 1);
	simple = lua_toboolean(L, 2);
	min_txg = luaL_optinteger(L, 3, 0);
	max_txg = luaL_optinteger(L, 4, 0);
	luaL_argcheck(L, lua_isfunction(L, 5), 5, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	lua_remove(L, 3);
	lua_remove(L, 4);
	top = lua_gettop(L);

	if ((error = zfs_iter_snapshots(zhp, simple, zfs_iter_cb, L, min_txg,
	    max_txg)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_snapshots failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_snapshots_sorted(lua_State *L)
{
	zfs_handle_t *zhp;
	uint64_t min_txg, max_txg;
	int top, error;

	zhp = checkzfs(L, 1);
	min_txg = luaL_optinteger(L, 2, 0);
	max_txg = luaL_optinteger(L, 3, 0);
	luaL_argcheck(L, lua_isfunction(L, 4), 4, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	lua_remove(L, 3);
	top = lua_gettop(L);

	if ((error = zfs_iter_snapshots_sorted(zhp, zfs_iter_cb, L, min_txg,
	    max_txg)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_snapshots_sorted failed");
		return (2);
	}
	return (success(L));
}

/* XXX: this shouldn't be externally visible, please break */
extern char *zfs_strdup(libzfs_handle_t *, const char *);

static int
l_zfs_iter_snapspec(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *snapspec;
	char *snapspeccopy;
	int top, error;

	zhp = checkzfs(L, 1);
	snapspec = luaL_checkstring(L, 2);
	luaL_argcheck(L, lua_isfunction(L, 3), 3, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	snapspeccopy = zfs_strdup(zfs_get_handle(zhp), snapspec);
	lua_remove(L, 2);
	top = lua_gettop(L);

	error = zfs_iter_snapspec(zhp, snapspeccopy, zfs_iter_cb, L);
	free(snapspeccopy);
	if (error != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_snapspec failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_bookmarks(lua_State *L)
{
	zfs_handle_t *zhp;
	int top = lua_gettop(L);
	int error;

	zhp = checkzfs(L, 1);
	luaL_argcheck(L, lua_isfunction(L, 2), 2, "callback function required");

	if ((error = zfs_iter_bookmarks(zhp, zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_bookmarks failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_children_v2(lua_State *L)
{
	zfs_handle_t *zhp;
	int flags, top, error;

	zhp = checkzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	luaL_argcheck(L, lua_isfunction(L, 3), 3, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	top = lua_gettop(L);

	if ((error = zfs_iter_children_v2(zhp, flags, zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_children_v2 failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_dependents_v2(lua_State *L)
{
	zfs_handle_t *zhp;
	boolean_t allowrecursion;
	int flags, top, error;

	zhp = checkzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	allowrecursion = lua_toboolean(L, 3);
	luaL_argcheck(L, lua_isfunction(L, 4), 4, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	lua_remove(L, 3);
	top = lua_gettop(L);

	if ((error = zfs_iter_dependents_v2(zhp, flags, allowrecursion,
	    zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_dependents_v2 failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_filesystems_v2(lua_State *L)
{
	zfs_handle_t *zhp;
	int flags, top, error;

	zhp = checkzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	luaL_argcheck(L, lua_isfunction(L, 3), 3, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	top = lua_gettop(L);

	if ((error = zfs_iter_filesystems_v2(zhp, flags, zfs_iter_cb, L))
	    != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_filesystems_v2 failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_snapshots_v2(lua_State *L)
{
	zfs_handle_t *zhp;
	uint64_t min_txg, max_txg;
	int flags, top, error;

	zhp = checkzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	min_txg = luaL_optinteger(L, 3, 0);
	max_txg = luaL_optinteger(L, 4, 0);
	luaL_argcheck(L, lua_isfunction(L, 5), 5, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	lua_remove(L, 3);
	lua_remove(L, 4);
	top = lua_gettop(L);

	if ((error = zfs_iter_snapshots_v2(zhp, flags, zfs_iter_cb, L, min_txg,
	    max_txg)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_snapshots_v2 failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_snapshots_sorted_v2(lua_State *L)
{
	zfs_handle_t *zhp;
	uint64_t min_txg, max_txg;
	int flags, top, error;

	zhp = checkzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	min_txg = luaL_optinteger(L, 3, 0);
	max_txg = luaL_optinteger(L, 4, 0);
	luaL_argcheck(L, lua_isfunction(L, 5), 5, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	lua_remove(L, 3);
	lua_remove(L, 4);
	top = lua_gettop(L);

	if ((error = zfs_iter_snapshots_sorted_v2(zhp, flags, zfs_iter_cb, L,
	    min_txg, max_txg)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_snapshots_sorted_v2 failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_snapspec_v2(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *snapspec;
	char *snapspeccopy;
	int flags, top, error;

	zhp = checkzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	snapspec = luaL_checkstring(L, 3);
	luaL_argcheck(L, lua_isfunction(L, 4), 4, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	snapspeccopy = zfs_strdup(zfs_get_handle(zhp), snapspec);
	lua_remove(L, 2);
	lua_remove(L, 3);
	top = lua_gettop(L);

	error = zfs_iter_snapspec_v2(zhp, flags, snapspeccopy, zfs_iter_cb, L);
	free(snapspeccopy);
	if (error != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_snapspec_v2 failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_bookmarks_v2(lua_State *L)
{
	zfs_handle_t *zhp;
	int flags, top, error;

	zhp = checkzfs(L, 1);
	flags = luaL_checkinteger(L, 2);
	luaL_argcheck(L, lua_isfunction(L, 3), 3, "callback function required");

	/* Massage the stack to fit zfs_iter_cb. */
	lua_remove(L, 2);
	top = lua_gettop(L);

	if ((error = zfs_iter_bookmarks_v2(zhp, flags, zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_bookmarks_v2 failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_iter_mounted(lua_State *L)
{
	zfs_handle_t *zhp;
	int top = lua_gettop(L);
	int error;

	zhp = checkzfs(L, 1);
	luaL_argcheck(L, lua_isfunction(L, 2), 2, "callback function required");

	if ((error = zfs_iter_mounted(zhp, zfs_iter_cb, L)) != 0) {
		int nresults = lua_gettop(L) - top;

		if (nresults > 0) {
			return (nresults);
		}
		/* XXX: We don't get useful error info back. */
		luaL_pushfail(L);
		lua_pushliteral(L, "zfs_iter_mounted failed");
		return (2);
	}
	return (success(L));
}

static int
l_zfs_wait_status(lua_State *L)
{
	zfs_handle_t *zhp;
	zfs_wait_activity_t activity;
	boolean_t missing, waited;
	int error;

	zhp = checkzfs(L, 1);
	activity = luaL_checkinteger(L, 2);

	if ((error = zfs_wait_status(zhp, activity, &missing, &waited)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_wait_status"));
	}
	lua_pushboolean(L, missing);
	lua_pushboolean(L, waited);
	return (2);
}

static int
l_zfs_crypto_get_encryption_root(lua_State *L)
{
	char encroot[ZFS_MAX_DATASET_NAME_LEN];
	zfs_handle_t *zhp;
	boolean_t is_encroot;
	int error;

	zhp = checkzfs(L, 1);

	if ((error = zfs_crypto_get_encryption_root(zhp, &is_encroot, encroot))
	    != 0) {
		return (zfsfail(L, zhp, error,
		    "zfs_crypto_get_encryption_root"));
	}
	lua_pushboolean(L, is_encroot);
	lua_pushstring(L, encroot);
	return (2);
}

static int
l_zfs_crypto_load_key(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *alt_keylocation;
	boolean_t noop;
	int error;

	zhp = checkzfs(L, 1);
	noop = lua_toboolean(L, 2);
	alt_keylocation = luaL_optstring(L, 3, NULL);

	if ((error = zfs_crypto_load_key(zhp, noop, alt_keylocation)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_crypto_load_key"));
	}
	return (success(L));
}

static int
l_zfs_crypto_unload_key(lua_State *L)
{
	zfs_handle_t *zhp;
	int error;

	zhp = checkzfs(L, 1);

	if ((error = zfs_crypto_unload_key(zhp)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_crypto_unload_key"));
	}
	return (success(L));
}

static int
l_zfs_crypto_rewrap(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *raw_props;
	boolean_t inherit_key;
	int error;

	zhp = checkzfs(L, 1);
	raw_props = checknvlist(L, 2);
	inherit_key = lua_toboolean(L, 3);

	if ((error = zfs_crypto_rewrap(zhp, raw_props, inherit_key)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_crypto_rewrap"));
	}
	return (success(L));
}

#if __FreeBSD_version > 1500044
static int
l_zfs_is_encrypted(lua_State *L)
{
	zfs_handle_t *zhp;

	zhp = checkzfs(L, 1);

	lua_pushboolean(L, zfs_is_encrypted(zhp));
	return (1);
}
#endif

static int
l_zfs_destroy(lua_State *L)
{
	zfs_handle_t *zhp;
	boolean_t defer;
	int error;

	zhp = checkzfs(L, 1);
	defer = lua_toboolean(L, 2);

	if ((error = zfs_destroy(zhp, defer)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_destroy"));
	}
	return (success(L));
}

static int
l_zfs_destroy_snaps(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *snapname;
	boolean_t defer;
	int error;

	zhp = checkzfs(L, 1);
	snapname = luaL_checkstring(L, 2);
	defer = lua_toboolean(L, 3);

	if ((error = zfs_destroy_snaps(zhp, __DECONST(char *, snapname), defer))
	    != 0) {
		return (zfsfail(L, zhp, error, "zfs_destroy_snaps"));
	}
	return (success(L));
}

static int
l_zfs_clone(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *target;
	nvlist_t *props;
	int error;

	zhp = checkzfs(L, 1);
	target = luaL_checkstring(L, 2);
	props = optnvlist(L, 3, NULL);

	if ((error = zfs_clone(zhp, target, props)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_clone"));
	}
	return (success(L));
}

static int
l_zfs_rollback(lua_State *L)
{
	zfs_handle_t *zhp, *snap;
	boolean_t force;
	int error;

	zhp = checkzfs(L, 1);
	snap = checkzfs(L, 2);
	force = lua_toboolean(L, 3);

	if ((error = zfs_rollback(zhp, snap, force)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_rollback"));
	}
	return (success(L));
}

static int
l_zfs_rename(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *target;
	renameflags_t flags;
	int error;

	memset(&flags, 0, sizeof(flags));

	zhp = checkzfs(L, 1);
	target = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
#define	FLAG(name) ({ \
	lua_getfield(L, 3, #name); \
	flags.name = lua_toboolean(L, -1); \
	lua_pop(L, 1); \
})
	FLAG(forceunmount);
	FLAG(nounmount);
	FLAG(recursive);
#undef FLAG

	if ((error = zfs_rename(zhp, target, flags)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_rename"));
	}
	return (success(L));
}

static boolean_t
snapfilter_cb(zfs_handle_t *zhp, void *arg)
{
	lua_State *L = arg;
	int nargs = lua_gettop(L) - 6;
	boolean_t result;
	int error;

	/* Copy the function. */
	lua_pushvalue(L, -nargs); /* hdl, cb, ..., cb */
	new(L, zhp, ZFS_HANDLE_METATABLE); /* hdl, cb, ..., cb, zhp */
	/* Copy any additional args. */
	for (int i = 1; i < nargs; i++) {
		lua_pushvalue(L, -nargs);
	}
	if ((error = lua_pcall(L, nargs, 1, 0)) != LUA_OK) {
		return (B_FALSE);
	}
	result = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return (result);
}

static int
l_zfs_send(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *fromsnap, *tosnap;
	sendflags_t flags;
	snapfilter_cb_t filter_func;
	nvlist_t *dbg;
	int outfd, error;

	memset(&flags, 0, sizeof(flags));
	dbg = NULL;

	zhp = checkzfs(L, 1);
	fromsnap = luaL_optstring(L, 2, NULL);
	tosnap = luaL_optstring(L, 3, NULL);
	checksendflags(L, 4, &flags);
	outfd = checkfd(L, 5);

	if ((error = zfs_send(zhp, fromsnap, tosnap, &flags, outfd,
	    lua_isfunction(L, 6) ? snapfilter_cb : NULL,
	    lua_isfunction(L, 6) ? L : NULL,
	    flags.verbosity >= 3 ? &dbg : NULL)) != 0) {
		zfsfail(L, zhp, error, "zfs_send");
		if (dbg != NULL) {
			pushnvlist(L, dbg);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (dbg != NULL) {
		pushnvlist(L, dbg);
		return (1);
	}
	return (success(L));
}

static int
l_zfs_send_one(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *fromsnap, *redactbook;
	sendflags_t flags;
	int outfd, error;

	memset(&flags, 0, sizeof(flags));

	zhp = checkzfs(L, 1);
	fromsnap = luaL_optstring(L, 2, NULL);
	outfd = checkfd(L, 3);
	checksendflags(L, 4, &flags);
	redactbook = luaL_optstring(L, 5, NULL);

	if ((error = zfs_send_one(zhp, fromsnap, outfd, &flags, redactbook))
	    != 0) {
		return (zfsfail(L, zhp, error, "zfs_send_one"));
	}
	return (success(L));
}

static int
l_zfs_send_progress(lua_State *L)
{
	zfs_handle_t *zhp;
	uint64_t bytes_written, blocks_visited;
	int outfd, error;

	zhp = checkzfs(L, 1);
	outfd = checkfd(L, 2);

	if ((error = zfs_send_progress(zhp, outfd, &bytes_written,
	    &blocks_visited)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_send_progress"));
	}
	lua_pushinteger(L, bytes_written);
	lua_pushinteger(L, blocks_visited);
	return (2);
}

static int
l_zfs_send_saved(lua_State *L)
{
	zfs_handle_t *zhp;
	sendflags_t flags;
	const char *resume_token;
	int outfd, error;

	memset(&flags, 0, sizeof(flags));

	zhp = checkzfs(L, 1);
	checksendflags(L, 2, &flags);
	outfd = checkfd(L, 3);
	resume_token = luaL_checkstring(L, 4);

	if ((error = zfs_send_saved(zhp, &flags, outfd, resume_token)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_send_saved"));
	}
	return (success(L));
}

static int
l_zfs_promote(lua_State *L)
{
	zfs_handle_t *zhp;
	int error;

	zhp = checkzfs(L, 1);

	if ((error = zfs_promote(zhp)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_promote"));
	}
	return (success(L));
}

static int
l_zfs_hold(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *snapname, *tag;
	boolean_t recursive;
	int cleanup_fd, error;

	zhp = checkzfs(L, 1);
	snapname = luaL_checkstring(L, 2);
	tag = luaL_checkstring(L, 3);
	recursive = lua_toboolean(L, 4);
	cleanup_fd = checkfd(L, 5);

	if ((error = zfs_hold(zhp, snapname, tag, recursive, cleanup_fd))
	    != 0) {
		return (zfsfail(L, zhp, error, "zfs_hold"));
	}
	return (success(L));
}

static int
l_zfs_hold_nvl(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *nvl;
	int cleanup_fd, error;

	zhp = checkzfs(L, 1);
	cleanup_fd = checkfd(L, 2);
	nvl = checknvlist(L, 3);

	if ((error = zfs_hold_nvl(zhp, cleanup_fd, nvl)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_hold_nvl"));
	}
	return (success(L));
}

static int
l_zfs_release(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *snapname, *tag;
	boolean_t recursive;
	int error;

	zhp = checkzfs(L, 1);
	snapname = luaL_checkstring(L, 2);
	tag = luaL_checkstring(L, 3);
	recursive = lua_toboolean(L, 4);

	if ((error = zfs_release(zhp, snapname, tag, recursive)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_release"));
	}
	return (success(L));
}

static int
l_zfs_get_holds(lua_State *L)
{
	zfs_handle_t *zhp;
	nvlist_t *holds;
	int error;

	zhp = checkzfs(L, 1);

	if ((error = zfs_get_holds(zhp, &holds)) != 0) {
		ASSERT0P(holds);
		return (zfsfail(L, zhp, error, "zfs_get_holds"));
	}
	ASSERT3P(holds, !=, NULL);
	pushnvlist(L, holds);
	return (1);
}

#if 0 /* TODO */
static int
l_zfs_userspace(lua_State *L)
{
}
#endif

static int
l_zfs_refresh_properties(lua_State *L)
{
	zfs_handle_t *zhp;

	zhp = checkzfs(L, 1);

	zfs_refresh_properties(zhp);
	return (0);
}

static int
l_zfs_parent_name(lua_State *L)
{
	char parent[ZFS_MAX_DATASET_NAME_LEN];
	zfs_handle_t *zhp;
	int error;

	parent[0] = '\0';

	zhp = checkzfs(L, 1);

	if ((error = zfs_parent_name(zhp, parent, sizeof(parent))) != 0) {
		return (zfsfail(L, zhp, error, "zfs_parent_name"));
	}
	lua_pushstring(L, parent);
	return (1);
}

static int
l_zfs_spa_version(lua_State *L)
{
	zfs_handle_t *zhp;
	int version, error;

	zhp = checkzfs(L, 1);

	if ((error = zfs_spa_version(zhp, &version)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_spa_version"));
	}
	lua_pushinteger(L, version);
	return (1);
}

static int
l_zfs_is_mounted(lua_State *L)
{
	zfs_handle_t *zhp;
	char *where;

	zhp = checkzfs(L, 1);

	if (zfs_is_mounted(zhp, &where)) {
		lua_pushboolean(L, B_TRUE);
		lua_pushstring(L, where);
		free(where);
		return (2);
	}
	lua_pushboolean(L, B_FALSE);
	return (1);
}

static int
l_zfs_mount(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *options;
	int flags, error;

	zhp = checkzfs(L, 1);
	options = luaL_optstring(L, 2, NULL);
	flags = luaL_optinteger(L, 3, 0);

	if ((error = zfs_mount(zhp, options, flags)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_mount"));
	}
	return (success(L));
}

static int
l_zfs_mount_at(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *options, *mountpoint;
	int flags, error;

	zhp = checkzfs(L, 1);
	options = luaL_optstring(L, 2, NULL);
	flags = luaL_optinteger(L, 3, 0);
	mountpoint = luaL_checkstring(L, 4);

	if ((error = zfs_mount_at(zhp, options, flags, mountpoint)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_mount_at"));
	}
	return (success(L));
}

static int
l_zfs_unmount(lua_State *L)
{
	zfs_handle_t *zhp;
	const char *mountpoint;
	int flags, error;

	zhp = checkzfs(L, 1);
	mountpoint = luaL_optstring(L, 2, NULL);
	flags = luaL_optinteger(L, 3, 0);

	if ((error = zfs_unmount(zhp, mountpoint, flags)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_unmount"));
	}
	return (success(L));
}

static int
l_zfs_unmountall(lua_State *L)
{
	zfs_handle_t *zhp;
	int flags, error;

	zhp = checkzfs(L, 1);
	flags = luaL_optinteger(L, 2, 0);

	if ((error = zfs_unmountall(zhp, flags)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_unmountall"));
	}
	return (success(L));
}

static int
l_zfs_jail(lua_State *L)
{
	zfs_handle_t *zhp;
	int jid, attach, error;

	zhp = checkzfs(L, 1);
	jid = luaL_checkinteger(L, 2);
	attach = lua_toboolean(L, 3);

	if ((error = zfs_jail(zhp, jid, attach)) != 0) {
		return (zfsfail(L, zhp, error, "zfs_jail"));
	}
	return (success(L));
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
l_zpool_get_state_str(lua_State *L)
{
	zpool_handle_t *zhp;

	zhp = checkzpool(L, 1);

	lua_pushstring(L, zpool_get_state_str(zhp));
	return (1);
}

static int
l_zpool_get_status(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *msgid;
	zpool_errata_t errata;
	zpool_status_t status;

	zhp = checkzpool(L, 1);

	status = zpool_get_status(zhp, &msgid, &errata);
	lua_pushinteger(L, status);
	if (msgid == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushstring(L, msgid);
	}
	if (status == ZPOOL_STATUS_ERRATA) {
		lua_pushinteger(L, errata);
		return (3);
	}
	return (2);
}

static int
l_zpool_wait(lua_State *L)
{
	zpool_handle_t *zhp;
	zpool_wait_activity_t activity;
	int error;

	zhp = checkzpool(L, 1);
	activity = luaL_checkinteger(L, 2);

	if ((error = zpool_wait(zhp, activity)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_wait"));
	}
	return (success(L));
}

static int
l_zpool_wait_status(lua_State *L)
{
	zpool_handle_t *zhp;
	zpool_wait_activity_t activity;
	boolean_t missing, waited;
	int error;

	zhp = checkzpool(L, 1);
	activity = luaL_checkinteger(L, 2);

	if ((error = zpool_wait_status(zhp, activity, &missing, &waited))
	    != 0) {
		return (zpoolfail(L, zhp, error, "zpool_wait_status"));
	}
	lua_pushboolean(L, waited);
	lua_pushboolean(L, missing);
	return (2);
}

static int
l_zpool_destroy(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *message;
	int error;

	zhp = checkzpool(L, 1);
	message = luaL_checkstring(L, 2);

	if ((error = zpool_destroy(zhp, message)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_destroy"));
	}
	return (success(L));
}

static int
l_zpool_add(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *nvroot;
	boolean_t check_ashift;
	int error;

	zhp = checkzpool(L, 1);
	nvroot = checknvlist(L, 2);
	check_ashift = lua_toboolean(L, 3);

	if ((error = zpool_add(zhp, nvroot, check_ashift)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_add"));
	}
	return (success(L));
}

static int
l_zpool_scan(lua_State *L)
{
	zpool_handle_t *zhp;
	pool_scan_func_t func;
	pool_scrub_cmd_t cmd;
	int error;

	zhp = checkzpool(L, 1);
	func = luaL_checkinteger(L, 2);
	cmd = luaL_checkinteger(L, 3);

	if ((error = zpool_scan(zhp, func, cmd)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_scan"));
	}
	return (success(L));
}

#if __FreeBSD_version > 1500056
static int
l_zpool_scan_range(lua_State *L)
{
	zpool_handle_t *zhp;
	pool_scan_func_t func;
	pool_scrub_cmd_t cmd;
	time_t date_start, date_end;
	int error;

	zhp = checkzpool(L, 1);
	func = luaL_checkinteger(L, 2);
	cmd = luaL_checkinteger(L, 3);
	date_start = luaL_optinteger(L, 4, 0);
	date_end = luaL_optinteger(L, 5, 0);

	if ((error = zpool_scan_range(zhp, func, cmd, date_start, date_end))
	    != 0) {
		return (zpoolfail(L, zhp, error, "zpool_scan_range"));
	}
	return (success(L));
}
#endif

#if __FreeBSD_version > 1500056
static int
l_zpool_initialize_one(lua_State *L)
{
	zpool_handle_t *zhp;
	initialize_cbdata_t cbdata;
	int error;

	memset(&cbdata, 0, sizeof(cbdata));

	zhp = checkzpool(L, 1);
	cbdata.cmd_type = luaL_checkinteger(L, 2);
	cbdata.wait = lua_toboolean(L, 3);

	if ((error = zpool_initialize_one(zhp, &cbdata)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_initialize_one"));
	}
	return (success(L));
}
#endif

static int
l_zpool_initialize(lua_State *L)
{
	zpool_handle_t *zhp;
	pool_initialize_func_t cmd_type;
	nvlist_t *vdevs;
	int error;

	zhp = checkzpool(L, 1);
	cmd_type = luaL_checkinteger(L, 2);
	vdevs = checknvlist(L, 3);

	if ((error = zpool_initialize(zhp, cmd_type, vdevs)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_initialize"));
	}
	return (success(L));
}

static int
l_zpool_initialize_wait(lua_State *L)
{
	zpool_handle_t *zhp;
	pool_initialize_func_t cmd_type;
	nvlist_t *vdevs;
	int error;

	zhp = checkzpool(L, 1);
	cmd_type = luaL_checkinteger(L, 2);
	vdevs = checknvlist(L, 3);

	if ((error = zpool_initialize_wait(zhp, cmd_type, vdevs)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_initialize_wait"));
	}
	return (success(L));
}

static inline void
checktrimflags(lua_State *L, int idx, trimflags_t *flags)
{
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "rate");
	flags->rate = lua_tointeger(L, -1);
	lua_pop(L, 1);
#define	FLAG(name) ({ \
	lua_getfield(L, idx, #name); \
	flags->name = lua_toboolean(L, -1); \
	lua_pop(L, 1); \
})
	FLAG(fullpool);
	FLAG(secure);
	FLAG(wait);
#undef FLAG
}

static int
l_zpool_trim(lua_State *L)
{
	zpool_handle_t *zhp;
	pool_trim_func_t cmd_type;
	nvlist_t *vdevs;
	trimflags_t flags;
	int error;

	memset(&flags, 0, sizeof(flags));

	zhp = checkzpool(L, 1);
	cmd_type = luaL_checkinteger(L, 2);
	vdevs = checknvlist(L, 3);
	checktrimflags(L, 4, &flags);

	if ((error = zpool_trim(zhp, cmd_type, vdevs, &flags)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_trim"));
	}
	return (success(L));
}

static int
l_zpool_clear(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	nvlist_t *rewind;
	int error;

	zhp = checkzpool(L, 1);
	path = luaL_optstring(L, 2, NULL);
	rewind = optnvlist(L, 3, NULL);

	if ((error = zpool_clear(zhp, path, rewind)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_clear"));
	}
	return (success(L));
}

static int
l_zpool_reguid(lua_State *L)
{
	zpool_handle_t *zhp;
	int error;

	zhp = checkzpool(L, 1);

	if ((error = zpool_reguid(zhp)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_reguid"));
	}
	return (success(L));
}

#if __FreeBSD_version > 1500023
static int
l_zpool_set_guid(lua_State *L)
{
	zpool_handle_t *zhp;
	uint64_t *guidp, guid;
	int error;

	zhp = checkzpool(L, 1);
	if (lua_isnoneornil(L, 2)) {
		guidp = NULL;
	} else {
		guid = luaL_checkinteger(L, 2);
		guidp = &guid;
	}

	if ((error = zpool_set_guid(zhp, guidp)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_set_guid"));
	}
	return (success(L));
}
#endif

static int
l_zpool_reopen_one(lua_State *L)
{
	zpool_handle_t *zhp;
	boolean_t scrub_restart;
	int error;

	zhp = checkzpool(L, 1);
	scrub_restart = lua_toboolean(L, 2);

	if ((error = zpool_reopen_one(zhp, &scrub_restart)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_reopen_one"));
	}
	return (success(L));
}

#if __FreeBSD_version > 1500056
static int
l_zpool_collect_leaves(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *nvroot, *vdevs;

	zhp = checkzpool(L, 1);
	nvroot = checknvlist(L, 2);

	vdevs = fnvlist_alloc();
	zpool_collect_leaves(zhp, nvroot, vdevs);
	pushnvlist(L, vdevs);
	return (1);
}
#endif

static int
l_zpool_sync_one(lua_State *L)
{
	zpool_handle_t *zhp;
	boolean_t force;
	int error;

	zhp = checkzpool(L, 1);
	force = lua_toboolean(L, 2);

	if ((error = zpool_sync_one(zhp, &force)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_sync_one"));
	}
	return (success(L));
}

#if __FreeBSD_version > 1500056
static int
l_zpool_trim_one(lua_State *L)
{
	zpool_handle_t *zhp;
	trim_cbdata_t cbdata;
	int error;

	memset(&cbdata, 0, sizeof(cbdata));

	zhp = checkzpool(L, 1);
	cbdata.cmd_type = luaL_checkinteger(L, 2);
	checktrimflags(L, 3, &cbdata.trim_flags);

	if ((error = zpool_trim_one(zhp, &cbdata)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_trim_one"));
	}
	return (success(L));
}
#endif

#if __FreeBSD_version > 1500023
static int
l_zpool_ddt_prune(lua_State *L)
{
	zpool_handle_t *zhp;
	zpool_ddt_prune_unit_t unit;
	uint64_t amount;
	int error;

	zhp = checkzpool(L, 1);
	unit = luaL_checkinteger(L, 2);
	amount = luaL_checkinteger(L, 3);

	if ((error = zpool_ddt_prune(zhp, unit, amount)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_ddt_prune"));
	}
	return (success(L));
}
#endif

static int
l_zpool_vdev_online(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	vdev_state_t newstate;
	int flags, error;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);
	flags = luaL_checkinteger(L, 3);

	if ((error = zpool_vdev_online(zhp, path, flags, &newstate)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_online"));
	}
	lua_pushinteger(L, newstate);
	return (1);
}

static int
l_zpool_vdev_offline(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	boolean_t istmp;
	int error;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);
	istmp = lua_toboolean(L, 3);

	if ((error = zpool_vdev_offline(zhp, path, istmp)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_offline"));
	}
	return (success(L));
}

static int
l_zpool_vdev_attach(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *old_disk, *new_disk;
	nvlist_t *nvroot;
	boolean_t replacing; /* XXX: integer in userspace, boolean in kernel */
	boolean_t rebuild;
	int error;

	zhp = checkzpool(L, 1);
	old_disk = luaL_checkstring(L, 2);
	new_disk = luaL_checkstring(L, 3);
	nvroot = checknvlist(L, 4);
	replacing = lua_toboolean(L, 5);
	rebuild = lua_toboolean(L, 6);

	if ((error = zpool_vdev_attach(zhp, old_disk, new_disk, nvroot,
	    replacing, rebuild)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_attach"));
	}
	return (success(L));
}

static int
l_zpool_vdev_detach(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	int error;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);

	if ((error = zpool_vdev_detach(zhp, path)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_detach"));
	}
	return (success(L));
}

static int
l_zpool_vdev_remove(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	int error;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);

	if ((error = zpool_vdev_remove(zhp, path)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_remove"));
	}
	return (success(L));
}

static int
l_zpool_vdev_remove_cancel(lua_State *L)
{
	zpool_handle_t *zhp;
	int error;

	zhp = checkzpool(L, 1);

	if ((error = zpool_vdev_remove_cancel(zhp)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_remove_cancel"));
	}
	return (success(L));
}

static int
l_zpool_vdev_indirect_size(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	uint64_t size;
	int error;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);

	if ((error = zpool_vdev_indirect_size(zhp, path, &size)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_indirect_size"));
	}
	lua_pushinteger(L, size);
	return (1);
}

static inline void
checksplitflags(lua_State *L, int idx, splitflags_t *flags)
{
	luaL_checktype(L, idx, LUA_TTABLE);
	lua_getfield(L, idx, "name_flags");
	flags->name_flags = lua_tointeger(L, -1);
	lua_pop(L, 1);
#define	FLAG(name) ({ \
	lua_getfield(L, idx, #name); \
	flags->name = lua_toboolean(L, -1); \
	lua_pop(L, 1); \
})
	FLAG(dryrun);
	FLAG(import);
#undef FLAG
}

static int
l_zpool_vdev_split(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *newname;
	nvlist_t *newroot, *props;
	splitflags_t flags;
	int error;

	memset(&flags, 0, sizeof(flags));

	zhp = checkzpool(L, 1);
	newname = luaL_checkstring(L, 2);
	newroot = optnvlist(L, 3, NULL);
	props = optnvlist(L, 4, NULL);
	checksplitflags(L, 5, &flags);

	if ((error = zpool_vdev_split(zhp, __DECONST(char *, newname), &newroot,
	    props, flags)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_split"));
	}
	return (success(L));
}

static int
l_zpool_vdev_remove_wanted(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	int error;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);

	if ((error = zpool_vdev_remove_wanted(zhp, path)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_remove_wanted"));
	}
	return (success(L));
}

static int
l_zpool_vdev_fault(lua_State *L)
{
	zpool_handle_t *zhp;
	uint64_t guid;
	vdev_aux_t aux;
	int error;

	zhp = checkzpool(L, 1);
	guid = luaL_checkinteger(L, 2);
	aux = luaL_checkinteger(L, 3);

	if ((error = zpool_vdev_fault(zhp, guid, aux)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_fault"));
	}
	return (success(L));
}

static int
l_zpool_vdev_degrade(lua_State *L)
{
	zpool_handle_t *zhp;
	uint64_t guid;
	vdev_aux_t aux;
	int error;

	zhp = checkzpool(L, 1);
	guid = luaL_checkinteger(L, 2);
	aux = luaL_checkinteger(L, 3);

	if ((error = zpool_vdev_degrade(zhp, guid, aux)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_degrade"));
	}
	return (success(L));
}

static int
l_zpool_vdev_set_removed_state(lua_State *L)
{
	zpool_handle_t *zhp;
	uint64_t guid;
	vdev_aux_t aux;
	int error;

	zhp = checkzpool(L, 1);
	guid = luaL_checkinteger(L, 2);
	aux = luaL_checkinteger(L, 3);

	if ((error = zpool_vdev_set_removed_state(zhp, guid, aux)) != 0) {
		return (zpoolfail(L, zhp, error,
		    "zpool_vdev_set_removed_state"));
	}
	return (success(L));
}

static int
l_zpool_vdev_clear(lua_State *L)
{
	zpool_handle_t *zhp;
	uint64_t guid;
	int error;

	zhp = checkzpool(L, 1);
	guid = luaL_checkinteger(L, 2);

	if ((error = zpool_vdev_clear(zhp, guid)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_vdev_clear"));
	}
	return (success(L));
}

static int
l_zpool_find_vdev(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	boolean_t avail_spare, l2cache, log;
	nvlist_t *vdev;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);

	if ((vdev = zpool_find_vdev(zhp, path, &avail_spare, &l2cache, &log))
	    == NULL) {
		return (zpoolfail(L, zhp, EZFS_UNKNOWN, "zpool_find_vdev"));
	}
	pushnvlist(L, vdev);
	lua_pushboolean(L, avail_spare);
	lua_pushboolean(L, l2cache);
	lua_pushboolean(L, log);
	return (4);
}

#if __FreeBSD_version > 1500023
static int
l_zpool_find_parent_vdev(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;
	boolean_t avail_spare, l2cache, log;
	nvlist_t *vdev;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);

	if ((vdev = zpool_find_parent_vdev(zhp, path, &avail_spare, &l2cache,
	    &log)) == NULL) {
		return (zpoolfail(L, zhp, EZFS_UNKNOWN,
		    "zpool_find_parent_vdev"));
	}
	pushnvlist(L, vdev);
	lua_pushboolean(L, avail_spare);
	lua_pushboolean(L, l2cache);
	lua_pushboolean(L, log);
	return (4);
}
#endif

static int
l_zpool_find_vdev_by_physpath(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *physpath;
	boolean_t avail_spare, l2cache, log;
	nvlist_t *vdev;

	zhp = checkzpool(L, 1);
	physpath = luaL_checkstring(L, 2);

	if ((vdev = zpool_find_vdev_by_physpath(zhp, physpath, &avail_spare,
	    &l2cache, &log)) == NULL) {
		return (zpoolfail(L, zhp, EZFS_UNKNOWN,
		    "zpool_find_vdev_by_physpath"));
	}
	pushnvlist(L, vdev);
	lua_pushboolean(L, avail_spare);
	lua_pushboolean(L, l2cache);
	lua_pushboolean(L, log);
	return (4);
}

static int
l_zpool_prepare_disk(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *vdev;
	const char *prepare_str;
	char **lines;
	int nlines, error;

	lines = NULL;

	zhp = checkzpool(L, 1);
	vdev = checknvlist(L, 2);
	prepare_str = luaL_checkstring(L, 3);

	if ((error = zpool_prepare_disk(zhp, vdev, prepare_str, &lines,
	    &nlines)) != 0) {
		ASSERT0P(lines);
		ASSERT0(nlines);
		return (zpoolfail(L, zhp, error, "zpool_prepare_disk"));
	}
	ASSERT3P(lines, !=, NULL);
	lua_createtable(L, nlines, 0);
	for (int i = 0; i < nlines; i++) {
		lua_pushstring(L, lines[i]);
		lua_rawseti(L, -2, i + 1);
	}
	libzfs_free_str_array(lines, nlines);
	return (1);
}

static int
l_zpool_vdev_path_to_guid(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *path;

	zhp = checkzpool(L, 1);
	path = luaL_checkstring(L, 2);

	lua_pushinteger(L, zpool_vdev_path_to_guid(zhp, path));
	return (1);
}

static int
l_zpool_set_prop(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *propname, *propval;
	int error;

	zhp = checkzpool(L, 1);
	propname = luaL_checkstring(L, 2);
	propval = luaL_checkstring(L, 3);

	if ((error = zpool_set_prop(zhp, propname, propval)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_set_prop"));
	}
	return (success(L));
}

static int
l_zpool_get_prop(lua_State *L)
{
	char value[ZPOOL_MAXPROPLEN];
	zpool_handle_t *zhp;
	zpool_prop_t prop;
	zprop_source_t source;
	boolean_t literal;
	int error;

	value[0] = '\0';

	zhp = checkzpool(L, 1);
	prop = luaL_checkinteger(L, 2); /* XXX: wrap this prop type too? */
	literal = lua_toboolean(L, 3);

	if ((error = zpool_get_prop(zhp, prop, value, sizeof(value), &source,
	    literal)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_get_prop"));
	}
	lua_pushstring(L, value);
	lua_pushinteger(L, source);
	return (2);
}

static int
l_zpool_get_userprop(lua_State *L)
{
	char value[ZPOOL_MAXPROPLEN];
	zpool_handle_t *zhp;
	const char *propname;
	zprop_source_t source;
	int error;

	value[0] = '\0';

	zhp = checkzpool(L, 1);
	propname = luaL_checkstring(L, 2);

	if ((error = zpool_get_userprop(zhp, propname, value, sizeof(value),
	    &source)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_get_userprop"));
	}
	lua_pushstring(L, value);
	lua_pushinteger(L, source);
	return (2);
}

static int
l_zpool_get_prop_int(lua_State *L)
{
	zpool_handle_t *zhp;
	zpool_prop_t prop;
	zprop_source_t source;

	zhp = checkzpool(L, 1);
	prop = luaL_checkinteger(L, 2);

	lua_pushinteger(L, zpool_get_prop_int(zhp, prop, &source));
	lua_pushinteger(L, source);
	return (2);
}

static int
l_zpool_props_refresh(lua_State *L)
{
	zpool_handle_t *zhp;
	int error;

	zhp = checkzpool(L, 1);

	if ((error = zpool_props_refresh(zhp)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_refresh_props"));
	}
	return (success(L));
}

static int
l_zpool_get_vdev_prop(lua_State *L)
{
	char value[ZPOOL_MAXPROPLEN];
	zpool_handle_t *zhp;
	const char *vdevname, *propname;
	vdev_prop_t prop;
	zprop_source_t source;
	boolean_t literal;
	int error;

	value[0] = '\0';

	zhp = checkzpool(L, 1);
	vdevname = luaL_checkstring(L, 2);
	prop = luaL_optinteger(L, 3, 0);
	propname = luaL_optstring(L, 4, NULL);
	literal = lua_toboolean(L, 5);

	if ((error = zpool_get_vdev_prop(zhp, vdevname, prop,
	    __DECONST(char *, propname), value, sizeof(value), &source,
	    literal)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_get_vdev_prop"));
	}
	lua_pushstring(L, value);
	lua_pushinteger(L, source);
	return (2);
}

static int
l_zpool_get_all_vdev_props(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *vdevname;
	nvlist_t *props;
	int error;

	zhp = checkzpool(L, 1);
	vdevname = luaL_checkstring(L, 2);

	if ((error = zpool_get_all_vdev_props(zhp, vdevname, &props)) != 0) {
		ASSERT0P(props);
		return (zpoolfail(L, zhp, error, "zpool_get_all_vdev_props"));
	}
	ASSERT3P(props, !=, NULL);
	pushnvlist(L, props);
	return (1);
}

static int
l_zpool_set_vdev_prop(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *vdevname, *propname, *propval;
	int error;

	zhp = checkzpool(L, 1);
	vdevname = luaL_checkstring(L, 2);
	propname = luaL_checkstring(L, 3);
	propval = luaL_checkstring(L, 4);

	if ((error = zpool_set_vdev_prop(zhp, vdevname, propname, propval))
	    != 0) {
		return (zpoolfail(L, zhp, error, "zpool_set_vdev_prop"));
	}
	return (success(L));
}

static int
l_zpool_get_config(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *oldconfig, *config;

	oldconfig = config = NULL;

	zhp = checkzpool(L, 1);

	if ((config = zpool_get_config(zhp, &oldconfig)) == NULL) {
		ASSERT0P(config);
		ASSERT0P(oldconfig);
		return (zpoolfail(L, zhp, EZFS_UNKNOWN, "zpool_get_config"));
	}
	ASSERT3P(config, !=, NULL);
	pushnvlist(L, config);
	if (oldconfig != NULL) {
		pushnvlist(L, oldconfig);
		return (2);
	}
	return (1);
}

static int
l_zpool_get_features(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *features;

	zhp = checkzpool(L, 1);

	if ((features = zpool_get_features(zhp)) == NULL) {
		return (zpoolfail(L, zhp, EZFS_UNKNOWN, "zpool_get_features"));
	}
	pushnvlist(L, features);
	return (1);
}

static int
l_zpool_refresh_stats(lua_State *L)
{
	zpool_handle_t *zhp;
	boolean_t missing;
	int error;

	zhp = checkzpool(L, 1);

	if ((error = zpool_refresh_stats(zhp, &missing)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_refresh_stats"));
	}
	lua_pushboolean(L, missing);
	return (1);
}

#if __FreeBSD_version > 1600001
static int
l_zpool_refresh_stats_from_handle(lua_State *L)
{
	zpool_handle_t *zhp, *fromzhp;

	zhp = checkzpool(L, 1);
	fromzhp = checkzpool(L, 2);

	zpool_refresh_stats_from_handle(zhp, fromzhp);
	return (0);
}
#endif

static int
l_zpool_get_errlog(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *errlist;
	int error;

	errlist = NULL;

	zhp = checkzpool(L, 1);

	if ((error = zpool_get_errlog(zhp, &errlist)) != 0) {
		ASSERT0P(errlist);
		return (zpoolfail(L, zhp, error, "zpool_get_errlog"));
	}
	if (errlist == NULL) {
		return (0);
	}
	pushnvlist(L, errlist);
	return (1);
}

#if __FreeBSD_version > 1500023
static int
l_zpool_add_propname(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *propname;

	zhp = checkzpool(L, 1);
	propname = luaL_checkstring(L, 2);

	zpool_add_propname(zhp, propname);
	return (0);
}
#endif

static int
l_zpool_export(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *message;
	boolean_t force;
	int error;

	zhp = checkzpool(L, 1);
	force = lua_toboolean(L, 2);
	message = luaL_checkstring(L, 3);

	if ((error = zpool_export(zhp, force, message)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_export"));
	}
	return (success(L));
}

static int
l_zpool_export_force(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *message;
	int error;

	zhp = checkzpool(L, 1);
	message = luaL_checkstring(L, 2);

	if ((error = zpool_export_force(zhp, message)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_export_force"));
	}
	return (success(L));
}

static int
l_zpool_upgrade(lua_State *L)
{
	zpool_handle_t *zhp;
	uint64_t new_version;
	int error;

	zhp = checkzpool(L, 1);
	new_version = luaL_checkinteger(L, 2);

	if ((error = zpool_upgrade(zhp, new_version)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_upgrade"));
	}
	return (success(L));
}

static int
l_zpool_get_history(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *history;
	uint64_t offset;
	boolean_t eof;
	int error;

	history = NULL;
	eof = B_FALSE;

	zhp = checkzpool(L, 1);
	offset = luaL_optinteger(L, 2, 0);

	if ((error = zpool_get_history(zhp, &history, &offset, &eof)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_get_history"));
	}
	pushnvlist(L, history);
	lua_pushinteger(L, offset);
	lua_pushboolean(L, eof);
	return (3);
}

static int
l_zpool_obj_to_path_ds(lua_State *L)
{
	char path[PATH_MAX];
	zpool_handle_t *zhp;
	uint64_t dsobj, obj;

	path[0] = '\0';

	zhp = checkzpool(L, 1);
	dsobj = luaL_checkinteger(L, 2);
	obj = luaL_checkinteger(L, 3);

	zpool_obj_to_path_ds(zhp, dsobj, obj, path, sizeof(path));
	lua_pushstring(L, path);
	return (1);
}

static int
l_zpool_obj_to_path(lua_State *L)
{
	char path[PATH_MAX];
	zpool_handle_t *zhp;
	uint64_t dsobj, obj;

	path[0] = '\0';

	zhp = checkzpool(L, 1);
	dsobj = luaL_checkinteger(L, 2);
	obj = luaL_checkinteger(L, 3);

	zpool_obj_to_path(zhp, dsobj, obj, path, sizeof(path));
	lua_pushstring(L, path);
	return (1);
}

static int
l_zpool_checkpoint(lua_State *L)
{
	zpool_handle_t *zhp;
	int error;

	zhp = checkzpool(L, 1);

	if ((error = zpool_checkpoint(zhp)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_checkpoint"));
	}
	return (success(L));
}

static int
l_zpool_discard_checkpoint(lua_State *L)
{
	zpool_handle_t *zhp;
	int error;

	zhp = checkzpool(L, 1);

	if ((error = zpool_discard_checkpoint(zhp)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_discard_checkpoint"));
	}
	return (success(L));
}

#if __FreeBSD_version > 1500023
static int
l_zpool_prefetch(lua_State *L)
{
	zpool_handle_t *zhp;
	zpool_prefetch_type_t type;
	int error;

	zhp = checkzpool(L, 1);
	type = luaL_checkinteger(L, 2);

	if ((error = zpool_prefetch(zhp, type)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_prefetch"));
	}
	return (success(L));
}
#endif

static int
l_zpool_prop_get_feature(lua_State *L)
{
	char value[ZPOOL_MAXPROPLEN];
	zpool_handle_t *zhp;
	const char *propname;
	int error;

	value[0] = '\0';

	zhp = checkzpool(L, 1);
	propname = luaL_checkstring(L, 2);

	if ((error = zpool_prop_get_feature(zhp, propname, value,
	    sizeof(value))) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_prop_get_feature"));
	}
	lua_pushstring(L, value);
	return (1);
}

static int
l_zvol_volsize_to_reservation(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *props;
	uint64_t volsize, reservation;

	zhp = checkzpool(L, 1);
	volsize = luaL_checkinteger(L, 2);
	props = checknvlist(L, 3);

	lua_pushinteger(L, zvol_volsize_to_reservation(zhp, volsize, props));
	return (1);
}

static int
l_zpool_set_bootenv(lua_State *L)
{
	zpool_handle_t *zhp;
	const nvlist_t *envmap;
	int error;

	zhp = checkzpool(L, 1);
	envmap = checknvlist(L, 2);

	if ((error = zpool_set_bootenv(zhp, envmap)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_set_bootenv"));
	}
	return (success(L));
}

static int
l_zpool_get_bootenv(lua_State *L)
{
	zpool_handle_t *zhp;
	nvlist_t *envmap;
	int error;

	envmap = NULL;

	zhp = checkzpool(L, 1);

	if ((error = zpool_get_bootenv(zhp, &envmap)) != 0) {
		ASSERT0P(envmap);
		return (zpoolfail(L, zhp, error, "zpool_get_bootenv"));
	}
	ASSERT3P(envmap, !=, NULL);
	pushnvlist(L, envmap);
	return (1);
}

#if __FreeBSD_version < 1500019
#define	zpool_enable_datasets(zhp, mntopts, flags, nthr) ({ \
	(void) nthr; \
	zpool_enable_datasets((zhp), (mntopts), (flags)); \
})
#endif

static int
l_zpool_enable_datasets(lua_State *L)
{
	zpool_handle_t *zhp;
	const char *mntopts;
	uint_t nthr;
	int flags, error;

	zhp = checkzpool(L, 1);
	mntopts = luaL_optstring(L, 2, NULL);
	flags = luaL_checkinteger(L, 3);
	nthr = luaL_checkinteger(L, 4);

	if ((error = zpool_enable_datasets(zhp, mntopts, flags, nthr)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_enable_datasets"));
	}
	return (success(L));
}

static int
l_zpool_disable_datasets(lua_State *L)
{
	zpool_handle_t *zhp;
	boolean_t force;
	int error;

	zhp = checkzpool(L, 1);
	force = lua_toboolean(L, 2);

	if ((error = zpool_disable_datasets(zhp, force)) != 0) {
		return (zpoolfail(L, zhp, error, "zpool_disable_datasets"));
	}
	return (success(L));
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
	{"iter_children", l_zfs_iter_children},
	{"iter_dependents", l_zfs_iter_dependents},
	{"iter_filesystems", l_zfs_iter_filesystems},
	{"iter_snapshots", l_zfs_iter_snapshots},
	{"iter_snapshots_sorted", l_zfs_iter_snapshots_sorted},
	{"iter_snapspec", l_zfs_iter_snapspec},
	{"iter_bookmarks", l_zfs_iter_bookmarks},
	{"iter_children_v2", l_zfs_iter_children_v2},
	{"iter_dependents_v2", l_zfs_iter_dependents_v2},
	{"iter_filesystems_v2", l_zfs_iter_filesystems_v2},
	{"iter_snapshots_v2", l_zfs_iter_snapshots_v2},
	{"iter_snapshots_sorted_v2", l_zfs_iter_snapshots_sorted_v2},
	{"iter_snapspec_v2", l_zfs_iter_snapspec_v2},
	{"iter_bookmarks_v2", l_zfs_iter_bookmarks_v2},
	{"iter_mounted", l_zfs_iter_mounted},
	{"wait_status", l_zfs_wait_status},
	{"crypto_get_encryption_root", l_zfs_crypto_get_encryption_root},
	{"crypto_load_key", l_zfs_crypto_load_key},
	{"crypto_unload_key", l_zfs_crypto_unload_key},
	{"crypto_rewrap", l_zfs_crypto_rewrap},
#if __FreeBSD_version > 1500044
	{"is_encrypted", l_zfs_is_encrypted},
#endif
	{"destroy", l_zfs_destroy},
	{"destroy_snaps", l_zfs_destroy_snaps},
	{"clone", l_zfs_clone},
	{"rollback", l_zfs_rollback},
	{"rename", l_zfs_rename},
	{"send", l_zfs_send},
	{"send_one", l_zfs_send_one},
	{"send_progress", l_zfs_send_progress},
	{"send_saved", l_zfs_send_saved},
	{"promote", l_zfs_promote},
	{"hold", l_zfs_hold},
	{"hold_nvl", l_zfs_hold_nvl},
	{"release", l_zfs_release},
	{"get_holds", l_zfs_get_holds},
#if 0 /* TODO */
	{"userspace", l_zfs_userspace},
#endif
	/* TODO: fsacl methods */
	{"refresh_properties", l_zfs_refresh_properties},
	{"parent_name", l_zfs_parent_name},
	{"spa_version", l_zfs_spa_version},
	{"is_mounted", l_zfs_is_mounted},
	{"mount", l_zfs_mount},
	{"mount_at", l_zfs_mount_at},
	{"unmount", l_zfs_unmount},
	{"unmountall", l_zfs_unmountall},
	/* TODO: sharing */
	{"jail", l_zfs_jail},
	{NULL, NULL}
};

static const struct luaL_Reg l_zpool_meta[] = {
	{"__close", l_zpool_close},
	{"__gc", l_zpool_close},
	{"close", l_zpool_close},
	{"get_name", l_zpool_get_name},
	{"get_state", l_zpool_get_state},
	{"get_state_str", l_zpool_get_state_str},
	{"get_status", l_zpool_get_status},
#if 0
	{"get_handle", l_zpool_get_handle},
#endif
	{"wait", l_zpool_wait},
	{"wait_status", l_zpool_wait_status},
	{"destroy", l_zpool_destroy},
	{"add", l_zpool_add},
	{"scan", l_zpool_scan},
#if __FreeBSD_version > 1500056
	{"scan_range", l_zpool_scan_range},
	{"initialize_one", l_zpool_initialize_one},
#endif
	{"initialize", l_zpool_initialize},
	{"initialize_wait", l_zpool_initialize_wait},
	{"trim", l_zpool_trim},
	{"clear", l_zpool_clear},
	{"reguid", l_zpool_reguid},
#if __FreeBSD_version > 1500023
	{"set_guid", l_zpool_set_guid},
#endif
	{"reopen_one", l_zpool_reopen_one},
#if __FreeBSD_version > 1500056
	{"collect_leaves", l_zpool_collect_leaves},
#endif
	{"sync_one", l_zpool_sync_one},
#if __FreeBSD_version > 1500056
	{"trim_one", l_zpool_trim_one},
#endif
#if __FreeBSD_version > 1500023
	{"ddt_prune", l_zpool_ddt_prune},
#endif
	{"vdev_online", l_zpool_vdev_online},
	{"vdev_offline", l_zpool_vdev_offline},
	{"vdev_attach", l_zpool_vdev_attach},
	{"vdev_detach", l_zpool_vdev_detach},
	{"vdev_remove", l_zpool_vdev_remove},
	{"vdev_remove_cancel", l_zpool_vdev_remove_cancel},
	{"vdev_indirect_size", l_zpool_vdev_indirect_size},
	{"vdev_split", l_zpool_vdev_split},
	{"vdev_remove_wanted", l_zpool_vdev_remove_wanted},
	{"vdev_fault", l_zpool_vdev_fault},
	{"vdev_degrade", l_zpool_vdev_degrade},
	{"vdev_set_removed_state", l_zpool_vdev_set_removed_state},
	{"vdev_clear", l_zpool_vdev_clear},
	{"find_vdev", l_zpool_find_vdev},
#if __FreeBSD_version > 1500023
	{"find_parent_vdev", l_zpool_find_parent_vdev},
#endif
	{"find_vdev_by_physpath", l_zpool_find_vdev_by_physpath},
	{"prepare_disk", l_zpool_prepare_disk},
	{"vdev_path_to_guid", l_zpool_vdev_path_to_guid},
	{"set_prop", l_zpool_set_prop},
	{"get_prop", l_zpool_get_prop},
	{"get_userprop", l_zpool_get_userprop},
	{"get_prop_int", l_zpool_get_prop_int},
	{"props_refresh", l_zpool_props_refresh},
	{"get_vdev_prop", l_zpool_get_vdev_prop},
	{"get_all_vdev_props", l_zpool_get_all_vdev_props},
	{"set_vdev_prop", l_zpool_set_vdev_prop},
	{"get_config", l_zpool_get_config},
	{"get_features", l_zpool_get_features},
	{"refresh_stats", l_zpool_refresh_stats},
#if __FreeBSD_version > 1600001
	{"refresh_stats_from_handle", l_zpool_refresh_stats_from_handle},
#endif
	{"get_errlog", l_zpool_get_errlog},
#if __FreeBSD_version > 1500023
	{"add_propname", l_zpool_add_propname},
#endif
	{"export", l_zpool_export},
	{"export_force", l_zpool_export_force},
	{"upgrade", l_zpool_upgrade},
	{"get_history", l_zpool_get_history},
	{"obj_to_path_ds", l_zpool_obj_to_path_ds},
	{"obj_to_path", l_zpool_obj_to_path},
	{"checkpoint", l_zpool_checkpoint},
	{"discard_checkpoint", l_zpool_discard_checkpoint},
#if __FreeBSD_version > 1500023
	{"prefetch", l_zpool_prefetch},
#endif
	/* TODO: proplist stuff */
	{"prop_get_feature", l_zpool_prop_get_feature},
	{"zvol_volsize_to_reservation", l_zvol_volsize_to_reservation},
	{"set_bootenv", l_zpool_set_bootenv},
	{"get_bootenv", l_zpool_get_bootenv},
	{"enable_datasets", l_zpool_enable_datasets},
	{"disable_datasets", l_zpool_disable_datasets},
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
	{"open_pool", l_zpool_open},
	{"open_pool_canfail", l_zpool_open_canfail},
	{"iter_pools", l_zpool_iter},
	{"create_pool", l_zpool_create},
	{"label_disk", l_zpool_label_disk},
	{"prepare_and_label_disk", l_zpool_prepare_and_label_disk},
	{"import", l_zpool_import},
	{"import_props", l_zpool_import_props},
	{"vdev_name", l_zpool_vdev_name},
	{"events_next", l_zpool_events_next},
	{"events_clear", l_zpool_events_clear},
	{"events_seek", l_zpool_events_seek},
	{"explain_recover", l_zpool_explain_recover},
	{"foreach_mountpoint", l_zfs_foreach_mountpoint},
	{"open", l_zfs_open},
	{"iter_root", l_zfs_iter_root},
	{"crypto_create", l_zfs_crypto_create},
	{"crypto_clone_check", l_zfs_crypto_clone_check},
	{"crypto_attempt_load_keys", l_zfs_crypto_attempt_load_keys},
	/* TODO: more zprop related functions */
	{"valid_proplist", l_zfs_valid_proplist},
	{"path_to_zhandle", l_zfs_path_to_zhandle},
	{"dataset_exists", l_zfs_dataset_exists},
	{"create", l_zfs_create},
	{"create_ancestors", l_zfs_create_ancestors},
#if __FreeBSD_version > 1600013
	{"create_ancestors_props", l_zfs_create_ancestors_props},
#endif
	{"destroy_snaps_nvl", l_zfs_destroy_snaps_nvl},
	{"snapshot", l_zfs_snapshot},
	{"snapshot_nvl", l_zfs_snapshot_nvl},
	{"send_resume", l_zfs_send_resume},
	{"send_resume_token_to_nvlist", l_zfs_send_resume_token_to_nvlist},
	{"receive", l_zfs_receive},
	{"is_mounted", l_is_mounted},
	{"nicestrtonum", l_zfs_nicestrtonum},
	{"pool_in_use", l_zpool_in_use},
	{"nextboot", l_zpool_nextboot},
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

	DEFINE(ZFS_ITER_RECURSE);
	DEFINE(ZFS_ITER_ARGS_CAN_BE_PATHS);
	DEFINE(ZFS_ITER_PROP_LISTSNAPS);
	DEFINE(ZFS_ITER_DEPTH_LIMIT);
	DEFINE(ZFS_ITER_RECVD_PROPS);
	DEFINE(ZFS_ITER_LITERAL_PROPS);
	DEFINE(ZFS_ITER_SIMPLE);
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
