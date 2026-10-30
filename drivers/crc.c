/**
 * @file crc.c
 * @brief Hardware CRC calculation (GD32L23x).
 */

#include "config.h"

#include "drivers/crc.h"
#include "utils/checksum.h"
#include "utils/macros.h"

#if !defined(GD32L23X)
  #error This MCU not supported
#endif

struct
{
  crc_alg_t current_alg;
  uint32_t init_value;
  uint32_t xor_out_value;
} crc = {
    .current_alg = CRC_ALG__UNDEFINED,
};

void crc_hw_set_alg(crc_alg_t alg)
{
  if (crc.current_alg == alg)
  {
    return;
  }

  switch (alg)
  {
    case CRC_ALG__CRC16_IBM:
      crc.init_value = CHECKSUM_CRC16_IBM_INIT_VALUE;
      crc.xor_out_value = CHECKSUM_CRC16_IBM_XOROUT_VALUE;

      rcu_periph_clock_enable(RCU_CRC);

      crc_polynomial_size_set(CRC_CTL_PS_16);
      crc_polynomial_set(CHECKSUM_CRC16_IBM_POLYNOM);
      crc_input_data_reverse_config(CRC_INPUT_DATA_BYTE);
      crc_reverse_output_data_enable();

      break;

    case CRC_ALG__CRC16_X25:
      crc.init_value = CHECKSUM_CRC16_X25_INIT_VALUE;
      crc.xor_out_value = CHECKSUM_CRC16_X25_XOROUT_VALUE;

      rcu_periph_clock_enable(RCU_CRC);

      crc_polynomial_size_set(CRC_CTL_PS_16);
      crc_polynomial_set(CHECKSUM_CRC16_X25_POLYNOM);
      crc_input_data_reverse_config(CRC_INPUT_DATA_BYTE);
      crc_reverse_output_data_enable();

      break;

    case CRC_ALG__CRC32_ETHERNET:
      crc.init_value = CHECKSUM_CRC32_ETHERNET_INIT_VALUE;
      crc.xor_out_value = CHECKSUM_CRC32_ETHERNET_XOROUT_VALUE;

      rcu_periph_clock_enable(RCU_CRC);

      crc_polynomial_size_set(CRC_CTL_PS_32);
      crc_polynomial_set(CHECKSUM_CRC32_ETHERNET_POLYNOM);
      crc_input_data_reverse_config(CRC_INPUT_DATA_BYTE);
      crc_reverse_output_data_enable();

      break;

    default:
      break;
  }

  crc.current_alg = alg;
}

uint32_t crc_hw_reset_accumulator(crc_alg_t alg)
{
  crc_hw_set_alg(alg);
  crc_init_data_register_write(crc.init_value);
  return crc.init_value;
}

uint32_t crc_hw_accumulate(crc_alg_t alg, uint32_t accumulator, uint8_t data)
{
  HAL_UNUSED(alg);
  HAL_UNUSED(accumulator);

  return crc_single_data_calculate(data, INPUT_FORMAT_BYTE);
}

uint32_t crc_hw_calculate(crc_alg_t alg, void *buf, uint32_t size)
{
  crc_hw_set_alg(alg);
  crc_init_data_register_write(crc.init_value);
  return crc_block_data_calculate(buf, size, INPUT_FORMAT_BYTE) ^ crc.xor_out_value;
}
