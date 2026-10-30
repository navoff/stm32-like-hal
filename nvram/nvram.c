/**
 * @file nvram.c
 * @brief Non-volatile memory (NVRAM) manager used to store device settings.
 *        NVRAM is backed by one of:
 *        - the flash built into the MCU: define FEATURE_ENABLE_INT_NVRAM
 *        - an external flash chip: define FEATURE_ENABLE_EXT_NOR_XX25_NVRAM or
 *          FEATURE_ENABLE_EXT_NOR_XX45_NVRAM
 *        The whole NVRAM area fits into one minimal erasable segment:
 *        - for internal flash - one page
 *        - for external flash of the M25P family - one sector
 *        - for external flash of the N25Q family - one subsector
 *        To extend flash endurance, several segments are allocated for the NVRAM area:
 *        - for internal flash - derived from the NVRAM region size in the linker script
 *        - for external flash - define NVRAM_EXT_FLASH_SEGMENT_AMOUNT
 *        and every write of new data to NVRAM uses the "next" segment.
 *        To tell one segment from another, a segment identifier is placed at the end of the
 *        NVRAM area, so on NVRAM initialization the segment with the largest identifier is
 *        looked up.
 *        To check NVRAM integrity, a checksum is placed at the end of the NVRAM area.
 *        The checksum is computed with the CRC16 algorithm.
 */

#include "config.h"

#include <string.h>

#include "drivers/ext_flash_hw.h"
#include "nvram/nvram.h"
#include "utils/assert.h"
#include "utils/checksum.h"
#include "utils/errors.h"

#if (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM)
  #if ((NVRAM_SIZE % 256) != 0)
    #error NVRAM size must be aligned by 256 bytes
  #endif
#elif (FEATURE_ENABLE_INT_NVRAM)
  #if ((NVRAM_SIZE % 8) != 0)
    // Strictly speaking, the NVRAM size must be a multiple of the minimal word that can be
    // written to the internal flash. The word size depends on the MCU type and ranges
    // from 1 to 8 bytes. To avoid piling up checks here, assume the "worst" case - 8 bytes.
    #error NVRAM size must be aligned by 8 bytes
  #endif
#endif

#if (NVRAM_CHECKSUM_CRC_ALG == CHECKSUM_CRC_ALG_SLOW)
  #define nvram_checksum_calculate checksum_crc16_x25_slow_calculate
#elif (NVRAM_CHECKSUM_CRC_ALG == CHECKSUM_CRC_ALG_FAST)
  #define nvram_checksum_calculate checksum_crc16_x25_fast_calculate
#elif (NVRAM_CHECKSUM_CRC_ALG == CHECKSUM_CRC_ALG_HW)
  #define nvram_checksum_calculate checksum_crc16_x25_hw_calculate
#else
  #error "You need to select CRC algorithm"
#endif

error_t nvram_find_segment_and_load(nvram_t *nvram);
void nvram_erase_and_write_segment(nvram_t *nvram);

/**
 * @brief Initialize the NVRAM configuration structure with default parameters
 * @param init_struct - pointer to the configuration structure
 * @param driver - flash driver type
 *
 * If internal flash is selected, the address and the number of segments are set
 * automatically: they are taken from the linker file.
 */
error_t nvram_struct_init(nvram_init_struct_t *init_struct, nvram_driver_t driver)
{
  init_struct->version = 0;
  init_struct->driver = driver;

  error_t res = ERR_RESULT;

#if (FEATURE_ENABLE_INT_NVRAM)
  if (driver == NVRAM_DRIVER__INT_FLASH)
  {
    // for internal flash the addresses are taken from the linker file
    res = nvram_int_flash_get_address(&init_struct->address_begin, &init_struct->segment_amount);
  }
#endif

#if (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM)
  if (driver == NVRAM_DRIVER__EXT_NOR_XXX5)
  {
    // for external flash the addresses are filled in by the user
    res = ERR_OK;
  }
#endif

  return res;
}

/**
 * @brief Initialize an NVRAM instance for operation
 * @param nvram - pointer to the NVRAM instance
 * @param init_struct - pointer to the configuration structure
 * @note On initialization failure it is recommended to write the default structure with
 *          nvram_write_default_struct()
 * @return ERR_OK - success, otherwise - error
 */
