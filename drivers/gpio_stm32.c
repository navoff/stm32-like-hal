/**
 * @file gpio_stm32.c
 * @brief GPIO driver implementation for STM32 MCUs.
 */

#include "config.h"

#include <stdbool.h>

#include "utils/assert.h"
#include "utils/macros.h"

#include "drivers/gpio.h"

#if defined(STM32F1XX)
  #define GPIO_AMOUNT 7

  /// Whether the pin requires remapping
  #define GPIO_IS_IO_NEED_REMAP(gpio_idx, pin_idx)                                                 \
    ((((gpio_idx) == 1) && ((pin_idx) == 3)) || (((gpio_idx) == 1) && ((pin_idx) == 4)) ||         \
     (((gpio_idx) == 0) && ((pin_idx) == 15)))

  /// Whether the pin is USART1 TX
  #define GPIO_IS_USART1_TX(gpio_idx, pin_idx)                                                     \
    ((((gpio_idx) == 0) && ((pin_idx) == 9)) || (((gpio_idx) == 1) && ((pin_idx) == 6)))
  /// Whether the pin is USART1 RX
  #define GPIO_IS_USART1_RX(gpio_idx, pin_idx)                                                     \
    ((((gpio_idx) == 0) && ((pin_idx) == 10)) || (((gpio_idx) == 1) && ((pin_idx) == 7)))
  /// Whether the pin requires remapping
  #define GPIO_IS_USART1_NEED_REMAP(gpio_idx) ((gpio_idx) == 1)

  /// Whether the pin is USART2 TX
  #define GPIO_IS_USART2_TX(gpio_idx, pin_idx)                                                     \
    ((((gpio_idx) == 0) && ((pin_idx) == 2)) || (((gpio_idx) == 3) && ((pin_idx) == 5)))
  /// Whether the pin is USART2 RX
  #define GPIO_IS_USART2_RX(gpio_idx, pin_idx)                                                     \
    ((((gpio_idx) == 0) && ((pin_idx) == 3)) || (((gpio_idx) == 3) && ((pin_idx) == 6)))
  /// Whether the pin requires remapping
  #define GPIO_IS_USART2_NEED_REMAP(gpio_idx) ((gpio_idx) == 3)

  /// SPI1: PA5/PA6/PA7 (SCK/MISO/MOSI), remapped: PB3/PB4/PB5
  #define GPIO_IS_SPI1_MISO(gpio_idx, pin_idx)                                                     \
    ((((gpio_idx) == 0) && ((pin_idx) == 6)) || (((gpio_idx) == 1) && ((pin_idx) == 4)))
  #define GPIO_IS_SPI1_SCK_MOSI(gpio_idx, pin_idx)                                                 \
    ((((gpio_idx) == 0) && (((pin_idx) == 5) || ((pin_idx) == 7))) ||                              \
     (((gpio_idx) == 1) && (((pin_idx) == 3) || ((pin_idx) == 5))))
  /// Whether the pin requires remapping (PB3/PB4 are JTAG pins, the SWJ remap is applied too)
  #define GPIO_IS_SPI1_NEED_REMAP(gpio_idx) ((gpio_idx) == 1)

  /// SPI2: PB13/PB14/PB15 (SCK/MISO/MOSI), no remap
  #define GPIO_IS_SPI2_MISO(gpio_idx, pin_idx) (((gpio_idx) == 1) && ((pin_idx) == 14))
  #define GPIO_IS_SPI2_SCK_MOSI(gpio_idx, pin_idx)                                                 \
    (((gpio_idx) == 1) && (((pin_idx) == 13) || ((pin_idx) == 15)))

#elif defined(STM32F30X)
  #define GPIO_AMOUNT 8

#elif defined(STM32F4XX)
  #define GPIO_AMOUNT 11

#elif defined(STM32L4XX)
  #define GPIO_AMOUNT 8

#elif defined(STM32F0XX)
  #define GPIO_AMOUNT 6

#else
  #error This MCU not supported
#endif

typedef struct
{
  uint16_t pin;

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  uint8_t pin_src;
#endif

#if defined(STM32F4XX) || defined(STM32L4XX) || defined(STM32F0XX)
  uint8_t exti_src;
#endif

  uint16_t exti_line;

  IRQn_Type exti_irq;
} gpio_pin_hw_item_t;

typedef struct
{
  GPIO_TypeDef *gpio;
#if defined(STM32F1XX)
  uint8_t gpio_port_src;
#elif defined(STM32F4XX)
  uint8_t exti_port_src;
#endif
} gpio_hw_item_t;

