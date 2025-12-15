/*
 * Copyright (c) 2025 Ryan Moeller
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef REFCOUNT_H
#define REFCOUNT_H

#include <stdatomic.h>
#include <stdbool.h>

typedef unsigned long refcount;
typedef _Atomic(refcount) atomic_refcount;

static inline void
refcount_init(atomic_refcount *refs, refcount value)
{
	atomic_init(refs, value);
}

static inline void
refcount_retain(atomic_refcount *refs)
{
	atomic_fetch_add_explicit(refs, 1, memory_order_relaxed);
}

static inline bool
refcount_release(atomic_refcount *refs)
{
	if (atomic_fetch_sub_explicit(refs, 1, memory_order_release) == 1) {
		atomic_thread_fence(memory_order_acquire);
		return (true);
	}
	return (false);
}

#endif
