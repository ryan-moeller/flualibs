/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fetch.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int luaopen_fetch(lua_State *);

static int
closestream(lua_State *L)
{
	luaL_Stream *stream;

	stream = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	return (luaL_fileresult(L, fclose(stream->f) == 0, NULL));
}

static void
newstream(lua_State *L, FILE *f)
{
	luaL_Stream *stream;

	stream = lua_newuserdatauv(L, sizeof(*stream), 0);
	luaL_setmetatable(L, LUA_FILEHANDLE);
	stream->f = f;
	stream->closef = closestream;
}

static int
fetcherr(lua_State *L)
{
	luaL_pushfail(L);
	lua_pushstring(L, fetchLastErrString);
	lua_pushinteger(L, fetchLastErrCode);
	return (3);
}

static int
l_fetch_get(lua_State *L)
{
	const char *URL, *flags;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	f = fetchGetURL(URL, flags);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	return (1);
}

static int
l_fetch_put(lua_State *L)
{
	const char *URL, *flags;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	f = fetchPutURL(URL, flags);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	return (1);
}

static void
newurlstat(lua_State *L, struct url_stat *stat)
{
	lua_newtable(L);
	lua_pushstring(L, "size");
	lua_pushinteger(L, stat->size);
	lua_rawset(L, -3);
	lua_pushstring(L, "atime");
	lua_pushinteger(L, stat->atime);
	lua_rawset(L, -3);
	lua_pushstring(L, "mtime");
	lua_pushinteger(L, stat->mtime);
	lua_rawset(L, -3);
}

static int
l_fetch_xget(lua_State *L)
{
	struct url_stat stat;
	const char *URL, *flags;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	f = fetchXGetURL(URL, &stat, flags);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	newurlstat(L, &stat);
	return (2);
}

static int
l_fetch_stat(lua_State *L)
{
	struct url_stat stat;
	const char *URL, *flags;
	int res;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	res = fetchStatURL(URL, &stat, flags);
	if (res == -1) {
		return (fetcherr(L));
	}
	newurlstat(L, &stat);
	return (1);
}

static void
newentlist(lua_State *L, struct url_ent *ents)
{
	lua_newtable(L);
	for (int i = 0; ents[i].name[0] != '\0'; i++) {
		lua_newtable(L);
		lua_pushstring(L, "name");
		lua_pushstring(L, ents[i].name);
		lua_rawset(L, -3);
		lua_pushstring(L, "stat");
		newurlstat(L, &ents[i].stat);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, i + 1);
	}
}

static int
l_fetch_list(lua_State *L)
{
	const char *URL, *flags;
	struct url_ent *ents;

	URL = luaL_checkstring(L, 1);
	flags = luaL_optstring(L, 2, NULL);
	ents = fetchListURL(URL, flags);
	if (ents == NULL) {
		return (fetcherr(L));
	}
	newentlist(L, ents);
	free(ents);
	return (1);
}

static int
l_fetch_request(lua_State *L)
{
	const char *URL, *method, *flags, *content_type, *body;
	struct url *u;
	FILE *f;

	URL = luaL_checkstring(L, 1);
	method = luaL_checkstring(L, 2);
	flags = luaL_optstring(L, 3, NULL);
	content_type = luaL_optstring(L, 4, NULL);
	body = luaL_optstring(L, 5, NULL);
	u = fetchParseURL(URL);
	if (u == NULL) {
		return (fetcherr(L));
	}
	f = fetchReqHTTP(u, method, flags, content_type, body);
	fetchFreeURL(u);
	if (f == NULL) {
		return (fetcherr(L));
	}
	newstream(L, f);
	return (1);
}

static void
free_request_fields(struct http_fields *fields)
{
	struct http_field *f1, *f2;

	f1 = TAILQ_FIRST(fields);
	while (f1 != NULL) {
		f2 = TAILQ_NEXT(f1, fields);
		free(f1);
		f1 = f2;
	}
	TAILQ_INIT(fields);
}

