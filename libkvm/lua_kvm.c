/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/pcpu.h>
#include <sys/resource.h>
#include <sys/user.h>
#include <assert.h>
#include <kvm.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define KVM_METATABLE "kvm_t *"
#define PCPU_METATABLE "struct pcpu *" /* TODO: sys.pcpu module */
#define KINFO_PROC_METATABLE "struct kinfo_proc *" /* TODO: sys.user module */
#define KINFO_PROCS_METATABLE "struct kinfo_proc[]"

int luaopen_kvm(lua_State *);

static inline int
kvmfail(lua_State *L, kvm_t *kd)
{
	luaL_pushfail(L);
	lua_pushstring(L, kvm_geterr(kd));
	return (2);
}

/* The resolver function has no parameter for passing in a Lua state. */
__thread static lua_State *thread_state;

static int
resolver_wrapper(const char *name, kvaddr_t *addr)
{
	lua_State *L = thread_state;
	int result;

	lua_pushvalue(L, -1); /* function on top of stack */
	lua_pushstring(L, name);
	if (lua_pcall(L, 1, 1, 0) == LUA_OK && lua_isinteger(L, -1)) {
		*addr = lua_tointeger(L, -1);
		result = 0;
	} else {
		result = -1;
	}
	lua_pop(L, 1);
	return (result);
}

static int
l_kvm_open2(lua_State *L)
{
	char errbuf[_POSIX2_LINE_MAX];
	const char *execfile, *corefile;
	int (*resolver)(const char *, kvaddr_t *);
	kvm_t *kd;
	int flags;

	execfile = luaL_optstring(L, 1, NULL);
	corefile = luaL_optstring(L, 2, NULL);
	flags = luaL_optinteger(L, 3, 0);
	if (lua_isfunction(L, 4)) {
		resolver = resolver_wrapper;
		thread_state = L;
		luaL_checktype(L, 5, LUA_TNONE);
	} else {
		resolver = NULL;
	}

	if ((kd = kvm_open2(execfile, corefile, flags, errbuf, resolver))
	    == NULL) {
		luaL_pushfail(L);
		lua_pushstring(L, errbuf);
		return (2);
	}
	return (new(L, kd, KVM_METATABLE));
}

static int
l_kvm_openfiles(lua_State *L)
{
	char errbuf[_POSIX2_LINE_MAX];
	const char *execfile, *corefile, *swapfile;
	kvm_t *kd;
	int flags;

	execfile = luaL_optstring(L, 1, NULL);
	corefile = luaL_optstring(L, 2, NULL);
	swapfile = luaL_optstring(L, 3, NULL);
	flags = luaL_optinteger(L, 4, 0);

	if ((kd = kvm_openfiles(execfile, corefile, swapfile, flags, errbuf))
	    == NULL) {
		luaL_pushfail(L);
		lua_pushstring(L, errbuf);
		return (2);
	}
	return (new(L, kd, KVM_METATABLE));
}

static int
l_kvm_close(lua_State *L)
{
	kvm_t *kd;

	kd = checkcookienull(L, 1, KVM_METATABLE);

	if (kd == NULL) {
		return (0);
	}
	if (kvm_close(kd) == -1) {
		return (fail(L, errno));
	}
	setcookie(L, 1, NULL);
	return (success(L));
}

static int
l_kvm_dpcpu_setcpu(lua_State *L)
{
	kvm_t *kd;
	u_int cpu;

	kd = checkcookie(L, 1, KVM_METATABLE);
	cpu = luaL_checkinteger(L, 2);

	if (kvm_dpcpu_setcpu(kd, cpu) == -1) {
		return (kvmfail(L, kd));
	}
	return (success(L));
}

static int
l_kvm_getmaxcpu(lua_State *L)
{
	kvm_t *kd;
	int maxcpu;

	kd = checkcookie(L, 1, KVM_METATABLE);

	if ((maxcpu = kvm_getmaxcpu(kd)) == -1) {
		return (kvmfail(L, kd));
	}
	lua_pushinteger(L, maxcpu);
	return (1);
}

static int
l_kvm_getncpus(lua_State *L)
{
	kvm_t *kd;
	int ncpus;

	kd = checkcookie(L, 1, KVM_METATABLE);

	if ((ncpus = kvm_getncpus(kd)) == -1) {
		return (kvmfail(L, kd));
	}
	lua_pushinteger(L, ncpus);
	return (1);
}

static int
l_kvm_getpcpu(lua_State *L)
{
	kvm_t *kd;
	struct pcpu *pcpu;
	int cpu;

	kd = checkcookie(L, 1, KVM_METATABLE);
	cpu = luaL_checkinteger(L, 2);

	if ((pcpu = kvm_getpcpu(kd, cpu)) == (void *)-1) {
		return (kvmfail(L, kd));
	}
	if (pcpu == NULL) {
		return (0);
	}
	return (new(L, pcpu, PCPU_METATABLE));
}

