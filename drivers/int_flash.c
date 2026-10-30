/**
 * @file int_flash.c
 * @brief Driver for the MCU internal flash memory.
 *
 * Usage:
 *   1. Create a driver instance, int_flash_t.
 *   2. Initialize the driver using int_flash_init_struct_t, which must define the start
 *      address of the flash region and, if possible, its end address (otherwise use
 *      INT_FLASH_UNKNOWN_END_ADDRESS), and call int_flash_init.
 *   3. To write to flash use int_flash_write - writes an array at the given address.
 *   4. To read use int_flash_read - copies an array from the given address into
 *      a prepared buffer.
 */

#include "config.h"

#include <string.h>

#include "utils/assert.h"
#include "utils/errors.h"

#include "drivers/int_flash.h"

#if defined(STM32F4XX)
  /**
   * @brief Flash sector layout for the STM32F4xx family
   * @addtogroup STM32F4XX_FLASH_SECTORS
   * @{
   */
  /// Sector 0 address, size 16 KB
  #define ADDR_FLASH_SECTOR_0 ((uint32_t)0x08000000)
  /// Sector 1 address, size 16 KB
  #define ADDR_FLASH_SECTOR_1 ((uint32_t)0x08004000)
  /// Sector 2 address, size 16 KB
  #define ADDR_FLASH_SECTOR_2 ((uint32_t)0x08008000)
  /// Sector 3 address, size 16 KB
  #define ADDR_FLASH_SECTOR_3 ((uint32_t)0x0800C000)
  /// Sector 4 address, size 64 KB
  #define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000)
  /// Sector 5 address, size 128 KB
  #define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08020000)
  /// Sector 6 address, size 128 KB
  #define ADDR_FLASH_SECTOR_6 ((uint32_t)0x08040000)
  /// Sector 7 address, size 128 KB
  #define ADDR_FLASH_SECTOR_7 ((uint32_t)0x08060000)
  /// Sector 8 address, size 128 KB
  #define ADDR_FLASH_SECTOR_8 ((uint32_t)0x08080000)
  /// Sector 9 address, size 128 KB
  #define ADDR_FLASH_SECTOR_9 ((uint32_t)0x080A0000)
  /// Sector 10 address, size 128 KB
  #define ADDR_FLASH_SECTOR_10 ((uint32_t)0x080C0000)
  /// Sector 11 address, size 128 KB
  #define ADDR_FLASH_SECTOR_11 ((uint32_t)0x080E0000)

  /// Sector 12 address, size 16 KB
  #define ADDR_FLASH_SECTOR_12 ((uint32_t)0x08100000)
  /// Sector 13 address, size 16 KB
  #define ADDR_FLASH_SECTOR_13 ((uint32_t)0x08104000)
  /// Sector 14 address, size 16 KB
  #define ADDR_FLASH_SECTOR_14 ((uint32_t)0x08108000)
  /// Sector 15 address, size 16 KB
  #define ADDR_FLASH_SECTOR_15 ((uint32_t)0x0810C000)
  /// Sector 16 address, size 64 KB
  #define ADDR_FLASH_SECTOR_16 ((uint32_t)0x08110000)
  /// Sector 17 address, size 128 KB
  #define ADDR_FLASH_SECTOR_17 ((uint32_t)0x08120000)
  /// Sector 18 address, size 128 KB
  #define ADDR_FLASH_SECTOR_18 ((uint32_t)0x08140000)
  /// Sector 19 address, size 128 KB
  #define ADDR_FLASH_SECTOR_19 ((uint32_t)0x08160000)
  /// Sector 20 address, size 128 KB
  #define ADDR_FLASH_SECTOR_20 ((uint32_t)0x08180000)
  /// Sector 21 address, size 128 KB
  #define ADDR_FLASH_SECTOR_21 ((uint32_t)0x081A0000)
  /// Sector 22 address, size 128 KB
  #define ADDR_FLASH_SECTOR_22 ((uint32_t)0x081C0000)
  /// Sector 23 address, size 128 KB
  #define ADDR_FLASH_SECTOR_23 ((uint32_t)0x081E0000)
  /// End address of sector 23
  #define ADDR_FLASH_END_SECTORS ((uint32_t)0x08200000)
