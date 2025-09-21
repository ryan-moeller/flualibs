/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Ryan Moeller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <bsddialog.h>
#include <bsddialog_theme.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

#define BSDDIALOG_CONF_METATABLE "struct bsddialog_conf *"

int luaopen_bsddialog(lua_State *);

static int
l_bsddialog_init(lua_State *L)
{
	if (bsddialog_init() == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_init: %s", bsddialog_geterror());
	}
	return (0);
}

static int
l_bsddialog_init_notheme(lua_State *L)
{
	if (bsddialog_init_notheme() == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_init_notheme: %s",
		    bsddialog_geterror());
	}
	return (0);
}

#if LIBBSDDIALOG_VERSION_MAJOR > 0
static int
l_bsddialog_inmode(lua_State *L)
{
	lua_pushboolean(L, bsddialog_inmode());
	return (1);
}

static int
l_bsddialog_clear(lua_State *L)
{
	unsigned int y;

	y = luaL_checkinteger(L, 1);

	bsddialog_clear(y);
	return (0);
}

static int
l_bsddialog_refresh(lua_State *L __unused)
{
	bsddialog_refresh();
	return (0);
}
#endif

static int
l_bsddialog_end(lua_State *L)
{
	if (bsddialog_end() == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_end: %s", bsddialog_geterror());
	}
	return (0);
}

static int
l_bsddialog_initconf(lua_State *L)
{
	struct bsddialog_conf *conf;

	conf = lua_newuserdatauv(L, sizeof(*conf), 0);
	luaL_setmetatable(L, BSDDIALOG_CONF_METATABLE);
	if (bsddialog_initconf(conf) == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_initconf: %s", bsddialog_geterror());
	}
	return (1);
}

static int
l_bsddialog_backtitle(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *backtitle;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	backtitle = luaL_checkstring(L, 2);

	if (bsddialog_backtitle(conf, backtitle) == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_backtitle: %s", bsddialog_geterror());
	}
	return (0);
}

static int
l_bsddialog_calendar(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int year, month, day;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	year = luaL_checkinteger(L, 5);
	month = luaL_checkinteger(L, 6);
	day = luaL_checkinteger(L, 7);

	rv = bsddialog_calendar(conf, text, rows, cols, &year, &month, &day);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_calendar: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, year);
	lua_pushinteger(L, month);
	lua_pushinteger(L, day);
	return (4);
}

static struct bsddialog_menuitem *
checkmenuitems(lua_State *L, int index, unsigned int *nitems)
{
	struct bsddialog_menuitem *items;
	size_t len;

	luaL_checktype(L, index, LUA_TTABLE);
	len = luaL_len(L, index);
	*nitems = len;
	items = calloc(len, sizeof(*items));
	if (items == NULL) {
		luaL_error(L, "no memory");
	}
	for (size_t i = 0; i < len; i++) {
		lua_geti(L, index, i + 1);
		luaL_checktype(L, -1, LUA_TTABLE);

#define COPYSTRING(field) do {                                   \
		lua_getfield(L, -1, #field);                     \
		items[i].field = lua_tostring(L, -1);            \
		if (items[i].field != NULL) {                    \
			items[i].field = strdup(items[i].field); \
			if (items[i].field == NULL) {            \
				luaL_error(L, "no memory");      \
			}                                        \
		}                                                \
		lua_pop(L, 1);                                   \
	} while (0)

		COPYSTRING(prefix);
		COPYSTRING(name);
		COPYSTRING(desc);
		COPYSTRING(bottomdesc);

#undef COPYSTRING

		lua_getfield(L, -1, "on");
		items[i].on = lua_toboolean(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "depth");
		items[i].depth = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pop(L, 1);
	}
	return (items);
}

static void
freemenuitems(struct bsddialog_menuitem *items, unsigned int nitems)
{
	while (nitems--) {
		free(__DECONST(char *, items[nitems].prefix));
		free(__DECONST(char *, items[nitems].name));
		free(__DECONST(char *, items[nitems].desc));
		free(__DECONST(char *, items[nitems].bottomdesc));
	}
	free(items);
}

static int
l_bsddialog_checklist(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int menurows, nitems;
	struct bsddialog_menuitem *items;
	int focusitem;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	menurows = luaL_checkinteger(L, 5);
	items = checkmenuitems(L, 6, &nitems);
	focusitem = luaL_checkinteger(L, 7) - 1;

	rv = bsddialog_checklist(conf, text, rows, cols, menurows, nitems,
	    items, &focusitem);
	freemenuitems(items, nitems);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_checklist: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, focusitem + 1);
	return (2);
}

static int
l_bsddialog_datebox(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int year, month, day;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	year = luaL_checkinteger(L, 5);
	month = luaL_checkinteger(L, 6);
	day = luaL_checkinteger(L, 7);

	rv = bsddialog_datebox(conf, text, rows, cols, &year, &month, &day);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_datebox: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, year);
	lua_pushinteger(L, month);
	lua_pushinteger(L, day);
	return (4);
}

