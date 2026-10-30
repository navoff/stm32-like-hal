#pragma once

/**
 * @file config_template.h
 * @brief Template of the project-level config.h that the HAL expects.
 *
 * Copy this file into your project as "config.h", put its directory on the include path and
 * adjust the values. Every HAL source includes "config.h" and relies on the macros below.
 * The build system must additionally define the MCU family macro (one of STM32F0XX, STM32F1XX,
 * STM32F30X, STM32F4XX, STM32L4XX), HW_TYPE and HSE_VALUE on the compiler command line;
 * see tests/CMakeLists.txt for an example.
 */

/* ---------------------------------------------------------------------------------------------
 * Board identification
 * ------------------------------------------------------------------------------------------- */

/// Board (hardware) type identifiers. The build passes one of them as HW_TYPE.
#define HW_TYPE__MY_BOARD_REV1 1

/// Board modification. Used by application code only, the HAL does not interpret it.
#define HW_MODIFICATION 0

/// Platform name stored in fw_info (up to 31 characters).
#define PLATFORM_TYPE "MY_PLATFORM"

/// Firmware version string stored in fw_info (up to 63 characters).
/// Normally passed from the build system as -DFW_VERSION="..."
#if !defined(FW_VERSION)
  #define FW_VERSION "undefined"
#endif

/// Offset of the vector table from the start of flash (non-zero when a bootloader is used).
#define VECT_TAB_OFFSET 0x00000

/* ---------------------------------------------------------------------------------------------
 * Build type and asserts
 * ------------------------------------------------------------------------------------------- */

/// hal_assert() is compiled in when HAL_ASSERT_ENABLE is 1. Defaults to 1 if not defined.
#if defined(DEBUG)
  #define HAL_ASSERT_ENABLE 1
#elif defined(RELEASE)
  #define HAL_ASSERT_ENABLE 0
#else
  #error Define DEBUG or RELEASE for this build
#endif

/* ---------------------------------------------------------------------------------------------
 * Vendor headers
 *
 * The HAL does not include CMSIS or vendor peripheral headers itself; config.h must do it.
 * ------------------------------------------------------------------------------------------- */

#if defined(STM32F0XX)
  #include "stm32f0xx.h"
  #include "stm32f0xx_conf.h"
#elif defined(STM32F1XX)
  #include "stm32f10x.h"
#elif defined(STM32F30X)
  #include "stm32f30x.h"
#elif defined(STM32F4XX)
  #include "stm32f4xx.h"
#elif defined(STM32L4XX)
  #include "stm32l4xx.h"
  #include "stm32l4xx_conf.h"
#else
  #error Unknown MCU family
#endif

/* ---------------------------------------------------------------------------------------------
 * Peripheral instances used by the project
 *
 * Every FEATURE_ENABLE_* macro must be defined as 0 or 1. A driver only compiles the code and
 * interrupt handlers for the enabled instances. Instances that do not exist on the selected
 * MCU family must stay 0.
 * ------------------------------------------------------------------------------------------- */

#define FEATURE_ENABLE_ADC1 0
#define FEATURE_ENABLE_ADC2 0
#define FEATURE_ENABLE_ADC3 0
#define FEATURE_ENABLE_ADC4 0

#define FEATURE_ENABLE_DMA0 0 // GD32 only
#define FEATURE_ENABLE_DMA1 1
#define FEATURE_ENABLE_DMA2 0

#define FEATURE_ENABLE_I2C1 0
#define FEATURE_ENABLE_I2C2 0
#define FEATURE_ENABLE_I2C3 0
#define FEATURE_ENABLE_I2C4 0

#define FEATURE_ENABLE_I2S2 0
#define FEATURE_ENABLE_I2S3 0

#define FEATURE_ENABLE_SPI1 0
#define FEATURE_ENABLE_SPI2 0
#define FEATURE_ENABLE_SPI3 0
#define FEATURE_ENABLE_SPI4 0

#define FEATURE_ENABLE_UART0 0 // GD32 only
#define FEATURE_ENABLE_UART1 1
#define FEATURE_ENABLE_UART2 0
#define FEATURE_ENABLE_UART3 0
#define FEATURE_ENABLE_UART4 0
#define FEATURE_ENABLE_UART5 0
#define FEATURE_ENABLE_UART6 0
#define FEATURE_ENABLE_UART7 0
#define FEATURE_ENABLE_UART8 0
#define FEATURE_ENABLE_LPUART 0 // GD32L23x only

#define FEATURE_ENABLE_TIM1 0
#define FEATURE_ENABLE_TIM2 0
#define FEATURE_ENABLE_TIM3 0
#define FEATURE_ENABLE_TIM4 0
#define FEATURE_ENABLE_TIM5 0
#define FEATURE_ENABLE_TIM8 0
#define FEATURE_ENABLE_TIM9 0
#define FEATURE_ENABLE_TIM10 0
#define FEATURE_ENABLE_TIM11 0
#define FEATURE_ENABLE_TIM12 0
#define FEATURE_ENABLE_TIM13 0
#define FEATURE_ENABLE_TIM14 0

#define FEATURE_ENABLE_USB 0

/// Hardware CRC unit (drivers/crc.c). Leave 0, the driver is not built yet.
#define FEATURE_ENABLE_CRC_DRIVER 0

/// RTC time representation: pick at most one.
#define FEATURE_ENABLE_RTC_BCD_FORMAT 0
#define FEATURE_ENABLE_RTC_UINT_FORMAT 0

/* ---------------------------------------------------------------------------------------------
 * NVRAM (settings storage), see docs/nvram.md
 *
 * Pick exactly one backend. The CMake options HAL_NVRAM_INT_FLASH_ENABLE /
 * HAL_NVRAM_EXT_FLASH_ENABLE must match these macros.
 * ------------------------------------------------------------------------------------------- */

/// Settings are stored in the internal flash. The linker script must provide
/// the .nvram region with __nvram_begin / __nvram_end symbols.
#define FEATURE_ENABLE_INT_NVRAM 1

/// Settings are stored in an external SPI NOR flash: M25Pxx-like (xx25) or N25Qxx-like (xx45).
#define FEATURE_ENABLE_EXT_NOR_XX25_NVRAM 0
#define FEATURE_ENABLE_EXT_NOR_XX45_NVRAM 0

/// Size of one NVRAM segment in bytes, including the 10-byte system trailer.
/// Internal flash: must fit into one flash page and be a multiple of 8.
/// External flash: must be a multiple of 256.
#define NVRAM_SIZE 1024

/// CRC implementation used for the NVRAM segment checksum:
/// CHECKSUM_CRC_ALG_SLOW (bitwise), CHECKSUM_CRC_ALG_FAST (table), CHECKSUM_CRC_ALG_HW.
#define NVRAM_CHECKSUM_CRC_ALG CHECKSUM_CRC_ALG_FAST

/* ---------------------------------------------------------------------------------------------
 * Optional driver knobs (defaults shown)
 * ------------------------------------------------------------------------------------------- */

/// Measure the real LSI frequency before configuring the independent watchdog.
// #define IWDG_LSI_FREQ_MEASURE_EN 0

/// Compiler-specific macros (critical sections, NO_INIT). Keep this include last.
#include "utils/hal_config.h"