error_t nvram_init(nvram_t *nvram, nvram_init_struct_t *init_struct)
{
  // the NVRAM structure size must match the allocated NVRAM size
  hal_assert(sizeof(nvram_map_t) == NVRAM_SIZE);

#if (FEATURE_ENABLE_INT_NVRAM == 0)
  hal_assert(init_struct->driver != NVRAM_DRIVER__INT_FLASH);
#endif

#if (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM == 0)
  hal_assert(init_struct->driver != NVRAM_DRIVER__EXT_NOR_XXX5);
#endif

  nvram->inited = false;
  nvram->driver = init_struct->driver;
  nvram->version = init_struct->version;
  nvram->address_begin = init_struct->address_begin;
  nvram->segment_amount = init_struct->segment_amount;

  error_t res = nvram_find_segment_and_load(nvram);

  if (res != ERR_OK)
  {
    memset(nvram->cache.app.buf, 0xFF, NVRAM_USER_SIZE);

    nvram->cache.sys.nvram_settings_version = nvram->version;
    nvram->cache.sys.segment_id = 0;
    nvram->cache.sys.segment_correct = NVRAM_FLASH_SEGMENT_CORRECT;
    nvram->segment_idx = 0;
  }

  nvram->need_update = 0;
  nvram->is_locked = false;
  nvram->state = NVRAM_STATE__WAIT;
  nvram->inited = true;

  return res;
}

/**
 * @brief Write the default structure to NVRAM
 * @param nvram - pointer to the NVRAM instance
 * @param data - pointer to the data array
 * @param size - data size
 */
void nvram_write_default_struct(nvram_t *nvram, uint8_t *data, uint16_t size)
{
  if (!nvram->inited)
  {
    return;
  }

  memset(nvram->cache.app.buf, 0xFF, NVRAM_USER_SIZE);
  memcpy(nvram->cache.app.buf, data, size);

  nvram->cache.sys.nvram_crc = nvram_get_crc(&nvram->cache);
  nvram_erase_and_write_segment(nvram);
}

/**
 * @brief Main NVRAM processing routine
 * @param nvram - pointer to the NVRAM instance
 */
void nvram_process(nvram_t *nvram)
{
  if (!nvram->inited)
  {
    return;
  }

  switch (nvram->state)
  {
    case NVRAM_STATE__WAIT:
      if (nvram->need_update)
      {
        nvram->need_update = 0;

        nvram->cache.sys.segment_id++;

        nvram->segment_idx++;
        if (nvram->segment_idx >= nvram->segment_amount)
        {
          nvram->segment_idx = 0;
        }

#if (FEATURE_ENABLE_INT_NVRAM)
        if (nvram->driver == NVRAM_DRIVER__INT_FLASH)
        {
          nvram->cache.sys.segment_correct = NVRAM_FLASH_SEGMENT_CORRECT;
          nvram->cache.sys.nvram_crc = nvram_get_crc(&nvram->cache);
          nvram_erase_and_write_segment(nvram);
        }
#endif

#if (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM)
        if (nvram->driver == NVRAM_DRIVER__EXT_NOR_XXX5)
        {
          nvram_ext_flash_erase_segment_req(nvram->address_begin, nvram->segment_idx);
          nvram->state = NVRAM_STATE__EXT_FLASH_WAIT_SEGMENT_ERASE;
        }
#endif
      }
      break;

#if (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM)
    case NVRAM_STATE__EXT_FLASH_WAIT_SEGMENT_ERASE:
      if (nvram_ext_flash_is_busy() == 0)
      {
        nvram->cache.sys.segment_correct = NVRAM_FLASH_SEGMENT_CORRECT;
        nvram->cache.sys.nvram_crc = nvram_get_crc(&nvram->cache);
        nvram_ext_flash_write_segment_req(nvram->address_begin, nvram->segment_idx, &nvram->cache);
        nvram->state = NVRAM_STATE__EXT_FLASH_WAIT_SEGMENT_WRITE;
      }
      break;

    case NVRAM_STATE__EXT_FLASH_WAIT_SEGMENT_WRITE:
      nvram_ext_flash_write_segment_process();
      if (nvram_ext_flash_write_in_progress() == 0)
      {
        nvram->state = NVRAM_STATE__WAIT;
      }
      break;
#endif

    default:
      nvram->state = NVRAM_STATE__WAIT;
  }
}

