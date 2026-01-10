/*
 * Copyright (c) 2026 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sys/param.h>
#include <sys/uio.h>
#include <aio.h>
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "sys/uio/lua_uio.h"
#include "libpthread/refcount.h"
#include "signal/lua_signal.h"
#include "luaerror.h"
#include "utils.h"

#define AIOCB_METATABLE "struct aiocb"

int luaopen_aio(lua_State *);

/* Specialized table copy for a flat array. */
static inline void
tcopy(lua_State *L, int idx)
{
	size_t len;

	idx = lua_absindex(L, idx);
	len = luaL_len(L, idx);
	lua_createtable(L, len, 0);
	for (size_t i = 1; i <= len; i++) {
		lua_geti(L, idx, i);
		lua_rawseti(L, -2, i);
	}
}

/*
 * Valid state transitions:
 *
 * UNINIT -> PENDING : before submission for read
 * PENDING -> UNINIT : submission or aio_waitcomplete for read failed
 * INIT -> PENDING : before submission for write/mlock/fsync
 * PENDING -> INIT : submission or aio_waitcomplete for write/mlock/fsync failed
 * PENDING -> RETURNING : before aio_return
 * RETURNING -> INIT : after aio_return succeeds
 * RETURNING -> UNINIT/INIT : after aio_return fails (last state is restored)
 *
 * Any other transition is invalid.
 */

enum aiocb_state {
	CB_UNINIT,    /* buffer contains initialized memory */
	CB_INIT,      /* buffer contains initialized memory or is NULL */
	CB_PENDING,   /* buffer in use by the kernel */
	CB_RETURNING, /* phase one of two-phase commit for aio_return */
};

/* We allocate space for a few extra fields under the hood. */
struct rcaiocb {
	struct aiocb cb;
	atomic_refcount refs;
	_Atomic(enum aiocb_state) state; /* current state */
};

#define aio_flags __spare__[0]
#define aio_state __spare__[1] /* state before submission, restored on error */

/* aio_flags */
#define CB_LOCAL  (1 << 0) /* aiocb buffer points into Lua state */
#define CB_VECTOR (1 << 1) /* aiocb buffer is an array of iovecs */

static inline bool
rcaiocb_trybeginwrite(struct rcaiocb *rccb, enum aiocb_state *state)
{
	/* TODO: skip atomics for local buffer */
	return (atomic_compare_exchange_strong(&rccb->state,
	    ({ *state = CB_INIT; state; }), CB_PENDING));
}

static inline bool
rcaiocb_trybeginread(struct rcaiocb *rccb, enum aiocb_state *state)
{
	/* TODO: skip atomics for local buffer */
	return (atomic_compare_exchange_strong(&rccb->state,
	    ({ *state = CB_UNINIT; state; }), CB_PENDING) ||
	    rcaiocb_trybeginwrite(rccb, state));
}

static inline bool
rcaiocb_trybeginreturn(struct rcaiocb *rccb, enum aiocb_state *state)
{
	/* TODO: skip atomics for local buffer */
	return (atomic_compare_exchange_strong(&rccb->state,
	    ({ *state = CB_PENDING; state; }), CB_RETURNING));
}

static inline void
rcaiocb_rollback(struct rcaiocb *rccb, enum aiocb_state state)
{
	/* TODO: skip atomics for local buffer */
	atomic_store(&rccb->state, state);
}

static inline void
rcaiocb_commit(struct rcaiocb *rccb)
{
	/* TODO: skip atomics for local buffer */
	atomic_store(&rccb->state, (rccb->cb.aio_state = CB_INIT));
}

static inline struct rcaiocb *
aiocb_container(struct aiocb *cb)
{
	return (__containerof(cb, struct rcaiocb, cb));
}

static inline bool
aiocb_islocal(struct aiocb *cb)
{
	return ((cb->aio_flags & CB_LOCAL) != 0);
}

static inline bool
aiocb_isshared(struct aiocb *cb)
{
	return ((cb->aio_flags & CB_LOCAL) == 0);
}

static inline bool
aiocb_isvector(struct aiocb *cb)
{
	return ((cb->aio_flags & CB_VECTOR) != 0);
}

static inline void
freeiovecs(struct iovec *iov, size_t iovcnt)
{
	for (size_t i = 0; i < iovcnt; i++) {
		free(iov[i].iov_base);
	}
	free(iov);
}