/** @} */

  #if (FLASH_SECTOR_AMOUNT == 8)
    /// Flash end address
    #define INT_FLASH_END_ADDRESS ADDR_FLASH_SECTOR_8
  #elif (FLASH_SECTOR_AMOUNT == 12)
    /// Flash end address
    #define INT_FLASH_END_ADDRESS ADDR_FLASH_SECTOR_12
  #elif (FLASH_SECTOR_AMOUNT == 24)
    /// Flash end address
    #define INT_FLASH_END_ADDRESS ADDR_FLASH_END_SECTORS
  #endif

  #define INT_FLASH_FLAG_ALL                                                                       \
    (FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |                   \
     FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR)

#elif defined(STM32L4XX)
  #define FLASH_SECTOR_ADDRESS_MSK (FLASH_SECTOR_SIZE - 1)

#elif defined(STM32F37X) || defined(STM32F30X)
  #define FLASH_SECTOR_ADDRESS_MSK (FLASH_SECTOR_SIZE - 1)

  #define INT_FLASH_FLAG_ALL (FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR)

#elif defined(STM32F1XX)
  #define FLASH_SECTOR_ADDRESS_MSK (FLASH_SECTOR_SIZE - 1)

  #define INT_FLASH_FLAG_ALL                                                                       \
    (FLASH_FLAG_EOP | FLASH_FLAG_WRPRTERR | FLASH_FLAG_PGERR | FLASH_FLAG_OPTERR)

#elif defined(GD32E11X)
  #define FLASH_SECTOR_ADDRESS_MSK (FLASH_SECTOR_SIZE - 1)

  #define INT_FLASH_FLAG_ALL (FMC_FLAG_PGERR | FMC_FLAG_PGAERR | FMC_FLAG_WPERR | FMC_FLAG_END)

#elif defined(GD32L23X)
  #define FLASH_SECTOR_ADDRESS_MSK (FLASH_SECTOR_SIZE - 1)

#else
  #error This MCU not supported
#endif

/// Number of bytes in a kilobyte
#define INT_FLASH_BYTES_PER_KB 1024

/// End address of the usable FLASH region
NO_INIT uint32_t int_flash_flash_end_addr;

#if defined(STM32F4XX)
/// FLASH sector information for the STM32F4xx family
const int_flash_sector_t int_flash_sectors[FLASH_SECTOR_AMOUNT] = {
    {ADDR_FLASH_SECTOR_0, ADDR_FLASH_SECTOR_1 - ADDR_FLASH_SECTOR_0, FLASH_Sector_0},
    {ADDR_FLASH_SECTOR_1, ADDR_FLASH_SECTOR_2 - ADDR_FLASH_SECTOR_1, FLASH_Sector_1},
    {ADDR_FLASH_SECTOR_2, ADDR_FLASH_SECTOR_3 - ADDR_FLASH_SECTOR_2, FLASH_Sector_2},
    {ADDR_FLASH_SECTOR_3, ADDR_FLASH_SECTOR_4 - ADDR_FLASH_SECTOR_3, FLASH_Sector_3},
    {ADDR_FLASH_SECTOR_4, ADDR_FLASH_SECTOR_5 - ADDR_FLASH_SECTOR_4, FLASH_Sector_4},
    {ADDR_FLASH_SECTOR_5, ADDR_FLASH_SECTOR_6 - ADDR_FLASH_SECTOR_5, FLASH_Sector_5},
    {ADDR_FLASH_SECTOR_6, ADDR_FLASH_SECTOR_7 - ADDR_FLASH_SECTOR_6, FLASH_Sector_6},
    {ADDR_FLASH_SECTOR_7, ADDR_FLASH_SECTOR_8 - ADDR_FLASH_SECTOR_7, FLASH_Sector_7},
  #if (FLASH_SECTOR_AMOUNT > 8)
    {ADDR_FLASH_SECTOR_8, ADDR_FLASH_SECTOR_9 - ADDR_FLASH_SECTOR_8, FLASH_Sector_8},
    {ADDR_FLASH_SECTOR_9, ADDR_FLASH_SECTOR_10 - ADDR_FLASH_SECTOR_9, FLASH_Sector_9},
    {ADDR_FLASH_SECTOR_10, ADDR_FLASH_SECTOR_11 - ADDR_FLASH_SECTOR_10, FLASH_Sector_10},
    {ADDR_FLASH_SECTOR_11, ADDR_FLASH_SECTOR_12 - ADDR_FLASH_SECTOR_11, FLASH_Sector_11},
  #endif
  #if (FLASH_SECTOR_AMOUNT > 12)
    {ADDR_FLASH_SECTOR_12, ADDR_FLASH_SECTOR_13 - ADDR_FLASH_SECTOR_12, FLASH_Sector_12},
    {ADDR_FLASH_SECTOR_13, ADDR_FLASH_SECTOR_14 - ADDR_FLASH_SECTOR_13, FLASH_Sector_13},
    {ADDR_FLASH_SECTOR_14, ADDR_FLASH_SECTOR_15 - ADDR_FLASH_SECTOR_14, FLASH_Sector_14},
    {ADDR_FLASH_SECTOR_15, ADDR_FLASH_SECTOR_16 - ADDR_FLASH_SECTOR_15, FLASH_Sector_15},
    {ADDR_FLASH_SECTOR_16, ADDR_FLASH_SECTOR_17 - ADDR_FLASH_SECTOR_16, FLASH_Sector_16},
    {ADDR_FLASH_SECTOR_17, ADDR_FLASH_SECTOR_18 - ADDR_FLASH_SECTOR_17, FLASH_Sector_17},
    {ADDR_FLASH_SECTOR_18, ADDR_FLASH_SECTOR_19 - ADDR_FLASH_SECTOR_18, FLASH_Sector_18},
    {ADDR_FLASH_SECTOR_19, ADDR_FLASH_SECTOR_20 - ADDR_FLASH_SECTOR_19, FLASH_Sector_19},
    {ADDR_FLASH_SECTOR_20, ADDR_FLASH_SECTOR_21 - ADDR_FLASH_SECTOR_20, FLASH_Sector_20},
    {ADDR_FLASH_SECTOR_21, ADDR_FLASH_SECTOR_22 - ADDR_FLASH_SECTOR_21, FLASH_Sector_21},
    {ADDR_FLASH_SECTOR_22, ADDR_FLASH_SECTOR_23 - ADDR_FLASH_SECTOR_22, FLASH_Sector_22},
    {ADDR_FLASH_SECTOR_23, ADDR_FLASH_END_SECTORS - ADDR_FLASH_SECTOR_23, FLASH_Sector_23},
  #endif
};
#endif