static int
l_pcpu_gc(lua_State *L)
{
	struct pcpu *pcpu;

	pcpu = checkcookie(L, 1, PCPU_METATABLE);

	free(pcpu);
	setcookie(L, 1, NULL);
	return (0);
}

static int
l_kvm_read_zpcpu(lua_State *L)
{
	luaL_Buffer b;
	kvm_t *kd;
	void *buf;
	u_long base;
	size_t size;
	ssize_t result;
	int cpu;

	kd = checkcookie(L, 1, KVM_METATABLE);
	base = luaL_checkinteger(L, 2);
	size = luaL_checkinteger(L, 3);
	cpu = luaL_checkinteger(L, 4);

	buf = luaL_buffinitsize(L, &b, size);
	if ((result = kvm_read_zpcpu(kd, base, buf, size, cpu)) == -1) {
		return (kvmfail(L, kd));
	}
	luaL_pushresultsize(&b, result);
	return (1);
}

static int
l_kvm_counter_u64_fetch(lua_State *L)
{
	kvm_t *kd;
	u_long base;

	kd = checkcookie(L, 1, KVM_METATABLE);
	base = luaL_checkinteger(L, 2);

	/* Can't discriminate if 0 is an error, just return it. */
	lua_pushinteger(L, kvm_counter_u64_fetch(kd, base));
	return (1);
}

