/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/disk.h>
#include <sys/queue.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NOTYET
#include <devstat.h>
#endif
#include <libgeom.h>

#include <lua.h>
#include <lauxlib.h>

#ifdef NOTYET
#include "libdevstat/lua_devstat.h"
#endif
#include "utils.h"

#ifdef NOTYET
#define GEOM_STATS_METATABLE "geom stats"
#define GEOM_STATS_SNAPSHOT_METATABLE "geom stats snapshot"
#endif
#define GCTL_REQ_METATABLE "struct gctl_req *"
#define GIDENT_METATABLE "struct gident *"
#define GMESH_METATABLE "struct gmesh *"
#define GCLASS_METATABLE "struct gclass *"
#define GGEOM_METATABLE "struct ggeom *"
#define GCONSUMER_METATABLE "struct gconsumer *"
#define GPROVIDER_METATABLE "struct gprovider *"

int luaopen_geom(lua_State *);

#ifdef NOTYET
static int
l_geom_stats_open(lua_State *L)
{
	if (geom_stats_open() != 0) {
		return (fail(L, errno));
	}
	(void) lua_newuserdatauv(L, 0, 0);
	luaL_setmetatable(L, GEOM_STATS_METATABLE);
	return (1);
}

static int
l_geom_stats_close(lua_State *L __unused)
{
	geom_stats_close();
	return (0);
}

static int
l_geom_stats_resync(lua_State *L)
{
	(void) luaL_checkudata(L, 1, GEOM_STATS_METATABLE);

	geom_stats_resync();
	return (0);
}

static int
l_geom_stats_snapshot_get(lua_State *L)
{
	void *snapshot;

	(void) luaL_checkudata(L, 1, GEOM_STATS_METATABLE);

	if ((snapshot = geom_stats_snapshot_get()) == NULL) {
		return (fail(L, errno));
	}
	return (newref(L, 1, snapshot, GEOM_STATS_SNAPSHOT_METATABLE));
}

static int
l_geom_stats_snapshot_free(lua_State *L)
{
	void *snapshot;

	snapshot = checkcookienull(L, 1, GEOM_STATS_SNAPSHOT_METATABLE);

	if (snapshot != NULL) {
		geom_stats_snapshot_free(snapshot);
		setcookie(L, 1, NULL);
	}
	return (0);
}

static int
l_geom_stats_snapshot_timestamp(lua_State *L)
{
	struct timespec timestamp;
	void *snapshot;

	snapshot = checkcookie(L, 1, GEOM_STATS_SNAPSHOT_METATABLE);

	geom_stats_snapshot_timestamp(snapshot, &timestamp);
	lua_pushinteger(L, timestamp.tv_sec);
	lua_pushinteger(L, timestamp.tv_nsec);
	return (2);
}

static int
l_geom_stats_snapshot_reset(lua_State *L)
{
	void *snapshot;

	snapshot = checkcookie(L, 1, GEOM_STATS_SNAPSHOT_METATABLE);

	geom_stats_snapshot_reset(snapshot);
	return (0);
}

static int
l_geom_stats_snapshot_next(lua_State *L)
{
	struct devstat *stat;
	void *snapshot;

	snapshot = checkcookie(L, 1, GEOM_STATS_SNAPSHOT_METATABLE);

	if ((stat = geom_stats_snapshot_next(snapshot)) == NULL) {
		return (0);
	}
	return (newref(L, 1, stat, DEVSTAT_METATABLE);
}
#endif

static int
l_gctl_get_handle(lua_State *L)
{
	struct gctl_req *req;

	if ((req = gctl_get_handle()) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, req, GCTL_REQ_METATABLE));
}

static int
l_gctl_ro_param(lua_State *L)
{
	struct gctl_req *req;
	const char *name;
	const void *value;
	size_t len;

	req = checkcookie(L, 1, GCTL_REQ_METATABLE);
	name = luaL_checkstring(L, 2);
	value = luaL_checklstring(L, 3, &len);

	gctl_ro_param(req, name, len, value);
	return (0);
}

