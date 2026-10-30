/**
 * @file ext_flash_xx25.c
 * @brief Driver for SPI NOR flash chips of the xx25 family (M25, N25, MX25, W25).
 */

#include "config.h"

#include "drivers/ext_flash_hw.h"
#include "drivers/ext_flash_xx25.h"
#include "drivers/tick_timer.h"
#include "utils/assert.h"

#define EXT_FLASH_XX25_TEST_SEGMENT_AMOUNT 4
#define EXT_FLASH_XX25_TEST_PAGE_AMOUNT 8
#define EXT_FLASH_XX25_TEST_MAGIC_NUMBER 42

/// Minimum segment erase timeout for the N25xx chip family, ms
#define FLASH_N25_SEGMENT_ERASE_TIMEOUT 800
/// Page program timeout for the N25xx chip family, ms
#define FLASH_N25_PAGE_PROGRAM_TIMEOUT 5

/// Minimum segment erase timeout for the M25xx chip family, ms
#define FLASH_M25_SEGMENT_ERASE_TIMEOUT 600
/// Page program timeout for the M25xx chip family, ms
#define FLASH_M25_PAGE_PROGRAM_TIMEOUT 1

/// Minimum segment erase timeout for the MX25xx chip family, ms
#define FLASH_MX25_SEGMENT_ERASE_TIMEOUT 120
/// Page program timeout for the MX25xx chip family, ms
#define FLASH_MX25_PAGE_PROGRAM_TIMEOUT 2

/// Minimum segment erase timeout for the WQ25xx chip family, ms
#define FLASH_WQ25_SEGMENT_ERASE_TIMEOUT 420
/// Page program timeout for the WQ25xx chip family, ms
#define FLASH_WQ25_PAGE_PROGRAM_TIMEOUT 4

/// Sector size, bytes
#define EXT_FLASH_XX45_SECTOR_SIZE 65536
/// Subsector size (not available in all chips), bytes
#define FLASH_XX25_SUBSECTOR_SIZE 4096

/// Commands for the FLASH chip
#define FLASH_XX25_CMD__READ_ID 0x9F
#define FLASH_XX25_CMD__READ_DATA_BYTES 0x03
#define FLASH_XX25_CMD__READ_STATUS_REGISTER 0x05
#define FLASH_XX25_CMD__PAGE_PROGRAM 0x02
#define FLASH_XX25_CMD__WRITE_ENABLE 0x06
#define FLASH_XX25_CMD__WRITE_DISABLE 0x04
#define FLASH_XX25_CMD__SECTOR_ERASE 0xD8
#define FLASH_XX25_CMD__SUBSECTOR_ERASE 0x20

/// Total flash size of M25P16, KB
#define FLASH_M25P16_TOTAL_SIZE 2048

/// Total flash size of N25Q128, KB
#define FLASH_N25Q128_TOTAL_SIZE 16384

/// Total flash size of N25Q032, KB
#define FLASH_N25Q032_TOTAL_SIZE 4096

/// Total flash size of MX25L128, KB
#define FLASH_MX25L128_TOTAL_SIZE 16384

/// Total flash size of WQ25F32, KB
#define FLASH_WQ25F32_TOTAL_SIZE 4096

#define EXT_FLASH_XX25_STATUS_WRITE_IN_PROG_MSK 0x01

#pragma pack(push, 1)
typedef union
{
  struct
  {
    uint8_t write_in_progress : 1;
    uint8_t write_enable_latch : 1;
    uint8_t block_protect : 3;
    uint8_t reserved : 2;
    uint8_t write_protect : 1;
  } m25;

  struct
  {
    uint8_t write_in_progress : 1;
    uint8_t write_enable_latch : 1;
    uint8_t block_protect : 3;
    /// 1 - bottom
    /// 0 - top
    uint8_t top_bottom : 1;
    uint8_t reserved : 1;
    uint8_t write_protect : 1;
  } n25;

  struct
  {
    uint8_t write_in_progress : 1;
    uint8_t write_enable_latch : 1;
    uint8_t block_protect : 4;
    uint8_t quad_en : 1;
    uint8_t write_protect : 1;
  } mx25;

  struct
  {
    uint8_t write_in_progress : 1;
    uint8_t write_enable_latch : 1;
    uint8_t block_protect : 3;
    uint8_t top_bottom_protect : 1;
    uint8_t sector_protect : 1;
    uint8_t status_reg_protect : 1;
  } wq25;

  uint8_t msk;
} ext_flash_xx25_status_t;
#pragma pack(pop)

uint32_t ext_flash_xx25_get_type(void);

ext_flash_xx25_t ext_flash_xx25;

