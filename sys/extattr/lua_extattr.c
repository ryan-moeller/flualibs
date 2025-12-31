/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/extattr.h>
#include <errno.h>
#include <libutil.h> /* XXX: prototypes for functions not actually in libutil */
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

int luaopen_sys_extattr(lua_State *);

static int
l_extattr_delete(lua_State *L)
{
	luaL_Stream *s;
	const char *attrname;
	int attrnamespace, fd;

	s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	if (extattr_delete_fd(fd, attrnamespace, attrname) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_extattr_delete_fd(lua_State *L)
{
	const char *attrname;
	int fd, attrnamespace;

	fd = luaL_checkinteger(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	if (extattr_delete_fd(fd, attrnamespace, attrname) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_extattr_delete_file(lua_State *L)
{
	const char *path, *attrname;
	int attrnamespace;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	if (extattr_delete_file(path, attrnamespace, attrname) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_extattr_delete_link(lua_State *L)
{
	const char *path, *attrname;
	int attrnamespace;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	if (extattr_delete_link(path, attrnamespace, attrname) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_extattr_get(lua_State *L)
{
	luaL_Stream *s;
	const char *attrname;
	void *data;
	ssize_t datalen, getlen;
	int attrnamespace, fd, error;

	s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	data = NULL;
	datalen = 0;
	for (;;) {
		if ((getlen = extattr_get_fd(fd, attrnamespace, attrname, data,
		    datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (getlen <= datalen) {
			break;
		}
		free(data);
		datalen = getlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	lua_pushlstring(L, data, getlen);
	free(data);
	return (1);
}

static int
l_extattr_get_fd(lua_State *L)
{
	const char *attrname;
	void *data;
	ssize_t datalen, getlen;
	int fd, attrnamespace, error;

	fd = luaL_checkinteger(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	data = NULL;
	datalen = 0;
	for (;;) {
		if ((getlen = extattr_get_fd(fd, attrnamespace, attrname, data,
		    datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (getlen <= datalen) {
			break;
		}
		free(data);
		datalen = getlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	lua_pushlstring(L, data, getlen);
	free(data);
	return (1);
}

static int
l_extattr_get_file(lua_State *L)
{
	const char *path, *attrname;
	void *data;
	ssize_t datalen, getlen;
	int attrnamespace, fd, error;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	data = NULL;
	datalen = 0;
	for (;;) {
		if ((getlen = extattr_get_file(path, attrnamespace, attrname,
		    data, datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (getlen <= datalen) {
			break;
		}
		free(data);
		datalen = getlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	lua_pushlstring(L, data, getlen);
	free(data);
	return (1);
}

static int
l_extattr_get_link(lua_State *L)
{
	const char *path, *attrname;
	void *data;
	ssize_t datalen, getlen;
	int attrnamespace, fd, error;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);

	data = NULL;
	datalen = 0;
	for (;;) {
		if ((getlen = extattr_get_link(path, attrnamespace, attrname,
		    data, datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (getlen <= datalen) {
			break;
		}
		free(data);
		datalen = getlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	lua_pushlstring(L, data, getlen);
	free(data);
	return (1);
}

static inline void
pushlist(lua_State *L, const char *p, const char *end)
{
	lua_newtable(L);
	while (p < end) {
		ptrdiff_t len = *p++;

		lua_pushlstring(L, p, len);
		lua_rawseti(L, -2, luaL_len(L, -2) + 1);
		p += len;
	}
}

static int
l_extattr_list(lua_State *L)
{
	luaL_Stream *s;
	void *data;
	ssize_t datalen, listlen;
	int attrnamespace, fd, error;

	s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	attrnamespace = luaL_checkinteger(L, 2);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	data = NULL;
	datalen = 0;
	for (;;) {
		if ((listlen = extattr_list_fd(fd, attrnamespace, data,
		    datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (listlen <= datalen) {
			break;
		}
		free(data);
		datalen = listlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	pushlist(L, data, data + listlen);
	free(data);
	return (1);
}

static int
l_extattr_list_fd(lua_State *L)
{
	void *data;
	ssize_t datalen, listlen;
	int fd, attrnamespace, error;

	fd = luaL_checkinteger(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);

	data = NULL;
	datalen = 0;
	for (;;) {
		if ((listlen = extattr_list_fd(fd, attrnamespace, data,
		    datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (listlen <= datalen) {
			break;
		}
		free(data);
		datalen = listlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	pushlist(L, data, data + listlen);
	free(data);
	return (1);
}

static int
l_extattr_list_file(lua_State *L)
{
	const char *path;
	void *data;
	ssize_t datalen, listlen;
	int attrnamespace, error;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);

	data = NULL;
	datalen = 0;
	for (;;) {
		if ((listlen = extattr_list_file(path, attrnamespace, data,
		    datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (listlen <= datalen) {
			break;
		}
		free(data);
		datalen = listlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	pushlist(L, data, data + listlen);
	free(data);
	return (1);
}

static int
l_extattr_list_link(lua_State *L)
{
	const char *path;
	void *data;
	ssize_t datalen, listlen;
	int attrnamespace, error;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);

	data = NULL;
	datalen = 0;
	for (;;) {
		if ((listlen = extattr_list_link(path, attrnamespace, data,
		    datalen)) == -1) {
			error = errno;
			free(data);
			return (fail(L, error));
		}
		if (listlen <= datalen) {
			break;
		}
		free(data);
		datalen = listlen * 2;
		if ((data = malloc(datalen)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	pushlist(L, data, data + listlen);
	free(data);
	return (1);
}

static int
l_extattr_set(lua_State *L)
{
	luaL_Stream *s;
	const char *attrname;
	const void *data;
	size_t len;
	ssize_t res;
	int attrnamespace, fd;

	s = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);
	data = luaL_checklstring(L, 4, &len);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	if ((res = extattr_set_fd(fd, attrnamespace, attrname, data, len))
	    == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, res);
	return (1);
}

static int
l_extattr_set_fd(lua_State *L)
{
	const char *attrname;
	const void *data;
	size_t len;
	ssize_t res;
	int fd, attrnamespace;

	fd = luaL_checkinteger(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);
	data = luaL_checklstring(L, 4, &len);

	if ((res = extattr_set_fd(fd, attrnamespace, attrname, data, len))
	    == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, res);
	return (1);
}

static int
l_extattr_set_file(lua_State *L)
{
	const char *path, *attrname;
	const void *data;
	size_t len;
	ssize_t res;
	int attrnamespace;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);
	data = luaL_checklstring(L, 4, &len);

	if ((res = extattr_set_file(path, attrnamespace, attrname, data, len))
	    == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, res);
	return (1);
}

static int
l_extattr_set_link(lua_State *L)
{
	const char *path, *attrname;
	const void *data;
	size_t len;
	ssize_t res;
	int attrnamespace;

	path = luaL_checkstring(L, 1);
	attrnamespace = luaL_checkinteger(L, 2);
	attrname = luaL_checkstring(L, 3);
	data = luaL_checklstring(L, 4, &len);

	if ((res = extattr_set_link(path, attrnamespace, attrname, data, len))
	    == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, res);
	return (1);
}

static int
l_extattr_namespace_to_string(lua_State *L)
{
	char *string;
	int attrnamespace;

	attrnamespace = luaL_checkinteger(L, 1);

	if (extattr_namespace_to_string(attrnamespace, &string) == -1) {
		return (fail(L, errno));
	}
	lua_pushstring(L, string);
	free(string);
	return (1);
}

static int
l_extattr_string_to_namespace(lua_State *L)
{
	const char *string;
	int attrnamespace;

	string = luaL_checkstring(L, 1);

	if (extattr_string_to_namespace(string, &attrnamespace) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, attrnamespace);
	return (1);
}

static const struct luaL_Reg l_extattr_funcs[] = {
	{"delete", l_extattr_delete},
	{"delete_fd", l_extattr_delete_fd},
	{"delete_file", l_extattr_delete_file},
	{"delete_link", l_extattr_delete_link},
	{"get", l_extattr_get},
	{"get_fd", l_extattr_get_fd},
	{"get_file", l_extattr_get_file},
	{"get_link", l_extattr_get_link},
	{"list", l_extattr_list},
	{"list_fd", l_extattr_list_fd},
	{"list_file", l_extattr_list_file},
	{"list_link", l_extattr_list_link},
	{"set", l_extattr_set},
	{"set_fd", l_extattr_set_fd},
	{"set_file", l_extattr_set_file},
	{"set_link", l_extattr_set_link},
	{"namespace_to_string", l_extattr_namespace_to_string},
	{"string_to_namespace", l_extattr_string_to_namespace},
	{NULL, NULL}
};

int
luaopen_sys_extattr(lua_State *L)
{
	luaL_newlib(L, l_extattr_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, EXTATTR_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(NAMESPACE_EMPTY);
	DEFINE(NAMESPACE_USER);
	DEFINE(NAMESPACE_SYSTEM);
#undef DEFINE
	return (1);
}