#ifdef NOTYET
/* FIXME: the C API doesn't map well to Lua because of rw params */
static int
l_gctl_rw_param(lua_State *L)
{
	struct gctl_req *req;
	const char *name;
	const void *init;
	void *value;
	size_t len;

	req = checkcookie(L, 1, GCTL_REQ_METATABLE);
	name = luaL_checkstring(L, 2);
	init = luaL_checklstring(L, 3, &len);

	if ((value = calloc(1, len)) == NULL) {
		return (fail(L, errno));
	}
	gctl_rw_param(req, name, len, value);
	/* FIXME: The value isn't updated until issue. */
	lua_pushlstring(L, value, len);
	return (1);
}
#endif

static int
l_gctl_issue(lua_State *L)
{
	struct gctl_req *req;
	const char *errmsg;

	req = checkcookie(L, 1, GCTL_REQ_METATABLE);

	if ((errmsg = gctl_issue(req)) != NULL) {
		luaL_pushfail(L);
		lua_pushstring(L, errmsg);
		return (2);
	}
	return (success(L));
}

static int
l_gctl_free(lua_State *L)
{
	struct gctl_req *req;

	req = checkcookie(L, 1, GCTL_REQ_METATABLE);

	gctl_free(req);
	return (0);
}

static int
l_gctl_dump(lua_State *L)
{
	struct gctl_req *req;
	luaL_Stream *s;

	req = checkcookie(L, 1, GCTL_REQ_METATABLE);
	s = luaL_checkudata(L, 2, LUA_FILEHANDLE);

	gctl_dump(req, s->f);
	return (0);
}

static int
l_geom_getxml(lua_State *L)
{
	char *confxml;

	if ((confxml = geom_getxml()) == NULL) {
		return (fail(L, errno));
	}

	lua_pushstring(L, confxml);
	free(confxml);
	return (1);
}

static int
l_geom_getxml_geom(lua_State *L)
{
	const char *class, *geom;
	char *confxml;
	bool parents;

	class = luaL_checkstring(L, 1);
	geom = luaL_checkstring(L, 2);
	parents = lua_toboolean(L, 3);

	if ((confxml = geom_getxml_geom(class, geom, parents)) == NULL) {
		return (fail(L, errno));
	}

	lua_pushstring(L, confxml);
	free(confxml);
	return (1);
}

static int
l_geom_xml2tree(lua_State *L)
{
	struct gmesh *gmp;
	const char *confxml;
	char *p;
	int error;

	confxml = luaL_checkstring(L, 1);

	gmp = lua_newuserdatauv(L, sizeof (*gmp), 0);
	/* XXX: API bug: the string should be const */
	p = __DECONST(char *, confxml);
	if ((error = geom_xml2tree(gmp, p)) != 0) {
		return (fail(L, error));
	}
	luaL_setmetatable(L, GMESH_METATABLE);
	return (1);
}

static int
l_geom_gettree(lua_State *L)
{
	struct gmesh *gmp;
	int error;

	gmp = lua_newuserdatauv(L, sizeof (*gmp), 0);
	if ((error = geom_gettree(gmp)) != 0) {
		return (fail(L, error));
	}
	luaL_setmetatable(L, GMESH_METATABLE);
	return (1);
}

static int
l_geom_gettree_geom(lua_State *L)
{
	const char *class, *geom;
	struct gmesh *gmp;
	bool parents;
	int error;

	class = luaL_checkstring(L, 1);
	geom = luaL_checkstring(L, 2);
	parents = lua_toboolean(L, 3);

	gmp = lua_newuserdatauv(L, sizeof (*gmp), 0);
	if ((error = geom_gettree_geom(gmp, class, geom, parents)) != 0) {
		return (fail(L, error));
	}
	luaL_setmetatable(L, GMESH_METATABLE);
	return (1);
}

static int
l_geom_deletetree(lua_State *L)
{
	struct gmesh *gmp;

	gmp = luaL_checkudata(L, 1, GMESH_METATABLE);

	geom_deletetree(gmp);
	return (0);
}

