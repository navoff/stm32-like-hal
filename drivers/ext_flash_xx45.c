/**
 * @file ext_flash_xx45.c
 * @brief Driver for SPI DataFlash chips of the xx45 family (AT45).
 */

#include "config.h"

#include "drivers/ext_flash_hw.h"
#include "drivers/ext_flash_xx45.h"
#include "utils/assert.h"

#define EXT_FLASH_XX45_TEST_SEGMENT_AMOUNT 4
#define EXT_FLASH_XX45_TEST_PAGE_AMOUNT 8
#define EXT_FLASH_XX45_TEST_MAGIC_NUMBER 43

/// Sector size, bytes
#define EXT_FLASH_XX45_SECTOR_SIZE 65536

/// Subsector size (not available in all chips), bytes
#define EXT_FLASH_XX45_SUBSECTOR_SIZE 4096

/// Total flash size of AT45DB32, KB
#define FLASH_AT45DB32_TOTAL_SIZE 4096

/// Page size of AT45DB32, bytes
#define FLASH_AT45_PAGE_SIZE 512

/// Minimum segment erase timeout for the AT45xx chip family, ms
#define FLASH_AT45_SEGMENT_ERASE_TIMEOUT 100

/// Page program timeout for the AT45xx chip family, ms
#define FLASH_AT45_PAGE_PROGRAM_TIMEOUT 6

/// Subsector size (not available in all chips), bytes
#define FLASH_XX45_SUBSECTOR_SIZE 4096

/// Commands for the FLASH chip
#define FLASH_XX45_CMD__READ_ID 0x9F
#define FLASH_XX45_CMD__READ_DATA_BYTES 0xD2
#define FLASH_XX45_CMD__READ_STATUS_REGISTER 0xD7
#define FLASH_XX45_CMD__PAGE_PROGRAM 0x02
#define FLASH_XX45_CMD__SECTOR_ERASE 0x7C
#define FLASH_XX45_CMD__SUBSECTOR_ERASE 0x50

#define FLASH_XX45_CMD__SET_BINARY_PAGE_SIZE 0xA6802A3D
#define FLASH_XX45_CMD__SET_STANDARD_DATA_FLASH_PAGE_SIZE 0xA7802A3D

#define EXT_FLASH_XX45_STATUS_DEVICE_IS_BUSY_MSK 0x80

#pragma pack(push, 1)
typedef union
{
  struct
  {
    /// Page size mode
    /// 0 - page size is 528 bytes
    /// 1 - page size is 512 bytes
    uint8_t page_size : 1;

    /// Whether sector protection is enabled
    uint8_t sector_protect : 1;

    uint8_t density_code : 4;
    uint8_t compare_result : 1;
    uint8_t device_is_ready : 1;
  } at45;

  uint8_t msk;
} ext_flash_xx45_status_t;
#pragma pack(pop)

uint32_t ext_flash_xx45_get_type(void);
void ext_flash_xx45_set_binary_page_size(void);
void ext_flash_xx45_set_standard_data_flash_page_size(void);

ext_flash_xx45_t ext_flash_xx45;

/**
 * External FLASH memory test.
 * Test algorithm:
 * 1. Detect the chip type (it must be supported by the code).
 * 2. Sequentially write test data into several pages of several sectors.
 * 3. Read the data back from the written pages and compare it with the test data.
 * The test passes if:
 * - the chip type was detected
 * - the chip type is supported by the driver
 * - the written data matches the data read back
 */
