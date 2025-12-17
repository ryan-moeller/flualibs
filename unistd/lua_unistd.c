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
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(_CS_PATH);
	SETCONST(_CS_POSIX_V6_ILP32_OFF32_CFLAGS);
	SETCONST(_CS_POSIX_V6_ILP32_OFF32_LDFLAGS);
	SETCONST(_CS_POSIX_V6_ILP32_OFF32_LIBS);
	SETCONST(_CS_POSIX_V6_ILP32_OFFBIG_CFLAGS);
	SETCONST(_CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS);
	SETCONST(_CS_POSIX_V6_ILP32_OFFBIG_LIBS);
	SETCONST(_CS_POSIX_V6_LP64_OFF64_CFLAGS);
	SETCONST(_CS_POSIX_V6_LP64_OFF64_LDFLAGS);
	SETCONST(_CS_POSIX_V6_LP64_OFF64_LIBS);
	SETCONST(_CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS);
	SETCONST(_CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS);
	SETCONST(_CS_POSIX_V6_LPBIG_OFFBIG_LIBS);
	SETCONST(_CS_POSIX_V6_WIDTH_RESTRICTED_ENVS);
#ifdef COPY_FILE_RANGE_CLONE
	SETCONST(COPY_FILE_RANGE_CLONE);
#endif
	SETCONST(_PC_LINK_MAX);
	SETCONST(_PC_MAX_CANON);
	SETCONST(_PC_MAX_INPUT);
	SETCONST(_PC_NAME_MAX);
	SETCONST(_PC_PATH_MAX);
	SETCONST(_PC_PIPE_BUF);
	SETCONST(_PC_CHOWN_RESTRICTED);
	SETCONST(_PC_NO_TRUNC);
	SETCONST(_PC_VDISABLE);
	SETCONST(_PC_ASYNC_IO);
	SETCONST(_PC_PRIO_IO);
	SETCONST(_PC_SYNC_IO);
	SETCONST(_PC_ALLOC_SIZE_MIN);
	SETCONST(_PC_FILESIZEBITS);
	SETCONST(_PC_REC_INCR_XFER_SIZE);
	SETCONST(_PC_REC_MAX_XFER_SIZE);
	SETCONST(_PC_REC_MIN_XFER_SIZE);
	SETCONST(_PC_REC_XFER_ALIGN);
	SETCONST(_PC_SYMLINK_MAX);
	SETCONST(_PC_ACL_EXTENDED);
	SETCONST(_PC_ACL_PATH_MAX);
	SETCONST(_PC_CAP_PRESENT);
	SETCONST(_PC_INF_PRESENT);
	SETCONST(_PC_MAC_PRESENT);
	SETCONST(_PC_ACL_NFS4);
#ifdef _PC_DEALLOC_PRESENT
	SETCONST(_PC_DEALLOC_PRESENT);
#endif
#ifdef _PC_NAMEDATTR_ENABLED
	SETCONST(_PC_NAMEDATTR_ENABLED);
#endif
#ifdef _PC_HAS_NAMEDATTR
	SETCONST(_PC_HAS_NAMEDATTR);
#endif
#ifdef _PC_HAS_HIDDENSYSTEM
	SETCONST(_PC_HAS_HIDDENSYSTEM);
#endif
#ifdef _PC_CLONE_BLKSIZE
	SETCONST(_PC_CLONE_BLKSIZE);
#endif
#ifdef _PC_CASE_INSENSITIVE
	SETCONST(_PC_CASE_INSENSITIVE);