#define FIELD(name) ({ \
	lua_pushstring(L, #name); \
	lua_rawseti(L, -2, luaL_len(L, -2) + 1); \
})
#define GEOM_STRUCT_PAIRS(lname, uname, ...) \
static int \
l_g ## lname ## _pairs_iter(lua_State *L) \
{ \
	const char *field; \
\
	(void) checkcookie(L, lua_upvalueindex(1), G ## uname ## _METATABLE); \
	field = luaL_checkstring(L, 2); \
\
	lua_pushvalue(L, 2); \
	if (lua_next(L, 1) == 0) { \
		return (0); \
	} \
	lua_pushvalue(L, -1); /* copy for return pair */ \
	if (l_g ## lname ## _index(L) != 0) { \
		return (0); \
	} \
	return (2); \
} \
\
static int \
l_g ## lname ## _pairs(lua_State *L) \
{ \
	(void) checkcookie(L, 1, G ## uname ## _METATABLE); \
\
	lua_pushvalue(L, 1); \
	lua_pushcclosure(L, l_g ## lname ## _pairs_iter, 1); \
	lua_newtable(L); \
	__VA_ARGS__; \
	lua_pushnil(L); \
	return (3); \
}

static int
l_gident_index(lua_State *L)
{
	struct gident *gid;
	const char *field;

	gid = checkcookie(L, 1, GIDENT_METATABLE);
	field = luaL_checkstring(L, 2);

	if (strcmp(field, "id") == 0) {
		lua_pushlightuserdata(L, gid->lg_id);
		return (1);
	}
	if (strcmp(field, "ptr") == 0) {
		void *ptr = gid->lg_ptr;

		switch (gid->lg_what) {
		case ISCLASS:
			return (newref(L, 1, ptr, GCLASS_METATABLE));
		case ISGEOM:
			return (newref(L, 1, ptr, GGEOM_METATABLE));
		case ISPROVIDER:
			return (newref(L, 1, ptr, GPROVIDER_METATABLE));
		case ISCONSUMER:
			return (newref(L, 1, ptr, GCONSUMER_METATABLE));
		default:
			lua_pushlightuserdata(L, gid->lg_ptr);
			return (1);
		}
		__builtin_unreachable();
	}
	if (strcmp(field, "what") == 0) {
		switch (gid->lg_what) {
		case ISCLASS:
			lua_pushliteral(L, "CLASS");
			return (1);
		case ISGEOM:
			lua_pushliteral(L, "GEOM");
			return (1);
		case ISPROVIDER:
			lua_pushliteral(L, "PROVIDER");
			return (1);
		case ISCONSUMER:
			lua_pushliteral(L, "CONSUMER");
			return (1);
		default:
			lua_pushinteger(L, gid->lg_what);
			return (1);
		}
		__builtin_unreachable();
	}
	return (0);
}

GEOM_STRUCT_PAIRS(ident, IDENT,
    FIELD(id),
    FIELD(ptr),
    FIELD(what)
)

static int
l_gclass_iter(lua_State *L)
{
	struct gmesh *mesh;
	struct gclass *classp, *nextp;

	mesh = luaL_checkudata(L, 1, GMESH_METATABLE);
	classp = checkcookienull(L, 2, GCLASS_METATABLE);

	nextp = classp == NULL ? LIST_FIRST(&mesh->lg_class) :
	    LIST_NEXT(classp, lg_class);
	return (nextp == NULL ? 0 : newref(L, 1, nextp, GCLASS_METATABLE));
}

static int
l_gmesh_class(lua_State *L)
{
	(void) luaL_checkudata(L, 1, GMESH_METATABLE);

	lua_pushcfunction(L, l_gclass_iter);
	lua_pushvalue(L, 1);
	new(L, NULL, GCLASS_METATABLE);
	return (3);
}

static int
l_gident_iter(lua_State *L)
{
	struct gmesh *mesh;
	struct gident *ident;
	int i;

	mesh = luaL_checkudata(L, 1, GMESH_METATABLE);
	i = luaL_checkinteger(L, lua_upvalueindex(1));

	ident = &mesh->lg_ident[i];
	if (ident->lg_id == NULL) {
		return (0);
	}
	lua_pushinteger(L, i + 1);
	lua_replace(L, lua_upvalueindex(1));
	return (newref(L, 1, ident, GIDENT_METATABLE));
}

static int
l_gmesh_ident(lua_State *L)
{
	(void) luaL_checkudata(L, 1, GMESH_METATABLE);

	lua_pushinteger(L, 0);
	lua_pushcclosure(L, l_gident_iter, 1);
	lua_pushvalue(L, 1);
	new(L, NULL, GIDENT_METATABLE);
	return (3);
}

static int
l_geom_lookupid(lua_State *L)
{
	const struct gmesh *mesh;
	const void *id;
	struct gident *ident;

	mesh = luaL_checkudata(L, 1, GMESH_METATABLE);
	id = lua_touserdata(L, 2);

	if ((ident = geom_lookupid(mesh, id)) == NULL) {
		return (0);
	}
	return (newref(L, 1, ident, GIDENT_METATABLE));
}

static int
l_ggeom_iter(lua_State *L)
{
	struct gclass *classp;
	struct ggeom *gp, *nextp;

	classp = checkcookie(L, 1, GCLASS_METATABLE);
	gp = checkcookienull(L, 2, GGEOM_METATABLE);

	nextp = gp == NULL ? LIST_FIRST(&classp->lg_geom) :
	    LIST_NEXT(gp, lg_geom);
	return (nextp == NULL ? 0 : newref(L, 1, nextp, GGEOM_METATABLE));
}

static int
l_gclass_geom(lua_State *L)
{
	struct gclass *classp;

	classp = checkcookie(L, 1, GCLASS_METATABLE);

	lua_pushcfunction(L, l_ggeom_iter);
	lua_pushvalue(L, 1);
	new(L, NULL, GGEOM_METATABLE);
	return (3);
}

static inline void
pushconf(lua_State *L, struct gconf *conf)
{
	struct gconfig *config;

	/* TODO: would be nice to cache this in a uservalue */
	lua_newtable(L);
	LIST_FOREACH(config, conf, lg_config) {
		if (config->lg_val == NULL) {
			lua_pushboolean(L, true);
		} else {
			lua_pushstring(L, config->lg_val);
		}
		lua_setfield(L, -2, config->lg_name);
	}
}

static int
l_gclass_index(lua_State *L)
{
	struct gclass *classp;
	const char *field;

	classp = checkcookie(L, 1, GCLASS_METATABLE);
	field = luaL_checkstring(L, 2);

	if (strcmp(field, "id") == 0) {
		lua_pushlightuserdata(L, classp->lg_id);
		return (1);
	}
	if (strcmp(field, "name") == 0) {
		lua_pushstring(L, classp->lg_name);
		return (1);
	}
	if (strcmp(field, "geom") == 0) {
		lua_pushcfunction(L, l_gclass_geom);
		return (1);
	}
	if (strcmp(field, "config") == 0) {
		pushconf(L, &classp->lg_config);
		return (1);
	}
	return (0);
}

GEOM_STRUCT_PAIRS(class, CLASS,
    FIELD(id),
    FIELD(name),
    FIELD(geom),
    FIELD(config)
)

static int
l_gconsumer_iter(lua_State *L)
{
	struct ggeom *gp;
	struct gconsumer *cp, *nextp;

	gp = checkcookie(L, 1, GGEOM_METATABLE);
	cp = checkcookienull(L, 2, GCONSUMER_METATABLE);

	nextp = cp == NULL ? LIST_FIRST(&gp->lg_consumer) :
	    LIST_NEXT(cp, lg_consumer);
	return (nextp == NULL ? 0 : newref(L, 1, nextp, GCONSUMER_METATABLE));
}

static int
l_ggeom_consumer(lua_State *L)
{
	struct ggeom *gp;

	gp = checkcookie(L, 1, GGEOM_METATABLE);

	lua_pushcfunction(L, l_gconsumer_iter);
	lua_pushvalue(L, 1);
	new(L, NULL, GCONSUMER_METATABLE);
	return (3);
}

static int
l_gprovider_iter(lua_State *L)
{
	struct ggeom *gp;
	struct gprovider *pp, *nextp;

	gp = checkcookie(L, 1, GGEOM_METATABLE);
	pp = checkcookienull(L, 2, GPROVIDER_METATABLE);

	nextp = pp == NULL ? LIST_FIRST(&gp->lg_provider) :
	    LIST_NEXT(pp, lg_provider);
	return (nextp == NULL ? 0 : newref(L, 1, nextp, GPROVIDER_METATABLE));
}

static int
l_ggeom_provider(lua_State *L)
{
	struct ggeom *gp;

	gp = checkcookie(L, 1, GGEOM_METATABLE);

	lua_pushcfunction(L, l_gprovider_iter);
	lua_pushvalue(L, 1);
	new(L, NULL, GPROVIDER_METATABLE);
	return (3);
}

static int
l_ggeom_index(lua_State *L)
{
	struct ggeom *gp;
	const char *field;

	gp = checkcookie(L, 1, GGEOM_METATABLE);
	field = luaL_checkstring(L, 2);

	if (strcmp(field, "id") == 0) {
		lua_pushlightuserdata(L, gp->lg_id);
		return (1);
	}
	if (strcmp(field, "class") == 0) {
		return (newref(L, 1, gp->lg_class, GCLASS_METATABLE));
	}
	if (strcmp(field, "name") == 0) {
		lua_pushstring(L, gp->lg_name);
		return (1);
	}
	if (strcmp(field, "rank") == 0) {
		lua_pushinteger(L, gp->lg_rank);
		return (1);
	}
	if (strcmp(field, "consumer") == 0) {
		lua_pushcfunction(L, l_ggeom_consumer);
		return (1);
	}
	if (strcmp(field, "provider") == 0) {
		lua_pushcfunction(L, l_ggeom_provider);
		return (1);
	}
	if (strcmp(field, "config") == 0) {
		pushconf(L, &gp->lg_config);
		return (1);
	}
	return (0);
}

GEOM_STRUCT_PAIRS(geom, GEOM,
    FIELD(id),
    FIELD(class),
    FIELD(name),
    FIELD(rank),
    FIELD(consumer),
    FIELD(provider),
    FIELD(config)
)

static int
l_gconsumer_index(lua_State *L)
{
	struct gconsumer *cp;
	const char *field;

	cp = checkcookie(L, 1, GCONSUMER_METATABLE);
	field = luaL_checkstring(L, 2);

	if (strcmp(field, "id") == 0) {
		lua_pushlightuserdata(L, cp->lg_id);
		return (1);
	}
	if (strcmp(field, "geom") == 0) {
		return (newref(L, 1, cp->lg_geom, GGEOM_METATABLE));
	}
	if (strcmp(field, "provider") == 0 && cp->lg_provider != NULL) {
		return (newref(L, 1, cp->lg_provider, GPROVIDER_METATABLE));
	}
	if (strcmp(field, "mode") == 0) {
		lua_pushstring(L, cp->lg_mode);
		return (1);
	}
	if (strcmp(field, "config") == 0) {
		pushconf(L, &cp->lg_config);
		return (1);
	}
	return (0);
}

GEOM_STRUCT_PAIRS(consumer, CONSUMER,
    FIELD(id),
    FIELD(geom),
    FIELD(provider),
    FIELD(mode),
    FIELD(config)
)

static int
l_gconsumers_iter(lua_State *L)
{
	struct gprovider *pp;
	struct gconsumer *cp, *nextp;

	pp = checkcookie(L, 1, GPROVIDER_METATABLE);
	cp = checkcookienull(L, 2, GCONSUMER_METATABLE);

	nextp = cp == NULL ? LIST_FIRST(&pp->lg_consumers) :
	    LIST_NEXT(cp, lg_consumers);
	return (nextp == NULL ? 0 : newref(L, 1, nextp, GCONSUMER_METATABLE));
}

static int
l_gprovider_consumers(lua_State *L)
{
	struct gprovider *pp;

	pp = checkcookie(L, 1, GPROVIDER_METATABLE);

	lua_pushcfunction(L, l_gconsumers_iter);
	lua_pushvalue(L, 1);
	new(L, NULL, GCONSUMER_METATABLE);
	return (3);
}

static int
l_gprovider_index(lua_State *L)
{
	struct gprovider *pp;
	const char *field;

	pp = checkcookie(L, 1, GPROVIDER_METATABLE);
	field = luaL_checkstring(L, 2);

	if (strcmp(field, "id") == 0) {
		lua_pushlightuserdata(L, pp->lg_id);
		return (1);
	}
	if (strcmp(field, "name") == 0) {
		lua_pushstring(L, pp->lg_name);
		return (1);
	}
	if (strcmp(field, "geom") == 0) {
		return (newref(L, 1, pp->lg_geom, GGEOM_METATABLE));
	}
	if (strcmp(field, "consumers") == 0) {
		lua_pushcfunction(L, l_gprovider_consumers);
		return (1);
	}
	if (strcmp(field, "mode") == 0) {
		lua_pushstring(L, pp->lg_mode);
		return (1);
	}
#define IFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		lua_pushinteger(L, pp->lg_ ## name); \
		return (1); \
	} \
})
	IFIELD(mediasize);
	IFIELD(sectorsize);
	IFIELD(stripeoffset);
	IFIELD(stripesize);
#undef IFIELD
	if (strcmp(field, "config") == 0) {
		pushconf(L, &pp->lg_config);
		return (1);
	}
	return (0);
}