error_t ext_flash_xx25_autodefine(void)
{
  error_t res;

  ext_flash_xx25.inited = 0;
  ext_flash_xx25.type = EXT_FLASH_XX25_TYPE__UNKNOWN;
  ext_flash_xx25.size_in_kb = 0;
  ext_flash_xx25.erasable_segment_size = 0;
  ext_flash_xx25.erase_segment_timeout = 0;

  // the first call is a dummy one
  ext_flash_xx25.type = (ext_flash_xx25_type_t)ext_flash_xx25_get_type();
  ext_flash_xx25.type = (ext_flash_xx25_type_t)ext_flash_xx25_get_type();

  switch (ext_flash_xx25.type)
  {
    case EXT_FLASH_XX25_TYPE__M25P16:
      ext_flash_xx25.size_in_kb = FLASH_M25P16_TOTAL_SIZE;
      ext_flash_xx25.erase_subsector_enable = false;
      ext_flash_xx25.erasable_segment_size = EXT_FLASH_XX45_SECTOR_SIZE;
      ext_flash_xx25.erase_segment_timeout = FLASH_M25_SEGMENT_ERASE_TIMEOUT;
      ext_flash_xx25.program_page_timeout = FLASH_M25_PAGE_PROGRAM_TIMEOUT;
      res = ERR_OK;
      break;

    case EXT_FLASH_XX25_TYPE__N25Q128:
      ext_flash_xx25.size_in_kb = FLASH_N25Q128_TOTAL_SIZE;
      ext_flash_xx25.erase_subsector_enable = true;
      ext_flash_xx25.erasable_segment_size = FLASH_XX25_SUBSECTOR_SIZE;
      ext_flash_xx25.erase_segment_timeout = FLASH_N25_SEGMENT_ERASE_TIMEOUT;
      ext_flash_xx25.program_page_timeout = FLASH_N25_PAGE_PROGRAM_TIMEOUT;
      res = ERR_OK;
      break;

    case EXT_FLASH_XX25_TYPE__N25Q032:
      ext_flash_xx25.size_in_kb = FLASH_N25Q032_TOTAL_SIZE;
      ext_flash_xx25.erase_subsector_enable = true;
      ext_flash_xx25.erasable_segment_size = FLASH_XX25_SUBSECTOR_SIZE;
      ext_flash_xx25.erase_segment_timeout = FLASH_N25_SEGMENT_ERASE_TIMEOUT;
      ext_flash_xx25.program_page_timeout = FLASH_N25_PAGE_PROGRAM_TIMEOUT;
      res = ERR_OK;
      break;

    case EXT_FLASH_XX25_TYPE__MX25L128:
      ext_flash_xx25.size_in_kb = FLASH_MX25L128_TOTAL_SIZE;
      ext_flash_xx25.erase_subsector_enable = true;
      ext_flash_xx25.erasable_segment_size = FLASH_XX25_SUBSECTOR_SIZE;
      ext_flash_xx25.erase_segment_timeout = FLASH_MX25_SEGMENT_ERASE_TIMEOUT;
      ext_flash_xx25.program_page_timeout = FLASH_MX25_PAGE_PROGRAM_TIMEOUT;
      res = ERR_OK;
      break;

    case EXT_FLASH_XX25_TYPE__W25Q32F:
      ext_flash_xx25.size_in_kb = FLASH_WQ25F32_TOTAL_SIZE;
      ext_flash_xx25.erase_subsector_enable = true;
      ext_flash_xx25.erasable_segment_size = FLASH_XX25_SUBSECTOR_SIZE;
      ext_flash_xx25.erase_segment_timeout = FLASH_WQ25_SEGMENT_ERASE_TIMEOUT;
      ext_flash_xx25.program_page_timeout = FLASH_WQ25_PAGE_PROGRAM_TIMEOUT;
      res = ERR_OK;
      break;

    default:
      res = ERR_RESULT;
      break;
  }

  if (res == ERR_OK)
  {
    ext_flash_xx25.inited = true;
  }

  return res;
}

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
error_t ext_flash_xx25_test(void)
{
  error_t res = ERR_OK;

  res = ext_flash_xx25_autodefine();
  if (res != ERR_OK)
  {
    return res;
  }

  uint8_t init_value = EXT_FLASH_XX25_TEST_MAGIC_NUMBER;
  uint8_t test_buf[EXT_FLASH_XX25_PAGE_SIZE];

  // write
  for (uint8_t segment_idx = 0; segment_idx < EXT_FLASH_XX25_TEST_SEGMENT_AMOUNT; segment_idx++)
  {
    uint32_t address = ext_flash_xx25.erasable_segment_size * segment_idx;
    if (ext_flash_xx25.erase_subsector_enable)
    {
      ext_flash_xx25_erase_subsector_req(address);
    }
    else
    {
      ext_flash_xx25_erase_sector_req(address);
    }

    while (ext_flash_xx25_flash_is_busy())
    {
    }

    for (uint8_t page_idx = 0; page_idx < EXT_FLASH_XX25_TEST_PAGE_AMOUNT; page_idx++)
    {
      for (uint16_t i = 0; i < EXT_FLASH_XX25_PAGE_SIZE; i++)
      {
        test_buf[i] = init_value++;
      }
      ext_flash_xx25_program_page_req(address, test_buf);

      while (ext_flash_xx25_flash_is_busy())
      {
      }

      // extra increment to avoid repeating values
      init_value++;
      address += EXT_FLASH_XX25_PAGE_SIZE;
    }
  }

  init_value = EXT_FLASH_XX25_TEST_MAGIC_NUMBER;

  // read
  for (uint8_t segment_idx = 0; segment_idx < EXT_FLASH_XX25_TEST_SEGMENT_AMOUNT; segment_idx++)
  {
    uint32_t address = ext_flash_xx25.erasable_segment_size * segment_idx;

    for (uint8_t page_idx = 0; page_idx < EXT_FLASH_XX25_TEST_PAGE_AMOUNT; page_idx++)
    {
      ext_flash_xx25_read_array(address, test_buf, EXT_FLASH_XX25_PAGE_SIZE);

      for (uint16_t i = 0; i < EXT_FLASH_XX25_PAGE_SIZE; i++)
      {
        if (test_buf[i] != init_value)
        {
          return ERR_RESULT;
        }
        init_value++;
      }

      // extra increment to avoid repeating values
      init_value++;
      address += EXT_FLASH_XX25_PAGE_SIZE;
    }
  }

  return res;
}