#endif
	SETCONST(_PC_MIN_HOLE_SIZE);
	SETCONST(_SC_ARG_MAX);
	SETCONST(_SC_CHILD_MAX);
	SETCONST(_SC_CLK_TCK);
	SETCONST(_SC_NGROUPS_MAX);
	SETCONST(_SC_OPEN_MAX);
	SETCONST(_SC_JOB_CONTROL);
	SETCONST(_SC_SAVED_IDS);
	SETCONST(_SC_VERSION);
	SETCONST(_SC_BC_BASE_MAX);
	SETCONST(_SC_BC_DIM_MAX);
	SETCONST(_SC_BC_SCALE_MAX);
	SETCONST(_SC_BC_STRING_MAX);
	SETCONST(_SC_COLL_WEIGHTS_MAX);
	SETCONST(_SC_EXPR_NEST_MAX);
	SETCONST(_SC_LINE_MAX);
	SETCONST(_SC_RE_DUP_MAX);
	SETCONST(_SC_2_VERSION);
	SETCONST(_SC_2_C_BIND);
	SETCONST(_SC_2_C_DEV);
	SETCONST(_SC_2_CHAR_TERM);
	SETCONST(_SC_2_FORT_DEV);
	SETCONST(_SC_2_FORT_RUN);
	SETCONST(_SC_2_LOCALEDEF);
	SETCONST(_SC_2_SW_DEV);
	SETCONST(_SC_2_UPE);
	SETCONST(_SC_STREAM_MAX);
	SETCONST(_SC_TZNAME_MAX);
	SETCONST(_SC_ASYNCHRONOUS_IO);
	SETCONST(_SC_MAPPED_FILES);
	SETCONST(_SC_MEMLOCK);
	SETCONST(_SC_MEMLOCK_RANGE);
	SETCONST(_SC_MEMORY_PROTECTION);
	SETCONST(_SC_MESSAGE_PASSING);
	SETCONST(_SC_PRIORITIZED_IO);
	SETCONST(_SC_PRIORITY_SCHEDULING);
	SETCONST(_SC_REALTIME_SIGNALS);
	SETCONST(_SC_SEMAPHORES);
	SETCONST(_SC_FSYNC);
	SETCONST(_SC_SHARED_MEMORY_OBJECTS);
	SETCONST(_SC_SYNCHRONIZED_IO);
	SETCONST(_SC_TIMERS);
	SETCONST(_SC_AIO_LISTIO_MAX);
	SETCONST(_SC_AIO_MAX);
	SETCONST(_SC_AIO_PRIO_DELTA_MAX);
	SETCONST(_SC_DELAYTIMER_MAX);
	SETCONST(_SC_MQ_OPEN_MAX);
	SETCONST(_SC_PAGESIZE);
	SETCONST(_SC_RTSIG_MAX);
	SETCONST(_SC_SEM_NSEMS_MAX);
	SETCONST(_SC_SEM_VALUE_MAX);
	SETCONST(_SC_SIGQUEUE_MAX);
	SETCONST(_SC_TIMER_MAX);
	SETCONST(_SC_2_PBS);
	SETCONST(_SC_2_PBS_ACCOUNTING);
	SETCONST(_SC_2_PBS_CHECKPOINT);
	SETCONST(_SC_2_PBS_LOCATE);
	SETCONST(_SC_2_PBS_MESSAGE);
	SETCONST(_SC_2_PBS_TRACK);
	SETCONST(_SC_ADVISORY_INFO);
	SETCONST(_SC_BARRIERS);
	SETCONST(_SC_CLOCK_SELECTION);
	SETCONST(_SC_CPUTIME);
	SETCONST(_SC_FILE_LOCKING);
	SETCONST(_SC_GETGR_R_SIZE_MAX);
	SETCONST(_SC_GETPW_R_SIZE_MAX);
	SETCONST(_SC_HOST_NAME_MAX);
	SETCONST(_SC_LOGIN_NAME_MAX);
	SETCONST(_SC_MONOTONIC_CLOCK);
	SETCONST(_SC_MQ_PRIO_MAX);
	SETCONST(_SC_READER_WRITER_LOCKS);
	SETCONST(_SC_REGEXP);
	SETCONST(_SC_SHELL);
	SETCONST(_SC_SPAWN);
	SETCONST(_SC_SPIN_LOCKS);
	SETCONST(_SC_SPORADIC_SERVER);
	SETCONST(_SC_THREAD_ATTR_STACKADDR);
	SETCONST(_SC_THREAD_ATTR_STACKSIZE);
	SETCONST(_SC_THREAD_CPUTIME);
	SETCONST(_SC_THREAD_DESTRUCTOR_ITERATIONS);
	SETCONST(_SC_THREAD_KEYS_MAX);
	SETCONST(_SC_THREAD_PRIO_INHERIT);
	SETCONST(_SC_THREAD_PRIO_PROTECT);
	SETCONST(_SC_THREAD_PRIORITY_SCHEDULING);
	SETCONST(_SC_THREAD_PROCESS_SHARED);
	SETCONST(_SC_THREAD_SAFE_FUNCTIONS);
	SETCONST(_SC_THREAD_SPORADIC_SERVER);
	SETCONST(_SC_THREAD_STACK_MIN);
	SETCONST(_SC_THREAD_THREADS_MAX);
	SETCONST(_SC_TIMEOUTS);
	SETCONST(_SC_THREADS);
	SETCONST(_SC_TRACE);
	SETCONST(_SC_TRACE_EVENT_FILTER);
	SETCONST(_SC_TRACE_INHERIT);
	SETCONST(_SC_TRACE_LOG);
	SETCONST(_SC_TTY_NAME_MAX);
	SETCONST(_SC_TYPED_MEMORY_OBJECTS);
	SETCONST(_SC_V6_ILP32_OFF32);
	SETCONST(_SC_V6_ILP32_OFFBIG);
	SETCONST(_SC_V6_LP64_OFF64);
	SETCONST(_SC_V6_LPBIG_OFFBIG);
	SETCONST(_SC_IPV6);
	SETCONST(_SC_RAW_SOCKETS);
	SETCONST(_SC_SYMLOOP_MAX);
	SETCONST(_SC_ATEXIT_MAX);
	SETCONST(_SC_IOV_MAX);
	SETCONST(_SC_PAGE_SIZE);
	SETCONST(_SC_XOPEN_CRYPT);
	SETCONST(_SC_XOPEN_ENH_I18N);
	SETCONST(_SC_XOPEN_LEGACY);
	SETCONST(_SC_XOPEN_REALTIME);
	SETCONST(_SC_XOPEN_REALTIME_THREADS);
	SETCONST(_SC_XOPEN_SHM);
	SETCONST(_SC_XOPEN_STREAMS);
	SETCONST(_SC_XOPEN_UNIX);
	SETCONST(_SC_XOPEN_VERSION);
	SETCONST(_SC_XOPEN_XCU_VERSION);
	SETCONST(_SC_NPROCESSORS_CONF);
	SETCONST(_SC_NPROCESSORS_ONLN);
	SETCONST(_SC_CPUSET_SIZE);
#ifdef _SC_UEXTERR_MAXLEN
	SETCONST(_SC_UEXTERR_MAXLEN);
#endif
#ifdef _SC_NSIG
	SETCONST(_SC_NSIG);
#endif
	SETCONST(_SC_PHYS_PAGES);
	SETCONST(STDIN_FILENO);
	SETCONST(STDOUT_FILENO);
	SETCONST(STDERR_FILENO);
	SETCONST(SSIZE_MAX); /* TODO: limits module? */
#undef SETCONST
	return (1);
}
