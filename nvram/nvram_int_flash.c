/**
 * @file nvram_int_flash.c
 * @brief NVRAM backend implementation for the MCU internal flash.
 */

#include "config.h"

#include <string.h>

#include "drivers/int_flash.h"
#include "nvram/nvram.h"
#include "utils/assert.h"
#include "utils/errors.h"

#include "nvram/nvram_int_flash.h"

typedef struct
{
  uint32_t address;
  uint32_t size;
  uint16_t id;
} flash_sector_t;

/// NVRAM segment start address from the linker script
extern const uint32_t __nvram_begin;

/// NVRAM segment end address from the linker script
extern const uint32_t __nvram_end;

int_flash_t nvram_int_flash_driver;

/**
 * @brief Get the NVRAM start address and number of allocated segments from the linker file
 * @param address_begin - pointer where the area start address is placed
 * @param segment_amount - pointer where the number of allocated segments is placed
 * @return ERR_OK - success, otherwise - error
 */
error_t nvram_int_flash_get_address(uint32_t *address_begin, uint16_t *segment_amount)
{
  int_flash_init_struct_t int_flash_init_struct;

  int_flash_init_struct.begin_addr = (uint32_t)&__nvram_begin;
  int_flash_init_struct.end_addr = (uint32_t)&__nvram_end;

  hal_assert(int_flash_init_struct.begin_addr < int_flash_init_struct.end_addr);

  int_flash_init(&nvram_int_flash_driver, &int_flash_init_struct);
  *address_begin = nvram_int_flash_driver.begin_addr;

  uint16_t sector_idx;
  bool is_sector_begin;
  int_flash_get_sector_idx(nvram_int_flash_driver.begin_addr, &sector_idx, &is_sector_begin);

  hal_assert(sector_idx != NVRAM_INT_FLASH_SEGMENT_IDX_UNDEF);
  // the NVRAM start address from the linker file must coincide with a flash sector start
  hal_assert(is_sector_begin);

  *segment_amount = 0;

#if defined(STM32F4XX)

  uint32_t address = nvram_int_flash_driver.begin_addr;
  while (address < nvram_int_flash_driver.end_addr)
  {
    hal_assert(int_flash_sectors[sector_idx].size >= NVRAM_SIZE);
    hal_assert((int_flash_sectors[sector_idx].size % NVRAM_SIZE) == 0);

    (*segment_amount) += int_flash_sectors[sector_idx].size / NVRAM_SIZE;

    address += NVRAM_SIZE;
    if (address >= (int_flash_sectors[sector_idx].address + int_flash_sectors[sector_idx].size))
    {
      sector_idx++;
    }
  }

#elif defined(STM32F1XX) || defined(STM32F30X) || defined(STM32L4XX) || defined(GD32E11X) ||       \
    defined(GD32L23X)

  // the erasable flash sector size must be at least the NVRAM size
  hal_assert(FLASH_SECTOR_SIZE >= NVRAM_SIZE);
  hal_assert((FLASH_SECTOR_SIZE % NVRAM_SIZE) == 0);

  (*segment_amount) =
      (nvram_int_flash_driver.end_addr + 1 - nvram_int_flash_driver.begin_addr) / NVRAM_SIZE;

#else
  #error This MCU not supported
#endif

  return ERR_OK;
}

/**
 * @brief Erase one flash segment and write the given data into it.
 * @param segment_idx - segment index within the flash area allocated for NVRAM.
 * @param data - buffer with the data to write.
 */
void nvram_int_flash_erase_and_write_segment(uint16_t segment_idx, nvram_map_t *data)
{
  uint32_t address_dst = nvram_int_flash_driver.begin_addr + NVRAM_SIZE * segment_idx;

  uint16_t sector_idx;
  bool is_sector_begin;
  int_flash_get_sector_idx(address_dst, &sector_idx, &is_sector_begin);

  if (is_sector_begin)
  {
    int_flash_erase_sector(sector_idx);
  }

  int_flash_write(&nvram_int_flash_driver, address_dst, (uint8_t *)data, sizeof(nvram_map_t));
}

/**
 * @brief Find and load the last correctly written segment
 * @param version - data structure version
 * @param segment_idx - pointer where the index of the segment holding the last correctly
 *        written data structure is placed (relative to the NVRAM area start)
 * @param data - pointer where the read data is stored
 * @return ERR_OK - success, otherwise - error
 */
error_t nvram_int_flash_find_segment_and_load(uint16_t version, uint16_t *segment_idx,
                                              nvram_map_t *data)
{
  error_t res = ERR_RESULT;
  uint16_t idx = 0;

  nvram_map_t *nvram_flash = (nvram_map_t *)nvram_int_flash_driver.begin_addr;
  uint32_t find_segment_id = 0;

  while ((uint32_t)nvram_flash < nvram_int_flash_driver.end_addr)
  {
    if (nvram_flash->sys.segment_correct == NVRAM_FLASH_SEGMENT_CORRECT)
    {
      uint16_t crc = nvram_get_crc(nvram_flash);
      if (crc == nvram_flash->sys.nvram_crc)
      {
        if (nvram_flash->sys.nvram_settings_version == version)
        {
          // if this is not the first valid segment
          if (res == ERR_OK)
          {
            if ((nvram_flash->sys.segment_id >= find_segment_id) ||
                (nvram_flash->sys.segment_id == 0))
            {
              *segment_idx = idx;
              find_segment_id = nvram_flash->sys.segment_id;
            }
          }
          else
          {
            *segment_idx = idx;
            find_segment_id = nvram_flash->sys.segment_id;
            res = ERR_OK;
          }
        }
      }
    }
    idx++;
    nvram_flash++;
  }

  if (res == ERR_OK)
  {
    uint32_t address = nvram_int_flash_driver.begin_addr + NVRAM_SIZE * (*segment_idx);
    memcpy(data, (const void *)address, sizeof(nvram_map_t));
  }

  return res;
}
