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
#include <sys/cpuset.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <pthread_np.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "refcount.h"
#include "../cpuset/lua_cpuset.h"
#include "../luaerror.h"

int luaopen_pthread(lua_State *);

#define PTHREAD_METATABLE "pthread_t"
#define PTHREAD_ONCE_METATABLE "pthread_once_t"
#define PTHREAD_MUTEX_METATABLE "pthread_mutex_t"
#define PTHREAD_COND_METATABLE "pthread_cond_t"
#define PTHREAD_RWLOCK_METATABLE "pthread_rwlock_t"
#define PTHREAD_KEY_METATABLE "pthread_key_t"
#define PTHREAD_BARRIER_METATABLE "pthread_barrier_t"

enum wrapperuv {
	COOKIE = 1,
};

static inline int
new(lua_State *L, const char *metatable)
{
	assert(lua_islightuserdata(L, -1));

	lua_newuserdatauv(L, 0, 1);
	luaL_getmetatable(L, metatable);
	lua_setmetatable(L, -2);

	lua_pushvalue(L, -2);
	lua_setiuservalue(L, -2, COOKIE);

	return (1);
}

static inline int
fail(lua_State *L, int error)
{
	luaL_pushfail(L);
	lua_pushstring(L, strerror(error));
	lua_pushinteger(L, error);
	return (3);
}

static inline void
tpush(lua_State *L, int idx)
{
	int len;

	len = luaL_len(L, idx);
	lua_rawseti(L, idx, len + 1);
}

static inline void
tpop(lua_State *L, int idx)
{
	int len;

	idx = lua_absindex(L, idx);
	len = luaL_len(L, idx);
	lua_rawgeti(L, idx, len);
	lua_pushnil(L);
	lua_rawseti(L, idx, len);
}

static inline void
tpack(lua_State *L, int n)
{
	int idx;

	lua_createtable(L, n, 0);
	if (n > 0) {
		idx = lua_gettop(L) - n;
		lua_insert(L, idx);
		for (int i = 1; i <= n; i++) {
			lua_pushvalue(L, idx + i);
			lua_rawseti(L, idx, i);
		}
		lua_settop(L, idx);
	}
}

static inline int
tunpack(lua_State *L, int idx)
{
	int len;

	idx = lua_absindex(L, idx);
	len = luaL_len(L, idx);
	for (int i = 1; i <= len; i++) {
		lua_rawgeti(L, idx, i);
	}
	lua_remove(L, idx);
	return (len);
}

static int
buffer_writer(lua_State *L __unused, const void *p, size_t sz, void *ud)
{
	luaL_addlstring(ud, p, sz);
	return (0);
}

static int
copyn(lua_State *L, lua_State *l, int idx, int n)
{
	int Ltop __unused = lua_gettop(L);
	int ltop __unused = lua_gettop(l);

	for (int end = idx + n; idx < end; idx++) {
		switch (lua_type(L, idx)) {
		case LUA_TNIL:
			lua_pushnil(l);
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean(l, lua_toboolean(L, idx));
			break;
		case LUA_TLIGHTUSERDATA:
			lua_pushlightuserdata(l, lua_touserdata(L, idx));
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, idx)) {
				lua_pushinteger(l, lua_tointeger(L, idx));
			} else {
				lua_pushnumber(l, lua_tonumber(L, idx));
			}
			break;
		case LUA_TSTRING: {
			const char *s;
			size_t len;

			s = lua_tolstring(L, idx, &len);
			lua_pushlstring(l, s, len);
			break;
		}
		case LUA_TFUNCTION: {
			luaL_Buffer b;
			const char *p, *name;
			size_t len;
			int error, lidx;

			luaL_buffinit(L, &b);
			lua_pushvalue(L, idx);
			if (lua_dump(L, buffer_writer, &b, 0) != 0) {
				return (idx);
			}
			luaL_pushresult(&b);

			p = lua_tolstring(L, -1, &len);
			if (luaL_loadbuffer(l, p, len, NULL) != LUA_OK) {
			     return (idx);
			}
			lua_pop(L, 2);

			lidx = lua_gettop(l);
			for (int uv = 1;
			    (name = lua_getupvalue(L, idx, uv)) != NULL; uv++) {
				if (strcmp(name, "_ENV") == 0) {
					/* leave _ENV table alone */
					lua_pop(L, 1);
					continue;
				}
				if (copyn(L, l, -1, 1) != 0) {
					return (idx);
				}
				lua_pop(L, 1);
				name = lua_setupvalue(l, lidx, uv);
				assert(name != NULL);
			}
			assert(lua_gettop(l) == lidx);
			break;
		}
		default:
			return (idx);
		}
	}
	assert(lua_gettop(L) == Ltop);
	assert(lua_gettop(l) == ltop + n);
	return (0);
}

struct rcthread {
	pthread_t thread;
	atomic_refcount refs;
};

