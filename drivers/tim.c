/**
 * @file tim.c
 * @brief Timer helpers: clocking, prescaler calculation and output compare setup.
 */

#include "config.h"

#include <stdbool.h>
#include <stddef.h>

#include "utils/assert.h"

#include "drivers/tim.h"

/// Maximum timer prescaler value
#define TIM_MAX_VALUE_PRESCALER (uint32_t)0xFFFF

/// Maximum number of compare channels
#define TIM_MAX_NUMBER_CHANNELS 4

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX) || defined(STM32L4XX)
  #define TIM_IS_ADVANCED(x) (((x) == 1) || ((x) == 8))
#else
  #warning "Not implemented for this MCU!"
#endif

#if defined(STM32F1XX)
const tim_hw_item_t tim_hw_items[TIM_AMOUNT] = {
    {TIM1, RCC_APB2Periph_TIM1, 2, 4, .is_32bit = false},
    {TIM2, RCC_APB1Periph_TIM2, 1, 4, .is_32bit = false},
    {TIM3, RCC_APB1Periph_TIM3, 1, 4, .is_32bit = false},
    {TIM4, RCC_APB1Periph_TIM4, 1, 4, .is_32bit = false},
    {TIM5, RCC_APB1Periph_TIM5, 1, 4, .is_32bit = false},
    {TIM6, RCC_APB1Periph_TIM6, 1, 4, .is_32bit = false},
    {TIM7, RCC_APB1Periph_TIM7, 1, 0, .is_32bit = false},
    {TIM8, RCC_APB2Periph_TIM8, 2, 4, .is_32bit = false},
    {TIM9, RCC_APB2Periph_TIM9, 2, 2, .is_32bit = false},
    {TIM10, RCC_APB2Periph_TIM10, 2, 1, .is_32bit = false},
    {TIM11, RCC_APB2Periph_TIM11, 2, 1, .is_32bit = false},
    {TIM12, RCC_APB1Periph_TIM12, 1, 2, .is_32bit = false},
    {TIM13, RCC_APB1Periph_TIM13, 1, 1, .is_32bit = false},
    {TIM14, RCC_APB1Periph_TIM14, 1, 1, .is_32bit = false},
};

#elif defined(STM32F30X)
const tim_hw_item_t tim_hw_items[TIM_AMOUNT] = {
    {TIM1, RCC_APB2Periph_TIM1, 2, 4, .is_32bit = false},
    {TIM2, RCC_APB1Periph_TIM2, 1, 4, .is_32bit = false},
    {TIM3, RCC_APB1Periph_TIM3, 1, 4, .is_32bit = false},
    {TIM4, RCC_APB1Periph_TIM4, 1, 4, .is_32bit = false},
    {NULL},
    {TIM6, RCC_APB1Periph_TIM6, 1, 0, .is_32bit = false},
    {TIM7, RCC_APB1Periph_TIM7, 1, 0, .is_32bit = false},
    {TIM8, RCC_APB2Periph_TIM8, 2, 4, .is_32bit = false},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {NULL},
    {TIM15, RCC_APB2Periph_TIM15, 2, 2, .is_32bit = false},
    {TIM16, RCC_APB2Periph_TIM16, 2, 2, .is_32bit = false},
    {TIM17, RCC_APB2Periph_TIM17, 2, 2, .is_32bit = false},
};

#elif defined(STM32F4XX)

const tim_hw_item_t tim_hw_items[TIM_AMOUNT] = {
    {TIM1, RCC_APB2Periph_TIM1, 2, 4, .is_32bit = false},
    {TIM2, RCC_APB1Periph_TIM2, 1, 4, .is_32bit = true},
    {TIM3, RCC_APB1Periph_TIM3, 1, 4, .is_32bit = false},
    {TIM4, RCC_APB1Periph_TIM4, 1, 4, .is_32bit = false},
    {TIM5, RCC_APB1Periph_TIM5, 1, 4, .is_32bit = true},
    {TIM6, RCC_APB1Periph_TIM6, 1, 4, .is_32bit = false},
    {TIM7, RCC_APB1Periph_TIM7, 1, 4, .is_32bit = false},
    {TIM8, RCC_APB2Periph_TIM8, 2, 4, .is_32bit = false},
    {TIM9, RCC_APB2Periph_TIM9, 2, 2, .is_32bit = false},
    {TIM10, RCC_APB2Periph_TIM10, 2, 1, .is_32bit = false},
    {TIM11, RCC_APB2Periph_TIM11, 2, 1, .is_32bit = false},
    {TIM12, RCC_APB1Periph_TIM12, 1, 2, .is_32bit = false},
    {TIM13, RCC_APB1Periph_TIM13, 1, 1, .is_32bit = false},
    {TIM14, RCC_APB1Periph_TIM14, 1, 1, .is_32bit = false},
};