bool int_flash_is_address_in_flash(uint32_t address);

/**
 * @brief Initialize the FLASH driver
 * @param int_flash - pointer to the FLASH driver instance
 * @param init_struct - pointer to the settings structure
 */
void int_flash_init(int_flash_t *int_flash, int_flash_init_struct_t *init_struct)
{
  hal_assert(init_struct != NULL);
  hal_assert(int_flash != NULL);

  // Get the flash end address by reading the dedicated register
  int_flash_flash_end_addr =
      FLASH_BASE + (*(__IO int16_t *)DEVICE_FLASH_SIZE_ADDRESS) * INT_FLASH_BYTES_PER_KB - 1;

  hal_assert(int_flash_is_address_in_flash(init_struct->begin_addr));

  int_flash->begin_addr = init_struct->begin_addr;
  int_flash->end_addr = int_flash_flash_end_addr;

  // If the end of the flash region is explicitly given in the init structure
  if (init_struct->end_addr != INT_FLASH_UNKNOWN_END_ADDRESS)
  {
    hal_assert(int_flash_is_address_in_flash(init_struct->end_addr));
    int_flash->end_addr = init_struct->end_addr;
  }
}

/**
 * @brief Read a data array from flash into a buffer
 * @param int_flash - flash driver instance
 * @param src_addr - start address to read from
 * @param buf - pointer to the array that receives the data
 * @param size - amount of data to read
 * @return ERR_OK - data read successfully
 *         ERR_PARAM - read error
 */
error_t int_flash_read(int_flash_t *int_flash, uint32_t src_addr, uint8_t *buf, uint16_t size)
{
  // Check that the read start address is within the accessible region
  if ((src_addr < int_flash->begin_addr) || (src_addr > int_flash->end_addr))
  {
    return ERR_PARAM;
  }

  // Check that the read end address is within the accessible region
  if ((src_addr + size - 1) > int_flash->end_addr)
  {
    return ERR_PARAM;
  }

  memcpy((void *)buf, (void *)src_addr, size);

  return ERR_OK;
}

/**
 * @brief Write an array byte by byte starting at the given address
 * @note Before writing, make sure the target region has been erased beforehand
 * @note Writing is done byte by byte
 * @param int_flash - pointer to the flash driver instance
 * @param address_dst - start address of the data to write
 * @param buf - pointer to the array to write
 * @param size - size of the data buffer
 * @return ERR_OK - data written successfully, otherwise - error
 */