// clang-format off
#if defined(STM32F1XX)
const gpio_pin_hw_item_t gpio_pin_hw_items[GPIO_PIN_AMOUNT] =
{
    { GPIO_Pin_0,  GPIO_PinSource0,  EXTI_Line0,  EXTI0_IRQn,     },
    { GPIO_Pin_1,  GPIO_PinSource1,  EXTI_Line1,  EXTI1_IRQn,     },
    { GPIO_Pin_2,  GPIO_PinSource2,  EXTI_Line2,  EXTI2_IRQn,     },
    { GPIO_Pin_3,  GPIO_PinSource3,  EXTI_Line3,  EXTI3_IRQn,     },
    { GPIO_Pin_4,  GPIO_PinSource4,  EXTI_Line4,  EXTI4_IRQn,     },
    { GPIO_Pin_5,  GPIO_PinSource5,  EXTI_Line5,  EXTI9_5_IRQn,   },
    { GPIO_Pin_6,  GPIO_PinSource6,  EXTI_Line6,  EXTI9_5_IRQn,   },
    { GPIO_Pin_7,  GPIO_PinSource7,  EXTI_Line7,  EXTI9_5_IRQn,   },
    { GPIO_Pin_8,  GPIO_PinSource8,  EXTI_Line8,  EXTI9_5_IRQn,   },
    { GPIO_Pin_9,  GPIO_PinSource9,  EXTI_Line9,  EXTI9_5_IRQn,   },
    { GPIO_Pin_10, GPIO_PinSource10, EXTI_Line10, EXTI15_10_IRQn, },
    { GPIO_Pin_11, GPIO_PinSource11, EXTI_Line11, EXTI15_10_IRQn, },
    { GPIO_Pin_12, GPIO_PinSource12, EXTI_Line12, EXTI15_10_IRQn, },
    { GPIO_Pin_13, GPIO_PinSource13, EXTI_Line13, EXTI15_10_IRQn, },
    { GPIO_Pin_14, GPIO_PinSource14, EXTI_Line14, EXTI15_10_IRQn, },
    { GPIO_Pin_15, GPIO_PinSource15, EXTI_Line15, EXTI15_10_IRQn, },
};

const gpio_hw_item_t gpio_hw_items[GPIO_AMOUNT] =
{
    { GPIOA, GPIO_PortSourceGPIOA, },
    { GPIOB, GPIO_PortSourceGPIOB, },
    { GPIOC, GPIO_PortSourceGPIOC, },
    { GPIOD, GPIO_PortSourceGPIOD, },
    { GPIOE, GPIO_PortSourceGPIOE, },
    { GPIOF, GPIO_PortSourceGPIOF, },
    { GPIOG, GPIO_PortSourceGPIOG, },
};

#elif defined(STM32F30X)
const gpio_pin_hw_item_t gpio_pin_hw_items[GPIO_PIN_AMOUNT] =
{
    { GPIO_Pin_0,  GPIO_PinSource0,  EXTI_Line0,  EXTI0_IRQn,     },
    { GPIO_Pin_1,  GPIO_PinSource1,  EXTI_Line1,  EXTI1_IRQn,     },
    { GPIO_Pin_2,  GPIO_PinSource2,  EXTI_Line2,  EXTI2_TS_IRQn,  },
    { GPIO_Pin_3,  GPIO_PinSource3,  EXTI_Line3,  EXTI3_IRQn,     },
    { GPIO_Pin_4,  GPIO_PinSource4,  EXTI_Line4,  EXTI4_IRQn,     },
    { GPIO_Pin_5,  GPIO_PinSource5,  EXTI_Line5,  EXTI9_5_IRQn,   },
    { GPIO_Pin_6,  GPIO_PinSource6,  EXTI_Line6,  EXTI9_5_IRQn,   },
    { GPIO_Pin_7,  GPIO_PinSource7,  EXTI_Line7,  EXTI9_5_IRQn,   },
    { GPIO_Pin_8,  GPIO_PinSource8,  EXTI_Line8,  EXTI9_5_IRQn,   },
    { GPIO_Pin_9,  GPIO_PinSource9,  EXTI_Line9,  EXTI9_5_IRQn,   },
    { GPIO_Pin_10, GPIO_PinSource10, EXTI_Line10, EXTI15_10_IRQn, },
    { GPIO_Pin_11, GPIO_PinSource11, EXTI_Line11, EXTI15_10_IRQn, },
    { GPIO_Pin_12, GPIO_PinSource12, EXTI_Line12, EXTI15_10_IRQn, },
    { GPIO_Pin_13, GPIO_PinSource13, EXTI_Line13, EXTI15_10_IRQn, },
    { GPIO_Pin_14, GPIO_PinSource14, EXTI_Line14, EXTI15_10_IRQn, },
    { GPIO_Pin_15, GPIO_PinSource15, EXTI_Line15, EXTI15_10_IRQn, },
};

const gpio_hw_item_t gpio_hw_items[GPIO_AMOUNT] =
{
    { GPIOA },
    { GPIOB },
    { GPIOC },
    { GPIOD },
    { GPIOE },
    { GPIOF },
};

#elif defined(STM32F4XX)