GEOM_STRUCT_PAIRS(provider, PROVIDER,
    FIELD(id),
    FIELD(name),
    FIELD(geom),
    FIELD(consumers),
    FIELD(mode),
    FIELD(mediasize),
    FIELD(sectorsize),
    FIELD(stripeoffset),
    FIELD(stripesize),
    FIELD(config)
)

#undef GEOM_STRUCT_PAIRS
#undef FIELD

static int
l_g_close(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	errno = 0;
	return (luaL_fileresult(L, g_close(fd) == 0, NULL));
}

static inline int
newgeom(lua_State *L, const char *name, int fd, bool dowrite)
{
	luaL_Stream *s;

	s = lua_newuserdatauv(L, sizeof (*s), 0);
	s->closef = NULL;
	if ((s->f = fdopen(fd, dowrite ? "w" : "r")) == NULL) {
		g_close(fd);
		return (luaL_fileresult(L, 0, name));
	}
	s->closef = l_g_close;
	luaL_setmetatable(L, LUA_FILEHANDLE);
	return (1);
}

static int
l_g_open(lua_State *L)
{
	const char *name;
	bool dowrite;
	int fd;

	name = luaL_checkstring(L, 1);
	dowrite = lua_toboolean(L, 2);

	if ((fd = g_open(name, dowrite)) == -1) {
		return (luaL_fileresult(L, 0, name));
	}
	return (newgeom(L, name, fd, dowrite));
}

