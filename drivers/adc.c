/**
 * @file adc.c
 * @brief ADC peripheral table and ADC clock control.
 */

#include "config.h"

#include <string.h>

#include "utils/assert.h"
#include "utils/macros.h"

#include "drivers/adc.h"

#if !defined(FEATURE_ENABLE_ADC1)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_ADC1 0
#endif

#if !defined(FEATURE_ENABLE_ADC2)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_ADC2 0
#endif

#if !defined(FEATURE_ENABLE_ADC3)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_ADC3 0
#endif

#if !defined(FEATURE_ENABLE_ADC4)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_ADC4 0
#endif

#define ADC_AMOUNT 4

const adc_hw_item_t adc_hw_items[ADC_AMOUNT] = {
#if (FEATURE_ENABLE_ADC1)
    {
        .adc = ADC1,
    },
#else
    {.adc = NULL},
#endif

#if (FEATURE_ENABLE_ADC2)
    {
        .adc = ADC2,
    },
#else
    {.adc = NULL},
#endif

#if (FEATURE_ENABLE_ADC3)
    {
        .adc = ADC3,
    },
#else
    {.adc = NULL},
#endif

#if (FEATURE_ENABLE_ADC4)
    {
        .adc = ADC4,
    },
#else
    {.adc = NULL},
#endif
};

void adc_rcc_cmd(uint8_t adc_number, bool new_state)
{
  hal_assert((adc_number > 0) && (adc_number <= ADC_AMOUNT));

#if defined(STM32F1XX) || defined(STM32F4XX)
  switch (adc_number)
  {
    case 1:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, new_state);
      break;
    case 2:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, new_state);
      break;
    case 3:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, new_state);
      break;
  }

#elif defined(STM32F30X)
  switch (adc_number)
  {
    case 1:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC12, new_state);
      break;
    case 2:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC12, new_state);
      break;
    case 3:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC34, new_state);
      break;
    case 4:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC34, new_state);
      break;
  }

#elif defined(STM32L4XX)
  HAL_UNUSED(adc_number);
  if (new_state)
  {
    __HAL_RCC_ADC_CLK_ENABLE();
  }
  else
  {
    __HAL_RCC_ADC_CLK_DISABLE();
  }
#else
  #error This MCU not supported
#endif
}