static struct bsddialog_formitem *
checkformitems(lua_State *L, int index, unsigned int *nitems)
{
	struct bsddialog_formitem *items;
	size_t len;

	luaL_checktype(L, index, LUA_TTABLE);
	len = luaL_len(L, index);
	*nitems = len;
	items = calloc(len, sizeof(*items));
	if (items == NULL) {
		luaL_error(L, "no memory");
	}
	for (size_t i = 0; i < len; i++) {
		lua_geti(L, index, i + 1);
		luaL_checktype(L, -1, LUA_TTABLE);

#define COPYSTRING(field) do {                                   \
		lua_getfield(L, -1, #field);                     \
		items[i].field = lua_tostring(L, -1);            \
		if (items[i].field != NULL) {                    \
			items[i].field = strdup(items[i].field); \
			if (items[i].field == NULL) {            \
				luaL_error(L, "no memory");      \
			}                                        \
		}                                                \
		lua_pop(L, 1);                                   \
	} while (0)

		COPYSTRING(label);
		COPYSTRING(init);
		COPYSTRING(bottomdesc);

#undef COPYSTRING

		lua_getfield(L, -1, "ylabel");
		items[i].ylabel = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "xlabel");
		items[i].xlabel = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "yfield");
		items[i].yfield = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "xfield");
		items[i].xfield = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "fieldlen");
		items[i].fieldlen = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "maxvaluelen");
		items[i].maxvaluelen = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "flags");
		items[i].flags = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_pop(L, 1);
	}
	return (items);
}

static void
pushformitems(lua_State *L, struct bsddialog_formitem *items,
    unsigned int nitems)
{
	lua_createtable(L, nitems, 0);
	for (unsigned int i = 0; i < nitems; i++) {
		lua_pushstring(L, items[i].value);
		lua_seti(L, -2, i + 1);
	}
}

static void
freeformitems(struct bsddialog_formitem *items, unsigned int nitems)
{
	while (nitems--) {
		free(items[nitems].value);
		free(__DECONST(char *, items[nitems].label));
		free(__DECONST(char *, items[nitems].init));
		free(__DECONST(char *, items[nitems].bottomdesc));
	}
	free(items);
}

static int
l_bsddialog_form(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int formrows, nitems;
	struct bsddialog_formitem *items;
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	int focusitem;
#endif
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	formrows = luaL_checkinteger(L, 5);
	items = checkformitems(L, 6, &nitems);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	focusitem = luaL_checkinteger(L, 7) - 1;
#endif

#if LIBBSDDIALOG_VERSION_MAJOR > 0
	rv = bsddialog_form(conf, text, rows, cols, formrows, nitems, items,
	    &focusitem);
#else
	rv = bsddialog_form(conf, text, rows, cols, formrows, nitems, items);
#endif
	if (rv == BSDDIALOG_ERROR) {
		freeformitems(items, nitems);
		luaL_error(L, "bsddialog_form: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	pushformitems(L, items, nitems);
	freeformitems(items, nitems);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	lua_pushinteger(L, focusitem + 1);
	return (3);
#else
	return (2);
#endif
}

static int
l_bsddialog_gauge(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int perc;
	int fd;
	const char *sep;
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	const char *end;
#endif
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	perc = luaL_checkinteger(L, 5);
	if (lua_gettop(L) == 5) {
		fd = -1;
		sep = NULL;
#if LIBBSDDIALOG_VERSION_MAJOR > 0
		end = NULL;
#endif
	} else {
		luaL_Stream *stream;

		stream = luaL_checkudata(L, 6, LUA_FILEHANDLE);
		fd = fileno(stream->f);
		sep = luaL_checkstring(L, 7);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
		end = luaL_checkstring(L, 8);
#endif
	}

#if LIBBSDDIALOG_VERSION_MAJOR > 0
	rv = bsddialog_gauge(conf, text, rows, cols, perc, fd, sep, end);
#else
	rv = bsddialog_gauge(conf, text, rows, cols, perc, fd, sep);
#endif
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_gauge: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	return (1);
}

static int
l_bsddialog_infobox(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);

	rv = bsddialog_infobox(conf, text, rows, cols);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_infobox: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	return (1);
}

static int
l_bsddialog_menu(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int menurows, nitems;
	struct bsddialog_menuitem *items;
	int focusitem;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	menurows = luaL_checkinteger(L, 5);
	items = checkmenuitems(L, 6, &nitems);
	focusitem = luaL_checkinteger(L, 7) - 1;

	rv = bsddialog_menu(conf, text, rows, cols, menurows, nitems, items,
	    &focusitem);
	freemenuitems(items, nitems);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_menu: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, focusitem + 1);
	return (2);
}

static const char **
checkminibars(lua_State *L, int index, unsigned int *nminibars,
    int **pminipercs)
{
	const char **minilabels;
	int *minipercs;
	size_t len;

	luaL_checktype(L, index, LUA_TTABLE);
	len = luaL_len(L, index);
	*nminibars = len;
	minilabels = calloc(len, sizeof(*minilabels));
	minipercs = calloc(len, sizeof(*minipercs));
	if (minilabels == NULL || minipercs == NULL) {
		luaL_error(L, "no memory");
	}

	for (size_t i = 0; i < len; i++) {
		lua_geti(L, index, i + 1);
		luaL_checktype(L, -1, LUA_TTABLE);

		lua_geti(L, -1, 1);
		minilabels[i] = luaL_checkstring(L, -1);
		minilabels[i] = strdup(minilabels[i]);
		lua_pop(L, 1);

		lua_geti(L, -1, 2);
		minipercs[i] = luaL_checkinteger(L, -1);
		lua_pop(L, 1);

		lua_pop(L, 1);
	}
	return (minilabels);
}

