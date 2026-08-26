/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>

#include <devstat.h>
#include <kvm.h>

#include <lua.h>
#include <lauxlib.h>

#include "libdevstat/lua_devstat.h"
#include "libkvm/lua_kvm.h"
#include "sys/time/lua_time.h"
#include "utils.h"

#define STATINFO_METATABLE "struct statinfo *"

int luaopen_devstat(lua_State *);

static inline int
devstatfail(lua_State *L)
{
	luaL_pushfail(L);
	lua_pushstring(L, devstat_errbuf);
	return (2);
}

static int
l_devstat_getnumdevs(lua_State *L)
{
	kvm_t *kd;
	int n;

	kd = luaL_opt(L, checkkvm, 1, NULL);

	if ((n = devstat_getnumdevs(kd)) == -1) {
		return (devstatfail(L));
	}
	lua_pushinteger(L, n);
	return (1);
}

static int
l_devstat_getgeneration(lua_State *L)
{
	kvm_t *kd;
	long gen;

	kd = luaL_opt(L, checkkvm, 1, NULL);

	if ((gen = devstat_getgeneration(kd)) == -1) {
		return (devstatfail(L));
	}
	lua_pushinteger(L, gen);
	return (1);
}

static int
l_devstat_getversion(lua_State *L)
{
	kvm_t *kd;
	int v;

	kd = luaL_opt(L, checkkvm, 1, NULL);

	if ((v = devstat_getversion(kd)) == -1) {
		return (devstatfail(L));
	}
	lua_pushinteger(L, v);
	return (1);
}

static int
l_devstat_checkversion(lua_State *L)
{
	kvm_t *kd;

	kd = luaL_opt(L, checkkvm, 1, NULL);

	if (devstat_checkversion(kd) == -1) {
		return (devstatfail(L));
	}
	return (success(L));
}

static int
l_devstat_getdevs(lua_State *L)
{
	struct statinfo *si;
	kvm_t *kd;

	kd = luaL_opt(L, checkkvm, 1, NULL);

	si = lua_newuserdatauv(L, sizeof (*si), 0);
	memset(si, 0, sizeof(*si));
	if ((si->dinfo = malloc(sizeof (*si->dinfo))) == NULL) {
		return (fail(L, ENOMEM));
	}
	luaL_setmetatable(L, STATINFO_METATABLE);
	if (devstat_getdevs(kd, si) == -1) {
		return (devstatfail(L));
	}
	return (1);
}

static int
l_statinfo_gc(lua_State *L)
{
	struct statinfo *si;

	si = luaL_checkudata(L, 1, STATINFO_METATABLE);

	free(si->dinfo);
	return (0);
}

static int
l_statinfo_index(lua_State *L)
{
	struct statinfo *si;
	const char *field;

	si = luaL_checkudata(L, 1, STATINFO_METATABLE);
	field = luaL_checkstring(L, 2);

	if (strcmp(field, "cp_time") == 0) {
		lua_createtable(L, CPUSTATES, 0);
		for (int i = 0; i < CPUSTATES; i++) {
			lua_pushinteger(L, si->cp_time[i]);
			lua_rawseti(L, -2, i + 1);
		}
		return (1);
	}
	if (strcmp(field, "tk_nin") == 0) {
		lua_pushinteger(L, si->tk_nin);
		return (1);
	}
	if (strcmp(field, "tk_nout") == 0) {
		lua_pushinteger(L, si->tk_nout);
		return (1);
	}
	if (strcmp(field, "dinfo") == 0) {
		struct devinfo *di = si->dinfo;

		lua_newtable(L);
		lua_createtable(L, di->numdevs, 0);
		for (int i = 0; i < di->numdevs; i++)  {
			newref(L, 1, &di->devices[i], DEVSTAT_METATABLE);
			lua_rawseti(L, -2, i + 1);
		}
		lua_setfield(L, -2, "devices");
		lua_pushinteger(L, di->generation);
		lua_setfield(L, -2, "generation");
		return (1);
	}
	if (strcmp(field, "snap_time") == 0) {
		lua_pushnumber(L, si->snap_time);
		return (1);
	}
	return (0);
}

/* TODO: selectdevs, buildmatch */

