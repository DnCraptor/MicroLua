_ENV = mlua.Module(...)

local base = require 'hardware.base'
local addressmap = require 'hardware.regs.addressmap'
local psm = require 'hardware.regs.psm'
local regs = require 'hardware.regs.watchdog'
local watchdog = require 'hardware.watchdog'
local time = require 'pico.time'

-- Return true iff the watchdog is enabled.
local function is_enabled()
    return (base.read32(addressmap.WATCHDOG_BASE + regs.CTRL_OFFSET)
            & regs.CTRL_ENABLE_BITS) ~= 0
end

-- Return the value of the SCRATCH4 register.
local function scratch4()
    return base.read32(addressmap.WATCHDOG_BASE + regs.SCRATCH4_OFFSET)
end

-- Prevent the watchdog from resetting any peripherals.
local function inhibit_reset()
    base.hw_clear_bits(addressmap.PSM_BASE + psm.WDSEL_OFFSET,
                       psm.WDSEL_BITS)
end

-- Disable the watchdog.
local function disable()
    base.hw_clear_bits(addressmap.WATCHDOG_BASE + regs.CTRL_OFFSET,
                       regs.CTRL_ENABLE_BITS)
    base.write32(addressmap.WATCHDOG_BASE + regs.SCRATCH4_OFFSET, 0)
    inhibit_reset()
end

function test_watchdog(t)
    t:cleanup(disable)

    disable()
    t:expect(is_enabled()):label("watchdog enabled"):eq(false)
    t:expect(t:expr(watchdog).enable_caused_reboot()):eq(false)

    watchdog.enable(1, true)
    inhibit_reset()
    t:expect(is_enabled()):label("watchdog enabled"):eq(true)
    -- TODO: Fix test failure in next line when flashing with picotool
    t:expect(t:expr(watchdog).get_count()):eq(1000)
    t:expect(scratch4()):label("SCRATCH4"):eq(0x6ab73121)
    watchdog.update()
    time.sleep_ms(1)
    t:expect(t:expr(watchdog).caused_reboot()):eq(true)
    t:expect(t:expr(watchdog).enable_caused_reboot()):eq(true)
end
