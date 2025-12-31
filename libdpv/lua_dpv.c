/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/cdefs.h>
#include <assert.h>
#include <dpv.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "luaerror.h"
#include "utils.h"

int luaopen_dpv(lua_State *);

struct file_node {
	struct dpv_file_node file;
	lua_State *L;
	int outref;
};

static int
closestream(lua_State *L)
{
	luaL_Stream *stream;
	int res;

	stream = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	res = fclose(stream->f);
	return (luaL_fileresult(L, res == 0, NULL));
}

static int
action(struct dpv_file_node *file, int out)
{
	struct file_node *node = __containerof(file, struct file_node, file);
	lua_State *L = node->L;
	char *s;
	int top = lua_gettop(L);
	int idx, pct;

	assert(lua_isfunction(L, top));
	/* Set up the file. */
	lua_newtable(L);
	idx = lua_gettop(L);
	lua_pushinteger(L, file->status);
	lua_setfield(L, idx, "status");
	if (file->msg != NULL) {
		lua_pushstring(L, file->msg);
		lua_setfield(L, idx, "msg");
	}
	lua_pushstring(L, file->name);
	lua_setfield(L, idx, "name");
	lua_pushstring(L, file->path);
	lua_setfield(L, idx, "path");
	lua_pushinteger(L, file->length);
	lua_setfield(L, idx, "length");
	lua_pushinteger(L, file->read);
	lua_setfield(L, idx, "read");
	/* Push a copy of the function. */
	lua_pushvalue(L, top);
	/* Push a copy of the file. */
	lua_pushvalue(L, idx);
	/* Push out. */
	if (out > 0) {
		/* 
		 * `out' is opened per-file and reused by actions.  Cache the
		 *  stream and its file handle with a ref stored in the node.
		 */
		if (node->outref == LUA_NOREF) {
			luaL_Stream *outs;

			outs = lua_newuserdatauv(L, sizeof(*outs), 0);
			if ((outs->f = fdopen(out, "w")) == NULL) {
				file->status = DPV_STATUS_FAILED;
				if (asprintf(&s, "fdopen: %m") != -1) {
					free(file->msg);
					file->msg = s;
				}
				dpv_abort = true;
				lua_settop(L, top);
				return (-1);
			}
			outs->closef = closestream;
			luaL_setmetatable(L, LUA_FILEHANDLE);
			lua_pushvalue(L, -1);
			node->outref = luaL_ref(L, LUA_REGISTRYINDEX);
		} else {
			lua_rawgeti(L, LUA_REGISTRYINDEX, node->outref);
		}
	} else {
		lua_pushnil(L);
	}
	/* Invoke the action. */
	if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
		file->status = DPV_STATUS_FAILED;
		if ((s = strdup(lua_tostring(L, -1))) == NULL) {
			goto strdupfail;
		}
		free(file->msg);
		file->msg = s;
		dpv_abort = true;
		lua_settop(L, top);
		return (-1);
	}
	pct = lua_tointeger(L, -1);
	/* Update the file. */
	lua_getfield(L, idx, "status");
	file->status = lua_tointeger(L, -1);
	if (lua_getfield(L, idx, "msg") == LUA_TSTRING) {
		if ((s = strdup(lua_tostring(L, -1))) == NULL) {
			goto strdupfail;
		}
		free(file->msg);
		file->msg = s;
	}
	lua_getfield(L, idx, "name");
	if ((s = strdup(lua_tostring(L, -1))) == NULL) {
		goto strdupfail;
	}
	free(file->name);
	file->name = s;
	lua_getfield(L, idx, "path");
	if ((s = strdup(lua_tostring(L, -1))) == NULL) {
		goto strdupfail;
	}
	free(file->path);
	file->path = s;
	lua_getfield(L, idx, "length");
	file->length = lua_tointeger(L, -1);
	lua_getfield(L, idx, "read");
	file->read = lua_tointeger(L, -1);
	/* Restore the stack. */
	lua_settop(L, top);
	return (pct);