const gpio_pin_hw_item_t gpio_pin_hw_items[GPIO_PIN_AMOUNT] =
{
    { GPIO_Pin_0,  GPIO_PinSource0,  EXTI_PinSource0,  EXTI_Line0,  EXTI0_IRQn,     },
    { GPIO_Pin_1,  GPIO_PinSource1,  EXTI_PinSource1,  EXTI_Line1,  EXTI1_IRQn,     },
    { GPIO_Pin_2,  GPIO_PinSource2,  EXTI_PinSource2,  EXTI_Line2,  EXTI2_IRQn,     },
    { GPIO_Pin_3,  GPIO_PinSource3,  EXTI_PinSource3,  EXTI_Line3,  EXTI3_IRQn,     },
    { GPIO_Pin_4,  GPIO_PinSource4,  EXTI_PinSource4,  EXTI_Line4,  EXTI4_IRQn,     },
    { GPIO_Pin_5,  GPIO_PinSource5,  EXTI_PinSource5,  EXTI_Line5,  EXTI9_5_IRQn,   },
    { GPIO_Pin_6,  GPIO_PinSource6,  EXTI_PinSource6,  EXTI_Line6,  EXTI9_5_IRQn,   },
    { GPIO_Pin_7,  GPIO_PinSource7,  EXTI_PinSource7,  EXTI_Line7,  EXTI9_5_IRQn,   },
    { GPIO_Pin_8,  GPIO_PinSource8,  EXTI_PinSource8,  EXTI_Line8,  EXTI9_5_IRQn,   },
    { GPIO_Pin_9,  GPIO_PinSource9,  EXTI_PinSource9,  EXTI_Line9,  EXTI9_5_IRQn,   },
    { GPIO_Pin_10, GPIO_PinSource10, EXTI_PinSource10, EXTI_Line10, EXTI15_10_IRQn, },
    { GPIO_Pin_11, GPIO_PinSource11, EXTI_PinSource11, EXTI_Line11, EXTI15_10_IRQn, },
    { GPIO_Pin_12, GPIO_PinSource12, EXTI_PinSource12, EXTI_Line12, EXTI15_10_IRQn, },
    { GPIO_Pin_13, GPIO_PinSource13, EXTI_PinSource13, EXTI_Line13, EXTI15_10_IRQn, },
    { GPIO_Pin_14, GPIO_PinSource14, EXTI_PinSource14, EXTI_Line14, EXTI15_10_IRQn, },
    { GPIO_Pin_15, GPIO_PinSource15, EXTI_PinSource15, EXTI_Line15, EXTI15_10_IRQn, },
};

const gpio_hw_item_t gpio_hw_items[GPIO_AMOUNT] =
{
    { GPIOA, EXTI_PortSourceGPIOA, },
    { GPIOB, EXTI_PortSourceGPIOB, },
    { GPIOC, EXTI_PortSourceGPIOC, },
    { GPIOD, EXTI_PortSourceGPIOD, },
    { GPIOE, EXTI_PortSourceGPIOE, },
    { GPIOF, EXTI_PortSourceGPIOF, },
    { GPIOG, EXTI_PortSourceGPIOG, },
    { GPIOH, EXTI_PortSourceGPIOH, },
    { GPIOI, EXTI_PortSourceGPIOI, },
    { GPIOJ, EXTI_PortSourceGPIOJ, },
    { GPIOK, EXTI_PortSourceGPIOK, },
};

#elif defined(STM32L4XX)

const gpio_pin_hw_item_t gpio_pin_hw_items[GPIO_PIN_AMOUNT] =
{
    { LL_GPIO_PIN_0,  0, 0, EXTI0_IRQn,     },
    { LL_GPIO_PIN_1,  0, 0, EXTI1_IRQn,     },
    { LL_GPIO_PIN_2,  0, 0, EXTI2_IRQn,     },
    { LL_GPIO_PIN_3,  0, 0, EXTI3_IRQn,     },
    { LL_GPIO_PIN_4,  0, 0, EXTI4_IRQn,     },
    { LL_GPIO_PIN_5,  0, 0, EXTI9_5_IRQn,   },
    { LL_GPIO_PIN_6,  0, 0, EXTI9_5_IRQn,   },
    { LL_GPIO_PIN_7,  0, 0, EXTI9_5_IRQn,   },
    { LL_GPIO_PIN_8,  0, 0, EXTI9_5_IRQn,   },
    { LL_GPIO_PIN_9,  0, 0, EXTI9_5_IRQn,   },
    { LL_GPIO_PIN_10, 0, 0, EXTI15_10_IRQn, },
    { LL_GPIO_PIN_11, 0, 0, EXTI15_10_IRQn, },
    { LL_GPIO_PIN_12, 0, 0, EXTI15_10_IRQn, },
    { LL_GPIO_PIN_13, 0, 0, EXTI15_10_IRQn, },
    { LL_GPIO_PIN_14, 0, 0, EXTI15_10_IRQn, },
    { LL_GPIO_PIN_15, 0, 0, EXTI15_10_IRQn, },
};

const gpio_hw_item_t gpio_hw_items[GPIO_AMOUNT] =
{
    { GPIOA, },
    { GPIOB, },
    { GPIOC, },
    { GPIOD, },
    { GPIOE, },
    { GPIOF, },
    { GPIOG, },
    { GPIOH, },
};

#elif defined(STM32F0XX)

const gpio_pin_hw_item_t gpio_pin_hw_items[GPIO_PIN_AMOUNT] =
{
    { LL_GPIO_PIN_0,  0, 0, EXTI0_1_IRQn,  },
    { LL_GPIO_PIN_1,  0, 0, EXTI0_1_IRQn,  },
    { LL_GPIO_PIN_2,  0, 0, EXTI2_3_IRQn,  },
    { LL_GPIO_PIN_3,  0, 0, EXTI2_3_IRQn,  },
    { LL_GPIO_PIN_4,  0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_5,  0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_6,  0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_7,  0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_8,  0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_9,  0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_10, 0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_11, 0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_12, 0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_13, 0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_14, 0, 0, EXTI4_15_IRQn, },
    { LL_GPIO_PIN_15, 0, 0, EXTI4_15_IRQn, },
};

const gpio_hw_item_t gpio_hw_items[GPIO_AMOUNT] =
{
    { GPIOA, },
    { GPIOB, },
    { GPIOC, },
    { GPIOD, },
    { GPIOE, },
    { GPIOF, },
};
#else
  #error This MCU not supported
