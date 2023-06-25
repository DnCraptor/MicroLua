#include "mlua/event.h"

#include "pico/platform.h"
#include "pico/time.h"

#include "mlua/int64.h"
#include "mlua/util.h"

// TODO: Add "performance" counters: dispatch cycles, sleeps

void mlua_event_require(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);
}

spin_lock_t* mlua_event_spinlock;

#define NUM_EVENTS 128
#define EVENTS_SIZE ((NUM_EVENTS + 31) / 32)

typedef struct EventState {
    uint32_t pending[EVENTS_SIZE];
    uint32_t mask[NUM_CORES][EVENTS_SIZE];
} EventState;

static EventState event_state;

char const* mlua_event_claim(MLuaEvent* ev) {
    uint32_t save = mlua_event_lock();
    MLuaEvent e = *ev;
    if (e < NUM_EVENTS) {
        mlua_event_unlock(save);
        return "event already claimed";
    }
    uint core = get_core_num();
    for (uint block = 0; block < EVENTS_SIZE; ++block) {
        uint32_t mask = 0;
        for (uint core = 0; core < NUM_CORES; ++core) {
            mask |= event_state.mask[core][block];
        }
        int idx = __builtin_ffs(~mask);
        if (idx > 0) {
            --idx;
            *ev = block * 32 + idx;
            event_state.mask[core][block] |= 1u << idx;
            mlua_event_unlock(save);
            return NULL;
        }
    }
    mlua_event_unlock(save);
    return "no events available";
}

void mlua_event_unclaim(lua_State* ls, MLuaEvent* ev) {
    uint32_t save = mlua_event_lock();
    MLuaEvent e = *ev;
    if (e >= NUM_EVENTS) {
        mlua_event_unlock(save);
        return;
    }
    uint32_t* pmask = &event_state.mask[get_core_num()][e / 32];
    uint32_t mask = *pmask & ~(1u << (e % 32));
    if (mask == *pmask) {
        mlua_event_unlock(save);
        return;
    }
    *pmask = mask;
    *ev = MLUA_EVENT_UNSET;
    mlua_event_unlock(save);
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
    lua_pushinteger(ls, e);
    lua_pushnil(ls);
    lua_rawset(ls, -3);
    lua_pop(ls, 1);
}

bool mlua_event_enable_irq_arg(lua_State* ls, int index,
                               lua_Integer* priority) {
    int type = lua_type(ls, index);
    switch (type) {
    case LUA_TBOOLEAN:
        if (!lua_toboolean(ls, index)) return false;
        break;
    case LUA_TNONE:
    case LUA_TNIL:
        break;
    default:
        *priority = luaL_checkinteger(ls, index);
        break;
    }
    return true;
}

void mlua_event_set_irq_handler(uint irq, void (*handler)(void),
                                lua_Integer priority) {
    if (priority < 0) {
        irq_set_exclusive_handler(irq, handler);
    } else {
        irq_add_shared_handler(irq, handler, priority);
    }
    irq_set_enabled(irq, true);
}

void mlua_event_remove_irq_handler(uint irq, void (*handler)(void)) {
    irq_set_enabled(irq, false);
    irq_remove_handler(irq, handler);
}

char const* mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                                  void (*handler)(void), int index,
                                  lua_Integer priority) {
    if (!mlua_event_enable_irq_arg(ls, index, &priority)) {  // Disable IRQ
        mlua_event_remove_irq_handler(irq, handler);
        mlua_event_unclaim(ls, ev);
        return NULL;
    }
    char const* err = mlua_event_claim(ev);
    if (err != NULL) return err;
    mlua_event_set_irq_handler(irq, handler, priority);
    return NULL;
}

void __time_critical_func(mlua_event_set)(MLuaEvent ev) {
    if (ev >= NUM_EVENTS) return;
    uint32_t save = mlua_event_lock();
    event_state.pending[ev / 32] |= 1u << (ev % 32);
    mlua_event_unlock(save);
    __sev();
}

void mlua_event_clear(MLuaEvent ev) {
    if (ev >= NUM_EVENTS) return;
    uint32_t save = mlua_event_lock();
    event_state.pending[ev / 32] &= ~(1u << (ev % 32));
    mlua_event_unlock(save);
}

void mlua_event_watch(lua_State* ls, MLuaEvent ev) {
    if (ev >= NUM_EVENTS) {
        luaL_error(ls, "watching disabled event");
        return;
    }
    if (!lua_isyieldable(ls)) {
        luaL_error(ls, "watching event in unyieldable thread");
        return;
    }
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
    switch (lua_rawgeti(ls, -1, ev)) {
    case LUA_TNIL:  // No watchers
        lua_pop(ls, 1);
        lua_pushthread(ls);
        lua_rawseti(ls, -2, ev);
        lua_pop(ls, 1);
        return;
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (lua_rawequal(ls, -2, -1)) {  // Already registered
            lua_pop(ls, 3);
            return;
        }
        lua_createtable(ls, 0, 2);
        lua_rotate(ls, -3, 1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -4);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_rawseti(ls, -2, ev);
        lua_pop(ls, 1);
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_pop(ls, 2);
        return;
    default:
        lua_pop(ls, 2);
        return;
    }
}

