/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* XXX: lfs doesn't expose st_flags */

#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

int luaopen_sys_stat(lua_State *);

static inline void
pushstat(lua_State *L, struct stat *sb)
{
	lua_newtable(L);
#define INTFIELD(name) ({ \
	lua_pushinteger(L, sb->st_##name); \
	lua_setfield(L, -2, #name); \
})
	INTFIELD(dev);
	INTFIELD(ino);
	INTFIELD(nlink);
	INTFIELD(mode);
#ifdef SFBSD_NAMEDATTR
	INTFIELD(bsdflags);
#endif
	INTFIELD(uid);
	INTFIELD(gid);
	INTFIELD(rdev);
	INTFIELD(size);
	INTFIELD(blocks);
	INTFIELD(blksize);
	INTFIELD(flags);
	INTFIELD(gen);
#if __FreeBSD_version > 1402501
	INTFIELD(filerev);
#endif
#undef INTFIELD
#define TIMEFIELD(name) ({ \
	lua_createtable(L, 0, 2); \
	lua_pushinteger(L, sb->st_##name.tv_sec); \
	lua_setfield(L, -2, "sec"); \
	lua_pushinteger(L, sb->st_##name.tv_nsec); \
	lua_setfield(L, -2, "nsec"); \
	lua_setfield(L, -2, #name); \
})
	TIMEFIELD(atim);
	TIMEFIELD(mtim);
	TIMEFIELD(ctim);
	TIMEFIELD(birthtim);
#undef TIMEFIELD
}

static int
l_chflags(lua_State *L)
{
	const char *path;
	unsigned long flags;

	path = luaL_checkstring(L, 1);
	flags = luaL_checkinteger(L, 2);

	if (chflags(path, flags) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_lchflags(lua_State *L)
{
	const char *path;
	unsigned long flags;

	path = luaL_checkstring(L, 1);
	flags = luaL_checkinteger(L, 2);

	if (lchflags(path, flags) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_fchflags(lua_State *L)
{
	unsigned long flags;
	int fd;

	fd = checkfd(L, 1);
	flags = luaL_checkinteger(L, 2);

	if (fchflags(fd, flags) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_chflagsat(lua_State *L)
{
	const char *path;
	unsigned long flags;
	int fd, atflag;

	fd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	flags = luaL_checkinteger(L, 3);
	atflag = luaL_checkinteger(L, 4);

	if (chflagsat(fd, path, flags, atflag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_stat(lua_State *L)
{
	struct stat sb;
	const char *path;

	path = luaL_checkstring(L, 1);

	if (stat(path, &sb) == -1) {
		return (fail(L, errno));
	}
	pushstat(L, &sb);
	return (1);
}

static int
l_lstat(lua_State *L)
{
	struct stat sb;
	const char *path;

	path = luaL_checkstring(L, 1);

	if (lstat(path, &sb) == -1) {
		return (fail(L, errno));
	}
	pushstat(L, &sb);
	return (1);
}

static int
l_fstat(lua_State *L)
{
	struct stat sb;
	int fd;

	fd = checkfd(L, 1);

	if (fstat(fd, &sb) == -1) {
		return (fail(L, errno));
	}
	pushstat(L, &sb);
	return (1);
}

static int
l_fstatat(lua_State *L)
{
	struct stat sb;
	const char *path;
	int fd, flag;

	fd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	flag = luaL_checkinteger(L, 3);

	if (fstatat(fd, path, &sb, flag) == -1) {
		return (fail(L, errno));
	}
	pushstat(L, &sb);
	return (1);
}

#define S_TYPES(X) \
	X(dir, DIR) \
	X(chr, CHR) \
	X(blk, BLK) \
	X(reg, REG) \
	X(fifo, FIFO) \
	X(lnk, LNK) \
	X(sock, SOCK) \
	X(wht, WHT)

#define S_TYPE(l, U) \
static int \
l_s_is##l(lua_State *L) \
{ \
	uint32_t mode; \
\
	mode = luaL_checkinteger(L, 1); \
\
	lua_pushboolean(L, S_IS##U(mode)); \
	return (1); \
}

S_TYPES(S_TYPE)

#undef S_TYPE

static const struct luaL_Reg l_stat_funcs[] = {
	{"chflags", l_chflags},
	{"lchflags", l_lchflags},
	{"fchflags", l_fchflags},
	{"chflagsat", l_chflagsat},
	{"stat", l_stat},
	{"lstat", l_lstat},
	{"fstat", l_fstat},
	{"fstatat", l_fstatat},
#define S_TYPE(l, U) {"is"#l, l_s_is##l},
	S_TYPES(S_TYPE)
#undef S_TYPE
	{NULL, NULL}
};

int
luaopen_sys_stat(lua_State *L)
{
	luaL_newlib(L, l_stat_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(S_ISUID);
	DEFINE(S_ISGID);
	DEFINE(S_ISTXT);
	DEFINE(S_IRWXU);
	DEFINE(S_IRUSR);
	DEFINE(S_IWUSR);
	DEFINE(S_IXUSR);
	DEFINE(S_IREAD);
	DEFINE(S_IWRITE);
	DEFINE(S_IEXEC);
	DEFINE(S_IRWXG);
	DEFINE(S_IRGRP);
	DEFINE(S_IWGRP);
	DEFINE(S_IXGRP);
	DEFINE(S_IRWXO);
	DEFINE(S_IROTH);
	DEFINE(S_IWOTH);
	DEFINE(S_IXOTH);
	DEFINE(S_IFMT);
	DEFINE(S_IFIFO);
	DEFINE(S_IFCHR);
	DEFINE(S_IFDIR);
	DEFINE(S_IFBLK);
	DEFINE(S_IFREG);
	DEFINE(S_IFLNK);
	DEFINE(S_IFSOCK);
	DEFINE(S_ISVTX);
	DEFINE(S_IFWHT);
	DEFINE(ACCESSPERMS);
	DEFINE(ALLPERMS);
	DEFINE(DEFFILEMODE);
	DEFINE(S_BLKSIZE);

	DEFINE(UF_SETTABLE);
	DEFINE(UF_NODUMP);
	DEFINE(UF_IMMUTABLE);
	DEFINE(UF_APPEND);
	DEFINE(UF_OPAQUE);
	DEFINE(UF_NOUNLINK);
	DEFINE(UF_SYSTEM);
	DEFINE(UF_SPARSE);
	DEFINE(UF_OFFLINE);
	DEFINE(UF_REPARSE);
	DEFINE(UF_ARCHIVE);
	DEFINE(UF_READONLY);
	DEFINE(UF_HIDDEN);
	DEFINE(SF_SETTABLE);
	DEFINE(SF_ARCHIVED);
	DEFINE(SF_IMMUTABLE);
	DEFINE(SF_APPEND);
	DEFINE(SF_NOUNLINK);
	DEFINE(SF_SNAPSHOT);

#ifdef SFBSD_NAMEDATTR
	DEFINE(SFBSD_NAMEDATTR);
#endif
	DEFINE(UTIME_NOW);
	DEFINE(UTIME_OMIT);
#undef DEFINE
	return (1);
}
