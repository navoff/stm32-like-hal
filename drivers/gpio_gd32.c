/**
 * @file gpio_gd32.c
 * @brief GPIO driver implementation for GD32 MCUs.
 */

#include "config.h"

#include "drivers/gpio.h"
#include "utils/assert.h"
#include <stddef.h>

#if defined(GD32E11X)
  #define GPIO_AMOUNT 5

  /// Whether the pin requires remapping
  #define GPIO_IS_IO_NEED_REMAP(gpio_idx, pin_idx)                                                 \
    ((((gpio_idx) == 1) && ((pin_idx) == 3)) || (((gpio_idx) == 1) && ((pin_idx) == 4)) ||         \
     (((gpio_idx) == 0) && ((pin_idx) == 15)))

  /// Whether the pin is USART TX
  #define GPIO_IS_USART_TX(gpio_idx, pin_idx)                                                      \
    ((((gpio_idx) == 0) && ((pin_idx) == 2)) || (((gpio_idx) == 1) && ((pin_idx) == 10)) ||        \
     (((gpio_idx) == 0) && ((pin_idx) == 9)) || (((gpio_idx) == 1) && ((pin_idx) == 6)))
  /// Whether the pin is USART RX
  #define GPIO_IS_USART_RX(gpio_idx, pin_idx)                                                      \
    ((((gpio_idx) == 0) && ((pin_idx) == 3)) || (((gpio_idx) == 1) && ((pin_idx) == 11)) ||        \
     (((gpio_idx) == 0) && ((pin_idx) == 10)) || (((gpio_idx) == 1) && ((pin_idx) == 7)))

  #define GPIO_IS_USART_NEED_REMAP(gpio_idx, pin_idx)                                              \
    ((((gpio_idx) == 1) && ((pin_idx) == 6)) || (((gpio_idx) == 1) && ((pin_idx) == 7)) ||         \
     (((gpio_idx) == 3) && ((pin_idx) == 5)) || (((gpio_idx) == 3) && ((pin_idx) == 6)) ||         \
     (((gpio_idx) == 3) && ((pin_idx) == 8)) || (((gpio_idx) == 3) && ((pin_idx) == 9)) ||         \
     (((gpio_idx) == 2) && ((pin_idx) == 10)) || (((gpio_idx) == 2) && ((pin_idx) == 11)))

  #define GPIO_IS_I2C_NEED_REMAP(gpio_idx, pin_idx)                                                \
    ((((gpio_idx) == 1) && ((pin_idx) == 8)) || (((gpio_idx) == 1) && ((pin_idx) == 9)))
#elif defined(GD32L23X)
  #define GPIO_AMOUNT 6

  /// Whether the pin is USART TX
  #define GPIO_IS_USART_TX(gpio_idx, pin_idx)                                                      \
    ((((gpio_idx) == 0) && ((pin_idx) == 0)) || (((gpio_idx) == 0) && ((pin_idx) == 2)) ||         \
     (((gpio_idx) == 0) && ((pin_idx) == 9)) || (((gpio_idx) == 0) && ((pin_idx) == 13)) ||        \
     (((gpio_idx) == 0) && ((pin_idx) == 14)) || (((gpio_idx) == 1) && ((pin_idx) == 6)))
  /// Whether the pin is USART RX
  #define GPIO_IS_USART_RX(gpio_idx, pin_idx)                                                      \
    ((((gpio_idx) == 0) && ((pin_idx) == 3)) || (((gpio_idx) == 0) && ((pin_idx) == 10)) ||        \
     (((gpio_idx) == 0) && ((pin_idx) == 13)) || (((gpio_idx) == 0) && ((pin_idx) == 14)) ||       \
     (((gpio_idx) == 0) && ((pin_idx) == 15)) || (((gpio_idx) == 1) && ((pin_idx) == 7)))

#else
  #error This MCU not supported
#endif

typedef struct
{
  rcu_periph_enum rcu_gpio;
  uint32_t gpio;
} gpio_hw_item_t;

typedef struct
{
  uint16_t pin;
} gpio_pin_hw_item_t;

const gpio_hw_item_t gpio_hw_items[GPIO_AMOUNT] = {
    {.rcu_gpio = RCU_GPIOA, .gpio = GPIOA}, {.rcu_gpio = RCU_GPIOB, .gpio = GPIOB},
    {.rcu_gpio = RCU_GPIOC, .gpio = GPIOC}, {.rcu_gpio = RCU_GPIOD, .gpio = GPIOD},
#if defined(GD32E11X)
    {.rcu_gpio = RCU_GPIOE, .gpio = GPIOE},
#elif defined(GD32L23X)
        {.rcu_gpio = RCU_GPIOF /* stub */, .gpio = (uint32_t)NULL},
        {.rcu_gpio = RCU_GPIOF, .gpio = GPIOF},
#else
  #error This MCU not supported
#endif
};

