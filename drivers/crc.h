#pragma once

/**
 * @file crc.h
 * @brief Hardware CRC calculation (GD32L23x).
 */

#include <stdint.h>

typedef enum
{
  CRC_ALG__UNDEFINED = 0,
  CRC_ALG__CRC16_IBM,
  CRC_ALG__CRC16_X25,
  CRC_ALG__CRC32_ETHERNET,
} crc_alg_t;

void crc_hw_set_alg(crc_alg_t alg);
uint32_t crc_hw_reset_accumulator(crc_alg_t alg);
uint32_t crc_hw_accumulate(crc_alg_t alg, uint32_t accumulator, uint8_t data);
uint32_t crc_hw_calculate(crc_alg_t alg, void *buf, uint32_t size);