error_t ext_flash_xx45_test(void)
{
  error_t res = ERR_OK;

  res = ext_flash_xx45_autodefine();
  if (res != ERR_OK)
  {
    return res;
  }

  uint8_t init_value = EXT_FLASH_XX45_TEST_MAGIC_NUMBER;
  uint8_t test_buf[EXT_FLASH_XX45_PAGE_SIZE];

  // write
  for (uint8_t segment_idx = 0; segment_idx < EXT_FLASH_XX45_TEST_SEGMENT_AMOUNT; segment_idx++)
  {
    uint32_t address = ext_flash_xx45.erasable_segment_size * segment_idx;
    if (ext_flash_xx45.erase_subsector_enable)
    {
      ext_flash_xx45_erase_subsector_req(address);
    }
    else
    {
      ext_flash_xx45_erase_sector_req(address);
    }

    while (ext_flash_xx45_flash_is_busy())
    {
    }

    for (uint8_t page_idx = 0; page_idx < EXT_FLASH_XX45_TEST_PAGE_AMOUNT; page_idx++)
    {
      for (uint16_t i = 0; i < EXT_FLASH_XX45_PAGE_SIZE; i++)
      {
        test_buf[i] = init_value++;
      }
      ext_flash_xx45_program_page_req(address, test_buf);

      while (ext_flash_xx45_flash_is_busy())
      {
      }

      // extra increment to avoid repeating values
      init_value++;
      address += EXT_FLASH_XX45_PAGE_SIZE;
    }
  }

  init_value = EXT_FLASH_XX45_TEST_MAGIC_NUMBER;

  // read
  for (uint8_t segment_idx = 0; segment_idx < EXT_FLASH_XX45_TEST_SEGMENT_AMOUNT; segment_idx++)
  {
    uint32_t address = ext_flash_xx45.erasable_segment_size * segment_idx;

    for (uint8_t page_idx = 0; page_idx < EXT_FLASH_XX45_TEST_PAGE_AMOUNT; page_idx++)
    {
      ext_flash_xx45_read_array(address, test_buf, EXT_FLASH_XX45_PAGE_SIZE);

      for (uint16_t i = 0; i < EXT_FLASH_XX45_PAGE_SIZE; i++)
      {
        if (test_buf[i] != init_value)
        {
          return ERR_RESULT;
        }
        init_value++;
      }

      // extra increment to avoid repeating values
      init_value++;
      address += EXT_FLASH_XX45_PAGE_SIZE;
    }
  }

  return res;
}

error_t ext_flash_xx45_autodefine(void)
{
  error_t res = ERR_RESULT;

  ext_flash_xx45.inited = false;
  ext_flash_xx45.type = EXT_FLASH_XX45_TYPE__UNKNOWN;
  ext_flash_xx45.size_in_kb = 0;
  ext_flash_xx45.erasable_segment_size = 0;
  ext_flash_xx45.erase_segment_timeout = 0;

  // the first call is a dummy one
  ext_flash_xx45.type = (ext_flash_xx45_type_t)ext_flash_xx45_get_type();
  ext_flash_xx45.type = (ext_flash_xx45_type_t)ext_flash_xx45_get_type();

  switch (ext_flash_xx45.type)
  {
    case EXT_FLASH_XX45_TYPE__AT45DB32:
    {
      ext_flash_xx45.size_in_kb = FLASH_AT45DB32_TOTAL_SIZE;
      ext_flash_xx45.erase_subsector_enable = true;
      ext_flash_xx45.erasable_segment_size = FLASH_XX45_SUBSECTOR_SIZE;
      ext_flash_xx45.erase_segment_timeout = FLASH_AT45_SEGMENT_ERASE_TIMEOUT;
      ext_flash_xx45.program_page_timeout = FLASH_AT45_PAGE_PROGRAM_TIMEOUT;

      if (EXT_FLASH_XX45_PAGE_SIZE == FLASH_AT45_PAGE_SIZE)
      {
        ext_flash_xx45_set_binary_page_size();
        if (ext_flash_xx45_is_binary_page_size())
        {
          res = ERR_OK;
        }
      }
    }
    break;
    default:
      res = ERR_RESULT;
  }

  if (res == ERR_OK)
  {
    ext_flash_xx45.inited = true;
  }

  return res;
}

uint32_t ext_flash_xx45_get_type(void)
{
  uint32_t res;

  ext_flash_hw_enable();

  ext_flash_hw_tx_rx_byte(FLASH_XX45_CMD__READ_ID);
  res = ext_flash_hw_tx_rx_byte(0x0);
  res <<= 8;
  res |= ext_flash_hw_tx_rx_byte(0x0);
  res <<= 8;
  res |= ext_flash_hw_tx_rx_byte(0x0);

  ext_flash_hw_disable();

  return res;
}

/**
 * @brief Check whether the flash is currently busy
 * @return true - busy, false - not busy
 */
