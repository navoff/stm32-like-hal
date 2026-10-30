#pragma once

/**
 * @file config.h
 * @brief Project configuration for the HAL test builds.
 */

/**
 * @defgroup HW_TYPE_LIST
 * Board types (identifiers).
 * @{
 */
#define HW_TYPE__STM32F0XX_HAL_TESTING 1
#define HW_TYPE__STM32F1XX_HAL_TESTING 2
#define HW_TYPE__STM32F30X_HAL_TESTING 3
#define HW_TYPE__STM32F4XX_HAL_TESTING 4
#define HW_TYPE__STM32L4XX_HAL_TESTING 5
/** @} */

#define PLATFORM_TYPE "HAL_TESTING"

#if !defined(FW_VERSION)
  #define FW_VERSION "undefined"
#endif

#if defined(DEBUG)
  #define HAL_ASSERT_ENABLE 1
#elif defined(RELEASE)
  #define HAL_ASSERT_ENABLE 0
#else
  #error The build type must be specified in the project settings for this build target!
#endif

#if (HW_TYPE == HW_TYPE__STM32F0XX_HAL_TESTING)
  #include "config_stm32f0xx_hal_testing.h"
  #include "stm32f0xx.h"
  #include "stm32f0xx_conf.h"

#elif (HW_TYPE == HW_TYPE__STM32F1XX_HAL_TESTING)
  #include "config_stm32f1xx_hal_testing.h"
  #include "stm32f10x.h"

#elif (HW_TYPE == HW_TYPE__STM32F30X_HAL_TESTING)
  #include "config_stm32f30x_hal_testing.h"
  #include "stm32f30x.h"

#elif (HW_TYPE == HW_TYPE__STM32F4XX_HAL_TESTING)
  #include "config_stm32f4xx_hal_testing.h"
  #include "stm32f4xx.h"

#elif (HW_TYPE == HW_TYPE__STM32L4XX_HAL_TESTING)
  #include "config_stm32l4xx_hal_testing.h"
  #include "stm32l4xx.h"
  #include "stm32l4xx_conf.h"

#else
  #error "Unknown HW_TYPE"
#endif

#define NVRAM_SIZE 2048
#define NVRAM_CHECKSUM_CRC_ALG CHECKSUM_CRC_ALG_FAST

#include "utils/hal_config.h"
