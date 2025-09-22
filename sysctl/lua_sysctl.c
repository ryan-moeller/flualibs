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
#include <sys/sysctl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../luaerror.h"

#define MIB_METATABLE "struct mib *"

struct mib {
	int oid[CTL_MAXNAME];
	size_t oidlen;
	size_t prefix;
	u_int kind;
	char *format;
	char *name;
	char *description;
};

int luaopen_sysctl(lua_State *);

static int
l_sysctl(lua_State *L)
{
	struct mib *mib;
	const char *name;
	int error;

	lua_remove(L, 1); /* func table from the __call metamethod */
	name = luaL_optstring(L, 1, NULL);
	mib = lua_newuserdatauv(L, sizeof(*mib), 0);
	luaL_setmetatable(L, MIB_METATABLE);
	memset(mib, 0, sizeof(*mib));
	if (name == NULL) {
		mib->oid[0] = CTL_KERN;
		mib->oidlen = 1;
		mib->prefix = 0;
	} else {
		mib->oidlen = nitems(mib->oid);
		error = sysctlnametomib(name, mib->oid, &mib->oidlen);
		if (error != 0) {
			luaL_error(L, "sysctlnametomib: %s", strerror(errno));
		}
		mib->prefix = mib->oidlen * sizeof(int);
		mib->name = strdup(name);
		if (mib->name == NULL) {
			luaL_error(L, "strdup: %s", strerror(errno));
		}
	}
	return (1);
}

static int
l_mib_gc(lua_State *L)
{
	struct mib *mib;

	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	free(mib->name);
	free(mib->description);
	free(mib->format);
	memset(mib, 0, sizeof(*mib));
	return (0);
}

