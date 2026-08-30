/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <devinfo.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define DEVINFO_SNAPSHOT_METATABLE "devinfo snapshot"

int luaopen_devinfo(lua_State *);

static int
l_devinfo_init(lua_State *L)
{
	int error;

	if ((error = devinfo_init()) != 0) {
		return (fail(L, error));
	}
	lua_newuserdatauv(L, 0, 1); /* one uservalue as a free sentinel */
	luaL_setmetatable(L, DEVINFO_SNAPSHOT_METATABLE);
	return (1);
}

static inline void
checksnapshot(lua_State *L, int idx)
{
	(void) luaL_checkudata(L, 1, DEVINFO_SNAPSHOT_METATABLE);
}

static int
l_devinfo_free(lua_State *L)
{
	checksnapshot(L, 1);

	/*
	 * Control the freeing of global state via <close> so that gc
	 * doesn't randomly clear out the next snapshot.
	 */
	if (lua_getiuservalue(L, 1, 1) == LUA_TNONE) {
		/* ^ pushed a nil, use it as a sentinel. */
		lua_setiuservalue(L, 1, 1);
		devinfo_free();
	}
	return (0);
}

static inline void
pushhandle(lua_State *L, devinfo_handle_t handle)
{
	lua_pushlightuserdata(L, (void *)handle);
}

static inline devinfo_handle_t
tohandle(lua_State *L, int idx)
{
	return ((uintptr_t)lua_touserdata(L, idx));
}

static inline void
pushdevice(lua_State *L, struct devinfo_dev *device)
{
	lua_newtable(L);

	pushhandle(L, device->dd_handle);
	lua_setfield(L, -2, "handle");
	pushhandle(L, device->dd_parent);
	lua_setfield(L, -2, "parent");

	lua_pushstring(L, device->dd_name);
	lua_setfield(L, -2, "name");
	lua_pushstring(L, device->dd_desc);
	lua_setfield(L, -2, "desc");
	lua_pushstring(L, device->dd_drivername);
	lua_setfield(L, -2, "drivername");
	lua_pushstring(L, device->dd_pnpinfo);
	lua_setfield(L, -2, "pnpinfo");
	lua_pushstring(L, device->dd_location);
	lua_setfield(L, -2, "location");

	lua_pushinteger(L, device->dd_devflags);
	lua_setfield(L, -2, "devflags");
	lua_pushinteger(L, device->dd_flags);
	lua_setfield(L, -2, "flags");
	lua_pushinteger(L, device->dd_state);
	lua_setfield(L, -2, "state");
}

static int
l_devinfo_handle_to_device(lua_State *L)
{
	devinfo_handle_t handle;
	struct devinfo_dev *device;

	checksnapshot(L, 1);
	handle = tohandle(L, 2);

	if ((device = devinfo_handle_to_device(handle)) == NULL) {
		return (0);
	}
	pushdevice(L, device);
	return (1);
}

static inline void
pushresource(lua_State *L, struct devinfo_res *resource)
{

	lua_newtable(L);

	pushhandle(L, resource->dr_handle);
	lua_setfield(L, -2, "handle");
	pushhandle(L, resource->dr_rman);
	lua_setfield(L, -2, "rman");
	pushhandle(L, resource->dr_device);
	lua_setfield(L, -2, "device");

	lua_pushinteger(L, resource->dr_start);
	lua_setfield(L, -2, "start");
	lua_pushinteger(L, resource->dr_size);
	lua_setfield(L, -2, "size");
}

static int
l_devinfo_handle_to_resource(lua_State *L)
{
	devinfo_handle_t handle;
	struct devinfo_res *resource;

	checksnapshot(L, 1);
	handle = tohandle(L, 2);

	if ((resource = devinfo_handle_to_resource(handle)) == NULL) {
		return (0);
	}
	pushresource(L, resource);
	return (1);
}

