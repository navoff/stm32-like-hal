/**
 * @file nmea_core.c
 * @brief NMEA sentence parser implementation.
 */

#include "config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drivers/tick_timer.h"
#include "utils/errors.h"
#include "utils/utils.h"

#include "protocols/nmea_core.h"

/// Maximum number of satellites described in a single GSV packet
#define NMEA_CORE_MAX_SVS_PER_GSV 4

/// Offset of the used-satellites information from the end of the string
#define NMEA_CORE_GSA_OFFSET_USED_SVS 2

/**
 * @brief Characters used when parsing the NMEA protocol
 * @addtogroup NMEA_CORE_SYMBOLS
 * @{
 */
#define NMEA_CORE_SYMBOL_START '$'
#define NMEA_CORE_SYMBOL_CHECKSUM '*'
#define NMEA_CORE_SYMBOL_END_1 '\r'
#define NMEA_CORE_SYMBOL_END_2 '\n'
#define NMEA_CORE_SPLITTER ','
/** @} */

/**
 * @brief Characters used by nmea_core_scanf during parsing
 * @addtogroup NMEA_CORE_SCANF_SYMBOLS
 * @{
 */
/// Character preceding the type of the variable to scan
#define NMEA_CORE_FORMAT_SYMBOL '%'

/// End-of-string character
#define NMEA_CORE_NULL_TERMINATE_SYMBOL '\0'

/// Character indicating that a float variable will be read
#define NMEA_CORE_FLOAT_SYMBOL 'f'

/// Character indicating that a double variable will be read
#define NMEA_CORE_DOUBLE_SYMBOL 'l'

/// Character indicating that an int variable will be read
#define NMEA_CORE_INT_SYMBOL 'd'

/// Character indicating that a char/string variable will be read
#define NMEA_CORE_CHAR_SYMBOL 'c'
/** @} */

/**
 * @brief NMEA packet headers
 * @addtogroup NMEA_CORE_HEAD_PACKETS
 * @{
 */
#define NMEA_CORE_HEAD_GGA "GGA"
#define NMEA_CORE_HEAD_GSA "GSA"
#define NMEA_CORE_HEAD_RMC "RMC"
/** @} */

/**
 * @brief Suffixes identifying which satellite system a message belongs to
 * @addtogroup NMEA_CORE_GNSS_TYPE_SUFFIXES
 * @{
 */
#define NMEA_CORE_PREFIX_GLONASS "GL"
#define NMEA_CORE_PREFIX_GPS "GP"
/** @} */

/// NMEA packet parser
typedef struct
{
  /// Parser state
  enum
  {
    /// Looking for the character that defines the variable type
    NMEA_SCANF__SEARCHING_START,

    /// Call the appropriate parser for the type of the variable being read
    NMEA_SCANF__IDENTIFY_PARAM,

    /// Check whether more data needs to be parsed
    NMEA_SCANF__CHECK_NEXT,

    /// Parsing completed successfully
    NMEA_SCANF__DONE,
  } state;

  /// For iterating over the variadic arguments
  va_list args;

  /// Current position (cursor) in the buffer
  uint8_t *curent;
} nmea_scnaf_sm_t;

uint8_t *nmea_core_scanf(char *buf, char *format, ...);
error_t nmea_core_parse_message(nmea_core_rx_t *nmea_core, uint8_t *buf);

uint8_t nmea_core_calc_checksum(uint8_t *buf, uint16_t size);
error_t nmea_core_decode_checksum(uint8_t *ascii_checksum, uint8_t *hex_checksum);
void nmea_core_encode_checksum(uint8_t hex_checksum, uint8_t *ascii_checksum);

error_t nmea_core_scanf_int(uint8_t **start, int *param, char separator);
error_t nmea_core_scanf_char(uint8_t **start, char *param, char separator);
error_t nmea_core_scanf_float(uint8_t **start, float *param, char separator);
error_t nmea_core_scanf_double(uint8_t **start, double *param, char separator);

error_t nmea_core_parse_gga(nmea_core_rx_t *nmea_core, uint8_t *buf);
error_t nmea_core_parse_rmc(nmea_core_rx_t *nmea_core, uint8_t *buf);
error_t nmea_core_parse_gsa(nmea_core_rx_t *nmea_core, uint8_t *buf);

