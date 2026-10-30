#pragma once

/**
 * @file utils.h
 * @brief Miscellaneous string, hex, PDU and numeric helper functions.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "utils/errors.h"

/// Whether the character is a decimal digit
#define UTILS_SYM_IS_DEC_DIGIT(sym) (((sym) >= '0') && ((sym) <= '9'))

/// Whether the character is a hexadecimal digit
#define UTILS_SYM_IS_HEX_DIGIT(sym)                                                                \
  ((((sym) >= '0') && ((sym) <= '9')) || (((sym) >= 'A') && ((sym) <= 'F')) ||                     \
   (((sym) >= 'a') && ((sym) <= 'f')))

/// Whether the character is printable
#define UTILS_SYM_IS_PRINTED(sym) (((sym) >= 0x20) && ((sym) <= 0xFE))

/// Convert a number (0 to 15) to a hexadecimal digit character
#define UTILS_CONVERT_HALFBYTE_TO_HEX_DIGIT(b) (((b) <= 9) ? ((b) + '0') : ((b) - 10 + 'A'))

#define UTILS_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define UTILS_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define UTILS_SWAP_BYTES_UINT16(ptr)                                                               \
  (*(uint16_t *)ptr = (*(uint16_t *)ptr << 8) + (*(uint16_t *)ptr >> 8))

void utils_unit_test(void);

size_t utils_strnlen(char *buf, size_t max_size);
size_t utils_remove_not_digit_sym(char *str, size_t size);
bool utils_is_contains_only_ascii(char *str, uint16_t max_size);
size_t utils_memcpy_only_printed_symbols(char *str_to, const char *str_from, size_t str_from_len);

void utils_convert_byte_to_hex_str(uint8_t byte, char *str);
uint8_t utils_convert_hex_digit_to_halfbyte(char c);
uint8_t utils_convert_hex_str_to_byte(char *str);
uint16_t utils_convert_hex_text_to_buf(uint8_t *buf, uint16_t buf_size, char *text);
uint16_t utils_convert_buf_to_hex_text(char *text, uint16_t text_size, uint8_t *buf,
                                       uint16_t buf_size);

uint16_t utils_convert_text_7bit_to_8bit(char *str_8_bit, uint16_t str_8_bit_len, char *str_7_bit,
                                         bool extra_shifting);
uint16_t utils_convert_text_8bit_to_7bit(char *str_7_bit, char *str_8_bit, bool extra_shifting);

time_t utils_convert_utc_time_to_unix(uint16_t years, uint8_t month, uint8_t day, uint8_t hour,
                                      uint8_t minute, uint8_t second);

uint16_t utils_convert_phone_to_pdu(uint8_t *pdu_phone, uint16_t pdu_phone_max_size, char *phone);
uint16_t utils_convert_pdu_to_phone(char *phone, uint16_t phone_max_size, uint8_t *pdu_phone,
                                    uint16_t pdu_phone_size);

error_t utils_check_pow_2(uint32_t number);

int utils_reverse_memcpy(void *out, int out_size, void *in, int size);

uint16_t utils_find_closest_value_index(int32_t val, const int32_t *search_array,
                                        uint16_t array_size);
void utils_find_closest_value_test(void);
