/**
 * @file main.c
 * @brief Minimal entry point of the HAL test application.
 */

#include "config.h"
#include "drivers/tick_timer.h"

int main(void)
{
  SystemCoreClockUpdate();

  tick_timer_init(NULL, 0);

  uint32_t led_timestamp;
  START_TIMER(led_timestamp);

  while (1)
  {
    if (CHECK_TIMER(led_timestamp, 500))
    {
      START_TIMER(led_timestamp);
    }
  }
}
