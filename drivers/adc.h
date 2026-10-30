#pragma once

/**
 * @file adc.h
 * @brief ADC peripheral table and ADC clock control.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

typedef struct
{
  ADC_TypeDef *adc;
} adc_hw_item_t;

/**
 * @brief Enable/disable ADC clock
 * @param adc_number - ADC number
 * @param new_state - new state: true - enable, false - disable
 */
void adc_rcc_cmd(uint8_t adc_number, bool new_state);

extern const adc_hw_item_t adc_hw_items[];

#ifdef __cplusplus
}
#endif