uint8_t *nmea_core_parse_gsa_used_svs(uint8_t *buf, int *svs_pnr_used);

/**
 * @brief Initialize the NMEA parser
 * @param nmea_core - NMEA parser instance
 */
void nmea_core_init(nmea_core_rx_t *nmea_core)
{
  nmea_core->rmc.count = 0;

  nmea_core->got_packet = NULL;
  nmea_core->idx = 0;
  nmea_core->state = NMEA_CORE_RX__WAIT_START;
  nmea_core->stats.success_packets = 0;
  nmea_core->stats.error_packets = 0;
  START_TIMER(nmea_core->stats.rx_success_timestamp);
}

/**
 * @brief Extract an NMEA packet from the stream
 * @param nmea_core - NMEA parser instance
 * @param symbol - received byte
 */
void nmea_core_rx_process(nmea_core_rx_t *nmea_core, uint8_t symbol)
{
  switch (nmea_core->state)
  {
    case NMEA_CORE_RX__WAIT_START:
      if (symbol == NMEA_CORE_SYMBOL_START)
      {
        nmea_core->state = NMEA_CORE_RX__WAIT_DATA;
      }
      break;

    case NMEA_CORE_RX__WAIT_DATA:
      if (symbol == NMEA_CORE_SYMBOL_CHECKSUM)
      {
        nmea_core->checksum_idx = 0;
        nmea_core->state = NMEA_CORE_RX__WAIT_CHECKSUM;
      }
      else if (symbol == NMEA_CORE_SYMBOL_START)
      {
        nmea_core->idx = 0;
        nmea_core->stats.error_packets++;
        nmea_core->state = NMEA_CORE_RX__WAIT_DATA;
      }
      else
      {
        nmea_core->buf[nmea_core->idx++] = symbol;

        if (nmea_core->idx == NMEA_PACKET_MAX_SIZE)
        {
          nmea_core->idx = 0;
          nmea_core->stats.error_packets++;
          nmea_core->state = NMEA_CORE_RX__WAIT_START;
        }
      }
      break;

    case NMEA_CORE_RX__WAIT_CHECKSUM:
      if (symbol == NMEA_CORE_SYMBOL_START)
      {
        nmea_core->idx = 0;
        nmea_core->stats.error_packets++;
        nmea_core->state = NMEA_CORE_RX__WAIT_DATA;
      }
      else
      {
        nmea_core->checksum[nmea_core->checksum_idx++] = symbol;
        if (nmea_core->checksum_idx == NMEA_PACKET_CHECKSUM_SIZE)
        {
          nmea_core->state = NMEA_CORE_RX__WAIT_END;
        }
      }
      break;

    case NMEA_CORE_RX__WAIT_END:
      if ((symbol != NMEA_CORE_SYMBOL_END_1) && (symbol != NMEA_CORE_SYMBOL_END_2))
      {
        nmea_core->idx = 0;
        nmea_core->stats.error_packets++;
        nmea_core->state = NMEA_CORE_RX__WAIT_START;
      }
      else if (symbol == NMEA_CORE_SYMBOL_START)
      {
        nmea_core->idx = 0;
        nmea_core->stats.error_packets++;
        nmea_core->state = NMEA_CORE_RX__WAIT_DATA;
      }
      else if (symbol == NMEA_CORE_SYMBOL_END_2)
      {
        uint8_t received_checksum;
        uint8_t calc_checksum;

        calc_checksum = nmea_core_calc_checksum(nmea_core->buf, nmea_core->idx);
        nmea_core_decode_checksum(nmea_core->checksum, &received_checksum);

        if (calc_checksum == received_checksum)
        {
          // Append a null terminator to the string
          nmea_core->buf[nmea_core->idx] = NMEA_CORE_NULL_TERMINATE_SYMBOL;

          // Perform the action required once a packet has been extracted
          if (nmea_core->got_packet != NULL)
          {
            nmea_core->got_packet(nmea_core->buf,
                                  utils_strnlen((char *)nmea_core->buf, NMEA_PACKET_MAX_SIZE));
          }

          // Parse the data packet
          nmea_core_parse_message(nmea_core, nmea_core->buf);
          nmea_core->stats.success_packets++;
          START_TIMER(nmea_core->stats.rx_success_timestamp);
        }
        else
        {
          nmea_core->stats.error_packets++;
        }
        nmea_core->idx = 0;
        nmea_core->state = NMEA_CORE_RX__WAIT_START;
      }
      break;
  }
}

