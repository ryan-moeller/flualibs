/*
 * Copyright (c) 2025-2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/acl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>

#include "luaerror.h"
#include "utils.h"

_Static_assert(sizeof(uid_t) == sizeof(gid_t), "fatal uid_t/gid_t");

#define ACL_METATABLE "acl_t"
#define ACL_ENTRY_METATABLE "acl_entry_t"
#define ACL_FLAGSET_METATABLE "acl_flagset_t"
#define ACL_PERMSET_METATABLE "acl_permset_t"

int luaopen_sys_acl(lua_State *);

static int
l_acl_delete(lua_State *L)
{
	luaL_Stream *s;
	acl_type_t type;
	int fd;

	s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	if (acl_delete_fd_np(fd, type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_delete_fd(lua_State *L)
{
	acl_type_t type;
	int fd;

	fd = luaL_checkinteger(L, 1);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if (acl_delete_fd_np(fd, type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_delete_file(lua_State *L)
{
	const char *path;
	acl_type_t type;

	path = luaL_checkstring(L, 1);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if (acl_delete_file_np(path, type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_delete_link(lua_State *L)
{
	const char *path;
	acl_type_t type;

	path = luaL_checkstring(L, 1);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if (acl_delete_link_np(path, type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

#if __FreeBSD_version > 1400032
static int
l_acl_extended_file(lua_State *L)
{
	const char *path;
	int extended;

	path = luaL_checkstring(L, 1);

	if ((extended = acl_extended_file_np(path)) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, extended);
	return (1);
}

static int
l_acl_extended_link(lua_State *L)
{
	const char *path;
	int extended;

	path = luaL_checkstring(L, 1);

	if ((extended = acl_extended_link_np(path)) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, extended);
	return (1);
}
#endif

#if __FreeBSD_version > 1400032
static int
l_acl_from_mode(lua_State *L)
{
	acl_t acl;
	mode_t mode;

	mode = luaL_checkinteger(L, 1);

	if ((acl = acl_from_mode_np(mode)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl, ACL_METATABLE));
}
#endif

static int
l_acl_from_text(lua_State *L)
{
	acl_t acl;
	const char *buf;

	buf = luaL_checkstring(L, 1);

	if ((acl = acl_from_text(buf)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl, ACL_METATABLE));
}

static int
l_acl_get(lua_State *L)
{
	acl_t acl;
	acl_type_t type;
	luaL_Stream *s;
	int fd;

	s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	if ((acl = acl_get_fd_np(fd, type)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl, ACL_METATABLE));
}

static int
l_acl_get_fd(lua_State *L)
{
	acl_t acl;
	acl_type_t type;
	int fd;

	fd = luaL_checkinteger(L, 1);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if ((acl = acl_get_fd_np(fd, type)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl, ACL_METATABLE));
}

static int
l_acl_get_file(lua_State *L)
{
	acl_t acl;
	const char *path;
	acl_type_t type;

	path = luaL_checkstring(L, 1);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if ((acl = acl_get_file(path, type)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl, ACL_METATABLE));
}

static int
l_acl_get_link(lua_State *L)
{
	acl_t acl;
	const char *path;
	acl_type_t type;

	path = luaL_checkstring(L, 1);
	type = luaL_optinteger(L, 2, ACL_TYPE_ACCESS);

	if ((acl = acl_get_link_np(path, type)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl, ACL_METATABLE));
}

static int
l_acl_new(lua_State *L)
{
	acl_t acl;
	int count;

	count = luaL_optinteger(L, 1, ACL_MAX_ENTRIES);

	if ((acl = acl_init(count)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl, ACL_METATABLE));
}

#if __FreeBSD_version > 1400032
static int
l_acl_cmp(lua_State *L)
{
	acl_t acl1, acl2;
	int cmp;

	acl1 = checkcookie(L, 1, ACL_METATABLE);
	acl2 = checkcookie(L, 2, ACL_METATABLE);

	if ((cmp = acl_cmp_np(acl1, acl2)) == -1) {
		return (fatal(L, "acl_cmp_np", errno));
	}
	lua_pushboolean(L, cmp == 0);
	return (1);
}
#endif

static int
l_acl_gc(lua_State *L)
{
	acl_t acl;

	acl = checkcookie(L, 1, ACL_METATABLE);

	if (acl_free(acl) == -1) {
		return (fatal(L, "acl_free", errno));
	}
	return (0);
}

static int
l_acl_calc_mask(lua_State *L)
{
	acl_t acl;

	acl = checkcookie(L, 1, ACL_METATABLE);

	if (acl_calc_mask(&acl) == -1) {
		return (fail(L, errno));
	}
	lua_pushlightuserdata(L, acl);
	lua_setiuservalue(L, 1, COOKIE);
	return (success(L));
}

static int
l_acl_create_entry(lua_State *L)
{
	acl_t acl;
	acl_entry_t entry;
	int index;

	acl = checkcookie(L, 1, ACL_METATABLE);
	index = luaL_optinteger(L, 2, -1);

	if (index == -1) {
		if (acl_create_entry(&acl, &entry) == -1) {
			return (fail(L, errno));
		}
	} else {
		if (acl_create_entry_np(&acl, &entry, index) == -1) {
			return (fail(L, errno));
		}
	}
	lua_pushlightuserdata(L, acl);
	lua_setiuservalue(L, 1, COOKIE);
	return (newref(L, 1, entry, ACL_ENTRY_METATABLE));
}

static int
l_acl_delete_entry(lua_State *L)
{
	acl_t acl;

	acl = checkcookie(L, 1, ACL_METATABLE);
	if (lua_isinteger(L, 2)) {
		int index = lua_tointeger(L, 2);

		if (acl_delete_entry_np(acl, index) == -1) {
			return (fail(L, errno));
		}
	} else {
		acl_entry_t entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);

		if (acl_delete_entry(acl, entry) == -1) {
			return (fail(L, errno));
		}
	}
	return (success(L));
}

static int
l_acl_dup(lua_State *L)
{
	acl_t acl1, acl2;

	acl1 = checkcookie(L, 1, ACL_METATABLE);

	if ((acl2 = acl_dup(acl1)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl2, ACL_METATABLE));
}

#if __FreeBSD_version > 1400032
static int
l_acl_equiv_mode(lua_State *L)
{
	acl_t acl;
	mode_t mode;
	int possible;

	acl = checkcookie(L, 1, ACL_METATABLE);

	switch (acl_equiv_mode_np(acl, &mode)) {
	case 0:
		lua_pushboolean(L, true);
		lua_pushinteger(L, mode);
		return (2);
	case 1:
		lua_pushboolean(L, false);
		return (1);
	case -1:
		return (fail(L, errno));
	default:
		return (luaL_error(L, "undocumented acl_equiv_mode_np result"));
	}
}
#endif

static int
l_acl_get_brand(lua_State *L)
{
	acl_t acl;
	int brand;

	acl = checkcookie(L, 1, ACL_METATABLE);

	if (acl_get_brand_np(acl, &brand) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, brand);
	return (1);
}

static int
l_acl_get_entry(lua_State *L)
{
	acl_t acl;
	acl_entry_t entry;
	int id;

	acl = checkcookie(L, 1, ACL_METATABLE);
	id = luaL_checkinteger(L, 2);

	if (acl_get_entry(acl, id, &entry) == -1) {
		return (fail(L, errno));
	}
	return (newref(L, 1, entry, ACL_ENTRY_METATABLE));
}

static int
l_acl_is_trivial(lua_State *L)
{
	acl_t acl;
	int trivial;

	acl = checkcookie(L, 1, ACL_METATABLE);

	if (acl_is_trivial_np(acl, &trivial)== -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, trivial);
	return (1);
}

static int
l_acl_set(lua_State *L)
{
	acl_t acl;
	luaL_Stream *s;
	acl_type_t type;
	int fd;

	acl = checkcookie(L, 1, ACL_METATABLE);
	s = luaL_checkudata(L, 2, LUA_FILEHANDLE);
	type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	if (acl_set_fd_np(fd, acl, type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_set_fd(lua_State *L)
{
	acl_t acl;
	acl_type_t type;
	int fd;

	acl = checkcookie(L, 1, ACL_METATABLE);
	fd = luaL_checkinteger(L, 2);
	type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

	if (acl_set_fd_np(fd, acl, type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_set_file(lua_State *L)
{
	acl_t acl;
	const char *path;
	acl_type_t type;

	acl = checkcookie(L, 1, ACL_METATABLE);
	path = luaL_checkstring(L, 2);
	type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

	if (acl_set_file(path, type, acl) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_set_link(lua_State *L)
{
	acl_t acl;
	const char *path;
	acl_type_t type;

	acl = checkcookie(L, 1, ACL_METATABLE);
	path = luaL_checkstring(L, 2);
	type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

	if (acl_set_link_np(path, type, acl) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_strip(lua_State *L)
{
	acl_t acl1, acl2;
	bool recalculate_mask;

	acl1 = checkcookie(L, 1, ACL_METATABLE);
	recalculate_mask = lua_toboolean(L, 2);

	if ((acl2 = acl_strip_np(acl1, recalculate_mask)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, acl2, ACL_METATABLE));
}

static int
l_acl_to_text(lua_State *L)
{
	acl_t acl;
	char *buf;
	ssize_t len;
	int flags;

	acl = checkcookie(L, 1, ACL_METATABLE);
	flags = luaL_optinteger(L, 2, 0);

	if ((buf = acl_to_text_np(acl, &len, flags)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushlstring(L, buf, len);
	return (1);
}

static int
l_acl_valid(lua_State *L)
{
	acl_t acl;

	acl = checkcookie(L, 1, ACL_METATABLE);
	if (lua_isuserdata(L, 2)) {
		luaL_Stream *s;
		acl_type_t type;
		int fd;

		s = luaL_checkudata(L, 2, LUA_FILEHANDLE);
		type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

		if ((fd = fileno(s->f)) < 0) {
			return (fatal(L, "fileno", errno));
		}
		if (acl_valid_fd_np(fd, type, acl) == -1) {
			return (fail(L, errno));
		}
		return (success(L));
	}

	if (acl_valid(acl) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_valid_fd(lua_State *L)
{
	acl_t acl;
	acl_type_t type;
	int fd;

	acl = checkcookie(L, 1, ACL_METATABLE);
	fd = luaL_checkinteger(L, 2);
	type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

	if (acl_valid_fd_np(fd, type, acl) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_valid_file(lua_State *L)
{
	acl_t acl;
	const char *path;
	acl_type_t type;

	acl = checkcookie(L, 1, ACL_METATABLE);
	path = luaL_checkstring(L, 2);
	type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

	if (acl_valid_file_np(path, type, acl) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_valid_link(lua_State *L)
{
	acl_t acl;
	const char *path;
	acl_type_t type;

	acl = checkcookie(L, 1, ACL_METATABLE);
	path = luaL_checkstring(L, 2);
	type = luaL_optinteger(L, 3, ACL_TYPE_ACCESS);

	if (acl_valid_link_np(path, type, acl) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_copy_entry(lua_State *L)
{
	acl_entry_t dst, src;

	dst = checkcookie(L, 1, ACL_ENTRY_METATABLE);
	src = checkcookie(L, 2, ACL_ENTRY_METATABLE);

	if (acl_copy_entry(dst, src) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_get_flagset(lua_State *L)
{
	acl_entry_t entry;
	acl_flagset_t flagset;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);

	if (acl_get_flagset_np(entry, &flagset) == -1) {
		return (fail(L, errno));
	}
	return (newref(L, 1, flagset, ACL_FLAGSET_METATABLE));
}

static int
l_acl_get_permset(lua_State *L)
{
	acl_entry_t entry;
	acl_permset_t permset;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);

	if (acl_get_permset(entry, &permset) == -1) {
		return (fail(L, errno));
	}
	return (newref(L, 1, permset, ACL_PERMSET_METATABLE));
}

static int
l_acl_get_qualifier(lua_State *L)
{
	acl_entry_t entry;
	uid_t *id;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);

	if ((id = acl_get_qualifier(entry)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, *id);
	acl_free(id);
	return (1);
}

static int
l_acl_get_entry_type(lua_State *L)
{
	acl_entry_t entry;
	acl_entry_type_t type;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);

	if (acl_get_entry_type_np(entry, &type)== -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, type);
	return (1);
}

static int
l_acl_set_entry_type(lua_State *L)
{
	acl_entry_t entry;
	acl_entry_type_t type;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);
	type = luaL_checkinteger(L, 2);

	if (acl_set_entry_type_np(entry, type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_get_tag_type(lua_State *L)
{
	acl_entry_t entry;
	acl_tag_t tag_type;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);

	if (acl_get_tag_type(entry, &tag_type) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, tag_type);
	return (1);
}

static int
l_acl_set_tag_type(lua_State *L)
{
	acl_entry_t entry;
	acl_tag_t tag_type;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);
	tag_type = luaL_checkinteger(L, 2);

	if (acl_set_tag_type(entry, tag_type) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_set_flagset(lua_State *L)
{
	acl_entry_t entry;
	acl_flagset_t flagset;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);
	flagset = checkcookie(L, 2, ACL_FLAGSET_METATABLE);

	if (acl_set_flagset_np(entry, flagset) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_set_permset(lua_State *L)
{
	acl_entry_t entry;
	acl_permset_t permset;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);
	permset = checkcookie(L, 2, ACL_PERMSET_METATABLE);

	if (acl_set_permset(entry, permset) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_set_qualifier(lua_State *L)
{
	acl_entry_t entry;
	uid_t id;

	entry = checkcookie(L, 1, ACL_ENTRY_METATABLE);
	id = luaL_checkinteger(L, 2);

	if (acl_set_qualifier(entry, &id) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_add_flag(lua_State *L)
{
	acl_flagset_t flagset;
	acl_flag_t flag;

	flagset = checkcookie(L, 1, ACL_FLAGSET_METATABLE);
	flag = luaL_checkinteger(L, 2);

	if (acl_add_flag_np(flagset, flag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_clear_flags(lua_State *L)
{
	acl_flagset_t flagset;

	flagset = checkcookie(L, 1, ACL_FLAGSET_METATABLE);

	if (acl_clear_flags_np(flagset) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_delete_flag(lua_State *L)
{
	acl_flagset_t flagset;
	acl_flag_t flag;

	flagset = checkcookie(L, 1, ACL_FLAGSET_METATABLE);
	flag = luaL_checkinteger(L, 2);

	if (acl_delete_flag_np(flagset, flag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_get_flag(lua_State *L)
{
	acl_flagset_t flagset;
	acl_flag_t flag;
	int set;

	flagset = checkcookie(L, 1, ACL_FLAGSET_METATABLE);
	flag = luaL_checkinteger(L, 2);

	if ((set = acl_get_flag_np(flagset, flag)) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, set);
	return (1);
}

static int
l_acl_add_perm(lua_State *L)
{
	acl_permset_t permset;
	acl_perm_t perm;

	permset = checkcookie(L, 1, ACL_PERMSET_METATABLE);
	perm = luaL_checkinteger(L, 2);

	if (acl_add_perm(permset, perm) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_clear_perms(lua_State *L)
{
	acl_permset_t permset;

	permset = checkcookie(L, 1, ACL_PERMSET_METATABLE);

	if (acl_clear_perms(permset) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_delete_perm(lua_State *L)
{
	acl_permset_t permset;
	acl_perm_t perm;

	permset = checkcookie(L, 1, ACL_PERMSET_METATABLE);
	perm = luaL_checkinteger(L, 2);

	if (acl_delete_perm(permset, perm) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acl_get_perm(lua_State *L)
{
	acl_permset_t permset;
	acl_perm_t perm;
	int set;

	permset = checkcookie(L, 1, ACL_PERMSET_METATABLE);
	perm = luaL_checkinteger(L, 2);

	if ((set = acl_get_perm_np(permset, perm)) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, set);
	return (1);
}

static const struct luaL_Reg l_acl_funcs[] = {
	/* acl_copy_ext and acl_copy_int only ENOSYS */
	{"delete", l_acl_delete},
	{"delete_fd", l_acl_delete_fd},
	{"delete_file", l_acl_delete_file},
	{"delete_link", l_acl_delete_link},
	/* acl_delete_def_file and acl_delete_def_link_np are redundant */