static void
freeminibars(const char **minilabels, int *minipercs, unsigned int nminibars)
{
	while (nminibars--) {
		free(__DECONST(char *, minilabels[nminibars]));
	}
	free(minilabels);
	free(minipercs);
}

static int
l_bsddialog_mixedgauge(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int mainperc, nminibars;
	const char **minilabels;
	int *minipercs;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	mainperc = luaL_checkinteger(L, 5);
	minilabels = checkminibars(L, 6, &nminibars, &minipercs);

	rv = bsddialog_mixedgauge(conf, text, rows, cols, mainperc, nminibars,
	    minilabels, minipercs);
	freeminibars(minilabels, minipercs, nminibars);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_mixedgauge: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	return (2);
}

static struct bsddialog_menugroup *
checkmenugroups(lua_State *L, int index, unsigned int *ngroups)
{
	struct bsddialog_menugroup *groups;
	size_t len;

	luaL_checktype(L, index, LUA_TTABLE);
	len = luaL_len(L, index);
	*ngroups = len;
	groups = calloc(len, sizeof(*groups));
	if (groups == NULL) {
		luaL_error(L, "no memory");
	}
	for (size_t i = 0; i < len; i++) {
		lua_geti(L, index, i + 1);
		luaL_checktype(L, -1, LUA_TTABLE);

		lua_getfield(L, -1, "type");
		groups[i].type = lua_tointeger(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "items");
		groups[i].items = checkmenuitems(L, -1, &groups[i].nitems);
		lua_pop(L, 1);

		/* min_on unused */

		lua_pop(L, 1);
	}
	return (groups);
}

static void
freemenugroups(struct bsddialog_menugroup *groups, unsigned int ngroups)
{
	while (ngroups--) {
		freemenuitems(groups[ngroups].items, groups[ngroups].nitems);
	}
	free(groups);
}

static int
l_bsddialog_mixedlist(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int menurows, ngroups;
	struct bsddialog_menugroup *groups;
	int focuslist, focusitem;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	menurows = luaL_checkinteger(L, 5);
	groups = checkmenugroups(L, 6, &ngroups);
	focuslist = luaL_checkinteger(L, 7);
	focusitem = luaL_checkinteger(L, 8);

	rv = bsddialog_mixedlist(conf, text, rows, cols, menurows, ngroups,
	    groups, &focuslist, &focusitem);
	freemenugroups(groups, ngroups);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_mixedlist: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, focuslist);
	lua_pushinteger(L, focusitem);
	return (3);
}

static int
l_bsddialog_msgbox(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);

	rv = bsddialog_msgbox(conf, text, rows, cols);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_msgbox: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	return (1);
}

static int
l_bsddialog_pause(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int seconds;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	seconds = luaL_checkinteger(L, 5);

#if LIBBSDDIALOG_VERSION_MAJOR > 0
	rv = bsddialog_pause(conf, text, rows, cols, &seconds);
#else
	rv = bsddialog_pause(conf, text, rows, cols, seconds);
#endif
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_pause: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	lua_pushinteger(L, seconds);
	return (2);
#else
	return (1);
#endif
}

static int
l_bsddialog_radiolist(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int menurows, nitems;
	struct bsddialog_menuitem *items;
	int focusitem;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	menurows = luaL_checkinteger(L, 5);
	items = checkmenuitems(L, 6, &nitems);
	focusitem = luaL_checkinteger(L, 7) - 1;

	rv = bsddialog_radiolist(conf, text, rows, cols, menurows, nitems,
	    items, &focusitem);
	freemenuitems(items, nitems);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_radiolist: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, focusitem);
	return (2);
}

static int
l_bsddialog_rangebox(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols, min, max, value;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	min = luaL_checkinteger(L, 5);
	max = luaL_checkinteger(L, 6);
	value = luaL_checkinteger(L, 7);

	rv = bsddialog_rangebox(conf, text, rows, cols, min, max, &value);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_rangebox: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, value);
	return (2);
}

static int
l_bsddialog_textbox(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *file;
	int rows, cols;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	file = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);

	rv = bsddialog_textbox(conf, file, rows, cols);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_textbox: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	return (1);
}

static int
l_bsddialog_timebox(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	unsigned int hh, mm, ss;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);
	hh = luaL_checkinteger(L, 5);
	mm = luaL_checkinteger(L, 6);
	ss = luaL_checkinteger(L, 7);

	rv = bsddialog_timebox(conf, text, rows, cols, &hh, &mm, &ss);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_timebox: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	lua_pushinteger(L, hh);
	lua_pushinteger(L, mm);
	lua_pushinteger(L, ss);
	return (4);
}

static int
l_bsddialog_yesno(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *text;
	int rows, cols;
	int rv;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	text = luaL_checkstring(L, 2);
	rows = luaL_checkinteger(L, 3);
	cols = luaL_checkinteger(L, 4);

	rv = bsddialog_yesno(conf, text, rows, cols);
	if (rv == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_yesno: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, rv);
	return (1);
}

static int
l_bsddialog_conf_proxy_index(lua_State *L)
{
	const char *parent, *child;

	parent = luaL_checkstring(L, lua_upvalueindex(2));
	child = luaL_checkstring(L, 2);

	lua_pushfstring(L, "%s.%s", parent, child);
	lua_gettable(L, lua_upvalueindex(1));
	return (1);
}