// clang-format off
const gpio_pin_hw_item_t gpio_pin_hw_items[GPIO_PIN_AMOUNT] =
    {
        { GPIO_PIN_0, },
        { GPIO_PIN_1, },
        { GPIO_PIN_2, },
        { GPIO_PIN_3, },
        { GPIO_PIN_4, },
        { GPIO_PIN_5, },
        { GPIO_PIN_6, },
        { GPIO_PIN_7, },
        { GPIO_PIN_8, },
        { GPIO_PIN_9, },
        { GPIO_PIN_10, },
        { GPIO_PIN_11, },
        { GPIO_PIN_12, },
        { GPIO_PIN_13, },
        { GPIO_PIN_14, },
        { GPIO_PIN_15, },
};
// clang-format on

void gpio_rcc_cmd(uint8_t gpio_idx, bool new_state)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (gpio_hw_items[gpio_idx].gpio != (uint32_t)NULL));

#if defined(GD32E11X) || defined(GD32L23X)
  if (new_state)
  {
    rcu_periph_clock_enable(gpio_hw_items[gpio_idx].rcu_gpio);
  }
  else
  {
    rcu_periph_clock_disable(gpio_hw_items[gpio_idx].rcu_gpio);
  }
#else
  #error This MCU not supported
#endif
}

void gpio_pin_set_mode(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT) &&
             (gpio_hw_items[gpio_idx].gpio != (uint32_t)NULL));

#if defined(GD32E11X)

  if ((mode == GPIO_MODE_OUTPUT_PP) || (mode == GPIO_MODE_OUTPUT_OD) || (mode == GPIO_MODE_IN_AN))
  {
    if (GPIO_IS_IO_NEED_REMAP(gpio_idx, pin_idx))
    {
      rcu_periph_clock_enable(RCU_AF);
      gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);
    }
  }

  switch (mode)
  {
    case GPIO_MODE_OUTPUT_PP:
      gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,
                gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_IN_PU:
      gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ,
                gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_IN_PD:
      gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ,
                gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_USART0:
    case GPIO_MODE_USART1:
    case GPIO_MODE_USART2:
    case GPIO_MODE_UART3:
    case GPIO_MODE_UART4:
      if (GPIO_IS_USART_NEED_REMAP(gpio_idx, pin_idx))
      {
        rcu_periph_clock_enable(RCU_AF);
        if (mode == GPIO_MODE_USART0)
        {
          gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);
        }
        else if (mode == GPIO_MODE_USART1)
        {
          gpio_pin_remap_config(GPIO_USART1_REMAP, ENABLE);
        }
        else if (mode == GPIO_MODE_USART2)
        {
          gpio_pin_remap_config(GPIO_USART2_PARTIAL_REMAP, ENABLE);
        }
      }
      if (GPIO_IS_USART_TX(gpio_idx, pin_idx))
      {
        gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ,
                  gpio_pin_hw_items[pin_idx].pin);
      }
      else if (GPIO_IS_USART_RX(gpio_idx, pin_idx))
      {
        gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ,
                  gpio_pin_hw_items[pin_idx].pin);
      }
      else
      {
        hal_assert(false);
      }
      break;

    case GPIO_MODE_I2C0:
      if (GPIO_IS_I2C_NEED_REMAP(gpio_idx, pin_idx))
      {
        rcu_periph_clock_enable(RCU_AF);
        gpio_pin_remap_config(GPIO_I2C0_REMAP, ENABLE);
      }
      gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ,
                gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_I2C1:
      gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ,
                gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_IN_AN:
      gpio_init(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AIN, GPIO_OSPEED_MAX,
                gpio_pin_hw_items[pin_idx].pin);
      break;

    default:
      hal_assert(false);
  }