#endif

// clang-format on

void gpio_rcc_cmd(uint8_t gpio_idx, bool new_state)
{
  hal_assert(gpio_idx < GPIO_AMOUNT);

#if defined(STM32F1XX)
  switch (gpio_idx)
  {
    case 0:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, new_state);
      break;
    case 1:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, new_state);
      break;
    case 2:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, new_state);
      break;
    case 3:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, new_state);
      break;
    case 4:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, new_state);
      break;
    case 5:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, new_state);
      break;
    case 6:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, new_state);
      break;
  }
#elif defined(STM32F30X)
  switch (gpio_idx)
  {
    case 0:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, new_state);
      break;
    case 1:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, new_state);
      break;
    case 2:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, new_state);
      break;
    case 3:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, new_state);
      break;
    case 4:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, new_state);
      break;
    case 5:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, new_state);
      break;
    case 6:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOG, new_state);
      break;
    case 7:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOH, new_state);
      break;
  }
#elif defined(STM32F4XX)
  switch (gpio_idx)
  {
    case 0:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, new_state);
      break;
    case 1:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, new_state);
      break;
    case 2:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, new_state);
      break;
    case 3:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, new_state);
      break;
    case 4:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, new_state);
      break;
    case 5:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, new_state);
      break;
    case 6:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, new_state);
      break;
    case 7:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, new_state);
      break;
    case 8:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, new_state);
      break;
    case 9:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOJ, new_state);
      break;
    case 10:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOK, new_state);
      break;
  }
#elif defined(STM32L4XX)
  if (new_state)
  {
    switch (gpio_idx)
    {
      case 0:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        break;
      case 1:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        break;
      case 2:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        break;
      case 3:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        break;
      case 4:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        break;
      case 5:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        break;
      case 6:
        __HAL_RCC_GPIOG_CLK_ENABLE();
        break;
      case 7:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        break;
    }
  }
  else
  {
    switch (gpio_idx)
    {
      case 0:
        __HAL_RCC_GPIOA_CLK_DISABLE();
        break;
      case 1:
        __HAL_RCC_GPIOB_CLK_DISABLE();
        break;
      case 2:
        __HAL_RCC_GPIOC_CLK_DISABLE();
        break;
      case 3:
        __HAL_RCC_GPIOD_CLK_DISABLE();
        break;
      case 4:
        __HAL_RCC_GPIOE_CLK_DISABLE();
        break;
      case 5:
        __HAL_RCC_GPIOF_CLK_DISABLE();
        break;
      case 6:
        __HAL_RCC_GPIOG_CLK_DISABLE();
        break;
      case 7:
        __HAL_RCC_GPIOH_CLK_DISABLE();
        break;
    }
  }
#elif defined(STM32F0XX)
  if (new_state)
  {
    switch (gpio_idx)
    {
      case 0:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        break;
      case 1:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        break;
      case 2:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        break;
      case 3:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        break;
      case 4:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        break;
      case 5:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        break;
    }
  }
  else
  {
    switch (gpio_idx)
    {
      case 0:
        __HAL_RCC_GPIOA_CLK_DISABLE();
        break;
      case 1:
        __HAL_RCC_GPIOB_CLK_DISABLE();
        break;
      case 2:
        __HAL_RCC_GPIOC_CLK_DISABLE();
        break;
      case 3:
        __HAL_RCC_GPIOD_CLK_DISABLE();
        break;
      case 4:
        __HAL_RCC_GPIOE_CLK_DISABLE();
        break;
      case 5:
        __HAL_RCC_GPIOF_CLK_DISABLE();
        break;
    }
  }
#else
  #error This MCU not supported
#endif
}

void gpio_deinit(uint8_t gpio_idx)
{
  hal_assert(gpio_idx < GPIO_AMOUNT);

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  GPIO_DeInit(gpio_hw_items[gpio_idx].gpio);
#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_GPIO_DeInit(gpio_hw_items[gpio_idx].gpio);
#else
  #error This MCU not supported
#endif
}