static int
l_bsddialog_conf_proxy_newindex(lua_State *L)
{
	const char *parent, *child;

	parent = luaL_checkstring(L, lua_upvalueindex(2));
	child = luaL_checkstring(L, 2);

	lua_pushfstring(L, "%s.%s", parent, child);
	lua_pushvalue(L, 3);
	lua_settable(L, lua_upvalueindex(1));
	return (0);
}

static void
pushconfproxy(lua_State *L)
{
	lua_createtable(L, 0, 0);
	lua_createtable(L, 0, 2);
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_pushcclosure(L, l_bsddialog_conf_proxy_index, 2);
	lua_setfield(L, -2, "__index");
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_pushcclosure(L, l_bsddialog_conf_proxy_newindex, 2);
	lua_setfield(L, -2, "__newindex");
	lua_setmetatable(L, -2);
}

static int
l_bsddialog_conf_index(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *key;
	char ch[2];

	memset(ch, 0, sizeof(ch));

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	key = luaL_checkstring(L, 2);

	if (strcmp(key, "ascii_lines") == 0) {
		lua_pushboolean(L, conf->ascii_lines);
		return (1);
	}
	if (strcmp(key, "auto_minheight") == 0) {
		lua_pushinteger(L, conf->auto_minheight);
		return (1);
	}
	if (strcmp(key, "auto_minwidth") == 0) {
		lua_pushinteger(L, conf->auto_minwidth);
		return (1);
	}
	if (strcmp(key, "auto_topmargin") == 0) {
		lua_pushinteger(L, conf->auto_topmargin);
		return (1);
	}
	if (strcmp(key, "auto_downmargin") == 0) {
		lua_pushinteger(L, conf->auto_downmargin);
		return (1);
	}
	if (strcmp(key, "bottomtitle") == 0) {
		lua_pushstring(L, conf->bottomtitle);
		return (1);
	}
	if (strcmp(key, "clear") == 0) {
		lua_pushboolean(L, conf->clear);
		return (1);
	}
	/* TODO: get_height and get_width need special treatment */
	if (strcmp(key, "no_lines") == 0) {
		lua_pushboolean(L, conf->no_lines);
		return (1);
	}
	if (strcmp(key, "shadow") == 0) {
		lua_pushboolean(L, conf->shadow);
		return (1);
	}
	if (strcmp(key, "sleep") == 0) {
		lua_pushinteger(L, conf->sleep);
		return (1);
	}
	if (strcmp(key, "title") == 0) {
		lua_pushstring(L, conf->title);
		return (1);
	}
	if (strcmp(key, "y") == 0) {
		lua_pushinteger(L, conf->y);
		return (1);
	}
	if (strcmp(key, "x") == 0) {
		lua_pushinteger(L, conf->x);
		return (1);
	}
	if (strcmp(key, "key") == 0) {
		pushconfproxy(L);
		return (1);
	}
	if (strcmp(key, "key.enable_esc") == 0) {
		lua_pushboolean(L, conf->key.enable_esc);
		return (1);
	}
	if (strcmp(key, "key.f1_file") == 0) {
		lua_pushstring(L, conf->key.f1_file);
		return (1);
	}
	if (strcmp(key, "key.f1_message") == 0) {
		lua_pushstring(L, conf->key.f1_message);
		return (1);
	}
	if (strcmp(key, "text") == 0) {
		pushconfproxy(L);
		return (1);
	}
	if (strcmp(key, "text.cols_per_row") == 0) {
		lua_pushinteger(L, conf->text.cols_per_row);
		return (1);
	}
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	if (strcmp(key, "text.escape") == 0) {
		lua_pushboolean(L, conf->text.escape);
		return (1);
	}
#endif
	if (strcmp(key, "text.tablen") == 0) {
		lua_pushinteger(L, conf->text.tablen);
		return (1);
	}
	if (strcmp(key, "menu") == 0) {
		pushconfproxy(L);
		return (1);
	}
	if (strcmp(key, "menu.align_left") == 0) {
		lua_pushboolean(L, conf->menu.align_left);
		return (1);
	}
	if (strcmp(key, "menu.no_desc") == 0) {
		lua_pushboolean(L, conf->menu.no_desc);
		return (1);
	}
	if (strcmp(key, "menu.no_name") == 0) {
		lua_pushboolean(L, conf->menu.no_name);
		return (1);
	}
	if (strcmp(key, "menu.shortcut_buttons") == 0) {
		lua_pushboolean(L, conf->menu.shortcut_buttons);
		return (1);
	}
	if (strcmp(key, "form") == 0) {
		pushconfproxy(L);
		return (1);
	}
	if (strcmp(key, "form.securech") == 0) {
		ch[0] = conf->form.securech;
		lua_pushstring(L, ch);
		return (1);
	}
	/* TODO: form.securembch, form.value_wchar */
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	if (strcmp(key, "date") == 0) {
		pushconfproxy(L);
		return (1);
	}
	if (strcmp(key, "date.format") == 0) {
		lua_pushstring(L, conf->date.format);
		return (1);
	}
#endif
	if (strcmp(key, "button") == 0) {
		pushconfproxy(L);
		return (1);
	}
	if (strcmp(key, "button.always_active") == 0) {
		lua_pushboolean(L, conf->button.always_active);
		return (1);
	}
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	if (strcmp(key, "button.left1_label") == 0) {
		lua_pushstring(L, conf->button.left1_label);
		return (1);
	}
	if (strcmp(key, "button.left2_label") == 0) {
		lua_pushstring(L, conf->button.left2_label);
		return (1);
	}
	if (strcmp(key, "button.left3_label") == 0) {
		lua_pushstring(L, conf->button.left3_label);
		return (1);
	}
#endif
	if (strcmp(key, "button.without_ok") == 0) {
		lua_pushboolean(L, conf->button.without_ok);
		return (1);
	}
	if (strcmp(key, "button.ok_label") == 0) {
		lua_pushstring(L, conf->button.ok_label);
		return (1);
	}
	if (strcmp(key, "button.with_extra") == 0) {
		lua_pushboolean(L, conf->button.with_extra);
		return (1);
	}
	if (strcmp(key, "button.extra_label") == 0) {
		lua_pushstring(L, conf->button.extra_label);
		return (1);
	}
	if (strcmp(key, "button.without_cancel") == 0) {
		lua_pushboolean(L, conf->button.without_cancel);
		return (1);
	}
	if (strcmp(key, "button.cancel_label") == 0) {
		lua_pushstring(L, conf->button.cancel_label);
		return (1);
	}
	if (strcmp(key, "button.default_cancel") == 0) {
		lua_pushboolean(L, conf->button.default_cancel);
		return (1);
	}
	if (strcmp(key, "button.with_help") == 0) {
		lua_pushboolean(L, conf->button.with_help);
		return (1);
	}
	if (strcmp(key, "button.help_label") == 0) {
		lua_pushstring(L, conf->button.help_label);
		return (1);
	}
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	if (strcmp(key, "button.right1_label") == 0) {
		lua_pushstring(L, conf->button.right1_label);
		return (1);
	}
	if (strcmp(key, "button.right2_label") == 0) {
		lua_pushstring(L, conf->button.right2_label);
		return (1);
	}
	if (strcmp(key, "button.right3_label") == 0) {
		lua_pushstring(L, conf->button.right3_label);
		return (1);
	}
#else
	if (strcmp(key, "button.generic1_label") == 0) {
		lua_pushstring(L, conf->button.generic1_label);
		return (1);
	}
	if (strcmp(key, "button.generic2_label") == 0) {
		lua_pushstring(L, conf->button.generic2_label);
		return (1);
	}
#endif
	if (strcmp(key, "button.default_label") == 0) {
		lua_pushstring(L, conf->button.default_label);
		return (1);
	}
	lua_pushnil(L);
	return (1);
}