#if __FreeBSD_version > 1400032
	{"extended_file", l_acl_extended_file},
	{"extended_link", l_acl_extended_link},
	{"from_mode", l_acl_from_mode},
#endif
	{"from_text", l_acl_from_text},
	{"get", l_acl_get},
	{"get_fd", l_acl_get_fd},
	{"get_file", l_acl_get_file},
	{"get_link", l_acl_get_link},
	{"new", l_acl_new},
	{NULL, NULL}
};

static const struct luaL_Reg l_acl_meta[] = {
#if __FreeBSD_version > 1400032
	{"__eq", l_acl_cmp},
#endif
	{"__gc", l_acl_gc},
	{"calc_mask", l_acl_calc_mask},
	{"create_entry", l_acl_create_entry},
	{"delete_entry", l_acl_delete_entry},
	{"dup", l_acl_dup},
#if __FreeBSD_version > 1400032
	{"equiv_mode", l_acl_equiv_mode},
#endif
	{"get_brand", l_acl_get_brand},
	{"get_entry", l_acl_get_entry},
	{"is_trivial", l_acl_is_trivial},
	{"set", l_acl_set},
	{"set_fd", l_acl_set_fd},
	{"set_file", l_acl_set_file},
	{"set_link", l_acl_set_link},
	/* acl_size doesn't actually exist */
	{"strip", l_acl_strip},
	{"to_text", l_acl_to_text},
	{"valid", l_acl_valid},
	{"valid_fd", l_acl_valid_fd},
	{"valid_file", l_acl_valid_file},
	{"valid_link", l_acl_valid_link},
	{NULL, NULL}
};

