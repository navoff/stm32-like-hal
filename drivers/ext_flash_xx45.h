#pragma once

/**
 * @file ext_flash_xx45.h
 * @brief Driver for SPI DataFlash chips of the xx45 family (AT45).
 */

#include <stdbool.h>
#include <stdint.h>

#include "utils/errors.h"

#define EXT_FLASH_XX45_PAGE_SIZE 512

/// Supported FLASH chip types
typedef enum
{
  EXT_FLASH_XX45_TYPE__UNKNOWN = 0x00000000,
  EXT_FLASH_XX45_TYPE__AT45DB32 = 0x001F2701,
} ext_flash_xx45_type_t;

typedef struct
{
  /// Whether the driver was initialized successfully
  bool inited;

  /// Type of the flash chip in use
  ext_flash_xx45_type_t type;

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
} ext_flash_xx45_t;

error_t ext_flash_xx45_autodefine(void);
error_t ext_flash_xx45_test(void);
bool ext_flash_xx45_flash_is_busy(void);
bool ext_flash_xx45_is_binary_page_size(void);
uint8_t ext_flash_xx45_flash_get_status_register(void);
void ext_flash_xx45_erase_sector_req(uint32_t address);
void ext_flash_xx45_erase_subsector_req(uint32_t address);
void ext_flash_xx45_program_page_req(uint32_t address, void *buf);
void ext_flash_xx45_read_array(uint32_t address, void *buf, uint32_t size);

extern ext_flash_xx45_t ext_flash_xx45;
