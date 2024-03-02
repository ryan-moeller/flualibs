/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, Ryan Moeller <ryan-moeller@att.net>
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

#include <sys/types.h>
#include <sys/event.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luaerror.h"

#define KQUEUE_METATABLE "kqueue"

int luaopen_kqueue(lua_State *);

static int
l_kqueue(lua_State *L)
{
	int kq, *kqp;

	kqp = (int *)lua_newuserdata(L, sizeof (int));
	luaL_getmetatable(L, KQUEUE_METATABLE);
	lua_setmetatable(L, -2);
	*kqp = -1;

	kq = kqueuex(KQUEUE_CLOEXEC);
	if (kq == -1) {
		luaL_error(L, "kqueuex(KQUEUE_CLOEXEC) failed");
	}
	*kqp = kq;

	return (1);
}

static int
l_kevent(lua_State *L)
{
	int kq, *kqp;
	struct kevent *changelist, event;
	int nchanges, ret;

	kqp = (int *)luaL_checkudata(L, 1, KQUEUE_METATABLE);
	luaL_argcheck(L, kqp != NULL, 1, "`kq' expected");
	kq = *kqp;

	if (lua_gettop(L) == 1) {
		changelist = NULL;
		nchanges = 0;
	} else {
		luaL_argcheck(L, lua_type(L, 2) == LUA_TTABLE, 2, "`changelist' expected");
		nchanges = lua_rawlen(L, 2);
		changelist = (struct kevent *)malloc(nchanges * sizeof (struct kevent));
		for (size_t i = 1; i <= nchanges; ++i) {
			uintptr_t ident;
			short filter;
			u_short flags;
			u_int fflags;
			int64_t data;
			void *udata;
			int type;

			type = lua_rawgeti(L, 2, i);
			luaL_argcheck(L, type == LUA_TTABLE, 2, "`changelist' invalid");

			lua_pushstring(L, "ident");
			type = lua_rawget(L, -2);
			luaL_argcheck(L, type == LUA_TNUMBER, 2, "`changelist' invalid ident");
			ident = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "filter");
			type = lua_rawget(L, -2);
			luaL_argcheck(L, type == LUA_TNUMBER, 2, "`changelist' invalid filter");
			filter = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "flags");
			type = lua_rawget(L, -2);
			luaL_argcheck(L, type == LUA_TNUMBER, 2, "`changelist' invalid flags");
			flags = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "fflags");
			type = lua_rawget(L, -2);
			luaL_argcheck(L, type == LUA_TNUMBER, 2, "`changelist' invalid fflags");
			fflags = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "data");
			type = lua_rawget(L, -2);
			luaL_argcheck(L, type == LUA_TNUMBER, 2, "`changelist' invalid data");
			data = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "udata");
			type = lua_rawget(L, -2);
			luaL_argcheck(L, type == LUA_TTHREAD, 2, "`changelist' invalid udata");
			udata = lua_tothread(L, -1);
			lua_pop(L, 1);

			EV_SET(&changelist[i-1], ident, filter, flags, fflags, data, udata);
			lua_pop(L, 1);
		}
	}

	ret = kevent(kq, changelist, nchanges, &event, 1, NULL);
	if (ret == -1) {
		luaL_error(L, "kevent failed: %s", strerror(errno));
	} else if (ret != 1) {
		luaL_error(L, "kevent failed spectacularly");
	}

	lua_newtable(L);

	lua_pushstring(L, "ident");
	lua_pushinteger(L, event.ident);
	lua_rawset(L, -3);

	lua_pushstring(L, "filter");
	lua_pushinteger(L, event.filter);
	lua_rawset(L, -3);

	lua_pushstring(L, "flags");
	lua_pushinteger(L, event.flags);
	lua_rawset(L, -3);

	lua_pushstring(L, "fflags");
	lua_pushinteger(L, event.fflags);
	lua_rawset(L, -3);

	lua_pushstring(L, "data");
	lua_pushinteger(L, event.data);
	lua_rawset(L, -3);

	lua_pushstring(L, "udata");
	lua_pushthread(event.udata);
	lua_xmove(event.udata, L, 1);
	lua_rawset(L, -3);

	return (1);
}

static int
l_close(struct lua_State *L)
{
	int kq, *kqp;

	kqp = (int *)luaL_checkudata(L, 1, KQUEUE_METATABLE);
	luaL_argcheck(L, kqp != NULL, 1, "`kq' expected");
	kq = *kqp;
	luaL_argcheck(L, kq != -1, 1, "`kq' already closed");
	close(kq);
	*kqp = -1;
	return (0);
}

static int
l_gc(struct lua_State *L)
{
	int kq, *kqp;

	kqp = (int *)luaL_checkudata(L, 1, KQUEUE_METATABLE);
	if (kqp == NULL) {
		return (0);
	}
	kq = *kqp;
	if (kq == -1) {
		return (0);
	}
	close(kq);
	*kqp = -1;
	return (0);
}


static const struct luaL_Reg l_kqueue_funcs[] = {
	{"kqueue", l_kqueue},
	{NULL, NULL}
};