static inline int
checkattr(lua_State *L, int idx, pthread_attr_t *attr)
{
	assert(lua_type(L, idx) == LUA_TTABLE);
	switch (lua_getfield(L, idx, "stack")) {
	case LUA_TTABLE: {
		void *stackaddr;
		size_t stacksize;

		if (lua_getfield(L, -1, "addr") != LUA_TLIGHTUSERDATA) {
			return (-1);
		}
		stackaddr = lua_touserdata(L, -1);
		lua_pop(L, 1);
		if (lua_getfield(L, -1, "size") != LUA_TNUMBER) {
			return (-1);
		}
		stacksize = lua_tointeger(L, -1);
		lua_pop(L, 1);
		if (pthread_attr_setstack(attr, stackaddr, stacksize) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	/* pthread_attr_setstackaddr is obsolete */
	switch (lua_getfield(L, idx, "stacksize")) {
	case LUA_TNUMBER: {
		size_t stacksize = lua_tointeger(L, -1);

		if (pthread_attr_setstacksize(attr, stacksize) != 0) {
			return (-1);
		}
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "guardsize")) {
	case LUA_TNUMBER: {
		size_t guardsize = lua_tointeger(L, -1);

		if (pthread_attr_setguardsize(attr, guardsize) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "detachstate")) {
	case LUA_TNUMBER: {
		int detachstate = lua_tointeger(L, -1);

		if (pthread_attr_setdetachstate(attr, detachstate) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "inheritsched")) {
	case LUA_TNUMBER: {
		int inheritsched = lua_tointeger(L, -1);

		if (pthread_attr_setinheritsched(attr, inheritsched) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "schedparam")) {
	case LUA_TTABLE: {
		struct sched_param param = {};

		if (lua_getfield(L, -1, "sched_priority") != LUA_TNUMBER) {
			return (-1);
		}
		param.sched_priority = lua_tointeger(L, -1);
		lua_pop(L, 1);
		if (pthread_attr_setschedparam(attr, &param) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "schedpolicy")) {
	case LUA_TNUMBER: {
		int policy = lua_tointeger(L, -1);

		if (pthread_attr_setschedpolicy(attr, policy) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "scope")) {
	case LUA_TNUMBER: {
		int contentionscope = lua_tointeger(L, -1);

		if (pthread_attr_setscope(attr, contentionscope) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "createsuspend_np")) {
	case LUA_TBOOLEAN:
		if (lua_toboolean(L, -1) &&
		    pthread_attr_setcreatesuspend_np(attr) != 0) {
			return (-1);
		}
		break;
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "affinity_np")) {
	case LUA_TUSERDATA: {
		cpuset_t *cpuset;

		if (luaL_getmetatable(L, CPUSET_METATABLE) != LUA_TTABLE ||
		    lua_getmetatable(L, -2) == 0 ||
		    lua_rawequal(L, -1, -2) == 0) {
			return (-1);
		}
		lua_pop(L, 2);

		cpuset = lua_touserdata(L, -1);
		if (pthread_attr_setaffinity_np(attr, sizeof(*cpuset), cpuset)
		    != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	return (0);
}

static void
getcleanupstack(lua_State *L)
{
	lua_pushlightuserdata(L, getcleanupstack);
	lua_rawget(L, LUA_REGISTRYINDEX);
}

static inline void
initcleanupstack(lua_State *L)
{
	lua_pushlightuserdata(L, getcleanupstack);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);
}

static void
thread_cleanup(void *arg)
{
	lua_State *L = arg;
	int idx, n;

	getcleanupstack(L);
	idx = lua_gettop(L);
	n = luaL_len(L, -1);
	for (int i = 1; i <= n; i++) {
		int len;

		lua_rawgeti(L, idx, i);
		len = tunpack(L, -1);
		lua_pcall(L, len - 1, 0, 0);
	}
	lua_close(L);
}

/*
 * XXX: Implementation details...
 *
 * libthr has weak aliases for these symbols which don't require push and pop to
 * appear in the same lexical scope.
 */
#undef pthread_cleanup_push
#undef pthread_cleanup_pop
extern void pthread_cleanup_push(void (*)(void *), void *);
extern void pthread_cleanup_pop(int);

static __dead2 int
thread_kfunction(lua_State *L, int status, lua_KContext ctx __unused)
{
	pthread_cleanup_pop(0);
	lua_pushboolean(L, status == LUA_OK || status == LUA_YIELD);
	lua_insert(L, 1);
	pthread_exit(L);
}

static __dead2 void *
thread_wrapper(void *arg)
{
	lua_State *L = arg;
	int nargs = lua_gettop(L) - 1;

	initcleanupstack(L);
	pthread_cleanup_push(thread_cleanup, L);
	thread_kfunction(L, lua_pcallk(L, nargs, LUA_MULTRET, 0, 0,
	    thread_kfunction), 0);
}

static inline void
attr_destroy(lua_State *L, pthread_attr_t *attr)
{
	int error;

	if ((error = pthread_attr_destroy(attr)) != 0) {
		luaL_error(L, "pthread_attr_destroy: %s", strerror(error));
	}
}

static int
l_pthread_create(lua_State *L)
{
	pthread_attr_t attr;
	struct rcthread *threadp;
	lua_State *l;
	int error, n, fnidx, error1;

	if ((error = pthread_attr_init(&attr)) != 0) {
		return (luaL_error(L, "pthread_attr_init: %s",
		    strerror(error)));
	}

	n = lua_gettop(L);
	if (lua_type(L, 1) == LUA_TTABLE) {
		if (checkattr(L, 1, &attr) != 0) {
			attr_destroy(L, &attr);
			return (luaL_argerror(L, 1, "invalid attr table"));
		}
		--n;
		fnidx = 2;
	} else {
		fnidx = 1;
	}
	if (!lua_isfunction(L, fnidx)) {
		attr_destroy(L, &attr);
		return (luaL_typeerror(L, fnidx, "function"));
	}

	if ((l = luaL_newstate()) == NULL) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "luaL_newstate: %s", strerror(ENOMEM)));
	}
	luaL_openlibs(l);
	if ((error = copyn(L, l, fnidx, n)) != 0) {
		lua_close(l);
		attr_destroy(L, &attr);
		return (luaL_argerror(L, error, "non-serializable value"));
	}

	if ((threadp = malloc(sizeof(*threadp))) == NULL) {
		lua_close(L);
		attr_destroy(L, &attr);
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	error = pthread_create(&threadp->thread, &attr, thread_wrapper, l);
	if ((error1 = pthread_attr_destroy(&attr)) != 0) {
		lua_close(l);
		free(threadp);
		return (luaL_error(L, "pthread_attr_destroy: %s",
		    strerror(error1)));
	}
	if (error != 0) {
		lua_close(l);
		free(threadp);
		return (fail(L, error));
	}
	refcount_init(&threadp->refs, 1);
	lua_pushlightuserdata(L, threadp);
	return (new(L, PTHREAD_METATABLE));
}

static int
l_pthread_retain(lua_State *L)
{
	struct rcthread *threadp;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TNONE);

	threadp = lua_touserdata(L, 1);
	assert(threadp != NULL);
	refcount_retain(&threadp->refs);
	return(new(L, PTHREAD_METATABLE));
}

static inline void
checkthread(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PTHREAD_METATABLE);

	lua_getiuservalue(L, idx, COOKIE);
}

static int
l_pthread_cookie(lua_State *L)
{
	checkthread(L, 1);

	return (1);
}

static inline struct rcthread *
checkthreadp(lua_State *L, int idx)
{
	struct rcthread *threadp;

	checkthread(L, idx);

	threadp = lua_touserdata(L, -1);
	assert(threadp != NULL);
	lua_pop(L, 1);
	return (threadp);
}

static int
l_pthread_gc(lua_State *L)
{
	struct rcthread *threadp;

	threadp = checkthreadp(L, 1);

	if (refcount_release(&threadp->refs)) {
		free(threadp);
	}
	return (0);
}

static int
l_pthread_cancel(lua_State *L)
{
	struct rcthread *threadp;
	int error;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_cancel(threadp->thread)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_detach(lua_State *L)
{
	struct rcthread *threadp;
	int error;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_detach(threadp->thread)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_equal(lua_State *L)
{
	struct rcthread *t1p, *t2p;

	t1p = checkthreadp(L, 1);
	t2p = checkthreadp(L, 2);

	lua_pushboolean(L, pthread_equal(t1p->thread, t2p->thread));
	return (1);
}

static __dead2 int
l_pthread_exit(lua_State *L)
{
	lua_pushboolean(L, true);
	lua_insert(L, 1);
	pthread_exit(L);
}

static int
l_pthread_join(lua_State *L)
{
	struct rcthread *threadp;
	void *result;
	lua_State *l;
	int error, nresults;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_join(threadp->thread, &result)) != 0) {
		return (fail(L, error));
	}
	if (result == PTHREAD_CANCELED) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "canceled");
		return (2);
	}
	l = result;
	if ((nresults = lua_gettop(l)) > 0 && copyn(l, L, 1, nresults) != 0) {
		lua_close(l);
		return (luaL_error(L,
		    "thread returned a non-serializable value"));
	}
	lua_close(l);
	return (nresults);
}

static int
l_pthread_attr_get_np(lua_State *L)
{
	pthread_attr_t attr;
	struct sched_param param;
	struct rcthread *threadp;
	cpuset_t *cpuset;
	void *addr;
	size_t size;
	int error, val;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_attr_init(&attr)) != 0) {
		return (luaL_error(L, "pthread_attr_init: %s",
		    strerror(error)));
	}
	if ((error = pthread_attr_get_np(threadp->thread, &attr)) != 0) {
		attr_destroy(L, &attr);
		return (fail(L, error));
	}

	lua_newtable(L);

	if ((error = pthread_attr_getstack(&attr, &addr, &size)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getstack: %s",
		    strerror(error)));
	}
	lua_createtable(L, 0, 2);
	lua_pushlightuserdata(L, addr);
	lua_setfield(L, -2, "addr");
	lua_pushinteger(L, size);
	lua_setfield(L, -2, "size");
	lua_setfield(L, -2, "stack");

	if ((error = pthread_attr_getguardsize(&attr, &size)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getguardsize: %s",
		    strerror(error)));
	}
	lua_pushinteger(L, size);
	lua_setfield(L, -2, "guardsize");

	if ((error = pthread_attr_getdetachstate(&attr, &val)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getdetachstate: %s",
		    strerror(error)));
	}
	lua_pushinteger(L, val);
	lua_setfield(L, -2, "detachstate");

	if ((error = pthread_attr_getinheritsched(&attr, &val)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getinheritsched: %s",
		    strerror(error)));
	}
	lua_pushinteger(L, val);
	lua_setfield(L, -2, "inheritsched");

	if ((error = pthread_attr_getschedparam(&attr, &param)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getschedparam: %s",
		    strerror(error)));
	}
	lua_createtable(L, 0, 1);
	lua_pushinteger(L, param.sched_priority);
	lua_setfield(L, -2, "sched_priority");
	lua_setfield(L, -2, "schedparam");

	if ((error = pthread_attr_getschedpolicy(&attr, &val)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getschedpolicy: %s",
		    strerror(error)));
	}
	lua_pushinteger(L, val);
	lua_setfield(L, -2, "schedpolicy");

	if ((error = pthread_attr_getscope(&attr, &val)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getscope: %s",
		    strerror(error)));
	}
	lua_pushinteger(L, val);
	lua_setfield(L, -2, "scope");

	cpuset = newanoncpuset(L);
	if ((error = pthread_attr_getaffinity_np(&attr, sizeof(*cpuset),
	    cpuset)) != 0) {
		attr_destroy(L, &attr);
		return (luaL_error(L, "pthread_attr_getaffinity_np: %s",
		    strerror(error)));
	}
	lua_setfield(L, -2, "affinity_np");

	/* no pthread_attr_getcreatesuspend_np(3) */

	attr_destroy(L, &attr);
	return (1);
}