const uint16_t tim_dma_channels[TIM_MAX_NUMBER_CHANNELS] = {
    TIM_DMA_CC1,
    TIM_DMA_CC2,
    TIM_DMA_CC3,
    TIM_DMA_CC4,
};

#elif defined(STM32L4XX)

const tim_hw_item_t tim_hw_items[TIM_AMOUNT] = {
    {TIM1, LL_APB2_GRP1_PERIPH_TIM1, 2, 4, .is_32bit = false},
    {TIM2, LL_APB1_GRP1_PERIPH_TIM2, 1, 4, .is_32bit = true},
    {TIM3, LL_APB1_GRP1_PERIPH_TIM3, 1, 4, .is_32bit = false},
    {TIM4, LL_APB1_GRP1_PERIPH_TIM4, 1, 4, .is_32bit = false},
    {TIM5, LL_APB1_GRP1_PERIPH_TIM5, 1, 4, .is_32bit = true},
    {TIM6, LL_APB1_GRP1_PERIPH_TIM6, 1, 4, .is_32bit = false},
    {TIM7, LL_APB1_GRP1_PERIPH_TIM7, 1, 4, .is_32bit = false},
    {TIM8, LL_APB2_GRP1_PERIPH_TIM8, 2, 4, .is_32bit = false},
    //    todo there is currently a problem with the array indices
    //    { TIM15, RCC_APB1Periph_TIM14, 1, 1, .is_32bit = false },
    //    { TIM16, RCC_APB1Periph_TIM14, 1, 1, .is_32bit = false },
    //    { TIM17, RCC_APB1Periph_TIM14, 1, 1, .is_32bit = false },
};
#endif

void tim_rcc_cmd(uint8_t tim_number, bool new_state)
{
  hal_assert((tim_number > 0) && (tim_number <= TIM_AMOUNT));

  tim_hw_item_t tim_hw_item = tim_hw_items[tim_number - 1];

  if (tim_hw_item.rcc_apb_number == 1)
  {
#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
    RCC_APB1PeriphClockCmd(tim_hw_item.rcc_apb_line, new_state);
#elif defined(STM32L4XX)
    LL_APB1_GRP1_EnableClock(tim_hw_item.rcc_apb_line);
#endif
  }
  else if (tim_hw_item.rcc_apb_number == 2)
  {
#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
    RCC_APB2PeriphClockCmd(tim_hw_item.rcc_apb_line, new_state);
#elif defined(STM32L4XX)
    if (new_state)
    {
      LL_APB2_GRP1_EnableClock(tim_hw_item.rcc_apb_line);
    }
    else
    {
      LL_APB2_GRP1_DisableClock(tim_hw_item.rcc_apb_line);
    }
#endif
  }
  else
  {
    hal_assert(false);
  }
}

#if defined(STM32F4XX)

/**
 * @brief Get the timer clock frequency
 * @param tim_number - timer number
 * @return Clock frequency, [Hz]
 */
uint32_t tim_get_tim_frequency(uint8_t tim_number)
{
  RCC_ClocksTypeDef rcc_clocks;
  RCC_GetClocksFreq(&rcc_clocks);

  uint32_t presc;
  uint32_t frequency;

  if (tim_hw_items[tim_number - 1].rcc_apb_number == 1)
  {
    frequency = rcc_clocks.PCLK1_Frequency;
    presc = RCC->CFGR & RCC_CFGR_PPRE1;

    if (presc != RCC_CFGR_PPRE1_DIV1)
    {
      frequency *= 2;
    }
  }
  else if (tim_hw_items[tim_number - 1].rcc_apb_number == 2)
  {
    frequency = rcc_clocks.PCLK2_Frequency;
    presc = RCC->CFGR & RCC_CFGR_PPRE2;

    if (presc != RCC_CFGR_PPRE2_DIV1)
    {
      frequency *= 2;
    }
  }
  else
  {
    frequency = 0;
  }

  return frequency;
}