static const struct luaL_Reg l_acl_entry_meta[] = {
	{"copy", l_acl_copy_entry},
	{"get_flagset", l_acl_get_flagset},
	{"get_permset", l_acl_get_permset},
	{"get_qualifier", l_acl_get_qualifier},
	{"get_tag_type", l_acl_get_tag_type},
	{"get_type", l_acl_get_entry_type},
	{"set_flagset", l_acl_set_flagset},
	{"set_permset", l_acl_set_permset},
	{"set_qualifier", l_acl_set_qualifier},
	{"set_tag_type", l_acl_set_tag_type},
	{"set_type", l_acl_set_entry_type},
	{NULL, NULL}
};

static const struct luaL_Reg l_acl_flagset_meta[] = {
	{"add", l_acl_add_flag},
	{"clear", l_acl_clear_flags},
	{"delete", l_acl_delete_flag},
	{"get", l_acl_get_flag},
	{NULL, NULL}
};

static const struct luaL_Reg l_acl_permset_meta[] = {
	{"add", l_acl_add_perm},
	{"clear", l_acl_clear_perms},
	{"delete", l_acl_delete_perm},
	{"get", l_acl_get_perm},
	{NULL, NULL}
};

int
luaopen_sys_acl(lua_State *L)
{
	luaL_newmetatable(L, ACL_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_acl_meta, 0);

	luaL_newmetatable(L, ACL_ENTRY_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_acl_entry_meta, 0);

	luaL_newmetatable(L, ACL_FLAGSET_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_acl_flagset_meta, 0);

	luaL_newmetatable(L, ACL_PERMSET_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_acl_permset_meta, 0);

	luaL_newlib(L, l_acl_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ACL_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(MAX_ENTRIES);

	DEFINE(BRAND_UNKNOWN);
	DEFINE(BRAND_POSIX);
	DEFINE(BRAND_NFS4);

	DEFINE(UNDEFINED_TAG);
	DEFINE(USER_OBJ);
	DEFINE(USER);
	DEFINE(GROUP_OBJ);
	DEFINE(GROUP);
	DEFINE(MASK);
	DEFINE(OTHER);
	DEFINE(OTHER_OBJ);
	DEFINE(EVERYONE);

	DEFINE(ENTRY_TYPE_ALLOW);
	DEFINE(ENTRY_TYPE_DENY);
	DEFINE(ENTRY_TYPE_AUDIT);
	DEFINE(ENTRY_TYPE_ALARM);

	DEFINE(TYPE_ACCESS_OLD);
	DEFINE(TYPE_DEFAULT_OLD);
	DEFINE(TYPE_ACCESS);
	DEFINE(TYPE_DEFAULT);
	DEFINE(TYPE_NFS4);

	DEFINE(EXECUTE);
	DEFINE(WRITE);
	DEFINE(READ);
	DEFINE(PERM_NONE);
	DEFINE(PERM_BITS);
	DEFINE(POSIX1E_BITS);

	DEFINE(READ_DATA);
	DEFINE(LIST_DIRECTORY);
	DEFINE(WRITE_DATA);
	DEFINE(ADD_FILE);
	DEFINE(APPEND_DATA);
	DEFINE(ADD_SUBDIRECTORY);
	DEFINE(READ_NAMED_ATTRS);
	DEFINE(WRITE_NAMED_ATTRS);
	DEFINE(DELETE_CHILD);
	DEFINE(READ_ATTRIBUTES);
	DEFINE(WRITE_ATTRIBUTES);
	DEFINE(DELETE);
	DEFINE(READ_ACL);
	DEFINE(WRITE_ACL);
	DEFINE(WRITE_OWNER);
	DEFINE(SYNCHRONIZE);

	DEFINE(FULL_SET);
	DEFINE(MODIFY_SET);
	DEFINE(READ_SET);
	DEFINE(WRITE_SET);

	DEFINE(NFS4_PERM_BITS);

	DEFINE(FIRST_ENTRY);
	DEFINE(NEXT_ENTRY);

	DEFINE(ENTRY_FILE_INHERIT);
	DEFINE(ENTRY_DIRECTORY_INHERIT);
	DEFINE(ENTRY_NO_PROPAGATE_INHERIT);
	DEFINE(ENTRY_INHERIT_ONLY);
	DEFINE(ENTRY_SUCCESSFUL_ACCESS);
	DEFINE(ENTRY_FAILED_ACCESS);
	DEFINE(ENTRY_INHERITED);

	DEFINE(FLAGS_BITS);

	DEFINE(UNDEFINED_ID);

	DEFINE(TEXT_VERBOSE);
	DEFINE(TEXT_NUMERIC_IDS);
	DEFINE(TEXT_APPEND_ID);

	DEFINE(OVERRIDE_MASK);
	DEFINE(PRESERVE_MASK);
#undef DEFINE
	return (1);
}
