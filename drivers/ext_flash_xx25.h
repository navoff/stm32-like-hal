#pragma once

/**
 * @file ext_flash_xx25.h
 * @brief Driver for SPI NOR flash chips of the xx25 family (M25, N25, MX25, W25).
 */

#include <stdbool.h>
#include <stdint.h>

#include "utils/errors.h"

#define EXT_FLASH_XX25_PAGE_SIZE 256

/// Calculate the number of segments from the area size
#define EXT_FLASH_XX25_SEGMENT_AMOUNT_FROM_SIZE(size)                                              \
  ((size) / ext_flash_xx25.erasable_segment_size)

/// Supported FLASH chip types
typedef enum
{
  EXT_FLASH_XX25_TYPE__UNKNOWN = 0x00000000,
  EXT_FLASH_XX25_TYPE__M25P16 = 0x00202015,
  EXT_FLASH_XX25_TYPE__N25Q128 = 0x0020BA18,
  EXT_FLASH_XX25_TYPE__N25Q032 = 0x0020BA16,
  EXT_FLASH_XX25_TYPE__MX25L128 = 0x00C22018,
  EXT_FLASH_XX25_TYPE__W25Q32F = 0x00EF4016,
} ext_flash_xx25_type_t;

typedef struct
{
  /// Whether the driver was initialized successfully
  bool inited;

  /// Type of the flash chip in use
  ext_flash_xx25_type_t type;

  /// Flash size, KB
  uint16_t size_in_kb;

  /// Whether this chip supports SUBSECTOR erase.
  /// Two naming conventions are used:
  /// - BLOCK { SECTOR_1 ... SECTOR_N }
  /// - SECTOR { SUBSECTOR_1 ... SUBSECTOR_N }
  bool erase_subsector_enable;

  /// Erasable segment size, bytes
  uint32_t erasable_segment_size;

  /// Segment erase time, ms
  uint16_t erase_segment_timeout;

  /// Single page program time, ms
  uint16_t program_page_timeout;
} ext_flash_xx25_t;

error_t ext_flash_xx25_autodefine(void);
error_t ext_flash_xx25_test(void);
bool ext_flash_xx25_flash_is_busy(void);
uint8_t ext_flash_xx25_flash_get_status_register(void);
void ext_flash_xx25_read_array(uint32_t address, void *buf, uint32_t size);
void ext_flash_xx25_erase_sector_req(uint32_t address);
void ext_flash_xx25_erase_subsector_req(uint32_t address);
void ext_flash_xx25_program_page_req(uint32_t address, void *buf);

extern ext_flash_xx25_t ext_flash_xx25;