static inline void
pushrman(lua_State *L, struct devinfo_rman *rman)
{
	lua_newtable(L);

	pushhandle(L, rman->dm_handle);
	lua_setfield(L, -2, "handle");

	lua_pushinteger(L, rman->dm_start);
	lua_setfield(L, -2, "start");
	lua_pushinteger(L, rman->dm_size);
	lua_setfield(L, -2, "size");

	lua_pushstring(L, rman->dm_desc);
	lua_setfield(L, -2, "desc");
}

static int
l_devinfo_handle_to_rman(lua_State *L)
{
	devinfo_handle_t handle;
	struct devinfo_rman *rman;

	checksnapshot(L, 1);
	handle = tohandle(L, 2);

	if ((rman = devinfo_handle_to_rman(handle)) == NULL) {
		return (0);
	}
	pushrman(L, rman);
	return (1);
}

static int
device_child_cb(struct devinfo_dev *child, void *arg)
{
	lua_State *L = arg;

	pushdevice(L, child);
	lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	return (0);
}

static int
l_devinfo_device_children(lua_State *L)
{
	devinfo_handle_t handle;
	struct devinfo_dev *device;

	checksnapshot(L, 1);
	handle = tohandle(L, 2);

	if ((device = devinfo_handle_to_device(handle)) == NULL) {
		return (0);
	}
	lua_newtable(L);
	(void) devinfo_foreach_device_child(device, device_child_cb, L);
	return (1);
}

static int
device_resource_cb(struct devinfo_dev *device, struct devinfo_res *resource,
    void *arg)
{
	lua_State *L = arg;

	(void)device;
	pushresource(L, resource);
	lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	return (0);
}

static int
l_devinfo_device_resources(lua_State *L)
{
	devinfo_handle_t handle;
	struct devinfo_dev *device;

	checksnapshot(L, 1);
	handle = tohandle(L, 2);

	if ((device = devinfo_handle_to_device(handle)) == NULL) {
		return (0);
	}
	lua_newtable(L);
	(void) devinfo_foreach_device_resource(device, device_resource_cb, L);
	return (1);
}

static int
rman_resource_cb(struct devinfo_res *resource, void *arg)
{
	lua_State *L = arg;

	pushresource(L, resource);
	lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	return (0);
}

static int
l_devinfo_rman_resources(lua_State *L)
{
	devinfo_handle_t handle;
	struct devinfo_rman *rman;

	checksnapshot(L, 1);
	handle = tohandle(L, 2);

	if ((rman = devinfo_handle_to_rman(handle)) == NULL) {
		return (0);
	}
	lua_newtable(L);
	(void) devinfo_foreach_rman_resource(rman, rman_resource_cb, L);
	return (1);
}

static int
rman_cb(struct devinfo_rman *rman, void *arg)
{
	lua_State *L = arg;

	pushrman(L, rman);
	lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	return (0);
}

static int
l_devinfo_rmans(lua_State *L)
{
	checksnapshot(L, 1);

	lua_newtable(L);
	(void) devinfo_foreach_rman(rman_cb, L);
	return (1);
}

static const struct luaL_Reg l_devinfo_funcs[] = {
	{"snapshot", l_devinfo_init},
	{NULL, NULL}
};

static const struct luaL_Reg l_devinfo_snapshot_meta[] = {
	{"__close", l_devinfo_free},
	{"__gc", l_devinfo_free},
	{"free", l_devinfo_free}, /* an explicit name for convenience */
	{"handle_to_device", l_devinfo_handle_to_device},
	{"handle_to_resource", l_devinfo_handle_to_resource},
	{"handle_to_rman", l_devinfo_handle_to_rman},
	{"device_children", l_devinfo_device_children},
	{"device_resources", l_devinfo_device_resources},
	{"rman_resources", l_devinfo_rman_resources},
	{"rmans", l_devinfo_rmans},
	{NULL, NULL}
};

int
luaopen_devinfo(lua_State *L)
{
	luaL_newmetatable(L, DEVINFO_SNAPSHOT_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_devinfo_snapshot_meta, 0);

	luaL_newlib(L, l_devinfo_funcs);

	pushhandle(L, DEVINFO_ROOT_DEVICE);
	lua_setfield(L, -2, "ROOT_DEVICE");

	return (1);
}