static int
l_bsddialog_conf_newindex(lua_State *L)
{
	struct bsddialog_conf *conf;
	const char *key, *s;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);
	key = luaL_checkstring(L, 2);

#define B(field) do {                              \
	if (strcmp(key, #field) == 0) {            \
		conf->field = lua_toboolean(L, 3); \
		return (0);                        \
	}                                          \
} while (0)
#define I(field) do {                              \
	if (strcmp(key, #field) == 0) {            \
		conf->field = lua_tointeger(L, 3); \
		return (0);                        \
	}                                          \
} while (0)
#define S(field) do {                                       \
	if (strcmp(key, #field) == 0) {                     \
		free(__DECONST(char *, conf->field));       \
		s = lua_tostring(L, 3);                     \
		if (s != NULL) {                            \
			s = strdup(s);                      \
			if (s == NULL) {                    \
				conf->field = NULL;         \
				luaL_error(L, "no memory"); \
			}                                   \
		}                                           \
		conf->field = s;                            \
		return (0);                                 \
	}                                                   \
} while (0)

	B(ascii_lines);
	I(auto_minheight);
	I(auto_minwidth);
	I(auto_topmargin);
	I(auto_downmargin);
	S(bottomtitle);
	B(clear);
	B(no_lines);
	B(shadow);
	I(sleep);
	S(title);
	I(y);
	I(x);
	B(key.enable_esc);
	S(key.f1_file);
	S(key.f1_message);
	I(text.cols_per_row);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	B(text.escape);
#endif
	I(text.tablen);
	B(menu.align_left);
	B(menu.no_desc);
	B(menu.no_name);
	B(menu.shortcut_buttons);
	if (strcmp(key, "form.securech") == 0) {
		s = lua_tostring(L, 3);
		conf->form.securech = s[0];
		return (0);
	}
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	S(date.format);
#endif
	B(button.always_active);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	S(button.left1_label);
	S(button.left2_label);
	S(button.left3_label);
#endif
	B(button.without_ok);
	S(button.ok_label);
	B(button.with_extra);
	S(button.extra_label);
	B(button.without_cancel);
	S(button.cancel_label);
	B(button.default_cancel);
	B(button.with_help);
	S(button.help_label);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	S(button.right1_label);
	S(button.right2_label);
	S(button.right3_label);
#else
	S(button.generic1_label);
	S(button.generic2_label);
#endif
	S(button.default_label);

#undef S
#undef I
#undef B

	luaL_error(L, "no such config field `%s'", key);
}

