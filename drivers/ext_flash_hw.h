#pragma once

/**
 * @file ext_flash_hw.h
 * @brief SPI transport layer for the external flash drivers.
 */

#include <stdint.h>

#include "drivers/spi.h"

void ext_flash_hw_init(spi_core_t *spi_core, uint8_t cs_gpio_idx, uint8_t cs_pin_idx);
void ext_flash_hw_enable(void);
void ext_flash_hw_disable(void);
uint8_t ext_flash_hw_tx_rx_byte(uint8_t data);
void ext_flash_hw_rx_array(uint8_t *buf, uint16_t size);
void ext_flash_hw_tx_array(uint8_t *buf, uint16_t size);