static int
l_mib_oid(lua_State *L)
{
	struct mib *mib;

	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	lua_createtable(L, mib->oidlen, 0);
	for (u_int i = 0; i < mib->oidlen; i++) {
		lua_pushinteger(L, mib->oid[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_mib_format(lua_State *L)
{
	struct mib *mib;

	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	if (mib->format == NULL) {
		char buf[BUFSIZ];
		int qoid[CTL_MAXNAME+2];
		size_t length;
		int error;

		qoid[0] = CTL_SYSCTL;
		qoid[1] = CTL_SYSCTL_OIDFMT;
		memcpy(qoid + 2, mib->oid, mib->oidlen * sizeof(int));
		length = sizeof(buf);
		memset(buf, 0, length);
		error = sysctl(qoid, mib->oidlen + 2, buf, &length, NULL, 0);
		if (error != 0) {
			luaL_error(L, "sysctl: %s", strerror(errno));
		}
		mib->kind = *(u_int *)buf;
		mib->format = strdup(buf + sizeof(u_int));
		if (mib->format == NULL) {
			luaL_error(L, "strdup: %s", strerror(errno));
		}
	}
	lua_pushinteger(L, mib->kind);
	lua_pushstring(L, mib->format);
	return (2);
}

static int
l_mib_name(lua_State *L)
{
	struct mib *mib;

	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	if (mib->name == NULL) {
		char buf[BUFSIZ];
		int qoid[CTL_MAXNAME+2];
		size_t length;
		int error;

		qoid[0] = CTL_SYSCTL;
		qoid[1] = CTL_SYSCTL_NAME;
		memcpy(qoid + 2, mib->oid, mib->oidlen * sizeof(int));
		length = sizeof(buf);
		memset(buf, 0, length);
		error = sysctl(qoid, mib->oidlen + 2, buf, &length, NULL, 0);
		if (error != 0) {
			luaL_error(L, "sysctl: %s", strerror(errno));
		}
		mib->name = strdup(buf);
		if (mib->name == NULL) {
			luaL_error(L, "strdup: %s", strerror(errno));
		}
	}
	lua_pushstring(L, mib->name);
	return (1);
}

static int
l_mib_description(lua_State *L)
{
	struct mib *mib;

	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	if (mib->description == NULL) {
		char buf[BUFSIZ];
		int qoid[CTL_MAXNAME+2];
		size_t length;
		int error;

		qoid[0] = CTL_SYSCTL;
		qoid[1] = CTL_SYSCTL_OIDDESCR;
		memcpy(qoid + 2, mib->oid, mib->oidlen * sizeof(int));
		length = sizeof(buf);
		memset(buf, 0, length);
		error = sysctl(qoid, mib->oidlen + 2, buf, &length, NULL, 0);
		if (error != 0) {
			if (errno == ENOENT) {
				lua_pushnil(L);
				return (1);
			}
			luaL_error(L, "sysctl: %s", strerror(errno));
		}
		mib->description = strdup(buf);
		if (mib->description == NULL) {
			luaL_error(L, "strdup: %s", strerror(errno));
		}
	}
	lua_pushstring(L, mib->description);
	return (1);
}

static int
l_mib_value(lua_State *L)
{
	struct mib *mib;
	void *p, *newp;
	size_t size;
	int argc, ctltype, error;

	argc = lua_gettop(L);
	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	if (mib->format == NULL) {
		(void)l_mib_format(L);
	}
	ctltype = mib->kind & CTLTYPE;
	if (argc == 1) {
		if (ctltype == CTLTYPE_NODE) {
			lua_pushnil(L);
			return (1);
		}
		size = 0;
		error = sysctl(mib->oid, mib->oidlen, NULL, &size, NULL, 0);
		if (error != 0) {
			luaL_error(L, "sysctl: %s", strerror(errno));
		}
		size = round_page(size * 2); /* in case it grows */
		p = malloc(size);
		if (p == NULL) {
			luaL_error(L, "malloc: %s", strerror(errno));
		}
		for (;;) {
			error =
			    sysctl(mib->oid, mib->oidlen, p, &size, NULL, 0);
			if (error == 0) {
				break;
			}
			if (error != ENOMEM) {
				error = errno;
				free(p);
				luaL_error(L, "sysctl: %s", strerror(error));
			}
			size = round_page(size * 2); /* in case it grows more */
			newp = realloc(p, size);
			if (newp == NULL) {
				error = errno;
				free(p);
				luaL_error(L, "realloc: %s", strerror(error));
			}
			p = newp;
		}
		switch (ctltype) {
		case CTLTYPE_INT:
			lua_pushinteger(L, *(int *)p);
			break;
		case CTLTYPE_UINT:
			lua_pushinteger(L, *(u_int *)p);
			break;
		case CTLTYPE_LONG:
			lua_pushinteger(L, *(long *)p);
			break;
		case CTLTYPE_ULONG:
			lua_pushinteger(L, *(u_long *)p);
			break;
		case CTLTYPE_S8:
			lua_pushinteger(L, *(int8_t *)p);
			break;
		case CTLTYPE_U8:
			lua_pushinteger(L, *(uint8_t *)p);
			break;
		case CTLTYPE_S16:
			lua_pushinteger(L, *(int16_t *)p);
			break;
		case CTLTYPE_U16:
			lua_pushinteger(L, *(uint16_t *)p);
			break;
		case CTLTYPE_S32:
			lua_pushinteger(L, *(int32_t *)p);
			break;
		case CTLTYPE_U32:
			lua_pushinteger(L, *(uint32_t *)p);
			break;
		case CTLTYPE_S64:
			lua_pushinteger(L, *(int64_t *)p);
			break;
		case CTLTYPE_U64:
			lua_pushinteger(L, *(uint64_t *)p);
			break;
		case CTLTYPE_STRING:
			lua_pushlstring(L, p, size - 1);
			break;
		case CTLTYPE_OPAQUE:
			lua_pushlstring(L, p, size);
			break;
		default:
			luaL_error(L, "unknown ctltype: %d", ctltype);
		}
		free(p);
		return (1);
	}
	if (ctltype == CTLTYPE_NODE) {
		return (0);
	}
	switch (ctltype) {
	case CTLTYPE_INT: {
		int value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_UINT: {
		u_int value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_LONG: {
		long value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_ULONG: {
		u_long value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_S8: {
		int8_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_U8: {
		uint8_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_S16: {
		int16_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_U16: {
		uint16_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_S32: {
		int32_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_U32: {
		uint32_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_S64: {
		int64_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_U64: {
		uint64_t value = luaL_checkinteger(L, 2);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, &value,
		    sizeof(value));
		break;
	}
	case CTLTYPE_STRING: {
		size_t length;
		const char *s = luaL_checklstring(L, 2, &length);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, s,
		    length + 1);
		break;
	}
	case CTLTYPE_OPAQUE: {
		size_t length;
		const char *b = luaL_checklstring(L, 2, &length);
		error = sysctl(mib->oid, mib->oidlen, NULL, NULL, b, length);
		break;
	}
	default:
		luaL_error(L, "unknown ctltype: %d", ctltype);
	}
	if (error != 0) {
		luaL_error(L, "sysctl: %s", strerror(errno));
	}
	return (0);
}

static int
l_mib_iter_next(lua_State *L)
{
	int qoid[CTL_MAXNAME+2];
	struct mib *prev, *next;
	size_t size;
	int error;

	prev = luaL_testudata(L, 2, MIB_METATABLE);
	if (prev == NULL) {
		lua_pushvalue(L, 1); /* the initial mib */
		return (1);
	}
	next = lua_newuserdatauv(L, sizeof(*next), 0);
	luaL_setmetatable(L, MIB_METATABLE);
	memset(next, 0, sizeof(*next));
	next->prefix = prev->prefix;
	qoid[0] = CTL_SYSCTL;
	qoid[1] = luaL_checkinteger(L, lua_upvalueindex(1));
	memcpy(qoid + 2, prev->oid, prev->oidlen * sizeof(int));
	size = sizeof(next->oid);
	error = sysctl(qoid, prev->oidlen + 2, next->oid, &size, NULL, 0);
	if (error != 0) {
		if (errno == ENOENT) {
			lua_pushnil(L);
			return (1);
		}
		luaL_error(L, "sysctl: %s", strerror(errno));
	}
	if (memcmp(prev->oid, next->oid, prev->prefix) != 0) {
		lua_pushnil(L);
		return (1);
	}
	next->oidlen = size / sizeof(int);
	return (1);
}

static int
l_mib_iter(lua_State *L)
{
	struct mib *mib;

	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	lua_pushinteger(L, CTL_SYSCTL_NEXT);
	lua_pushcclosure(L, l_mib_iter_next, 1);
	if (mib->prefix == 0) {
		lua_pushvalue(L, 1);
		lua_pushnil(L);
	} else {
		lua_pushnil(L);
		lua_pushvalue(L, 1);
	}
	return (3);
}

static int
l_mib_iter_noskip(lua_State *L)
{
	struct mib *mib;

	mib = luaL_checkudata(L, 1, MIB_METATABLE);
	lua_pushinteger(L, CTL_SYSCTL_NEXTNOSKIP);
	lua_pushcclosure(L, l_mib_iter_next, 1);
	if (mib->prefix == 0) {
		lua_pushvalue(L, 1);
		lua_pushnil(L);
	} else {
		lua_pushnil(L);
		lua_pushvalue(L, 1);
	}
	return (3);
}

static const struct luaL_Reg l_mib_meta[] = {
	{"__gc", l_mib_gc},
	{"oid", l_mib_oid},
	{"format", l_mib_format},
	{"name", l_mib_name},
	{"description", l_mib_description},
	{"value", l_mib_value},
	{"iter", l_mib_iter},
	{"iter_noskip", l_mib_iter_noskip},
	{NULL, NULL}
};

int
luaopen_sysctl(lua_State *L)
{
	luaL_newmetatable(L, MIB_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_mib_meta, 0);

	lua_newtable(L);
	/* Set up the library in a metatable so we can call it. */
	lua_newtable(L);
	lua_pushcfunction(L, l_sysctl);
	lua_setfield(L, -2, "__call");
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(CTL_MAXNAME);
	SETCONST(CTLTYPE);
	SETCONST(CTLTYPE_NODE);
	SETCONST(CTLTYPE_INT);
	SETCONST(CTLTYPE_STRING);
	SETCONST(CTLTYPE_S64);
	SETCONST(CTLTYPE_OPAQUE);
	SETCONST(CTLTYPE_STRUCT);
	SETCONST(CTLTYPE_UINT);
	SETCONST(CTLTYPE_LONG);
	SETCONST(CTLTYPE_ULONG);
	SETCONST(CTLTYPE_U64);
	SETCONST(CTLTYPE_U8);
	SETCONST(CTLTYPE_U16);
	SETCONST(CTLTYPE_S8);
	SETCONST(CTLTYPE_S16);
	SETCONST(CTLTYPE_S32);
	SETCONST(CTLTYPE_U32);
	SETCONST(CTLFLAG_RD);
	SETCONST(CTLFLAG_WR);
	SETCONST(CTLFLAG_RW);
	SETCONST(CTLFLAG_DORMANT);
	SETCONST(CTLFLAG_ANYBODY);
	SETCONST(CTLFLAG_SECURE);
	SETCONST(CTLFLAG_PRISON);
	SETCONST(CTLFLAG_DYN);
	SETCONST(CTLFLAG_SKIP);
	SETCONST(CTLMASK_SECURE);
	SETCONST(CTLFLAG_TUN);
	SETCONST(CTLFLAG_RDTUN);
	SETCONST(CTLFLAG_RWTUN);
	SETCONST(CTLFLAG_MPSAFE);
	SETCONST(CTLFLAG_VNET);
	SETCONST(CTLFLAG_DYING);
	SETCONST(CTLFLAG_CAPRD);
	SETCONST(CTLFLAG_CAPWR);
	SETCONST(CTLFLAG_STATS);
	SETCONST(CTLFLAG_NOFETCH);
	SETCONST(CTLFLAG_CAPRW);
	SETCONST(CTLFLAG_NEEDGIANT);
	SETCONST(CTLSHIFT_SECURE);
	SETCONST(CTLFLAG_SECURE1);
	SETCONST(CTLFLAG_SECURE2);
	SETCONST(CTLFLAG_SECURE3);
	/* XXX: not defining identifiers here, we don't use them */
#undef SETCONST
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	return (lua_setmetatable(L, -2));
}