/**
 * @brief Decode the NMEA packet checksum byte from ASCII into a uint8_t byte
 * @param ascii_checksum - checksum in the NMEA protocol ASCII format
 * @param hex_checksum - byte representation of the checksum
 * @return ERR_OK on success, otherwise an error
 */
error_t nmea_core_decode_checksum(uint8_t *ascii_checksum, uint8_t *hex_checksum)
{
  unsigned value;
  int res = sscanf((char *)ascii_checksum, "%02X", &value);

  if (res != 1)
  {
    return ERR_RESULT;
  }
  *hex_checksum = (uint8_t)value;
  return ERR_OK;
}

/**
 * @brief Convert the checksum from hex to the NMEA (ASCII) format
 * @param hex_checksum - checksum byte in hex
 * @param ascii_checksum - two checksum bytes in ASCII
 */
void nmea_core_encode_checksum(uint8_t hex_checksum, uint8_t *ascii_checksum)
{
  sprintf((char *)ascii_checksum, "%02X", hex_checksum);
}

/**
 * @brief Calculate the NMEA packet checksum
 * @param buf - pointer to the buffer with the NMEA packet, between '$' and '*'
 * @param size - buffer size
 * @return Checksum
 */
uint8_t nmea_core_calc_checksum(uint8_t *buf, uint16_t size)
{
  uint8_t checksum = 0;

  for (int i = 0; i < size; i++)
  {
    checksum ^= buf[i];
  }
  return checksum;
}

/**
 * @brief Parse an NMEA message without the checksum (between the '$' and '*' characters).
 * @param nmea_core - NMEA parser instance
 * @param buf - pointer to the buffer with the received message.
 *          Null-terminated string without the '$' and '*' characters
 * @return ERR_OK - parsing succeeded
 *         ERR_RESULT - error while parsing the string
 */
error_t nmea_core_parse_message(nmea_core_rx_t *nmea_core, uint8_t *buf)
{
  error_t res = ERR_OK;

  if (strstr((const char *)buf, NMEA_CORE_HEAD_GGA) != NULL)
  {
    res = nmea_core_parse_gga(nmea_core, buf);
  }
  else if (strstr((const char *)buf, NMEA_CORE_HEAD_RMC) != NULL)
  {
    res = nmea_core_parse_rmc(nmea_core, buf);

    if (res == ERR_OK)
    {
      // todo move out of core, add a callback
      // nmea_core_convert_time_to_unix(nmea_core->rmc.nmea_time, nmea_core->rmc.nmea_date);
    }
  }
  else if (strstr((const char *)buf, NMEA_CORE_HEAD_GSA) != NULL)
  {
    res = nmea_core_parse_gsa(nmea_core, buf);
  }

  return res;
}

/**
 * @brief Parse an RMC packet with position data
 * @param nmea_core - NMEA parser instance
 * @param buf - pointer to a null-terminated string with data
 * @return ERR_OK - parsing succeeded
 *         ERR_RESULT - parsing failed
 */
error_t nmea_core_parse_rmc(nmea_core_rx_t *nmea_core, uint8_t *buf)
{
  uint8_t *res = nmea_core_scanf(
      (char *)buf, "%c,%f,%c,%l,%c,%l,%c,%f,%f,%d,%f,%c,%c", &nmea_core->rmc.message_id,
      &nmea_core->rmc.nmea_time, &nmea_core->rmc.status, &nmea_core->rmc.latitude,
      &nmea_core->rmc.n_s_indicator, &nmea_core->rmc.longitude, &nmea_core->rmc.e_w_indicator,
      &nmea_core->rmc.horizontal_speed, &nmea_core->rmc.heading, &nmea_core->rmc.nmea_date,
      &nmea_core->rmc.mag_dec, &nmea_core->rmc.mag_dec_dir, &nmea_core->rmc.mode_indicator);

  if (res != NULL)
  {
    nmea_core->rmc.count++;
    return ERR_OK;
  }
  else
  {
    return ERR_RESULT;
  }
}

/**
 * @brief Parse a GGA packet with position information
 * @param nmea_core - NMEA parser instance
 * @param buf - pointer to a null-terminated string with data
 * @return ERR_OK - parsing succeeded
 *         ERR_RESULT - parsing failed
 */
