#pragma once

/**
 * @file tim.h
 * @brief Timer helpers: clocking, prescaler calculation and output compare setup.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "utils/errors.h"

#if defined(STM32F1XX)
  /// Number of timers in STM32F1XX
  #define TIM_AMOUNT 14
#elif defined(STM32F30X)
  /// Number of timers in STM32F30X
  #define TIM_AMOUNT 17
#elif defined(STM32F4XX)
  /// Number of timers in STM32F4XX
  #define TIM_AMOUNT 14
#elif defined(STM32L4XX)
  /// Number of timers in STM32L4XX
  #define TIM_AMOUNT (11 - 3)
#endif

typedef struct
{
  /// Pointer to the timer
  TIM_TypeDef *timer;

  /// Timer clock line
  /// For STM32F1xx - RCC_APBxPeriph_TIMx
  /// For STM32F4xx - RCC_APBx_Peripherals
  /// For STM32L4xx - LL_APBx_GRP1_PERIPH_TIMx
  uint32_t rcc_apb_line;

  /// Number of the APB bus the timer is on
  uint8_t rcc_apb_number;

  /// Number of channels (capture/compare registers)
  uint8_t channel_amount;

  /// Whether the timer is 32-bit
  bool is_32bit;
} tim_hw_item_t;

extern const uint16_t tim_dma_channels[];
extern const tim_hw_item_t tim_hw_items[];

void tim_rcc_cmd(uint8_t tim_number, bool new_state);

#if defined(STM32F4XX)
uint32_t tim_get_tim_frequency(uint8_t tim_number);
error_t tim_calc_prescaler(uint8_t tim_number, uint32_t desired_frequency, uint16_t *prescaler);

void tim_oc_init(uint8_t tim_number, uint8_t channel_number, TIM_OCInitTypeDef *init_struct);
void tim_oc_preload_config(uint8_t tim_number, uint8_t channel_number, bool state);
#endif

#ifdef __cplusplus
}
#endif
