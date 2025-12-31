/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <fcntl.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

int luaopen_fcntl(lua_State *);

static int
l_open(lua_State *L)
{
	const char *path;
	int flags, fd;

	path = luaL_checkstring(L, 1);
	flags = luaL_checkinteger(L, 2);

	if ((flags & O_CREAT) != 0) {
		mode_t mode;

	mode = luaL_checkinteger(L, 3);

		fd = open(path, flags, mode);
	} else {
		fd = open(path, flags);
	}
	if (fd == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fd);
	return (1);
}

#define FLOCK_FIELDS(X) \
	X(start) \
	X(len) \
	X(pid) \
	X(type) \
	X(whence) \
	X(sysid)

static inline void
checkflock(lua_State *L, int idx, struct flock *fl)
{
	luaL_checktype(L, idx, LUA_TTABLE);

	memset(fl, 0, sizeof(*fl));
#define FIELD(name) \
	if (lua_getfield(L, idx, #name) != LUA_TNIL) { \
		if (lua_isinteger(L, -1)) { \
			fl->l_##name = lua_tointeger(L, -1); \
		} else { \
			luaL_argerror(L, idx, "invalid flock "#name); \
		} \
	}
	FLOCK_FIELDS(FIELD)
#undef FIELD
	lua_pop(L, 6);
}

static inline void
pushflock(lua_State *L, const struct flock *fl)
{
	lua_newtable(L);
#define FIELD(name) \
	lua_pushinteger(L, fl->l_##name); \
	lua_setfield(L, -2, #name);
	FLOCK_FIELDS(FIELD)
#undef FIELD
}

static int
l_fcntl(lua_State *L)
{
	int fd, cmd, result;

	fd = checkfd(L, 1);
	cmd = luaL_checkinteger(L, 2);

	switch (cmd) {
#ifdef F_DUP3FD
	case F_DUP3FD:
		return (luaL_error(L, "dup3 fcntl cmd not supported"));
#endif
	case F_OGETLK:
	case F_OSETLK:
	case F_OSETLKW:
		return (luaL_error(L, "old lock fcntl cmds not supported"));
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
#ifdef F_DUPFD_CLOFORK
	case F_DUPFD_CLOFORK:
#endif
	case F_DUP2FD:
	case F_DUP2FD_CLOEXEC: {
		int arg;

		arg = checkfd(L, 3);

		if ((result = fcntl(fd, cmd, arg)) == -1) {
			return (fail(L, errno));
		}
		lua_pushinteger(L, result);
		return (1);
	}
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
	case F_SETLK_REMOTE: {
		struct flock fl;

		checkflock(L, 3, &fl);

		if ((result = fcntl(fd, cmd, &fl)) == -1) {
			return (fail(L, errno));
		}
		if (cmd == F_GETLK) {
			pushflock(L, &fl);
			return (1);
		}
		return (success(L));
	}
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
	case F_GET_SEALS:
		if ((result = fcntl(fd, cmd)) == -1) {
			return (fail(L, errno));
		}
		lua_pushinteger(L, result);
		return (1);
	case F_ISUNIONSTACK:
		if ((result = fcntl(fd, cmd)) == -1) {
			return (fail(L, errno));
		}
		lua_pushboolean(L, result);
		return (1);
	case F_KINFO:
		return (luaL_error(L, "TODO: sys.user"));
	default: {
		int arg;

		arg = luaL_checkinteger(L, 3);

		if ((result = fcntl(fd, cmd, arg)) == -1) {
			return (fail(L, errno));
		}
		return (success(L));
	}
	}
	__unreachable();
}

static int
l_flock(lua_State *L)
{
	int fd, operation;

	fd = checkfd(L, 1);
	operation = luaL_checkinteger(L, 2);

	if (flock(fd, operation) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

#if __FreeBSD_version > 1400028
#define SR_FIELDS(X) \
	X(offset) \
	X(len)

static inline void
checksr(lua_State *L, int idx, struct spacectl_range *sr)
{
	luaL_checktype(L, idx, LUA_TTABLE);

	memset(sr, 0, sizeof(*sr));
#define FIELD(name) \
	if (lua_getfield(L, idx, #name) != LUA_TNIL) { \
		if (lua_isinteger(L, -1)) { \
			sr->r_##name = lua_tointeger(L, -1); \
		} else { \
			luaL_argerror(L, idx, "invalid spacectl_range "#name); \
		} \
	}
	SR_FIELDS(FIELD)
#undef FIELD
	lua_pop(L, 2);
}

static inline void
pushsr(lua_State *L, const struct spacectl_range *sr)
{
	lua_createtable(L, 0, 2);
#define FIELD(name) \
	lua_pushinteger(L, sr->r_##name); \
	lua_setfield(L, -2, #name);
	SR_FIELDS(FIELD)
#undef FIELD
}

static int
l_fspacectl(lua_State *L)
{
	struct spacectl_range rqsr, rmsr;
	int fd, cmd, flags;

	fd = checkfd(L, 1);
	cmd = luaL_checkinteger(L, 2);
	checksr(L, 3, &rqsr);
	flags = luaL_optinteger(L, 4, 0);

	if (fspacectl(fd, cmd, &rqsr, flags, &rmsr) == -1) {
		return (fail(L, errno));
	}
	pushsr(L, &rmsr);
	return (1);
}
#endif

static int
l_openat(lua_State *L)
{
	const char *path;
	int atfd, flags, fd;

	atfd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	flags = luaL_checkinteger(L, 3);

	if ((flags & O_CREAT) != 0) {
		mode_t mode;

	mode = luaL_checkinteger(L, 4);

		fd = openat(atfd, path, flags, mode);
	} else {
		fd = openat(atfd, path, flags);
	}
	if (fd == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fd);
	return (1);
}

static int
l_posix_fadvise(lua_State *L)
{
	off_t offset, len;
	int fd, advice, error;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 2);
	len = luaL_checkinteger(L, 3);
	advice = luaL_checkinteger(L, 4);

	if ((error = posix_fadvise(fd, offset, len, advice)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_posix_fallocate(lua_State *L)
{
	off_t offset, len;
	int fd, error;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 2);
	len = luaL_checkinteger(L, 3);

	if ((error = posix_fallocate(fd, offset, len)) != 0) {
		return (fail(L, error));
	}
	return (success(L));
}

static const struct luaL_Reg l_fcntl_funcs[] = {
	/* creat(2) is obsolete */
	{"open", l_open},
	{"fcntl", l_fcntl},
	{"flock", l_flock},
#if __FreeBSD_version > 1400028
	{"fspacectl", l_fspacectl},
#endif
	{"openat", l_openat},
	{"posix_fadvise", l_posix_fadvise},
	{"posix_fallocate", l_posix_fallocate},
	{NULL, NULL}
};

int
luaopen_fcntl(lua_State *L)
{
	luaL_newlib(L, l_fcntl_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(O_RDONLY);
	DEFINE(O_WRONLY);
	DEFINE(O_RDWR);
	DEFINE(O_ACCMODE);
	DEFINE(O_NONBLOCK);
	DEFINE(O_APPEND);
	DEFINE(O_SHLOCK);
	DEFINE(O_EXLOCK);
	DEFINE(O_ASYNC);
	DEFINE(O_FSYNC);
	DEFINE(O_SYNC);
	DEFINE(O_NOFOLLOW);
	DEFINE(O_CREAT);
	DEFINE(O_TRUNC);
	DEFINE(O_EXCL);
	DEFINE(O_NOCTTY);
	DEFINE(O_DIRECT);
	DEFINE(O_DIRECTORY);
	DEFINE(O_EXEC);
	DEFINE(O_SEARCH);
	DEFINE(O_TTY_INIT);
	DEFINE(O_CLOEXEC);
	DEFINE(O_VERIFY);
	DEFINE(O_PATH);
	DEFINE(O_RESOLVE_BENEATH);
	DEFINE(O_DSYNC);
	DEFINE(O_EMPTY_PATH);
#ifdef O_NAMEDATTR
	DEFINE(O_NAMEDATTR);
#endif
#ifdef O_CLOFORK
	DEFINE(O_CLOFORK);
#endif

	DEFINE(AT_FDCWD);
	DEFINE(AT_EACCESS);
	DEFINE(AT_SYMLINK_NOFOLLOW);
	DEFINE(AT_SYMLINK_FOLLOW);
	DEFINE(AT_REMOVEDIR);
	DEFINE(AT_RESOLVE_BENEATH);
	DEFINE(AT_EMPTY_PATH);

	DEFINE(F_DUPFD);
	DEFINE(F_GETFD);
	DEFINE(F_SETFD);
	DEFINE(F_GETFL);
	DEFINE(F_SETFL);
	DEFINE(F_GETOWN);
	DEFINE(F_SETOWN);
	/* "Old" (Obsolete) F_O*LK cmds omitted. */
	DEFINE(F_DUP2FD);
	DEFINE(F_GETLK);
	DEFINE(F_SETLK);
	DEFINE(F_SETLK_REMOTE);
	DEFINE(F_READAHEAD);
	DEFINE(F_RDAHEAD);
	DEFINE(F_DUPFD_CLOEXEC);
	DEFINE(F_DUP2FD_CLOEXEC);
	DEFINE(F_ADD_SEALS);
	DEFINE(F_GET_SEALS);
	DEFINE(F_ISUNIONSTACK);
	DEFINE(F_KINFO);
#ifdef F_DUPFD_CLOFORK
	DEFINE(F_DUPFD_CLOFORK);
#endif
	/* Use dup3(3) not F_DUP3FD. */
	DEFINE(F_SEAL_SEAL);
	DEFINE(F_SEAL_SHRINK);
	DEFINE(F_SEAL_GROW);
	DEFINE(F_SEAL_WRITE);

	DEFINE(FD_CLOEXEC);
#ifdef FD_RESOLVE_BENEATH
	DEFINE(FD_RESOLVE_BENEATH);
#endif
#ifdef FD_CLOFORK
	DEFINE(FD_CLOFORK);
#endif

	DEFINE(F_RDLCK);
	DEFINE(F_UNLCK);
	DEFINE(F_WRLCK);
	DEFINE(F_UNLCKSYS);
	DEFINE(F_CANCEL);

	DEFINE(LOCK_SH);
	DEFINE(LOCK_EX);
	DEFINE(LOCK_NB);
	DEFINE(LOCK_UN);

	DEFINE(POSIX_FADV_NORMAL);
	DEFINE(POSIX_FADV_RANDOM);
	DEFINE(POSIX_FADV_SEQUENTIAL);
	DEFINE(POSIX_FADV_WILLNEED);
	DEFINE(POSIX_FADV_DONTNEED);
	DEFINE(POSIX_FADV_NOREUSE);

	DEFINE(FD_NONE);

#ifdef SPACECTL_DEALLOC
	DEFINE(SPACECTL_DEALLOC);
#endif
#ifdef SPACECTL_F_SUPPORTED
	DEFINE(SPACECTL_F_SUPPORTED);
#endif
#undef DEFINE
	return (1);
}
