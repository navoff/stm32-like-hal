#pragma once

/**
 * @file checksum.h
 * @brief Checksum (CRC) calculation functions.
 *
 * About the CRC-16-IBM algorithm (crc16_ibm)
 *     Polynomial: x^16 + x^15 + x^2 + 1
 *                 0x8005
 *     Initial value: 0xFFFF
 *     RefIn: true
 *     RefOut: true
 *     XorOut: 0
 *     Check value for the string "123456789": 0x4B37
 *     https://crccalc.com/?method=CRC-16/MODBUS
 *
 * About the CRC-16-X25 algorithm (crc16_x25)
 *     Polynomial: x^16 + x^12 + x^5 + 1
 *                 0x1021
 *     Initial value: 0xFFFF
 *     RefIn: true
 *     RefOut: true
 *     XorOut: 0xFFFF
 *     Check value for the string "123456789": 0x906E (0x6F91 ^ 0xFFFF)
 *     https://crccalc.com/?method=CRC-16/X-25
 *
 * About the CRC32 algorithm (crc32_ethernet)
 *     Polynomial: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 +
 *                 x^2 + x + 1
 *                 0x04C11DB7
 *     Initial value: 0xFFFFFFFF
 *     RefIn: true
 *     RefOut: true
 *     XorOut: 0
 *     Check value for the string "123456789": 0x340BC6D9
 *     https://crccalc.com/?method=CRC-32/JAMCRC
 */

#include <stdint.h>

#define CHECKSUM_CRC_ALG_SLOW 1
#define CHECKSUM_CRC_ALG_FAST 2
#define CHECKSUM_CRC_ALG_HW 3

/// Polynomial for CRC-16-IBM checksum calculation
#define CHECKSUM_CRC16_IBM_POLYNOM 0x8005

/// Initial value for CRC-16-IBM checksum calculation
#define CHECKSUM_CRC16_IBM_INIT_VALUE 0xFFFF

/// Value for the "final" XOR of the calculated CRC-16-IBM checksum
#define CHECKSUM_CRC16_IBM_XOROUT_VALUE 0

/// Check value,
/// which the CRC must equal after applying the algorithm to the whole buffer
#define CHECKSUM_CRC16_IBM_CHECK_VALUE 0

/// Check value for CRC-16-IBM and the string "123456789" (9 bytes)
#define CHECKSUM_CRC16_IBM_REF_STRING_CHECK_VALUE 0x4B37

/// Polynomial for CRC-16-X25 checksum calculation
#define CHECKSUM_CRC16_X25_POLYNOM 0x1021

/// Initial value for CRC-16-X25 checksum calculation
#define CHECKSUM_CRC16_X25_INIT_VALUE 0xFFFF

/// Value for the "final" XOR of the calculated CRC-16-X25 checksum
#define CHECKSUM_CRC16_X25_XOROUT_VALUE 0xFFFF

/// Check value,
/// which the CRC must equal after applying the algorithm to the whole buffer
#define CHECKSUM_CRC16_X25_CHECK_VALUE 0x0F47

/// Check value for CRC-16-X25 and the string "123456789" (9 bytes)
#define CHECKSUM_CRC16_X25_REF_STRING_CHECK_VALUE 0x906E

/// Polynomial for CRC-32-Ethernet checksum calculation
#define CHECKSUM_CRC32_ETHERNET_POLYNOM 0x04C11DB7

/// Initial value for CRC-32-Ethernet checksum calculation
#define CHECKSUM_CRC32_ETHERNET_INIT_VALUE 0xFFFFFFFF

/// Value for the "final" XOR of the calculated CRC-32-Ethernet checksum
#define CHECKSUM_CRC32_ETHERNET_XOROUT_VALUE 0

/// Check value for CRC-32-Ethernet and the string "123456789" (9 bytes)
#define CHECKSUM_CRC32_ETHERNET_REF_STRING_CHECK_VALUE 0x340BC6D9

/**
 * Group of functions for resetting the internal accumulator for CRC-16-IBM calculation
 * - using a lookup table
 * - using the hardware unit
 *
 * @return Accumulator value
 */
uint16_t checksum_crc16_ibm_slow_reset_accumulator(void);
uint16_t checksum_crc16_ibm_fast_reset_accumulator(void);
uint16_t checksum_crc16_ibm_hw_reset_accumulator(void);