#if defined(STM32F1XX)
void gpio_pin_set_mode_stm32f1xx(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = gpio_pin_hw_items[pin_idx].pin;

  GPIO_TypeDef *gpio = gpio_hw_items[gpio_idx].gpio;

  if ((mode == GPIO_MODE_OUTPUT_PP) || (mode == GPIO_MODE_OUTPUT_OD) || (mode == GPIO_MODE_IN_AN))
  {
    if (GPIO_IS_IO_NEED_REMAP(gpio_idx, pin_idx))
    {
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
      GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    }
  }

  switch (mode)
  {
    case GPIO_MODE_OUTPUT_PP:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
      break;
    case GPIO_MODE_OUTPUT_OD:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
      break;
    case GPIO_MODE_IN_AN:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
      break;

  #if (FEATURE_ENABLE_UART1)
    case GPIO_MODE_USART1:
      if (GPIO_IS_USART1_TX(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      }
      else if (GPIO_IS_USART1_RX(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
      }
      else
      {
        hal_assert(false);
      }

      if (GPIO_IS_USART1_NEED_REMAP(gpio_idx))
      {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
      }
      break;
  #endif

  #if (FEATURE_ENABLE_UART2)
    case GPIO_MODE_USART2:
      if (GPIO_IS_USART2_TX(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      }
      else if (GPIO_IS_USART2_RX(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
      }
      else
      {
        hal_assert(false);
      }

      if (GPIO_IS_USART2_NEED_REMAP(gpio_idx))
      {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
      }
      break;
  #endif

  #if (FEATURE_ENABLE_USB)
    case GPIO_MODE_USB:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      break;
  #endif

  #if (FEATURE_ENABLE_SPI1)
    case GPIO_MODE_SPI1:
      if (GPIO_IS_SPI1_SCK_MOSI(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      }
      else if (GPIO_IS_SPI1_MISO(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
      }
      else
      {
        hal_assert(false);
      }

      if (GPIO_IS_SPI1_NEED_REMAP(gpio_idx))
      {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);
      }
      break;
  #endif

  #if (FEATURE_ENABLE_SPI2)
    case GPIO_MODE_SPI2:
      if (GPIO_IS_SPI2_SCK_MOSI(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      }
      else if (GPIO_IS_SPI2_MISO(gpio_idx, pin_idx))
      {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
      }
      else
      {
        hal_assert(false);
      }
      break;
  #endif

    default:
      hal_assert(false);
  }

  GPIO_Init(gpio, &GPIO_InitStructure);
}
#endif

#if defined(STM32F30X)
void gpio_pin_set_mode_stm32f30x(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = gpio_pin_hw_items[pin_idx].pin;

  GPIO_TypeDef *gpio = gpio_hw_items[gpio_idx].gpio;

  switch (mode)
  {
    case GPIO_MODE_OUTPUT_PP:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
      break;
    case GPIO_MODE_OUTPUT_OD:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      break;

  #if (FEATURE_ENABLE_UART1 || FEATURE_ENABLE_UART2 || FEATURE_ENABLE_UART3)
    case GPIO_MODE_USART1:
    case GPIO_MODE_USART2:
    case GPIO_MODE_USART3:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_7);
      break;
  #endif

  #if (FEATURE_ENABLE_UART4 || FEATURE_ENABLE_UART5)
    case GPIO_MODE_UART4:
    case GPIO_MODE_UART5:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_5);
      break;
  #endif

  #if (FEATURE_ENABLE_USB)
    case GPIO_MODE_USB:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_14);
      break;
  #endif

    default:
      hal_assert(false);
  }

  GPIO_Init(gpio, &GPIO_InitStructure);
}
#endif

#if defined(STM32F4XX)
void gpio_pin_set_mode_stm32f4xx(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = gpio_pin_hw_items[pin_idx].pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

  GPIO_TypeDef *gpio = gpio_hw_items[gpio_idx].gpio;

  switch (mode)
  {
    case GPIO_MODE_OUTPUT_PP:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      break;

    case GPIO_MODE_OUTPUT_OD:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      break;

    case GPIO_MODE_IN_AN:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      break;

    case GPIO_MODE_CAN1:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_CAN1);
      break;

    case GPIO_MODE_CAN2:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_CAN2);
      break;

  #if (FEATURE_ENABLE_SPI1)
    case GPIO_MODE_SPI1:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_SPI1);
      break;
  #endif

  #if (FEATURE_ENABLE_SPI2)
    case GPIO_MODE_SPI2:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_SPI2);
      break;
  #endif

  #if (FEATURE_ENABLE_SPI3)
    case GPIO_MODE_SPI3:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_SPI3);
      break;
  #endif

  #if (FEATURE_ENABLE_SPI4)
    case GPIO_MODE_SPI4:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_SPI4);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM1)
    case GPIO_MODE_TIM1:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM1);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM2)
    case GPIO_MODE_TIM2:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM2);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM3)
    case GPIO_MODE_TIM3:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM3);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM4)
    case GPIO_MODE_TIM4:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM4);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM5)
    case GPIO_MODE_TIM5:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM5);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM8)
    case GPIO_MODE_TIM8:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM8);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM9)
    case GPIO_MODE_TIM9:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM9);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM10)
    case GPIO_MODE_TIM10:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM10);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM11)
    case GPIO_MODE_TIM11:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM11);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM12)
    case GPIO_MODE_TIM12:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM12);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM13)
    case GPIO_MODE_TIM13:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM13);
      break;
  #endif

  #if (FEATURE_ENABLE_TIM14)
    case GPIO_MODE_TIM14:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_TIM14);
      break;
  #endif

  #if (FEATURE_ENABLE_UART1)
    case GPIO_MODE_USART1:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_USART1);
      break;
  #endif

  #if (FEATURE_ENABLE_UART2)
    case GPIO_MODE_USART2:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_USART2);
      break;
  #endif

  #if (FEATURE_ENABLE_UART3)
    case GPIO_MODE_USART3:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_USART3);
      break;
  #endif

  #if (FEATURE_ENABLE_UART4)
    case GPIO_MODE_UART4:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_UART4);
      break;
  #endif

  #if (FEATURE_ENABLE_UART5)
    case GPIO_MODE_UART5:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_UART5);
      break;
  #endif

  #if (FEATURE_ENABLE_UART6)
    case GPIO_MODE_USART6:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_USART6);
      break;
  #endif

  #if (FEATURE_ENABLE_UART7)
    case GPIO_MODE_UART7:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_UART7);
      break;
  #endif

  #if (FEATURE_ENABLE_UART8)
    case GPIO_MODE_UART8:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_UART8);
      break;
  #endif

  #if (FEATURE_ENABLE_I2C1)
    case GPIO_MODE_I2C1:
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_I2C1);
      break;
  #endif

  #if (FEATURE_ENABLE_I2C2)
    case GPIO_MODE_I2C2:
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_I2C2);
      break;
  #endif

  #if (FEATURE_ENABLE_I2C3)
    case GPIO_MODE_I2C3:
      GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
      GPIO_PinAFConfig(gpio, gpio_pin_hw_items[pin_idx].pin_src, GPIO_AF_I2C3);
      break;
  #endif

    default:
      hal_assert(false);
  }

  GPIO_Init(gpio, &GPIO_InitStructure);
}
#endif

