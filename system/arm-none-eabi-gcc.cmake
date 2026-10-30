# ARM GCC Toolchain file for STM32 microcontrollers
# This file configures CMake to use the ARM embedded toolchain

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross-compiler
# You can override this by setting ARM_TOOLCHAIN_PATH environment variable
if(DEFINED ENV{ARM_TOOLCHAIN_PATH})
    set(ARM_TOOLCHAIN_PATH $ENV{ARM_TOOLCHAIN_PATH})
else()
    # Try to find arm-none-eabi-gcc in PATH
    find_program(ARM_GCC_COMPILER arm-none-eabi-gcc)
    if(ARM_GCC_COMPILER)
        get_filename_component(ARM_TOOLCHAIN_PATH ${ARM_GCC_COMPILER} DIRECTORY)
    else()
        # Default paths for common installations
        if(WIN32)
            set(ARM_TOOLCHAIN_PATH "C:/Program Files (x86)/GNU Arm Embedded Toolchain/bin")
        elseif(APPLE)
            set(ARM_TOOLCHAIN_PATH "/usr/local/bin")
        else()
            set(ARM_TOOLCHAIN_PATH "/usr/bin")
        endif()
    endif()
endif()

# Set the toolchain prefix
set(TOOLCHAIN_PREFIX "arm-none-eabi-")

# Set compiler paths
set(CMAKE_C_COMPILER   "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}g++")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_AR           "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}ar")
set(CMAKE_RANLIB       "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}ranlib")
set(CMAKE_SIZE         "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}size")
set(CMAKE_OBJCOPY      "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}objcopy")
set(CMAKE_OBJDUMP      "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}objdump")
set(CMAKE_NM           "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}nm")
set(CMAKE_STRIP        "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}strip")
set(CMAKE_GDB          "${ARM_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}gdb")

# Check if compiler exists
if(NOT EXISTS ${CMAKE_C_COMPILER})
    message(FATAL_ERROR "ARM GCC compiler not found at: ${CMAKE_C_COMPILER}")
endif()

# Set compiler flags for CMake tests
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set common ARM Cortex-M4/M3/M0 flags
if(MCU_FAMILY STREQUAL "STM32F0XX")
  set(ARM_CPU_FLAGS "-mcpu=cortex-m0 -mthumb -mfloat-abi=soft")
elseif(MCU_FAMILY STREQUAL "STM32F1XX")
  set(ARM_CPU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")
elseif(MCU_FAMILY STREQUAL "STM32F30X")
  set(ARM_CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
elseif(MCU_FAMILY STREQUAL "STM32F4XX")
  set(ARM_CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
elseif(MCU_FAMILY STREQUAL "STM32L4XX")
  set(ARM_CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
endif()

# Set default compiler flags for embedded systems (STM32F3 Cortex-M4)
set(CMAKE_C_FLAGS_INIT "${ARM_CPU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${ARM_CPU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${ARM_CPU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${ARM_CPU_FLAGS} -specs=nano.specs")

# Print toolchain information
message(STATUS "ARM Toolchain Configuration:")
message(STATUS "  Toolchain path: ${ARM_TOOLCHAIN_PATH}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Size utility: ${CMAKE_SIZE}")
message(STATUS "  Objcopy utility: ${CMAKE_OBJCOPY}")
