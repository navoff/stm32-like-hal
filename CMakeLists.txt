cmake_minimum_required(VERSION 3.20)

# HAL for MCUs
# This CMakeLists.txt handles compilation of HAL for MCU

# Options

# NVRAM drivers are selected per project, in sync with FEATURE_ENABLE_INT_NVRAM and
# FEATURE_ENABLE_EXT_NOR_XX25/XX45_NVRAM in the project's config.h.
option(HAL_NVRAM_INT_FLASH_ENABLE "Enable NVRAM in internal flash" OFF)
option(HAL_NVRAM_EXT_FLASH_ENABLE "Enable NVRAM in external NOR flash (xx25/xx45)" OFF)

# Sources and includes

if (DEFINED HAL_INCLUDES)
  set(HAL_ADD_INCLUDES ${HAL_INCLUDES})
endif()

set(HAL_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/protocols
  ${CMAKE_CURRENT_SOURCE_DIR}/usb
  ${CMAKE_CURRENT_SOURCE_DIR}/mems
)

set(HAL_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/adc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/can.c
  # ${CMAKE_CURRENT_SOURCE_DIR}/drivers/crc.c # does not supported yet
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/dma.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/ext_flash_hw.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/ext_flash_xx25.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/ext_flash_xx45.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/gpio_stm32.c
  # ${CMAKE_CURRENT_SOURCE_DIR}/drivers/i2c_test.c
  # ${CMAKE_CURRENT_SOURCE_DIR}/drivers/i2c.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/int_flash.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/iwdg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/mcu.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/rtc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/spi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/tick_timer.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/tim.c # TODO need to refactor
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart/uart.c
  ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart/uart_core_stm32.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/assert.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/checksum_test.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/checksum.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/fifo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/lp_filter.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/unit_tests.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.c
  ${CMAKE_CURRENT_SOURCE_DIR}/version/fw_info.c
)

# nvram_settings_template.c is not built here: every project provides its own nvram_settings.c
if (HAL_NVRAM_INT_FLASH_ENABLE OR HAL_NVRAM_EXT_FLASH_ENABLE)
  list(APPEND HAL_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/nvram
  )

  list(APPEND HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/nvram/nvram.c
  )
endif()

if (HAL_NVRAM_INT_FLASH_ENABLE)
  list(APPEND HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/nvram/nvram_int_flash.c
  )
endif()

if (HAL_NVRAM_EXT_FLASH_ENABLE)
  list(APPEND HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/nvram/nvram_ext_flash.c
  )
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/std_periph_lib std_periph_lib_build)

if(MCU_FAMILY STREQUAL "STM32F0XX")

  # does not supported yet
  list(REMOVE_ITEM HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/int_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nvram/nvram_int_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart/uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart/uart_core_stm32.c
  )

elseif(MCU_FAMILY STREQUAL "STM32F1XX")
  # does not supported yet
  list(REMOVE_ITEM HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/can.c
  )

elseif(MCU_FAMILY STREQUAL "STM32F30X")

  # does not supported yet; ext_flash and external NVRAM need the spi driver
  list(REMOVE_ITEM HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/ext_flash_hw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/ext_flash_xx25.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/ext_flash_xx45.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nvram/nvram_ext_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart/uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart/uart_core_stm32.c
  )

elseif(MCU_FAMILY STREQUAL "STM32F4XX")

  # does not supported yet
  list(REMOVE_ITEM HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/rtc.c
  )

elseif(MCU_FAMILY STREQUAL "STM32L4XX")

  # does not supported yet
  list(REMOVE_ITEM HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/rtc.c
  )

else()
    message(FATAL_ERROR "Unknown MCU_FAMILY: ${MCU_FAMILY}")
endif()

# Create static library target

add_library(hal_lib STATIC ${HAL_SOURCES})

target_include_directories(hal_lib PUBLIC ${HAL_INCLUDES} ${HAL_ADD_INCLUDES})
target_compile_definitions(hal_lib PUBLIC ${HAL_DEFINITIONS})
target_link_libraries(hal_lib PUBLIC std_periph_lib)

# One section per function and object, so that a firmware linked with --gc-sections keeps
# only the drivers and functions it calls
target_compile_options(hal_lib PRIVATE
  -Wall -Wextra -Werror
  -ffunction-sections -fdata-sections
)

if(DEFINED CONFIG_FLAGS)
    target_compile_options(hal_lib PRIVATE ${CONFIG_FLAGS})
endif()

message(STATUS "HAL Library configured for ${MCU_FAMILY}")