/**
 * @brief Calculate the timer prescaler from the desired timer clock frequency
 * @note The bus frequency must be a multiple of the desired timer clock frequency
 * @param tim_number - timer number
 * @param desired_frequency - desired timer clock frequency, [Hz]
 * @param prescaler - pointer to the variable that receives the prescaler
 * @return ERR_OK - on success, otherwise - error
 */
error_t tim_calc_prescaler(uint8_t tim_number, uint32_t desired_frequency, uint16_t *prescaler)
{
  error_t result = ERR_RESULT;
  *prescaler = 0;

  uint32_t temp_prescaler;
  uint32_t frequency = tim_get_tim_frequency(tim_number);

  if (frequency % desired_frequency == 0)
  {
    temp_prescaler = frequency / desired_frequency - 1;

    if (temp_prescaler <= TIM_MAX_VALUE_PRESCALER)
    {
      *prescaler = temp_prescaler;
      result = ERR_OK;
    }
  }

  return result;
}

/**
 * @brief Initialize a timer output compare channel
 * @param tim_number - timer number
 * @param channel_number - compare channel number
 * @param init_struct - pointer to the settings structure
 */
void tim_oc_init(uint8_t tim_number, uint8_t channel_number, TIM_OCInitTypeDef *init_struct)
{
  hal_assert((tim_number > 0) && (tim_number <= TIM_AMOUNT));
  hal_assert((channel_number > 0) &&
             (channel_number <= tim_hw_items[tim_number - 1].channel_amount));
  hal_assert(init_struct != 0);

  switch (channel_number)
  {
    case 1:
      TIM_OC1Init(tim_hw_items[tim_number - 1].timer, init_struct);
      break;

    case 2:
      TIM_OC2Init(tim_hw_items[tim_number - 1].timer, init_struct);
      break;

    case 3:
      TIM_OC3Init(tim_hw_items[tim_number - 1].timer, init_struct);
      break;

    case 4:
      TIM_OC4Init(tim_hw_items[tim_number - 1].timer, init_struct);
      break;
  }

  // For advanced timers the OC outputs are controlled by a separate register TIMx->BDTR,
  // provided the corresponding CCxE, CCxNE bits are set in TIMx->CCER
  if (TIM_IS_ADVANCED(tim_number))
  {
    TIM_CtrlPWMOutputs(tim_hw_items[tim_number - 1].timer, ENABLE);
  }
}

/**
 * @brief Enable the preload register
 * @param tim_number - timer number
 * @param channel_number - compare channel number
 * @param state - preload register state
 */
void tim_oc_preload_config(uint8_t tim_number, uint8_t channel_number, bool state)
{
  hal_assert((tim_number > 0) && (tim_number <= TIM_AMOUNT));
  hal_assert((channel_number > 0) &&
             (channel_number <= tim_hw_items[tim_number - 1].channel_amount));

  uint16_t oc_preload;

  #if defined(STM32F4XX)
  if (state)
  {
    oc_preload = TIM_OCPreload_Enable;
  }
  else
  {
    oc_preload = TIM_OCPreload_Disable;
  }
  #else
  hal_assert(false);
  #endif

  switch (channel_number)
  {
    case 1:
      TIM_OC1PreloadConfig(tim_hw_items[tim_number - 1].timer, oc_preload);
      break;

    case 2:
      TIM_OC2PreloadConfig(tim_hw_items[tim_number - 1].timer, oc_preload);
      break;

    case 3:
      TIM_OC3PreloadConfig(tim_hw_items[tim_number - 1].timer, oc_preload);
      break;

    case 4:
      TIM_OC4PreloadConfig(tim_hw_items[tim_number - 1].timer, oc_preload);
      break;
  }
}
#endif // #if defined(STM32F4XX)
