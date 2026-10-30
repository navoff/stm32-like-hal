/**
 * @file utils.c
 * @brief Miscellaneous string, hex, PDU and numeric helper functions.
 */

#include "config.h"

#include "utils/assert.h"
#include "utils/macros.h"
#include "utils/utils.h"
#include <stdlib.h>

static const int32_t test_array1[1] __attribute__((unused)) = {8};
static const int32_t test_array2[2] __attribute__((unused)) = {6, 2};
static const int32_t test_array3[7] __attribute__((unused)) = {128, 127, 126, 126, 122, 32, 16};
static const int32_t test_array4[5] __attribute__((unused)) = {100000, 90000, 10000, -1000, -50000};

void utils_unit_test(void)
{
#if (HAL_ASSERT_ENABLE == 1)
  {
    uint8_t pdu_phone[32];
    uint16_t pdu_len;
    {
      // even number of digits, everything fits
      memset(pdu_phone, 0, 32);
      pdu_len = utils_convert_phone_to_pdu(pdu_phone, 32, "7987123456");
      uint8_t pdu_phone_stamp[32] = {0x97, 0x78, 0x21, 0x43, 0x65};
      hal_assert(pdu_len == 5);
      hal_assert(memcmp(pdu_phone, pdu_phone_stamp, pdu_len) == 0);
    }

    {
      // odd number of digits, everything fits
      memset(pdu_phone, 0, 32);
      pdu_len = utils_convert_phone_to_pdu(pdu_phone, 32, "79871234567");
      uint8_t pdu_phone_stamp[32] = {0x97, 0x78, 0x21, 0x43, 0x65, 0xF7};
      hal_assert(pdu_len == 6);
      hal_assert(memcmp(pdu_phone, pdu_phone_stamp, pdu_len) == 0);
    }

    {
      // even number of digits, not everything fits
      memset(pdu_phone, 0, 32);
      pdu_len = utils_convert_phone_to_pdu(pdu_phone, 4, "7987123456");
      uint8_t pdu_phone_stamp[32] = {0x97, 0x78, 0x21, 0x43};
      hal_assert(pdu_len == 4);
      hal_assert(memcmp(pdu_phone, pdu_phone_stamp, pdu_len) == 0);
    }
  }

  {
    char phone[32];
    uint16_t phone_len;

    {
      // even number of digits, everything fits
      memset(phone, 0, 32);
      uint8_t pdu_phone[32] = {0x97, 0x78, 0x21, 0x43, 0x65};
      phone_len = utils_convert_pdu_to_phone(phone, 32, pdu_phone, 5);
      hal_assert(phone_len == 10);
      hal_assert(memcmp(phone, "7987123456", phone_len) == 0);
    }

    {
      // odd number of digits, everything fits
      memset(phone, 0, 32);
      uint8_t pdu_phone[32] = {0x97, 0x78, 0x21, 0x43, 0x65, 0xF7};
      phone_len = utils_convert_pdu_to_phone(phone, 32, pdu_phone, 6);
      hal_assert(phone_len == 11);
      hal_assert(memcmp(phone, "79871234567", phone_len) == 0);
    }

    {
      // even number of digits, not everything fits
      memset(phone, 0, 32);
      uint8_t pdu_phone[32] = {0x97, 0x78, 0x21, 0x43, 0x65, 0xF7};
      phone_len = utils_convert_pdu_to_phone(phone, 10, pdu_phone, 6);
      hal_assert(phone_len == 10);
      hal_assert(memcmp(phone, "7987123456", phone_len) == 0);
    }

    {
      // odd number of digits, not everything fits
      memset(phone, 0, 32);
      uint8_t pdu_phone[32] = {0x97, 0x78, 0x21, 0x43, 0x65, 0xF7};
      phone_len = utils_convert_pdu_to_phone(phone, 9, pdu_phone, 6);
      hal_assert(phone_len == 9);
      hal_assert(memcmp(phone, "798712345", phone_len) == 0);
    }
  }
#endif
}

/**
 * @brief Function for checking the size of a null-terminated string.
 * @param buf - pointer to the null-terminated string to check
 * @param max_size - maximum allowed string size
 * @return If the string length is less than max_size, returns the number of characters
 *          before '\0', not including it. Otherwise returns max_size.
 */
size_t utils_strnlen(char *buf, size_t max_size)
{
  size_t length = strlen(buf);

  if (length > max_size)
  {
    length = max_size;
  }

  return length;
}

