cmake_minimum_required(VERSION 3.20)

# Standard Peripheral Library for MCUs
# This CMakeLists.txt handles compilation of MCU-specific standard libraries

# Sources and includes

set(STD_PERIPH_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Core/Include
)

if(DEFINED STD_PERIPH_DEFINITIONS)
    set(STD_PERIPH_ADD_DEFINITIONS ${STD_PERIPH_DEFINITIONS})
endif()

if(MCU_FAMILY STREQUAL "STM32F0XX")
  set(STD_PERIPH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_comp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_crs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Src/stm32f0xx_ll_utils.c
  )

  list(APPEND STD_PERIPH_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx/STM32F0xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f0xx
  )

  set(STD_PERIPH_DEFINITIONS
    USE_FULL_LL_DRIVER
  )

elseif(MCU_FAMILY STREQUAL "STM32F1XX")
  set(STD_PERIPH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_cec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_dbgmcu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_fsmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_sdio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/src/stm32f10x_wwdg.c
  )

  list(APPEND STD_PERIPH_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/CMSIS/CM3/DeviceSupport/ST/STM32F10x
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx/STM32F10x_StdPeriph_Driver/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f1xx
  )

  set(STD_PERIPH_DEFINITIONS
    USE_STDPERIPH_DRIVER
  )

elseif(MCU_FAMILY STREQUAL "STM32F30X")
  set(STD_PERIPH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_comp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_dbgmcu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_fmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_hrtim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_opamp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_syscfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/src/stm32f30x_wwdg.c
  )

  list(APPEND STD_PERIPH_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/CMSIS/Device/ST/STM32F30x/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x/STM32F30x_StdPeriph_Driver/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f30x
  )

  set(STD_PERIPH_DEFINITIONS
    USE_STDPERIPH_DRIVER
  )

elseif(MCU_FAMILY STREQUAL "STM32F4XX")
  set(STD_PERIPH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_cec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_cryp_aes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_cryp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_cryp_des.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_cryp_tdes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dbgmcu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dcmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dfsdm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma2d.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dsi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_flash_ramfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_fmpi2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_fsmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_hash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_hash_md5.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_hash_sha1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_lptim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_ltdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_qspi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rng.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_sai.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_sdio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_spdifrx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_syscfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_wwdg.c
  )

  list(APPEND STD_PERIPH_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx/STM32F4xx_StdPeriph_Driver/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx
  )

  set(STD_PERIPH_DEFINITIONS
    USE_STDPERIPH_DRIVER
  )

elseif(MCU_FAMILY STREQUAL "STM32L4XX")
  set(STD_PERIPH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_comp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_crc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_crs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_dma2d.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_lptim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_lpuart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_opamp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_pka.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_rng.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_swpmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_ll_utils.c
    # Cube HAL subset used by drivers/int_flash.c (see stm32l4xx/stm32l4xx_hal_conf.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr_ex.c
  )

  list(APPEND STD_PERIPH_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx/STM32L4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32l4xx
  )

  set(STD_PERIPH_DEFINITIONS
    USE_FULL_LL_DRIVER
  )

else()
  message(FATAL_ERROR "Unknown MCU_FAMILY: ${MCU_FAMILY}")
endif()

# Create static library target

add_library(std_periph_lib STATIC ${STD_PERIPH_SOURCES})

# Set target properties
target_include_directories(std_periph_lib PUBLIC ${STD_PERIPH_INCLUDES})
target_compile_definitions(std_periph_lib PUBLIC ${STD_PERIPH_DEFINITIONS} ${STD_PERIPH_ADD_DEFINITIONS})

# One section per function and object, so that a firmware linked with --gc-sections keeps
# only the vendor functions it calls
target_compile_options(std_periph_lib PRIVATE
  -Wall -Wextra -Werror
  -Wno-unused-parameter
  -Wno-implicit-fallthrough
  -ffunction-sections -fdata-sections
)

if(DEFINED CONFIG_FLAGS)
  target_compile_options(std_periph_lib PRIVATE ${CONFIG_FLAGS})
endif()

message(STATUS "Standard Peripheral Library configured for ${MCU_FAMILY}")
