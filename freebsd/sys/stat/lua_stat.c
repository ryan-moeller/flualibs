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
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h> /* TODO: move these parts to freebsd.unistd? */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

int luaopen_freebsd_sys_stat(lua_State *);

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
	lua_pushboolean(L, true);
	return (1);
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
	lua_pushboolean(L, true);
	return (1);
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
	lua_pushboolean(L, true);
	return (1);
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
	lua_pushboolean(L, true);
	return (1);
}

static int
l_fflagstostr(lua_State *L)
{
	char *str;
	u_long flags;

	flags = luaL_checkinteger(L, 1);

	if ((str = fflagstostr(flags)) == NULL) {
		return (fail(L, ENOMEM));
	}
	lua_pushstring(L, str);
	free(str);
	return (1);
}

static int
l_strtofflags(lua_State *L)
{
	const char *s;
	char *str;
	u_long set, clr;

	s = luaL_checkstring(L, 1);

	if ((str = strdup(s)) == NULL) {
		return (fatal(L, "strdup", ENOMEM));
	}
	if (strtofflags(&str, &set, &clr) != 0) {
		luaL_pushfail(L);
		lua_pushstring(L, str);
		free(str);
		return (2);
	}
	free(str);
	lua_pushinteger(L, set);
	lua_pushinteger(L, clr);
	return (2);
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
	{"fflagstostr", l_fflagstostr},
	{"strtofflags", l_strtofflags},
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
luaopen_freebsd_sys_stat(lua_State *L)
{
	luaL_newlib(L, l_stat_funcs);
#define SETCONST(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	SETCONST(AT_SYMLINK_FOLLOW);
	SETCONST(AT_RESOLVE_BENEATH);
	SETCONST(AT_EMPTY_PATH);
	SETCONST(AT_FDCWD);
	SETCONST(S_ISUID);
	SETCONST(S_ISGID);
	SETCONST(S_ISTXT);
	SETCONST(S_IRWXU);
	SETCONST(S_IRUSR);
	SETCONST(S_IWUSR);
	SETCONST(S_IXUSR);
	SETCONST(S_IREAD);
	SETCONST(S_IWRITE);
	SETCONST(S_IEXEC);
	SETCONST(S_IRWXG);
	SETCONST(S_IRGRP);
	SETCONST(S_IWGRP);
	SETCONST(S_IXGRP);
	SETCONST(S_IRWXO);
	SETCONST(S_IROTH);
	SETCONST(S_IWOTH);
	SETCONST(S_IXOTH);
	SETCONST(S_IFMT);
	SETCONST(S_IFIFO);
	SETCONST(S_IFCHR);
	SETCONST(S_IFDIR);
	SETCONST(S_IFBLK);
	SETCONST(S_IFREG);
	SETCONST(S_IFLNK);
	SETCONST(S_IFSOCK);
	SETCONST(S_ISVTX);
	SETCONST(S_IFWHT);
	SETCONST(ACCESSPERMS);
	SETCONST(ALLPERMS);
	SETCONST(DEFFILEMODE);
	SETCONST(S_BLKSIZE);

	SETCONST(UF_SETTABLE);
	SETCONST(UF_NODUMP);
	SETCONST(UF_IMMUTABLE);
	SETCONST(UF_APPEND);
	SETCONST(UF_OPAQUE);
	SETCONST(UF_NOUNLINK);
	SETCONST(UF_SYSTEM);
	SETCONST(UF_SPARSE);
	SETCONST(UF_OFFLINE);
	SETCONST(UF_REPARSE);
	SETCONST(UF_ARCHIVE);
	SETCONST(UF_READONLY);
	SETCONST(UF_HIDDEN);
	SETCONST(SF_SETTABLE);
	SETCONST(SF_ARCHIVED);
	SETCONST(SF_IMMUTABLE);
	SETCONST(SF_APPEND);
	SETCONST(SF_NOUNLINK);
	SETCONST(SF_SNAPSHOT);

#ifdef SFBSD_NAMEDATTR
	SETCONST(SFBSD_NAMEDATTR);
#endif
	SETCONST(UTIME_NOW);
	SETCONST(UTIME_OMIT);
#undef SETCONST
	return (1);
}
