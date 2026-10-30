#pragma once

/**
 * @file int_flash.h
 * @brief Driver for the MCU internal flash memory: read, write and sector erase.
 */

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

#include "utils/errors.h"

/// End address of the usable FLASH region is unknown
#define INT_FLASH_UNKNOWN_END_ADDRESS 0xFFFFFFFF

#if defined(STM32F4XX)
  #if defined(STM32F411xE)
    #define FLASH_SECTOR_AMOUNT 8
  #elif defined(STM32F40_41xxx)
    #define FLASH_SECTOR_AMOUNT 12
  #elif defined(STM32F427_437xx)
    #define FLASH_SECTOR_AMOUNT 24
  #elif defined(STM32F446xx)
    #define FLASH_SECTOR_AMOUNT 8
  #endif

  /// Address of the register that holds the flash size in KB, 16-bit
  #define DEVICE_FLASH_SIZE_ADDRESS (0x1FFF7A22)

#elif defined(STM32L4XX)
  /// Flash sector size
  #define FLASH_SECTOR_SIZE 2048

  /// Address of the register that holds the flash size in KB, 16-bit
  #define DEVICE_FLASH_SIZE_ADDRESS (0x1FFF75E0)

#elif defined(STM32F37X) || defined(STM32F30X)
  /// Flash sector size
  #define FLASH_SECTOR_SIZE 2048

  /// Address of the register that holds the flash size in KB, 16-bit
  #define DEVICE_FLASH_SIZE_ADDRESS (0x1FFFF7CC)

#elif defined(STM32F1XX)
  /// Flash sector size
  #define FLASH_SECTOR_SIZE 1024

  /// Address of the register that holds the flash size in KB, 16-bit
  #define DEVICE_FLASH_SIZE_ADDRESS (0x1FFFF7E0)

#elif defined(GD32E11X)
  /// Flash sector size
  /// TODO read the flash size from the register instead of a define,
  /// different MCU variants in the series have different flash with different sector sizes
  #define FLASH_SECTOR_SIZE 1024

  /// Address of the register that holds the flash size in KB, 16-bit
  #define DEVICE_FLASH_SIZE_ADDRESS (0x1FFFF7E0)
#elif defined(GD32L23X)
  /// Flash sector size
  /// TODO read the flash size from the register instead of a define,
  /// different MCU variants in the series have different flash with different sector sizes
  #define FLASH_SECTOR_SIZE 2048

  /// Address of the register that holds the flash size in KB, 16-bit
  #define DEVICE_FLASH_SIZE_ADDRESS (0x1FFFF7E0)
#else
  #error This MCU not supported
#endif

/// Sector information
typedef struct
{
  /// Sector start address
  uint32_t address;

  /// Sector size
  uint32_t size;

  /// Sector identifier
  uint16_t id;
} int_flash_sector_t;

/// Internal flash driver initialization structure
typedef struct
{
  /// Start address of the usable FLASH region
  uint32_t begin_addr;

  /// End address of the usable FLASH region
  uint32_t end_addr;
} int_flash_init_struct_t;

/// Internal FLASH access control structure
typedef struct
{
  /// Start address of the writable FLASH region
  uint32_t begin_addr;

  /// End address of the writable FLASH region
  uint32_t end_addr;
} int_flash_t;

#if defined(STM32F4XX)
extern const int_flash_sector_t int_flash_sectors[FLASH_SECTOR_AMOUNT];
#endif

void int_flash_init(int_flash_t *int_flash, int_flash_init_struct_t *init_struct);
error_t int_flash_read(int_flash_t *int_flash, uint32_t src_addr, uint8_t *buf, uint16_t size);
error_t int_flash_write(int_flash_t *int_flash, uint32_t address_dst, uint8_t *buf, uint16_t size);
error_t int_flash_get_sector_idx(uint32_t address, uint16_t *sector_idx, bool *is_sector_begin);
error_t int_flash_erase_sector(uint16_t sector_idx);
