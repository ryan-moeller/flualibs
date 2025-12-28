/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/dirent.h>
#include <assert.h>
#include <errno.h>
#include <paths.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "utils.h"

#define SETMODE_METATABLE "setmode"

int luaopen_unistd(lua_State *);

static int
l_fork(lua_State *L)
{
	pid_t pid;

	if ((pid = fork()) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, pid);
	return (1);
}

static int
l__Fork(lua_State *L)
{
	pid_t pid;

	if ((pid = _Fork()) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, pid);
	return (1);
}

static int __dead2
l__exit(lua_State *L)
{
	int status;

	status = luaL_optinteger(L, 1, 0);

	_exit(status);
}

static int
l_access(lua_State *L)
{
	const char *path;
	int mode;

	path = luaL_checkstring(L, 1);
	mode = luaL_checkinteger(L, 2);

	if (access(path, mode) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_eaccess(lua_State *L)
{
	const char *path;
	int mode;

	path = luaL_checkstring(L, 1);
	mode = luaL_checkinteger(L, 2);

	if (eaccess(path, mode) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_faccessat(lua_State *L)
{
	const char *path;
	int dfd, mode, flag;

	dfd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	mode = luaL_checkinteger(L, 3);
	flag = luaL_optinteger(L, 4, 0);

	if (faccessat(dfd, path, mode, flag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_acct(lua_State *L)
{
	const char *path;

	path = luaL_optstring(L, 1, NULL);

	if (acct(path) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_chdir(lua_State *L)
{
	const char *path;

	path = luaL_checkstring(L, 1);

	if (chdir(path) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_fchdir(lua_State *L)
{
	int fd;

	fd = luaL_checkinteger(L, 1);

	if (fchdir(fd) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_check_utility_compat(lua_State *L)
{
	const char *utils;

	utils = luaL_checkstring(L, 1);

	lua_pushboolean(L, check_utility_compat(utils) == 0);
	return (1);
}

static int
l_chown(lua_State *L)
{
	const char *path;
	uid_t owner;
	gid_t group;

	path = luaL_checkstring(L, 1);
	owner = luaL_checkinteger(L, 2);
	group = luaL_checkinteger(L, 3);

	if (chown(path, owner, group) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_fchown(lua_State *L)
{
	int fd;
	uid_t owner;
	gid_t group;

	fd = checkfd(L, 1);
	owner = luaL_checkinteger(L, 2);
	group = luaL_checkinteger(L, 3);

	if (fchown(fd, owner, group) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_lchown(lua_State *L)
{
	const char *path;
	uid_t owner;
	gid_t group;

	path = luaL_checkstring(L, 1);
	owner = luaL_checkinteger(L, 2);
	group = luaL_checkinteger(L, 3);

	if (lchown(path, owner, group) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_fchownat(lua_State *L)
{
	const char *path;
	uid_t owner;
	gid_t group;
	int dfd, flag;

	dfd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	owner = luaL_checkinteger(L, 3);
	group = luaL_checkinteger(L, 4);
	flag = luaL_optinteger(L, 5, 0);

	if (fchownat(dfd, path, owner, group, flag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_chroot(lua_State *L)
{
	const char *dirname;

	dirname = luaL_checkstring(L, 1);

	if (chroot(dirname) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

#if __FreeBSD_version > 1500028
static int
l_fchroot(lua_State *L)
{
	int dfd;

	dfd = luaL_checkinteger(L, 1);

	if (fchroot(dfd) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}
#endif

static int
l_close(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	if (close(fd) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_close_range(lua_State *L)
{
	u_int lowfd, highfd;
	int flags;

	lowfd = checkfd(L, 1);
	highfd = checkfd(L, 2);
	flags = luaL_optinteger(L, 3, 0);

	if (close_range(lowfd, highfd, flags) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_closefrom(lua_State *L)
{
	int lowfd;

	lowfd = checkfd(L, 1);

	closefrom(lowfd);
	return (0);
}

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
	flags = luaL_optinteger(L, 6, 0);

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
l_dup(lua_State *L)
{
	int oldd, newd;

	oldd = checkfd(L, 1);

	if ((newd = dup(oldd)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, newd);
	return (1);
}

static int
l_dup2(lua_State *L)
{
	int oldd, newd;

	oldd = checkfd(L, 1);
	newd = checkfd(L, 2);

	if (dup2(oldd, newd) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, newd);
	return (1);
}

static int
l_dup3(lua_State *L)
{
	int oldd, newd, flags;

	oldd = checkfd(L, 1);
	newd = checkfd(L, 2);
	flags = luaL_optinteger(L, 3, 0);

	if (dup3(oldd, newd, flags) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, newd);
	return (1);
}

static int
l_getusershell(lua_State *L)
{
	lua_pushstring(L, getusershell());
	return (1);
}

static int
l_setusershell(lua_State *L __unused)
{
	setusershell();
	return (0);
}

static int
l_endusershell(lua_State *L __unused)
{
	endusershell();
	return (0);
}

static int
l_enter_restricted_mode(lua_State *L __unused)
{
	/* This was used by ftpd before it got banished to ports. */
	__FreeBSD_libc_enter_restricted_mode();
	return (0);
}

static int
l_fdatasync(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	if (fdatasync(fd) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_fsync(lua_State *L)
{
	int fd;

	fd = checkfd(L, 1);

	if (fsync(fd) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_sync(lua_State *L __unused)
{
	sync();
	return (0);
}

static int
l_feature_present(lua_State *L)
{
	const char *feature;

	feature = luaL_checkstring(L, 1);

	lua_pushboolean(L, feature_present(feature));
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
l_unlink(lua_State *L)
{
	const char *path;

	path = luaL_checkstring(L, 1);

	if (unlink(path) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_unlinkat(lua_State *L)
{
	const char *path;
	int dfd, flag;

	dfd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	flag = luaL_optinteger(L, 3, 0);

	if (unlinkat(dfd, path, flag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_funlinkat(lua_State *L)
{
	const char *path;
	int dfd, fd, flag;

	dfd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);
	fd = checkfd(L, 3);
	flag = luaL_optinteger(L, 4, 0);

	if (funlinkat(dfd, path, fd, flag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_getcwd(lua_State *L)
{
	char *cwd;

	if ((cwd = getcwd(NULL, 0)) == NULL) {
		return (fail(L, errno));
	}
	lua_pushstring(L, cwd);
	free(cwd);
	return (1);
}

static int
l_getdomainname(lua_State *L)
{
	char name[MAXHOSTNAMELEN];

	if (getdomainname(name, sizeof(name)) == -1) {
		return (fail(L, errno));
	}
	lua_pushstring(L, name);
	return (1);
}

static int
l_setdomainname(lua_State *L)
{
	const char *name;
	size_t namelen;

	name = luaL_checklstring(L, 1, &namelen);

	if (setdomainname(name, namelen) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_getdtablesize(lua_State *L)
{
	lua_pushinteger(L, getdtablesize());
	return (1);
}

static int
l_getgid(lua_State *L)
{
	lua_pushinteger(L, getgid());
	return (1);
}

static int
l_getegid(lua_State *L)
{
	lua_pushinteger(L, getegid());
	return (1);
}

static int
l_getentropy(lua_State *L)
{
	luaL_Buffer b;
	char *buf;
	size_t buflen;

	buflen = luaL_checkinteger(L, 1);

	buf = luaL_buffinitsize(L, &b, buflen);
	if (getentropy(buf, buflen) == -1) {
		return (fail(L, errno));
	}
	luaL_pushresultsize(&b, buflen);
	return (1);
}

static int
l_getuid(lua_State *L)
{
	lua_pushinteger(L, getuid());
	return (1);
}

static int
l_geteuid(lua_State *L)
{
	lua_pushinteger(L, geteuid());
	return (1);
}

static int
l_getgrouplist(lua_State *L)
{
	const char *name;
	gid_t *groups;
	gid_t basegid;
	int ngroups;

	name = luaL_checkstring(L, 1);
	basegid = luaL_checkinteger(L, 2);

	groups = NULL;
	ngroups = 0;
	while (getgrouplist(name, basegid, groups, &ngroups) == -1) {
		free(groups);
		if ((groups = malloc(ngroups * sizeof(*groups))) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	lua_createtable(L, ngroups, 0);
	while (ngroups-- > 0) {
		lua_pushinteger(L, groups[ngroups]);
		lua_rawseti(L, -2, ngroups + 1);
	}
	free(groups);
	return (1);
}

static int
l_getgroups(lua_State *L)
{
	gid_t *gidset;
	int gidsetlen, result;

retry:
	gidset = NULL;
	switch ((gidsetlen = getgroups(0, NULL))) {
	case -1:
		return (fail(L, errno));
	case 0:
		break;
	default:
		if ((gidset = malloc(gidsetlen * sizeof(*gidset))) == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
		if ((result = getgroups(gidsetlen, gidset)) == -1) {
			int error = errno;

			free(gidset);
			if (error == EINVAL) {
				/* The list of groups grew. */
				goto retry;
			}
			return (fail(L, error));
		}
		break;
	}
	lua_createtable(L, gidsetlen, 0);
	while (gidsetlen-- > 0) {
		lua_pushinteger(L, gidset[gidsetlen]);
		lua_rawseti(L, -2, gidsetlen + 1);
	}
	free(gidset);
	return (1);
}

static int
l_gethostname(lua_State *L)
{
	char *name;
	long namelen;

	if ((namelen = sysconf(_SC_HOST_NAME_MAX)) == -1) {
		return (fatal(L, "sysconf(_SC_HOST_NAME_MAX)", errno));
	}
	namelen++; /* terminator */
	if ((name = malloc(namelen)) == NULL) {
		return (fatal(L, "malloc", ENOMEM));
	}
	if (gethostname(name, namelen) == -1) {
		int error = errno;

		free(name);
		return (fail(L, error));
	}
	lua_pushstring(L, name);
	free(name);
	return (1);
}

static int
l_sethostname(lua_State *L)
{
	const char *name;
	size_t namelen;

	name = luaL_checklstring(L, 1, &namelen);

	if (sethostname(name, namelen) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_getlogin(lua_State *L)
{
	char name[MAXLOGNAME];
	int error;

	if ((error = getlogin_r(name, sizeof(name))) != 0) {
		return (fail(L, error));
	}
	lua_pushstring(L, name);
	return (1);
}

static int
l_setlogin(lua_State *L)
{
	const char *name;

	name = luaL_checkstring(L, 1);

	if (setlogin(name) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_getloginclass(lua_State *L)
{
	char name[MAXLOGNAME];
	int error;

	if (getloginclass(name, sizeof(name)) == -1) {
		return (fail(L, errno));
	}
	lua_pushstring(L, name);
	return (1);
}

static int
l_setloginclass(lua_State *L)
{
	const char *name;

	name = luaL_checkstring(L, 1);

	if (setloginclass(name) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_getmode(lua_State *L)
{
	const void *set;
	mode_t mode;

	set = checkcookie(L, 1, SETMODE_METATABLE);
	mode = luaL_optinteger(L, 2, 0);

	lua_pushinteger(L, getmode(set, mode));
	return (1);
}

static int
l_setmode(lua_State *L)
{
	const char *mode;
	void *set;

	mode = luaL_checkstring(L, 1);

	if ((set = setmode(mode)) == NULL) {
		return (fail(L, errno));
	}
	return (new(L, set, SETMODE_METATABLE));
}

static int
l_setmode_gc(lua_State *L)
{
	void *set;

	set = checkcookie(L, 1, SETMODE_METATABLE);

	free(set);
	return (0);
}

static int
l_getosreldate(lua_State *L)
{
	int osversion;

	if ((osversion = getosreldate()) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, osversion);
	return (1);
}

static int
l_getpagesize(lua_State *L)
{
	lua_pushinteger(L, getpagesize());
	return (1);
}

static int
l_getpass(lua_State *L)
{
	const char *prompt;

	prompt = luaL_optstring(L, 1, "");

	lua_pushstring(L, getpass(prompt));
	return (1);
}

static int
l_getpeereid(lua_State *L)
{
	uid_t euid;
	gid_t egid;
	int s;

	s = checkfd(L, 1);

	if (getpeereid(s, &euid, &egid) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, euid);
	lua_pushinteger(L, egid);
	return (2);
}

static int
l_getpgrp(lua_State *L)
{
	lua_pushinteger(L, getpgrp());
	return (1);
}

static int
l_getpgid(lua_State *L)
{
	pid_t pid, pgid;

	pid = luaL_checkinteger(L, 1);

	if ((pgid = getpgid(pid)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, pgid);
	return (1);
}

static int
l_getpid(lua_State *L)
{
	lua_pushinteger(L, getpid());
	return (1);
}

static int
l_getppid(lua_State *L)
{
	lua_pushinteger(L, getppid());
	return (1);
}

static int
l_getresgid(lua_State *L)
{
	gid_t rgid, egid, sgid;

	if (getresgid(&rgid, &egid, &sgid) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, rgid);
	lua_pushinteger(L, egid);
	lua_pushinteger(L, sgid);
	return (3);
}

static int
l_getresuid(lua_State *L)
{
	uid_t ruid, euid, suid;

	if (getresuid(&ruid, &euid, &suid) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, ruid);
	lua_pushinteger(L, euid);
	lua_pushinteger(L, suid);
	return (3);
}

static int
l_setresgid(lua_State *L)
{
	gid_t rgid, egid, sgid;

	rgid = luaL_checkinteger(L, 1);
	egid = luaL_checkinteger(L, 2);
	sgid = luaL_checkinteger(L, 3);

	if (setresgid(rgid, egid, sgid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setresuid(lua_State *L)
{
	uid_t ruid, euid, suid;

	ruid = luaL_checkinteger(L, 1);
	euid = luaL_checkinteger(L, 2);
	suid = luaL_checkinteger(L, 3);

	if (setresuid(ruid, euid, suid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_getsid(lua_State *L)
{
	pid_t pid, sid;

	pid = luaL_checkinteger(L, 1);

	if ((sid = getsid(pid)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, sid);
	return (1);
}

static int
l_initgroups(lua_State *L)
{
	const char *name;
	gid_t basegid;

	name = luaL_checkstring(L, 1);
	basegid = luaL_checkinteger(L, 2);

	if (initgroups(name, basegid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_ttyname(lua_State *L)
{
	char name[sizeof(_PATH_DEV) + MAXNAMLEN];
	int fd, error;

	fd = checkfd(L, 1);

	if ((error = ttyname_r(fd, name, sizeof(name))) != 0) {
		return (fail(L, error));
	}
	lua_pushstring(L, name);
	return (1);
}

static int
l_isatty(lua_State *L)
{
	int fd, error;
	bool is;

	fd = checkfd(L, 1);

	errno = 0;
	is = isatty(fd);
	error = errno;
	lua_pushboolean(L, is);
	if (error != 0) {
		char msg[NL_TEXTMAX];

		strerror_r(error, msg, sizeof(msg));
		lua_pushstring(L, msg);
		lua_pushinteger(L, error);
		return (3);
	}
	return (1);
}

static int
l_issetugid(lua_State *L)
{
	lua_pushboolean(L, issetugid());
	return (1);
}

static int
l_kcmp(lua_State *L)
{
	uintptr_t idx1, idx2;
	pid_t pid1, pid2;
	int type, result;

	pid1 = luaL_checkinteger(L, 1);
	pid2 = luaL_checkinteger(L, 2);
	type = luaL_checkinteger(L, 3);
	idx1 = luaL_optinteger(L, 4, 0);
	idx2 = luaL_optinteger(L, 5, 0);

	if ((result = kcmp(pid1, pid2, type, idx1, idx2)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, result);
	return (1);
}

static int
l_link(lua_State *L)
{
	const char *name1, *name2;

	name1 = luaL_checkstring(L, 1);
	name2 = luaL_checkstring(L, 2);

	if (link(name1, name2) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_linkat(lua_State *L)
{
	const char *name1, *name2;
	int dfd1, dfd2, flag;

	dfd1 = luaL_checkinteger(L, 1);
	name1 = luaL_checkstring(L, 2);
	dfd2 = luaL_checkinteger(L, 3);
	name2 = luaL_checkstring(L, 4);
	flag = luaL_optinteger(L, 5, 0);

	if (linkat(dfd1, name1, dfd2, name2, flag) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_lockf(lua_State *L)
{
	off_t size;
	int fd, function;

	fd = checkfd(L, 1);
	function = luaL_checkinteger(L, 2);
	size = luaL_checkinteger(L, 3);

	if (lockf(fd, function, size) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_lseek(lua_State *L)
{
	off_t offset, result;
	int fd, whence;

	fd = checkfd(L, 1);
	offset = luaL_checkinteger(L, 2);
	whence = luaL_checkinteger(L, 3);

	if ((result = lseek(fd, offset, whence)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, result);
	return (1);
}

static int
l_mktemp(lua_State *L)
{
	const char *template;
	char *buf;

	template = luaL_checkstring(L, 1);

	if ((buf = strdup(template)) == NULL) {
		return (fatal(L, "strdup", errno));
	}
	if (mktemp(buf) == NULL) {
		int error = errno;

		free(buf);
		return (fail(L, error));
	}
	lua_pushstring(L, buf);
	free(buf);
	return (1);
}

static int
l_mkstemp(lua_State *L)
{
	const char *template;
	char *buf;
	int fd;

	template = luaL_checkstring(L, 1);

	if ((buf = strdup(template)) == NULL) {
		return (fatal(L, "strdup", errno));
	}
	if ((fd = mkstemp(buf)) == -1) {
		int error = errno;

		free(buf);
		return (fail(L, error));
	}
	lua_pushinteger(L, fd);
	lua_pushstring(L, buf);
	free(buf);
	return (2);
}

static int
l_mkdtemp(lua_State *L)
{
	const char *template;
	char *buf;

	template = luaL_checkstring(L, 1);

	if ((buf = strdup(template)) == NULL) {
		return (fatal(L, "strdup", errno));
	}
	if (mkdtemp(buf) == NULL) {
		int error = errno;

		free(buf);
		return (fail(L, error));
	}
	lua_pushstring(L, buf);
	free(buf);
	return (1);
}

static int
l_mkstemps(lua_State *L)
{
	const char *template;
	char *buf;
	int suffixlen, fd;

	template = luaL_checkstring(L, 1);
	suffixlen = luaL_checkinteger(L, 2);

	if ((buf = strdup(template)) == NULL) {
		return (fatal(L, "strdup", errno));
	}
	if ((fd = mkstemps(buf, suffixlen)) == -1) {
		int error = errno;

		free(buf);
		return (fail(L, error));
	}
	lua_pushinteger(L, fd);
	lua_pushstring(L, buf);
	free(buf);
	return (2);
}

static int
l_mknod(lua_State *L)
{
	const char *path;
	mode_t mode;
	dev_t dev;

	path = luaL_checkstring(L, 1);
	mode = luaL_checkinteger(L, 2);
	dev = luaL_checkinteger(L, 3);

	if (mknod(path, mode, dev) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_pipe2(lua_State *L)
{
	int fds[2];
	int flags;

	flags = luaL_optinteger(L, 1, 0);

	if (pipe2(fds, flags) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, fds[0]);
	lua_pushinteger(L, fds[1]);
	return (2);
}

static int
l_read(lua_State *L)
{
	luaL_Buffer b;
	void *buf;
	ssize_t result;
	size_t nbytes;
	int fd;

	fd = checkfd(L, 1);
	nbytes = luaL_checkinteger(L, 2);

	buf = luaL_buffinitsize(L, &b, nbytes);
	if ((result = read(fd, buf, nbytes)) == -1) {
		return (fail(L, errno));
	}
	luaL_pushresultsize(&b, result);
	return (1);
}

static int
l_pread(lua_State *L)
{
	luaL_Buffer b;
	void *buf;
	ssize_t result;
	size_t nbytes;
	off_t offset;
	int fd;

	fd = checkfd(L, 1);
	nbytes = luaL_checkinteger(L, 2);
	offset = luaL_checkinteger(L, 3);

	buf = luaL_buffinitsize(L, &b, nbytes);
	if ((result = pread(fd, buf, nbytes, offset)) == -1) {
		return (fail(L, errno));
	}
	luaL_pushresultsize(&b, result);
	return (1);
}

static int
l_profil(lua_State *L)
{
	luaL_Buffer b;
	char *samples;
	size_t size;
	vm_offset_t offset;
	int scale;

	size = luaL_checkinteger(L, 1);
	offset = luaL_checkinteger(L, 2);
	scale = luaL_checkinteger(L, 3);

	samples = luaL_buffinitsize(L, &b, size);
	if (profil(samples, size, offset, scale) == -1) {
		return (fail(L, errno));
	}
	luaL_pushresultsize(&b, size);
	return (1);
}

static int
l_write(lua_State *L)
{
	const void *buf;
	size_t nbytes;
	ssize_t result;
	int fd;

	fd = checkfd(L, 1);
	buf = luaL_checklstring(L, 2, &nbytes);

	if ((result = write(fd, buf, nbytes)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, result);
	return (1);
}

static int
l_pwrite(lua_State *L)
{
	const void *buf;
	ssize_t result;
	size_t nbytes;
	off_t offset;
	int fd;

	fd = checkfd(L, 1);
	buf = luaL_checklstring(L, 2, &nbytes);
	offset = luaL_checkinteger(L, 3);

	if ((result = pwrite(fd, buf, nbytes, offset)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, result);
	return (1);
}

static int
l_readlink(lua_State *L)
{
	char buf[MAXPATHLEN];
	const char *path;
	ssize_t result;

	path = luaL_checkstring(L, 1);

	if ((result = readlink(path, buf, sizeof(buf))) == -1) {
		return (fail(L, errno));
	}
	lua_pushlstring(L, buf, result);
	return (1);
}

static int
l_readlinkat(lua_State *L)
{
	char buf[MAXPATHLEN];
	const char *path;
	ssize_t result;
	int dfd;

	dfd = luaL_checkinteger(L, 1);
	path = luaL_checkstring(L, 2);

	if ((result = readlinkat(dfd, path, buf, sizeof(buf))) == -1) {
		return (fail(L, errno));
	}
	lua_pushlstring(L, buf, result);
	return (1);
}

static int
l_reboot(lua_State *L)
{
	int howto;

	howto = luaL_checkinteger(L, 1);

	reboot(howto);
	return (fail(L, errno));
}

static int
l_revoke(lua_State *L)
{
	const char *path;

	path = luaL_checkstring(L, 1);

	if (revoke(path) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_rfork(lua_State *L)
{
	pid_t pid;
	int flags;

	flags = luaL_checkinteger(L, 1);

	if ((pid = rfork(flags)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, pid);
	return (1);
}

static int
l_rmdir(lua_State *L)
{
	const char *path;

	path = luaL_checkstring(L, 1);

	if (rmdir(path) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setuid(lua_State *L)
{
	uid_t uid;

	uid = luaL_checkinteger(L, 1);

	if (setuid(uid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_seteuid(lua_State *L)
{
	uid_t euid;

	euid = luaL_checkinteger(L, 1);

	if (seteuid(euid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setgid(lua_State *L)
{
	gid_t gid;

	gid = luaL_checkinteger(L, 1);

	if (setgid(gid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setegid(lua_State *L)
{
	gid_t egid;

	egid = luaL_checkinteger(L, 1);

	if (setegid(egid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setgroups(lua_State *L)
{
	gid_t *gidset;
	int ngroups;

	if (lua_isnoneornil(L, 1)) {
		gidset = NULL;
		ngroups = 0;
	} else {
		luaL_checktype(L, 1, LUA_TTABLE);
		if ((ngroups = luaL_len(L, 1)) == 0) {
			gidset = NULL;
		} else if ((gidset = malloc(ngroups * sizeof(*gidset)))
		    == NULL) {
			return (fatal(L, "malloc", ENOMEM));
		}
	}
	if (setgroups(ngroups, gidset) == -1) {
		int error = errno;

		free(gidset);
		return (fail(L, error));
	}
	free(gidset);
	return (success(L));
}

static int
l_setpgid(lua_State *L)
{
	pid_t pid, pgid;

	pid = luaL_checkinteger(L, 1);
	pgid = luaL_checkinteger(L, 2);

	if (setpgid(pid, pgid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setproctitle(lua_State *L)
{
	if (lua_isnoneornil(L, 1)) {
		setproctitle(NULL);
	} else {
		const char *title, *fmt;

		title = luaL_checkstring(L, 1);
		fmt = lua_toboolean(L, 2) ? "-%s" : "%s";

		setproctitle(fmt, title);
	}
	return (0);
}

static int
l_setproctitle_fast(lua_State *L)
{
	if (lua_isnoneornil(L, 1)) {
		setproctitle_fast(NULL);
	} else {
		const char *title, *fmt;

		title = luaL_checkstring(L, 1);
		fmt = lua_toboolean(L, 2) ? "-%s" : "%s";

		setproctitle_fast(fmt, title);
	}
	return (0);
}

static int
l_setregid(lua_State *L)
{
	gid_t rgid, egid;

	rgid = luaL_checkinteger(L, 1);
	egid = luaL_checkinteger(L, 2);

	if (setregid(rgid, egid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setreuid(lua_State *L)
{
	uid_t ruid, euid;

	ruid = luaL_checkinteger(L, 1);
	euid = luaL_checkinteger(L, 2);

	if (setreuid(ruid, euid) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_setsid(lua_State *L)
{
	pid_t sid;

	if ((sid = setsid()) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, sid);
	return (1);
}

static int
l_sleep(lua_State *L)
{
	unsigned int seconds, remaining;

	seconds = luaL_checkinteger(L, 1);

	if ((remaining = sleep(seconds)) == 0) {
		return (0);
	}
	lua_pushinteger(L, remaining);
	return (1);
}

static int
l_swab(lua_State *L)
{
	luaL_Buffer b;
	const void *src;
	void *dst;
	size_t len;

	src = luaL_checklstring(L, 1, &len);

	dst = luaL_buffinitsize(L, &b, len);
	swab(src, dst, len);
	luaL_pushresultsize(&b, len);
	return (1);
}

static int
l_swapon(lua_State *L)
{
	const char *special;

	special = luaL_checkstring(L, 1);

	if (swapon(special) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_swapoff(lua_State *L)
{
	const char *special;
	u_int flags;

	special = luaL_checkstring(L, 1);
	flags = luaL_optinteger(L, 2, 0);

	if (swapoff(special, flags) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_symlink(lua_State *L)
{
	const char *name1, *name2;

	name1 = luaL_checkstring(L, 1);
	name2 = luaL_checkstring(L, 2);

	if (symlink(name1, name2) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_symlinkat(lua_State *L)
{
	const char *name1, *name2;
	int dfd;

	name1 = luaL_checkstring(L, 1);
	dfd = luaL_checkinteger(L, 2);
	name2 = luaL_checkstring(L, 3);

	if (symlinkat(name1, dfd, name2) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
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
l_tcgetpgrp(lua_State *L)
{
	pid_t pgrp;
	int fd;

	fd = checkfd(L, 1);

	if ((pgrp = tcgetpgrp(fd)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, pgrp);
	return (1);
}

static int
l_tcsetpgrp(lua_State *L)
{
	pid_t pgrp;
	int fd;

	fd = checkfd(L, 1);
	pgrp = luaL_checkinteger(L, 2);

	if (tcsetpgrp(fd, pgrp) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
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
	return (success(L));
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
	return (success(L));
}

static int
l_ualarm(lua_State *L)
{
	useconds_t microseconds, interval, remaining;

	microseconds = luaL_checkinteger(L, 1);
	interval = luaL_optinteger(L, 2, 0);

	if ((remaining = ualarm(microseconds, interval)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, remaining);
	return (1);
}

static int
l_undelete(lua_State *L)
{
	const char *path;

	path = luaL_checkstring(L, 1);

	if (undelete(path) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_usleep(lua_State *L)
{
	useconds_t microseconds;

	microseconds = luaL_checkinteger(L, 1);

	if (usleep(microseconds) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static const struct luaL_Reg l_unistd_funcs[] = {
	{"_Fork", l__Fork},
	{"_exit", l__exit},
	{"access", l_access},
	{"acct", l_acct},
	/* alarm obsolete in favor of setitimer(2) */
	/* async_daemon has no implementation */
	/* brk and sbrk omitted in favor of mmap */
	{"chdir", l_chdir},
	{"check_utility_compat", l_check_utility_compat},
	{"chown", l_chown},
	{"chroot", l_chroot},
	{"close", l_close},
	{"close_range", l_close_range},
	{"closefrom", l_closefrom},
	{"confstr", l_confstr},
	{"copy_file_range", l_copy_file_range},
	/* crypt(3) functions in libcrypt crypt(3lua) module */
	{"dup", l_dup},
	{"dup2", l_dup2},
	{"dup3", l_dup3},
	{"eaccess", l_eaccess},
	{"endusershell", l_endusershell},
	{"enter_restricted_mode", l_enter_restricted_mode}, /* XXX */
	/* TODO: which exec* variants are usable? */
	{"faccessat", l_faccessat},
	{"fchdir", l_fchdir},
	{"fchown", l_fchown},
	{"fchownat", l_fchownat},
#if __FreeBSD_version > 1500028
	{"fchroot", l_fchroot},
#endif
	{"fdatasync", l_fdatasync},
	{"feature_present", l_feature_present},
	{"fflagstostr", l_fflagstostr},
	{"fork", l_fork},
	{"fpathconf", l_fpathconf},
	{"fsync", l_fsync},
	{"ftruncate", l_ftruncate},
	{"funlinkat", l_funlinkat},
	{"getcwd", l_getcwd},
	{"getdomainname", l_getdomainname},
	{"getdtablesize", l_getdtablesize},
	{"getegid", l_getegid},
	{"getentropy", l_getentropy},
	{"geteuid", l_geteuid},
	{"getgid", l_getgid},
	{"getgrouplist", l_getgrouplist},
	{"getgroups", l_getgroups},
	/* gethostid/sethostid deprecated in favor of sysctl(3) */
	{"gethostname", l_gethostname},
	{"getlogin", l_getlogin},
	{"getloginclass", l_getloginclass},
	{"getmode", l_getmode},
	/* getopt(3) does not map well to Lua */
	{"getosreldate", l_getosreldate},
	{"getpagesize", l_getpagesize},
	{"getpass", l_getpass},
	{"getpeereid", l_getpeereid},
	{"getpgid", l_getpgid},
	{"getpgrp", l_getpgrp},
	{"getpid", l_getpid},
	{"getppid", l_getppid},
	{"getresgid", l_getresgid},
	{"getresuid", l_getresuid},
	{"getsid", l_getsid},
	{"getuid", l_getuid},
	{"getusershell", l_getusershell},
	/* getwd omitted in favor of getcwd */
	{"initgroups", l_initgroups},
	{"isatty", l_isatty},
	/* remote command API omitted */
	{"issetugid", l_issetugid},
	{"kcmp", l_kcmp},
	{"lchown", l_lchown},
	{"link", l_link},
	{"linkat", l_linkat},
	{"lockf", l_lockf},
	{"lpathconf", l_lpathconf},
	{"lseek", l_lseek},
	{"mkdtemp", l_mkdtemp}, /* XXX: duplicates declaration in stdlib.h */
	{"mknod", l_mknod}, /* XXX: duplicates declaration in sys/stat.h */
	{"mkstemp", l_mkstemp}, /* XXX: duplicates declaration in stdlib.h */
	{"mkstemps", l_mkstemps},
	{"mktemp", l_mktemp}, /* XXX: duplicates declaration in stdlib.h */
	/* nfs internal syscalls omitted */
	/* nice(3) obsolete in favor of setpriority(2) */
	{"pathconf", l_pathconf},
	/* pause(3) obsolete in favor of sigsuspend(2) */
	{"pipe", l_pipe2}, /* same syscall in libc */
	{"pipe2", l_pipe2},
	{"pread", l_pread},
	{"profil", l_profil},
	{"pwrite", l_pwrite},
	/* obsolete re_comp/re_exec omitted in favor of regex(3) */
	{"read", l_read},
	{"readlink", l_readlink},
	{"readlinkat", l_readlinkat},
	{"reboot", l_reboot},
	{"revoke", l_revoke},
	{"rfork", l_rfork},
	/* rfork_thread omitted in favor of pthread_create(3) */
	{"rmdir", l_rmdir},
	/* select(2) declaration omitted, see sys.select */
	{"setdomainname", l_setdomainname},
	{"setegid", l_setegid},
	{"seteuid", l_seteuid},
	{"setgid", l_setgid},
	{"setgroups", l_setgroups},
	{"sethostname", l_sethostname},
	{"setlogin", l_setlogin},
	{"setloginclass", l_setloginclass},
	{"setmode", l_setmode},
	{"setpgid", l_setpgid},
	/* obsolete setpgrp(2) omitted */
	{"setproctitle", l_setproctitle},
	{"setproctitle_fast", l_setproctitle_fast},
	{"setregid", l_setregid},
	{"setresgid", l_setresgid},
	{"setresuid", l_setresuid},
	{"setreuid", l_setreuid},
	/* setrgid/setruid omitted because their use is discouraged */
	{"setsid", l_setsid},
	{"setuid", l_setuid},
	{"setusershell", l_setusershell},
	{"sleep", l_sleep},
	{"strtofflags", l_strtofflags},
	{"swab", l_swab},
	{"swapon", l_swapon},
	{"swapoff", l_swapoff},
	{"symlink", l_symlink},
	{"symlinkat", l_symlinkat},
	{"sync", l_sync},
	/* syscall(2) omitted */
	{"sysconf", l_sysconf},
	{"tcgetpgrp", l_tcgetpgrp},
	{"tcsetpgrp", l_tcsetpgrp},
	{"truncate", l_truncate},
	{"ttyname", l_ttyname},
	{"ualarm", l_ualarm},
	{"undelete", l_undelete},
	{"unlink", l_unlink},
	{"unlinkat", l_unlinkat},
	/* unwhiteout has no implementation */
	{"usleep", l_usleep},
	/* valloc omitted in favor of posix_memalign(3) */
	/* vfork omitted in favor of posix_spawn(3) or fork(2) */
	{"write", l_write},
	{NULL, NULL}
};

static const struct luaL_Reg l_setmode_meta[] = {
	{"__gc", l_setmode_gc},
	{NULL, NULL}
};

int
luaopen_unistd(lua_State *L)
{
	luaL_newmetatable(L, SETMODE_METATABLE);
	luaL_setfuncs(L, l_setmode_meta, 0);

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
#ifdef COPY_FILE_RANGE_USERFLAGS
	DEFINE(COPY_FILE_RANGE_USERFLAGS);
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
	DEFINE(F_ULOCK);
	DEFINE(F_LOCK);
	DEFINE(F_TLOCK);
	DEFINE(F_TEST);
	DEFINE(CLOSE_RANGE_CLOEXEC);
#ifdef CLOSE_RANGE_CLOFORK
	DEFINE(CLOSE_RANGE_CLOFORK);
#endif
	DEFINE(_POSIX_VERSION);
	DEFINE(_POSIX_BARRIERS);
	DEFINE(_POSIX_CPUTIME);
	DEFINE(_POSIX_READER_WRITER_LOCKS);
	DEFINE(_POSIX_REGEXP);
	DEFINE(_POSIX_SHELL);
	DEFINE(_POSIX_SPAWN);
	DEFINE(_POSIX_SPIN_LOCKS);
	DEFINE(_POSIX_THREAD_ATTR_STACKADDR);
	DEFINE(_POSIX_THREAD_ATTR_STACKSIZE);
	DEFINE(_POSIX_THREAD_CPUTIME);
	DEFINE(_POSIX_THREAD_PRIO_INHERIT);
	DEFINE(_POSIX_THREAD_PRIO_PROTECT);
	DEFINE(_POSIX_THREAD_PRIORITY_SCHEDULING);
	DEFINE(_POSIX_THREAD_PROCESS_SHARED);
	DEFINE(_POSIX_THREAD_SAFE_FUNCTIONS);
	DEFINE(_POSIX_THREAD_SPORADIC_SERVER);
	DEFINE(_POSIX_THREADS);
	DEFINE(_POSIX_TRACE);
	DEFINE(_POSIX_TRACE_EVENT_FILTER);
	DEFINE(_POSIX_TRACE_INHERIT);
	DEFINE(_POSIX_TRACE_LOG);
	DEFINE(_POSIX2_C_BIND);
	DEFINE(_POSIX2_C_DEV);
	DEFINE(_POSIX2_CHAR_TERM);
	DEFINE(_POSIX2_FORT_DEV);
	DEFINE(_POSIX2_FORT_RUN);
	DEFINE(_POSIX2_LOCALEDEF);
	DEFINE(_POSIX2_PBS);
	DEFINE(_POSIX2_PBS_ACCOUNTING);
	DEFINE(_POSIX2_PBS_CHECKPOINT);
	DEFINE(_POSIX2_PBS_LOCATE);
	DEFINE(_POSIX2_PBS_MESSAGE);
	DEFINE(_POSIX2_PBS_TRACK);
	DEFINE(_POSIX2_SW_DEV);
	DEFINE(_POSIX2_UPE);
	DEFINE(_V6_ILP32_OFF32);
	DEFINE(_V6_ILP32_OFFBIG);
	DEFINE(_V6_LP64_OFF64);
	DEFINE(_V6_LPBIG_OFFBIG);
	DEFINE(_XOPEN_CRYPT);
	DEFINE(_XOPEN_ENH_I18N);
	DEFINE(_XOPEN_LEGACY);
	DEFINE(_XOPEN_REALTIME);
	DEFINE(_XOPEN_REALTIME_THREADS);
	DEFINE(_XOPEN_UNIX);
	DEFINE(_POSIX2_VERSION);
#undef DEFINE
	return (1);
}