/**
 * Group of functions for updating the CRC-16-IBM checksum value
 * using the value from the previous checksum calculation step.
 * - without a lookup table
 * - using a lookup table
 * - using the hardware unit
 *
 * @param accumulator - value from the previous checksum calculation step (accumulator)
 * @param data - new data
 * @return New calculated checksum value
 */
uint16_t checksum_crc16_ibm_slow_accumulate(uint16_t accumulator, uint8_t data);
uint16_t checksum_crc16_ibm_fast_accumulate(uint16_t accumulator, uint8_t data);
uint16_t checksum_crc16_ibm_hw_accumulate(uint16_t accumulator, uint8_t data);

/**
 * Group of functions for calculating CRC-16-IBM:
 * - without a lookup table
 * - using a lookup table
 * - using the hardware unit
 *
 * @param buf - pointer to the data buffer
 * @param size - buffer size in bytes
 * @return Calculated checksum
 */
uint16_t checksum_crc16_ibm_slow_calculate(void *buf, uint32_t size);
uint16_t checksum_crc16_ibm_fast_calculate(void *buf, uint32_t size);
uint16_t checksum_crc16_ibm_hw_calculate(void *buf, uint32_t size);

/**
 * Group of functions for resetting the internal accumulator for CRC-16-X25 calculation
 * - using a lookup table
 * - using the hardware unit
 *
 * @return Accumulator value
 */
uint16_t checksum_crc16_x25_fast_reset_accumulator(void);
uint16_t checksum_crc16_x25_hw_reset_accumulator(void);

/**
 * Group of functions for updating the CRC-16-X25 checksum value
 * using the value from the previous checksum calculation step.
 * - using a lookup table
 * - using the hardware unit
 *
 * @param accumulator - checksum value from the previous step (accumulator)
 * @param data - new data
 * @return New calculated checksum value
 */
uint16_t checksum_crc16_x25_fast_accumulate(uint16_t accumulator, uint8_t data);
uint16_t checksum_crc16_x25_hw_accumulate(uint16_t accumulator, uint8_t data);

/**
 * Group of functions for calculating CRC-16-X25:
 * - without a lookup table
 * - using a lookup table
 * - using the hardware unit
 *
 * @param buf - pointer to the data buffer
 * @param size - buffer size in bytes
 * @return Calculated checksum
 */
uint16_t checksum_crc16_x25_fast_calculate(void *buf, uint32_t size);
uint16_t checksum_crc16_x25_hw_calculate(void *buf, uint32_t size);

/**
 * Group of functions for resetting the internal accumulator for CRC-32-Ethernet calculation
 * - without a lookup table
 * - using a lookup table
 * - using the hardware unit
 *
 * @return Accumulator value
 */
uint32_t checksum_crc32_ethernet_slow_reset_accumulator(void);
uint32_t checksum_crc32_ethernet_fast_reset_accumulator(void);
uint32_t checksum_crc32_ethernet_hw_reset_accumulator(void);

/**
 * Group of functions for updating the CRC-32-Ethernet checksum value
 * using the value from the previous checksum calculation step.
 * - without a lookup table
 * - using a lookup table
 * - using the hardware unit
 *
 * @param accumulator - checksum value from the previous step (accumulator)
 * @param data - new data
 * @return New calculated checksum value
 */
uint32_t checksum_crc32_ethernet_slow_accumulate(uint32_t accumulator, uint8_t data);
uint32_t checksum_crc32_ethernet_fast_accumulate(uint32_t accumulator, uint8_t data);
uint32_t checksum_crc32_ethernet_hw_accumulate(uint32_t accumulator, uint8_t data);

/**
 * Group of functions for calculating CRC-32-Ethernet:
 * - without a lookup table
 * - using a lookup table
 * - using the hardware unit
 *
 * @param buf - pointer to the data buffer
 * @param size - buffer size in bytes
 * @return Calculated checksum
 */
uint32_t checksum_crc32_ethernet_slow_calculate(void *buf, uint32_t size);
uint32_t checksum_crc32_ethernet_fast_calculate(void *buf, uint32_t size);
uint32_t checksum_crc32_ethernet_hw_calculate(void *buf, uint32_t size);

uint32_t checksum_hw_accumulate_test(uint32_t crc, uint8_t data, uint8_t is_first);