static int
l_bsddialog_conf_gc(lua_State *L)
{
	struct bsddialog_conf *conf;

	conf = luaL_checkudata(L, 1, BSDDIALOG_CONF_METATABLE);

	free(__DECONST(char *, conf->bottomtitle));
	free(__DECONST(char *, conf->title));
	free(__DECONST(char *, conf->key.f1_file));
	free(__DECONST(char *, conf->key.f1_message));
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	free(__DECONST(char *, conf->date.format));
	free(__DECONST(char *, conf->button.left1_label));
	free(__DECONST(char *, conf->button.left2_label));
	free(__DECONST(char *, conf->button.left3_label));
#endif
	free(__DECONST(char *, conf->button.ok_label));
	free(__DECONST(char *, conf->button.extra_label));
	free(__DECONST(char *, conf->button.cancel_label));
	free(__DECONST(char *, conf->button.help_label));
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	free(__DECONST(char *, conf->button.right1_label));
	free(__DECONST(char *, conf->button.right2_label));
	free(__DECONST(char *, conf->button.right3_label));
#else
	free(__DECONST(char *, conf->button.generic1_label));
	free(__DECONST(char *, conf->button.generic2_label));
#endif
	free(__DECONST(char *, conf->button.default_label));
	return (0);
}

static int
l_bsddialog_color(lua_State *L)
{
	enum bsddialog_color foreground, background;
	unsigned int flags;
	int color;

	foreground = luaL_checkinteger(L, 1);
	background = luaL_checkinteger(L, 2);
	flags = luaL_checkinteger(L, 3);

	color = bsddialog_color(foreground, background, flags);
	if (color == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_color: %s", bsddialog_geterror());
	}
	lua_pushinteger(L, color);
	return (1);
}

static int
l_bsddialog_color_attrs(lua_State *L)
{
	int color;
	enum bsddialog_color foreground, background;
	unsigned int flags;

	color = luaL_checkinteger(L, 1);

	if (bsddialog_color_attrs(color, &foreground, &background, &flags)
	    == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_color_attrs: %s",
		    bsddialog_geterror());
	}
	lua_pushinteger(L, foreground);
	lua_pushinteger(L, background);
	lua_pushinteger(L, flags);
	return (3);
}

static int
l_bsddialog_hascolors(lua_State *L)
{
	lua_pushboolean(L, bsddialog_hascolors());
	return (1);
}

static int
l_bsddialog_set_default_theme(lua_State *L)
{
	enum bsddialog_default_theme theme;

	theme = luaL_checkinteger(L, 1);

	if (bsddialog_set_default_theme(theme) == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_set_default_theme: %s",
		    bsddialog_geterror());
	}
	return (0);
}

static void
pushtheme(lua_State *L, struct bsddialog_theme *theme)
{
	char delim[2];

	memset(delim, 0, sizeof(delim));

	lua_createtable(L, 0, 7);

	lua_createtable(L, 0, 1);
	lua_pushinteger(L, theme->screen.color);
	lua_setfield(L, -2, "color");
	lua_setfield(L, -2, "screen");

	lua_createtable(L, 0, 3);
	lua_pushinteger(L, theme->shadow.color);
	lua_setfield(L, -2, "color");
	lua_pushinteger(L, theme->shadow.y);
	lua_setfield(L, -2, "y");
	lua_pushinteger(L, theme->shadow.x);
	lua_setfield(L, -2, "x");
	lua_setfield(L, -2, "shadow");

	lua_createtable(L, 0, 7);
	lua_pushinteger(L, theme->dialog.color);
	lua_setfield(L, -2, "color");
	lua_pushboolean(L, theme->dialog.delimtitle);
	lua_setfield(L, -2, "delimtitle");
	lua_pushinteger(L, theme->dialog.titlecolor);
	lua_setfield(L, -2, "titlecolor");
	lua_pushinteger(L, theme->dialog.lineraisecolor);
	lua_setfield(L, -2, "lineraisecolor");
	lua_pushinteger(L, theme->dialog.linelowercolor);
	lua_setfield(L, -2, "linelowercolor");
	lua_pushinteger(L, theme->dialog.bottomtitlecolor);
	lua_setfield(L, -2, "bottomtitlecolor");
	lua_pushinteger(L, theme->dialog.arrowcolor);
	lua_setfield(L, -2, "arrowcolor");
	lua_setfield(L, -2, "dialog");

	lua_createtable(L, 0, 13);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	lua_pushinteger(L, theme->menu.f_prefixcolor);
	lua_setfield(L, -2, "f_prefixcolor");
	lua_pushinteger(L, theme->menu.prefixcolor);
	lua_setfield(L, -2, "prefixcolor");
#endif
	lua_pushinteger(L, theme->menu.f_selectorcolor);
	lua_setfield(L, -2, "f_selectorcolor");
	lua_pushinteger(L, theme->menu.selectorcolor);
	lua_setfield(L, -2, "selectorcolor");
	lua_pushinteger(L, theme->menu.f_namecolor);
	lua_setfield(L, -2, "f_namecolor");
	lua_pushinteger(L, theme->menu.namecolor);
	lua_setfield(L, -2, "namecolor");
	lua_pushinteger(L, theme->menu.f_desccolor);
	lua_setfield(L, -2, "f_desccolor");
	lua_pushinteger(L, theme->menu.desccolor);
	lua_setfield(L, -2, "desccolor");
	lua_pushinteger(L, theme->menu.f_shortcutcolor);
	lua_setfield(L, -2, "f_shortcutcolor");
	lua_pushinteger(L, theme->menu.shortcutcolor);
	lua_setfield(L, -2, "shortcutcolor");
	lua_pushinteger(L, theme->menu.bottomdesccolor);
	lua_setfield(L, -2, "bottomdesccolor");
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	lua_pushinteger(L, theme->menu.sepnamecolor);
	lua_setfield(L, -2, "sepnamecolor");
	lua_pushinteger(L, theme->menu.sepdesccolor);
	lua_setfield(L, -2, "sepdesccolor");
#endif
	lua_setfield(L, -2, "menu");

	lua_createtable(L, 0, 4);
	lua_pushinteger(L, theme->form.f_fieldcolor);
	lua_setfield(L, -2, "f_fieldcolor");
	lua_pushinteger(L, theme->form.fieldcolor);
	lua_setfield(L, -2, "fieldcolor");
	lua_pushinteger(L, theme->form.readonlycolor);
	lua_setfield(L, -2, "readonlycolor");
	lua_pushinteger(L, theme->form.bottomdesccolor);
	lua_setfield(L, -2, "bottomdesccolor");
	lua_setfield(L, -2, "form");

	lua_createtable(L, 0, 2);
	lua_pushinteger(L, theme->bar.f_color);
	lua_setfield(L, -2, "f_color");
	lua_pushinteger(L, theme->bar.color);
	lua_setfield(L, -2, "color");
	lua_setfield(L, -2, "bar");

	lua_createtable(L, 0, 10);
	lua_pushinteger(L, theme->button.minmargin);
	lua_setfield(L, -2, "minmargin");
	lua_pushinteger(L, theme->button.maxmargin);
	lua_setfield(L, -2, "maxmargin");
	delim[0] = theme->button.leftdelim;
	lua_pushstring(L, delim);
	lua_setfield(L, -2, "leftdelim");
	delim[0] = theme->button.rightdelim;
	lua_pushstring(L, delim);
	lua_setfield(L, -2, "rightdelim");
	lua_pushinteger(L, theme->button.f_delimcolor);
	lua_setfield(L, -2, "f_delimcolor");
	lua_pushinteger(L, theme->button.delimcolor);
	lua_setfield(L, -2, "delimcolor");
	lua_pushinteger(L, theme->button.f_color);
	lua_setfield(L, -2, "f_color");
	lua_pushinteger(L, theme->button.color);
	lua_setfield(L, -2, "color");
	lua_pushinteger(L, theme->button.f_shortcutcolor);
	lua_setfield(L, -2, "f_shortcutcolor");
	lua_pushinteger(L, theme->button.shortcutcolor);
	lua_setfield(L, -2, "shortcutcolor");
	lua_setfield(L, -2, "button");
}