static inline void
checkiovecs(lua_State *L, int idx, struct rcaiocb *rccb)
{
	struct iovec *iov;
	size_t iovcnt, ninit;

	luaL_checktype(L, idx, LUA_TTABLE);

	iovcnt = luaL_len(L, idx);
	if ((iov = calloc(iovcnt, sizeof(*iov))) == NULL) {
		fatal(L, "calloc", ENOMEM);
	}
	for (size_t i = ninit = 0; i < iovcnt; i++) {
		const char *init;
		void *base;
		size_t len;

		if (lua_geti(L, idx, i + 1) == LUA_TSTRING) {
			init = lua_tolstring(L, -1, &len);
		} else if (lua_isinteger(L, -1)) {
			init = NULL;
			len = lua_tointeger(L, -1);
		} else {
			freeiovecs(iov, iovcnt);
			luaL_argerror(L, idx, "invalid buffer description");
		}
		if ((base = malloc(len)) == NULL) {
			freeiovecs(iov, iovcnt);
			fatal(L, "malloc", ENOMEM);
		}
		if (init != NULL) {
			memcpy(base, init, len);
			ninit++;
		}
		iov[i].iov_base = base;
		iov[i].iov_len = len;
		lua_pop(L, 1);
	}
	if (ninit > 0) {
		if (ninit != iovcnt) {
			freeiovecs(iov, iovcnt);
			luaL_argerror(L, idx, "invalid buffer description");
		}
		rcaiocb_commit(rccb);
	}
	rccb->cb.aio_iov = iov;
	rccb->cb.aio_iovcnt = iovcnt;
	rccb->cb.aio_flags |= CB_VECTOR;
}

static inline void
checkbuf(lua_State *L, int idx, struct rcaiocb *rccb)
{
	const char *init;
	void *base;
	size_t len;

	if (lua_type(L, idx) == LUA_TSTRING) {
		init = lua_tolstring(L, idx, &len);
	} else if (lua_isinteger(L, idx)) {
		init = NULL;
		len = lua_tointeger(L, idx);
	} else {
		luaL_argerror(L, idx, "invalid buffer description");
	}
	if ((base = malloc(len)) == NULL) {
		fatal(L, "malloc", ENOMEM);
	}
	if (init != NULL) {
		memcpy(base, init, len);
		rcaiocb_commit(rccb);
	}
	rccb->cb.aio_buf = base;
	rccb->cb.aio_nbytes = len;
}

static int
l_aiocb_shared(lua_State *L)
{
	struct rcaiocb *rccb;
	struct aiocb *cb;
	int error;

	/*
	 * We allocate these objects from the heap in order to support
	 * multithreaded environments.  In particular, aio_waitcomplete() can
	 * return an aiocb from any thread in the process.  Another practical
	 * use case is separate submission and completion threads.
	 */
	if ((rccb = calloc(1, sizeof(*rccb))) == NULL) {
		return (fatal(L, "calloc", ENOMEM));
	}
	refcount_init(&rccb->refs, 1);
#if CB_UNINIT != 0 /* avoid zeroing memory twice */
	rccb->cb.aio_state = CB_UNINIT;
	atomic_store(&rccb->state, CB_UNINIT);
#endif
	lua_settop(L, 5); /* space for optional args */
	new(L, rccb, AIOCB_METATABLE);

	cb = &rccb->cb;
	cb->aio_fildes = checkfd(L, 1); /* XXX: user must keep fd open */
	cb->aio_offset = luaL_checkinteger(L, 2);
	switch (lua_type(L, 3)) {
	case LUA_TNIL:
		rcaiocb_commit(rccb);
		break;
	case LUA_TTABLE:
		checkiovecs(L, 3, rccb);
		break;
	default:
		checkbuf(L, 3, rccb);
		break;
	}
	cb->aio_lio_opcode = luaL_optinteger(L, 4, 0);
	if (!lua_isnoneornil(L, 5)) {
		checksigevent(L, 5, &cb->aio_sigevent);
	}
	return (1);
}

static int
l_aiocb_cookie(lua_State *L)
{
	checkcookieuv(L, 1, AIOCB_METATABLE);

	return (1);
}

static int
l_aiocb_retain(lua_State *L)
{
	struct rcaiocb *rccb;

	rccb = checklightuserdata(L, 1);
	luaL_argcheck(L, aiocb_isshared(&rccb->cb), 1, "aiocb not shareable");

	refcount_retain(&rccb->refs);
	return (new(L, rccb, AIOCB_METATABLE));
}

