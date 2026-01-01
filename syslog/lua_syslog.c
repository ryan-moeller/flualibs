/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <syslog.h>

#include <lua.h>
#include <lauxlib.h>

int luaopen_syslog(lua_State *);

static int
l_openlog(lua_State *L)
{
	const char *ident;
	int logopt, facility;

	ident = luaL_checkstring(L, 1);
	logopt = luaL_checkinteger(L, 2);
	facility = luaL_optinteger(L, 3, 0);

	openlog(ident, logopt, facility);
	return (0);
}

static int
l_log_fac(lua_State *L)
{
	int p;

	p = luaL_checkinteger(L, 1);

	lua_pushinteger(L, LOG_FAC(p));
	return (1);
}

static int
l_setlogmask(lua_State *L)
{
	int maskpri;

	maskpri = luaL_checkinteger(L, 1);

	lua_pushinteger(L, setlogmask(maskpri));
	return (1);
}

static int
l_log_mask(lua_State *L)
{
	int pri;

	pri = luaL_checkinteger(L, 1);

	lua_pushinteger(L, LOG_MASK(pri));
	return (1);
}

static int
l_log_upto(lua_State *L)
{
	int pri;

	pri = luaL_checkinteger(L, 1);

	lua_pushinteger(L, LOG_UPTO(pri));
	return (1);
}

static int
l_syslog(lua_State *L)
{
	const char *message;
	int priority;

	priority = luaL_checkinteger(L, 1);
	message = luaL_checkstring(L, 2);

	syslog(priority, "%s", message);
	return (0);
}

static int
l_log_pri(lua_State *L)
{
	int p;

	p = luaL_checkinteger(L, 1);

	lua_pushinteger(L, LOG_PRI(p));
	return (1);
}

static int
l_log_makepri(lua_State *L)
{
	int fac, pri;

	fac = luaL_checkinteger(L, 1);
	pri = luaL_checkinteger(L, 2);

	lua_pushinteger(L, LOG_MAKEPRI(fac, pri));
	return (1);
}

static int
l_closelog(lua_State *L __unused)
{
	closelog();
	return (0);
}

static const struct luaL_Reg l_syslog_funcs[] = {
	{"openlog", l_openlog},
	{"fac", l_log_fac},
	{"setlogmask", l_setlogmask},
	{"mask", l_log_mask},
	{"upto", l_log_upto},
	{"syslog", l_syslog},
	{"pri", l_log_pri},
	{"makepri", l_log_makepri},
	{"closelog", l_closelog},
	{NULL, NULL}
};

int
luaopen_syslog(lua_State *L)
{
	luaL_newlib(L, l_syslog_funcs);
#define DEFINE(ident) ({ \
	lua_pushstring(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(_PATH_LOG);
	DEFINE(_PATH_LOG_PRIV);
#undef DEFINE
#define DEFINE(ident) ({ \
	lua_pushinteger(L, LOG_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(EMERG);
	DEFINE(ALERT);
	DEFINE(CRIT);
	DEFINE(ERR);
	DEFINE(WARNING);
	DEFINE(NOTICE);
	DEFINE(INFO);
	DEFINE(DEBUG);
	DEFINE(PRIMASK);
	DEFINE(KERN);
	DEFINE(USER);
	DEFINE(MAIL);
	DEFINE(DAEMON);
	DEFINE(AUTH);
	DEFINE(SYSLOG);
	DEFINE(LPR);
	DEFINE(NEWS);
	DEFINE(UUCP);
	DEFINE(CRON);
	DEFINE(AUTHPRIV);
	DEFINE(FTP);
	DEFINE(NTP);
	DEFINE(SECURITY);
	DEFINE(CONSOLE);
	DEFINE(LOCAL0);
	DEFINE(LOCAL1);
	DEFINE(LOCAL2);
	DEFINE(LOCAL3);
	DEFINE(LOCAL4);
	DEFINE(LOCAL5);
	DEFINE(LOCAL6);
	DEFINE(LOCAL7);
	DEFINE(NFACILITIES);
	DEFINE(FACMASK);
	DEFINE(PID);
	DEFINE(CONS);
	DEFINE(ODELAY);
	DEFINE(NDELAY);
	DEFINE(NOWAIT);
	DEFINE(PERROR);
#undef DEFINE
	return (1);
}
