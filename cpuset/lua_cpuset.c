/*
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
#include <sys/cpuset.h>
#include <strings.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lua_cpuset.h"

int luaopen_cpuset(lua_State *);

/* TODO: Administrative control over numbered sets a la cpuset(1). */

static int
l_cpuset_copied(lua_State *L)
{
	cpuset_t *from, *to;

	from = luaL_checkudata(L, 1, CPUSET_METATABLE);

	to = newanoncpuset(L);
	CPU_COPY(from, to);
	return (1);
}

static int
l_cpuset_filled(lua_State *L)
{
	cpuset_t *set;

	set = newanoncpuset(L);
	CPU_FILL(set);
	return (1);
}

static int
l_cpuset_only(lua_State *L)
{
	cpuset_t *set;
	size_t cpu_idx;

	cpu_idx = luaL_checkinteger(L, 1);

	set = newanoncpuset(L);
	CPU_SETOF(cpu_idx, set);
	return (1);
}

static int
l_cpuset_ored(lua_State *L)
{
	cpuset_t *src1, *src2, *dst;

	src1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	src2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	dst = newanoncpuset(L);
	CPU_OR(dst, src1, src2);
	return (1);
}

#ifdef CPU_ORNOT
static int
l_cpuset_ornoted(lua_State *L)
{
	cpuset_t *src1, *src2, *dst;

	src1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	src2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	dst = newanoncpuset(L);
	CPU_ORNOT(dst, src1, src2);
	return (1);
}
#endif

static int
l_cpuset_anded(lua_State *L)
{
	cpuset_t *src1, *src2, *dst;

	src1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	src2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	dst = newanoncpuset(L);
	CPU_AND(dst, src1, src2);
	return (1);
}

static int
l_cpuset_andnoted(lua_State *L)
{
	cpuset_t *src1, *src2, *dst;

	src1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	src2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	dst = newanoncpuset(L);
	CPU_ANDNOT(dst, src1, src2);
	return (1);
}

static int
l_cpuset_xored(lua_State *L)
{
	cpuset_t *src1, *src2, *dst;

	src1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	src2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	dst = newanoncpuset(L);
	CPU_XOR(dst, src1, src2);
	return (1);
}

static int
l_cpuset_zeroed(lua_State *L)
{
	cpuset_t *set;

	set = newanoncpuset(L);
	CPU_ZERO(set);
	return (1);
}

static int
l_cpuset_clr(lua_State *L)
{
	cpuset_t *set;
	size_t cpu_idx;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	cpu_idx = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_CLR(cpu_idx, set);
	lua_pop(L, 1);
	return (1);
}

static int
l_cpuset_copy(lua_State *L)
{
	cpuset_t *to, *from;

	to = luaL_checkudata(L, 1, CPUSET_METATABLE);
	from = luaL_checkudata(L, 2, CPUSET_METATABLE);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_COPY(from, to);
	lua_pop(L, 1);
	return (1);
}

static int
l_cpuset_isset(lua_State *L)
{
	cpuset_t *set;
	size_t cpu_idx;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	cpu_idx = luaL_checkinteger(L, 2);

	lua_pushboolean(L, CPU_ISSET(cpu_idx, set));
	return (1);
}

static int
l_cpuset_set(lua_State *L)
{
	cpuset_t *set;
	size_t cpu_idx;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	cpu_idx = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_SET(cpu_idx, set);
	lua_pop(L, 1);
	return (1);
}

static int
l_cpuset_zero(lua_State *L)
{
	cpuset_t *set;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	luaL_checktype(L, 2, LUA_TNONE);

	CPU_ZERO(set);
	return (1);
}

static int
l_cpuset_fill(lua_State *L)
{
	cpuset_t *set;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	luaL_checktype(L, 2, LUA_TNONE);

	CPU_FILL(set);
	return (1);
}

static int
l_cpuset_setof(lua_State *L)
{
	cpuset_t *set;
	size_t cpu_idx;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	cpu_idx = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_SETOF(cpu_idx, set);
	lua_pop(L, 1);
	return (1);
}

static int
l_cpuset_empty(lua_State *L)
{
	cpuset_t *set;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);

	lua_pushboolean(L, CPU_EMPTY(set));
	return (1);
}

static int
l_cpuset_isfullset(lua_State *L)
{
	cpuset_t *set;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);

	lua_pushboolean(L, CPU_ISFULLSET(set));
	return (1);
}

static int
l_cpuset_ffs(lua_State *L)
{
	cpuset_t *set;
	long cpu_idx;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);

	/* Hide the weirdness of FFS. */
	if ((cpu_idx = CPU_FFS(set) - 1) >= 0) {
		lua_pushinteger(L, cpu_idx);
	} else {
		lua_pushnil(L);
	}
	return (1);
}