static int
l_aiocb_local(lua_State *L)
{
	/* TODO: const aiocb variant optimized for single-thread use */
	return (luaL_error(L, "TODO"));
}

static int
l_aiocb_eq(lua_State *L)
{
	struct rcaiocb *a, *b;

	a = checkcookie(L, 1, AIOCB_METATABLE);
	b = checkcookie(L, 2, AIOCB_METATABLE);

	lua_pushboolean(L, a == b);
	return (1);
}

static int
l_aiocb_free(lua_State *L)
{
	char errmsg[NL_TEXTMAX];
	char why[128];
	struct rcaiocb *rccb;
	struct aiocb *cb;
	int error;
	bool shared, vector;

	rccb = checkcookienull(L, 1, AIOCB_METATABLE);

	if (rccb == NULL) {
		return (0);
	}
	cb = &rccb->cb;
	shared = aiocb_isshared(cb);
	if (shared && !refcount_release(&rccb->refs)) {
		return (0);
	}
	/* We have exclusive access. */
	if (atomic_load(&rccb->state) == CB_PENDING) {
		const struct aiocb *cbs[1];

		/*
		 * We must cancel or complete active IO before we continue, or
		 * die trying.  Otherwise, the kernel may access memory that is
		 * no longer valid.
		 */
		switch (aio_cancel(cb->aio_fildes, cb)) {
		case -1:
			error = errno;
			strerror_r(error, errmsg, sizeof(errmsg));
			snprintf(why, sizeof(why), "FATAL: aio_cancel failed: "
			    "%s (%d); cannot safely continue", errmsg, error);
			abort2(why, 0, NULL);
		case AIO_NOTCANCELED:
			/* XXX: We have to wait for raw disk IO to complete. */
			cbs[0] = cb;
retry:
			if (aio_suspend(cbs, nitems(cbs), NULL) == -1) {
				if ((error = errno) == EINTR) {
					goto retry;
				}
				strerror_r(error, errmsg, sizeof(errmsg));
				snprintf(why, sizeof(why), "FATAL: aio_suspend "
				    "failed: %s (%d); cannot safely continue",
				    errmsg, error);
				abort2(why, 0, NULL);
			}
			break;
		default:
			/* It is safe to continue. */
			break;
		}
		aio_return(cb); /* XXX: status ignored */
	}
	vector = aiocb_isvector(cb);
	if (shared && vector) {
		freeiovecs(__DEVOLATILE(struct iovec *, cb->aio_iov),
		    cb->aio_iovcnt);
	} else if (shared || vector) {
		free(__DEVOLATILE(void *, cb->aio_buf));
	}
	free(rccb);
	setcookie(L, 1, NULL);
	return (0);
}

static int
l_aiocb_index(lua_State *L)
{
	struct rcaiocb *rccb;
	struct aiocb *cb;
	const char *field;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);
	field = luaL_checkstring(L, 2);

	lua_getmetatable(L, 1);
	if (lua_getfield(L, -1, field) != LUA_TNIL) {
		return (1);
	}
	cb = &rccb->cb;
#define INTFIELD(name) ({ \
	if (strcmp(field, #name) == 0) { \
		lua_pushinteger(L, cb->aio_ ## name); \
		return (1); \
	} \
})
	INTFIELD(fildes);
	INTFIELD(offset);
	INTFIELD(lio_opcode);
#undef INTFIELD
	if (strcmp(field, "buf") == 0) {
		if (aiocb_isvector(cb)) {
			/* XXX: could concat */
			return (0);
		}
		if (atomic_load(&rccb->state) != CB_INIT) {
			/* XXX: could allow during write */
			return (luaL_error(L, "buffer not available"));
		}
		if (aiocb_isshared(cb)) {
			/* XXX: could cache this in ref slot */
			lua_pushlstring(L, __DEVOLATILE(void *, cb->aio_buf),
			    cb->aio_nbytes);
		} else {
			getref(L, 1);
		}
		return (1);
	}
	if (strcmp(field, "iov") == 0) {
		if (aiocb_isvector(cb)) {
			/* XXX: could pack */
			return (0);
		}
		if (atomic_load(&rccb->state) != CB_INIT) {
			/* XXX: could allow during write */
			return (luaL_error(L, "buffers not available"));
		}
		if (aiocb_isshared(cb)) {
			/* XXX: could cache this in ref slot */
			pushriovecs(L, __DEVOLATILE(struct iovec *,
			    cb->aio_iov), cb->aio_iovcnt);
		} else {
			getref(L, 1);
			tcopy(L, -1);
		}
		return (1);
	}
	if (strcmp(field, "sigevent") == 0) {
		pushsigevent(L, &cb->aio_sigevent);
		return (1);
	}
	/* No matching field name. */
	return (0);
}