#elif defined(GD32L23X)

  switch (mode)
  {
    case GPIO_MODE_OUTPUT_PP:
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
                    gpio_pin_hw_items[pin_idx].pin);
      gpio_output_options_set(gpio_hw_items[gpio_idx].gpio, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                              gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_OUTPUT_OD:
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    gpio_pin_hw_items[pin_idx].pin);
      gpio_output_options_set(gpio_hw_items[gpio_idx].gpio, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ,
                              gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_IN_PU:
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                    gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_IN_PD:
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN,
                    gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_IN_FLOAT:
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                    gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_USART0:
    case GPIO_MODE_USART1:
    case GPIO_MODE_UART3:
    case GPIO_MODE_UART4:
    {
      uint32_t uart_af;
      switch (mode)
      {
        case GPIO_MODE_USART0:
          uart_af = GPIO_AF_7;
          break;
        case GPIO_MODE_USART1:
          uart_af = (gpio_idx == 0 && pin_idx == 14) ? GPIO_AF_8 : GPIO_AF_7;
          break;
        case GPIO_MODE_UART3:
          uart_af = (gpio_idx == 0) ? GPIO_AF_8 : GPIO_AF_7;
          break;
        case GPIO_MODE_UART4:
          uart_af = (gpio_idx == 1) ? GPIO_AF_8 : GPIO_AF_7;
          break;
        default:
          hal_assert(false);
          return;
      }

      if (GPIO_IS_USART_TX(gpio_idx, pin_idx))
      {
        gpio_af_set(gpio_hw_items[gpio_idx].gpio, uart_af, gpio_pin_hw_items[pin_idx].pin);
        gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                      gpio_pin_hw_items[pin_idx].pin);
        gpio_output_options_set(gpio_hw_items[gpio_idx].gpio, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                                gpio_pin_hw_items[pin_idx].pin);
      }
      else if (GPIO_IS_USART_RX(gpio_idx, pin_idx))
      {
        gpio_af_set(gpio_hw_items[gpio_idx].gpio, uart_af, gpio_pin_hw_items[pin_idx].pin);
        gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF, GPIO_PUPD_NONE,
                      gpio_pin_hw_items[pin_idx].pin);
        gpio_output_options_set(gpio_hw_items[gpio_idx].gpio, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                                gpio_pin_hw_items[pin_idx].pin);
      }
      else
      {
        hal_assert(false);
      }
      break;
    }

    case GPIO_MODE_I2C0:
    case GPIO_MODE_I2C1:
    case GPIO_MODE_I2C2:
      gpio_af_set(gpio_hw_items[gpio_idx].gpio,
                  ((gpio_idx == 1) && (mode == GPIO_MODE_I2C1)) ? GPIO_AF_5 : GPIO_AF_4,
                  gpio_pin_hw_items[pin_idx].pin);
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                    gpio_pin_hw_items[pin_idx].pin);
      gpio_output_options_set(gpio_hw_items[gpio_idx].gpio, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ,
                              gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_CKOUT:
      hal_assert((gpio_hw_items[gpio_idx].gpio == GPIOA) && (pin_idx == 8 || pin_idx == 9));

      gpio_af_set(gpio_hw_items[gpio_idx].gpio, GPIO_AF_0, gpio_pin_hw_items[pin_idx].pin);
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    gpio_pin_hw_items[pin_idx].pin);
      gpio_output_options_set(gpio_hw_items[gpio_idx].gpio, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                              gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_IN_AN:
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                    gpio_pin_hw_items[pin_idx].pin);
      break;

    case GPIO_MODE_TIM1:
    case GPIO_MODE_TIM2:
      gpio_mode_set(gpio_hw_items[gpio_idx].gpio, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    gpio_pin_hw_items[pin_idx].pin);
      gpio_output_options_set(gpio_hw_items[gpio_idx].gpio, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                              gpio_pin_hw_items[pin_idx].pin);
      gpio_af_set(gpio_hw_items[gpio_idx].gpio, GPIO_AF_1, gpio_pin_hw_items[pin_idx].pin);
      break;

    default:
      hal_assert(false);
  }

#else
  #error This MCU not supported
#endif
}

void gpio_pin_cmd(uint8_t gpio_idx, uint8_t pin_idx, bool new_state)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT) &&
             (gpio_hw_items[gpio_idx].gpio != (uint32_t)NULL));

#if defined(GD32E11X) || defined(GD32L23X)
  gpio_bit_write(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin,
                 (bit_status)new_state);

#else
  #error This MCU not supported
#endif
}

bool gpio_pin_state(uint8_t gpio_idx, uint8_t pin_idx)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT) &&
             (gpio_hw_items[gpio_idx].gpio != (uint32_t)NULL));

#if defined(GD32E11X) || defined(GD32L23X)
  return (SET == gpio_input_bit_get(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin));
#else
  #error This MCU not supported
#endif
}

volatile uint32_t *gpio_get_bsrr(uint8_t gpio_idx)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (gpio_hw_items[gpio_idx].gpio != (uint32_t)NULL));

#if defined(GD32E11X) || defined(GD32L23X)
  return &GPIO_BOP(gpio_hw_items[gpio_idx].gpio);
#else
  #error This MCU not supported
#endif
}

void gpio_pin_toggle(uint8_t gpio_idx, uint8_t pin_idx)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT) &&
             (gpio_hw_items[gpio_idx].gpio != (uint32_t)NULL));

#if defined(GD32E11X) || defined(GD32L23X)
  // TODO need to create new function gpio_toggle in gd32 lib
  uint32_t pin = gpio_pin_hw_items[pin_idx].pin;
  if (GPIO_OCTL(gpio_hw_items[gpio_idx].gpio) & pin)
  {
    GPIO_BC(gpio_hw_items[gpio_idx].gpio) = pin;
  }
  else
  {
    GPIO_BOP(gpio_hw_items[gpio_idx].gpio) = pin;
  }
#else
  #error This MCU not supported
#endif
}