/**
 * @brief Remove non-digit characters from a string
 * @param str - string with text
 * @param size - original string size
 * @return New size of the string
 */
size_t utils_remove_not_digit_sym(char *str, size_t size)
{
  size_t res_size = 0;

  for (size_t i = 0; i < size; i++)
  {
    if (UTILS_SYM_IS_DEC_DIGIT(str[i]) || (str[i] == 0))
    {
      str[res_size++] = str[i];
    }
  }

  return res_size;
}

/**
 * @brief Determine whether a string contains only ASCII characters
 * @param str - string in a single-byte encoding
 * @param max_size - string length limit
 * @return true - the string contains only ASCII characters
 */
bool utils_is_contains_only_ascii(char *str, uint16_t max_size)
{
  while ((*str != 0) && (max_size != 0))
  {
    if (*(uint8_t *)str++ > 0x7f)
    {
      return false;
    }

    max_size--;
  }

  return true;
}

size_t utils_memcpy_only_printed_symbols(char *to, const char *from, size_t len)
{
  for (size_t i = 0; i < len; i++)
  {
    if (!UTILS_SYM_IS_PRINTED(from[i]))
    {
      return i;
    }
    to[i] = from[i];
  }

  return len;
}

/**
 * @brief Convert a single-byte number to a string of hexadecimal digits
 * @param byte - number
 * @param str - string of hexadecimal digits
 * @note Example: the number 143 (0x8F) is converted to the string "8F"
 */
void utils_convert_byte_to_hex_str(uint8_t byte, char *str)
{
  str[0] = UTILS_CONVERT_HALFBYTE_TO_HEX_DIGIT(byte >> 4);
  str[1] = UTILS_CONVERT_HALFBYTE_TO_HEX_DIGIT(byte & 0x0F);
}

/**
 * @brief Convert a hexadecimal digit character to a single-byte number
 * @param c - hexadecimal character ('0', '1', ... 'F', ... 'f')
 * @return number
 */
uint8_t utils_convert_hex_digit_to_halfbyte(char c)
{
  if ((c >= '0') && (c <= '9'))
  {
    return c - '0';
  }
  else if ((c >= 'a') && (c <= 'f'))
  {
    return c - 'a' + 10;
  }
  else if ((c >= 'A') && (c <= 'F'))
  {
    return c - 'A' + 10;
  }

  return 0;
}

/**
 * @brief Convert a string of 2 hexadecimal digits to a single-byte number
 * @param str - string of hexadecimal digits
 * @return number
 * @note Example: the string "8F" is converted to the number 143 (0x8F)
 */
uint8_t utils_convert_hex_str_to_byte(char *str)
{
  uint8_t res = (utils_convert_hex_digit_to_halfbyte(str[0]) << 4) |
                (utils_convert_hex_digit_to_halfbyte(str[1]) << 0);
  return res;
}

/**
 * @brief Convert text of hexadecimal digits to an array of single-byte numbers
 * @param buf - output array of numbers
 * @param buf_size - maximum size of the output array
 * @param text - text of hexadecimal digits.
 *        Must contain an even number of digits, otherwise the last number is not converted.
 * @return Number of converted numbers
 * @note Example: the string "8F5a" is converted to the array { 143, 90 }
 */
uint16_t utils_convert_hex_text_to_buf(uint8_t *buf, uint16_t buf_size, char *text)
{
  uint16_t buf_idx = 0;

  while (true)
  {
    if ((text[0] == '\0') || (text[1] == '\0'))
    {
      break;
    }

    if (buf_idx >= buf_size)
    {
      break;
    }

    buf[buf_idx] = utils_convert_hex_str_to_byte(text);

    text += 2;
    buf_idx += 1;
  }

  return buf_idx;
}

/**
 * @brief Convert an array to a string of hexadecimal digits
 * @param text - output string of hexadecimal digits
 * @param text_size - maximum string size
 * @param buf - source array
 * @param buf_size - array size
 * @return Size of the resulting string
 */
uint16_t utils_convert_buf_to_hex_text(char *text, uint16_t text_size, uint8_t *buf,
                                       uint16_t buf_size)
{
  uint16_t idx = 0;

  for (; idx < buf_size; idx++)
  {
    if ((idx + 2) > text_size)
    {
      break;
    }

    utils_convert_byte_to_hex_str(buf[idx], text);
    text += 2;
  }

  return idx;
}

