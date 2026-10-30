#pragma once

/**
 * @file unit_tests.h
 * @brief Compile-time switches and API for the UART driver unit tests.
 */

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

#include "drivers/tick_timer.h"
#include "utils/assert.h"

// Set of tests for checking UART operation

/// To trigger an ORE error set to 1, then send the device at least
///   UNIT_TEST_UART_ORE_REPETITION + 1
/// data arrays of size
///   UNIT_TEST_UART_ORE_BYTE_NUMBER + 1
#define UNIT_TEST_ERROR_ORE 0

/// To check that UART data transmission is correct set to 1.
/// Verify that the PC receives an incrementing uint8_t value without gaps
#define UNIT_TEST_UART_TX 0

#if (UNIT_TEST_UART_TX)
typedef error_t (*unit_test_uart_tx_byte)(uint8_t data);
#endif

#if (UNIT_TEST_ERROR_ORE)
void unit_test_uart_ore_cause(void);
void unit_test_uart_ore_process(uint16_t sr);
#endif

#if (UNIT_TEST_UART_TX)
void unit_test_uart_tx_init(unit_test_uart_tx_byte func);
void unit_test_uart_tx_procress(void);
#endif
