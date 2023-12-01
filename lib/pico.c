// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "pico.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"

extern char const __flash_binary_start[];
extern char const __flash_binary_end[];

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(board, string, PICO_BOARD),
    MLUA_SYM_V(build_type, string, PICO_CMAKE_BUILD_TYPE),
    MLUA_SYM_V(build_target, string, PICO_TARGET_NAME),
    MLUA_SYM_V(flash_binary_start, integer, (lua_Integer)__flash_binary_start),
    MLUA_SYM_V(flash_binary_end, integer, (lua_Integer)__flash_binary_end),
    MLUA_SYM_V(OK, integer, PICO_OK),
    MLUA_SYM_V(ERROR_NONE, integer, PICO_ERROR_NONE),
    MLUA_SYM_V(ERROR_TIMEOUT, integer, PICO_ERROR_TIMEOUT),
    MLUA_SYM_V(ERROR_GENERIC, integer, PICO_ERROR_GENERIC),
    MLUA_SYM_V(ERROR_NO_DATA, integer, PICO_ERROR_NO_DATA),
    MLUA_SYM_V(ERROR_NOT_PERMITTED, integer, PICO_ERROR_NOT_PERMITTED),
    MLUA_SYM_V(ERROR_INVALID_ARG, integer, PICO_ERROR_INVALID_ARG),
    MLUA_SYM_V(ERROR_IO, integer, PICO_ERROR_IO),
    MLUA_SYM_V(ERROR_BADAUTH, integer, PICO_ERROR_BADAUTH),
    MLUA_SYM_V(ERROR_CONNECT_FAILED, integer, PICO_ERROR_CONNECT_FAILED),
    MLUA_SYM_V(ERROR_INSUFFICIENT_RESOURCES, integer, PICO_ERROR_INSUFFICIENT_RESOURCES),
    MLUA_SYM_V(SDK_VERSION_MAJOR, integer, PICO_SDK_VERSION_MAJOR),
    MLUA_SYM_V(SDK_VERSION_MINOR, integer, PICO_SDK_VERSION_MINOR),
    MLUA_SYM_V(SDK_VERSION_REVISION, integer, PICO_SDK_VERSION_REVISION),
    MLUA_SYM_V(SDK_VERSION_STRING, string, PICO_SDK_VERSION_STRING),
#ifdef PICO_DEFAULT_UART
    MLUA_SYM_V(DEFAULT_UART, integer, PICO_DEFAULT_UART),
#else
    MLUA_SYM_V(DEFAULT_UART, boolean, false),
#endif
#ifdef PICO_DEFAULT_UART_TX_PIN
    MLUA_SYM_V(DEFAULT_UART_TX_PIN, integer, PICO_DEFAULT_UART_TX_PIN),
#else
    MLUA_SYM_V(DEFAULT_UART_TX_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_UART_RX_PIN
    MLUA_SYM_V(DEFAULT_UART_RX_PIN, integer, PICO_DEFAULT_UART_RX_PIN),
#else
    MLUA_SYM_V(DEFAULT_UART_RX_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_LED_PIN
    MLUA_SYM_V(DEFAULT_LED_PIN, integer, PICO_DEFAULT_LED_PIN),
#else
    MLUA_SYM_V(DEFAULT_LED_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_I2C
    MLUA_SYM_V(DEFAULT_I2C, integer, PICO_DEFAULT_I2C),
#else
    MLUA_SYM_V(DEFAULT_I2C, boolean, false),
#endif
#ifdef PICO_DEFAULT_I2C_SDA_PIN
    MLUA_SYM_V(DEFAULT_I2C_SDA_PIN, integer, PICO_DEFAULT_I2C_SDA_PIN),
#else
    MLUA_SYM_V(DEFAULT_I2C_SDA_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_I2C_SCL_PIN
    MLUA_SYM_V(DEFAULT_I2C_SCL_PIN, integer, PICO_DEFAULT_I2C_SCL_PIN),
#else
    MLUA_SYM_V(DEFAULT_I2C_SCL_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_SCK_PIN
    MLUA_SYM_V(DEFAULT_SCK_PIN, integer, PICO_DEFAULT_SCK_PIN),
#else
    MLUA_SYM_V(DEFAULT_SCK_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_TX_PIN
    MLUA_SYM_V(DEFAULT_TX_PIN, integer, PICO_DEFAULT_TX_PIN),
#else
    MLUA_SYM_V(DEFAULT_TX_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_SPI_RX_PIN
    MLUA_SYM_V(DEFAULT_SPI_RX_PIN, integer, PICO_DEFAULT_SPI_RX_PIN),
#else
    MLUA_SYM_V(DEFAULT_SPI_RX_PIN, boolean, false),
#endif
#ifdef PICO_DEFAULT_SPI_CSN_PIN
    MLUA_SYM_V(DEFAULT_SPI_CSN_PIN, integer, PICO_DEFAULT_SPI_CSN_PIN),
#else
    MLUA_SYM_V(DEFAULT_SPI_CSN_PIN, boolean, false),
#endif
#ifdef PICO_FLASH_SPI_CLKDIV
    MLUA_SYM_V(FLASH_SPI_CLKDIV, integer, PICO_FLASH_SPI_CLKDIV),
#else
    MLUA_SYM_V(FLASH_SPI_CLKDIV, boolean, false),
#endif
#ifdef PICO_FLASH_SIZE_BYTES
    MLUA_SYM_V(FLASH_SIZE_BYTES, integer, PICO_FLASH_SIZE_BYTES),
#else
    MLUA_SYM_V(FLASH_SIZE_BYTES, boolean, false),
#endif
#ifdef PICO_SMPS_MODE_PIN
    MLUA_SYM_V(SMPS_MODE_PIN, integer, PICO_SMPS_MODE_PIN),
#else
    MLUA_SYM_V(SMPS_MODE_PIN, boolean, false),
#endif
#ifdef CYW43_WL_GPIO_COUNT
    MLUA_SYM_V(CYW43_WL_GPIO_COUNT, integer, CYW43_WL_GPIO_COUNT),
#else
    MLUA_SYM_V(CYW43_WL_GPIO_COUNT, boolean, false),
#endif
#ifdef CYW43_WL_GPIO_LED_PIN
    MLUA_SYM_V(CYW43_WL_GPIO_LED_PIN, integer, CYW43_WL_GPIO_LED_PIN),
#else
    MLUA_SYM_V(CYW43_WL_GPIO_LED_PIN, boolean, false),
#endif
};

MLUA_OPEN_MODULE(pico) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
