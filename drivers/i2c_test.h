#pragma once

/**
 * @file i2c_test.h
 * @brief Loopback self-test for the I2C driver in master and slave modes.
 */

#include <stdint.h>

#include "drivers/i2c.h"

void i2c_test_master_start(i2c_core_t *i2c_core, uint16_t slave_address);
void i2c_test_slave_start(i2c_core_t *i2c_core);