error_t int_flash_write(int_flash_t *int_flash, uint32_t address_dst, uint8_t *buf, uint16_t size)
{
  // Check that the write start address is within the accessible region
  if ((address_dst < int_flash->begin_addr) || (address_dst > int_flash->end_addr))
  {
    return ERR_PARAM;
  }

  // Check that the write end address is within the accessible region
  if ((address_dst + size - 1) > int_flash->end_addr)
  {
    return ERR_PARAM;
  }

  error_t res = ERR_OK;

  uint32_t save;
  ENTER_CRITICAL(save);

// Write the data
#if defined(STM32F4XX)
  FLASH_Unlock();
  FLASH_ClearFlag(INT_FLASH_FLAG_ALL);

  for (uint32_t i = 0; i < size; i++)
  {
    FLASH_Status result = FLASH_ProgramByte(address_dst++, *buf++);

    if (result != FLASH_COMPLETE)
    {
      res = ERR_RESULT;
      break;
    }
  }
  FLASH_Lock();

#elif defined(STM32L4XX)
  // Size must be a multiple of 8 bytes
  hal_assert((size & (sizeof(uint64_t) - 1)) == 0);

  HAL_FLASH_Unlock();

  // Writing is done in 8-byte groups
  for (uint32_t i = 0; i < (size / sizeof(uint64_t)); i++)
  {
    // without this trick the compiler would use the LDRD instruction,
    // which requires aligned operands
    uint64_t write_data = ((uint32_t *)buf)[i * 2];
    write_data |= ((uint64_t)(((uint32_t *)buf)[i * 2 + 1])) << 32;

    HAL_StatusTypeDef result =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address_dst, write_data);
    address_dst += sizeof(uint64_t);

    if (result != HAL_OK)
    {
      res = ERR_RESULT;
      break;
    }
  }
  HAL_FLASH_Lock();

#elif (defined(STM32F37X) || defined(STM32F30X))
  FLASH_Unlock();
  FLASH_ClearFlag(INT_FLASH_FLAG_ALL);

  // Writing is done in 2-byte groups
  for (uint32_t i = 0; i < (size / sizeof(uint16_t)); i++)
  {
    FLASH_Status result = FLASH_ProgramHalfWord(address_dst, ((uint16_t *)buf)[i]);
    address_dst += sizeof(uint16_t);

    if (result != FLASH_COMPLETE)
    {
      res = ERR_RESULT;
      break;
    }
  }
  FLASH_Lock();

#elif defined(STM32F1XX)
  FLASH_Unlock();
  FLASH_ClearFlag(INT_FLASH_FLAG_ALL);

  // Writing is done in 2-byte groups
  for (uint32_t i = 0; i < (size / sizeof(uint16_t)); i++)
  {
    FLASH_Status result = FLASH_ProgramHalfWord(address_dst, ((uint16_t *)buf)[i]);
    address_dst += sizeof(uint16_t);

    if (result != FLASH_COMPLETE)
    {
      res = ERR_RESULT;
      break;
    }
  }
  FLASH_Lock();

#elif defined(GD32E11X) || defined(GD32L23X)
  fmc_unlock();

  #if defined(GD32E11X)
  fmc_flag_clear(INT_FLASH_FLAG_ALL);
  #elif defined(GD32L23X)
  // On GD32L23X these flags can ONLY be cleared one at a time
  fmc_flag_clear(FMC_FLAG_PGERR);
  fmc_flag_clear(FMC_FLAG_PGAERR);
  fmc_flag_clear(FMC_FLAG_WPERR);
  fmc_flag_clear(FMC_FLAG_END);
  fmc_flag_clear(FMC_FLAG_SLP);
  #endif

  // Writing is done in 4-byte groups
  for (uint32_t i = 0; i < (size / sizeof(uint32_t)); i++)
  {
    fmc_state_enum result = fmc_word_program(address_dst, ((uint32_t *)buf)[i]);
    address_dst += sizeof(uint32_t);

    if (result != FMC_READY)
    {
      res = ERR_RESULT;
      break;
    }
  }
  fmc_lock();
#else
  #error This MCU not supported
#endif

  LEAVE_CRITICAL(save);

  return res;
}

/**
 * @brief Erase one FLASH sector/page
 * @param sector_idx - sector index
 * @return ERR_OK - sector erased successfully
 *         ERR_RESULT - sector erase error
 */