static int
l_aio_read(lua_State *L)
{
	struct rcaiocb *rccb;
	struct aiocb *cb;
	int flags;
	enum aiocb_state state;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);
	flags = luaL_optinteger(L, 2, 0);

	cb = &rccb->cb;
	luaL_argcheck(L, aiocb_isshared(cb), 1, "const aiocb buffer");
	if (!rcaiocb_trybeginread(rccb, &state)) {
		return (luaL_argerror(L, 1, "buffer not available"));
	}
	if (aiocb_isvector(cb)) {
		if (aio_readv(cb) == -1) {
			int error = errno;

			rcaiocb_rollback(rccb, state);
			return (fail(L, error));
		}
	} else {
#if __FreeBSD_version > 1500013 || \
    (__FreeBSD_version > 1400508 && __FreeBSD_version < 1500000)
		if (aio_read2(cb, flags) == -1) {
#else
		(void)flags;
		if (aio_read(cb) == -1) {
#endif
			int error = errno;

			rcaiocb_rollback(rccb, state);
			return (fail(L, error));
		}
	}
	return (success(L));
}

static int
l_aio_write(lua_State *L)
{
	struct rcaiocb *rccb;
	struct aiocb *cb;
	int flags;
	enum aiocb_state state;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);
	flags = luaL_optinteger(L, 2, 0);

	cb = &rccb->cb;
	if (!rcaiocb_trybeginwrite(rccb, &state)) {
		return (luaL_argerror(L, 1, "buffer not available"));
	}
	if (aiocb_isvector(cb)) {
		if (aio_writev(cb) == -1) {
			int error = errno;

			rcaiocb_rollback(rccb, state);
			return (fail(L, error));
		}
	} else {
#if __FreeBSD_version > 1500013 || \
    (__FreeBSD_version > 1400508 && __FreeBSD_version < 1500000)
		if (aio_write2(cb, flags) == -1) {
#else
		(void)flags;
		if (aio_write(cb) == -1) {
#endif
			int error = errno;

			rcaiocb_rollback(rccb, state);
			return (fail(L, error));
		}
	}
	return (success(L));
}

static int
l_aio_error(lua_State *L)
{
	char errmsg[NL_TEXTMAX];
	const struct rcaiocb *rccb;
	int error;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);

	if ((error = aio_error(&rccb->cb)) == -1) {
		return (fail(L, errno));
	}
	strerror_r(error, errmsg, sizeof(errmsg));
	lua_pushstring(L, errmsg);
	lua_pushinteger(L, error);
	return (2);
}

static int
l_aio_return(lua_State *L)
{
	struct rcaiocb *rccb;
	ssize_t result;
	enum aiocb_state state;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);

	if (!rcaiocb_trybeginreturn(rccb, &state)) {
		return (luaL_argerror(L, 1, "buffer not pending"));
	}
	if ((result = aio_return(&rccb->cb)) == -1) {
		int error = errno;

		rcaiocb_rollback(rccb, rccb->cb.aio_state);
		return (fail(L, error));
	}
	rcaiocb_commit(rccb);
	lua_pushinteger(L, result);
	return (1);
}

static int
l_cancel(lua_State *L)
{
	int fd, status;

	fd = checkfd(L, 1);

	if ((status = aio_cancel(fd, NULL)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, status);
	return (1);
}

static int
l_aio_cancel(lua_State *L)
{
	struct rcaiocb *rccb;
	int status;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);

	if (atomic_load(&rccb->state) != CB_PENDING) {
		return (luaL_argerror(L, 1, "buffer not pending"));
	}
	if ((status = aio_cancel(rccb->cb.aio_fildes, &rccb->cb)) == -1) {
		return (fail(L, errno));
	}
	lua_pushinteger(L, status);
	return (1);
}