error_t nmea_core_parse_gga(nmea_core_rx_t *nmea_core, uint8_t *buf)
{
  uint8_t *res = nmea_core_scanf(
      (char *)buf, "%c,%f,%l,%c,%l,%c,%d,%d,%f,%f,%c,%f,%c,%d,%d", &nmea_core->gga.message_id,
      &nmea_core->gga.nmea_time, &nmea_core->gga.latitude, &nmea_core->gga.n_s_indicator,
      &nmea_core->gga.longtitude, &nmea_core->gga.e_w_indicator, &nmea_core->gga.fix_indicator,
      &nmea_core->gga.used_satellites_number, &nmea_core->gga.hdop, &nmea_core->gga.msl_altitude,
      &nmea_core->gga.altitude_units, &nmea_core->gga.diff_ellipsoid_and_msl,
      &nmea_core->gga.geoid_units, &nmea_core->gga.age_corrections, &nmea_core->gga.ref_station_id);

  if (res != NULL)
  {
    return ERR_OK;
  }
  else
  {
    return ERR_RESULT;
  }
}

/**
 * @brief Parse the IDs of the satellites used in the GPS fix from a GSA packet
 * @param buf - pointer to a null-terminated string with data
 * @param svs_pnr_used - pointer to the array that receives the satellite IDs
 * @return Pointer to the string element after parsing
 *          NULL - on error
 */
uint8_t *nmea_core_parse_gsa_used_svs(uint8_t *buf, int *svs_pnr_used)
{
  /// Count the number of satellites described in the string
  uint8_t num_used_svs = 0;
  uint8_t *tmp = buf;

  while (*tmp)
  {
    if (*tmp == NMEA_CORE_SPLITTER)
    {
      num_used_svs++;
    }
    tmp++;
  }

  for (uint16_t i = 0; i < num_used_svs - NMEA_CORE_GSA_OFFSET_USED_SVS; i++)
  {
    buf = nmea_core_scanf((char *)buf, "%d,", &svs_pnr_used[i]);

    if (buf == NULL)
    {
      return NULL;
    }
  }

  return buf;
}

/**
 * @brief Parse a GSA packet with the satellites used in the fix,
 *          the GPS fix quality and the dilution of precision factors
 * @param nmea_core - NMEA parser instance
 * @param buf - pointer to a null-terminated string with data
 * @return ERR_OK - parsing succeeded
 *         ERR_RESULT - parsing failed
 */
error_t nmea_core_parse_gsa(nmea_core_rx_t *nmea_core, uint8_t *buf)
{
  // Fill in the GPS fix quality information
  buf = nmea_core_scanf((char *)buf, "%c,%c,%d,", &nmea_core->gsa.message_id,
                        &nmea_core->gsa.mode_indicator, &nmea_core->gsa.current_mode);

  if (buf == NULL)
  {
    return ERR_RESULT;
  }

  // Fill in the information about the satellites in use
  if (strstr(nmea_core->gsa.message_id, NMEA_CORE_PREFIX_GLONASS) != NULL)
  {
    // Clear the old information
    for (int i = 0; i < NMEA_CORE_MAX_USED_GLO; i++)
    {
      nmea_core->gsa.glo_pnr_used[i] = 0;
    }
    // Fill in the new one
    buf = nmea_core_parse_gsa_used_svs(buf, nmea_core->gsa.glo_pnr_used);
  }
  else if (strstr(nmea_core->gsa.message_id, NMEA_CORE_PREFIX_GPS) != NULL)
  {
    // Clear the old information
    for (int i = 0; i < NMEA_CORE_MAX_USED_GPS; i++)
    {
      nmea_core->gsa.gps_pnr_used[i] = 0;
    }
    // Fill in the new one
    buf = nmea_core_parse_gsa_used_svs(buf, nmea_core->gsa.gps_pnr_used);
  }
  else
  {
    return ERR_RESULT;
  }

  if (buf == NULL)
  {
    return ERR_RESULT;
  }

  // Fill in the dilution of precision information
  buf = nmea_core_scanf((char *)buf, "%f,%f,%f,", &nmea_core->gsa.pdop, &nmea_core->gsa.hdop,
                        &nmea_core->gsa.vdop);

  if (buf == NULL)
  {
    return ERR_RESULT;
  }

  return ERR_OK;
}

