#pragma once

/**
 * @file nvram_ext_flash.h
 * @brief NVRAM backend for external NOR flash (XX25/XX45 families).
 */

#include <stdint.h>

#include "config.h"
#include "nvram/nvram_map.h"
#include "utils/errors.h"

#if (FEATURE_ENABLE_EXT_NOR_XX25_NVRAM || FEATURE_ENABLE_EXT_NOR_XX45_NVRAM)
  #define FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM 1
#else
  #define FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM 0
#endif

error_t nvram_ext_flash_check_defines(uint32_t *address_begin, uint16_t *segment_amount);

uint8_t nvram_ext_flash_is_busy(void);
uint8_t nvram_ext_flash_write_in_progress(void);

error_t nvram_ext_flash_find_segment_and_load(uint32_t address_begin, uint16_t segment_amount,
                                              uint16_t version, uint16_t *segment_idx,
                                              nvram_map_t *data);

void nvram_ext_flash_erase_and_write_segment(uint32_t address_begin, uint16_t segment_idx,
                                             nvram_map_t *data);

void nvram_ext_flash_erase_segment_req(uint32_t address_begin, uint16_t segment_idx);

void nvram_ext_flash_write_segment_req(uint32_t address_begin, uint16_t segment_idx,
                                       nvram_map_t *data);

void nvram_ext_flash_write_segment_process(void);