/**
 * @brief Re-encode a string from 7-bit encoding to 8-bit.
 * @param [out] str_8_bit - string in 8-bit encoding (NOT null-terminated).
 * @param [in] str_8_bit_len - maximum size of the output string.
 * @param [in] str_7_bit - string in 7-bit encoding.
 * @param [in] extra_shifting - whether an extra shift is required.
 * @return Length of the resulting re-encoded string.
 * @note The input string contains not the codes themselves but their textual
 * representation, i.e. the code 0x3b is passed as the string "3B".
 * The shift is used in concatenated mode.
 */
uint16_t utils_convert_text_7bit_to_8bit(char *str_8_bit, uint16_t str_8_bit_len, char *str_7_bit,
                                         bool extra_shifting)
{
  if ((*str_7_bit == 0) || (*(str_7_bit + 1) == 0) || (str_8_bit_len == 0))
  {
    return 0;
  }

  uint16_t res = 0;
  uint16_t idx = 0;
  uint8_t prev_byte_mask = 0x00;
  uint8_t current_byte_mask = 0x7F;
  uint8_t current_char = utils_convert_hex_str_to_byte(str_7_bit);
  uint8_t prev_char = 0;

  if (extra_shifting)
  {
    str_8_bit[res++] = current_char >> 1;
    str_7_bit += 2;
    current_char = utils_convert_hex_str_to_byte(str_7_bit);
    if (res >= str_8_bit_len)
    {
      return res;
    }
  }

  char temp;
  while (true)
  {
    temp =
        ((current_char & current_byte_mask) << idx) | ((prev_char & prev_byte_mask) >> (8 - idx));

    str_8_bit[res++] = temp;
    if (res >= str_8_bit_len)
    {
      return res;
    }

    idx++;
    str_7_bit += 2;

    if (idx == 7)
    {
      prev_byte_mask = 0x00;
      current_byte_mask = 0x7f;
      idx = 0;
      prev_char = 0;
      str_8_bit[res++] = current_char >> 1;
      if (res >= str_8_bit_len)
      {
        return res;
      }
    }
    else
    {
      current_byte_mask >>= 1;
      prev_byte_mask = (prev_byte_mask >> 1) | 0x80;
      prev_char = current_char;
    }

    if ((*str_7_bit == 0) || (*(str_7_bit + 1) == 0))
    {
      break;
    }
    current_char = utils_convert_hex_str_to_byte(str_7_bit);
  }

  return res;
}

/**
 * @brief Re-encode a string from 8-bit encoding to 7-bit
 * @param str_7_bit - string in 7-bit encoding
 * @param str_8_bit - string in 8-bit encoding
 * @param extra_shifting - whether an extra shift is required
 * @return Length of the resulting re-encoded 7-bit string
 * @note The output string contains not the codes themselves but their textual
 * representation, i.e. the code 0x3B is passed as the string "3B".
 * The shift is used in concatenated mode.
 */
uint16_t utils_convert_text_8bit_to_7bit(char *str_7_bit, char *str_8_bit, bool extra_shifting)
{
  uint16_t str_8_bit_idx = 0;
  uint16_t str_7_bit_idx = 0;
  uint8_t buf_char;

  if (extra_shifting)
  {
    buf_char = (str_8_bit[0] & 0x7f) << 1;
    utils_convert_byte_to_hex_str(buf_char, &str_7_bit[0]);
    str_8_bit_idx++;
    str_7_bit_idx++;
  }

  uint16_t idx = 0;
  uint8_t current_byte_mask = 0x7f;
  uint8_t next_byte_mask = 0x01;
  while (str_8_bit[str_8_bit_idx] != 0)
  {
    buf_char = ((str_8_bit[str_8_bit_idx] & current_byte_mask) >> idx) |
               ((str_8_bit[str_8_bit_idx + 1] & next_byte_mask) << (7 - idx));

    utils_convert_byte_to_hex_str(buf_char, &str_7_bit[str_7_bit_idx * 2]);

    idx++;
    str_7_bit_idx++;
    str_8_bit_idx++;
    if (idx == 7)
    {
      if (str_8_bit[str_8_bit_idx] == 0)
      {
        break;
      }

      idx = 0;
      current_byte_mask = 0x7f;
      next_byte_mask = 0x01;

      str_8_bit_idx++;
    }
    else
    {
      next_byte_mask = (next_byte_mask << 1) | 0x01;
      current_byte_mask = (current_byte_mask << 1) & 0x7f;
    }
  }

  str_7_bit[str_7_bit_idx * 2] = 0;

  return str_7_bit_idx * 2;
}