/**
 * @brief Parse a float
 * @param start - pointer to the pointer to the buffer start
 * @param param - pointer to the variable that receives the result
 * @param separator - character that marks the end of the variable.
 If parsing the last element of the string, NULL must be passed
 * @return see error_t
 */
error_t nmea_core_scanf_float(uint8_t **start, float *param, char separator)
{
  char *end;
  end = strchr((const char *)*start, (int)separator);

  if (end != NULL)
  {
    *end = 0;
  }
  else if (separator != NMEA_CORE_NULL_TERMINATE_SYMBOL)
  {
    return ERR_RESULT;
  }

  // If we are already at the separator, the parameter is missing from the string
  if (*start == (uint8_t *)end)
  {
    *param = 0.0f;
  }
  else
  {
    *param = strtof((const char *)*start, &end);
  }

  // Restore the separator character
  *end = (int)separator;

  // Advance the buffer pointer to the next element
  *start = (uint8_t *)++end;

  return ERR_OK;
}

/**
 * @brief Parse a double
 * @param start - pointer to the pointer to the buffer start
 * @param param - pointer to the variable that receives the result
 * @param separator - character that marks the end of the variable.
 * If parsing the last element of the string, NULL must be passed
 * @return see error_t
 */
error_t nmea_core_scanf_double(uint8_t **start, double *param, char separator)
{
  char *end;
  end = strchr((const char *)*start, (int)separator);

  if (end != NULL)
  {
    *end = 0;
  }
  else if (separator != NMEA_CORE_NULL_TERMINATE_SYMBOL)
  {
    return ERR_RESULT;
  }

  // If we are already at the separator, the parameter is missing from the string
  if (*start == (uint8_t *)end)
  {
    *param = 0.0f;
  }
  else
  {
    *param = strtod((const char *)*start, &end);
  }

  // Restore the separator character
  *end = (int)separator;

  // Advance the buffer pointer to the next element
  *start = (uint8_t *)++end;

  return ERR_OK;
}

/**
 * @brief Parse a char
 * @param start - pointer to the pointer to the buffer start
 * @param param - pointer to the variable that receives the result
 * @param separator - character that marks the end of the variable.
 If parsing the last element of the string, NULL must be passed
 * @return see error_t
 */
error_t nmea_core_scanf_char(uint8_t **start, char *param, char separator)
{
  char *end;
  // Check whether the separator is present in the string
  end = strchr((const char *)*start, (int)separator);

  if (end != NULL)
  {
    // If the separator is found, put a null at that character
    // to split it from the rest of the string
    *end = 0;
  }
  else if (separator != NMEA_CORE_NULL_TERMINATE_SYMBOL)
  {
    return ERR_RESULT;
  }

  // If we are already at the separator, the parameter is missing from the string
  if (*start == (uint8_t *)end)
  {
    strcpy(param, "\0");
  }
  else
  {
    strcpy(param, (const char *)*start);
  }

  // Restore the separator character
  *end = (int)separator;

  // Advance the buffer pointer to the next element
  *start = (uint8_t *)++end;

  return ERR_OK;
}

/**
 * @brief Parse an int
 * @param start - pointer to the pointer to the buffer start
 * @param param - pointer to the variable that receives the result
 * @param separator - character that marks the end of the variable.
 If parsing the last element of the string, NULL must be passed
 * @return see error_t
 */
error_t nmea_core_scanf_int(uint8_t **start, int *param, char separator)
{
  char *end;
  end = strchr((const char *)*start, (int)separator);

  if (end != NULL)
  {
    *end = 0;
  }
  else if (separator != NMEA_CORE_NULL_TERMINATE_SYMBOL)
  {
    return ERR_RESULT;
  }

  // If we are already at the separator, the parameter is missing from the string
  if (*start == (uint8_t *)end)
  {
    *param = 0;
  }
  else
  {
    *param = atoi((const char *)*start);
  }

  // Restore the separator character
  *end = (int)separator;

  // Advance the buffer pointer to the next element
  *start = (uint8_t *)++end;

  return ERR_OK;
}

/**
 * @brief Parse an NMEA string, taking possible missing data into account
 * @param buf - pointer to the buffer with a null-terminated string
 * @param format - string defining the parsing template. Supported:
 *           %f - float
 *           %l - double
 *           %d - int
 *           %c - char/string
 * @param ... - variadic pointers to the variables that receive the result
 * @note The main difference from sscanf is that if the data between separator characters
 *          is missing, the parser does not break
 * @return Pointer to the string element where parsing stopped.
 *         NULL - if an error occurred
 */