strdupfail:
	file->status = DPV_STATUS_FAILED;
	if (asprintf(&s, "strdup: %m") != -1) {
		free(file->msg);
		file->msg = s;
	}
	dpv_abort = true;
	lua_settop(L, top);
	return (-1);
}

static inline void
freenodes(struct file_node *nodes, lua_Integer n)
{
	for (lua_Integer i = 0; i < n; i++) {
		struct dpv_file_node *file = &nodes[i].file;

		luaL_unref(nodes[i].L, LUA_REGISTRYINDEX, nodes[i].outref);
		free(file->msg);
		free(file->name);
		free(file->path);
	}
	free(nodes);
}

static int
l_dpv(lua_State *L)
{
	struct dpv_config config = {0};
	struct dpv_config *configp;
	struct dpv_file_node *files;
	struct file_node *nodes;
	lua_Integer n;
	int fidx;

	lua_remove(L, 1); /* dpv table */
	luaL_argexpected(L, lua_istable(L, 2), 2, "table");
	if (lua_isnil(L, 1)) {
		configp = NULL;
	} else {
		luaL_argexpected(L, lua_istable(L, 1), 1, "table or nil");
		configp = &config;
		switch (lua_getfield(L, 1, "keep_tite")) {
		case LUA_TNIL:
			config.keep_tite = false;
			break;
		case LUA_TBOOLEAN:
			config.keep_tite = lua_toboolean(L, -1);
			break;
		default:
			return (luaL_argerror(L, 1,
			    "keep_tite: boolean expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "display_type")) {
		case LUA_TNIL:
			config.display_type = DPV_DISPLAY_LIBDIALOG;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.display_type = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			return (luaL_argerror(L, 1,
			    "display_type: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "output_type")) {
		case LUA_TNIL:
			config.output_type = DPV_OUTPUT_NONE;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.display_type = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			return (luaL_argerror(L, 1,
			    "output_type: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "debug")) {
		case LUA_TNIL:
			config.debug = 0;
			break;
		case LUA_TBOOLEAN:
			config.debug = lua_toboolean(L, -1);
			break;
		default:
			return (luaL_argerror(L, 1,
			    "debug: boolean expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "display_limit")) {
		case LUA_TNIL:
			/* dpv_private.h: DISPLAY_LIMIT_DEFAULT */
			config.display_limit = 0;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.display_limit = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			return (luaL_argerror(L, 1,
			    "display_limit: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "label_size")) {
		case LUA_TNIL:
			/* dpv_private.h: LABEL_SIZE_DEFAULT */
			config.label_size = 28;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.label_size = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			return (luaL_argerror(L, 1,
			    "label_size: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "pbar_size")) {
		case LUA_TNIL:
			/* dpv_private.h: PBAR_SIZE_DEFAULT */
			config.pbar_size = 17;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.pbar_size = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			return (luaL_argerror(L, 1,
			    "pbar_size: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "dialog_updates_per_second")) {
		case LUA_TNIL:
			/* dpv_private.h: [X]DIALOG_UPDATES_PER_SEC */
			config.dialog_updates_per_second =
			    config.display_type == DPV_DISPLAY_XDIALOG ? 4 : 16;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.dialog_updates_per_second =
				    lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			return (luaL_argerror(L, 1,
			    "dialog_updates_per_second: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "status_updates_per_second")) {
		case LUA_TNIL:
			/* dpv_private.h: STATUS_UPDATES_PER_SEC */
			config.status_updates_per_second = 2;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.status_updates_per_second =
				    lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			return (luaL_argerror(L, 1,
			    "status_updates_per_second: integer expected"));
		}
		lua_pop(L, 1);
		int t;
		switch ((t = lua_getfield(L, 1, "options"))) {
		case LUA_TNIL:
			config.options = 0;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				config.options = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			fprintf(stderr, "...%d\n", t);
			return (luaL_argerror(L, 1,
			    "options: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, 1, "title")) {
		case LUA_TNIL:
			config.title = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.title = __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "title: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "backtitle")) {
		case LUA_TNIL:
			config.backtitle = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.backtitle =
			    __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "backtitle: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "aprompt")) {
		case LUA_TNIL:
			config.aprompt = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.aprompt = __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "aprompt: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "pprompt")) {
		case LUA_TNIL:
			config.pprompt = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.pprompt = __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "pprompt: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "msg_done")) {
		case LUA_TNIL:
			config.msg_done = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.msg_done =
			    __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "msg_done: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "msg_fail")) {
		case LUA_TNIL:
			config.msg_fail = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.msg_fail =
			    __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "msg_fail: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "msg_pending")) {
		case LUA_TNIL:
			config.msg_pending = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.msg_pending =
			    __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "msg_pending: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "output")) {
		case LUA_TNIL:
			config.output = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.output = __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "output: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "status_solo")) {
		case LUA_TNIL:
			config.status_solo = DPV_STATUS_SOLO;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.status_solo =
			    __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "status_solo: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "status_many")) {
		case LUA_TNIL:
			config.status_many = DPV_STATUS_MANY;
			lua_pop(L, 1);
			break;
		case LUA_TSTRING:
			config.status_many =
			    __DECONST(char *, lua_tostring(L, -1));
			break;
		default:
			return (luaL_argerror(L, 1,
			    "status_many: string expected"));
		}
		/* string needs to stay on the stack */
		switch (lua_getfield(L, 1, "action")) {
		case LUA_TNIL:
			config.action = NULL;
			lua_pop(L, 1);
			break;
		case LUA_TFUNCTION:
			config.action = action;
			fidx = lua_gettop(L);
			break;
		default:
			return (luaL_argerror(L, 1,
			    "action: function expected"));
		}
		/* function needs to stay on the stack */
	}
	if ((n = luaL_len(L, 2)) == 0) {
		nodes = NULL;
		files = NULL;
	} else if ((nodes = calloc(n, sizeof(*nodes))) == NULL) {
		return (fatal(L, "calloc", ENOMEM));
	}
	for (lua_Integer i = 0; i < n; i++) {
		struct dpv_file_node *file = &nodes[i].file;
		int idx;

		nodes[i].L = L;
		nodes[i].outref = LUA_NOREF;
		if (lua_geti(L, 2, i + 1) != LUA_TTABLE) {
			freenodes(nodes, i);
			return (luaL_argerror(L, 2, "tables expected"));
		}
		idx = lua_gettop(L);
		switch (lua_getfield(L, idx, "status")) {
		case LUA_TNIL:
			file->status = DPV_STATUS_RUNNING;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				file->status = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			freenodes(nodes, i);
			return (luaL_argerror(L, 2,
			    "status: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, idx, "msg")) {
		case LUA_TNIL:
			file->msg = NULL;
			break;
		case LUA_TSTRING:
			file->msg = strdup(lua_tostring(L, -1));
			if (file->msg == NULL) {
				freenodes(nodes, i);
				return (fatal(L, "strdup", ENOMEM));
			}
			break;
		default:
			freenodes(nodes, i);
			return (luaL_argerror(L, 2,
			    "msg: string expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, idx, "name")) {
		case LUA_TSTRING:
			file->name = strdup(lua_tostring(L, -1));
			if (file->name == NULL) {
				freenodes(nodes, i);
				return (fatal(L, "strdup", ENOMEM));
			}
			break;
		default:
			freenodes(nodes, i);
			return (luaL_argerror(L, 2,
			    "name: string expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, idx, "path")) {
		case LUA_TSTRING:
			file->path = strdup(lua_tostring(L, -1));
			if (file->path == NULL) {
				freenodes(nodes, i);
				return (fatal(L, "strdup", ENOMEM));
			}
			break;
		default:
			freenodes(nodes, i);
			return (luaL_argerror(L, 2,
			    "path: string expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, idx, "length")) {
		case LUA_TNIL:
			file->length = -1;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				file->length = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			freenodes(nodes, i);
			return (luaL_argerror(L, 2,
			    "length: integer expected"));
		}
		lua_pop(L, 1);
		switch (lua_getfield(L, idx, "read")) {
		case LUA_TNIL:
			file->read = 0;
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, -1)) {
				file->read = lua_tointeger(L, -1);
				break;
			}
			__attribute__((fallthrough));
		default:
			freenodes(nodes, i);
			return (luaL_argerror(L, 2,
			    "read: integer expected"));
		}
		lua_pop(L, 1);
		file->next = i + 1 == n ? NULL : &nodes[i + 1].file;
	}
	if (configp != NULL && config.action != NULL) {
		lua_pushvalue(L, fidx);
	}
	if (nodes != NULL) {
		files = &nodes[0].file;
	}
	lua_pushboolean(L, dpv(configp, files) == 0);
	freenodes(nodes, n);
	dpv_free();
	return (1);
}

static int
l_dpv_abort(lua_State *L)
{
	if (lua_isboolean(L, 1)) {
		dpv_abort = lua_toboolean(L, 1);
	}

	lua_pushboolean(L, dpv_abort);
	return (1);
}

static int
l_dpv_interrupt(lua_State *L)
{
	if (lua_isboolean(L, 1)) {
		dpv_interrupt = lua_toboolean(L, 1);
	}

	lua_pushboolean(L, dpv_interrupt);
	return (1);
}

static int
l_dpv_overall_read(lua_State *L)
{
	if (lua_isinteger(L, 1)) {
		dpv_overall_read = lua_tointeger(L, 1);
	}

	lua_pushinteger(L, dpv_overall_read);
	return (1);
}

static const struct luaL_Reg l_dpv_funcs[] = {
	{"__call", l_dpv},
	{"abort", l_dpv_abort},
	{"interrupt", l_dpv_interrupt},
	{"overall_read", l_dpv_overall_read},
	{NULL, NULL}
};

int
luaopen_dpv(lua_State *L)
{
	lua_newtable(L); /* dpv */

	luaL_newlib(L, l_dpv_funcs); /* dpv metatable */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_newtable(L); /* dpv.display */
#define DPV_DISPLAY(ident) ({ \
	lua_pushinteger(L, DPV_DISPLAY_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DPV_DISPLAY(LIBDIALOG);
	DPV_DISPLAY(STDOUT);
	DPV_DISPLAY(DIALOG);
	DPV_DISPLAY(XDIALOG);
#undef DPV_DISPLAY
	lua_setfield(L, -2, "display");

	lua_newtable(L); /* dpv.options */
#define DPV(ident) ({ \
	lua_pushinteger(L, DPV_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DPV(TEST_MODE);
	DPV(WIDE_MODE);
	DPV(NO_LABELS);
	DPV(USE_COLOR);
	DPV(NO_OVERRUN);
#undef DPV
	lua_setfield(L, -2, "options");

	lua_newtable(L); /* dpv.output */
#define DPV_OUTPUT(ident) ({ \
	lua_pushinteger(L, DPV_OUTPUT_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DPV_OUTPUT(NONE);
	DPV_OUTPUT(FILE);
	DPV_OUTPUT(SHELL);
#undef DPV_OUTPUT
	lua_setfield(L, -2, "output");

	lua_newtable(L); /* dpv.status */
#define DPV_STATUS(ident) ({ \
	lua_pushinteger(L, DPV_STATUS_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DPV_STATUS(RUNNING);
	DPV_STATUS(DONE);
	DPV_STATUS(FAILED);
#undef DPV_STATUS
	lua_setfield(L, -2, "status");

	return (lua_setmetatable(L, -2));
}
