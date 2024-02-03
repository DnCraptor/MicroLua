// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_HOST_MLUA_EVENT_H
#define _MLUA_LIB_HOST_MLUA_EVENT_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// An event.
typedef struct MLuaEvent {
    uintptr_t dummy;
} MLuaEvent;

// Return true iff the event is enabled.
static inline bool mlua_event_enabled(MLuaEvent const* ev) { return false; }

// A function that resumes a thread, and returns true iff the thread was
// actually resumed.
typedef bool (*MLuaResume)(lua_State*, lua_State*);

// Dispatch pending events.
void mlua_event_dispatch(lua_State* ls, uint64_t deadline, MLuaResume resume);

#ifdef __cplusplus
}
#endif

#endif