static int
l_devstat_compute_statistics(lua_State *L)
{
	struct devstat *current, *previous;
	long double etime;
	union {
		long double	real;
		uint64_t	integer;
	} metric[DSM_MAX];

	current = checkdevstat(L, 1);
	previous = luaL_opt(L, checkdevstat, 2, NULL);
	etime = luaL_checknumber(L, 3);

	if (devstat_compute_statistics(current, previous, etime,
#define METRIC(name) (DSM_ ## name), (&metric[(DSM_ ## name)])
	    METRIC(TOTAL_BYTES),
	    METRIC(TOTAL_BYTES_READ),
	    METRIC(TOTAL_BYTES_WRITE),
	    METRIC(TOTAL_TRANSFERS),
	    METRIC(TOTAL_TRANSFERS_READ),
	    METRIC(TOTAL_TRANSFERS_WRITE),
	    METRIC(TOTAL_TRANSFERS_OTHER),
	    METRIC(TOTAL_BLOCKS),
	    METRIC(TOTAL_BLOCKS_READ),
	    METRIC(TOTAL_BLOCKS_WRITE),
	    METRIC(KB_PER_TRANSFER),
	    METRIC(KB_PER_TRANSFER_READ),
	    METRIC(KB_PER_TRANSFER_WRITE),
	    METRIC(TRANSFERS_PER_SECOND),
	    METRIC(TRANSFERS_PER_SECOND_READ),
	    METRIC(TRANSFERS_PER_SECOND_WRITE),
	    METRIC(TRANSFERS_PER_SECOND_OTHER),
	    METRIC(MB_PER_SECOND),
	    METRIC(MB_PER_SECOND_READ),
	    METRIC(MB_PER_SECOND_WRITE),
	    METRIC(BLOCKS_PER_SECOND),
	    METRIC(BLOCKS_PER_SECOND_READ),
	    METRIC(BLOCKS_PER_SECOND_WRITE),
	    METRIC(MS_PER_TRANSACTION),
	    METRIC(MS_PER_TRANSACTION_READ),
	    METRIC(MS_PER_TRANSACTION_WRITE),
	    METRIC(TOTAL_BYTES_FREE),
	    METRIC(TOTAL_TRANSFERS_FREE),
	    METRIC(TOTAL_BLOCKS_FREE),
	    METRIC(KB_PER_TRANSFER_FREE),
	    METRIC(MB_PER_SECOND_FREE),
	    METRIC(TRANSFERS_PER_SECOND_FREE),
	    METRIC(BLOCKS_PER_SECOND_FREE),
	    METRIC(MS_PER_TRANSACTION_OTHER),
	    METRIC(MS_PER_TRANSACTION_FREE),
	    METRIC(BUSY_PCT),
	    METRIC(QUEUE_LENGTH),
	    METRIC(TOTAL_DURATION),
	    METRIC(TOTAL_DURATION_READ),
	    METRIC(TOTAL_DURATION_WRITE),
	    METRIC(TOTAL_DURATION_FREE),
	    METRIC(TOTAL_DURATION_OTHER),
	    METRIC(TOTAL_BUSY_TIME),
#undef METRIC
	    DSM_NONE) == -1) {
		return (devstatfail(L));
	}
	lua_createtable(L, 0, DSM_MAX);
#define INTEGER_METRIC(name) ({ \
	lua_pushinteger(L, metric[(DSM_ ## name)].integer); \
	lua_setfield(L, -2, #name); \
})
#define REAL_METRIC(name) ({ \
	lua_pushnumber(L, metric[(DSM_ ## name)].real); \
	lua_setfield(L, -2, #name); \
})
	INTEGER_METRIC(TOTAL_BYTES);
	INTEGER_METRIC(TOTAL_BYTES_READ);
	INTEGER_METRIC(TOTAL_BYTES_WRITE);
	INTEGER_METRIC(TOTAL_TRANSFERS);
	INTEGER_METRIC(TOTAL_TRANSFERS_READ);
	INTEGER_METRIC(TOTAL_TRANSFERS_WRITE);
	INTEGER_METRIC(TOTAL_TRANSFERS_OTHER);
	INTEGER_METRIC(TOTAL_BLOCKS);
	INTEGER_METRIC(TOTAL_BLOCKS_READ);
	INTEGER_METRIC(TOTAL_BLOCKS_WRITE);
	REAL_METRIC(KB_PER_TRANSFER);
	REAL_METRIC(KB_PER_TRANSFER_READ);
	REAL_METRIC(KB_PER_TRANSFER_WRITE);
	REAL_METRIC(TRANSFERS_PER_SECOND);
	REAL_METRIC(TRANSFERS_PER_SECOND_READ);
	REAL_METRIC(TRANSFERS_PER_SECOND_WRITE);
	REAL_METRIC(TRANSFERS_PER_SECOND_OTHER);
	REAL_METRIC(MB_PER_SECOND);
	REAL_METRIC(MB_PER_SECOND_READ);
	REAL_METRIC(MB_PER_SECOND_WRITE);
	REAL_METRIC(BLOCKS_PER_SECOND);
	REAL_METRIC(BLOCKS_PER_SECOND_READ);
	REAL_METRIC(BLOCKS_PER_SECOND_WRITE);
	REAL_METRIC(MS_PER_TRANSACTION);
	REAL_METRIC(MS_PER_TRANSACTION_READ);
	REAL_METRIC(MS_PER_TRANSACTION_WRITE);
	INTEGER_METRIC(TOTAL_BYTES_FREE);
	INTEGER_METRIC(TOTAL_TRANSFERS_FREE);
	INTEGER_METRIC(TOTAL_BLOCKS_FREE);
	REAL_METRIC(KB_PER_TRANSFER_FREE);
	REAL_METRIC(MB_PER_SECOND_FREE);
	REAL_METRIC(TRANSFERS_PER_SECOND_FREE);
	REAL_METRIC(BLOCKS_PER_SECOND_FREE);
	REAL_METRIC(MS_PER_TRANSACTION_OTHER);
	REAL_METRIC(MS_PER_TRANSACTION_FREE);
	REAL_METRIC(BUSY_PCT);
	INTEGER_METRIC(QUEUE_LENGTH);
	REAL_METRIC(TOTAL_DURATION);
	REAL_METRIC(TOTAL_DURATION_READ);
	REAL_METRIC(TOTAL_DURATION_WRITE);
	REAL_METRIC(TOTAL_DURATION_FREE);
	REAL_METRIC(TOTAL_DURATION_OTHER);
	REAL_METRIC(TOTAL_BUSY_TIME);
#undef INTEGER_METRIC
#undef REAL_METRIC
	return (1);
}

static int
l_devstat_compute_etime(lua_State *L)
{
	struct bintime cur_time, prev_time, *ptp;
	long double etime;

	checkbintime(L, 1, &cur_time);
	if (lua_isnoneornil(L, 2)) {
		ptp = NULL;
	} else {
		checkbintime(L, 2, &prev_time);
		ptp = &prev_time;
	}

	etime = devstat_compute_etime(&cur_time, ptp);
	lua_pushnumber(L, etime);
	return (1);
}

static int
l_devstat_index(lua_State *L)
{
	struct devstat *ds;
	const char *field;

	ds = checkdevstat(L, 1);
	field = luaL_checkstring(L, 2);

#define IFIELD(fieldname) ({ \
	if (strcmp(field, #fieldname) == 0) { \
		lua_pushinteger(L, ds->fieldname); \
		return (1); \
	} \
})
#define TFIELD(fieldname) ({ \
	if (strcmp(field, #fieldname) == 0) { \
		pushbintime(L, &ds->fieldname); \
		return (1); \
	} \
})
#define AFIELD(fieldname, n) ({ \
	if (strcmp(field, #fieldname) == 0) { \
		lua_createtable(L, (n), 0); \
		for (int i = 0; i < n; i++) { \
			lua_pushinteger(L, ds->fieldname[i]); \
			lua_rawseti(L, -2, i + 1); \
		} \
		return (1); \
	} \
})
	IFIELD(start_count);
	IFIELD(end_count);
	TFIELD(busy_from);
	IFIELD(device_number);
	if (strcmp(field, "device_name") == 0) {
		lua_pushstring(L, ds->device_name);
		return (1);
	}
	IFIELD(unit_number);
	AFIELD(bytes, DEVSTAT_N_TRANS_FLAGS);
	AFIELD(operations, DEVSTAT_N_TRANS_FLAGS);
	if (strcmp(field, "duration") == 0) {
		lua_createtable(L, DEVSTAT_N_TRANS_FLAGS, 0);
		for (int i = 0; i < DEVSTAT_N_TRANS_FLAGS; i++) {
			pushbintime(L, &ds->duration[i]);
			lua_rawseti(L, -2, i + 1);
		}
		return (1);
	}
	TFIELD(busy_time);
	TFIELD(creation_time);
	IFIELD(block_size);
	AFIELD(tag_types, 3);
	IFIELD(flags);
	IFIELD(device_type);
	IFIELD(priority);
	if (strcmp(field, "id") == 0) {
		lua_pushlightuserdata(L, __DECONST(void *, ds->id));
		return (1);
	}
#undef IFIELD
#undef TFIELD
#undef AFIELD
	return (0);
}

static const struct luaL_Reg l_devstat_funcs[] = {
	{"getnumdevs", l_devstat_getnumdevs},
	{"getgeneration", l_devstat_getgeneration},
	{"getversion", l_devstat_getversion},
	{"checkversion", l_devstat_checkversion},
	{"getdevs", l_devstat_getdevs},
	{"compute_statistics", l_devstat_compute_statistics},
	{"compute_etime", l_devstat_compute_etime},
	{NULL, NULL}
};

static const struct luaL_Reg l_statinfo_meta[] = {
	{"__gc", l_statinfo_gc},
	{"__index", l_statinfo_index},
	{NULL, NULL}
};

static const struct luaL_Reg l_devstat_meta[] = {
	{"__index", l_devstat_index},
	{NULL, NULL}
};

int
luaopen_devstat(lua_State *L)
{
	luaL_newmetatable(L, DEVSTAT_METATABLE);
	luaL_setfuncs(L, l_devstat_meta, 0);

	luaL_newmetatable(L, STATINFO_METATABLE);
	luaL_setfuncs(L, l_statinfo_meta, 0);

	luaL_newlib(L, l_devstat_funcs);
	return (1);
}
