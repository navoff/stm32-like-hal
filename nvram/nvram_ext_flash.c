/**
 * @file nvram_ext_flash.c
 * @brief NVRAM backend implementation for external NOR flash (XX25/XX45 families).
 */

#include "config.h"

#include <string.h>

#include "drivers/ext_flash_xx25.h"
#include "drivers/ext_flash_xx45.h"
#include "nvram/nvram.h"
#include "nvram/nvram_ext_flash.h"
#include "nvram/nvram_map.h"

#if (!FEATURE_ENABLE_EXT_NOR_XX25_NVRAM) && (!FEATURE_ENABLE_EXT_NOR_XX45_NVRAM)
  #error You need to select NOR type
#endif

#if (FEATURE_ENABLE_EXT_NOR_XX25_NVRAM)

  #define ext_flash_xxx5 ext_flash_xx25
  #define ext_flash_xxx5_autodefine ext_flash_xx25_autodefine
  #define ext_flash_xxx5_erase_sector_req ext_flash_xx25_erase_sector_req
  #define ext_flash_xxx5_erase_subsector_req ext_flash_xx25_erase_subsector_req
  #define ext_flash_xxx5_program_page_req ext_flash_xx25_program_page_req
  #define ext_flash_xxx5_flash_is_busy ext_flash_xx25_flash_is_busy
  #define ext_flash_xxx5_read_array ext_flash_xx25_read_array
  #define EXT_FLASH_XXX5_PAGE_SIZE EXT_FLASH_XX25_PAGE_SIZE

#elif (FEATURE_ENABLE_EXT_NOR_XX45_NVRAM)

  #define ext_flash_xxx5 ext_flash_xx45
  #define ext_flash_xxx5_autodefine ext_flash_xx45_autodefine
  #define ext_flash_xxx5_erase_sector_req ext_flash_xx45_erase_sector_req
  #define ext_flash_xxx5_erase_subsector_req ext_flash_xx45_erase_subsector_req
  #define ext_flash_xxx5_program_page_req ext_flash_xx45_program_page_req
  #define ext_flash_xxx5_flash_is_busy ext_flash_xx45_flash_is_busy
  #define ext_flash_xxx5_read_array ext_flash_xx45_read_array
  #define EXT_FLASH_XXX5_PAGE_SIZE EXT_FLASH_XX45_PAGE_SIZE

#else
  #error This driver can only work with one type of NOR
#endif

/// Number of flash segments used for NVRAM
/// Several segments are used to extend flash endurance
#define NVRAM_EXT_FLASH_SEGMENT_AMOUNT 16

#define NVRAM_EXT_FLASH_DEFAULT_ADDRESS_BEGIN 0

#define ADDRESS_FROM_SEGMENT_IDX(idx) (ext_flash_xxx5.erasable_segment_size * idx)

#define SEGMENT_IDX_FROM_ADDRESS(addr) ((addr) / ext_flash_xxx5.erasable_segment_size)

typedef struct
{
  nvram_map_t nvram;
  uint32_t address;
  uint16_t size;
  uint8_t in_progress;
} nvram_ext_flash_write_segment_t;

nvram_ext_flash_write_segment_t nvram_ext_flash_write;

/**
 * @brief Auto-detect the type and size of the external flash
 * @note The global variables used for further work with the memory
 * are initialized right away.
 * @param address_begin - pointer where the default NVRAM area start address is placed
 * @param [out] segment_amount - number of segments used.
 * @result Result of the function (see error_t)
 */
error_t nvram_ext_flash_check_defines(uint32_t *address_begin, uint16_t *segment_amount)
{
  error_t res;

  res = ext_flash_xxx5_autodefine();

  if (res == ERR_OK)
  {
    if (ext_flash_xxx5.erasable_segment_size < NVRAM_SIZE)
    {
      res = ERR_RESULT;
    }
  }

  nvram_ext_flash_write.in_progress = 0;
  (*segment_amount) = NVRAM_EXT_FLASH_SEGMENT_AMOUNT;
  *address_begin = NVRAM_EXT_FLASH_DEFAULT_ADDRESS_BEGIN;

  return res;
}

/**
 * @brief Check whether any flash operation is currently in progress
 * @return 0 - flash is idle, otherwise - busy
 */
uint8_t nvram_ext_flash_is_busy(void)
{
  return ext_flash_xxx5_flash_is_busy();
}

/**
 * @brief Check whether a flash write operation is currently in progress
 * @return 1 - a flash write is in progress, 0 - idle
 */
uint8_t nvram_ext_flash_write_in_progress(void)
{
  return nvram_ext_flash_write.in_progress;
}

/**
 * Find the segment in the external flash that holds the most recent settings
 * and load it.
 * The most recent data is held by the segment with the largest identifier and
 * a valid checksum.
 * @param address_begin - NVRAM area start address
 * @param segment_amount - number of segments allocated for NVRAM
 * @param version - version of the data structure stored in NVRAM
 * @param segment_idx - index of the found segment.
 * @param data - buffer the data is loaded into.
 * @return see error_t
 */
