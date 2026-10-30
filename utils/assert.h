#pragma once

/**
 * @file assert.h
 * @brief Assertion utilities for runtime validation of conditions
 *
 * This file provides assertion macros for runtime validation of conditions.
 * When an assertion fails, the program will halt execution and provide
 * debugging information about the failure location.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"
#include <stdint.h>

#if !defined(HAL_ASSERT_ENABLE)
  #define HAL_ASSERT_ENABLE 1
#endif

#if (HAL_ASSERT_ENABLE)
  /**
   * @brief Assert that a condition is true
   *
   * If the condition evaluates to false, the program will call @ref hal_assert_handler
   * with information about the failed assertion, including the expression, file name,
   * and line number where the assertion failed.
   *
   * @param expr The condition to check
   */
  #define hal_assert(expr) ((expr) ? (void)0 : hal_assert_handler(#expr, __FILE__, __LINE__))

#else
  /**
   * @brief Assert that a condition is true (disabled version)
   *
   * This version of hal_assert does nothing when HAL_ASSERT_ENABLE is 0.
   *
   * @param expr The condition to check (not evaluated when assertions are disabled)
   */
  #define hal_assert(expr) ((void)0)
#endif

#if defined(__GNUC__) // GCC
  #if defined(STM32F0XX)
    #define HALT_IF_DEBUGGING()                                                                    \
      {                                                                                            \
      }
  #else
    /**
     * @brief Halt execution if a debugger is attached
     *
     * This macro checks if a debugger is attached and, if so, triggers a breakpoint.
     * This is useful for debugging assertion failures.
     */
    #define HALT_IF_DEBUGGING()                                                                    \
      do                                                                                           \
      {                                                                                            \
        if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)                                      \
        {                                                                                          \
          __asm("bkpt 1");                                                                         \
        }                                                                                          \
      } while (0)
  #endif

  /**
   * @brief Static assertion for compile-time validation
   *
   * This macro checks a condition at compile time and generates a compiler error
   * if the condition is false.
   *
   * @param cond The condition to check at compile time
   * @param msg The error message to display if the condition is false
   */
  #define HAL_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg);
#else
  #error Toolchain not supported
#endif

/**
 * @brief Handler function called when an assertion fails
 *
 * This function is called when an assertion fails. It provides information about
 * the failed assertion, including the expression, file name, and line number.
 * The function will halt execution after reporting the failure.
 *
 * @param expr The expression that failed
 * @param file The file where the assertion failed
 * @param line The line number where the assertion failed
 */
void hal_assert_handler(char const *expr, char const *file, uint32_t line);

#ifdef __cplusplus
}
#endif