/**
 * @brief Convert UTC date/time to UNIX time
 * @param years - UTC year
 * @param month - UTC month number
 * @param day - UTC day of month
 * @param hour - UTC hours
 * @param minute - UTC minutes
 * @param second - UTC seconds
 * @return Timestamp, sec
 */
time_t utils_convert_utc_time_to_unix(uint16_t years, uint8_t month, uint8_t day, uint8_t hour,
                                      uint8_t minute, uint8_t second)
{
  struct tm time_info;

  time_info.tm_year = years - 1900; // year 1900 is taken as 0
  time_info.tm_mon = month - 1;
  time_info.tm_mday = day;
  time_info.tm_hour = hour;
  time_info.tm_min = minute;
  time_info.tm_sec = second;
  time_info.tm_isdst = 0;

  return mktime(&time_info);
}

/**
 * @brief Convert a phone number from an ASCII string to a PDU-format byte array
 * @param pdu_phone - PDU-format byte array
 * @param pdu_phone_max_size - maximum buffer size
 * @param phone - phone number string
 * @return Number of bytes added to the array
 * @note In PDU format the digits of the number are swapped in pairs,
 *       i.e. the number 79161239081 is represented by the array 0x97 0x61 0x21 0x93 0x80 0xF1
 */
uint16_t utils_convert_phone_to_pdu(uint8_t *pdu_phone, uint16_t pdu_phone_max_size, char *phone)
{
  if (pdu_phone_max_size == 0)
  {
    return 0;
  }

  uint16_t phone_idx = 0;
  uint16_t pdu_phone_idx = 0;

  while (phone[phone_idx] != 0)
  {
    uint8_t data = utils_convert_hex_digit_to_halfbyte(phone[phone_idx]);

    if (phone_idx & 0x01)
    {
      pdu_phone[pdu_phone_idx] |= (data << 4);

      pdu_phone_idx++;

      if (pdu_phone_idx >= pdu_phone_max_size)
      {
        return pdu_phone_idx;
      }
    }
    else
    {
      pdu_phone[pdu_phone_idx] = data;
    }
    phone_idx++;
  }

  if (phone_idx & 0x01)
  {
    pdu_phone[pdu_phone_idx] |= 0xF0;
    pdu_phone_idx++;
  }

  return pdu_phone_idx;
}

/**
 * @brief Convert a phone number from a PDU-format byte array to an ASCII string
 * @param phone - phone number string
 * @param phone_max_size - maximum string size
 * @param pdu_phone - PDU-format byte array
 * @param pdu_phone_size - array size
 * @return Number of characters added to the phone number string
 * @note In PDU format the digits of the number are swapped in pairs,
 *       i.e. the number 79161239081 is represented by the array 0x97 0x61 0x21 0x93 0x80 0xF1
 */
uint16_t utils_convert_pdu_to_phone(char *phone, uint16_t phone_max_size, uint8_t *pdu_phone,
                                    uint16_t pdu_phone_size)
{
  if (phone_max_size == 0)
  {
    return 0;
  }

  uint16_t phone_idx = 0;

  for (uint16_t i = 0; i < pdu_phone_size; i++)
  {
    uint8_t data = pdu_phone[i] & 0x0F;
    phone[phone_idx] = UTILS_CONVERT_HALFBYTE_TO_HEX_DIGIT(data);
    phone_idx++;
    if (phone_idx >= phone_max_size)
    {
      return phone_idx;
    }

    data = pdu_phone[i] >> 4;
    if (data == 0x0F)
    {
      return phone_idx;
    }

    phone[phone_idx] = UTILS_CONVERT_HALFBYTE_TO_HEX_DIGIT(data);
    phone_idx++;
    if (phone_idx >= phone_max_size)
    {
      return phone_idx;
    }
  }

  return phone_idx;
}

/**
 * @brief Check that a number is a power of 2
 * @param number - number
 * @return ERR_OK - it is, otherwise - it is not
 */
error_t utils_check_pow_2(uint32_t number)
{
  if (number <= 0x01)
  {
    return ERR_RESULT;
  }

  while (number != 0x01)
  {
    if ((number & 0x01) != 0)
    {
      return ERR_RESULT;
    }

    number >>= 1;
  }

  return ERR_OK;
}