static int
fill_request_fields(lua_State *L, int idx, struct http_fields *fields)
{
	struct http_field *field;
	int top, nfields;

	if (!lua_istable(L, idx)) {
		return (0);
	}

	top = lua_gettop(L);
	nfields = luaL_len(L, idx);
	for (int i = 1; i <= nfields; i++) {
		if (lua_geti(L, idx, i) != LUA_TTABLE) {
			goto badfield;
		}
		if (lua_getfield(L, -1, "name") != LUA_TSTRING) {
			goto badfield;
		}
		switch (lua_getfield(L, -2, "value")) {
		case LUA_TSTRING:
		case LUA_TNUMBER:
			break;
		default:
			goto badfield;
		}
		if ((field = malloc(sizeof(*field))) == NULL) {
			goto badfield;
		}
		if ((field->name = lua_tostring(L, -2)) == NULL ||
		    (field->value = lua_tostring(L, -1)) == NULL) {
			free(field);
			goto badfield;
		}
		TAILQ_INSERT_TAIL(fields, field, fields);
		lua_remove(L, -3); /* field[i] table */
		/* leave the strings on the stack */
	}
	return (nfields);
badfield:
	free_request_fields(fields);
	lua_pop(L, lua_gettop(L) - top);
	return (-1);
}

/* call with the trailers arg on the top of the stack (may be nil) */
static void
fill_response_fields(lua_State *L, struct http_fields *fields)
{
	struct http_field *field;

	if (!lua_istable(L, -1)) {
		return;
	}

	TAILQ_FOREACH(field, fields, fields) {
		lua_newtable(L);
		lua_pushstring(L, field->name);
		lua_setfield(L, -2, "name");
		lua_pushstring(L, field->value);
		lua_setfield(L, -2, "value");
		lua_rawseti(L, -2, luaL_len(L, -2) + 1);
	}
}

struct response_body {
	luaL_Stream stream; /* must be first */
	struct http_fields trailers;
	int trailers_ref;
};

static int
closeresponse(lua_State *L)
{
	struct response_body *res_body;
	int res;

	res_body = luaL_checkudata(L, 1, LUA_FILEHANDLE);

	/* Get the response trailers table passed to xrequest (or nil). */
	lua_rawgeti(L, LUA_REGISTRYINDEX, res_body->trailers_ref);
	/* Add any new trailers before we clean up. */
	fill_response_fields(L, &res_body->trailers);
	/* Clean up. */
	fetchFreeResFields(&res_body->trailers);
	luaL_unref(L, LUA_REGISTRYINDEX, res_body->trailers_ref);
	res = fclose(res_body->stream.f);
	return (luaL_fileresult(L, res == 0, NULL));
}

/* call with the trailers arg on the top of the stack (may be nil) */
static struct response_body *
newresponse(lua_State *L)
{
	struct response_body *res_body;
	int ref;

	ref = luaL_ref(L, LUA_REGISTRYINDEX); /* pops trailers */
	res_body = lua_newuserdatauv(L, sizeof(*res_body), 0);
	res_body->stream.f = NULL;
	res_body->stream.closef = closeresponse;
	TAILQ_INIT(&res_body->trailers);
	res_body->trailers_ref = ref;
	luaL_setmetatable(L, LUA_FILEHANDLE);
	return (res_body);
}

static int
l_fetch_trailers(lua_State *L)
{
	struct response_body *res_body;

	res_body = luaL_checkudata(L, 1, LUA_FILEHANDLE);
	/* It may just be a stream, not a response body. */
	if (res_body->stream.closef != closeresponse) {
		return (0);
	}
	/* Get the response trailers table passed to xrequest (or nil). */
	lua_rawgeti(L, LUA_REGISTRYINDEX, res_body->trailers_ref);
	/* Add any new trailers. */
	fill_response_fields(L, &res_body->trailers);
	/* Free the trailers to prevent re-adding them to the same table. */
	fetchFreeResFields(&res_body->trailers);
	return (1);
}

struct request_body {
	lua_State *L;
	struct http_fields trailers;
	int ntrailers;
};