error_t int_flash_erase_sector(uint16_t sector_idx)
{
#if defined(STM32F4XX)
  if (IS_FLASH_SECTOR(int_flash_sectors[sector_idx].id) == false)
  {
    return ERR_RESULT;
  }
#endif

  error_t res = ERR_OK;

  uint32_t save;
  ENTER_CRITICAL(save);

#if defined(STM32L4XX)
  HAL_FLASH_Unlock();

  uint32_t address_dst = FLASH_BASE + FLASH_SECTOR_SIZE * sector_idx;
  if (address_dst > int_flash_flash_end_addr)
  {
    return ERR_PARAM;
  }

  FLASH_EraseInitTypeDef erase_init;
  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.NbPages = 1;
  erase_init.Page = sector_idx;

  if ((address_dst & (~FLASH_BASE)) < FLASH_BANK_SIZE)
  {
    erase_init.Banks = FLASH_BANK_1;
  }
  else
  {
    erase_init.Banks = FLASH_BANK_2;
  }

  uint32_t page_error;
  HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);

  HAL_FLASH_Lock();

  if (status != HAL_OK)
  {
    res = ERR_RESULT;
  }

#elif defined(GD32E11X) || defined(GD32L23X)
  fmc_unlock();

  #if defined(GD32E11X)
  fmc_flag_clear(INT_FLASH_FLAG_ALL);
  #elif defined(GD32L23X)
  // On GD32L23X these flags can ONLY be cleared one at a time
  fmc_flag_clear(FMC_FLAG_PGERR);
  fmc_flag_clear(FMC_FLAG_PGAERR);
  fmc_flag_clear(FMC_FLAG_WPERR);
  fmc_flag_clear(FMC_FLAG_END);
  fmc_flag_clear(FMC_FLAG_SLP);
  #endif

  uint32_t address_dst = FLASH_BASE + FLASH_SECTOR_SIZE * sector_idx;
  if (address_dst > int_flash_flash_end_addr)
  {
    fmc_lock();
    return ERR_PARAM;
  }
  fmc_state_enum state = fmc_page_erase(address_dst);

  fmc_lock();

  if (state != FMC_READY)
  {
    res = ERR_RESULT;
  }
#else

  FLASH_Unlock();
  FLASH_ClearFlag(INT_FLASH_FLAG_ALL);

  #if defined(STM32F30X) || defined(STM32F37X) || defined(STM32F1XX)
  uint32_t address_dst = FLASH_BASE + FLASH_SECTOR_SIZE * sector_idx;
  if (address_dst > int_flash_flash_end_addr)
  {
    return ERR_PARAM;
  }
  FLASH_Status status = FLASH_ErasePage(address_dst);

  #elif defined(STM32F4XX)

  FLASH_Status status = FLASH_EraseSector(int_flash_sectors[sector_idx].id, VoltageRange_3);
  #endif

  FLASH_Lock();

  if (status != FLASH_COMPLETE)
  {
    res = ERR_RESULT;
  }
#endif

  LEAVE_CRITICAL(save);

  return res;
}

/**
 * @brief Get the index of the flash sector that contains the given address
 * @param address - address to look up
 * @param sector_idx - resulting flash sector
 * @param is_sector_begin - flag that the given address is the beginning of a sector
 * @return ERR_OK - sector index found, otherwise - error.
 */
error_t int_flash_get_sector_idx(uint32_t address, uint16_t *sector_idx, bool *is_sector_begin)
{
  error_t res = ERR_RESULT;

#if defined(STM32F4XX)
  uint8_t idx;

  for (idx = 0; idx < FLASH_SECTOR_AMOUNT; idx++)
  {
    if ((address >= int_flash_sectors[idx].address) &&
        (address < (int_flash_sectors[idx].address + int_flash_sectors[idx].size)))
    {

      *is_sector_begin = (address == int_flash_sectors[idx].address);
      *sector_idx = idx;
      res = ERR_OK;
      break;
    }
  }

#elif defined(STM32F30X) || defined(STM32F37X) || defined(STM32L4XX) || defined(STM32F1XX) ||      \
    defined(GD32E11X) || defined(GD32L23X)
  if (address >= int_flash_flash_end_addr)
  {
    return ERR_PARAM;
  }

  *is_sector_begin = ((address & FLASH_SECTOR_ADDRESS_MSK) == 0);
  *sector_idx = (address - FLASH_BASE) / FLASH_SECTOR_SIZE;

  res = ERR_OK;

#else
  #error This MCU not supported
#endif

  return res;
}

/**
 * @brief Check whether the address lies within flash
 * @param address - address to check
 * @return true if the address is within flash
 */
bool int_flash_is_address_in_flash(uint32_t address)
{
  return ((address >= FLASH_BASE) && (address <= int_flash_flash_end_addr));
}
