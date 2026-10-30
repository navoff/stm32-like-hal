#pragma once

/**
 * @file rtc.h
 * @brief Real-time clock (RTC) driver.
 */

#include <stdint.h>

#include "utils/errors.h"

#define RTC_YEAR_OFFSET 2000

#pragma pack(push, 1)

/// Current RTC time in BCD format
typedef struct
{
#if (FEATURE_ENABLE_RTC_BCD_FORMAT)
  /// Tens digit of hours
  uint8_t hour_hi;

  /// Units digit of hours
  uint8_t hour_lo;

  /// Tens digit of minutes
  uint8_t minute_hi;

  /// Units digit of minutes
  uint8_t minute_lo;

  /// Tens digit of seconds
  uint8_t second_hi;

  /// Units digit of seconds
  uint8_t second_lo;
#endif

#if (FEATURE_ENABLE_RTC_UINT_FORMAT)
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
#endif
} rtc_time_t;

#pragma pack(pop)

void rtc_init(void);
void rtc_process(void);
void rtc_get_current_time(rtc_time_t *time);
void rtc_set_current_time(rtc_time_t *time);
uint8_t rtc_check_and_clear_sec_flag(void);