static int
bodyread(void *cookie, char *buf, int n)
{
	struct request_body *req_body = cookie;
	lua_State *L = req_body->L;
	const char *p;
	size_t len;
	int argc;

	if (lua_isfunction(L, 8)) {
		lua_pushvalue(L, 8);
		argc = 1;
	} else {
		lua_getfield(L, 8, "read");
		lua_pushvalue(L, 8);
		argc = 2;
	}
	lua_pushinteger(L, n);

	if (lua_pcall(L, argc, 3, 0) != LUA_OK) {
		lua_pushnil(L);
		lua_insert(L, -2);
		lua_pushinteger(L, EIO);
	}
	if (lua_isnil(L, -3)) {
		errno = lua_tointeger(L, -1);
		return (-1);
	}
	if ((p = lua_tolstring(L, -3, &len)) == NULL) {
		p = lua_typename(L, -3);
		lua_pushnil(L);
		lua_pushfstring(L, "body:read(%d) returned a %s", n, p);
		lua_pushinteger(L, EIO);
		return (-1);
	}
	if (len == 0) {
		lua_pop(L, 3);
		req_body->ntrailers = fill_request_fields(L, 5,
		    &req_body->trailers);
		if (req_body->ntrailers == -1) {
			/* We'll raise an error after cleanup. */
			return (-1);
		}
		return (0);
	}
	if (len > n) {
		len = n;
	}
	memcpy(buf, p, len);
	lua_pop(L, 3);
	return (len);
}

static fpos_t
bodyseek(void *cookie, fpos_t offset, int whence)
{
	struct request_body *req_body = cookie;
	lua_State *L = req_body->L;
	fpos_t pos;

	lua_getfield(L, 8, "seek");
	lua_pushvalue(L, 8);
	switch (whence) {
	case SEEK_SET:
		lua_pushliteral(L, "set");
		break;
	case SEEK_CUR:
		lua_pushliteral(L, "cur");
		break;
	case SEEK_END:
		lua_pushliteral(L, "end");
		break;
	default:
		/* If we end up here it's a fetch(3) problem. */
		lua_pop(L, 2);
		errno = EINVAL;
		return (-1);
	}
	lua_pushinteger(L, offset);

	if (lua_pcall(L, 3, 3, 0) != LUA_OK) {
		lua_pushnil(L);
		lua_insert(L, -2);
		lua_pushinteger(L, EIO);
	}
	if (lua_isnil(L, -3)) {
		errno = lua_tointeger(L, -1);
		return (-1);
	}
	pos = lua_tointeger(L, -3);
	lua_pop(L, 3);
	return (pos);
}

static int
fieldserr(lua_State *L, int idx)
{
	return (luaL_argerror(L, idx,
	    "fields must be a list of {name=\"\",value=\"\"} tables"));
}