void mlua_event_unwatch(lua_State* ls, MLuaEvent ev) {
    if (ev >= NUM_EVENTS) return;
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
    switch (lua_rawgeti(ls, -1, ev)) {
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (!lua_rawequal(ls, -2, -1)) {  // Not the current thread
            lua_pop(ls, 3);
            return;
        }
        lua_pop(ls, 2);
        lua_pushnil(ls);
        lua_rawseti(ls, -2, ev);
        lua_pop(ls, 1);
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushnil(ls);
        lua_rawset(ls, -3);
        lua_pop(ls, 2);
        return;
    default:
        lua_pop(ls, 2);
        return;
    }
}

int mlua_event_yield(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                     int nresults) {
    lua_yieldk(ls, nresults, ctx, cont);
    return luaL_error(ls, "unable to yield");
}

int mlua_event_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                       int index) {
    if (index != 0) {
        lua_pushvalue(ls, index);
    } else {
        lua_pushboolean(ls, true);
    }
    return mlua_event_yield(ls, cont, ctx, 1);
}

static int mlua_event_wait_1(lua_State* ls, MLuaEvent event,
                             lua_CFunction try_get, int index);
static int mlua_event_wait_2(lua_State* ls, int status, lua_KContext ctx);

int mlua_event_wait(lua_State* ls, MLuaEvent event, lua_CFunction try_get,
                    int index) {
    if (event >= NUM_EVENTS) return luaL_error(ls, "wait for unclaimed event");
    int res = try_get(ls);
    if (res >= 0) return res;
    mlua_event_watch(ls, event);
    return mlua_event_wait_1(ls, event, try_get, index);
}

static int mlua_event_wait_1(lua_State* ls, MLuaEvent event,
                             lua_CFunction try_get, int index) {
    lua_pushcfunction(ls, try_get);
    lua_pushinteger(ls, index);
    return mlua_event_suspend(ls, &mlua_event_wait_2, event, index);
}

static int mlua_event_wait_2(lua_State* ls, int status, lua_KContext ctx) {
    lua_CFunction try_get = lua_tocfunction(ls, -2);
    int index = lua_tointeger(ls, -1);
    lua_pop(ls, 2);  // Restore the stack for try_get
    int res = try_get(ls);
    if (res < 0) return mlua_event_wait_1(ls, (MLuaEvent)ctx, try_get, index);
    mlua_event_unwatch(ls, (MLuaEvent)ctx);
    return res;
}

static int mod_dispatch(lua_State* ls) {
    absolute_time_t deadline = from_us_since_boot(mlua_check_int64(ls, 2));
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
    uint32_t* masks = event_state.mask[get_core_num()];
    for (;;) {
        // Check for pending events and resume the corresponding watcher
        // threads.
        bool wake = false;
        for (uint block = 0; block < EVENTS_SIZE; ++block) {
            uint32_t* pending = &event_state.pending[block];
            uint32_t save = mlua_event_lock();
            uint32_t active = *pending;
            uint32_t mask = masks[block];
            *pending = active & ~mask;
            mlua_event_unlock(save);
            active &= mask;
            for (;;) {
                int idx = __builtin_ffs(active);
                if (idx == 0) break;
                --idx;
                active &= ~(1u << idx);
                switch (lua_rawgeti(ls, -1, block * 32 + idx)) {
                case LUA_TTHREAD:  // A single watcher
                    lua_pushvalue(ls, 1);
                    lua_rotate(ls, -2, 1);
                    lua_call(ls, 1, 1);
                    wake = wake || lua_toboolean(ls, -1);
                    lua_pop(ls, 1);
                    break;
                case LUA_TTABLE:  // Multiple watchers
                    lua_pushnil(ls);
                    while (lua_next(ls, -2)) {
                        lua_pop(ls, 1);
                        lua_pushvalue(ls, 1);
                        lua_pushvalue(ls, -2);
                        lua_call(ls, 1, 1);
                        wake = wake || lua_toboolean(ls, -1);
                        lua_pop(ls, 1);
                    }
                    lua_pop(ls, 1);
                    break;
                default:
                    lua_pop(ls, 1);
                    break;
                }
            }
        }

        // Return if at least one thread was resumed or the deadline has passed.
        if (wake || is_nil_time(deadline) || time_reached(deadline)) break;

        // Wait for events, up to the deadline.
        if (!is_at_the_end_of_time(deadline)) {
            best_effort_wfe_or_timeout(deadline);
        } else {
            __wfe();
        }
    }
    return 0;
}

int mod_set_thread_metatable(lua_State* ls) {
    lua_pushthread(ls);
    lua_pushvalue(ls, 1);
    lua_setmetatable(ls, -2);
    return 0;
}

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(dispatch),
    MLUA_SYM(set_thread_metatable),
#undef MLUA_SYM
};

static __attribute__((constructor)) void init(void) {
    mlua_event_spinlock = spin_lock_instance(next_striped_spin_lock_num());
}

int luaopen_mlua_event(lua_State* ls) {
    mlua_require(ls, "mlua.int64", false);

    lua_newtable(ls);  // Watcher thread table
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &event_state);

    // Create the module.
    mlua_new_table(ls, module_regs);
    return 1;
}
