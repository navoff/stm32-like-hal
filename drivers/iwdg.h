#pragma once

/**
 * @file iwdg.h
 * @brief Independent watchdog (IWDG) driver.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Disable IWDG when MCU core is halted
 */
void iwdg_stop_in_halted_mode(void);

/**
 * @brief Initialize IWDG
 * @param timeout - IWDG operation timeout, ms
 */
void iwdg_init(uint32_t timeout);

/**
 * @brief Update IWDG counter
 */
void iwdg_update(void);

/**
 * @brief Get status whether IWDG is enabled
 * @return true - if IWDG is running, false - not running
 */
bool iwdg_is_enabled(void);
