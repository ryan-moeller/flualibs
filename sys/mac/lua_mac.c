/*
 * Copyright (c) 2025-2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/mac.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define MAC_METATABLE "mac_t"

int luaopen_sys_mac(lua_State *);

static int
l_mac_is_present(lua_State *L)
{
	const char *policyname;
	int present;

	policyname = luaL_optstring(L, 1, NULL);

	if ((present = mac_is_present(policyname)) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, present);
	return (1);
}

static int
l_mac_syscall(lua_State *L)
{
	const char *policyname;
	const void *p;
	void *arg;
	size_t len;
	int call, error;

	policyname = luaL_checkstring(L, 1);
	call = luaL_checkinteger(L, 2);
	p = luaL_optlstring(L, 3, NULL, &len);

	if (p == NULL) {
		arg = NULL;
	} else {
		if ((arg = malloc(len)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
		memcpy(arg, p, len);
	}
	if (mac_syscall(policyname, call, arg) == -1) {
		error = errno;
		free(arg);
		return (fail(L, error));
	}
	lua_pushlstring(L, arg, len);
	free(arg);
	return (1);
}

static int
l_mac_from_text(lua_State *L)
{
	mac_t label;
	const char *text;

	text = luaL_checkstring(L, 1);

	if (mac_from_text(&label, text) == -1) {
		return (fail(L, errno));
	}
	return (new(L, label, MAC_METATABLE));
}

static int
l_mac_prepare(lua_State *L)
{
	mac_t label;
	const char *elements;

	elements = luaL_checkstring(L, 1);

	if (mac_prepare(&label, elements) == -1) {
		return (fail(L, errno));
	}
	return (new(L, label, MAC_METATABLE));
}

static int
l_mac_prepare_file_label(lua_State *L)
{
	mac_t label;

	if (mac_prepare_file_label(&label) == -1) {
		return (fail(L, errno));
	}
	return (new(L, label, MAC_METATABLE));
}

static int
l_mac_prepare_ifnet_label(lua_State *L)
{
	mac_t label;

	if (mac_prepare_ifnet_label(&label)== -1) {
		return (fail(L, errno));
	}
	return (new(L, label, MAC_METATABLE));
}

static int
l_mac_prepare_process_label(lua_State *L)
{
	mac_t label;

	if (mac_prepare_process_label(&label)== -1) {
		return (fail(L, errno));
	}
	return (new(L, label, MAC_METATABLE));
}

static int
l_mac_prepare_type(lua_State *L)
{
	mac_t label;
	const char *type;

	type = luaL_checkstring(L, 1);

	if (mac_prepare_type(&label, type) == -1) {
		return (fail(L, errno));
	}
	return (new(L, label, MAC_METATABLE));
}

static int
l_mac_gc(lua_State *L)
{
	mac_t label;

	label = checkcookie(L, 1, MAC_METATABLE);

	if (mac_free(label) == -1) {
		return (fatal(L, "mac_free", errno));
	}
	return (0);
}

extern char **environ;

static inline void
freeenvv(char **envv, int envc)
{
	if (envv != environ) {
		for (int i = 0; i < envc; i++) {
			free(envv[i]);
		}
		free(envv);
	}
}

static int
l_mac_execve(lua_State *L)
{
	mac_t label;
	const char *fname;
	const char **argv;
	char **envv;
	int argc, envc, error;

	label = checkcookie(L, 1, MAC_METATABLE);
	fname = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	envv = lua_istable(L, 4) ? NULL : environ;

	argc = luaL_len(L, 3);
	if ((argv = malloc((argc + 1) * sizeof(*argv))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	for (int i = 0; i < argc; i++) {
		lua_geti(L, 3, i + 1);
		argv[i] = lua_tostring(L, -1);
	}
	argv[argc] = NULL;
	if (envv == NULL) {
		envc = 0;
		lua_pushnil(L);
		while (lua_next(L, 4) != 0) {
			envc++;
			lua_pop(L, 1);
		}
		if ((envv = calloc(envc + 1, sizeof(*envv))) == NULL) {
			free(argv);
			return (fatal(L, "malloc", ENOMEM));
		}
		lua_pushnil(L);
		for (int i = 0; i < envc; i++) {
			const char *name, *value;
			size_t namelen, valuelen;

			lua_next(L, 4);
			if ((name = lua_tolstring(L, -2, &namelen)) == NULL ||
			    (value = lua_tolstring(L, -1, &valuelen)) == NULL) {
				freeenvv(envv, envc);
				free(argv);
				return (luaL_argerror(L, 4,
				    "string conversion failed"));
			}
			if (asprintf(&envv[i], "%.*s=%.*s", (int)namelen, name,
			    (int)valuelen, value) == -1) {
				error = errno;
				freeenvv(envv, envc);
				free(argv);
				return (fatal(L, "asprintf", error));
			}
			lua_pop(L, 1);
		}
	}
	mac_execve(__DECONST(char *, fname), __DECONST(char **, argv), envv,
	    label);
	error = errno;
	freeenvv(envv, envc);
	free(argv);
	return (fail(L, error));
}

static int
l_mac_get(lua_State *L)
{
	mac_t label;
	luaL_Stream *s;
	int fd;

	label = checkcookie(L, 1, MAC_METATABLE);
	s = luaL_checkudata(L, 2, LUA_FILEHANDLE);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	if (mac_get_fd(fd, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_get_fd(lua_State *L)
{
	mac_t label;
	int fd;

	label = checkcookie(L, 1, MAC_METATABLE);
	fd = luaL_checkinteger(L, 2);

	if (mac_get_fd(fd, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_get_file(lua_State *L)
{
	mac_t label;
	const char *path;

	label = checkcookie(L, 1, MAC_METATABLE);
	path = luaL_checkstring(L, 2);

	if (mac_get_file(path, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_get_link(lua_State *L)
{
	mac_t label;
	const char *path;

	label = checkcookie(L, 1, MAC_METATABLE);
	path = luaL_checkstring(L, 2);

	if (mac_get_link(path, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_get_peer(lua_State *L)
{
	mac_t label;
	int fd;

	label = checkcookie(L, 1, MAC_METATABLE);
	fd = luaL_checkinteger(L, 2);

	if (mac_get_peer(fd, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_get_pid(lua_State *L)
{
	mac_t label;
	pid_t pid;

	label = checkcookie(L, 1, MAC_METATABLE);
	pid = luaL_checkinteger(L, 2);

	if (mac_get_pid(pid, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_get_proc(lua_State *L)
{
	mac_t label;

	label = checkcookie(L, 1, MAC_METATABLE);

	if (mac_get_proc(label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_set(lua_State *L)
{
	mac_t label;
	luaL_Stream *s;
	int fd;

	label = checkcookie(L, 1, MAC_METATABLE);
	s = luaL_checkudata(L, 2, LUA_FILEHANDLE);

	if ((fd = fileno(s->f)) < 0) {
		return (fatal(L, "fileno", errno));
	}
	if (mac_set_fd(fd, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_set_fd(lua_State *L)
{
	mac_t label;
	int fd;

	label = checkcookie(L, 1, MAC_METATABLE);
	fd = luaL_checkinteger(L, 2);

	if (mac_set_fd(fd, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_set_file(lua_State *L)
{
	mac_t label;
	const char *path;

	label = checkcookie(L, 1, MAC_METATABLE);
	path = luaL_checkstring(L, 2);

	if (mac_set_file(path, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_set_link(lua_State *L)
{
	mac_t label;
	const char *path;

	label = checkcookie(L, 1, MAC_METATABLE);
	path = luaL_checkstring(L, 2);

	if (mac_set_link(path, label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_set_proc(lua_State *L)
{
	mac_t label;

	label = checkcookie(L, 1, MAC_METATABLE);

	if (mac_set_proc(label) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_mac_to_text(lua_State *L)
{
	mac_t label;
	char *text;

	label = checkcookie(L, 1, MAC_METATABLE);

	if (mac_to_text(label, &text) == -1) {
		return (fail(L, errno));
	}
	lua_pushstring(L, text);
	free(text);
	return (1);
}

static const struct luaL_Reg l_mac_funcs[] = {
	{"is_present", l_mac_is_present},
	{"syscall", l_mac_syscall}, /* XXX: probably not useful */
	{"from_text", l_mac_from_text},
	{"prepare", l_mac_prepare},
	{"prepare_file_label", l_mac_prepare_file_label},
	{"prepare_ifnet_label", l_mac_prepare_ifnet_label},
	{"prepare_process_label", l_mac_prepare_process_label},
	{"prepare_type", l_mac_prepare_type},
	{NULL, NULL}
};

static const struct luaL_Reg l_mac_meta[] = {
	{"__gc", l_mac_gc},
	{"execve", l_mac_execve},
	{"get", l_mac_get},
	{"get_fd", l_mac_get_fd},
	{"get_file", l_mac_get_file},
	{"get_link", l_mac_get_link},
	{"get_peer", l_mac_get_peer},
	{"get_pid", l_mac_get_pid},
	{"get_proc", l_mac_get_proc},
	{"set", l_mac_set},
	{"set_fd", l_mac_set_fd},
	{"set_file", l_mac_set_file},
	{"set_link", l_mac_set_link},
	{"set_proc", l_mac_set_proc},
	{"to_text", l_mac_to_text},
	{NULL, NULL}
};

int
luaopen_sys_mac(lua_State *L)
{
	luaL_newmetatable(L, MAC_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_mac_meta, 0);

	luaL_newlib(L, l_mac_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, MAC_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(MAX_POLICY_NAME);
	DEFINE(MAX_LABEL_ELEMENT_NAME);
	DEFINE(MAX_LABEL_ELEMENT_DATA);
	DEFINE(MAX_LABEL_BUF_LEN);
#undef DEFINE
	return (1);
}
