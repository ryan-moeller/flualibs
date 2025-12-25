/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

int luaopen_unistd(lua_State *);

static int
l_confstr(lua_State *L)
{
	char *value;
	size_t len, newlen;
	int name;

	name = luaL_checkinteger(L, 1);

	value = NULL;
	len = 0;
	errno = 0;
	while ((newlen = confstr(name, value, len)) > len) {
		free(value);
		len = newlen * 2;
		if ((value = malloc(len)) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	if (newlen == 0) {
		int error = errno;

		free(value);
		if (error == 0) {
			return (0);
		}
		return (fail(L, error));
	}
	lua_pushlstring(L, value, newlen);
	free(value);
	return (1);
}


static int
l_copy_file_range(lua_State *L)
{
	off_t *inoffp, *outoffp;
	off_t inoff, outoff;
	size_t len;
	ssize_t copied;
	unsigned int flags;
	int infd, outfd;

	infd = checkfd(L, 1);
	if (lua_isnil(L, 2)) {
		inoffp = NULL;
	} else {
		inoff = luaL_checkinteger(L, 2);
		inoffp = &inoff;
	}
	outfd = checkfd(L, 3);
	if (lua_isnil(L, 4)) {
		outoffp = NULL;
	} else {
		outoff = luaL_checkinteger(L, 4);
		outoffp = &outoff;
	}
	len = luaL_checkinteger(L, 5);
	flags = luaL_checkinteger(L, 6);

	if ((copied = copy_file_range(infd, inoffp, outfd, outoffp, len, flags))
	    == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, copied);
	if (inoffp == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushinteger(L, inoff);
	}
	if (outoffp == NULL) {
		lua_pushnil(L);
	} else {
		lua_pushinteger(L, outoff);
	}
	return (3);
}

static int
l_pathconf(lua_State *L)
{
	const char *path;
	long value;
	int name;

	path = luaL_checkstring(L, 1);
	name = luaL_checkinteger(L, 2);

	errno = 0;
	if ((value = pathconf(path, name)) == -1) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_lpathconf(lua_State *L)
{
	const char *path;
	long value;
	int name;

	path = luaL_checkstring(L, 1);
	name = luaL_checkinteger(L, 2);

	errno = 0;
	if ((value = lpathconf(path, name)) == -1) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_fpathconf(lua_State *L)
{
	long value;
	int fd, name;

	fd = checkfd(L, 1);
	name = luaL_checkinteger(L, 2);

	errno = 0;
	if ((value = fpathconf(fd, name)) == -1) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_sysconf(lua_State *L)
{
	long value;
	int name;

	name = luaL_checkinteger(L, 1);

	errno = 0;
	if ((value = sysconf(name)) == -1) {
		if (errno == 0) {
			return (0);
		}
		return (fail(L, errno));
	}
	lua_pushinteger(L, value);
	return (1);
}

static int
l_truncate(lua_State *L)
{
	const char *path;
	off_t length;

	path = luaL_checkstring(L, 1);
	length = luaL_checkinteger(L, 2);

	if (truncate(path, length) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static int
l_ftruncate(lua_State *L)
{
	off_t length;
	int fd;

	fd = checkfd(L, 1);
	length = luaL_checkinteger(L, 2);

	if (ftruncate(fd, length) == -1) {
		return (fail(L, errno));
	}
	lua_pushboolean(L, true);
	return (1);
}

static const struct luaL_Reg l_unistd_funcs[] = {
	{"confstr", l_confstr},
	{"copy_file_range", l_copy_file_range},
	{"pathconf", l_pathconf},
	{"lpathconf", l_lpathconf},
	{"fpathconf", l_fpathconf},
	{"sysconf", l_sysconf},
	{"truncate", l_truncate},
	{"ftruncate", l_ftruncate},
	{NULL, NULL}
};

int
luaopen_unistd(lua_State *L)
{
	luaL_newlib(L, l_unistd_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(_CS_PATH);
	DEFINE(_CS_POSIX_V6_ILP32_OFF32_CFLAGS);
	DEFINE(_CS_POSIX_V6_ILP32_OFF32_LDFLAGS);
	DEFINE(_CS_POSIX_V6_ILP32_OFF32_LIBS);
	DEFINE(_CS_POSIX_V6_ILP32_OFFBIG_CFLAGS);
	DEFINE(_CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS);
	DEFINE(_CS_POSIX_V6_ILP32_OFFBIG_LIBS);
	DEFINE(_CS_POSIX_V6_LP64_OFF64_CFLAGS);
	DEFINE(_CS_POSIX_V6_LP64_OFF64_LDFLAGS);
	DEFINE(_CS_POSIX_V6_LP64_OFF64_LIBS);
	DEFINE(_CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS);
	DEFINE(_CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS);
	DEFINE(_CS_POSIX_V6_LPBIG_OFFBIG_LIBS);
	DEFINE(_CS_POSIX_V6_WIDTH_RESTRICTED_ENVS);
#ifdef COPY_FILE_RANGE_CLONE
	DEFINE(COPY_FILE_RANGE_CLONE);
#endif
	DEFINE(_PC_LINK_MAX);
	DEFINE(_PC_MAX_CANON);
	DEFINE(_PC_MAX_INPUT);
	DEFINE(_PC_NAME_MAX);
	DEFINE(_PC_PATH_MAX);
	DEFINE(_PC_PIPE_BUF);
	DEFINE(_PC_CHOWN_RESTRICTED);
	DEFINE(_PC_NO_TRUNC);
	DEFINE(_PC_VDISABLE);
	DEFINE(_PC_ASYNC_IO);
	DEFINE(_PC_PRIO_IO);
	DEFINE(_PC_SYNC_IO);
	DEFINE(_PC_ALLOC_SIZE_MIN);
	DEFINE(_PC_FILESIZEBITS);
	DEFINE(_PC_REC_INCR_XFER_SIZE);
	DEFINE(_PC_REC_MAX_XFER_SIZE);
	DEFINE(_PC_REC_MIN_XFER_SIZE);
	DEFINE(_PC_REC_XFER_ALIGN);
	DEFINE(_PC_SYMLINK_MAX);
	DEFINE(_PC_ACL_EXTENDED);
	DEFINE(_PC_ACL_PATH_MAX);
	DEFINE(_PC_CAP_PRESENT);
	DEFINE(_PC_INF_PRESENT);
	DEFINE(_PC_MAC_PRESENT);
	DEFINE(_PC_ACL_NFS4);
#ifdef _PC_DEALLOC_PRESENT
	DEFINE(_PC_DEALLOC_PRESENT);
#endif
#ifdef _PC_NAMEDATTR_ENABLED
	DEFINE(_PC_NAMEDATTR_ENABLED);
#endif
#ifdef _PC_HAS_NAMEDATTR
	DEFINE(_PC_HAS_NAMEDATTR);
#endif
#ifdef _PC_HAS_HIDDENSYSTEM
	DEFINE(_PC_HAS_HIDDENSYSTEM);
#endif
#ifdef _PC_CLONE_BLKSIZE
	DEFINE(_PC_CLONE_BLKSIZE);
#endif
#ifdef _PC_CASE_INSENSITIVE
	DEFINE(_PC_CASE_INSENSITIVE);
#endif
	DEFINE(_PC_MIN_HOLE_SIZE);
	DEFINE(_SC_ARG_MAX);
	DEFINE(_SC_CHILD_MAX);
	DEFINE(_SC_CLK_TCK);
	DEFINE(_SC_NGROUPS_MAX);
	DEFINE(_SC_OPEN_MAX);
	DEFINE(_SC_JOB_CONTROL);
	DEFINE(_SC_SAVED_IDS);
	DEFINE(_SC_VERSION);
	DEFINE(_SC_BC_BASE_MAX);
	DEFINE(_SC_BC_DIM_MAX);
	DEFINE(_SC_BC_SCALE_MAX);
	DEFINE(_SC_BC_STRING_MAX);
	DEFINE(_SC_COLL_WEIGHTS_MAX);
	DEFINE(_SC_EXPR_NEST_MAX);
	DEFINE(_SC_LINE_MAX);
	DEFINE(_SC_RE_DUP_MAX);
	DEFINE(_SC_2_VERSION);
	DEFINE(_SC_2_C_BIND);
	DEFINE(_SC_2_C_DEV);
	DEFINE(_SC_2_CHAR_TERM);
	DEFINE(_SC_2_FORT_DEV);
	DEFINE(_SC_2_FORT_RUN);
	DEFINE(_SC_2_LOCALEDEF);
	DEFINE(_SC_2_SW_DEV);
	DEFINE(_SC_2_UPE);
	DEFINE(_SC_STREAM_MAX);
	DEFINE(_SC_TZNAME_MAX);
	DEFINE(_SC_ASYNCHRONOUS_IO);
	DEFINE(_SC_MAPPED_FILES);
	DEFINE(_SC_MEMLOCK);
	DEFINE(_SC_MEMLOCK_RANGE);
	DEFINE(_SC_MEMORY_PROTECTION);
	DEFINE(_SC_MESSAGE_PASSING);
	DEFINE(_SC_PRIORITIZED_IO);
	DEFINE(_SC_PRIORITY_SCHEDULING);
	DEFINE(_SC_REALTIME_SIGNALS);
	DEFINE(_SC_SEMAPHORES);
	DEFINE(_SC_FSYNC);
	DEFINE(_SC_SHARED_MEMORY_OBJECTS);
	DEFINE(_SC_SYNCHRONIZED_IO);
	DEFINE(_SC_TIMERS);
	DEFINE(_SC_AIO_LISTIO_MAX);
	DEFINE(_SC_AIO_MAX);
	DEFINE(_SC_AIO_PRIO_DELTA_MAX);
	DEFINE(_SC_DELAYTIMER_MAX);
	DEFINE(_SC_MQ_OPEN_MAX);
	DEFINE(_SC_PAGESIZE);
	DEFINE(_SC_RTSIG_MAX);
	DEFINE(_SC_SEM_NSEMS_MAX);
	DEFINE(_SC_SEM_VALUE_MAX);
	DEFINE(_SC_SIGQUEUE_MAX);
	DEFINE(_SC_TIMER_MAX);
	DEFINE(_SC_2_PBS);
	DEFINE(_SC_2_PBS_ACCOUNTING);
	DEFINE(_SC_2_PBS_CHECKPOINT);
	DEFINE(_SC_2_PBS_LOCATE);
	DEFINE(_SC_2_PBS_MESSAGE);
	DEFINE(_SC_2_PBS_TRACK);
	DEFINE(_SC_ADVISORY_INFO);
	DEFINE(_SC_BARRIERS);
	DEFINE(_SC_CLOCK_SELECTION);
	DEFINE(_SC_CPUTIME);
	DEFINE(_SC_FILE_LOCKING);
	DEFINE(_SC_GETGR_R_SIZE_MAX);
	DEFINE(_SC_GETPW_R_SIZE_MAX);
	DEFINE(_SC_HOST_NAME_MAX);
	DEFINE(_SC_LOGIN_NAME_MAX);
	DEFINE(_SC_MONOTONIC_CLOCK);
	DEFINE(_SC_MQ_PRIO_MAX);
	DEFINE(_SC_READER_WRITER_LOCKS);
	DEFINE(_SC_REGEXP);
	DEFINE(_SC_SHELL);
	DEFINE(_SC_SPAWN);
	DEFINE(_SC_SPIN_LOCKS);
	DEFINE(_SC_SPORADIC_SERVER);
	DEFINE(_SC_THREAD_ATTR_STACKADDR);
	DEFINE(_SC_THREAD_ATTR_STACKSIZE);
	DEFINE(_SC_THREAD_CPUTIME);
	DEFINE(_SC_THREAD_DESTRUCTOR_ITERATIONS);
	DEFINE(_SC_THREAD_KEYS_MAX);
	DEFINE(_SC_THREAD_PRIO_INHERIT);
	DEFINE(_SC_THREAD_PRIO_PROTECT);
	DEFINE(_SC_THREAD_PRIORITY_SCHEDULING);
	DEFINE(_SC_THREAD_PROCESS_SHARED);
	DEFINE(_SC_THREAD_SAFE_FUNCTIONS);
	DEFINE(_SC_THREAD_SPORADIC_SERVER);
	DEFINE(_SC_THREAD_STACK_MIN);
	DEFINE(_SC_THREAD_THREADS_MAX);
	DEFINE(_SC_TIMEOUTS);
	DEFINE(_SC_THREADS);
	DEFINE(_SC_TRACE);
	DEFINE(_SC_TRACE_EVENT_FILTER);
	DEFINE(_SC_TRACE_INHERIT);
	DEFINE(_SC_TRACE_LOG);
	DEFINE(_SC_TTY_NAME_MAX);
	DEFINE(_SC_TYPED_MEMORY_OBJECTS);
	DEFINE(_SC_V6_ILP32_OFF32);
	DEFINE(_SC_V6_ILP32_OFFBIG);
	DEFINE(_SC_V6_LP64_OFF64);
	DEFINE(_SC_V6_LPBIG_OFFBIG);
	DEFINE(_SC_IPV6);
	DEFINE(_SC_RAW_SOCKETS);
	DEFINE(_SC_SYMLOOP_MAX);
	DEFINE(_SC_ATEXIT_MAX);
	DEFINE(_SC_IOV_MAX);
	DEFINE(_SC_PAGE_SIZE);
	DEFINE(_SC_XOPEN_CRYPT);
	DEFINE(_SC_XOPEN_ENH_I18N);
	DEFINE(_SC_XOPEN_LEGACY);
	DEFINE(_SC_XOPEN_REALTIME);
	DEFINE(_SC_XOPEN_REALTIME_THREADS);
	DEFINE(_SC_XOPEN_SHM);
	DEFINE(_SC_XOPEN_STREAMS);
	DEFINE(_SC_XOPEN_UNIX);
	DEFINE(_SC_XOPEN_VERSION);
	DEFINE(_SC_XOPEN_XCU_VERSION);
	DEFINE(_SC_NPROCESSORS_CONF);
	DEFINE(_SC_NPROCESSORS_ONLN);
	DEFINE(_SC_CPUSET_SIZE);
#ifdef _SC_UEXTERR_MAXLEN
	DEFINE(_SC_UEXTERR_MAXLEN);
#endif
#ifdef _SC_NSIG
	DEFINE(_SC_NSIG);
#endif
	DEFINE(_SC_PHYS_PAGES);
	DEFINE(STDIN_FILENO);
	DEFINE(STDOUT_FILENO);
	DEFINE(STDERR_FILENO);
	DEFINE(SSIZE_MAX); /* TODO: limits module? */
#undef DEFINE
	return (1);
}