static int
l_bsddialog_get_theme(lua_State *L)
{
	struct bsddialog_theme theme;

	if (bsddialog_get_theme(&theme) == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_get_theme: %s", bsddialog_geterror());
	}
	pushtheme(L, &theme);
	return (1);
}

static void
checktheme(lua_State *L, int index, struct bsddialog_theme *theme)
{
	const char *s;

	memset(theme, 0, sizeof(*theme));

	luaL_checktype(L, index, LUA_TTABLE);

	luaL_getsubtable(L, index, "screen");
	lua_getfield(L, -1, "color");
	theme->screen.color = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pop(L, 1); /* screen */

	luaL_getsubtable(L, index, "shadow");
	lua_getfield(L, -1, "color");
	theme->shadow.color = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "y");
	theme->shadow.y = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "x");
	theme->shadow.x = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pop(L, 1); /* shadow */

	luaL_getsubtable(L, index, "dialog");
	lua_getfield(L, -1, "color");
	theme->dialog.color = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "delimtitle");
	theme->dialog.delimtitle = lua_toboolean(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "titlecolor");
	theme->dialog.titlecolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "lineraisecolor");
	theme->dialog.lineraisecolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "linelowercolor");
	theme->dialog.linelowercolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "bottomtitlecolor");
	theme->dialog.bottomtitlecolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "arrowcolor");
	theme->dialog.arrowcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pop(L, 1); /* dialog */

	luaL_getsubtable(L, index, "menu");
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	lua_getfield(L, -1, "f_prefixcolor");
	theme->menu.f_prefixcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "prefixcolor");
	theme->menu.prefixcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
#endif
	lua_getfield(L, -1, "f_selectorcolor");
	theme->menu.f_selectorcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "selectorcolor");
	theme->menu.selectorcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "f_namecolor");
	theme->menu.f_namecolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "namecolor");
	theme->menu.namecolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "f_desccolor");
	theme->menu.f_desccolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "desccolor");
	theme->menu.desccolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "f_shortcutcolor");
	theme->menu.f_shortcutcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "shortcutcolor");
	theme->menu.shortcutcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "bottomdesccolor");
	theme->menu.bottomdesccolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	lua_getfield(L, -1, "sepnamecolor");
	theme->menu.sepnamecolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "sepdesccolor");
	theme->menu.sepdesccolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
