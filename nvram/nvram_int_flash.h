#pragma once

/**
 * @file nvram_int_flash.h
 * @brief NVRAM backend for the MCU internal flash.
 */

#include <stdint.h>

#include "nvram/nvram_map.h"
#include "utils/errors.h"

#define NVRAM_INT_FLASH_SEGMENT_IDX_UNDEF 0xFF

error_t nvram_int_flash_get_address(uint32_t *address_begin, uint16_t *segment_amount);

error_t nvram_int_flash_find_segment_and_load(uint16_t version, uint16_t *segment_idx,
                                              nvram_map_t *data);

void nvram_int_flash_erase_and_write_segment(uint16_t segment_idx, nvram_map_t *data);
