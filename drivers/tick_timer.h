#pragma once

/**
 * @file tick_timer.h
 * @brief SysTick-based 1 ms tick timer and blocking delay.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

#define START_TIMER(timestamp) (timestamp = tick_time)
#define CHECK_TIMER(timestamp, time) ((tick_time - timestamp) >= time)
#define OVERFLOW_TIMER(timestamp) (timestamp = (tick_time - 86400000))
#define GET_TIMESPAN(timestamp_old, timestamp_new) ((timestamp_new) - (timestamp_old))

/**
 * @brief Configure and start SysTick timer.
 * @param app_process - pointer to function that will be called every millisecond.
 * If the value is NULL, no function will be called.
 */
void tick_timer_init(void (*app_process)(void), uint32_t start_timestamp);

/**
 * @brief Disable SysTick timer
 */
void tick_timer_deinit(void);

/**
 * @brief Blocking delay function using tick timer
 * @param delay - delay time in milliseconds
 */
void tick_timer_ms_delay(uint32_t delay);

extern volatile uint32_t tick_time;

#ifdef __cplusplus
}
#endif
