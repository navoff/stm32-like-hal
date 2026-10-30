/**
 * @file tick_timer.c
 * @brief SysTick-based 1 ms tick timer and blocking delay.
 */

#include "config.h"

#include "drivers/tick_timer.h"

volatile uint32_t tick_time;
void (*tick_timer_app_process)(void);

void tick_timer_init(void (*app_process)(void), uint32_t start_timestamp)
{
  tick_time = start_timestamp;
  tick_timer_app_process = app_process;

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)

  // period = 1 ms
  // do not change the order!!!
  SysTick_Config(SystemCoreClock / 8 / 1000);
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

#elif defined(GD32E11X) || defined(GD32L23X)
  SysTick_Config(SystemCoreClock / 8 / 1000);
  systick_clksource_set(SYSTICK_CLKSOURCE_HCLK_DIV8);

#elif defined(STM32F0XX) || defined(STM32L4XX)

  // period = 1 ms
  // do not change the order!!!
  SysTick_Config(SystemCoreClock / 8 / 1000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK_DIV8);

#else
  #error This MCU not supported
#endif
}

void tick_timer_deinit(void)
{
#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX) || defined(STM32L4XX) ||        \
    defined(STM32F0XX) || defined(GD32E11X) || defined(GD32L23X)
  SysTick->VAL = 0UL;

  SysTick->CTRL = (0ul << SysTick_CTRL_CLKSOURCE_Pos) | (0ul << SysTick_CTRL_TICKINT_Pos) |
                  (0ul << SysTick_CTRL_ENABLE_Pos);

#else

  #error This MCU not supported
#endif
}

void tick_timer_ms_delay(uint32_t delay)
{
  uint32_t timestamp;
  START_TIMER(timestamp);
  while (CHECK_TIMER(timestamp, delay) == 0)
  {
  }
}

void SysTick_Handler(void)
{
  tick_time++;

  if (tick_timer_app_process != NULL)
  {
    tick_timer_app_process();
  }
}
