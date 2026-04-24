/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/zfs_context.h>
#include <libnvpair.h>
#include <libzfs_core.h>

#include <lua.h>
#include <lauxlib.h>

#include "../../libnvpair/lua_nvpair.h"
#include "utils.h"

int luaopen_zfs_core(lua_State *);

static int
l_lzc_init(lua_State *L __unused)
{
	libzfs_core_init();
	return (0);
}

static int
l_lzc_fini(lua_State *L __unused)
{
	libzfs_core_fini();
	return (0);
}

static int
l_lzc_snapshot(lua_State *L)
{
	nvlist_t *snaps, *props, *errlist;
	int error;

	snaps = checknvlist(L, 1);
	props = optnvlist(L, 2, NULL);

	if ((error = lzc_snapshot(snaps, props, &errlist)) != 0) {
		fail(L, error);
		if (errlist != NULL) {
			pushnvlist(L, errlist);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_create(lua_State *L)
{
	const char *dsname;
	enum lzc_dataset_type type;
	nvlist_t *props;
	uint8_t *wkeydata;
	size_t wkeylen;
	int error;

	dsname = luaL_checkstring(L, 1);
	type = luaL_checkinteger(L, 2);
	props = checknvlist(L, 3);
	wkeydata = __DECONST(uint8_t *, luaL_optlstring(L, 4, NULL, &wkeylen));

	if ((error = lzc_create(dsname, type, props, wkeydata, wkeylen)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_promote(lua_State *L)
{
	char snapnamebuf[ZFS_MAX_DATASET_NAME_LEN];
	const char *dsname;
	int error;

	dsname = luaL_checkstring(L, 1);

	snapnamebuf[0] = '\0';
	if ((error = lzc_promote(dsname, snapnamebuf, sizeof(snapnamebuf)))
	    != 0) {
		fail(L, error);
		lua_pushstring(L, snapnamebuf);
		lua_replace(L, -3);
		return (3);
	}
	return (success(L));
}

static int
l_lzc_destroy_snaps(lua_State *L)
{
	nvlist_t *snaps, *errlist;
	boolean_t defer;
	int error;

	snaps = checknvlist(L, 1);
	defer = lua_toboolean(L, 2);

	if ((error = lzc_destroy_snaps(snaps, defer, &errlist)) != 0) {
		fail(L, error);
		if (errlist != NULL) {
			pushnvlist(L, errlist);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_bookmark(lua_State *L)
{
	nvlist_t *bookmarks, *errlist;
	int error;

	bookmarks = checknvlist(L, 1);

	if ((error = lzc_bookmark(bookmarks, &errlist)) != 0) {
		fail(L, error);
		if (errlist != NULL) {
			new(L, errlist, NVLIST_METATABLE);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_get_bookmarks(lua_State *L)
{
	const char *dsname;
	nvlist_t *props, *bookmarks;
	int error;

	dsname = luaL_checkstring(L, 1);
	props = checknvlist(L, 2);

	if ((error = lzc_get_bookmarks(dsname, props, &bookmarks)) != 0) {
		ASSERT0P(bookmarks);
		return (fail(L, error));
	}
	ASSERT3P(bookmarks, !=, NULL);
	pushnvlist(L, bookmarks);
	return (1);
}

static int
l_lzc_get_bookmark_props(lua_State *L)
{
	const char *bookmark;
	nvlist_t *props;
	int error;

	bookmark = luaL_checkstring(L, 1);

	if ((error = lzc_get_bookmark_props(bookmark, &props)) != 0) {
		ASSERT0P(props);
		return (fail(L, error));
	}
	ASSERT3P(props, !=, NULL);
	pushnvlist(L, props);
	return (1);
}

static int
l_lzc_destroy_bookmarks(lua_State *L)
{
	nvlist_t *bookmarks, *errlist;
	int error;

	bookmarks = checknvlist(L, 1);

	if ((error = lzc_destroy_bookmarks(bookmarks, &errlist)) != 0) {
		fail(L, error);
		if (errlist != NULL) {
			pushnvlist(L, errlist);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_load_key(lua_State *L)
{
	const char *dsname;
	boolean_t noop;
	uint8_t *wkeydata;
	size_t wkeylen;
	int error;

	dsname = luaL_checkstring(L, 1);
	noop = lua_toboolean(L, 2);
	wkeydata = __DECONST(uint8_t *, luaL_checklstring(L, 3, &wkeylen));

	if ((error = lzc_load_key(dsname, noop, wkeydata, wkeylen)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_unload_key(lua_State *L)
{
	const char *dsname;
	int error;

	dsname = luaL_checkstring(L, 1);

	if ((error = lzc_unload_key(dsname)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_change_key(lua_State *L)
{
	const char *dsname;
	uint64_t crypt_cmd;
	nvlist_t *props;
	uint8_t *wkeydata;
	size_t wkeylen;
	int error;

	dsname = luaL_checkstring(L, 1);
	crypt_cmd = luaL_checkinteger(L, 2);
	props = optnvlist(L, 3, NULL);
	wkeydata = __DECONST(uint8_t *, luaL_optlstring(L, 4, NULL, &wkeylen));

	if ((error = lzc_change_key(dsname, crypt_cmd, props, wkeydata,
	    wkeylen)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_initialize(lua_State *L)
{
	const char *poolname;
	pool_initialize_func_t cmd_type;
	nvlist_t *vdevs, *errlist;
	int error;

	poolname = luaL_checkstring(L, 1);
	cmd_type = luaL_checkinteger(L, 2);
	vdevs = checknvlist(L, 3);

	if ((error = lzc_initialize(poolname, cmd_type, vdevs, &errlist))
	    != 0) {
		fail(L, error);
		if (errlist != NULL) {
			pushnvlist(L, errlist);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_trim(lua_State *L)
{
	const char *poolname;
	pool_trim_func_t cmd_type;
	uint64_t rate;
	boolean_t secure;
	nvlist_t *vdevs, *errlist;
	int error;

	poolname = luaL_checkstring(L, 1);
	cmd_type = luaL_checkinteger(L, 2);
	rate = luaL_checkinteger(L, 3);
	secure = lua_toboolean(L, 4);
	vdevs = checknvlist(L, 5);

	if ((error = lzc_trim(poolname, cmd_type, rate, secure, vdevs,
	    &errlist)) != 0) {
		fail(L, error);
		if (errlist != 0) {
			pushnvlist(L, errlist);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_redact(lua_State *L)
{
	const char *snapshot, *bookname;
	nvlist_t *snapnv;
	int error;

	snapshot = luaL_checkstring(L, 1);
	bookname = luaL_checkstring(L, 2);
	snapnv = checknvlist(L, 3);

	if ((error = lzc_redact(snapshot, bookname, snapnv)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_snaprange_space(lua_State *L)
{
	const char *firstsnap, *lastsnap;
	uint64_t used;
	int error;

	firstsnap = luaL_checkstring(L, 1);
	lastsnap = luaL_checkstring(L, 2);

	if ((error = lzc_snaprange_space(firstsnap, lastsnap, &used)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, used);
	return (1);
}

static int
l_lzc_hold(lua_State *L)
{
	nvlist_t *holds, *errlist;
	int cleanup_fd, error;

	holds = checknvlist(L, 1);
	if (lua_isnoneornil(L, 2)) {
		cleanup_fd = -1;
	} else {
		cleanup_fd = checkfd(L, 3);
	}

	if ((error = lzc_hold(holds, cleanup_fd, &errlist)) != 0) {
		fail(L, error);
		if (errlist != NULL) {
			pushnvlist(L, errlist);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_release(lua_State *L)
{
	nvlist_t *holds, *errlist;
	int error;

	holds = checknvlist(L, 1);

	if ((error = lzc_release(holds, &errlist)) != 0) {
		fail(L, error);
		if (errlist != NULL) {
			pushnvlist(L, errlist);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (errlist != NULL) {
		pushnvlist(L, errlist);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_get_holds(lua_State *L)
{
	const char *snapname;
	nvlist_t *holds;
	int error;

	snapname = luaL_checkstring(L, 1);

	if ((error = lzc_get_holds(snapname, &holds)) != 0) {
		ASSERT0P(holds);
		return (fail(L, error));
	}
	ASSERT3P(holds, !=, NULL);
	pushnvlist(L, holds);
	return (1);
}

#if __FreeBSD_version > 1500035
static int
l_lzc_get_props(lua_State *L)
{
	const char *poolname;
	nvlist_t *props;
	int error;

	poolname = luaL_checkstring(L, 1);

	if ((error = lzc_get_props(poolname, &props)) != 0) {
		ASSERT0P(props);
		return (fail(L, error));
	}
	ASSERT3P(props, !=, NULL);
	pushnvlist(L, props);
	return (1);
}
#endif

static int
l_lzc_send(lua_State *L)
{
	const char *snapname, *from;
	enum lzc_send_flags flags;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	from = luaL_optstring(L, 2, NULL);
	fd = checkfd(L, 3);
	flags = luaL_optinteger(L, 4, 0);

	if ((error = lzc_send(snapname, from, fd, flags)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_send_resume(lua_State *L)
{
	const char *snapname, *from;
	uint64_t resumeobj, resumeoff;
	enum lzc_send_flags flags;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	from = luaL_optstring(L, 2, NULL);
	fd = checkfd(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	resumeobj = luaL_checkinteger(L, 5);
	resumeoff = luaL_checkinteger(L, 6);

	if ((error = lzc_send_resume(snapname, from, fd, flags, resumeobj,
	    resumeoff)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_send_redacted(lua_State *L)
{
	const char *snapname, *from, *redactbook;
	enum lzc_send_flags flags;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	from = luaL_optstring(L, 2, NULL);
	fd = checkfd(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	redactbook = luaL_optstring(L, 5, NULL);

	if ((error = lzc_send_redacted(snapname, from, fd, flags, redactbook))
	    != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_send_resume_redacted(lua_State *L)
{
	const char *snapname, *from, *redactbook;
	uint64_t resumeobj, resumeoff;
	enum lzc_send_flags flags;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	from = luaL_optstring(L, 2, NULL);
	fd = checkfd(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	resumeobj = luaL_optinteger(L, 5, 0);
	resumeoff = luaL_optinteger(L, 6, 0);
	redactbook = luaL_optstring(L, 7, NULL);

	if ((error = lzc_send_resume_redacted(snapname, from, fd, flags,
	    resumeobj, resumeoff, redactbook)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_send_space(lua_State *L)
{
	const char *snapname, *from;
	uint64_t space;
	enum lzc_send_flags flags;
	int error;

	snapname = luaL_checkstring(L, 1);
	from = luaL_optstring(L, 2, NULL);
	flags = luaL_optinteger(L, 3, 0);

	if ((error = lzc_send_space(snapname, from, flags, &space)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, space);
	return (1);
}

static int
l_lzc_send_space_resume_redacted(lua_State *L)
{
	const char *snapname, *from, *redactbook;
	uint64_t resumeobj, resumeoff, resume_bytes, space;
	enum lzc_send_flags flags;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	from = luaL_optstring(L, 2, NULL);
	flags = luaL_optinteger(L, 3, 0);
	resumeobj = luaL_optinteger(L, 4, 0);
	resumeoff = luaL_optinteger(L, 5, 0);
	resume_bytes = luaL_optinteger(L, 6, 0);
	redactbook = luaL_optstring(L, 7, NULL);
	if (!lua_isnoneornil(L, 8)) {
		fd = -1;
	} else {
		fd = checkfd(L, 8);
	}

	if ((error = lzc_send_space_resume_redacted(snapname, from, flags,
	    resumeobj, resumeoff, resume_bytes, redactbook, fd, &space)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, space);
	return (1);
}

#if __FreeBSD_version > 1600013
static int
l_lzc_send_progress(lua_State *L)
{
	const char *snapname;
	uint64_t bytes_written, blocks_visited;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	fd = checkfd(L, 2);

	if ((error = lzc_send_progress(snapname, fd, &bytes_written,
	    &blocks_visited)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, bytes_written);
	lua_pushinteger(L, blocks_visited);
	return (2);
}
#endif

static int
l_lzc_receive(lua_State *L)
{
	const char *snapname, *origin;
	nvlist_t *props;
	boolean_t force, raw;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	props = optnvlist(L, 2, NULL);
	origin = luaL_optstring(L, 3, NULL);
	force = lua_toboolean(L, 4);
	raw = lua_toboolean(L, 5);
	fd = checkfd(L, 6);

	if ((error = lzc_receive(snapname, props, origin, force, raw, fd))
	    != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_receive_resumable(lua_State *L)
{
	const char *snapname, *origin;
	nvlist_t *props;
	boolean_t force, raw;
	int fd, error;

	snapname = luaL_checkstring(L, 1);
	props = optnvlist(L, 2, NULL);
	origin = luaL_optstring(L, 3, NULL);
	force = lua_toboolean(L, 4);
	raw = lua_toboolean(L, 5);
	fd = checkfd(L, 6);

	if ((error = lzc_receive(snapname, props, origin, force, raw, fd))
	    != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

#if 0 /* TODO: unmarshall a dmu_replay_record */
static int
l_lzc_receive_with_header(lua_State *L)
{
}

static int
l_lzc_receive_one(lua_State *L)
{
}

static int
l_lzc_receive_with_cmdprops(lua_State *L)
{
}

static int
l_lzc_receive_with_heal(lua_State *L)
{
}
#endif

static int
l_lzc_exists(lua_State *L)
{
	const char *dsname;

	dsname = luaL_checkstring(L, 1);

	lua_pushboolean(L, lzc_exists(dsname));
	return (1);
}

static int
l_lzc_rollback(lua_State *L)
{
	char snapnamebuf[ZFS_MAX_DATASET_NAME_LEN];
	const char *dsname;
	int error;

	snapnamebuf[0] = '\0';

	dsname = luaL_checkstring(L, 1);

	if ((error = lzc_rollback(dsname, snapnamebuf, sizeof(snapnamebuf)))
	    != 0) {
		return (fail(L, error));
	}
	lua_pushstring(L, snapnamebuf);
	return (1);
}

static int
l_lzc_rollback_to(lua_State *L)
{
	const char *dsname, *snapname;
	int error;

	dsname = luaL_checkstring(L, 1);
	snapname = luaL_checkstring(L, 2);

	if ((error = lzc_rollback_to(dsname, snapname)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_rename(lua_State *L)
{
	const char *source, *target;
	int error;

	source = luaL_checkstring(L, 1);
	target = luaL_checkstring(L, 2);

	if ((error = lzc_rename(source, target)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_destroy(lua_State *L)
{
	const char *dsname;
	int error;

	dsname = luaL_checkstring(L, 1);

	if ((error = lzc_destroy(dsname)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_channel_program(lua_State *L)
{
	const char *pool, *program;
	nvlist_t *argnvl, *resultnvl;
	uint64_t instrlimit, memlimit;
	int error;

	pool = luaL_checkstring(L, 1);
	program = luaL_checkstring(L, 2);
	instrlimit = luaL_optinteger(L, 3, ZCP_DEFAULT_INSTRLIMIT);
	memlimit = luaL_optinteger(L, 4, ZCP_DEFAULT_MEMLIMIT);
	argnvl = checknvlist(L, 5);

	if ((error = lzc_channel_program(pool, program, instrlimit, memlimit,
	    argnvl, &resultnvl)) != 0) {
		fail(L, error);
		if (resultnvl != NULL) {
			pushnvlist(L, resultnvl);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (resultnvl != NULL) {
		pushnvlist(L, resultnvl);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_channel_program_nosync(lua_State *L)
{
	const char *pool, *program;
	nvlist_t *argnvl, *resultnvl;
	uint64_t instrlimit, memlimit;
	int error;

	pool = luaL_checkstring(L, 1);
	program = luaL_checkstring(L, 2);
	instrlimit = luaL_optinteger(L, 3, ZCP_DEFAULT_INSTRLIMIT);
	memlimit = luaL_optinteger(L, 4, ZCP_DEFAULT_MEMLIMIT);
	argnvl = checknvlist(L, 5);

	if ((error = lzc_channel_program_nosync(pool, program, instrlimit,
	    memlimit, argnvl, &resultnvl)) != 0) {
		fail(L, error);
		if (resultnvl != NULL) {
			pushnvlist(L, resultnvl);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (resultnvl != NULL) {
		pushnvlist(L, resultnvl);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_sync(lua_State *L)
{
	const char *pool;
	nvlist_t *args, *results;
	int error;

	results = NULL; /* XXX: currently unused by libzfs_core */

	pool = luaL_checkstring(L, 1);
	args = optnvlist(L, 2, NULL);

	if ((error = lzc_sync(pool, args, &results)) != 0) {
		fail(L, error);
		if (results != NULL) {
			pushnvlist(L, results);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (results != NULL) {
		pushnvlist(L, results);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_reopen(lua_State *L)
{
	const char *pool;
	boolean_t scrub_restart;
	int error;

	pool = luaL_checkstring(L, 1);
	scrub_restart = lua_toboolean(L, 2);

	if ((error = lzc_reopen(pool, scrub_restart)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_pool_checkpoint(lua_State *L)
{
	const char *pool;
	int error;

	pool = luaL_checkstring(L, 1);

	if ((error = lzc_pool_checkpoint(pool)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_pool_checkpoint_discard(lua_State *L)
{
	const char *pool;
	int error;

	pool = luaL_checkstring(L, 1);

	if ((error = lzc_pool_checkpoint_discard(pool)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_wait(lua_State *L)
{
	const char *pool;
	zpool_wait_activity_t activity;
	boolean_t waited;
	int error;

	pool = luaL_checkstring(L, 1);
	activity = luaL_checkinteger(L, 2);

	if ((error = lzc_wait(pool, activity, &waited)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, waited);
	return (1);
}

static int
l_lzc_wait_tag(lua_State *L)
{
	const char *pool;
	zpool_wait_activity_t activity;
	uint64_t tag;
	boolean_t waited;
	int error;

	pool = luaL_checkstring(L, 1);
	activity = luaL_checkinteger(L, 2);
	tag = luaL_optinteger(L, 3, 0);

	if ((error = lzc_wait_tag(pool, activity, tag, &waited)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, waited);
	return (1);
}

#if __FreeBSD_version > 1500023
static int
l_lzc_pool_prefetch(lua_State *L)
{
	const char *pool;
	zpool_prefetch_type_t type;
	int error;

	pool = luaL_checkstring(L, 1);
	type = luaL_checkinteger(L, 2);

	if ((error = lzc_pool_prefetch(pool, type)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}
#endif

static int
l_lzc_wait_fs(lua_State *L)
{
	const char *fsname;
	zfs_wait_activity_t activity;
	boolean_t waited;
	int error;

	fsname = luaL_checkstring(L, 1);
	activity = luaL_checkinteger(L, 2);

	if ((error = lzc_wait_fs(fsname, activity, &waited)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, waited);
	return (1);
}

static int
l_lzc_set_bootenv(lua_State *L)
{
	const char *pool;
	nvlist_t *props;
	int error;

	pool = luaL_checkstring(L, 1);
	props = checknvlist(L, 2);

	if ((error = lzc_set_bootenv(pool, props)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_lzc_get_bootenv(lua_State *L)
{
	const char *pool;
	nvlist_t *props;
	int error;

	pool = luaL_checkstring(L, 1);

	if ((error = lzc_get_bootenv(pool, &props)) != 0) {
		ASSERT0P(props);
		return (fail(L, error));
	}
	ASSERT3P(props, !=, NULL);
	pushnvlist(L, props);
	return (1);
}

static int
l_lzc_get_vdev_prop(lua_State *L)
{
	const char *pool;
	nvlist_t *innvl, *outnvl;
	int error;

	outnvl = NULL;

	pool = luaL_checkstring(L, 1);
	innvl = checknvlist(L, 2);

	if ((error = lzc_get_vdev_prop(pool, innvl, &outnvl)) != 0) {
		fail(L, error);
		if (outnvl != NULL) {
			pushnvlist(L, outnvl);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (outnvl != NULL) {
		pushnvlist(L, outnvl);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_set_vdev_prop(lua_State *L)
{
	const char *pool;
	nvlist_t *innvl, *outnvl;
	int error;

	outnvl = NULL;

	pool = luaL_checkstring(L, 1);
	innvl = checknvlist(L, 2);

	if ((error = lzc_set_vdev_prop(pool, innvl, &outnvl)) != 0) {
		fail(L, error);
		if (outnvl != NULL) {
			pushnvlist(L, outnvl);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (outnvl != NULL) {
		pushnvlist(L, outnvl);
		return (1);
	}
	return (success(L));
}

static int
l_lzc_scrub(lua_State *L)
{
	const char *pool;
	nvlist_t *args, *result;
	int error;

	result = NULL; /* XXX: currently unused by kernel */

	pool = luaL_checkstring(L, 1);
	args = checknvlist(L, 2);

	if ((error = lzc_scrub(ZFS_IOC_POOL_SCRUB, pool, args, &result)) != 0) {
		fail(L, error);
		if (result != NULL) {
			pushnvlist(L, result);
			lua_replace(L, -3);
		}
		return (3);
	}
	if (result != NULL) {
		pushnvlist(L, result);
		return (1);
	}
	return (success(L));
}

#if __FreeBSD_version > 1500023
static int
l_lzc_ddt_prune(lua_State *L)
{
	const char *pool;
	zpool_ddt_prune_unit_t unit;
	uint64_t amount;
	int error;

	pool = luaL_checkstring(L, 1);
	unit = luaL_checkinteger(L, 2);
	amount = luaL_checkinteger(L, 3);

	if ((error = lzc_ddt_prune(pool, unit, amount)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}
#endif

static const struct luaL_Reg l_lzc_funcs[] = {
	{"init", l_lzc_init},
	{"fini", l_lzc_fini},
	{"snapshot", l_lzc_snapshot},
	{"create", l_lzc_create},
	{"promote", l_lzc_promote},
	{"destroy_snaps", l_lzc_destroy_snaps},
	{"bookmark", l_lzc_bookmark},
	{"get_bookmarks", l_lzc_get_bookmarks},
	{"get_bookmark_props", l_lzc_get_bookmark_props},
	{"destroy_bookmarks", l_lzc_destroy_bookmarks},
	{"load_key", l_lzc_load_key},
	{"unload_key", l_lzc_unload_key},
	{"change_key", l_lzc_change_key},
	{"initialize", l_lzc_initialize},
	{"trim", l_lzc_trim},
	{"redact", l_lzc_redact},
	{"snaprange_space", l_lzc_snaprange_space},
	{"hold", l_lzc_hold},
	{"release", l_lzc_release},
	{"get_holds", l_lzc_get_holds},
#if __FreeBSD_version > 1500035
	{"get_props", l_lzc_get_props},
#endif
#if 0 /* dubious */
	{"send_wrapper", l_lzc_send_wrapper},
#endif
	{"send", l_lzc_send},
	{"send_resume", l_lzc_send_resume},
	{"send_redacted", l_lzc_send_redacted},
	{"send_resume_redacted", l_lzc_send_resume_redacted},
	{"send_space", l_lzc_send_space},
	{"send_space_resume_redacted", l_lzc_send_space_resume_redacted},
#if __FreeBSD_version > 1600013
	{"send_progress", l_lzc_send_progress},
#endif
	{"receive", l_lzc_receive},
	{"receive_resumable", l_lzc_receive_resumable},
#if 0 /* TODO */
	{"receive_with_header", l_lzc_receive_with_header},
	{"receive_one", l_lzc_receive_one},
	{"receive_with_cmdprops", l_lzc_receive_with_cmdprops},
	{"receive_with_heal", l_lzc_receive_with_heal},
#endif
	{"exists", l_lzc_exists},
	{"rollback", l_lzc_rollback},
	{"rollback_to", l_lzc_rollback_to},
	{"rename", l_lzc_rename},
	{"destroy", l_lzc_destroy},
	{"channel_program", l_lzc_channel_program},
	{"channel_program_nosync", l_lzc_channel_program_nosync},
	{"sync", l_lzc_sync},
	{"reopen", l_lzc_reopen},
	{"pool_checkpoint", l_lzc_pool_checkpoint},
	{"pool_checkpoint_discard", l_lzc_pool_checkpoint_discard},
	{"wait", l_lzc_wait},
	{"wait_tag", l_lzc_wait_tag},
#if __FreeBSD_version > 1500023
	{"pool_prefetch", l_lzc_pool_prefetch},
#endif
	{"wait_fs", l_lzc_wait_fs},
	{"set_bootenv", l_lzc_set_bootenv},
	{"get_bootenv", l_lzc_get_bootenv},
	{"get_vdev_prop", l_lzc_get_vdev_prop},
	{"set_vdev_prop", l_lzc_set_vdev_prop},
	{"scrub", l_lzc_scrub},
#if __FreeBSD_version > 1500023
	{"ddt_prune", l_lzc_ddt_prune},
#endif
	{NULL, NULL}
};

int
luaopen_zfs_core(lua_State *L)
{
	luaL_newlib(L, l_lzc_funcs);

#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(LZC_DATSET_TYPE_ZFS);
	DEFINE(LZC_DATSET_TYPE_ZVOL);

	DEFINE(LZC_SEND_FLAG_EMBED_DATA);
	DEFINE(LZC_SEND_FLAG_LARGE_BLOCK);
	DEFINE(LZC_SEND_FLAG_COMPRESS);
	DEFINE(LZC_SEND_FLAG_RAW);
	DEFINE(LZC_SEND_FLAG_SAVED);

	DEFINE(POOL_INITIALIZE_START);
	DEFINE(POOL_INITIALIZE_CANCEL);
	DEFINE(POOL_INITIALIZE_SUSPEND);
	DEFINE(POOL_INITIALIZE_UNINIT);
	DEFINE(POOL_INITIALIZE_FUNCS);

	DEFINE(POOL_TRIM_START);
	DEFINE(POOL_TRIM_CANCEL);
	DEFINE(POOL_TRIM_SUSPEND);
	DEFINE(POOL_TRIM_FUNCS);

	DEFINE(ZPOOL_WAIT_CKPT_DISCARD);
	DEFINE(ZPOOL_WAIT_FREE);
	DEFINE(ZPOOL_WAIT_INITIALIZE);
	DEFINE(ZPOOL_WAIT_REPLACE);
	DEFINE(ZPOOL_WAIT_REMOVE);
	DEFINE(ZPOOL_WAIT_RESILVER);
	DEFINE(ZPOOL_WAIT_SCRUB);
	DEFINE(ZPOOL_WAIT_TRIM);
#if __FreeBSD_version > 1500003
	DEFINE(ZPOOL_WAIT_RAIDZ_EXPAND);
#endif
	DEFINE(ZPOOL_WAIT_NUM_ACTIVITIES);

	DEFINE(ZFS_WAIT_DELETEQ);
	DEFINE(ZFS_WAIT_NUM_ACTIVITIES);

#if __FreeBSD_version > 1500023
	DEFINE(ZPOOL_PREFETCH_NONE);
	DEFINE(ZPOOL_PREFETCH_DDT);
#if __FreeBSD_version > 1600004
	DEFINE(ZPOOL_PREFETCH_BRT);
#endif

	DEFINE(ZPOOL_DDT_PRUNE_NONE);
	DEFINE(ZPOOL_DDT_PRUNE_AGE);
	DEFINE(ZPOOL_DDT_PRUNE_PERCENTAGE);
#endif
#undef DEFINE
	return (1);
}