static const struct  luaL_Reg l_kqueue_meta[] = {
	{"kevent", l_kevent},
	{"close", l_close},
	{NULL, NULL}
};

int
luaopen_kqueue(lua_State *L)
{
	lua_newtable(L);

	luaL_newmetatable(L, KQUEUE_METATABLE);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, l_gc);
	lua_settable(L, -3);

	luaL_setfuncs(L, l_kqueue_meta, 0);

	luaL_setfuncs(L, l_kqueue_funcs, 0);

	lua_pushstring(L, "EV_ADD");
	lua_pushinteger(L, EV_ADD);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_ENABLE");
	lua_pushinteger(L, EV_ENABLE);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_DISABLE");
	lua_pushinteger(L, EV_DISABLE);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_DISPATCH");
	lua_pushinteger(L, EV_DISPATCH);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_DELETE");
	lua_pushinteger(L, EV_DELETE);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_RECEIPT");
	lua_pushinteger(L, EV_RECEIPT);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_ONESHOT");
	lua_pushinteger(L, EV_ONESHOT);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_CLEAR");
	lua_pushinteger(L, EV_CLEAR);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_EOF");
	lua_pushinteger(L, EV_EOF);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_ERROR");
	lua_pushinteger(L, EV_ERROR);
	lua_settable(L, -3);

	lua_pushstring(L, "EV_KEEPUDATA");
	lua_pushinteger(L, EV_KEEPUDATA);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_READ");
	lua_pushinteger(L, EVFILT_READ);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_WRITE");
	lua_pushinteger(L, EVFILT_WRITE);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_EMPTY");
	lua_pushinteger(L, EVFILT_EMPTY);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_AIO");
	lua_pushinteger(L, EVFILT_AIO);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_VNODE");
	lua_pushinteger(L, EVFILT_VNODE);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_PROC");
	lua_pushinteger(L, EVFILT_PROC);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_PROCDESC");
	lua_pushinteger(L, EVFILT_PROCDESC);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_SIGNAL");
	lua_pushinteger(L, EVFILT_SIGNAL);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_TIMER");
	lua_pushinteger(L, EVFILT_TIMER);
	lua_settable(L, -3);

	lua_pushstring(L, "EVFILT_USER");
	lua_pushinteger(L, EVFILT_USER);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FFNOP");
	lua_pushinteger(L, NOTE_FFNOP);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FFAND");
	lua_pushinteger(L, NOTE_FFAND);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FFOR");
	lua_pushinteger(L, NOTE_FFOR);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FFCOPY");
	lua_pushinteger(L, NOTE_FFCOPY);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FFCTRLMASK");
	lua_pushinteger(L, NOTE_FFCTRLMASK);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FFLAGSMASK");
	lua_pushinteger(L, NOTE_FFLAGSMASK);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_TRIGGER");
	lua_pushinteger(L, NOTE_TRIGGER);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_LOWAT");
	lua_pushinteger(L, NOTE_LOWAT);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FILE_POLL");
	lua_pushinteger(L, NOTE_FILE_POLL);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_DELETE");
	lua_pushinteger(L, NOTE_DELETE);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_WRITE");
	lua_pushinteger(L, NOTE_WRITE);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_EXTEND");
	lua_pushinteger(L, NOTE_EXTEND);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_ATTRIB");
	lua_pushinteger(L, NOTE_ATTRIB);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_LINK");
	lua_pushinteger(L, NOTE_LINK);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_RENAME");
	lua_pushinteger(L, NOTE_RENAME);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_REVOKE");
	lua_pushinteger(L, NOTE_REVOKE);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_OPEN");
	lua_pushinteger(L, NOTE_OPEN);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_CLOSE");
	lua_pushinteger(L, NOTE_CLOSE);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_CLOSE_WRITE");
	lua_pushinteger(L, NOTE_CLOSE_WRITE);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_READ");
	lua_pushinteger(L, NOTE_READ);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_EXIT");
	lua_pushinteger(L, NOTE_EXIT);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_FORK");
	lua_pushinteger(L, NOTE_FORK);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_EXEC");
	lua_pushinteger(L, NOTE_EXEC);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_PCTRLMASK");
	lua_pushinteger(L, NOTE_PCTRLMASK);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_PDATAMASK");
	lua_pushinteger(L, NOTE_PDATAMASK);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_TRACK");
	lua_pushinteger(L, NOTE_TRACK);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_TRACKERR");
	lua_pushinteger(L, NOTE_TRACKERR);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_CHILD");
	lua_pushinteger(L, NOTE_CHILD);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_SECONDS");
	lua_pushinteger(L, NOTE_SECONDS);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_MSECONDS");
	lua_pushinteger(L, NOTE_MSECONDS);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_USECONDS");
	lua_pushinteger(L, NOTE_USECONDS);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_NSECONDS");
	lua_pushinteger(L, NOTE_NSECONDS);
	lua_settable(L, -3);

	lua_pushstring(L, "NOTE_ABSTIME");
	lua_pushinteger(L, NOTE_ABSTIME);
	lua_settable(L, -3);

	return (1);
}