#if defined(STM32L4XX)
void gpio_pin_set_mode_stm32l4xx(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode)
{
  LL_GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = gpio_pin_hw_items[pin_idx].pin;
  GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStructure.Alternate = LL_GPIO_AF_0;

  GPIO_TypeDef *gpio = gpio_hw_items[gpio_idx].gpio;

  GPIO_InitStructure.Mode = LL_GPIO_MODE_ALTERNATE;
  switch (mode)
  {
    case GPIO_MODE_OUTPUT_PP:
      GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;
      break;

    case GPIO_MODE_OUTPUT_OD:
      GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;
      GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
      GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;
      break;

    case GPIO_MODE_IN_AN:
      GPIO_InitStructure.Mode = LL_GPIO_MODE_ANALOG;
      GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;

      LL_GPIO_EnablePinAnalogControl(gpio, GPIO_InitStructure.Pin);
      break;

  #if (FEATURE_ENABLE_SPI1)
    case GPIO_MODE_SPI1:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_5;
      break;
  #endif

  #if (FEATURE_ENABLE_SPI2)
    case GPIO_MODE_SPI2:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_5;
      break;
  #endif

  #if (FEATURE_ENABLE_SPI3)
    case GPIO_MODE_SPI3:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_6;
      break;
  #endif

  #if (FEATURE_ENABLE_UART1)
    case GPIO_MODE_USART1:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_7;
      break;
  #endif

  #if (FEATURE_ENABLE_UART2)
    case GPIO_MODE_USART2:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_7;
      break;
  #endif

  #if (FEATURE_ENABLE_UART3)
    case GPIO_MODE_USART3:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_7;
      break;
  #endif

  #if (FEATURE_ENABLE_UART4)
    case GPIO_MODE_UART4:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_8;
      break;
  #endif

  #if (FEATURE_ENABLE_UART5)
    case GPIO_MODE_UART5:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_8;
      break;
  #endif

  #if (FEATURE_ENABLE_I2C1)
    case GPIO_MODE_I2C1:
      // without this sequence there will be a dominant pulse on the I2C line
      LL_GPIO_SetAFPin_0_7(gpio, GPIO_InitStructure.Pin, LL_GPIO_AF_4);

      GPIO_InitStructure.Pull = LL_GPIO_PULL_UP;
      GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
      GPIO_InitStructure.Alternate = LL_GPIO_AF_4;
      break;
  #endif

  #if (FEATURE_ENABLE_I2C2)
    case GPIO_MODE_I2C2:
      // without this sequence there will be a dominant pulse on the I2C line
      LL_GPIO_SetAFPin_0_7(gpio, GPIO_InitStructure.Pin, LL_GPIO_AF_4);

      GPIO_InitStructure.Pull = LL_GPIO_PULL_UP;
      GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
      GPIO_InitStructure.Alternate = LL_GPIO_AF_4;
      break;
  #endif

  #if (FEATURE_ENABLE_I2C3)
    case GPIO_MODE_I2C3:
      // without this sequence there will be a dominant pulse on the I2C line
      LL_GPIO_SetAFPin_0_7(gpio, GPIO_InitStructure.Pin, LL_GPIO_AF_4);

      GPIO_InitStructure.Pull = LL_GPIO_PULL_UP;
      GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
      GPIO_InitStructure.Alternate = LL_GPIO_AF_4;
      break;
  #endif

    default:
      hal_assert(false);
  }

  LL_GPIO_Init(gpio, &GPIO_InitStructure);
}
#endif