static int
l_kvm_getcptime(lua_State *L)
{
	long cp_time[CPUSTATES]; /* Should this be dynamic for flexibility? */
	kvm_t *kd;

	kd = checkcookie(L, 1, KVM_METATABLE);

	if (kvm_getcptime(kd, cp_time) == -1) {
		return (kvmfail(L, kd));
	}
	lua_createtable(L, CPUSTATES, 0);
	for (int i = 0; i < CPUSTATES; i++) {
		lua_pushinteger(L, cp_time[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_kvm_getcptime_clearcache(lua_State *L __unused)
{
	kvm_getcptime(NULL, NULL);
	return (0);
}

static int
l_kvm_getloadavg(lua_State *L)
{
	double loadavg[3];
	kvm_t *kd;
	int samples;

	kd = checkcookie(L, 1, KVM_METATABLE);

	if ((samples = kvm_getloadavg(kd, loadavg, nitems(loadavg))) == -1) {
		return (kvmfail(L, kd));
	}
	assert(samples <= nitems(loadavg));
	lua_createtable(L, samples, 0);
	for (int i = 0; i < samples; i++) {
		lua_pushnumber(L, loadavg[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

enum { CNT = 1 };

static int
l_kvm_getprocs(lua_State *L)
{
	struct kinfo_proc *procs, *copy;
	kvm_t *kd;
	int op, arg, cnt;

	kd = checkcookie(L, 1, KVM_METATABLE);
	op = luaL_checkinteger(L, 2);
	arg = luaL_checkinteger(L, 3);

	if ((procs = kvm_getprocs(kd, op, arg, &cnt)) == NULL) {
		return (kvmfail(L, kd));
	}
	copy = lua_newuserdatauv(L, cnt * sizeof(*copy), 1);
	memcpy(copy, procs, cnt * sizeof(*copy));
	lua_pushinteger(L, cnt);
	lua_setiuservalue(L, -2, CNT);
	luaL_setmetatable(L, KINFO_PROCS_METATABLE);
	return (1);
}

static int
l_kinfo_procs_index(lua_State *L)
{
	struct kinfo_proc *procs;
	size_t i, cnt;

	procs = luaL_checkudata(L, 1, KINFO_PROCS_METATABLE);
	i = luaL_checkinteger(L, 2);

	lua_getiuservalue(L, 1, CNT);
	cnt = lua_tointeger(L, -1);
	if (i == 0 || i > cnt) {
		return (0);
	}
	return (newref(L, 1, &procs[i - 1], KINFO_PROC_METATABLE));
}

static int
l_kinfo_procs_len(lua_State *L)
{
	luaL_checkudata(L, 1, KINFO_PROCS_METATABLE);

	lua_getiuservalue(L, 1, CNT);
	return (1);
}

static int
l_kvm_getargv(lua_State *L)
{
	kvm_t *kd;
	const struct kinfo_proc *p;
	char **argv;
	int nchr;

	kd = checkcookie(L, 1, KVM_METATABLE);
	p = checkcookie(L, 2, KINFO_PROC_METATABLE);
	nchr = luaL_optinteger(L, 3, 0);

	if ((argv = kvm_getargv(kd, p, nchr)) == NULL) {
		return (kvmfail(L, kd));
	}
	lua_newtable(L);
	for (int i = 0; argv[i] != NULL; i++) {
		lua_pushstring(L, argv[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static int
l_kvm_getenvv(lua_State *L)
{
	kvm_t *kd;
	const struct kinfo_proc *p;
	char **envv;
	int nchr;

	kd = checkcookie(L, 1, KVM_METATABLE);
	p = checkcookie(L, 2, KINFO_PROC_METATABLE);
	nchr = luaL_optinteger(L, 3, 0);

	if ((envv = kvm_getenvv(kd, p, nchr)) == NULL) {
		return (kvmfail(L, kd));
	}
	lua_newtable(L);
	for (int i = 0; envv[i] != NULL; i++) {
		lua_pushstring(L, envv[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return (1);
}

static inline void
pushswap(lua_State *L, struct kvm_swap *swap)
{
	lua_newtable(L);
	lua_pushstring(L, swap->ksw_devname);
	lua_setfield(L, -2, "devname");
#define IFIELD(name) ({ \
	lua_pushinteger(L, swap->ksw_ ## name); \
	lua_setfield(L, -2, #name); \
})
	IFIELD(total);
	IFIELD(used);
	IFIELD(flags);
#undef IFIELD
}

static int
l_kvm_getswapinfo(lua_State *L)
{
	kvm_t *kd;
	struct kvm_swap *ksi;
	int maxswap, flags, nswap;

	kd = checkcookie(L, 1, KVM_METATABLE);
	maxswap = 1 + luaL_optinteger(L, 2, 0); /* XXX: deviates from C API */
	flags = luaL_optinteger(L, 3, 0);

	if ((ksi = malloc(maxswap * sizeof(*ksi))) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if ((nswap = kvm_getswapinfo(kd, ksi, maxswap, flags)) == -1) {
		free(ksi);
		return (kvmfail(L, kd));
	}
	pushswap(L, &ksi[nswap - 1]);
	lua_createtable(L, nswap - 1, 0);
	for (int i = 0; i < nswap - 1; i++) {
		pushswap(L, &ksi[i]);
		lua_rawseti(L, -2, i + 1);
	}
	free(ksi);
	return (2);
}

static int
l_kvm_getswapinfo_clearcache(lua_State *L __unused)
{
	kvm_getswapinfo(NULL, NULL, 0, 0);
	return (0);
}

static int
l_kvm_native(lua_State *L)
{
	kvm_t *kd;

	kd = checkcookie(L, 1, KVM_METATABLE);

	lua_pushboolean(L, kvm_native(kd));
	return (1);
}

static inline void
pushnlist(lua_State *L, const struct kvm_nlist *n)
{
	lua_newtable(L);
	lua_pushstring(L, n->n_name);
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, n->n_type);
	lua_setfield(L, -2, "type");
	lua_pushinteger(L, n->n_value);
	lua_setfield(L, -2, "value");
}

static int
l_kvm_nlist2(lua_State *L)
{
	kvm_t *kd;
	struct kvm_nlist *nl;
	size_t n;
	int ninvalid;

	kd = checkcookie(L, 1, KVM_METATABLE);
	luaL_checktype(L, 2, LUA_TTABLE);

	n = luaL_len(L, 2);
	if ((nl = calloc(n + 1, sizeof(*nl))) == NULL) {
		return (fatal(L, "calloc", ENOMEM));
	}
	for (size_t i = 0; i < n; i++) {
		if (lua_geti(L, 2, i + 1) != LUA_TSTRING) {
			free(nl);
			return (luaL_argerror(L, 2, "expected strings"));
		}
		nl[i].n_name = lua_tostring(L, -1);
	}
	if ((ninvalid = kvm_nlist2(kd, nl)) == -1) {
		free(nl);
		return (kvmfail(L, kd));
	}
	lua_createtable(L, n, 0);
	for (size_t i = 0; i < n; i++) {
		if (nl[i].n_value == 0) {
			continue;
		}
		pushnlist(L, &nl[i]);
		lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	}
	free(nl);
	lua_pushinteger(L, ninvalid);
	return (2);
}

static int
l_kvm_read2(lua_State *L)
{
	luaL_Buffer b;
	kvm_t *kd;
	kvaddr_t addr;
	void *buf;
	size_t nbytes;
	ssize_t n;

	kd = checkcookie(L, 1, KVM_METATABLE);
	addr = luaL_checkinteger(L, 2);
	nbytes = luaL_checkinteger(L, 3);

	buf = luaL_buffinitsize(L, &b, nbytes);
	if ((n = kvm_read2(kd, addr, buf, nbytes)) == -1) {
		return (kvmfail(L, kd));
	}
	luaL_pushresultsize(&b, n);
	return (1);
}

static int
l_kvm_write(lua_State *L)
{
	kvm_t *kd;
	unsigned long addr;
	const void *buf;
	size_t nbytes;
	ssize_t n;

	kd = checkcookie(L, 1, KVM_METATABLE);
	addr = luaL_checkinteger(L, 2);
	buf = luaL_checklstring(L, 3, &nbytes);

	if ((n = kvm_write(kd, addr, buf, nbytes)) == -1) {
		return (kvmfail(L, kd));
	}
	lua_pushinteger(L, n);
	return (1);
}

static int
l_kvm_kerndisp(lua_State *L)
{
	kvm_t *kd;

	kd = checkcookie(L, 1, KVM_METATABLE);

	lua_pushinteger(L, kvm_kerndisp(kd));
	return (1);
}

static inline void
pushpage(lua_State *L, const struct kvm_page *kp)
{
	lua_newtable(L);
#define IFIELD(name) ({ \
	lua_pushinteger(L, kp->kp_ ## name); \
	lua_setfield(L, -2, #name); \
})
	IFIELD(version);
	IFIELD(paddr);
	IFIELD(kmap_vaddr);
	IFIELD(dmap_vaddr);
	IFIELD(prot);
	IFIELD(offset);
	IFIELD(len);
#undef IFIELD
}

static int
walk_cb_wrapper(struct kvm_page *kp, void *closure)
{
	lua_State *L = closure;
	int result;

	lua_pushvalue(L, -1); /* ..., cb, cb */
	pushpage(L, kp); /* ..., cb, cb, page */
	if (lua_pcall(L, 1, 1, 0) == LUA_OK && lua_toboolean(L, -1)) {
		result = 1;
	} else {
		result = 0;
	}
	lua_pop(L, 1);
	return (result);
}

static int
l_kvm_walk_pages(lua_State *L)
{
	kvm_t *kd;

	kd = checkcookie(L, 1, KVM_METATABLE);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	luaL_checktype(L, 3, LUA_TNONE);

	/* kvm_walk_pages doesn't set any error info of its own. */
	lua_pushboolean(L, kvm_walk_pages(kd, walk_cb_wrapper, L));
	return (1);
}

static const struct luaL_Reg l_kvm_funcs[] = {
	{"open2", l_kvm_open2},
	{"openfiles", l_kvm_openfiles},
	{"getcptime_clearcache", l_kvm_getcptime_clearcache},
	{"getswapinfo_clearcache", l_kvm_getswapinfo_clearcache},
	{NULL, NULL}
};

static const struct luaL_Reg l_kvm_meta[] = {
	{"__close", l_kvm_close},
	{"__gc", l_kvm_close},
	{"close", l_kvm_close},
	{"dpcpu_setcpu", l_kvm_dpcpu_setcpu},
	{"getmaxcpu", l_kvm_getmaxcpu},
	{"getncpus", l_kvm_getncpus},
	{"getpcpu", l_kvm_getpcpu},
	{"read_zpcpu", l_kvm_read_zpcpu},
	{"counter_u64_fetch", l_kvm_counter_u64_fetch},
	{"getcptime", l_kvm_getcptime},
	{"getloadavg", l_kvm_getloadavg},
	{"getargv", l_kvm_getargv},
	{"getenvv", l_kvm_getenvv},
	{"getswapinfo", l_kvm_getswapinfo},
	{"native", l_kvm_native},
	/* nlist omitted, use nlist2 */
	{"nlist2", l_kvm_nlist2},
	/* read omitted, use read2 */
	{"read2", l_kvm_read2},
	{"write", l_kvm_write},
	{"kerndisp", l_kvm_kerndisp},
	{"walk_pages", l_kvm_walk_pages},
	{NULL, NULL}
};

static const struct luaL_Reg l_pcpu_meta[] = {
	{"__gc", l_pcpu_gc},
	/* TODO */
	{NULL, NULL}
};

static const struct luaL_Reg l_kinfo_proc_meta[] = {
	/* TODO */
	{NULL, NULL}
};

static const struct luaL_Reg l_kinfo_procs_meta[] = {
	{"__index", l_kinfo_procs_index},
	{"__len", l_kinfo_procs_len},
	{NULL, NULL}
};

int
luaopen_kvm(lua_State *L)
{
	luaL_newmetatable(L, KVM_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_kvm_meta, 0);

	luaL_newmetatable(L, PCPU_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_pcpu_meta, 0);

	luaL_newmetatable(L, KINFO_PROC_METATABLE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, l_kinfo_proc_meta, 0);

	luaL_newmetatable(L, KINFO_PROCS_METATABLE);
	luaL_setfuncs(L, l_kinfo_procs_meta, 0);

	luaL_newlib(L, l_kvm_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(SWIF_DEV_PREFIX);
	DEFINE(LIBKVM_WALK_PAGES_VERSION);
#undef DEFINE
	return (1);
}
