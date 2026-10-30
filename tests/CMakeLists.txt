cmake_minimum_required(VERSION 3.20)

project(stm32_like_hal
  VERSION 1.0.0
  DESCRIPTION "HAL for STM32 MCU"
  LANGUAGES C ASM
)

# Always export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# ----------------------------------------------------------------------
# Languages / standards
# ----------------------------------------------------------------------
enable_language(ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Assemble with preprocessor
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")

# ----------------------------------------------------------------------
# Global compile definitions (keep CPU/FPU/ABI in toolchain)
# ----------------------------------------------------------------------
add_compile_definitions(
  ${MCU_FAMILY}
  HSE_VALUE=8000000
  HW_TYPE=${HW_TYPE}
)

# ----------------------------------------------------------------------
# Common flags (do not duplicate CPU/FPU/ABI or -specs here — toolchain handles it)
# ----------------------------------------------------------------------
set(COMMON_FLAGS
  -Wall -Wextra -Werror
  -ffunction-sections -fdata-sections
  -fno-common
  -fmessage-length=0
)

# Config-specific flags for multi-config generators
set(CONFIG_FLAGS
  $<$<CONFIG:Debug>:-O0 -g3 -Werror -DDEBUG>
  $<$<CONFIG:Release>:-O2 -DRELEASE>
)

# Output directories per configuration (Debug/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

# Sources and includes
set(HAL_INCLUDES
  ${PROJECT_ROOT}/tests
)

set(HAL_NVRAM_INT_FLASH_ENABLE ON)
set(HAL_NVRAM_EXT_FLASH_ENABLE ON)

if(MCU_FAMILY STREQUAL "STM32F0XX")
  set(MCU_FAMILY_SOURCES
    ${PROJECT_ROOT}/std_periph_lib/stm32f0xx/CMSIS/Source/Templates/system_stm32f0xx.c
    ${PROJECT_ROOT}/std_periph_lib/stm32f0xx/CMSIS/Source/Templates/gcc/startup_stm32f072xb.s
  )

  set(STD_PERIPH_DEFINITIONS
    STM32F072xB
  )

  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/system/stm32f072xb.ld")

elseif(MCU_FAMILY STREQUAL "STM32F1XX")
  set(MCU_FAMILY_SOURCES
    ${PROJECT_ROOT}/std_periph_lib/stm32f1xx/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c
    ${PROJECT_ROOT}/std_periph_lib/stm32f1xx/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/gcc_ride7/startup_stm32f10x_md.s
  )

  set(STD_PERIPH_DEFINITIONS
    STM32F10X_MD
  )

  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/system/stm32f103xb.ld")

elseif(MCU_FAMILY STREQUAL "STM32F30X")
  set(MCU_FAMILY_SOURCES
    ${PROJECT_ROOT}/std_periph_lib/stm32f30x/CMSIS/Device/ST/STM32F30x/Source/Templates/system_stm32f30x.c
    ${PROJECT_ROOT}/std_periph_lib/stm32f30x/CMSIS/Device/ST/STM32F30x/Source/Templates/gcc_ride7/startup_stm32f30x.s
  )

  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/system/stm32f303xb.ld")

elseif(MCU_FAMILY STREQUAL "STM32F4XX")

  set(MCU_FAMILY_SOURCES
    ${PROJECT_ROOT}/std_periph_lib/stm32f4xx/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
    ${PROJECT_ROOT}/std_periph_lib/stm32f4xx/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f40_41xxx.s
  )

  set(STD_PERIPH_DEFINITIONS
    STM32F40_41xxx
  )
  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/system/stm32f407xg.ld")

elseif(MCU_FAMILY STREQUAL "STM32L4XX")
  set(MCU_FAMILY_SOURCES
    ${PROJECT_ROOT}/std_periph_lib/stm32l4xx/CMSIS/Source/Templates/system_stm32l4xx.c
    ${PROJECT_ROOT}/std_periph_lib/stm32l4xx/CMSIS/Source/Templates/gcc/startup_stm32l476xx.s
  )

  set(STD_PERIPH_DEFINITIONS
    STM32L476xx
  )

  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/system/stm32l476xg.ld")
else()
  message(FATAL_ERROR "Unknown MCU_FAMILY")
endif()

set(SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/main.c
  ${MCU_FAMILY_SOURCES}
  ${PROJECT_ROOT}/nvram/nvram_settings_template.c
)

add_subdirectory(${PROJECT_ROOT} hal_build)

# ----------------------------------------------------------------------
# Target
# ----------------------------------------------------------------------
set(TARGET_NAME firmware)
set(OUTPUT_BASENAME "${PROJECT_NAME}")

add_executable(${TARGET_NAME} ${SOURCES})
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "${OUTPUT_BASENAME}")

target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_DIRS})
target_compile_options(${TARGET_NAME} PRIVATE ${COMMON_FLAGS} ${CONFIG_FLAGS})

# Link the whole archive so that every driver object is linked and checked for
# unresolved symbols, not only the ones referenced from main.c
target_link_libraries(${TARGET_NAME} PRIVATE
  -Wl,--whole-archive hal_lib -Wl,--no-whole-archive
)

# Link options (CPU/FPU/ABI and -specs must be in the toolchain)
target_link_options(${TARGET_NAME} PRIVATE
  -T${LINKER_SCRIPT}
  -Wl,-Map=${OUTPUT_BASENAME}.map,--cref
  # No --gc-sections on purpose: together with --whole-archive this makes the linker
  # resolve every symbol of every driver, so a missing definition fails the build
  # stub syscalls for the newlib stdio used by some drivers (sscanf/sprintf)
  -specs=nosys.specs
  -Wl,--start-group
  -lc -lm
  -Wl,--end-group
  -Wl,--print-memory-usage
)

# ----------------------------------------------------------------------
# Summary (note: CMAKE_BUILD_TYPE is empty for multi-config generators)
# ----------------------------------------------------------------------
message(STATUS "")
message(STATUS "Build summary (Static Library):")
message(STATUS "  MCU Family:        ${MCU_FAMILY}")
message(STATUS "  Available configs: ${CMAKE_CONFIGURATION_TYPES}")
message(STATUS "  Target:            ${TARGET_NAME} (static library)")
message(STATUS "")