static int
l_pthread_getaffinity_np(lua_State *L)
{
	struct rcthread *threadp;
	cpuset_t *cpuset;
	int error;

	threadp = checkthreadp(L, 1);

	cpuset = newanoncpuset(L);
	if ((error = pthread_getaffinity_np(threadp->thread, sizeof(*cpuset),
	    cpuset)) != 0) {
		return (fail(L, error));
	}
	return (1);
}

static int
l_pthread_setaffinity_np(lua_State *L)
{
	struct rcthread *threadp;
	cpuset_t *cpuset;
	int error;

	threadp = checkthreadp(L, 1);
	cpuset = luaL_checkudata(L, 2, CPUSET_METATABLE);

	if ((error = pthread_setaffinity_np(threadp->thread, sizeof(*cpuset),
	    cpuset)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_resume_np(lua_State *L)
{
	struct rcthread *threadp;
	int error;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_resume_np(threadp->thread)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_peekjoin_np(lua_State *L)
{
	struct rcthread *threadp;
	void *result;
	lua_State *l;
	int error, nresults;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_peekjoin_np(threadp->thread, &result)) != 0) {
		return (fail(L, error));
	}
	if (result == PTHREAD_CANCELED) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "canceled");
		return (2);
	}
	l = result;
	if ((nresults = lua_gettop(l)) > 0 && copyn(l, L, 1, nresults) != 0) {
		return (luaL_error(L,
		    "thread returned a non-serializable value"));
	}
	return (nresults);
}

static int
l_pthread_suspend_np(lua_State *L)
{
	struct rcthread *threadp;
	int error;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_suspend_np(threadp->thread)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_timedjoin_np(lua_State *L)
{
	struct timespec abstime;
	struct rcthread *threadp;
	void *result;
	lua_State *l;
	int error, nresults;

	threadp = checkthreadp(L, 1);
	abstime.tv_sec = luaL_checkinteger(L, 2);
	abstime.tv_nsec = luaL_optinteger(L, 3, 0);

	if ((error = pthread_timedjoin_np(threadp->thread, &result, &abstime))
	    != 0) {
		return (fail(L, error));
	}
	if (result == PTHREAD_CANCELED) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "canceled");
		return (2);
	}
	l = result;
	if ((nresults = lua_gettop(l)) > 0 && copyn(l, L, 1, nresults) != 0) {
		lua_close(l);
		return (luaL_error(L,
		    "thread returned a non-serializable value"));
	}
	lua_close(l);
	return (nresults);
}