uint8_t *nmea_core_scanf(char *buf, char *format, ...)
{
  nmea_scnaf_sm_t parser;
  parser.state = NMEA_SCANF__SEARCHING_START;

  parser.curent = (uint8_t *)buf;

  va_start(parser.args, format);
  bool is_done = false;

  while (!is_done)
  {

    switch (parser.state)
    {
      case NMEA_SCANF__SEARCHING_START:
        if (*format == NMEA_CORE_FORMAT_SYMBOL)
        {
          format++;
          parser.state = NMEA_SCANF__IDENTIFY_PARAM;
        }
        else
        {
          format++;
          parser.curent++;
          if (*format == NMEA_CORE_NULL_TERMINATE_SYMBOL)
          {
            is_done = true;
          }
        }
        break;

      case NMEA_SCANF__IDENTIFY_PARAM:
        if (*format == NMEA_CORE_CHAR_SYMBOL)
        {
          nmea_core_scanf_char(&parser.curent, va_arg(parser.args, char *), *++format);
        }
        else if (*format == NMEA_CORE_INT_SYMBOL)
        {
          nmea_core_scanf_int(&parser.curent, va_arg(parser.args, int *), *++format);
        }
        else if (*format == NMEA_CORE_FLOAT_SYMBOL)
        {
          nmea_core_scanf_float(&parser.curent, va_arg(parser.args, float *), *++format);
        }
        else if (*format == NMEA_CORE_DOUBLE_SYMBOL)
        {
          nmea_core_scanf_double(&parser.curent, va_arg(parser.args, double *), *++format);
        }
        else
        {
          va_end(parser.args);
          return NULL;
        }

        parser.state = NMEA_SCANF__CHECK_NEXT;
        break;

      case NMEA_SCANF__CHECK_NEXT:
        if (*++format == NMEA_CORE_NULL_TERMINATE_SYMBOL)
        {
          is_done = true;
          parser.state = NMEA_SCANF__DONE;
        }
        else
        {
          parser.state = NMEA_SCANF__SEARCHING_START;
        }
        break;

      case NMEA_SCANF__DONE:
        break;
    }
  }

  va_end(parser.args);
  return parser.curent;
}

/**
 * @brief Wrap an NMEA packet with the start character and the checksum
 * @param packet - pointer to the string that receives the built NMEA packet
 * @param max_size - maximum allowed size of the resulting NMEA packet
 * @param data - pointer to a null-terminated string with the packet payload (between $ and *)
 * @param size - size of the data string, not counting '\0'
 * @return ERR_OK - if the packet was built successfully
 *         ERR_RESULT - if an error occurred
 */
error_t nmea_core_create_message(char *packet, uint16_t max_size, char *data, uint16_t size)
{
  error_t res = ERR_OK;

  uint8_t checksum = nmea_core_calc_checksum((uint8_t *)data, size);

  int length = snprintf(packet, max_size, "%c%s*%02X", NMEA_CORE_SYMBOL_START, data, checksum);

  if ((length <= 0) || (length >= max_size))
  {
    res = ERR_RESULT;
  }

  return res;
}

/**
 * @brief Convert a UTC date/time from the NMEA format to the UNIX format
 * @param nmea_time - time in the NMEA format, hhmmss.ss
 * @param nmea_date - date in the NMEA format, ddmmyy
 * @return Timestamp in the UNIX format, [ms]
 */
uint32_t nmea_core_convert_time_to_unix(float nmea_time, int nmea_date)
{
  // Extract the integer part
  int time = (int)nmea_time;

  // Extract the hours
  int hour = time / 10000;

  // Extract the minutes
  time = time % 10000;
  int min = time / 100;

  // Extract the seconds
  time = time % 100;
  int sec = time;

  // Extract the day
  int day = nmea_date / 10000;

  // Extract the month
  nmea_date = nmea_date % 10000;
  int month = nmea_date / 100;

  // Extract the year
  nmea_date = nmea_date % 100;
  int year = nmea_date + 2000;

  uint32_t res_time = utils_convert_utc_time_to_unix(year, month, day, hour, min, sec);

  return res_time;
}
