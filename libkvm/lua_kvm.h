/*
 * Copyright (c) 2026 Ryan Moeller
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <kvm.h>

#include <lua.h>
#include <lauxlib.h>

#include "utils.h"

#define KVM_METATABLE "kvm_t *"

static inline kvm_t *
checkkvm(lua_State *L, int idx)
{
	return (checkcookie(L, idx, KVM_METATABLE));
}