/**
 * @brief Check whether the flash is currently busy with this NVRAM instance
 * @param nvram - pointer to the NVRAM instance
 * @return 1 - busy, 0 - idle
 */

uint8_t nvram_is_busy(nvram_t *nvram)
{
  return ((nvram->state != NVRAM_STATE__WAIT) || (nvram->need_update));
}

/**
 * @brief Lock/unlock write access to NVRAM
 * @param need_to_lock - if true, the NVRAM is locked for writing
 */
void nvram_lock(nvram_t *nvram, bool need_to_lock)
{
  nvram->is_locked = need_to_lock;
}

uint32_t nvram_get_segment_id(nvram_t *nvram)
{
  return nvram->cache.sys.segment_id;
}

/**
 * @brief Write data from the cache to flash
 * @param nvram - pointer to the NVRAM instance
 */
void nvram_erase_and_write_segment(nvram_t *nvram)
{
#if (FEATURE_ENABLE_INT_NVRAM)
  if (nvram->driver == NVRAM_DRIVER__INT_FLASH)
  {
    nvram_int_flash_erase_and_write_segment(nvram->segment_idx, &nvram->cache);
  }
#endif

#if (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM)
  if (nvram->driver == NVRAM_DRIVER__EXT_NOR_XXX5)
  {
    nvram_ext_flash_erase_and_write_segment(nvram->address_begin, nvram->segment_idx,
                                            &nvram->cache);
  }
#endif
}

/**
 * @brief Get the NVRAM checksum.
 * @return Checksum.
 */
uint16_t nvram_get_crc(nvram_map_t *nvram_map)
{
  return nvram_checksum_calculate(nvram_map, NVRAM_SIZE - NVRAM_CRC_SIZE);
}

/**
 * @brief Read a block from the cache
 * @param nvram - pointer to the NVRAM instance
 * @param data - pointer to where the data is placed
 * @param address - address of the data to read
 * @param size - size of the data to read
 */
void nvram_read_block(nvram_t *nvram, void *data, uint32_t address, uint16_t size)
{
  hal_assert(nvram->inited == true);
  memcpy(data, &nvram->cache.buf[address], size);
}

/**
 * @brief Put new data into the cache
 * @param nvram - pointer to the NVRAM instance
 * @param data - pointer to the data source
 * @param address - cache address to put the data at
 * @param size - size of the new data
 */
void nvram_write_block(nvram_t *nvram, void *data, uint32_t address, uint16_t size)
{
  hal_assert(nvram->inited == true);
  if (nvram->is_locked)
  {
    return;
  }
  for (uint16_t i = 0; i < size; i++, address++)
  {
    if (nvram->cache.buf[address] != ((uint8_t *)data)[i])
    {
      nvram->cache.buf[address] = ((uint8_t *)data)[i];
      nvram->need_update = 1;
    }
  }
}

/**
 * Find the flash segment that holds the most recent settings and load it.
 * @param nvram - pointer to the NVRAM instance
 * @return see error_t
 */
error_t nvram_find_segment_and_load(nvram_t *nvram)
{
  error_t res = ERR_RESULT;

#if (FEATURE_ENABLE_INT_NVRAM)
  if (nvram->driver == NVRAM_DRIVER__INT_FLASH)
  {
    res = nvram_int_flash_find_segment_and_load(nvram->version, &nvram->segment_idx, &nvram->cache);
  }
#endif

#if (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM)
  if (nvram->driver == NVRAM_DRIVER__EXT_NOR_XXX5)
  {
    res = nvram_ext_flash_find_segment_and_load(nvram->address_begin, nvram->segment_amount,
                                                nvram->version, &nvram->segment_idx, &nvram->cache);
  }
#endif

  return res;
}