bool ext_flash_xx45_flash_is_busy(void)
{
  if (!ext_flash_xx45.inited)
  {
    return false;
  }

  ext_flash_xx45_status_t status;
  status.msk = ext_flash_xx45_flash_get_status_register();

  return ((status.msk & EXT_FLASH_XX45_STATUS_DEVICE_IS_BUSY_MSK) == 0);
}

/**
 * @brief Check whether the page size is set to a power of 2 (256 or 512)
 * @return true - page size is a power of 2 (256 or 512)
 */
bool ext_flash_xx45_is_binary_page_size(void)
{
  ext_flash_xx45_status_t status;
  status.msk = ext_flash_xx45_flash_get_status_register();

  return (status.at45.page_size != 0);
}

uint8_t ext_flash_xx45_flash_get_status_register(void)
{
  ext_flash_hw_enable();

  ext_flash_hw_tx_rx_byte(FLASH_XX45_CMD__READ_STATUS_REGISTER);
  uint8_t res = ext_flash_hw_tx_rx_byte(0);

  ext_flash_hw_disable();

  return res;
}

void ext_flash_xx45_set_binary_page_size(void)
{
  if (!ext_flash_xx45.inited)
  {
    return;
  }

  ext_flash_hw_enable();

  uint32_t cmd = FLASH_XX45_CMD__SET_BINARY_PAGE_SIZE;
  ext_flash_hw_tx_array((uint8_t *)&cmd, sizeof(cmd));

  ext_flash_hw_disable();

  while (ext_flash_xx45_flash_is_busy())
  {
  }
}

void ext_flash_xx45_set_standard_data_flash_page_size(void)
{
  if (!ext_flash_xx45.inited)
  {
    return;
  }

  ext_flash_hw_enable();

  uint32_t cmd = FLASH_XX45_CMD__SET_STANDARD_DATA_FLASH_PAGE_SIZE;
  ext_flash_hw_tx_array((uint8_t *)&cmd, sizeof(cmd));

  ext_flash_hw_disable();

  while (ext_flash_xx45_flash_is_busy())
  {
  }
}

void ext_flash_xx45_erase_sector_req(uint32_t address)
{
  if (!ext_flash_xx45.inited)
  {
    return;
  }

  address &= ~(EXT_FLASH_XX45_SECTOR_SIZE - 1);

  while (ext_flash_xx45_flash_is_busy())
  {
  }

  ext_flash_hw_enable();

  uint8_t cmd[] = {FLASH_XX45_CMD__SECTOR_ERASE, address >> 16, address >> 8, address >> 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));

  ext_flash_hw_disable();
}

void ext_flash_xx45_erase_subsector_req(uint32_t address)
{
  if (!ext_flash_xx45.inited)
  {
    return;
  }

  hal_assert(ext_flash_xx45.erase_subsector_enable);

  address &= ~(EXT_FLASH_XX45_SUBSECTOR_SIZE - 1);

  while (ext_flash_xx45_flash_is_busy())
  {
  }

  ext_flash_hw_enable();

  uint8_t cmd[] = {FLASH_XX45_CMD__SUBSECTOR_ERASE, address >> 16, address >> 8, address >> 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));

  ext_flash_hw_disable();
}

void ext_flash_xx45_program_page_req(uint32_t address, void *buf)
{
  if (!ext_flash_xx45.inited)
  {
    return;
  }

  address &= ~(EXT_FLASH_XX45_PAGE_SIZE - 1);

  while (ext_flash_xx45_flash_is_busy())
  {
  }

  ext_flash_hw_enable();

  uint8_t cmd[] = {FLASH_XX45_CMD__PAGE_PROGRAM, address >> 16, address >> 8, address >> 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));
  ext_flash_hw_tx_array((uint8_t *)buf, EXT_FLASH_XX45_PAGE_SIZE);

  ext_flash_hw_disable();
}

void ext_flash_xx45_read_array(uint32_t address, void *buf, uint32_t size)
{
  if (!ext_flash_xx45.inited)
  {
    return;
  }

  ext_flash_hw_enable();

  uint8_t cmd[] = {
      FLASH_XX45_CMD__READ_DATA_BYTES, address >> 16, address >> 8, address >> 0, 0, 0, 0, 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));

  ext_flash_hw_rx_array(buf, size);

  ext_flash_hw_disable();
}
