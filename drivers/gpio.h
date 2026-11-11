#pragma once

/**
 * @file gpio.h
 * @brief GPIO port, pin and external interrupt (EXTI) control.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/// Undefined (invalid) port index value
#define GPIO_IDX_UNDEF 0xFF

/// Undefined (invalid) pin index value
#define GPIO_PIN_IDX_UNDEF 0xFF

#define GPIO_PIN_AMOUNT 16

typedef enum
{
  GPIO_MODE_OUTPUT_PP = 0,
  GPIO_MODE_OUTPUT_OD,
#if defined(STM32F1XX)
  GPIO_MODE_USART1,
  GPIO_MODE_USART2,
  GPIO_MODE_USB,
  GPIO_MODE_IN_AN,
  GPIO_MODE_SPI1,
  GPIO_MODE_SPI2,

#elif defined(STM32F30X)
  GPIO_MODE_USART1,
  GPIO_MODE_USART2,
  GPIO_MODE_USART3,
  GPIO_MODE_UART4,
  GPIO_MODE_UART5,
  GPIO_MODE_USB,

#elif defined(STM32F4XX)
  GPIO_MODE_USART1,
  GPIO_MODE_USART2,
  GPIO_MODE_IN_AN,

  GPIO_MODE_USART3,
  GPIO_MODE_UART4,
  GPIO_MODE_UART5,
  GPIO_MODE_USART6,
  GPIO_MODE_UART7,
  GPIO_MODE_UART8,

  GPIO_MODE_CAN1,
  GPIO_MODE_CAN2,

  GPIO_MODE_SPI1,
  GPIO_MODE_SPI2,
  GPIO_MODE_SPI3,
  GPIO_MODE_SPI4,

  GPIO_MODE_TIM1,
  GPIO_MODE_TIM2,
  GPIO_MODE_TIM3,
  GPIO_MODE_TIM4,
  GPIO_MODE_TIM5,
  GPIO_MODE_TIM8,
  GPIO_MODE_TIM9,
  GPIO_MODE_TIM10,
  GPIO_MODE_TIM11,
  GPIO_MODE_TIM12,
  GPIO_MODE_TIM13,
  GPIO_MODE_TIM14,

  GPIO_MODE_I2C1,
  GPIO_MODE_I2C2,
  GPIO_MODE_I2C3,

#elif defined(STM32L4XX)
  GPIO_MODE_USART1,
  GPIO_MODE_USART2,
  GPIO_MODE_IN_AN,

  GPIO_MODE_USART3,
  GPIO_MODE_UART4,
  GPIO_MODE_UART5,

  GPIO_MODE_SPI1,
  GPIO_MODE_SPI2,
  GPIO_MODE_SPI3,

  GPIO_MODE_I2C1,
  GPIO_MODE_I2C2,
  GPIO_MODE_I2C3,

#elif defined(STM32F0XX)
  GPIO_MODE_USART1,
  GPIO_MODE_USART2,
  GPIO_MODE_USART3,
  GPIO_MODE_UART4,
  GPIO_MODE_SPI1,
  GPIO_MODE_SPI2,

  GPIO_MODE_I2C1,
  GPIO_MODE_I2C2,

#elif defined(GD32E11X)
  GPIO_MODE_IN_AN,
  GPIO_MODE_IN_PU,
  GPIO_MODE_IN_PD,

  GPIO_MODE_USART0,
  GPIO_MODE_USART1,
  GPIO_MODE_USART2,
  GPIO_MODE_UART3,
  GPIO_MODE_UART4,

  GPIO_MODE_I2C0,
  GPIO_MODE_I2C1,

#elif defined(GD32L23X)
  GPIO_MODE_IN_AN,
  GPIO_MODE_IN_PU,
  GPIO_MODE_IN_PD,
  GPIO_MODE_IN_FLOAT,

  GPIO_MODE_USART0,
  GPIO_MODE_USART1,
  GPIO_MODE_LPUART,
  GPIO_MODE_UART3,
  GPIO_MODE_UART4,

  GPIO_MODE_I2C0,
  GPIO_MODE_I2C1,
  GPIO_MODE_I2C2,

  GPIO_MODE_CKOUT,

  GPIO_MODE_TIM1,
  GPIO_MODE_TIM2,
#else
  #error This MCU not supported
#endif
  GPIO_MODE_COUNT,
} gpio_pin_mode_t;

typedef enum
{
  GPIO_PIN_PULL__UP = 0,
  GPIO_PIN_PULL__DOWN,
  GPIO_PIN_PULL__NO,
  GPIO_PIN_PULL__COUNT,
} gpio_pin_pull_t;

typedef enum
{
  GPIO_EXTI_EDGE__RISING = 0,
  GPIO_EXTI_EDGE__FALLING,
  GPIO_EXTI_EDGE__BOTH,
  GPIO_EXTI_EDGE__COUNT,
} gpio_exti_edge_t;

/**
 * @brief Enable/disable port clock
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 * @param new_state - new state: true - enable, false - disable
 */
void gpio_rcc_cmd(uint8_t gpio_idx, bool new_state);

/**
 * @brief Reset the port to its default state: input without pull-up/pull-down
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 */
#if !defined(GD32E11X) && !defined(GD32L23X) // TODO need to rename
void gpio_deinit(uint8_t gpio_idx);
#endif

/**
 * @brief Set pin mode
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 * @param mode - new pin mode
 */
void gpio_pin_set_mode(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode);

/**
 * @brief Configure pin as input
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 * @param pin_pull - input pull mode
 */
void gpio_pin_set_in_mode(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_pull_t pin_pull);

/**
 * @brief Set pin output state on/off
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 * @param new_state - new state: true - set (high), false - reset (low)
 */
void gpio_pin_cmd(uint8_t gpio_idx, uint8_t pin_idx, bool new_state);

/**
 * @brief Toggle pin state
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 */
void gpio_pin_toggle(uint8_t gpio_idx, uint8_t pin_idx);

/**
 * @brief Address of the port bit set/reset register (BSRR on STM32, BOP on GD32), for code that
 *        must switch pins faster than gpio_pin_cmd() allows, for example in an interrupt.
 *        Writing (1 << pin_idx) sets the pin, writing (1 << (pin_idx + 16)) resets it; pins
 *        whose bits are zero keep their state.
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 */
volatile uint32_t *gpio_get_bsrr(uint8_t gpio_idx);

/**
 * @brief Read input state
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 * @result true - input is logic high, otherwise - logic low
 */
bool gpio_pin_state(uint8_t gpio_idx, uint8_t pin_idx);

/**
 * @brief Configure input as an external interrupt source
 * @param gpio_idx - port index: GPIOA - 0, GPIOB - 1, etc.
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 * @param pin_pull - input pull mode
 * @param exti_edge - input edge that triggers the interrupt
 * @param need_irq - whether the interrupt will be enabled; if it is not enabled,
 *        the event must be handled by polling with gpio_exti_check_status
 */
void gpio_exti_init(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_pull_t pin_pull,
                    gpio_exti_edge_t exti_edge, bool need_irq);

/**
 * @brief Check whether the external interrupt event is set
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 * @return true - external interrupt event is set, otherwise - not set
 */
bool gpio_exti_check_status(uint8_t pin_idx);

/**
 * @brief Clear the external interrupt event
 * @param pin_idx - pin index: 0..(GPIO_PIN_AMOUNT - 1)
 */
void gpio_exti_clear_status(uint8_t pin_idx);

#ifdef __cplusplus
}
#endif
