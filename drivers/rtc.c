/**
 * @file rtc.c
 * @brief Real-time clock (RTC) driver.
 */

#include "config.h"

#include <string.h>

#include "rtc.h"

#if !defined(STM32F1XX) && !defined(STM32F0XX)
  #error This MCU not supported
#endif

#if !defined(FEATURE_ENABLE_RTC_BCD_FORMAT)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_RTC_BCD_FORMAT 0
#endif

#if !defined(FEATURE_ENABLE_RTC_UINT_FORMAT)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_RTC_UINT_FORMAT 0
#endif

#define SECONDS_IN_DAY (24ul * 60ul * 60ul)
#define SECONDS_IN_HOUR (60ul * 60ul)

#define RTC_INIT_KEY 0x7AB8

void rtc_config_after_power_on(void);
void rtc_update_current_time(uint32_t counter);

struct
{
  rtc_time_t current_time;
  uint8_t sec_flag;
} rtc;

void rtc_init(void)
{
#if defined(STM32F1XX)
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

  // if the RTC has not been configured yet
  if (BKP_ReadBackupRegister(BKP_DR1) != RTC_INIT_KEY)
  {
    rtc_config_after_power_on();

    BKP_WriteBackupRegister(BKP_DR1, RTC_INIT_KEY);

    rtc_time_t time = {
        .hour_hi = 1,
        .hour_lo = 2,
        .minute_hi = 0,
        .minute_lo = 0,
        .second_hi = 0,
        .second_lo = 0,
    };
    rtc_set_current_time(&time);
  }
  else
  {
    RTC_WaitForSynchro();

    // enable the RTC "second" interrupt
    RTC_ITConfig(RTC_IT_SEC, ENABLE);
    RTC_WaitForLastTask();

    uint32_t counter = RTC_GetCounter();
    rtc_update_current_time(counter);
  }

#elif defined(STM32F0XX)
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  LL_PWR_EnableBkUpAccess();

  if (LL_RTC_BAK_GetRegister(RTC, LL_RTC_BKP_DR0) != RTC_INIT_KEY)
  {
    rtc_config_after_power_on();
    LL_RTC_BAK_SetRegister(RTC, LL_RTC_BKP_DR0, RTC_INIT_KEY);
  }
  else
  {
  }

#else
  #error This MCU not supported
#endif
}

void rtc_process(void)
{
#if defined(STM32F1XX)
  if (RTC->CRL & RTC_CRL_SECF)
  {
    RTC->CRL &= ~RTC_CRL_SECF;

  #if (FEATURE_ENABLE_RTC_BCD_FORMAT)
    rtc.current_time.second_lo++;
    if (rtc.current_time.second_lo >= 10)
    {
      rtc.current_time.second_lo = 0;

      rtc.current_time.second_hi++;
      if (rtc.current_time.second_hi >= 6)
      {
        rtc.current_time.second_hi = 0;

        rtc.current_time.minute_lo++;
        if (rtc.current_time.minute_lo >= 10)
        {
          rtc.current_time.minute_lo = 0;

          rtc.current_time.minute_hi++;
          if (rtc.current_time.minute_hi >= 6)
          {
            rtc.current_time.minute_hi = 0;

            rtc.current_time.hour_lo++;
            if (rtc.current_time.hour_hi == 2)
            {
              if (rtc.current_time.hour_lo >= 4)
              {
                rtc.current_time.hour_lo = 0;
                rtc.current_time.hour_hi = 0;
              }
            }
            else
            {
              if (rtc.current_time.hour_lo >= 10)
              {
                rtc.current_time.hour_lo = 0;

                rtc.current_time.hour_hi++;
              }
            }
          }
        }
      }
    }
  #endif

  #if (FEATURE_ENABLE_RTC_UINT_FORMAT)
    rtc.current_time.second++;
    if (rtc.current_time.second >= 60)
    {
      rtc.current_time.second = 0;

      rtc.current_time.minute++;
      if (rtc.current_time.minute >= 60)
      {
        rtc.current_time.minute = 0;

        rtc.current_time.hour++;
        if (rtc.current_time.hour >= 24)
        {
          rtc.current_time.hour = 0;
        }
      }
    }
  #endif
    rtc.sec_flag = 1;
  }
#elif defined(STM32F0XX)

  #if (FEATURE_ENABLE_RTC_BCD_FORMAT)
    #error Not implemented
  #endif

  #if (FEATURE_ENABLE_RTC_UINT_FORMAT)
  uint8_t second = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetSecond(RTC));
  if (second != rtc.current_time.second)
  {
    rtc.current_time.year = RTC_YEAR_OFFSET + __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC));
    rtc.current_time.month = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC));
    rtc.current_time.day = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));
    rtc.current_time.hour = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetHour(RTC));
    rtc.current_time.minute = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetMinute(RTC));
    rtc.current_time.second = second;
  }
  #endif

#else
  #error This MCU not supported
#endif
}

uint8_t rtc_check_and_clear_sec_flag(void)
{
  uint8_t res = rtc.sec_flag;
  rtc.sec_flag = 0;
  return res;
}