#endif
	lua_pop(L, 1); /* menu */

	luaL_getsubtable(L, index, "form");
	lua_getfield(L, -1, "f_fieldcolor");
	theme->form.f_fieldcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "fieldcolor");
	theme->form.fieldcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "readonlycolor");
	theme->form.readonlycolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "bottomdesccolor");
	theme->form.bottomdesccolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pop(L, 1); /* form */

	luaL_getsubtable(L, index, "bar");
	lua_getfield(L, -1, "f_color");
	theme->bar.f_color = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "color");
	theme->bar.color = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pop(L, 1); /* bar */

	luaL_getsubtable(L, index, "button");
	lua_getfield(L, -1, "minmargin");
	theme->button.minmargin = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "maxmargin");
	theme->button.maxmargin = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "leftdelim");
	s = lua_tostring(L, -1);
	if (s != NULL)
		theme->button.leftdelim = s[0];
	lua_pop(L, 1);
	lua_getfield(L, -1, "rightdelim");
	s = lua_tostring(L, -1);
	if (s != NULL)
		theme->button.rightdelim = s[0];
	lua_pop(L, 1);
	lua_getfield(L, -1, "f_delimcolor");
	theme->button.f_delimcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "delimcolor");
	theme->button.delimcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "f_color");
	theme->button.f_color = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "color");
	theme->button.color = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "f_shortcutcolor");
	theme->button.f_shortcutcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, -1, "shortcutcolor");
	theme->button.shortcutcolor = lua_tointeger(L, -1);
	lua_pop(L, 1);
	lua_pop(L, 1); /* button */
}

static int
l_bsddialog_set_theme(lua_State *L)
{
	struct bsddialog_theme theme;

	checktheme(L, 1, &theme);
	if (bsddialog_set_theme(&theme) == BSDDIALOG_ERROR) {
		luaL_error(L, "bsddialog_set_theme: %s", bsddialog_geterror());
	}
	return (0);
}

static const struct luaL_Reg l_bsddialog_funcs[] = {
	{"init", l_bsddialog_init},
	{"init_notheme", l_bsddialog_init_notheme},
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	{"inmode", l_bsddialog_inmode},
	{"clear", l_bsddialog_clear},
	{"refresh", l_bsddialog_refresh},
#endif
	{"_end", l_bsddialog_end},
	{"initconf", l_bsddialog_initconf},
	{"backtitle", l_bsddialog_backtitle},
	{"calendar", l_bsddialog_calendar},
	{"checklist", l_bsddialog_checklist},
	{"datebox", l_bsddialog_datebox},
	{"form", l_bsddialog_form},
	{"gauge", l_bsddialog_gauge},
	{"infobox", l_bsddialog_infobox},
	{"menu", l_bsddialog_menu},
	{"mixedgauge", l_bsddialog_mixedgauge},
	{"mixedlist", l_bsddialog_mixedlist},
	{"msgbox", l_bsddialog_msgbox},
	{"pause", l_bsddialog_pause},
	{"radiolist", l_bsddialog_radiolist},
	{"rangebox", l_bsddialog_rangebox},
	{"textbox", l_bsddialog_textbox},
	{"timebox", l_bsddialog_timebox},
	{"yesno", l_bsddialog_yesno},
	{"color", l_bsddialog_color},
	{"color_attrs", l_bsddialog_color_attrs},
	{"hascolors", l_bsddialog_hascolors},
	{"set_default_theme", l_bsddialog_set_default_theme},
	{"get_theme", l_bsddialog_get_theme},
	{"set_theme", l_bsddialog_set_theme},
	{NULL, NULL}
};

static const struct luaL_Reg l_bsddialog_conf_meta[] = {
	{"__index", l_bsddialog_conf_index},
	{"__newindex", l_bsddialog_conf_newindex},
	{"__gc", l_bsddialog_conf_gc},
	{NULL, NULL}
};

int
luaopen_bsddialog(lua_State *L)
{
	luaL_newmetatable(L, BSDDIALOG_CONF_METATABLE);
	luaL_setfuncs(L, l_bsddialog_conf_meta, 0);

	luaL_newlib(L, l_bsddialog_funcs);

	lua_pushstring(L, LIBBSDDIALOG_VERSION);
	lua_setfield(L, -2, "VERSION");

#define SETCONST(ident) do {                     \
	lua_pushinteger(L, BSDDIALOG_ ## ident); \
	lua_setfield(L, -2, #ident);             \
} while (0)

	SETCONST(ERROR);
	SETCONST(OK);
	SETCONST(YES);
	SETCONST(CANCEL);
	SETCONST(NO);
	SETCONST(HELP);
	SETCONST(EXTRA);
	SETCONST(TIMEOUT);
	SETCONST(ESC);
#if LIBBSDDIALOG_VERSION_MAJOR > 0
	SETCONST(LEFT1);
	SETCONST(LEFT2);
	SETCONST(LEFT3);
	SETCONST(RIGHT1);
	SETCONST(RIGHT2);
	SETCONST(RIGHT3);
#else
	SETCONST(GENERIC1);
	SETCONST(GENERIC2);
#endif
	SETCONST(FULLSCREEN);
	SETCONST(AUTOSIZE);
	SETCONST(CENTER);
	SETCONST(MG_SUCCEEDED);
	SETCONST(MG_FAILED);
	SETCONST(MG_PASSED);
	SETCONST(MG_COMPLETED);
	SETCONST(MG_CHECKED);
	SETCONST(MG_DONE);
	SETCONST(MG_SKIPPED);
	SETCONST(MG_INPROGRESS);
	SETCONST(MG_BLANK);
	SETCONST(MG_NA);
	SETCONST(MG_PENDING);
	SETCONST(FIELDHIDDEN);
	SETCONST(FIELDREADONLY);
	SETCONST(FIELDNOCOLOR);
	SETCONST(FIELDCURSOREND);
	SETCONST(FIELDEXTEND);
	SETCONST(FIELDSINGLEBYTE);
	SETCONST(CHECKLIST);
	SETCONST(RADIOLIST);
	SETCONST(SEPARATOR);

#undef SETCONST

	return (1);
}