uint32_t ext_flash_xx25_get_type(void)
{
  uint32_t res;

  ext_flash_hw_enable();

  ext_flash_hw_tx_rx_byte(FLASH_XX25_CMD__READ_ID);
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
bool ext_flash_xx25_flash_is_busy(void)
{
  if (!ext_flash_xx25.inited)
  {
    return false;
  }

  ext_flash_hw_enable();

  ext_flash_xx25_status_t status;

  ext_flash_hw_tx_rx_byte(FLASH_XX25_CMD__READ_STATUS_REGISTER);
  status.msk = ext_flash_hw_tx_rx_byte(0);

  ext_flash_hw_disable();

  return ((status.msk & EXT_FLASH_XX25_STATUS_WRITE_IN_PROG_MSK) != 0);
}

uint8_t ext_flash_xx25_flash_get_status_register(void)
{
  ext_flash_hw_enable();

  ext_flash_hw_tx_rx_byte(FLASH_XX25_CMD__READ_STATUS_REGISTER);
  uint8_t res = ext_flash_hw_tx_rx_byte(0);

  ext_flash_hw_disable();

  return res;
}

void ext_flash_xx25_read_array(uint32_t address, void *buf, uint32_t size)
{
  if (!ext_flash_xx25.inited)
  {
    return;
  }

  ext_flash_hw_enable();

  uint8_t cmd[] = {FLASH_XX25_CMD__READ_DATA_BYTES, address >> 16, address >> 8, address >> 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));

  ext_flash_hw_rx_array(buf, size);

  ext_flash_hw_disable();
}

void ext_flash_xx25_erase_sector_req(uint32_t address)
{
  if (!ext_flash_xx25.inited)
  {
    return;
  }

  address &= ~(EXT_FLASH_XX45_SECTOR_SIZE - 1);

  while (ext_flash_xx25_flash_is_busy())
  {
  }

  ext_flash_hw_enable();
  ext_flash_hw_tx_rx_byte(FLASH_XX25_CMD__WRITE_ENABLE);
  ext_flash_hw_disable();

  ext_flash_hw_enable();

  uint8_t cmd[] = {FLASH_XX25_CMD__SECTOR_ERASE, address >> 16, address >> 8, address >> 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));

  ext_flash_hw_disable();
}

void ext_flash_xx25_erase_subsector_req(uint32_t address)
{
  if (!ext_flash_xx25.inited)
  {
    return;
  }

  hal_assert(ext_flash_xx25.erase_subsector_enable);

  address &= ~(FLASH_XX25_SUBSECTOR_SIZE - 1);

  while (ext_flash_xx25_flash_is_busy())
  {
  }

  ext_flash_hw_enable();
  ext_flash_hw_tx_rx_byte(FLASH_XX25_CMD__WRITE_ENABLE);
  ext_flash_hw_disable();

  ext_flash_hw_enable();

  uint8_t cmd[] = {FLASH_XX25_CMD__SUBSECTOR_ERASE, address >> 16, address >> 8, address >> 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));

  ext_flash_hw_disable();
}

void ext_flash_xx25_program_page_req(uint32_t address, void *buf)
{
  if (!ext_flash_xx25.inited)
  {
    return;
  }

  address &= ~(EXT_FLASH_XX25_PAGE_SIZE - 1);

  while (ext_flash_xx25_flash_is_busy())
  {
  }

  ext_flash_hw_enable();
  ext_flash_hw_tx_rx_byte(FLASH_XX25_CMD__WRITE_ENABLE);
  ext_flash_hw_disable();

  ext_flash_hw_enable();

  uint8_t cmd[] = {FLASH_XX25_CMD__PAGE_PROGRAM, address >> 16, address >> 8, address >> 0};
  ext_flash_hw_tx_array(cmd, sizeof(cmd));
  ext_flash_hw_tx_array((uint8_t *)buf, EXT_FLASH_XX25_PAGE_SIZE);

  ext_flash_hw_disable();
}