static int
l_fetch_xrequest(lua_State *L)
{
	struct url_stat stat;
	struct request_body req_body;
	struct http_fields req_headers, res_headers;
	const char *URL, *method, *flags, *s;
	struct url *u;
	struct response_body *res_body;
	luaL_Stream *stream;
	FILE *b;
	fpos_t (*seekfn)(void *, fpos_t, int);
	size_t len;
	int res_body_idx;
	bool want_res_headers, want_res_trailers;

	TAILQ_INIT(&req_headers);
	TAILQ_INIT(&res_headers);
	TAILQ_INIT(&req_body.trailers);

	URL = luaL_checkstring(L, 1);
	method = luaL_checkstring(L, 2);
	while (lua_gettop(L) < 8) {
		/* Fill in missing parameters. */
		lua_pushnil(L);
	}
	flags = luaL_optstring(L, 3, NULL);
	switch (lua_type(L, 4)) {
	case LUA_TNIL:
	case LUA_TTABLE:
		break;
	default:
		return (luaL_argerror(L, 4,
		    "request headers must be a table or nil"));
	}
	switch (lua_type(L, 5)) {
	case LUA_TNIL:
	case LUA_TTABLE:
		break;
	default:
		return (luaL_argerror(L, 5,
		    "request trailers must be a table or nil"));
	}
	switch (lua_type(L, 6)) {
	case LUA_TNIL:
		want_res_headers = false;
		break;
	case LUA_TTABLE:
		want_res_headers = true;
		break;
	default:
		return (luaL_argerror(L, 6,
		    "response headers must be a table or nil"));
	}
	switch (lua_type(L, 7)) {
	case LUA_TNIL:
		want_res_trailers = false;
		break;
	case LUA_TTABLE:
		want_res_trailers = true;
		break;
	default:
		return (luaL_argerror(L, 7,
		    "response trailers must be a table or nil"));
	}
	seekfn = NULL;
	req_body.ntrailers = 0;
	switch (lua_type(L, 8)) {
	case LUA_TNIL:
		s = NULL;
		stream = NULL;
		b = NULL;
		break;
	case LUA_TUSERDATA:
		s = NULL;
		stream = luaL_checkudata(L, 8, LUA_FILEHANDLE);
		b = stream->f;
		break;
	case LUA_TSTRING:
		s = luaL_checklstring(L, 8, &len);
		stream = NULL;
		if ((b = fmemopen(__DECONST(char *, s), len, "r")) == NULL &&
		    len > 0) {
			return (luaL_error(L, strerror(ENOMEM)));
		}
		break;
	case LUA_TTABLE:
		lua_getfield(L, 8, "read");
		if (!lua_isfunction(L, -1)) {
			return (luaL_argerror(L, 8,
			    "body table must have a 'read' method"));
		}
		lua_getfield(L, 8, "seek");
		if (lua_isfunction(L, -1)) {
			seekfn = bodyseek;
		}
		lua_pop(L, 2);
		/* fallthrough */
	case LUA_TFUNCTION:
		req_body.L = L;
		s = NULL;
		stream = NULL;
		if ((b = funopen(&req_body, bodyread, NULL, seekfn, NULL))
		    == NULL) {
			return (luaL_error(L, strerror(errno)));
		}
		break;
	default:
		return (luaL_argerror(L, 8,
		    "body must be a string, or a function, or a table with "
		    "'read' and optional 'seek' methods, or nil"));
	}

	u = fetchParseURL(URL);
	if (u == NULL) {
		if (stream == NULL && b != NULL) {
			fclose(b);
		}
		return (fetcherr(L));
	}
	if (fill_request_fields(L, 4, &req_headers) == -1) {
		fetchFreeURL(u);
		if (stream == NULL && b != NULL) {
			fclose(b);
		}
		return (fieldserr(L, 4));
	}
	/* Handle trailers up front for bodies that don't invoke bodyread. */
	if ((s != NULL || stream != NULL) &&
	    fill_request_fields(L, 5, &req_body.trailers) == -1) {
		free_request_fields(&req_headers);
		fetchFreeURL(u);
		if (stream == NULL) {
			fclose(b);
		}
		return (fieldserr(L, 5));
	}
	lua_pushvalue(L, 7);
	res_body = newresponse(L); /* XXX: raises error on failure */
	res_body_idx = lua_gettop(L);
	res_body->stream.f = fetchXReqHTTP(u, &stat, method, flags,
	    &req_headers, &req_body.trailers,
	    want_res_headers ? &res_headers : NULL,
	    want_res_trailers ? &res_body->trailers : NULL, b);
	free_request_fields(&req_body.trailers);
	free_request_fields(&req_headers);
	fetchFreeURL(u);
	if (stream == NULL && b != NULL) {
		fclose(b);
	}
	if (res_body->stream.f == NULL) {
		fetchFreeResFields(&res_headers);
		if (req_body.ntrailers == -1) {
			/* error from fill_request_fields */
			return (fieldserr(L, 5));
		}
		if (lua_gettop(L) != res_body_idx + 2 * req_body.ntrailers) {
			/* error results from bodyread or bodyseek */
			return (3);
		}
		return (fetcherr(L));
	}
	lua_pushvalue(L, res_body_idx);
	newurlstat(L, &stat);
	lua_pushvalue(L, 6);
	fill_response_fields(L, &res_headers);
	fetchFreeResFields(&res_headers);
	lua_pushvalue(L, 7);
	return (4);
}

static const struct luaL_Reg l_fetch_funcs[] = {
	{"get", l_fetch_get},
	{"put", l_fetch_put},
	{"xget", l_fetch_xget},
	{"stat", l_fetch_stat},
	{"list", l_fetch_list},
	{"request", l_fetch_request},
	{"xrequest", l_fetch_xrequest},
	{"trailers", l_fetch_trailers},
	{NULL, NULL}
};

int
luaopen_fetch(lua_State *L)
{
	luaL_newlib(L, l_fetch_funcs);
#define DEFINE(ident) ({ \
	lua_pushinteger(L, FETCH_ ## ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(ABORT);
	DEFINE(AUTH);
	DEFINE(DOWN);
	DEFINE(EXISTS);
	DEFINE(FULL);
	DEFINE(INFO);
	DEFINE(MEMORY);
	DEFINE(MOVED);
	DEFINE(NETWORK);
	DEFINE(OK);
	DEFINE(PROTO);
	DEFINE(RESOLV);
	DEFINE(SERVER);
	DEFINE(TEMP);
	DEFINE(TIMEOUT);
	DEFINE(UNAVAIL);
	DEFINE(UNKNOWN);
	DEFINE(URL);
#undef DEFINE
	return (1);
}