#if defined(STM32F0XX)
void gpio_pin_set_mode_stm32f0xx(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode)
{
  LL_GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = gpio_pin_hw_items[pin_idx].pin;
  GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStructure.Alternate = LL_GPIO_AF_0;

  GPIO_TypeDef *gpio = gpio_hw_items[gpio_idx].gpio;

  GPIO_InitStructure.Mode = LL_GPIO_MODE_ALTERNATE;
  switch (mode)
  {
    case GPIO_MODE_OUTPUT_PP:
      GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;
      break;

    case GPIO_MODE_OUTPUT_OD:
      GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;
      GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
      GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;
      break;

  #if (FEATURE_ENABLE_UART1)
    case GPIO_MODE_USART1:
      if (gpio_idx == 0)
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_1;
      }
      else if (gpio_idx == 1)
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_0;
      }
      else
      {
        hal_assert(false);
      }
      break;
  #endif

  #if (FEATURE_ENABLE_UART2)
    case GPIO_MODE_USART2:
      if (gpio_idx == 0)
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_1;
      }
      else if (gpio_idx == 3)
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_0;
      }
      else
      {
        hal_assert(false);
      }
      break;
  #endif

  #if (FEATURE_ENABLE_UART3)
    case GPIO_MODE_USART3:
      if ((gpio_idx == 0) || (gpio_idx == 1))
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_4;
      }
      else if (gpio_idx == 2)
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_1;
      }
      else if (gpio_idx == 3)
      {
        if (pin_idx == 2)
        {
          GPIO_InitStructure.Alternate = LL_GPIO_AF_1;
        }
        else
        {
          GPIO_InitStructure.Alternate = LL_GPIO_AF_0;
        }
      }
      else
      {
        hal_assert(false);
      }
      break;
  #endif

  #if (FEATURE_ENABLE_UART4)
    case GPIO_MODE_UART4:
      if ((gpio_idx == 0) || (gpio_idx == 1))
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_4;
      }
      else if (gpio_idx == 2)
      {
        GPIO_InitStructure.Alternate = LL_GPIO_AF_0;
      }
      else
      {
        hal_assert(false);
      }
      break;
  #endif

  #if (FEATURE_ENABLE_SPI1)
    case GPIO_MODE_SPI1:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_0;
      break;
  #endif

  #if (FEATURE_ENABLE_SPI2)
    case GPIO_MODE_SPI2:
      GPIO_InitStructure.Alternate = LL_GPIO_AF_0;
      break;
  #endif

  #if (FEATURE_ENABLE_I2C1)
    case GPIO_MODE_I2C1:
      GPIO_InitStructure.Pull = LL_GPIO_PULL_UP;
      GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
      GPIO_InitStructure.Alternate = LL_GPIO_AF_1;
      break;
  #endif

  #if (FEATURE_ENABLE_I2C2)
    case GPIO_MODE_I2C2:
    {
      uint32_t gpio_af;
      if ((pin_idx == 10) || (pin_idx == 11))
      {
        gpio_af = LL_GPIO_AF_1;
      }
      else
      {
        gpio_af = LL_GPIO_AF_5;
      }
      GPIO_InitStructure.Pull = LL_GPIO_PULL_UP;
      GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
      GPIO_InitStructure.Alternate = gpio_af;
    }
    break;
  #endif

    default:
      hal_assert(false);
  }

  LL_GPIO_Init(gpio, &GPIO_InitStructure);
}
#endif

void gpio_pin_set_mode(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_mode_t mode)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT));
  hal_assert(mode < GPIO_MODE_COUNT);

#if defined(STM32F1XX)
  gpio_pin_set_mode_stm32f1xx(gpio_idx, pin_idx, mode);

#elif defined(STM32F30X)
  gpio_pin_set_mode_stm32f30x(gpio_idx, pin_idx, mode);

#elif defined(STM32F4XX)
  gpio_pin_set_mode_stm32f4xx(gpio_idx, pin_idx, mode);

#elif defined(STM32L4XX)
  gpio_pin_set_mode_stm32l4xx(gpio_idx, pin_idx, mode);

#elif defined(STM32F0XX)
  gpio_pin_set_mode_stm32f0xx(gpio_idx, pin_idx, mode);

#else
  #error This MCU not supported
#endif
}

void gpio_pin_set_in_mode(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_pull_t pin_pull)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT));
  hal_assert(pin_pull < GPIO_PIN_PULL__COUNT);

#if defined(STM32F1XX)

  if (GPIO_IS_IO_NEED_REMAP(gpio_idx, pin_idx))
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
  }

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = gpio_pin_hw_items[pin_idx].pin;
  if (pin_pull == GPIO_PIN_PULL__UP)
  {
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  }
  else if (pin_pull == GPIO_PIN_PULL__DOWN)
  {
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  }
  else if (pin_pull == GPIO_PIN_PULL__NO)
  {
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  }

  GPIO_Init(gpio_hw_items[gpio_idx].gpio, &GPIO_InitStructure);

#elif defined(STM32F30X) || defined(STM32F4XX)
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = gpio_pin_hw_items[pin_idx].pin;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;

  if (pin_pull == GPIO_PIN_PULL__UP)
  {
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  }
  else if (pin_pull == GPIO_PIN_PULL__DOWN)
  {
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  }
  else if (pin_pull == GPIO_PIN_PULL__NO)
  {
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  }

  GPIO_Init(gpio_hw_items[gpio_idx].gpio, &GPIO_InitStructure);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin = gpio_pin_hw_items[pin_idx].pin;
  GPIO_InitStructure.Mode = LL_GPIO_MODE_INPUT;

  if (pin_pull == GPIO_PIN_PULL__UP)
  {
    GPIO_InitStructure.Pull = LL_GPIO_PULL_UP;
  }
  else if (pin_pull == GPIO_PIN_PULL__DOWN)
  {
    GPIO_InitStructure.Pull = LL_GPIO_PULL_DOWN;
  }
  else if (pin_pull == GPIO_PIN_PULL__NO)
  {
    GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;
  }

  LL_GPIO_Init(gpio_hw_items[gpio_idx].gpio, &GPIO_InitStructure);

#else
  #error This MCU not supported
#endif
}

void gpio_pin_cmd(uint8_t gpio_idx, uint8_t pin_idx, bool new_state)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT));

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  if (new_state == ENABLE)
  {
    GPIO_SetBits(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin);
  }
  else
  {
    GPIO_ResetBits(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin);
  }

#elif defined(STM32L4XX) || defined(STM32F0XX)
  if (new_state == ENABLE)
  {
    LL_GPIO_SetOutputPin(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin);
  }
  else
  {
    LL_GPIO_ResetOutputPin(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin);
  }

