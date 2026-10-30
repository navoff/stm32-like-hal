#pragma once

/**
 * @file hal_config.h
 * @brief Toolchain-specific critical section and no-init variable macros.
 */

#if defined(__GNUC__)
  /// Enter a critical section, saving the interrupt state into x (uint32_t)
  #define ENTER_CRITICAL(x)                                                                        \
    x = __get_PRIMASK();                                                                           \
    __disable_irq()
  /// Leave a critical section, restoring the interrupt state saved by ENTER_CRITICAL
  #define LEAVE_CRITICAL(x)                                                                        \
    if (!x)                                                                                        \
    __enable_irq()

  /// Marks variables that must not be initialized at program start (.noinit section)
  #define NO_INIT __attribute__((section(".noinit")))
#else
  #error Only GCC (arm-none-eabi-gcc) is supported
#endif
