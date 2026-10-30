/**
 * @file checksum_test.c
 * @brief Self-test of the CRC implementations against reference check values.
 */

#include "config.h"

#include <string.h>

#include "utils/assert.h"
#include "utils/checksum.h"
#include "utils/macros.h"

void checksum_crc_test(void)
{
  char test_str[] = "123456789";
  uint32_t size = strlen(test_str);
  uint32_t crc = 0;

  // 1. check crc32 ethernet
  // 1.1. check slow alg
  hal_assert(checksum_crc32_ethernet_slow_calculate(test_str, size) ==
             CHECKSUM_CRC32_ETHERNET_REF_STRING_CHECK_VALUE);

  crc = checksum_crc32_ethernet_slow_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc32_ethernet_slow_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC32_ETHERNET_XOROUT_VALUE) ==
             CHECKSUM_CRC32_ETHERNET_REF_STRING_CHECK_VALUE);

  // 1.2. check fast alg
  hal_assert(checksum_crc32_ethernet_fast_calculate(test_str, size) ==
             CHECKSUM_CRC32_ETHERNET_REF_STRING_CHECK_VALUE);

  crc = checksum_crc32_ethernet_fast_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc32_ethernet_fast_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC32_ETHERNET_XOROUT_VALUE) ==
             CHECKSUM_CRC32_ETHERNET_REF_STRING_CHECK_VALUE);

#if (FEATURE_ENABLE_CRC_DRIVER)
  // 1.3. check hw alg
  hal_assert(checksum_crc32_ethernet_hw_calculate(test_str, size) ==
             CHECKSUM_CRC32_ETHERNET_REF_STRING_CHECK_VALUE);

  crc = checksum_crc32_ethernet_hw_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc32_ethernet_hw_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC32_ETHERNET_XOROUT_VALUE) ==
             CHECKSUM_CRC32_ETHERNET_REF_STRING_CHECK_VALUE);
#endif

  // 2. check crc16 x25
  // 2.1. check fast alg
  hal_assert(checksum_crc16_x25_fast_calculate(test_str, size) ==
             CHECKSUM_CRC16_X25_REF_STRING_CHECK_VALUE);

  crc = checksum_crc16_x25_fast_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc16_x25_fast_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC16_X25_XOROUT_VALUE) == CHECKSUM_CRC16_X25_REF_STRING_CHECK_VALUE);

#if (FEATURE_ENABLE_CRC_DRIVER)
  // 2.2. check hw alg
  hal_assert(checksum_crc16_x25_hw_calculate(test_str, size) ==
             CHECKSUM_CRC16_X25_REF_STRING_CHECK_VALUE);

  crc = checksum_crc16_x25_hw_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc16_x25_hw_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC16_X25_XOROUT_VALUE) == CHECKSUM_CRC16_X25_REF_STRING_CHECK_VALUE);
#endif

  // 3. check crc16 x25
  // 3.1. check slow alg
  hal_assert(checksum_crc16_ibm_fast_calculate(test_str, size) ==
             CHECKSUM_CRC16_IBM_REF_STRING_CHECK_VALUE);

  crc = checksum_crc16_ibm_slow_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc16_ibm_slow_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC16_IBM_XOROUT_VALUE) == CHECKSUM_CRC16_IBM_REF_STRING_CHECK_VALUE);

  // 3.2. check fast alg
  hal_assert(checksum_crc16_ibm_fast_calculate(test_str, size) ==
             CHECKSUM_CRC16_IBM_REF_STRING_CHECK_VALUE);

  crc = checksum_crc16_ibm_fast_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc16_ibm_fast_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC16_IBM_XOROUT_VALUE) == CHECKSUM_CRC16_IBM_REF_STRING_CHECK_VALUE);

#if (FEATURE_ENABLE_CRC_DRIVER)
  // 3.3. check hw alg
  hal_assert(checksum_crc16_ibm_hw_calculate(test_str, size) ==
             CHECKSUM_CRC16_IBM_REF_STRING_CHECK_VALUE);

  crc = checksum_crc16_ibm_hw_reset_accumulator();
  for (uint32_t i = 0; i < size; i++)
  {
    crc = checksum_crc16_ibm_hw_accumulate(crc, test_str[i]);
  }
  hal_assert((crc ^ CHECKSUM_CRC16_IBM_XOROUT_VALUE) == CHECKSUM_CRC16_IBM_REF_STRING_CHECK_VALUE);
#endif

  HAL_UNUSED(crc);
}