static int
l_cpuset_count(lua_State *L)
{
	cpuset_t *set;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);

	lua_pushinteger(L, CPU_COUNT(set));
	return (1);
}

static int
l_cpuset_subset(lua_State *L)
{
	cpuset_t *haystack, *needle;

	haystack = luaL_checkudata(L, 1, CPUSET_METATABLE);
	needle = luaL_checkudata(L, 2, CPUSET_METATABLE);

	lua_pushboolean(L, CPU_SUBSET(haystack, needle));
	return (1);
}

static int
l_cpuset_overlap(lua_State *L)
{
	cpuset_t *set1, *set2;

	set1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	set2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	lua_pushboolean(L, CPU_OVERLAP(set1, set2));
	return (1);
}

static int
l_cpuset_cmp(lua_State *L)
{
	cpuset_t *set1, *set2;

	set1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	set2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	lua_pushboolean(L, CPU_CMP(set1, set2));
	return (1);
}

static int
l_cpuset_equal(lua_State *L)
{
	cpuset_t *set1, *set2;

	set1 = luaL_checkudata(L, 1, CPUSET_METATABLE);
	set2 = luaL_checkudata(L, 2, CPUSET_METATABLE);

	lua_pushboolean(L, !CPU_CMP(set1, set2));
	return (1);
}

static int
l_cpuset_or(lua_State *L)
{
	cpuset_t *set, *other;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	other = luaL_checkudata(L, 2, CPUSET_METATABLE);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_OR(set, set, other);
	lua_pop(L, 1);
	return (1);
}

#ifdef CPU_ORNOT
static int
l_cpuset_ornot(lua_State *L)
{
	cpuset_t *set, *other;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	other = luaL_checkudata(L, 2, CPUSET_METATABLE);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_ORNOT(set, set, other);
	lua_pop(L, 1);
	return (1);
}
#endif

static int
l_cpuset_and(lua_State *L)
{
	cpuset_t *set, *other;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	other = luaL_checkudata(L, 2, CPUSET_METATABLE);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_AND(set, set, other);
	lua_pop(L, 1);
	return (1);
}

static int
l_cpuset_andnot(lua_State *L)
{
	cpuset_t *set, *other;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	other = luaL_checkudata(L, 2, CPUSET_METATABLE);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_ANDNOT(set, set, other);
	lua_pop(L, 1);
	return (1);
}

static int
l_cpuset_xor(lua_State *L)
{
	cpuset_t *set, *other;

	set = luaL_checkudata(L, 1, CPUSET_METATABLE);
	other = luaL_checkudata(L, 2, CPUSET_METATABLE);
	luaL_checktype(L, 3, LUA_TNONE);

	CPU_XOR(set, set, other);
	lua_pop(L, 1);
	return (1);
}

static const struct luaL_Reg l_cpuset_funcs[] = {
	{"copy", l_cpuset_copied},
	{"fill", l_cpuset_filled},
	{"only", l_cpuset_only},
	{"or", l_cpuset_ored},
#ifdef CPU_ORNOT
	{"ornot", l_cpuset_ornoted},
#endif
	{"and", l_cpuset_anded},
	{"andnot", l_cpuset_andnoted},
	{"xor", l_cpuset_xored},
	{"zero", l_cpuset_zeroed},
	{NULL, NULL}
};

static const struct luaL_Reg l_cpuset_meta[] = {
	{"__band", l_cpuset_anded},
	{"__bor", l_cpuset_ored},
	{"__bxor", l_cpuset_xored},
	{"__len", l_cpuset_count},
	{"__eq", l_cpuset_equal},
	{"clr", l_cpuset_clr},
	{"copy", l_cpuset_copy},
	{"isset", l_cpuset_isset},
	{"set", l_cpuset_set},
	{"zero", l_cpuset_zero},
	{"fill", l_cpuset_fill},
	{"setof", l_cpuset_setof},
	{"empty", l_cpuset_empty},
	{"isfullset", l_cpuset_isfullset},
	{"ffs", l_cpuset_ffs},
	{"count", l_cpuset_count},
	{"subset", l_cpuset_subset},
	{"overlap", l_cpuset_overlap},
	{"cmp", l_cpuset_cmp},
	{"equal", l_cpuset_equal},
	{"or", l_cpuset_or},
#ifdef CPU_ORNOT
	{"ornot", l_cpuset_ornot},
#endif
	{"and", l_cpuset_and},
	{"andnot", l_cpuset_andnot},
	{"xor", l_cpuset_xor},
	{NULL, NULL}
};

int
luaopen_cpuset(lua_State *L)
{
	luaL_newmetatable(L, CPUSET_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_cpuset_meta, 0);

	luaL_newlib(L, l_cpuset_funcs);
	lua_pushinteger(L, CPU_SETSIZE);
	lua_setfield(L, -2, "CPU_SETSIZE");
	return (1);
}
