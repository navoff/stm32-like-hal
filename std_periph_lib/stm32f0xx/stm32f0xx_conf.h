#pragma once

/**
 * @file stm32f0xx_conf.h
 * @brief
 */

#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_adc.h"
#include "stm32f0xx_ll_comp.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_crc.h"
#include "stm32f0xx_ll_crs.h"
#include "stm32f0xx_ll_dac.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_i2c.h"
#include "stm32f0xx_ll_iwdg.h"
#include "stm32f0xx_ll_rtc.h"
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_tim.h"
#include "stm32f0xx_ll_usart.h"
#include "stm32f0xx_ll_wwdg.h"

#define HAL_RCC_MODULE_ENABLED
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f0xx_hal_rcc.h"
#endif

#define HAL_FLASH_MODULE_ENABLED
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f0xx_hal_flash.h"
#endif

// ======================================= Definition ==============================================

// ======================================= Declaration =============================================
