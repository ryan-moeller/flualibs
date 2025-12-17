/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

#include "luaerror.h"
#include "utils.h"

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

int luaopen_sys_sysctl(lua_State *);

static int
l_sysctl(lua_State *L)
{
	struct mib *mib;
	const char *name;
	int error;

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
			return (fail(L, errno));
		}
		mib->prefix = mib->oidlen * sizeof(int);
		mib->name = strdup(name);
		if (mib->name == NULL) {
			return (fatal(L, "strdup", errno));
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
			return (fail(L, errno));
		}
		mib->kind = *(u_int *)buf;
		mib->format = strdup(buf + sizeof(u_int));
		if (mib->format == NULL) {
			return (fatal(L, "strdup", errno));
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
			return (fail(L, errno));
		}
		mib->name = strdup(buf);
		if (mib->name == NULL) {
			return (fatal(L, "strdup", errno));
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
			return (fail(L, errno));
		}
		mib->description = strdup(buf);
		if (mib->description == NULL) {
			return (fatal(L, "strdup", errno));
		}
	}
	lua_pushstring(L, mib->description);
	return (1);
}

static int
l_mib_value(lua_State *L)
{
	struct mib *mib;
	void *p;
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
		p = NULL;
		size = 0;
		for (;;) {
			if (sysctl(mib->oid, mib->oidlen, p, &size, NULL, 0)
			    == -1) {
				if ((error = errno) != ENOMEM) {
					free(p);
					return (fail(L, error));
				}
			} else if (p != NULL) {
				break;
			}
			free(p);
			size *= 2; /* in case it grows */
			if ((p = malloc(size)) == NULL) {
				return (fatal(L, "malloc", ENOMEM));
			}
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
			return (luaL_error(L, "unknown ctltype: %d", ctltype));
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
		return (fail(L, errno));
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
		return (fail(L, errno));
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

static const struct luaL_Reg l_sysctl_funcs[] = {
	{"sysctl", l_sysctl},
	{NULL, NULL}
};

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
luaopen_sys_sysctl(lua_State *L)
{
	luaL_newmetatable(L, MIB_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_mib_meta, 0);

	luaL_newlib(L, l_sysctl_funcs);
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
	return (1);
}