static int
l_pthread_kill(lua_State *L)
{
	struct rcthread *threadp;
	int sig, error;

	threadp = checkthreadp(L, 1);
	sig = luaL_checkinteger(L, 2);

	if ((error = pthread_kill(threadp->thread, sig)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static pthread_key_t thread_state_key;

__attribute__((constructor))
static void
init_thread_state_key(void)
{
	pthread_key_create(&thread_state_key, NULL);
}

__attribute__((destructor))
static void
fini_thread_state_key(void)
{
	pthread_key_delete(thread_state_key);
}

static void
once_wrapper(void)
{
	lua_State *L = pthread_getspecific(thread_state_key);
	int status;

	if (L != NULL) {
		status = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);
		lua_pushboolean(L, status == LUA_OK);
	}
}

struct rconce {
	pthread_once_t control;
	atomic_refcount refs;
};

static int
l_pthread_once_new(lua_State *L)
{
	struct rconce *oncep;
	int error;

	luaL_checktype(L, 1, LUA_TNONE);

	if ((oncep = malloc(sizeof(*oncep))) == NULL) {
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	oncep->control = (pthread_once_t)PTHREAD_ONCE_INIT;
	refcount_init(&oncep->refs, 1);
	lua_pushlightuserdata(L, oncep);
	return (new(L, PTHREAD_ONCE_METATABLE));
}

static int
l_pthread_once_retain(lua_State *L)
{
	struct rconce *oncep;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TNONE);

	oncep = lua_touserdata(L, 1);
	assert(oncep != NULL);
	refcount_retain(&oncep->refs);
	return (new(L, PTHREAD_ONCE_METATABLE));
}

static inline void
checkonce(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PTHREAD_ONCE_METATABLE);

	lua_getiuservalue(L, idx, COOKIE);
}

static int
l_pthread_once_cookie(lua_State *L)
{
	checkonce(L, 1);

	return (1);
}

static inline struct rconce *
checkoncep(lua_State *L, int idx)
{
	struct rconce *oncep;

	checkonce(L, idx);

	oncep = lua_touserdata(L, -1);
	assert(oncep != NULL);
	lua_pop(L, 1);
	return (oncep);
}

static int
l_pthread_once_gc(lua_State *L)
{
	struct rconce *oncep;

	oncep = checkoncep(L, 1);

	if (refcount_release(&oncep->refs)) {
		free(oncep);
	}
	return (0);
}

static int
l_pthread_once_call(lua_State *L)
{
	struct rconce *oncep;
	int top, error, nresults;

	oncep = checkoncep(L, 1);

	top = lua_gettop(L);
	if ((error = pthread_once(&oncep->control, once_wrapper)) != 0) {
		return (fail(L, error));
	}
	nresults = lua_gettop(L) - top;
	assert(nresults > 0);
	lua_insert(L, top + 1); /* move status to the front */
	return (nresults);
}

static int
l_pthread_self(lua_State *L)
{
	struct rcthread *threadp;

	if ((threadp = malloc(sizeof(*threadp))) == NULL) {
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	threadp->thread = pthread_self();
	refcount_init(&threadp->refs, 1);
	lua_pushlightuserdata(L, threadp);
	return (new(L, PTHREAD_METATABLE));
}

static int
l_pthread_setcancelstate(lua_State *L)
{
	int state, oldstate, error;

	state = luaL_checkinteger(L, 1);

	if ((error = pthread_setcancelstate(state, &oldstate)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, oldstate);
	return (1);
}

static int
l_pthread_setcanceltype(lua_State *L)
{
	int type, oldtype, error;

	type = luaL_checkinteger(L, 1);

	if ((error = pthread_setcanceltype(type, &oldtype)) != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, oldtype);
	return (1);
}

static int
l_pthread_testcancel(lua_State *L)
{
	pthread_testcancel();
	return (0);
}

static int
l_pthread_yield(lua_State *L __unused)
{
	pthread_yield();
	return (0);
}

static int
l_pthread_getthreadid_np(lua_State *L)
{
	lua_pushinteger(L, pthread_getthreadid_np());
	return (1);
}

static int
l_pthread_main_np(lua_State *L)
{
	int ismain = pthread_main_np();

	if (ismain == -1) {
		luaL_pushfail(L);
		lua_pushstring(L, "not ready");
		return (2);
	}
	lua_pushboolean(L, ismain);
	return (1);
}

static int
l_pthread_multi_np(lua_State *L)
{
	int error = pthread_multi_np();

	assert(error == 0);
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_resume_all_np(lua_State *L __unused)
{
	pthread_resume_all_np();
	return (0);
}

static int
l_pthread_signals_block_np(lua_State *L __unused)
{
	pthread_signals_block_np();
	return (0);
}

static int
l_pthread_signals_unblock_np(lua_State *L __unused)
{
	pthread_signals_unblock_np();
	return (0);
}

static int
l_pthread_single_np(lua_State *L)
{
	int error = pthread_single_np();

	assert(error == 0);
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_suspend_all_np(lua_State *L __unused)
{
	pthread_suspend_all_np();
	return (0);
}

struct rcmutex {
	pthread_mutex_t mutex;
	atomic_refcount refs;
};

static inline int
checkmutexattr(lua_State *L, int idx, pthread_mutexattr_t *attr)
{
	assert(lua_type(L, idx) == LUA_TTABLE);
	switch (lua_getfield(L, idx, "prioceiling")) {
	case LUA_TNUMBER: {
		int ceiling = lua_tointeger(L, -1);

		if (pthread_mutexattr_setprioceiling(attr, ceiling) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "protocol")) {
	case LUA_TNUMBER: {
		int protocol = lua_tointeger(L, -1);

		if (pthread_mutexattr_setprotocol(attr, protocol) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "type")) {
	case LUA_TNUMBER: {
		int type = lua_tointeger(L, -1);

		if (pthread_mutexattr_settype(attr, type) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	return (0);
}

static inline void
mutexattr_destroy(lua_State *L, pthread_mutexattr_t *attr)
{
	int error;

	if ((error = pthread_mutexattr_destroy(attr)) != 0) {
		luaL_error(L, "pthread_mutexattr_destroy: %s", strerror(error));
	}
}

static int
l_pthread_mutex_new(lua_State *L)
{
	pthread_mutexattr_t attr;
	struct rcmutex *mutexp;
	int error, error1;

	luaL_checktype(L, 2, LUA_TNONE);

	if ((error = pthread_mutexattr_init(&attr)) != 0) {
		return (luaL_error(L, "pthread_mutexattr_init: %s",
		    strerror(error)));
	}

	switch (lua_type(L, 1)) {
	case LUA_TTABLE:
		if (checkmutexattr(L, 1, &attr) != 0) {
			mutexattr_destroy(L, &attr);
			return (luaL_argerror(L, 1, "invalid attr table"));
		}
		break;
	case LUA_TNIL:
	case LUA_TNONE:
		break;
	default:
		mutexattr_destroy(L, &attr);
		return (luaL_argerror(L, 1, "invalid argument"));
	}

	if ((mutexp = malloc(sizeof(*mutexp))) == NULL) {
		mutexattr_destroy(L, &attr);
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	error = pthread_mutex_init(&mutexp->mutex, &attr);
	if ((error1 = pthread_mutexattr_destroy(&attr)) != 0) {
		free(mutexp);
		return (luaL_error(L, "pthread_mutexattr_destroy: %s",
		    strerror(error1)));
	}
	if (error != 0) {
		free(mutexp);
		return (fail(L, error));
	}
	refcount_init(&mutexp->refs, 1);
	lua_pushlightuserdata(L, mutexp);
	return (new(L, PTHREAD_MUTEX_METATABLE));
}

static int
l_pthread_mutex_retain(lua_State *L)
{
	struct rcmutex *mutexp;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TNONE);

	mutexp = lua_touserdata(L, 1);
	assert(mutexp != NULL);
	refcount_retain(&mutexp->refs);
	return (new(L, PTHREAD_MUTEX_METATABLE));
}

static inline void
checkmutex(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PTHREAD_MUTEX_METATABLE);

	lua_getiuservalue(L, idx, COOKIE);
}

static int
l_pthread_mutex_cookie(lua_State *L)
{
	checkmutex(L, 1);

	return (1);
}

static inline struct rcmutex *
checkmutexp(lua_State *L, int idx)
{
	struct rcmutex *mutexp;

	checkmutex(L, idx);

	mutexp = lua_touserdata(L, -1);
	assert(mutexp != NULL);
	lua_pop(L, 1);
	return (mutexp);
}

static int
l_pthread_mutex_gc(lua_State *L)
{
	struct rcmutex *mutexp;
	int error;

	mutexp = checkmutexp(L, 1);

	if (refcount_release(&mutexp->refs)) {
		error = pthread_mutex_destroy(&mutexp->mutex);
		free(mutexp);
		if (error != 0) {
			return (luaL_error(L, "pthread_mutex_destroy: %s",
			    strerror(error)));
		}
	}
	return (0);
}

static int
l_pthread_mutex_lock(lua_State *L)
{
	struct rcmutex *mutexp;
	int error;

	mutexp = checkmutexp(L, 1);

	if ((error = pthread_mutex_lock(&mutexp->mutex)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_mutex_timedlock(lua_State *L)
{
	struct timespec abstime;
	struct rcmutex *mutexp;
	int error;

	mutexp = checkmutexp(L, 1);
	abstime.tv_sec = luaL_checkinteger(L, 2);
	abstime.tv_nsec = luaL_optinteger(L, 3, 0);

	if ((error = pthread_mutex_timedlock(&mutexp->mutex, &abstime)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_mutex_trylock(lua_State *L)
{
	struct rcmutex *mutexp;
	int error;

	mutexp = checkmutexp(L, 1);

	if ((error = pthread_mutex_trylock(&mutexp->mutex)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_mutex_unlock(lua_State *L)
{
	struct rcmutex *mutexp;
	int error;

	mutexp = checkmutexp(L, 1);

	if ((error = pthread_mutex_unlock(&mutexp->mutex)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_mutex_getspinloops_np(lua_State *L)
{
	struct rcmutex *mutexp;
	int error, count;

	mutexp = checkmutexp(L, 1);

	if ((error = pthread_mutex_getspinloops_np(&mutexp->mutex, &count))
	    != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, count);
	return (1);
}

static int
l_pthread_mutex_setspinloops_np(lua_State *L)
{
	struct rcmutex *mutexp;
	int count, error;

	mutexp = checkmutexp(L, 1);
	count = luaL_checkinteger(L, 2);

	if ((error = pthread_mutex_setspinloops_np(&mutexp->mutex, count))
	    != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_mutex_getyieldloops_np(lua_State *L)
{
	struct rcmutex *mutexp;
	int error, count;

	mutexp = checkmutexp(L, 1);

	if ((error = pthread_mutex_getyieldloops_np(&mutexp->mutex, &count))
	    != 0) {
		return (fail(L, error));
	}
	lua_pushinteger(L, count);
	return (1);
}

static int
l_pthread_mutex_setyieldloops_np(lua_State *L)
{
	struct rcmutex *mutexp;
	int count, error;

	mutexp = checkmutexp(L, 1);
	count = luaL_checkinteger(L, 2);

	if ((error = pthread_mutex_setyieldloops_np(&mutexp->mutex, count))
	    != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_mutex_isowned_np(lua_State *L)
{
	struct rcmutex *mutexp;
	int isowned;

	mutexp = checkmutexp(L, 1);

	if ((isowned = pthread_mutex_isowned_np(&mutexp->mutex)) == -1) {
		luaL_pushfail(L);
		lua_pushstring(L, "unknown error");
		return (2);
	}
	lua_pushboolean(L, isowned);
	return (1);
}

struct rccond {
	pthread_cond_t cond;
	atomic_refcount refs;
};

static inline int
checkcondattr(lua_State *L, int idx, pthread_condattr_t *attr)
{
	assert(lua_type(L, idx) == LUA_TTABLE);
	switch (lua_getfield(L, idx, "clock")) {
	case LUA_TNUMBER: {
		clockid_t clock_id = lua_tointeger(L, -1);

		if (pthread_condattr_setclock(attr, clock_id) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	switch (lua_getfield(L, idx, "pshared")) {
	case LUA_TNUMBER: {
		int pshared = lua_tointeger(L, -1);

		if (pthread_condattr_setpshared(attr, pshared) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	return (0);
}

static inline void
condattr_destroy(lua_State *L, pthread_condattr_t *attr)
{
	int error;

	if ((error = pthread_condattr_destroy(attr)) != 0) {
		luaL_error(L, "pthread_condattr_destroy: %s", strerror(error));
	}
}

static int
l_pthread_cond_new(lua_State *L)
{
	pthread_condattr_t attr;
	struct rccond *condp;
	int error, error1;

	luaL_checktype(L, 2, LUA_TNONE);

	if ((error = pthread_condattr_init(&attr)) != 0) {
		return (luaL_error(L, "pthread_condattr_init: %s",
		    strerror(error)));
	}

	switch (lua_type(L, 1)) {
	case LUA_TTABLE:
		if (checkcondattr(L, 1, &attr) != 0) {
			condattr_destroy(L, &attr);
			return (luaL_argerror(L, 1, "invalid attr table"));
		}
		break;
	case LUA_TNIL:
	case LUA_TNONE:
		break;
	default:
		condattr_destroy(L, &attr);
		return (luaL_argerror(L, 1, "invalid argument"));
	}

	if ((condp = malloc(sizeof(*condp))) == NULL) {
		condattr_destroy(L, &attr);
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	error = pthread_cond_init(&condp->cond, &attr);
	if ((error1 = pthread_condattr_destroy(&attr)) != 0) {
		free(condp);
		return (luaL_error(L, "pthread_condattr_destroy: %s",
		    strerror(error1)));
	}
	if (error != 0) {
		free(condp);
		return (fail(L, error));
	}
	refcount_init(&condp->refs, 1);
	lua_pushlightuserdata(L, condp);
	return (new(L, PTHREAD_COND_METATABLE));
}

static int
l_pthread_cond_retain(lua_State *L)
{
	struct rccond *condp;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TNONE);

	condp = lua_touserdata(L, 1);
	assert(condp != NULL);
	refcount_retain(&condp->refs);
	return (new(L, PTHREAD_COND_METATABLE));
}

static inline void
checkcond(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PTHREAD_COND_METATABLE);

	lua_getiuservalue(L, idx, COOKIE);
}

static int
l_pthread_cond_cookie(lua_State *L)
{
	checkcond(L, 1);

	return (1);
}

static inline struct rccond *
checkcondp(lua_State *L, int idx)
{
	struct rccond *condp;

	checkcond(L, idx);

	condp = lua_touserdata(L, -1);
	assert(condp != NULL);
	lua_pop(L, 1);
	return (condp);
}

static int
l_pthread_cond_gc(lua_State *L)
{
	struct rccond *condp;
	int error;

	condp = checkcondp(L, 1);

	if (refcount_release(&condp->refs)) {
		error = pthread_cond_destroy(&condp->cond);
		free(condp);
		if (error != 0) {
			return (luaL_error(L, "pthread_cond_destroy: %s",
			    strerror(error)));
		}
	}
	return (0);
}

static int
l_pthread_cond_broadcast(lua_State *L)
{
	struct rccond *condp;
	int error;

	condp = checkcondp(L, 1);

	if ((error = pthread_cond_broadcast(&condp->cond)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_cond_signal(lua_State *L)
{
	struct rccond *condp;
	int error;

	condp = checkcondp(L, 1);

	if ((error = pthread_cond_signal(&condp->cond)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_cond_timedwait(lua_State *L)
{
	struct timespec abstime;
	struct rccond *condp;
	struct rcmutex *mutexp;
	int error;

	condp = checkcondp(L, 1);
	mutexp = checkmutexp(L, 2);
	abstime.tv_sec = luaL_checkinteger(L, 3);
	abstime.tv_nsec = luaL_optinteger(L, 4, 0);

	if ((error = pthread_cond_timedwait(&condp->cond, &mutexp->mutex,
	    &abstime)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_cond_wait(lua_State *L)
{
	struct rccond *condp;
	struct rcmutex *mutexp;
	int error;

	condp = checkcondp(L, 1);
	mutexp = checkmutexp(L, 2);

	if ((error = pthread_cond_wait(&condp->cond, &mutexp->mutex)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

struct rcrwlock {
	pthread_rwlock_t lock;
	atomic_refcount refs;
};

static inline int
checkrwlockattr(lua_State *L, int idx, pthread_rwlockattr_t *attr)
{
	assert(lua_type(L, idx) == LUA_TTABLE);
	switch (lua_getfield(L, idx, "pshared")) {
	case LUA_TNUMBER: {
		int pshared = lua_tointeger(L, -1);

		if (pthread_rwlockattr_setpshared(attr, pshared) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	/* pthread_rwlockattr_setkind_np doesn't actually exist */
	return (0);
}

static inline void
rwlockattr_destroy(lua_State *L, pthread_rwlockattr_t *attr)
{
	int error;

	if ((error = pthread_rwlockattr_destroy(attr)) != 0) {
		luaL_error(L, "pthread_rwlockattr_destroy: %s",
		    strerror(error));
	}
}

static int
l_pthread_rwlock_new(lua_State *L)
{
	pthread_rwlockattr_t attr;
	struct rcrwlock *lockp;
	int error, error1;

	luaL_checktype(L, 2, LUA_TNONE);

	if ((error = pthread_rwlockattr_init(&attr)) != 0) {
		return (luaL_error(L, "pthread_rwlockattr_init: %s",
		    strerror(error)));
	}

	switch (lua_type(L, 1)) {
	case LUA_TTABLE:
		if (checkrwlockattr(L, 1, &attr) != 0) {
			rwlockattr_destroy(L, &attr);
			return (luaL_argerror(L, 1, "invalid attr table"));
		}
		break;
	case LUA_TNIL:
	case LUA_TNONE:
		break;
	default:
		rwlockattr_destroy(L, &attr);
		return (luaL_argerror(L, 1, "invalid argument"));
	}

	if ((lockp = malloc(sizeof(*lockp))) == NULL) {
		rwlockattr_destroy(L, &attr);
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	error = pthread_rwlock_init(&lockp->lock, &attr);
	if ((error1 = pthread_rwlockattr_destroy(&attr)) != 0) {
		free(lockp);
		return (luaL_error(L, "pthread_rwlockattr_destroy: %s",
		    strerror(error1)));
	}
	if (error != 0) {
		free(lockp);
		return (fail(L, error));
	}
	refcount_init(&lockp->refs, 1);
	lua_pushlightuserdata(L, lockp);
	return (new(L, PTHREAD_RWLOCK_METATABLE));
}

static int
l_pthread_rwlock_retain(lua_State *L)
{
	struct rcrwlock *lockp;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TNONE);

	lockp = lua_touserdata(L, 1);
	assert(lockp != NULL);
	refcount_retain(&lockp->refs);
	return (new(L, PTHREAD_RWLOCK_METATABLE));
}

static inline void
checkrwlock(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PTHREAD_RWLOCK_METATABLE);

	lua_getiuservalue(L, idx, COOKIE);
}

static int
l_pthread_rwlock_cookie(lua_State *L)
{
	checkrwlock(L, 1);

	return (1);
}

static inline struct rcrwlock *
checkrwlockp(lua_State *L, int idx)
{
	struct rcrwlock *lockp;

	checkrwlock(L, idx);

	lockp = lua_touserdata(L, -1);
	assert(lockp != NULL);
	lua_pop(L, 1);
	return (lockp);
}

static int
l_pthread_rwlock_gc(lua_State *L)
{
	struct rcrwlock *lockp;
	int error;

	lockp = checkrwlockp(L, 1);

	if (refcount_release(&lockp->refs)) {
		error = pthread_rwlock_destroy(&lockp->lock);
		free(lockp);
		if (error != 0) {
			return (luaL_error(L, "pthread_rwlock_destroy: %s",
			    strerror(error)));
		}
	}
	return (0);
}

static int
l_pthread_rwlock_rdlock(lua_State *L)
{
	struct rcrwlock *lockp;
	int error;

	lockp = checkrwlockp(L, 1);

	if ((error = pthread_rwlock_rdlock(&lockp->lock)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_rwlock_tryrdlock(lua_State *L)
{
	struct rcrwlock *lockp;
	int error;

	lockp = checkrwlockp(L, 1);

	if ((error = pthread_rwlock_tryrdlock(&lockp->lock)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_rwlock_trywrlock(lua_State *L)
{
	struct rcrwlock *lockp;
	int error;

	lockp = checkrwlockp(L, 1);

	if ((error = pthread_rwlock_trywrlock(&lockp->lock)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_rwlock_unlock(lua_State *L)
{
	struct rcrwlock *lockp;
	int error;

	lockp = checkrwlockp(L, 1);

	if ((error = pthread_rwlock_unlock(&lockp->lock)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_rwlock_wrlock(lua_State *L)
{
	struct rcrwlock *lockp;
	int error;

	lockp = checkrwlockp(L, 1);

	if ((error = pthread_rwlock_wrlock(&lockp->lock)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

struct rckey {
	pthread_key_t key;
	atomic_refcount refs;
};

static int
l_pthread_key_create(lua_State *L)
{
	void (*destructor)(void *);
	struct rckey *keyp;
	int error;

	destructor = lua_islightuserdata(L, 1) ? lua_touserdata(L, 1) : NULL;
	luaL_checktype(L, 2, LUA_TNONE);

	if ((keyp = malloc(sizeof(*keyp))) == NULL) {
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	if ((error = pthread_key_create(&keyp->key, destructor)) != 0) {
		free(keyp);
		return (fail(L, error));
	}
	refcount_init(&keyp->refs, 1);
	lua_pushlightuserdata(L, keyp);
	return (new(L, PTHREAD_KEY_METATABLE));
}

static int
l_pthread_key_retain(lua_State *L)
{
	struct rckey *keyp;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TNONE);

	keyp = lua_touserdata(L, 1);
	assert(keyp != NULL);
	refcount_retain(&keyp->refs);
	return (new(L, PTHREAD_KEY_METATABLE));
}

static inline void
checkkey(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PTHREAD_KEY_METATABLE);

	lua_getiuservalue(L, idx, COOKIE);
}

static int
l_pthread_key_cookie(lua_State *L)
{
	checkkey(L, 1);

	return (1);
}

static inline struct rckey *
checkkeyp(lua_State *L, int idx)
{
	struct rckey *keyp;

	checkkey(L, idx);

	keyp = lua_touserdata(L, -1);
	assert(keyp != NULL);
	lua_pop(L, 1);
	return (keyp);
}

static int
l_pthread_key_gc(lua_State *L)
{
	struct rckey *keyp;
	int error;

	keyp = checkkeyp(L, 1);

	if (refcount_release(&keyp->refs)) {
		error = pthread_key_delete(keyp->key);
		free(keyp);
		if (error != 0) {
			return (luaL_error(L, "pthread_key_delete: %s",
			    strerror(error)));
		}
	}
	return (0);
}

static int
l_pthread_key_getspecific(lua_State *L)
{
	struct rckey *keyp;
	void *value;

	keyp = checkkeyp(L, 1);

	value = pthread_getspecific(keyp->key);
	lua_pushlightuserdata(L, value);
	return (1);
}

static int
l_pthread_key_setspecific(lua_State *L)
{
	struct rckey *keyp;
	void *value;
	int error;

	keyp = checkkeyp(L, 1);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	value = lua_touserdata(L, 2);

	if ((error = pthread_setspecific(keyp->key, value)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_getname_np(lua_State *L)
{
	char name[MAXCOMLEN + 1];
	struct rcthread *threadp;
	int error;

	threadp = checkthreadp(L, 1);

	if ((error = pthread_getname_np(threadp->thread, name, sizeof(name)))
	    != 0) {
		return (fail(L, error));
	}
	lua_pushstring(L, name);
	return (1);
}

static int
l_pthread_setname_np(lua_State *L)
{
	struct rcthread *threadp;
	const char *name;
	int error;

	threadp = checkthreadp(L, 1);
	name = luaL_checkstring(L, 2);

	if ((error = pthread_setname_np(threadp->thread, name)) != 0) {
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_pthread_cleanup_pop(lua_State *L)
{
	int idx, len, status;
	bool execute;

	execute = lua_toboolean(L, 1);

	getcleanupstack(L);
	idx = lua_gettop(L);
	tpop(L, idx);
	lua_remove(L, idx);
	if (execute) {
		len = tunpack(L, -1);
		status = lua_pcall(L, len - 1, LUA_MULTRET, 0);
		lua_pushboolean(L, status == LUA_OK);
		lua_insert(L, idx);
	} else {
		lua_pushboolean(L, true);
	}
	return (lua_gettop(L) - idx);
}

static int
l_pthread_cleanup_push(lua_State *L)
{
	int idx;

	tpack(L, lua_gettop(L));

	idx = lua_gettop(L);
	getcleanupstack(L);
	lua_pushvalue(L, idx);
	tpush(L, -2);
	return (0);
}

struct rcbarrier {
	pthread_barrier_t barrier;
	atomic_refcount refs;
};

static inline int
checkbarrierattr(lua_State *L, int idx, pthread_barrierattr_t *attr)
{
	assert(lua_type(L, idx) == LUA_TTABLE);
	switch (lua_getfield(L, idx, "pshared")) {
	case LUA_TNUMBER: {
		int pshared = lua_tointeger(L, -1);

		if (pthread_barrierattr_setpshared(attr, pshared) != 0) {
			return (-1);
		}
		break;
	}
	case LUA_TNIL:
		break;
	default:
		return (-1);
	}
	lua_pop(L, 1);
	return (0);
}

static inline void
barrierattr_destroy(lua_State *L, pthread_barrierattr_t *attr)
{
	int error;

	if ((error = pthread_barrierattr_destroy(attr)) != 0) {
		luaL_error(L, "pthread_barrierattr_destroy: %s",
		    strerror(error));
	}
}

static int
l_pthread_barrier_new(lua_State *L)
{
	pthread_barrierattr_t attr;
	struct rcbarrier *barrierp;
	unsigned count;
	int error, countidx, error1;

	if ((error = pthread_barrierattr_init(&attr)) != 0) {
		return (luaL_error(L, "pthread_barrierattr_init: %s",
		    strerror(error)));
	}

	if (lua_type(L, 1) == LUA_TTABLE) {
		if (checkbarrierattr(L, 1, &attr) != 0) {
			barrierattr_destroy(L, &attr);
			return (luaL_argerror(L, 1, "invalid attr table"));
		}
		countidx = 2;
	} else {
		countidx = 1;
	}
	if (!lua_isinteger(L, countidx)) {
		barrierattr_destroy(L, &attr);
		return (luaL_typeerror(L, 1, "integer"));
	}
	count = lua_tointeger(L, countidx);

	if ((barrierp = malloc(sizeof(*barrierp))) == NULL) {
		barrierattr_destroy(L, &attr);
		return (luaL_error(L, "malloc: %s", strerror(ENOMEM)));
	}
	error = pthread_barrier_init(&barrierp->barrier, &attr, count);
	if ((error1 = pthread_barrierattr_destroy(&attr)) != 0) {
		free(barrierp);
		return (luaL_error(L, "pthread_barrierattr_destroy: %s",
		    strerror(error1)));
	}
	if (error != 0) {
		free(barrierp);
		return (fail(L, error));
	}
	refcount_init(&barrierp->refs, 1);
	lua_pushlightuserdata(L, barrierp);
	return (new(L, PTHREAD_BARRIER_METATABLE));
}

static int
l_pthread_barrier_retain(lua_State *L)
{
	struct rcbarrier *barrierp;

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TNONE);

	barrierp = lua_touserdata(L, 1);
	assert(barrierp != NULL);
	refcount_retain(&barrierp->refs);
	return (new(L, PTHREAD_BARRIER_METATABLE));
}

static inline void
checkbarrier(lua_State *L, int idx)
{
	luaL_checkudata(L, idx, PTHREAD_BARRIER_METATABLE);

	lua_getiuservalue(L, idx, COOKIE);
}

static int
l_pthread_barrier_cookie(lua_State *L)
{
	checkbarrier(L, 1);

	return (1);
}

static inline struct rcbarrier *
checkbarrierp(lua_State *L, int idx)
{
	struct rcbarrier *barrierp;

	checkbarrier(L, idx);

	barrierp = lua_touserdata(L, -1);
	assert(barrierp != NULL);
	lua_pop(L, 1);
	return (barrierp);
}

static int
l_pthread_barrier_gc(lua_State *L)
{
	struct rcbarrier *barrierp;
	int error;

	barrierp = checkbarrierp(L, 1);

	if (refcount_release(&barrierp->refs)) {
		error = pthread_barrier_destroy(&barrierp->barrier);
		free(barrierp);
		if (error != 0) {
			return (luaL_error(L, "pthread_barrier_destroy: %s",
			    strerror(error)));
		}
	}
	return (0);
}

static int
l_pthread_barrier_wait(lua_State *L)
{
	struct rcbarrier *barrierp;
	int error;

	barrierp = checkbarrierp(L, 1);

	if ((error = pthread_barrier_wait(&barrierp->barrier)) != 0) {
		if (error == PTHREAD_BARRIER_SERIAL_THREAD) {
			lua_pushinteger(L, error);
			return (1);
		}
		return (fail(L, error));
	}
	lua_pushboolean(L, true);
	return (1);
}

static const struct luaL_Reg l_pthread_funcs[] = {
	{"create", l_pthread_create}, /* (...) */
	{"retain", l_pthread_retain},
	{"exit", l_pthread_exit}, /* (...) */
	{"self", l_pthread_self},
	{"setcancelstate", l_pthread_setcancelstate},
	{"setcanceltype", l_pthread_setcanceltype},
	{"testcancel", l_pthread_testcancel}, /* (...) */
	{"yield", l_pthread_yield},
	{"getthreadid_np", l_pthread_getthreadid_np},
	{"main_np", l_pthread_main_np},
	{"multi_np", l_pthread_multi_np},
	{"resume_all_np", l_pthread_resume_all_np},
	{"signals_block_np", l_pthread_signals_block_np},
	{"signals_unblock_np", l_pthread_signals_unblock_np},
	{"single_np", l_pthread_single_np},
	{"suspend_all_np", l_pthread_suspend_all_np},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_meta[] = {
	{"__eq", l_pthread_equal},
	{"__gc", l_pthread_gc},
	{"cookie", l_pthread_cookie},
	{"cancel", l_pthread_cancel},
	{"detach", l_pthread_detach},
	{"equal", l_pthread_equal},
	{"join", l_pthread_join},
	{"attr_get_np", l_pthread_attr_get_np},
	{"getaffinity_np", l_pthread_getaffinity_np},
	{"setaffinity_np", l_pthread_setaffinity_np},
	{"resume_np", l_pthread_resume_np},
	{"peekjoin_np", l_pthread_peekjoin_np},
	{"suspend_np", l_pthread_suspend_np},
	{"timedjoin_np", l_pthread_timedjoin_np},
	{"getname_np", l_pthread_getname_np},
	{"setname_np", l_pthread_setname_np},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_once_funcs[] = {
	{"new", l_pthread_once_new},
	{"retain", l_pthread_once_retain},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_once_meta[] = {
	{"__call", l_pthread_once_call}, /* (...) */
	{"__gc", l_pthread_once_gc},
	{"cookie", l_pthread_once_cookie},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_mutex_funcs[] = {
	{"new", l_pthread_mutex_new},
	{"retain", l_pthread_mutex_retain},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_mutex_meta[] = {
	{"__gc", l_pthread_mutex_gc},
	{"cookie", l_pthread_mutex_cookie},
	{"lock", l_pthread_mutex_lock},
	{"timedlock", l_pthread_mutex_timedlock},
	{"trylock", l_pthread_mutex_trylock},
	{"unlock", l_pthread_mutex_unlock},
	{"getspinloops_np", l_pthread_mutex_getspinloops_np},
	{"setspinloops_np", l_pthread_mutex_setspinloops_np},
	{"getyieldloops_np", l_pthread_mutex_getyieldloops_np},
	{"setyieldloops_np", l_pthread_mutex_setyieldloops_np},
	{"isowned_np", l_pthread_mutex_isowned_np},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_cond_funcs[] = {
	{"new", l_pthread_cond_new},
	{"retain", l_pthread_cond_retain},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_cond_meta[] = {
	{"__gc", l_pthread_cond_gc},
	{"cookie", l_pthread_cond_cookie},
	{"broadcast", l_pthread_cond_broadcast},
	{"signal", l_pthread_cond_signal},
	{"timedwait", l_pthread_cond_timedwait},
	{"wait", l_pthread_cond_wait},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_rwlock_funcs[] = {
	{"new", l_pthread_rwlock_new},
	{"retain", l_pthread_rwlock_retain},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_rwlock_meta[] = {
	{"__gc", l_pthread_rwlock_gc},
	{"cookie", l_pthread_rwlock_cookie},
	{"rdlock", l_pthread_rwlock_rdlock},
	{"tryrdlock", l_pthread_rwlock_tryrdlock},
	{"trywrlock", l_pthread_rwlock_trywrlock},
	{"unlock", l_pthread_rwlock_unlock},
	{"wrlock", l_pthread_rwlock_wrlock},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_key_funcs[] = {
	{"create", l_pthread_key_create},
	{"retain", l_pthread_key_retain},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_key_meta[] = {
	{"__gc", l_pthread_key_gc},
	{"cookie", l_pthread_key_cookie},
	{"getspecific", l_pthread_key_getspecific},
	{"setspecific", l_pthread_key_setspecific},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_cleanup_funcs[] = {
	/* atfork handlers tricky to wrap */
	{"pop", l_pthread_cleanup_pop},
	{"push", l_pthread_cleanup_push},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_barrier_funcs[] = {
	{"new", l_pthread_barrier_new},
	{"retain", l_pthread_barrier_retain},
	{NULL, NULL}
};

static const struct luaL_Reg l_pthread_barrier_meta[] = {
	{"__gc", l_pthread_barrier_gc},
	{"cookie", l_pthread_barrier_cookie},
	{"wait", l_pthread_barrier_wait},
	{NULL, NULL}
};

int
luaopen_pthread(lua_State *L)
{
	int error;

	/* Load cpuset module for its metatable. */
	lua_getglobal(L, "require");
	lua_pushstring(L, "cpuset");
	lua_call(L, 1, 0);

	if ((error = pthread_setspecific(thread_state_key, L)) != 0) {
		return (luaL_error(L, "pthread_setspecific: %s",
		    strerror(error)));
	}

	luaL_newmetatable(L, PTHREAD_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pthread_meta, 0);

	luaL_newmetatable(L, PTHREAD_ONCE_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pthread_once_meta, 0);

	luaL_newmetatable(L, PTHREAD_MUTEX_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pthread_mutex_meta, 0);

	luaL_newmetatable(L, PTHREAD_COND_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pthread_cond_meta, 0);

	luaL_newmetatable(L, PTHREAD_RWLOCK_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pthread_rwlock_meta, 0);

	luaL_newmetatable(L, PTHREAD_KEY_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pthread_key_meta, 0);

	luaL_newmetatable(L, PTHREAD_BARRIER_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pthread_barrier_meta, 0);

	luaL_newlib(L, l_pthread_funcs);

	luaL_newlib(L, l_pthread_once_funcs);
	lua_setfield(L, -2, "once");

	luaL_newlib(L, l_pthread_mutex_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, PTHREAD_MUTEX_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(ERRORCHECK);
	SETCONST(RECURSIVE);
	SETCONST(NORMAL);
	SETCONST(ADAPTIVE_NP);
	SETCONST(TYPE_MAX);
	SETCONST(DEFAULT);

	SETCONST(STALLED);
	SETCONST(ROBUST);
#undef SETCONST
	lua_setfield(L, -2, "mutex");

	luaL_newlib(L, l_pthread_cond_funcs);
	lua_setfield(L, -2, "cond");

	luaL_newlib(L, l_pthread_rwlock_funcs);
	lua_setfield(L, -2, "rwlock");

	luaL_newlib(L, l_pthread_key_funcs);
	lua_setfield(L, -2, "key");

	luaL_newlib(L, l_pthread_cleanup_funcs);
	lua_setfield(L, -2, "cleanup");

	luaL_newlib(L, l_pthread_barrier_funcs);
	lua_pushinteger(L, PTHREAD_BARRIER_SERIAL_THREAD);
	lua_setfield(L, -2, "SERIAL_THREAD");
	lua_setfield(L, -2, "barrier");

#define SETCONST(ident) ({ \
	lua_pushinteger(L, PTHREAD_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(DESTRUCTOR_ITERATIONS);
	SETCONST(KEYS_MAX);
	SETCONST(STACK_MIN);
	SETCONST(THREADS_MAX);

	SETCONST(DETACHED);
	SETCONST(SCOPE_SYSTEM);
	SETCONST(INHERIT_SCHED);
	SETCONST(NOFLOAT);

	SETCONST(CREATE_DETACHED);
	SETCONST(CREATE_JOINABLE);
	SETCONST(SCOPE_PROCESS);
	SETCONST(EXPLICIT_SCHED);

	SETCONST(PROCESS_PRIVATE);
	SETCONST(PROCESS_SHARED);

	SETCONST(CANCEL_ENABLE);
	SETCONST(CANCEL_DISABLE);
	SETCONST(CANCEL_DEFERRED);
	SETCONST(CANCEL_ASYNCHRONOUS);

	SETCONST(PRIO_NONE);
	SETCONST(PRIO_INHERIT);
	SETCONST(PRIO_PROTECT);
#undef SETCONST
	return (1);
}
