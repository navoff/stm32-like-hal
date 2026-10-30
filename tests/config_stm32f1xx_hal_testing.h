#pragma once

/**
 * @file config_stm32f1xx_hal_testing.h
 * @brief Feature configuration for the STM32F1xx HAL test build.
 */

/// Board modifications
/// @note Base modification without changes
#define HW_MOD_0 0

/// Selected board modification
#define HW_MODIFICATION HW_MOD_0

/// Vector table offset
#define VECT_TAB_OFFSET 0x00000

// TODO enable as many peripherals as possible
#define FEATURE_ENABLE_ADC1 1
#define FEATURE_ENABLE_ADC2 0
#define FEATURE_ENABLE_ADC3 0
#define FEATURE_ENABLE_ADC4 0

#define FEATURE_ENABLE_DMA0 0
#define FEATURE_ENABLE_DMA1 1
#define FEATURE_ENABLE_DMA2 0

#define FEATURE_ENABLE_I2S2 0
#define FEATURE_ENABLE_I2S3 0

#define FEATURE_ENABLE_SPI1 1
#define FEATURE_ENABLE_SPI2 1
#define FEATURE_ENABLE_SPI3 0
#define FEATURE_ENABLE_SPI4 0

#define FEATURE_ENABLE_UART1 1
#define FEATURE_ENABLE_UART2 1
#define FEATURE_ENABLE_UART3 0
#define FEATURE_ENABLE_UART4 0
#define FEATURE_ENABLE_UART5 0
#define FEATURE_ENABLE_UART6 0
#define FEATURE_ENABLE_UART7 0
#define FEATURE_ENABLE_UART8 0

#define FEATURE_ENABLE_EXT_NOR_XX25_NVRAM 1
#define FEATURE_ENABLE_EXT_NOR_XX45_NVRAM 0
#define FEATURE_ENABLE_INT_NVRAM 1

#define FEATURE_ENABLE_CRC_DRIVER 0

#define FEATURE_ENABLE_RTC_BCD_FORMAT 1
#define FEATURE_ENABLE_RTC_UINT_FORMAT 0
