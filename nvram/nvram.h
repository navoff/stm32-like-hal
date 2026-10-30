#pragma once

/**
 * @file nvram.h
 * @brief NVRAM manager: cached access to settings stored in internal or external flash.
 */

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

#include "nvram/nvram_ext_flash.h"
#include "nvram/nvram_int_flash.h"
#include "nvram/nvram_map.h"

typedef enum
{
  NVRAM_DRIVER__INT_FLASH,
  NVRAM_DRIVER__EXT_NOR_XXX5,
} nvram_driver_t;

typedef struct
{
  /// Whether the driver was initialized successfully
  bool inited;

  /// NVRAM state
  enum
  {
    NVRAM_STATE__WAIT,
    NVRAM_STATE__EXT_FLASH_WAIT_SEGMENT_ERASE,
    NVRAM_STATE__EXT_FLASH_WAIT_SEGMENT_WRITE,
  } state;

  nvram_driver_t driver;

  /// Cache of the structure stored in NVRAM
  nvram_map_t __attribute__((aligned(sizeof(uint32_t)))) cache;

  /// NVRAM structure version
  uint16_t version;

  /// Start address of the NVRAM area
  uint32_t address_begin;

  /// Index of the current segment (0 to segment_amount)
  uint16_t segment_idx;

  /// Number of memory segments used for NVRAM
  uint16_t segment_amount;

  /// Flag that the cache was updated and the structure must be rewritten to flash
  uint8_t need_update;

  /// Write-protect flag.
  /// When set, writes to the NVRAM cache (and hence to NVRAM itself) are ignored.
  bool is_locked;
} nvram_t;

/// NVRAM configuration structure
typedef struct
{
  /// NVRAM structure version
  uint16_t version;

  /// Start address of the NVRAM area
  uint32_t address_begin;

  /// Number of segments allocated for NVRAM
  uint16_t segment_amount;

  /// Flash driver type
  nvram_driver_t driver;
} nvram_init_struct_t;

/**
 * Read the given field from the NVRAM cache.
 * @param field - field name in the nvram_settings_t structure
 * @param p_dst - pointer to the variable (array) the value is read into.
 * Usage example:
 *   uint8_t read_value;
 *   NVRAM_SETTINGS_READ_FIELD(test, &read_value);
 */
#define NVRAM_SETTINGS_READ_FIELD(field, p_dst)                                                    \
  nvram_read_block(&nvram_settings, (void *)p_dst, (uint32_t)&(((nvram_settings_t *)0)->field),    \
                   sizeof(((nvram_settings_t *)0)->field))

/**
 * Write the given field into the NVRAM cache.
 * On the next run of the NVRAM manager the cache is written to flash
 * @param field - field name in the nvram_settings_t structure
 * @param p_src - pointer to the variable (array) the value is copied from.
 * Usage example:
 *   uint8_t write_value = 42;
 *   NVRAM_SETTINGS_WRITE_FIELD(test, &write_value);
 */
#define NVRAM_SETTINGS_WRITE_FIELD(field, p_src)                                                   \
  nvram_write_block(&nvram_settings, (void *)p_src, (uint32_t)&(((nvram_settings_t *)0)->field),   \
                    sizeof(((nvram_settings_t *)0)->field))

extern nvram_t nvram_settings;

error_t nvram_struct_init(nvram_init_struct_t *init_struct, nvram_driver_t driver);
error_t nvram_init(nvram_t *nvram, nvram_init_struct_t *init_struct);
void nvram_write_default_struct(nvram_t *nvram, uint8_t *data, uint16_t size);
void nvram_process(nvram_t *nvram);

uint8_t nvram_is_busy(nvram_t *nvram);
void nvram_lock(nvram_t *nvram, bool need_to_lock);
uint32_t nvram_get_segment_id(nvram_t *nvram);

uint16_t nvram_get_crc(nvram_map_t *nvram_map);
void nvram_read_block(nvram_t *nvram, void *data, uint32_t address, uint16_t size);
void nvram_write_block(nvram_t *nvram, void *data, uint32_t address, uint16_t size);