error_t nvram_ext_flash_find_segment_and_load(uint32_t address_begin, uint16_t segment_amount,
                                              uint16_t version, uint16_t *segment_idx,
                                              nvram_map_t *data)
{
  error_t res = ERR_RESULT;
  uint8_t idx;
  uint32_t current_id;

  uint32_t address = address_begin;

  for (uint8_t i = 0; i < segment_amount; i++)
  {
    ext_flash_xxx5_read_array(address, data, sizeof(nvram_map_t));

    if (data->sys.segment_correct == NVRAM_FLASH_SEGMENT_CORRECT)
    {
      uint16_t crc = nvram_get_crc(data);
      if (crc == data->sys.nvram_crc)
      {
        if (data->sys.nvram_settings_version == version)
        {
          // if this is not the first valid segment
          if (res == ERR_OK)
          {
            if ((data->sys.segment_id >= current_id) || (data->sys.segment_id == 0))
            {
              idx = i;
              current_id = data->sys.segment_id;
            }
          }
          else
          {
            idx = i;
            current_id = data->sys.segment_id;
            res = ERR_OK;
          }
        }
      }
    }

    address += ext_flash_xxx5.erasable_segment_size;
  }

  if (res == ERR_OK)
  {
    *segment_idx = idx;
    address = address_begin + idx * ext_flash_xxx5.erasable_segment_size;
    ext_flash_xxx5_read_array(address, data, NVRAM_SIZE);
  }

  return res;
}

/**
 * @brief Erase a segment and write data into it
 * @param address_begin - NVRAM area start address
 * @param segment_idx - index of the segment to write, relative to the area start
 * @param data - pointer to the data structure
 */
void nvram_ext_flash_erase_and_write_segment(uint32_t address_begin, uint16_t segment_idx,
                                             nvram_map_t *data)
{
  while (ext_flash_xxx5_flash_is_busy())
    ;
  nvram_ext_flash_erase_segment_req(address_begin, segment_idx);

  memcpy(&nvram_ext_flash_write.nvram, data, sizeof(nvram_map_t));

  uint16_t size = 0;
  uint32_t address = address_begin + segment_idx * ext_flash_xxx5.erasable_segment_size;
  while (size < sizeof(nvram_map_t))
  {
    while (ext_flash_xxx5_flash_is_busy())
      ;
    ext_flash_xxx5_program_page_req(address, &nvram_ext_flash_write.nvram.buf[size]);

    size += EXT_FLASH_XXX5_PAGE_SIZE;
    address += EXT_FLASH_XXX5_PAGE_SIZE;
  }
}

/**
 * @brief Erase a segment
 * @param address_begin - NVRAM area start address
 * @param segment_idx - index of the segment to erase (relative to the area start)
 */
void nvram_ext_flash_erase_segment_req(uint32_t address_begin, uint16_t segment_idx)
{
  uint32_t address = address_begin + ext_flash_xxx5.erasable_segment_size * segment_idx;

  if (ext_flash_xxx5.erase_subsector_enable)
  {
    ext_flash_xxx5_erase_subsector_req(address);
  }
  else
  {
    ext_flash_xxx5_erase_sector_req(address);
  }
}

/**
 * @brief Write data to the given segment
 * @param address_begin - NVRAM area start address
 * @param segment_idx - index of the segment to write, relative to the area start
 * @param data - pointer to the data to write
 */
void nvram_ext_flash_write_segment_req(uint32_t address_begin, uint16_t segment_idx,
                                       nvram_map_t *data)
{
  nvram_ext_flash_write.address =
      address_begin + ext_flash_xxx5.erasable_segment_size * segment_idx;

  nvram_ext_flash_write.in_progress = 1;
  nvram_ext_flash_write.size = 0;

  uint32_t save;
  ENTER_CRITICAL(save);
  memcpy(&nvram_ext_flash_write.nvram, data, sizeof(nvram_map_t));
  LEAVE_CRITICAL(save);

  ext_flash_xxx5_program_page_req(nvram_ext_flash_write.address,
                                  &nvram_ext_flash_write.nvram.buf[nvram_ext_flash_write.size]);
}

/**
 * @brief Process that writes data to flash incrementally, page by page
 */
void nvram_ext_flash_write_segment_process(void)
{
  if (ext_flash_xxx5_flash_is_busy() == 0)
  {
    nvram_ext_flash_write.size += EXT_FLASH_XXX5_PAGE_SIZE;
    nvram_ext_flash_write.address += EXT_FLASH_XXX5_PAGE_SIZE;

    if (nvram_ext_flash_write.size >= sizeof(nvram_map_t))
    {
      nvram_ext_flash_write.in_progress = 0;
    }
    else
    {
      ext_flash_xxx5_program_page_req(nvram_ext_flash_write.address,
                                      &nvram_ext_flash_write.nvram.buf[nvram_ext_flash_write.size]);
    }
  }
}