static int
l_suspend(lua_State *L)
{
	struct timespec timeout;
	const struct timespec *timeoutp;
	const struct aiocb **cbs;
	int niocb;

	luaL_checktype(L, 1, LUA_TTABLE);
	if (lua_isnoneornil(L, 2)) {
		timeoutp = NULL;
	} else {
		timeout.tv_sec = luaL_checkinteger(L, 2);
		timeout.tv_nsec = luaL_optinteger(L, 3, 0);
		timeoutp = &timeout;
	}

	niocb = luaL_len(L, 1);
	/* Allocate the array as userdata for automatic cleanup. */
	cbs = lua_newuserdatauv(L, niocb * sizeof(*cbs), 0);
	for (int i = 0; i < niocb; i++) {
		const struct rcaiocb *rccb;

		lua_geti(L, 1, i + 1);
		rccb = checkcookie(L, -1, AIOCB_METATABLE);
		luaL_argcheck(L, atomic_load(&rccb->state) == CB_PENDING, 1,
		    "buffer not pending");
		cbs[i] = &rccb->cb;
		lua_pop(L, 1);
	}
	if (aio_suspend(cbs, niocb, timeoutp) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_aio_suspend(lua_State *L)
{
	struct timespec timeout;
	const struct timespec *timeoutp;
	const struct aiocb *cbs[1];
	const struct rcaiocb *rccb;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);
	luaL_argcheck(L, atomic_load(&rccb->state) == CB_PENDING, 1,
	    "buffer not pending");
	if (lua_isnoneornil(L, 2)) {
		timeoutp = NULL;
	} else {
		timeout.tv_sec = luaL_checkinteger(L, 2);
		timeout.tv_nsec = luaL_optinteger(L, 3, 0);
		timeoutp = &timeout;
	}

	cbs[0] = &rccb->cb;
	if (aio_suspend(cbs, nitems(cbs), timeoutp) == -1) {
		return (fail(L, errno));
	}
	return (success(L));
}

static int
l_aio_mlock(lua_State *L)
{
	struct rcaiocb *rccb;
	enum aiocb_state state;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);

	if (!rcaiocb_trybeginwrite(rccb, &state)) {
		/* TODO: better error messages? */
		return (luaL_argerror(L, 1, "buffer not available"));
	}
	if (aio_mlock(&rccb->cb) == -1) {
		int error = errno;

		rcaiocb_rollback(rccb, state);
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_aio_fsync(lua_State *L)
{
	struct rcaiocb *rccb;
	int op;
	enum aiocb_state state;

	rccb = checkcookie(L, 1, AIOCB_METATABLE);
	op = luaL_checkinteger(L, 2);

	if (!rcaiocb_trybeginwrite(rccb, &state)) {
		/* TODO: better error messages? */
		return (luaL_argerror(L, 1, "buffer not available"));
	}
	if (aio_fsync(op, &rccb->cb) == -1) {
		int error = errno;

		rcaiocb_rollback(rccb, state);
		return (fail(L, error));
	}
	return (success(L));
}

static inline void
rollback_states(struct aiocb **cbs, enum aiocb_state *states, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		rcaiocb_rollback(aiocb_container(cbs[i]), states[i]);
	}
}

static int
l_lio_listio(lua_State *L)
{
	struct sigevent sig, *sigp;
	struct aiocb **cbs;
	enum aiocb_state *states;
	int mode, nent, i;

	mode = luaL_checkinteger(L, 1);
	luaL_checktype(L, 3, LUA_TTABLE);
	if (lua_isnoneornil(L, 4)) {
		sigp = NULL;
	} else {
		sigp = &sig;
		checksigevent(L, 4, sigp);
	}

	nent = luaL_len(L, 2);
	/* Allocate the arrays as userdata for automatic cleanup. */
	cbs = lua_newuserdatauv(L, nent * (sizeof(*cbs) + sizeof(*states)), 0);
	states = (enum aiocb_state *)(cbs + nent);
	for (i = 0; i < nent; i++) {
		struct rcaiocb *rccb;
		bool pending;

		lua_geti(L, 2, i + 1);
		if ((rccb = testcookie(L, -1, AIOCB_METATABLE)) == NULL) {
			rollback_states(cbs, states, i);
			return (luaL_argerror(L, 2, "invalid aiocb"));
		}
		if ((rccb->cb.aio_lio_opcode & LIO_READ) == 0) {
			pending = rcaiocb_trybeginwrite(rccb, &states[i]);
		} else {
			pending = rcaiocb_trybeginread(rccb, &states[i]);
		}
		if (!pending) {
			rollback_states(cbs, states, i + 1);
			return (luaL_argerror(L, 2, "buffer not initialized"));
		}
		cbs[i] = &rccb->cb;
		lua_pop(L, 1);
	}
	if (lio_listio(mode, cbs, nent, sigp) == -1) {
		int error = errno;

		rollback_states(cbs, states, nent);
		return (fail(L, error));
	}
	return (success(L));
}

static int
l_aio_waitcomplete(lua_State *L)
{
	struct timespec timeout;
	struct timespec *timeoutp;
	struct rcaiocb *rccb;
	struct aiocb *cb;
	ssize_t result;
	int error;

	if (lua_isnoneornil(L, 1)) {
		timeoutp = NULL;
	} else {
		timeout.tv_sec = luaL_checkinteger(L, 1);
		timeout.tv_nsec = luaL_optinteger(L, 2, 0);
		timeoutp = &timeout;
	}

	/*
	 * XXX: We have no opportunity to enter the RETURNING state to prevent
	 * racing with aio_return() of the same cb from another thread.  It is
	 * up to the caller to avoid making this mistake.
	 */
	cb = NULL;
	if ((result = aio_waitcomplete(&cb, timeoutp)) == -1) {
		if (cb == NULL) {
			return (fail(L, errno));
		}
		error = errno;
	}
	if (aiocb_islocal(cb)) {
		/* TODO: decide how to handle this */
		return (luaL_error(L, "TODO"));
	}
	/*
	 * XXX: This is bogus unless only this module submits AIO requests.  The
	 * caller asserts there is no other source of AIO requests in this
	 * process.
	 */
	rccb = aiocb_container(cb);
	refcount_retain(&rccb->refs);
	new(L, rccb, AIOCB_METATABLE);
	if (result == -1) {
		rcaiocb_rollback(rccb, cb->aio_state);
		fail(L, error);
		return (4);
	}
	rcaiocb_commit(rccb);
	lua_pushinteger(L, result);
	return (2);
}

static const struct luaL_Reg l_aio_funcs[] = {
	{"cancel", l_cancel},
	{"suspend", l_suspend},
	{"listio", l_lio_listio},
	{"waitcomplete", l_aio_waitcomplete},
	{NULL, NULL}
};

static const struct luaL_Reg l_aiocb_funcs[] = {
	{"shared", l_aiocb_shared},
	{"retain", l_aiocb_retain},
#if 0 /* TODO */
	{"local", l_aiocb_local},
#endif
	{NULL, NULL}
};

static const struct luaL_Reg l_aiocb_meta[] = {
	{"__close", l_aiocb_free},
	{"__eq", l_aiocb_eq},
	{"__gc", l_aiocb_free},
	{"__index", l_aiocb_index},
	/* TODO: :buf(len) method to get partial contents? what about iov? */
	{"cookie", l_aiocb_cookie},
	{"read", l_aio_read}, /* aio_read2, aio_readv */
	{"write", l_aio_write}, /* aio_write2, aio_writev */
	{"error", l_aio_error},
	{"_return", l_aio_return}, /* XXX: return is a keyword */
	{"cancel", l_aio_cancel},
	{"suspend", l_aio_suspend},
	{"mlock", l_aio_mlock},
	{"fsync", l_aio_fsync},
	{NULL, NULL}
};

int
luaopen_aio(lua_State *L)
{
	luaL_newmetatable(L, AIOCB_METATABLE);
	luaL_setfuncs(L, l_aiocb_meta, 0);

	luaL_newlib(L, l_aio_funcs);
	luaL_newlib(L, l_aiocb_funcs);
	lua_setfield(L, -2, "aiocb");
#define DEFINE(ident) ({ \
	lua_pushinteger(L, ident); \
	lua_setfield(L, -2, #ident); \
})
	DEFINE(AIO_CANCELED);
	DEFINE(AIO_NOTCANCELED);
	DEFINE(AIO_ALLDONE);

	DEFINE(LIO_NOP);
	DEFINE(LIO_WRITE);
	DEFINE(LIO_READ);
	DEFINE(LIO_VECTORED);
	DEFINE(LIO_WRITEV);
	DEFINE(LIO_READV);
#ifdef LIO_FOFFSET
	DEFINE(LIO_FOFFSET);
#endif

#ifdef AIO_OP2_FOFFSET
	DEFINE(AIO_OP2_FOFFSET);
#endif
#ifdef AIO_OP2_VECTORED
	DEFINE(AIO_OP2_VECTORED);
#endif

	DEFINE(LIO_NOWAIT);
	DEFINE(LIO_WAIT);

	DEFINE(AIO_LISTIO_MAX);
#undef DEFINE
	return (1);
}