/**
 * @brief Copy the contents of the source buffer to the second one
 *        with the byte order reversed.
 * @param out - output array
 * @param out_size - maximum size of the output array
 * @param in - source array
 * @param size - size of the source array
 * @return Number of copied bytes
 */
int utils_reverse_memcpy(void *out, int out_size, void *in, int size)
{
  if (out_size < size)
  {
    return 0;
  }

  for (int i = 0; i < size; i++)
  {
    ((uint8_t *)out)[size - i - 1] = ((uint8_t *)in)[i];
  }

  return size;
}

/**
 * @brief Find, in an array of numbers sorted in descending order,
 * the index of the element whose value is closest to the target
 * @param val - target value
 * @param search_array - array to search in
 * @param array_size - array size
 * @return index of the element within the array
 */
uint16_t utils_find_closest_value_index(int32_t val, const int32_t *search_array,
                                        uint16_t array_size)
{
  if (search_array == NULL || array_size == 0)
  {
    // Return an invalid index if the array is empty or the pointer is invalid
    return UINT16_MAX;
  }

  uint16_t i = array_size / 2; // Jump to the middle of the array
  uint16_t interval = i; // The first search interval is half the array
  uint16_t prev_i;
  bool was_zero = false;

  while (1)
  {
    prev_i = i;

    // Halve the interval each time,
    // but not below 1
    interval /= 2;
    if (interval == 0)
    {
      interval = 1;
      // If the remainder of division by 2 == 0
      // we have definitely approached the closest value
      was_zero = true;
    }

    // Is the array value greater than the target number?
    // yes - jump up, no - jump down
    // (for an ascending array it would be the other way round)
    if (search_array[i] > val)
    {
      i += interval;
    }
    else
    {
      i -= interval;
    }

    // If we jumped above (or below) the array bounds,
    // return to the maximum possible index
    if (i >= array_size)
    {
      i = array_size - 1;
    }

    // If the interval size == 1, we are in the region of the closest value,
    // i.e. no further than two steps away
    if (interval == 1 && was_zero)
    {
      // compute 2 deltas - between the target and the previous iteration value
      // and between the target and the current iteration value
      int32_t prev_distance = abs(search_array[prev_i] - val);
      int32_t current_distance = abs(search_array[i] - val);
      if (current_distance >= prev_distance)
      {
        // As soon as the delta starts growing or stays the same,
        // it means we are moving away from the closest value and
        // the previous value is the closest to the target
        break;
      }
    }
  }

  return prev_i;
}

void utils_find_closest_value_test(void)
{
  hal_assert(0 == utils_find_closest_value_index(12, test_array1, ARRAY_SIZE(test_array1)));
  hal_assert(0 == utils_find_closest_value_index(4, test_array1, ARRAY_SIZE(test_array1)));
  hal_assert(1 == utils_find_closest_value_index(2, test_array2, ARRAY_SIZE(test_array2)));
  hal_assert(1 == utils_find_closest_value_index(1, test_array2, ARRAY_SIZE(test_array2)));
  hal_assert(0 == utils_find_closest_value_index(5, test_array2, ARRAY_SIZE(test_array2)));
  hal_assert(0 == utils_find_closest_value_index(8, test_array2, ARRAY_SIZE(test_array2)));
  hal_assert(0 == utils_find_closest_value_index(130, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(0 == utils_find_closest_value_index(128, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(1 == utils_find_closest_value_index(127, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(2 == utils_find_closest_value_index(126, test_array3, ARRAY_SIZE(test_array3)) ||
             3 == utils_find_closest_value_index(126, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(3 == utils_find_closest_value_index(124, test_array3, ARRAY_SIZE(test_array3)) ||
             4 == utils_find_closest_value_index(124, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(4 == utils_find_closest_value_index(120, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(6 == utils_find_closest_value_index(17, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(6 == utils_find_closest_value_index(8, test_array3, ARRAY_SIZE(test_array3)));
  hal_assert(0 == utils_find_closest_value_index(110000, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(0 == utils_find_closest_value_index(100000, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(1 == utils_find_closest_value_index(92000, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(3 == utils_find_closest_value_index(500, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(3 == utils_find_closest_value_index(-1000, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(3 == utils_find_closest_value_index(-2000, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(4 == utils_find_closest_value_index(-45000, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(4 == utils_find_closest_value_index(-50000, test_array4, ARRAY_SIZE(test_array4)));
  hal_assert(4 == utils_find_closest_value_index(-55000, test_array4, ARRAY_SIZE(test_array4)));
}