void rtc_config_after_power_on(void)
{
#if defined(STM32F1XX)
  // allow access to BKP
  PWR_BackupAccessCmd(ENABLE);

  // reset BKP
  BKP_DeInit();

  // enable LSE
  RCC_LSEConfig(RCC_LSE_ON);
  while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
  }

  // select LSE as the RTC CLK source
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

  // enable RTC CLK
  RCC_RTCCLKCmd(ENABLE);
  RTC_WaitForSynchro();
  RTC_WaitForLastTask();

  // set the RTC period to 1 s
  RTC_SetPrescaler(32767);
  RTC_WaitForLastTask();

  // enable the RTC "second" interrupt
  RTC_ITConfig(RTC_IT_SEC, ENABLE);
  RTC_WaitForLastTask();

#elif defined(STM32F0XX)

  LL_RCC_EnableRTC();

  LL_RTC_InitTypeDef rtc_init_struct = {0};

  rtc_init_struct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
  rtc_init_struct.AsynchPrescaler = 127;
  rtc_init_struct.SynchPrescaler = 255;

  LL_RTC_Init(RTC, &rtc_init_struct);

  LL_RTC_DisableWriteProtection(RTC);
  LL_RTC_EnableInitMode(RTC);
  LL_RTC_DATE_Config(RTC, LL_RTC_WEEKDAY_MONDAY, 0x01, LL_RTC_MONTH_JANUARY, 0x00);
  LL_RTC_TIME_Config(RTC, LL_RTC_TIME_FORMAT_PM, 0x00, 0x00, 0x00);
  LL_RTC_DisableInitMode(RTC);
  LL_RTC_EnableWriteProtection(RTC);

#else
  #error This MCU not supported
#endif
}

void rtc_get_current_time(rtc_time_t *time)
{
  memcpy(time, &rtc.current_time, sizeof(rtc_time_t));
}

void rtc_update_current_time(uint32_t counter)
{
  counter %= SECONDS_IN_DAY;

  uint32_t h = counter / SECONDS_IN_HOUR;
  uint32_t seconds = counter % SECONDS_IN_HOUR;
#if (FEATURE_ENABLE_RTC_UINT_FORMAT)
  rtc.current_time.hour = h;
#endif
#if (FEATURE_ENABLE_RTC_BCD_FORMAT)
  rtc.current_time.hour_hi = h / 10;
  rtc.current_time.hour_lo = h % 10;
#endif

  uint32_t m = seconds / 60;
#if (FEATURE_ENABLE_RTC_UINT_FORMAT)
  rtc.current_time.minute = m;
#endif
#if (FEATURE_ENABLE_RTC_BCD_FORMAT)
  rtc.current_time.minute_hi = m / 10;
  rtc.current_time.minute_lo = m % 10;
#endif

  uint32_t s = seconds % 60;
#if (FEATURE_ENABLE_RTC_UINT_FORMAT)
  rtc.current_time.second = s;
#endif
#if (FEATURE_ENABLE_RTC_BCD_FORMAT)
  rtc.current_time.second_hi = s / 10;
  rtc.current_time.second_lo = s % 10;
#endif

  rtc.sec_flag = 0;
}

void rtc_set_current_time(rtc_time_t *time)
{
#if defined(STM32F1XX)
  memcpy(&rtc.current_time, time, sizeof(rtc_time_t));
  rtc.sec_flag = 0;

  #if (FEATURE_ENABLE_RTC_UINT_FORMAT)
    #error Not implemented
  #endif
  #if (FEATURE_ENABLE_RTC_BCD_FORMAT)
  uint32_t counter = (time->hour_hi * 10 + time->hour_lo) * 60;
  counter += (time->minute_hi * 10 + time->minute_lo) * 60;
  counter += time->second_hi * 10 + time->second_lo;
  #endif

  PWR_BackupAccessCmd(ENABLE);

  RTC_WaitForLastTask();
  RTC_SetCounter(counter);
  RTC_WaitForLastTask();

  PWR_BackupAccessCmd(DISABLE);
#elif defined(STM32F0XX)

  LL_RTC_DisableWriteProtection(RTC);
  LL_RTC_EnableInitMode(RTC);

  #if (FEATURE_ENABLE_RTC_UINT_FORMAT)
  LL_RTC_DATE_Config(RTC, LL_RTC_WEEKDAY_MONDAY, __LL_RTC_CONVERT_BIN2BCD(time->day),
                     __LL_RTC_CONVERT_BIN2BCD(time->month),
                     __LL_RTC_CONVERT_BIN2BCD(time->year - RTC_YEAR_OFFSET));
  LL_RTC_TIME_Config(RTC, LL_RTC_TIME_FORMAT_PM, __LL_RTC_CONVERT_BIN2BCD(time->hour),
                     __LL_RTC_CONVERT_BIN2BCD(time->minute),
                     __LL_RTC_CONVERT_BIN2BCD(time->second));
  #endif
  #if (FEATURE_ENABLE_RTC_BCD_FORMAT)
    #error Not implemented
  #endif

  LL_RTC_DisableInitMode(RTC);
  LL_RTC_EnableWriteProtection(RTC);

#else
  #error This MCU not supported
#endif
}