static int
l_g_mediasize(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	lua_pushinteger(L, g_mediasize(fd));
	return (1);
}

static int
l_g_sectorsize(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	lua_pushinteger(L, g_sectorsize(fd));
	return (1);
}

static int
l_g_stripeoffset(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	lua_pushinteger(L, g_stripeoffset(fd));
	return (1);
}

static int
l_g_stripesize(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	lua_pushinteger(L, g_stripesize(fd));
	return (1);
}

static int
l_g_flush(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	if (g_flush(fd) != 0) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_g_delete(lua_State *L)
{
	off_t offset, length;
	int fd;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 2);
	length = luaL_checkinteger(L, 3);

	if (g_delete(fd, offset, length) != 0) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_g_device_path(lua_State *L)
{
	const char *devpath;
	char *fullpath;

	devpath = luaL_checkstring(L, 1);

	if ((fullpath = g_device_path(devpath)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, fullpath);
	free(fullpath);
	return (1);
}

static int
l_g_get_ident(lua_State *L)
{
	char ident[DISK_IDENT_SIZE];
	int fd;

	fd = checkfd(L, 1);

	if (g_get_ident(fd, ident, sizeof (ident)) != 0) {
		return (fail(L, errno));
	}
	lua_pushstring(L, ident);
	return (1);
}

static int
l_g_get_name(lua_State *L)
{
	char name[MAXPATHLEN];
	const char *ident;

	ident = luaL_checkstring(L, 1);

	if (g_get_name(ident, name, sizeof (name)) != 0) {
		return (fail(L, errno));
	}
	lua_pushstring(L, name);
	return (1);
}

static int
l_g_open_by_ident(lua_State *L)
{
	char name[MAXPATHLEN];
	const char *ident;
	bool dowrite;
	int fd, n;

	ident = luaL_checkstring(L, 1);
	dowrite = lua_toboolean(L, 2);

	if ((fd = g_open_by_ident(ident, dowrite, name, sizeof (name))) == -1) {
		return (fail(L, errno));
	}
	lua_pushstring(L, name);
	if ((n = newgeom(L, name, fd, dowrite)) != 1) {
		return (n);
	}
	return (2);
}

static int
l_g_providername(lua_State *L)
{
	char *name;
	int fd;

	fd = checkfd(L, 1);

	if ((name = g_providername(fd)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, name);
	free(name);
	return (1);
}

static const struct luaL_Reg l_geom_funcs[] = {
#ifdef NOTYET
	{"stats_open", l_geom_stats_open},
#endif
	{"gctl_get_handle", l_gctl_get_handle},
	{"getxml", l_geom_getxml},
	{"getxml_geom", l_geom_getxml_geom},
	{"xml2tree", l_geom_xml2tree},
	{"gettree", l_geom_gettree},
	{"gettree_geom", l_geom_gettree_geom},
	{"open", l_g_open},
	{"close", l_g_close},
	{"mediasize", l_g_mediasize},
	{"sectorsize", l_g_sectorsize},
	{"stripeoffset", l_g_stripeoffset},
	{"stripesize", l_g_stripesize},
	{"flush", l_g_flush},
	{"delete", l_g_delete},
	{"device_path", l_g_device_path},
	{"get_ident", l_g_get_ident},
	{"get_name", l_g_get_name},
	{"open_by_ident", l_g_open_by_ident},
	{"providername", l_g_providername},
	{NULL, NULL}
};

#ifdef NOTYET
static const struct luaL_Reg l_geom_stats_meta[] = {
	{"__close", l_geom_stats_close},
	{"__gc", l_geom_stats_close},
	{"close", l_geom_stats_close},
	{"resync", l_geom_stats_resync},
	{"snapshot_get", l_geom_stats_snapshot_get},
	{NULL, NULL}
};

static const struct luaL_Reg l_geom_stats_snapshot_meta[] = {
	{"__close", l_geom_stats_snapshot_free},
	{"__gc", l_geom_stats_snapshot_free},
	{"timestamp", l_geom_stats_snapshot_timestamp},
	{"reset", l_geom_stats_snapshot_reset},
	{"next", l_geom_stats_snapshot_next},
	{NULL, NULL}
};
#endif

static const struct luaL_Reg l_gctl_req_meta[] = {
	{"__gc", l_gctl_free},
	{"ro_param", l_gctl_ro_param},
	{"issue", l_gctl_issue},
	{"dump", l_gctl_dump},
	{NULL, NULL}
};

static const struct luaL_Reg l_gident_meta[] = {
	{"__index", l_gident_index},
	{"__pairs", l_gident_pairs},
	{NULL, NULL}
};

static const struct luaL_Reg l_gmesh_meta[] = {
	{"__gc", l_geom_deletetree},
	{"class", l_gmesh_class},
	{"ident", l_gmesh_ident},
	{"lookupid", l_geom_lookupid},
	{NULL, NULL}
};

static const struct luaL_Reg l_gclass_meta[] = {
	{"__index", l_gclass_index},
	{"__pairs", l_gclass_pairs},
	{NULL, NULL}
};

static const struct luaL_Reg l_ggeom_meta[] = {
	{"__index", l_ggeom_index},
	{"__pairs", l_ggeom_pairs},
	{NULL, NULL}
};

static const struct luaL_Reg l_gconsumer_meta[] = {
	{"__index", l_gconsumer_index},
	{"__pairs", l_gconsumer_pairs},
	{NULL, NULL}
};

static const struct luaL_Reg l_gprovider_meta[] = {
	{"__index", l_gprovider_index},
	{"__pairs", l_gprovider_pairs},
	{NULL, NULL}
};

int
luaopen_geom(lua_State *L)
{
	/* TODO: devstat, require devstat */
#ifdef NOTYET
	luaL_newmetatable(L, GEOM_STATS_METATABLE);
	luaL_setfuncs(L, l_geom_stats_meta, 0);

	luaL_newmetatable(L, GEOM_STATS_SNAPSHOT_METATABLE);
	luaL_setfuncs(L, l_geom_stats_snapshot_meta, 0);
#endif
	luaL_newmetatable(L, GIDENT_METATABLE);
	luaL_setfuncs(L, l_gident_meta, 0);

	luaL_newmetatable(L, GMESH_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_gmesh_meta, 0);

	luaL_newmetatable(L, GCLASS_METATABLE);
	luaL_setfuncs(L, l_gclass_meta, 0);

	luaL_newmetatable(L, GGEOM_METATABLE);
	luaL_setfuncs(L, l_ggeom_meta, 0);

	luaL_newmetatable(L, GCONSUMER_METATABLE);
	luaL_setfuncs(L, l_gconsumer_meta, 0);

	luaL_newmetatable(L, GPROVIDER_METATABLE);
	luaL_setfuncs(L, l_gprovider_meta, 0);

	luaL_newlib(L, l_geom_funcs);
	return (1);
}
