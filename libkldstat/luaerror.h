/*
 * Fix some compiler warnings.
 */

#ifndef LUAERROR_FIX_H
#define LUAERROR_FIX_H

#if defined(LUA_VERSION_NUM) && defined(LUA_API)
LUA_API int   (lua_error) (lua_State *L) __dead2;
#endif
#if defined(LUA_ERRFILE) && defined(LUALIB_API)
LUALIB_API int (luaL_argerror) (lua_State *L, int arg, const char *extramsg) __dead2;
LUALIB_API int (luaL_typeerror) (lua_State *L, int arg, const char *tname) __dead2;
LUALIB_API int (luaL_error) (lua_State *L, const char *fmt, ...) __dead2;
#endif

#endif