#else
  #error This MCU not supported
#endif
}

void gpio_pin_toggle(uint8_t gpio_idx, uint8_t pin_idx)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT));

#if defined(STM32F1XX) || defined(STM32F30X)
  // todo ideally a GPIO_ToggleBits function should be added to STD_PeriphLib
  gpio_hw_items[gpio_idx].gpio->ODR ^= gpio_pin_hw_items[pin_idx].pin;
#elif defined(STM32F4XX)
  GPIO_ToggleBits(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin);
#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_GPIO_TogglePin(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin);
#else
  #error This MCU not supported
#endif
}

volatile uint32_t *gpio_get_bsrr(uint8_t gpio_idx)
{
  hal_assert(gpio_idx < GPIO_AMOUNT);

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX) || defined(STM32L4XX) ||        \
    defined(STM32F0XX)
  return &gpio_hw_items[gpio_idx].gpio->BSRR;
#else
  #error This MCU not supported
#endif
}

bool gpio_pin_state(uint8_t gpio_idx, uint8_t pin_idx)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT));

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  return (GPIO_ReadInputDataBit(gpio_hw_items[gpio_idx].gpio, gpio_pin_hw_items[pin_idx].pin) != 0);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  uint32_t port_state = LL_GPIO_ReadInputPort(gpio_hw_items[gpio_idx].gpio);
  return ((port_state & gpio_pin_hw_items[pin_idx].pin) != 0);

#else
  #error This MCU not supported
#endif
}

void gpio_exti_init(uint8_t gpio_idx, uint8_t pin_idx, gpio_pin_pull_t pin_pull,
                    gpio_exti_edge_t exti_edge, bool need_irq)
{
  hal_assert((gpio_idx < GPIO_AMOUNT) && (pin_idx < GPIO_PIN_AMOUNT));
  hal_assert(pin_pull < GPIO_PIN_PULL__COUNT);
  hal_assert(exti_edge < GPIO_EXTI_EDGE__COUNT);

  gpio_pin_set_in_mode(gpio_idx, pin_idx, pin_pull);

#if defined(STM32F1XX)

  // the alternate function clock must be enabled for EXTI to work
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  GPIO_EXTILineConfig(gpio_hw_items[gpio_idx].gpio_port_src, gpio_pin_hw_items[pin_idx].pin_src);

  EXTI_InitTypeDef exti_init_struct;
  exti_init_struct.EXTI_Line = gpio_pin_hw_items[pin_idx].exti_line;
  exti_init_struct.EXTI_Mode = EXTI_Mode_Interrupt;
  if (exti_edge == GPIO_EXTI_EDGE__RISING)
  {
    exti_init_struct.EXTI_Trigger = EXTI_Trigger_Rising;
  }
  else if (exti_edge == GPIO_EXTI_EDGE__FALLING)
  {
    exti_init_struct.EXTI_Trigger = EXTI_Trigger_Falling;
  }
  else if (exti_edge == GPIO_EXTI_EDGE__BOTH)
  {
    exti_init_struct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  }
  exti_init_struct.EXTI_LineCmd = ENABLE;
  EXTI_Init(&exti_init_struct);

  if (need_irq)
  {
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel = gpio_pin_hw_items[pin_idx].exti_irq;
    nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 0x0F;
    nvic_init_struct.NVIC_IRQChannelSubPriority = 0x0F;
    nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init_struct);
  }

#elif defined(STM32F4XX)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  SYSCFG_EXTILineConfig(gpio_hw_items[gpio_idx].exti_port_src, gpio_pin_hw_items[pin_idx].exti_src);

  EXTI_InitTypeDef EXTI_InitStructure;
  EXTI_InitStructure.EXTI_Line = gpio_pin_hw_items[pin_idx].exti_line;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;

  if (exti_edge == GPIO_EXTI_EDGE__RISING)
  {
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  }
  else if (exti_edge == GPIO_EXTI_EDGE__FALLING)
  {
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  }
  else if (exti_edge == GPIO_EXTI_EDGE__BOTH)
  {
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  }
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  if (need_irq)
  {
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = gpio_pin_hw_items[pin_idx].exti_irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }

#elif defined(STM32L4XX) || defined(STM32F0XX) || defined(STM32F30X)
  HAL_UNUSED(gpio_idx);
  HAL_UNUSED(pin_idx);
  HAL_UNUSED(pin_pull);
  HAL_UNUSED(exti_edge);
  HAL_UNUSED(need_irq);
  hal_assert(false);

#else
  #error This MCU not supported
#endif
}

bool gpio_exti_check_status(uint8_t pin_idx)
{
  hal_assert(pin_idx < GPIO_PIN_AMOUNT);

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  return (EXTI_GetITStatus(gpio_pin_hw_items[pin_idx].exti_line) == SET);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  HAL_UNUSED(pin_idx);
  hal_assert(false);
  return false;

#else
  #error This MCU not supported
#endif
}

void gpio_exti_clear_status(uint8_t pin_idx)
{
  hal_assert(pin_idx < GPIO_PIN_AMOUNT);

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  EXTI_ClearITPendingBit(gpio_pin_hw_items[pin_idx].exti_line);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  HAL_UNUSED(pin_idx);
  hal_assert(false);

#else
  #error This MCU not supported
#endif
}
