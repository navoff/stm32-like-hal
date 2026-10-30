/**
 * @file uart_core_stm32.c
 * @brief STM32 (F1/F4/L4) UART core: peripheral, GPIO, DMA and IRQ setup.
 */

#include "config.h"

#include <stdbool.h>
#include <string.h>

#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "utils/assert.h"
#include "utils/errors.h"

#include "drivers/uart/uart_core.h"

typedef struct
{
  USART_TypeDef *uart;
  gpio_pin_mode_t gpio_pin_mode;
  IRQn_Type irqn;
} uart_hw_item_t;

#if !defined(FEATURE_ENABLE_UART1)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART1 0
#endif

#if !defined(FEATURE_ENABLE_UART2)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART2 0
#endif

#if !defined(FEATURE_ENABLE_UART3)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART3 0
#endif

#if !defined(FEATURE_ENABLE_UART4)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART4 0
#endif

#if !defined(FEATURE_ENABLE_UART5)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART5 0
#endif

#if !defined(FEATURE_ENABLE_UART6)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART6 0
#endif

#if !defined(FEATURE_ENABLE_UART7)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART7 0
#endif

#if !defined(FEATURE_ENABLE_UART8)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART8 0
#endif

uart_hw_item_t const uart_hw_items[UART_AMOUNT + 1] = {
    // fake USART0
    {.uart = 0},

#if (FEATURE_ENABLE_UART1)
    {.uart = USART1, .gpio_pin_mode = GPIO_MODE_USART1, .irqn = USART1_IRQn},
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART2)
    {.uart = USART2, .gpio_pin_mode = GPIO_MODE_USART2, .irqn = USART2_IRQn},
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART3)
    {.uart = USART3, .gpio_pin_mode = GPIO_MODE_USART3, .irqn = USART3_IRQn},
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART4)
    {.uart = UART4, .gpio_pin_mode = GPIO_MODE_UART4, .irqn = UART4_IRQn},
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART5)
    {.uart = UART5, .gpio_pin_mode = GPIO_MODE_UART5, .irqn = UART5_IRQn},
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART6)
    {.uart = USART6, .gpio_pin_mode = GPIO_MODE_USART6, .irqn = USART6_IRQn},
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART7)
    {.uart = UART7, .gpio_pin_mode = GPIO_MODE_UART7, .irqn = UART7_IRQn},
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART8)
    {.uart = UART8, .gpio_pin_mode = GPIO_MODE_UART8, .irqn = UART8_IRQn},
#else
    {.uart = 0},
#endif
};

uart_unit_t uart_units[UART_AMOUNT];

bool uart_core_is_need_over8(uint8_t uart_number, uint32_t baudrate);
void uart_core_rcc_cmd(uint8_t uart_number, bool new_state);

void uart_core_rx_irq_init(uart_unit_t *uart_unit, IRQn_Type IRQn);
void uart_core_tx_irq_init(IRQn_Type IRQn);

#if defined(STM32F1XX) || defined(STM32L4XX)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_channel_idx);
#elif defined(STM32F4XX)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_stream_idx,
                           uint8_t dma_channel_idx);
#endif

/**
 * @brief Fill the init structure with default values.
 *        The following values are set:
 *        - pins and DMA to the disabled (undefined) state
 *        - RS485 disabled and pauses set to zero
 * @param uart_init_struct - init structure
 */
void uart_core_struct_init(uart_init_struct_t *uart_init_struct)
{
  uart_init_struct->uart_number = UART_NUMBER_UNDEF;
  uart_init_struct->baudrate = 0;

  uart_init_struct->tx.mode = UART_LINE_MODE__IRQ;
  uart_init_struct->tx.buf = NULL;
  uart_init_struct->tx.buf_size = 0;
  uart_init_struct->tx.gpio_idx = GPIO_IDX_UNDEF;
  uart_init_struct->tx.pin_idx = GPIO_PIN_IDX_UNDEF;
  uart_init_struct->tx.dma_number = DMA_NUMBER_UNDEF;
  uart_init_struct->tx.dma_channel_idx = DMA_CHANNEL_IDX_UNDEF;

  uart_init_struct->rx.mode = UART_LINE_MODE__IRQ;
  uart_init_struct->rx.buf = NULL;
  uart_init_struct->rx.buf_size = 0;
  uart_init_struct->rx.gpio_idx = GPIO_IDX_UNDEF;
  uart_init_struct->rx.pin_idx = GPIO_PIN_IDX_UNDEF;
  uart_init_struct->rx.dma_number = DMA_NUMBER_UNDEF;
  uart_init_struct->rx.dma_channel_idx = DMA_CHANNEL_IDX_UNDEF;

  uart_init_struct->rs485.is_enable = false;
  uart_init_struct->rs485.dir_gpio_idx = GPIO_IDX_UNDEF;
  uart_init_struct->rs485.dir_pin_idx = GPIO_PIN_IDX_UNDEF;
  uart_init_struct->rs485.pause_before_dir_on = 0;
  uart_init_struct->rs485.pause_before_tx = 0;
  uart_init_struct->rs485.pause_after_tx = 0;
}

/**
 * @brief Initialize the UART
 * @param uart_unit - pointer to the UART driver instance
 * @param uart_init_struct - pointer to the settings structure
 */
void uart_core_init(uart_unit_t *uart_unit, uart_init_struct_t *uart_init_struct)
{
  hal_assert(uart_init_struct->rx.mode < UART_LINE_MODE__COUNT);
  hal_assert(uart_init_struct->tx.mode < UART_LINE_MODE__COUNT);
  hal_assert((uart_init_struct->uart_number > 0) && (uart_init_struct->uart_number <= UART_AMOUNT));

  // in this mode packets are very likely to be missed
  hal_assert(uart_init_struct->rx.mode != UART_LINE_MODE__POLLING);
  // this mode is not implemented yet
  hal_assert(uart_init_struct->tx.mode != UART_LINE_MODE__DMA);

  uart_hw_item_t const *uart_hw = &uart_hw_items[uart_init_struct->uart_number];

  hal_assert(uart_hw->uart != 0);

  uart_unit->UART = uart_hw->uart;

  fifo_uint8_init(&uart_unit->tx.fifo, uart_init_struct->tx.buf_size, uart_init_struct->tx.buf);
  fifo_uint8_init(&uart_unit->rx.fifo, uart_init_struct->rx.buf_size, uart_init_struct->rx.buf);

  gpio_rcc_cmd(uart_init_struct->tx.gpio_idx, true);
  gpio_rcc_cmd(uart_init_struct->rx.gpio_idx, true);

  gpio_pin_set_mode(uart_init_struct->tx.gpio_idx, uart_init_struct->tx.pin_idx,
                    uart_hw->gpio_pin_mode);
  gpio_pin_set_mode(uart_init_struct->rx.gpio_idx, uart_init_struct->rx.pin_idx,
                    uart_hw->gpio_pin_mode);

  uart_core_rcc_cmd(uart_init_struct->uart_number, true);

#if defined(STM32F1XX) || defined(STM32F4XX)
  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_BaudRate = uart_init_struct->baudrate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  if (uart_core_is_need_over8(uart_init_struct->uart_number, uart_init_struct->baudrate))
  {
    USART_OverSampling8Cmd(uart_unit->UART, ENABLE);
  }

  USART_Init(uart_unit->UART, &USART_InitStructure);

#elif defined(STM32L4XX)
  LL_USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.BaudRate = uart_init_struct->baudrate;
  USART_InitStructure.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStructure.StopBits = LL_USART_STOPBITS_1;
  USART_InitStructure.Parity = LL_USART_PARITY_NONE;
  USART_InitStructure.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStructure.TransferDirection = LL_USART_DIRECTION_TX_RX;

  if (uart_core_is_need_over8(uart_init_struct->uart_number, uart_init_struct->baudrate))
  {
    USART_InitStructure.OverSampling = LL_USART_OVERSAMPLING_8;
  }
  else
  {
    USART_InitStructure.OverSampling = LL_USART_OVERSAMPLING_16;
  }

  LL_USART_Init(uart_unit->UART, &USART_InitStructure);

#else
  #error This MCU not supported
#endif

  uart_unit->state = UART_STATE__READY_TX;

  // Initialize RS-485
  if (uart_init_struct->rs485.is_enable)
  {
    gpio_rcc_cmd(uart_init_struct->rs485.dir_gpio_idx, true);
    gpio_pin_set_mode(uart_init_struct->rs485.dir_gpio_idx, uart_init_struct->rs485.dir_pin_idx,
                      GPIO_MODE_OUTPUT_PP);

    uart_unit->state = UART_STATE__WAIT;

    memcpy(&uart_unit->rs485.settings, &uart_init_struct->rs485, sizeof(uart_init_rs485_struct_t));
  }

  uart_unit->rx.mode = uart_init_struct->rx.mode;
  if (uart_unit->rx.mode == UART_LINE_MODE__DMA)
  {
    uart_core_rx_dma_init(uart_unit, uart_init_struct->rx.dma_number,
#if defined(STM32F4XX)
                          uart_init_struct->rx.dma_stream_idx,
#endif
                          uart_init_struct->rx.dma_channel_idx);
  }
  else if (uart_unit->rx.mode == UART_LINE_MODE__IRQ)
  {
    uart_core_rx_irq_init(uart_unit, uart_hw->irqn);
  }
  else if (uart_unit->rx.mode == UART_LINE_MODE__POLLING)
  {
  }

  uart_unit->tx.mode = uart_init_struct->tx.mode;
  if (uart_unit->tx.mode == UART_LINE_MODE__DMA)
  {
    // TODO this mode is not implemented yet
    hal_assert(false);
    uart_unit->tx_started = 0;
    return;
  }
  else if (uart_unit->tx.mode == UART_LINE_MODE__IRQ)
  {
    uart_unit->tx_started = 0;
    uart_core_tx_irq_init(uart_hw->irqn);
  }
  else if (uart_unit->tx.mode == UART_LINE_MODE__POLLING)
  {
  }

#if defined(STM32F1XX) || defined(STM32F4XX)
  USART_Cmd(uart_unit->UART, ENABLE);
#elif defined(STM32L4XX)
  LL_USART_Enable(uart_unit->UART);
#else
  #error This MCU not supported
#endif

  uart_unit->inited = 1;
}

/**
 * @brief Reset the UART to its initial state.
 *        The DMA channels/streams in use are reset as well
 * @param uart_number - UART number (1 to UART_AMOUNT)
 */
void uart_core_deinit(uint8_t uart_number)
{
  hal_assert((uart_number > 0) && (uart_number <= UART_AMOUNT));

  uart_unit_t *uart_unit = &uart_units[uart_number - 1];

  uart_unit->inited = 0;

  NVIC_DisableIRQ(uart_hw_items[uart_number].irqn);

#if defined(STM32F1XX) || defined(STM32F4XX)
  USART_DeInit(uart_unit->UART);
#elif defined(STM32L4XX)
  LL_USART_DeInit(uart_unit->UART);
#else
  #error This MCU not supported
#endif

  if (uart_unit->rx.mode == UART_LINE_MODE__DMA)
  {
#if defined(STM32F1XX)
    DMA_DeInit(uart_unit->rx.dma_channel);
#elif defined(STM32F4XX)
    DMA_DeInit(uart_unit->rx.dma_stream);
#elif defined(STM32L4XX)
    LL_DMA_DeInit(uart_unit->rx.dma, uart_unit->rx.ll_dma_channel);
#else
  #error This MCU not supported
#endif
  }

  if (uart_unit->tx.mode == UART_LINE_MODE__DMA)
  {
#if defined(STM32F1XX)
    DMA_DeInit(uart_unit->tx.dma_channel);
#elif defined(STM32F4XX)
    DMA_DeInit(uart_unit->tx.dma_stream);
#elif defined(STM32L4XX)
    LL_DMA_DeInit(uart_unit->tx.dma, uart_unit->tx.ll_dma_channel);
#else
  #error This MCU not supported
#endif
  }
}

/**
 * @brief Check whether, for the given UART (bus clock) and the required baudrate, the UART
 *        divider has to be set to 8 (OVER8 mode) instead of the default 16.
 * @param uart_number - UART number (1 to UART_AMOUNT)
 * @param baudrate - required baudrate, bit/s
 * @return true - if OVER8 mode is required, false - otherwise
 */
bool uart_core_is_need_over8(uint8_t uart_number, uint32_t baudrate)
{
  uint32_t apbclock;

#if defined(STM32F4XX)
  RCC_ClocksTypeDef rcc_clock;
  RCC_GetClocksFreq(&rcc_clock);

  if ((uart_number == 1) || (uart_number == 6))
  {
    apbclock = rcc_clock.PCLK2_Frequency;
  }
  else
  {
    apbclock = rcc_clock.PCLK1_Frequency;
  }

#elif defined(STM32L4XX)
  LL_RCC_ClocksTypeDef rcc_clock;
  LL_RCC_GetSystemClocksFreq(&rcc_clock);

  if (uart_number == 1)
  {
    apbclock = rcc_clock.PCLK2_Frequency;
  }
  else
  {
    apbclock = rcc_clock.PCLK1_Frequency;
  }

#elif defined(STM32F1XX)
  RCC_ClocksTypeDef rcc_clock;
  RCC_GetClocksFreq(&rcc_clock);

  // this family has no such UARTs
  hal_assert(uart_number != 6);

  if (uart_number == 1)
  {
    apbclock = rcc_clock.PCLK2_Frequency;
  }
  else
  {
    apbclock = rcc_clock.PCLK1_Frequency;
  }
#else
  #error This MCU not supported
#endif

  bool res = false;

  // the magic number 16 is the default built-in divider of the MCU UART unit
  uint32_t integer_divider = apbclock / 16 / baudrate;
  if (integer_divider == 0)
  {
    res = true;
  }

  return res;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
/**
 * @brief Control the UART clock
 * @param uart_number - UART number (1 to UART_AMOUNT)
 * @param new_state - clock line state
 */
void uart_core_rcc_cmd(uint8_t uart_number, bool new_state)
{
  hal_assert((uart_number > 0) && (uart_number <= UART_AMOUNT));

#if defined(STM32F1XX) || defined(STM32F4XX)
  switch (uart_number)
  {
  #if (FEATURE_ENABLE_UART1)
    case 1:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_UART2)
    case 2:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_UART3)
    case 3:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_UART4)
    case 4:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_UART5)
    case 5:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_UART6)
    case 6:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_UART7)
    case 7:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART7, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_UART8)
    case 8:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART8, new_state);
      break;
  #endif
  }

#elif defined(STM32L4XX)
  if (new_state)
  {
    switch (uart_number)
    {
  #if (FEATURE_ENABLE_UART1)
      case 1:
        __HAL_RCC_USART1_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART2)
      case 2:
        __HAL_RCC_USART2_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART3)
      case 3:
        __HAL_RCC_USART3_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART4)
      case 4:
        __HAL_RCC_UART4_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART5)
      case 5:
        __HAL_RCC_UART5_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART6)
      case 6:
        __HAL_RCC_USART6_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART7)
      case 7:
        __HAL_RCC_UART7_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART8)
      case 8:
        __HAL_RCC_UART8_CLK_ENABLE();
        break;
  #endif
    }
  }
  else
  {
    switch (uart_number)
    {
  #if (FEATURE_ENABLE_UART1)
      case 1:
        __HAL_RCC_USART1_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART2)
      case 2:
        __HAL_RCC_USART2_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART3)
      case 3:
        __HAL_RCC_USART3_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART4)
      case 4:
        __HAL_RCC_UART4_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART5)
      case 5:
        __HAL_RCC_UART5_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART6)
      case 6:
        __HAL_RCC_USART6_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART7)
      case 7:
        __HAL_RCC_UART7_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_UART8)
      case 8:
        __HAL_RCC_UART8_CLK_DISABLE();
        break;
  #endif
    }
  }
#else
  #error This MCU not supported
#endif
}
#pragma GCC diagnostic pop

/**
 * @brief Initialize interrupt-driven reception
 * @param uart_unit - pointer to the UART driver instance
 * @param IRQn - interrupt
 */
void uart_core_rx_irq_init(uart_unit_t *uart_unit, IRQn_Type IRQn)
{
#if defined(STM32F1XX) || defined(STM32F4XX)
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

#elif defined(STM32L4XX)
  NVIC_EnableIRQ(IRQn);
  NVIC_SetPriority(IRQn, 0);

#else
  #error This MCU not supported
#endif

  UART_IT_RXNE_ENABLE(uart_unit->UART);
}

/**
 * @brief Initialize interrupt-driven transmission
 * @param uart_unit - pointer to the UART driver instance
 * @param IRQn - interrupt
 */
void uart_core_tx_irq_init(IRQn_Type IRQn)
{
#if defined(STM32F1XX) || defined(STM32F4XX)
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

#elif defined(STM32L4XX)
  NVIC_EnableIRQ(IRQn);
  NVIC_SetPriority(IRQn, 0);

#else
  #error This MCU not supported
#endif
}

#if defined(STM32F1XX)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_channel_idx)
{
  check_dma_number(dma_number);
  hal_assert(dma_channel_idx < DMA_CHANNEL_AMOUNT);

  dma_rcc_cmd(dma_number, true);

  uart_unit->rx.dma_channel = dma_channel_hw_items[dma_number][dma_channel_idx].channel;
  hal_assert(uart_unit->rx.dma_channel != 0);

  DMA_DeInit(uart_unit->rx.dma_channel);

  DMA_InitTypeDef dma_init_structure;
  dma_init_structure.DMA_BufferSize = (uint16_t)uart_unit->rx.fifo.size;
  dma_init_structure.DMA_DIR = DMA_DIR_PeripheralSRC;
  dma_init_structure.DMA_Mode = DMA_Mode_Circular;
  dma_init_structure.DMA_MemoryBaseAddr = (uint32_t)uart_unit->rx.fifo.buf;
  dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t)&(uart_unit->UART->DR);
  dma_init_structure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  dma_init_structure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma_init_structure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma_init_structure.DMA_M2M = DMA_M2M_Disable;
  dma_init_structure.DMA_Priority = DMA_Priority_High;
  DMA_Init(uart_unit->rx.dma_channel, &dma_init_structure);

  USART_DMACmd(uart_unit->UART, USART_DMAReq_Rx, ENABLE);

  DMA_Cmd(uart_unit->rx.dma_channel, ENABLE);
}

#elif defined(STM32F4XX)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_stream_idx,
                           uint8_t dma_channel_idx)
{
  check_dma_number(dma_number);
  hal_assert(dma_stream_idx < DMA_STREAM_AMOUNT);
  hal_assert(dma_channel_idx < DMA_CHANNEL_AMOUNT);

  dma_rcc_cmd(dma_number, true);

  uart_unit->rx.dma_stream = dma_stream_hw_items[dma_number][dma_stream_idx].stream;
  DMA_DeInit(uart_unit->rx.dma_stream);

  DMA_InitTypeDef dma_init_structure;
  dma_init_structure.DMA_Channel = dma_channels[dma_channel_idx];
  dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t)&uart_unit->UART->DR;
  dma_init_structure.DMA_Memory0BaseAddr = (uint32_t)uart_unit->rx.fifo.buf;
  dma_init_structure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  dma_init_structure.DMA_BufferSize = (uint32_t)uart_unit->rx.fifo.size;
  dma_init_structure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma_init_structure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  dma_init_structure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  dma_init_structure.DMA_Mode = DMA_Mode_Circular;
  dma_init_structure.DMA_Priority = DMA_Priority_High;
  dma_init_structure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  dma_init_structure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  dma_init_structure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  dma_init_structure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(uart_unit->rx.dma_stream, &dma_init_structure);

  USART_DMACmd(uart_unit->UART, USART_DMAReq_Rx, ENABLE);

  DMA_Cmd(uart_unit->rx.dma_stream, ENABLE);
}

#elif defined(STM32L4XX)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_channel_idx)
{
  check_dma_number(dma_number);
  hal_assert(dma_channel_idx < DMA_CHANNEL_AMOUNT);

  dma_rcc_cmd(dma_number, true);

  uart_unit->rx.dma = dma_get_by_number(dma_number);
  hal_assert(uart_unit->rx.dma != NULL);

  uart_unit->rx.ll_dma_channel = dma_channel_hw_items[dma_number][dma_channel_idx].ll_channel;
  hal_assert(uart_unit->rx.ll_dma_channel != 0);
  LL_DMA_DeInit(uart_unit->rx.dma, uart_unit->rx.ll_dma_channel);

  LL_DMA_InitTypeDef dma_init_structure;
  dma_init_structure.NbData = (uint16_t)uart_unit->rx.fifo.size;
  dma_init_structure.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  dma_init_structure.Mode = LL_DMA_MODE_CIRCULAR;
  dma_init_structure.MemoryOrM2MDstAddress = (uint32_t)uart_unit->rx.fifo.buf;
  dma_init_structure.PeriphOrM2MSrcAddress = (uint32_t)&(uart_unit->UART->RDR);
  dma_init_structure.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
  dma_init_structure.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
  dma_init_structure.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
  dma_init_structure.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
  dma_init_structure.PeriphRequest = LL_DMA_REQUEST_2; // the same for all STM32L4x5/6
  dma_init_structure.Priority = LL_DMA_PRIORITY_HIGH;

  LL_DMA_Init(uart_unit->rx.dma, uart_unit->rx.ll_dma_channel, &dma_init_structure);

  LL_USART_EnableDMAReq_RX(uart_unit->UART);

  LL_DMA_EnableChannel(uart_unit->rx.dma, uart_unit->rx.ll_dma_channel);
}
#endif

/**
 * @brief Handle UART line errors
 * @param uart_unit - UART driver instance
 * @param sr - value of the UARTx->SR status register
 */
void uart_core_error_handler(uart_unit_t *uart_unit, uint16_t sr)
{
  if (uart_unit->inited == 0)
  {
    return;
  }

#if (UNIT_TEST_ERROR_ORE)
  unit_test_uart_ore_process(sr);
#endif

// Error handling
#if (defined(STM32F4XX) || defined(STM32F1XX))
  if (sr & USART_FLAG_ORE)
  {
    uart_unit->error_status = sr;
    USART_ReceiveData(uart_unit->UART);
    USART_ClearFlag(uart_unit->UART, USART_FLAG_ORE);
  }

  // STM32F1xx & STM32F4xx: an FE error does not generate an interrupt,
  // since it appears together with the RXNE flag, which generates the interrupt itself.
  if (sr & USART_FLAG_FE)
  {
    uart_unit->error_status = sr;
    USART_ReceiveData(uart_unit->UART);
    USART_ClearFlag(uart_unit->UART, USART_FLAG_FE);
  }

  if (sr & USART_FLAG_PE)
  {
    uart_unit->error_status = sr;
    USART_ReceiveData(uart_unit->UART);
    USART_ClearFlag(uart_unit->UART, USART_FLAG_PE);
  }

#elif defined(STM32L4XX)
  if (sr & LL_USART_ISR_ORE)
  {
    uart_unit->error_status = sr;
    LL_USART_ReceiveData8(uart_unit->UART);
    LL_USART_ClearFlag_ORE(uart_unit->UART);
  }

  // STM32L4xx: an FE error does not generate an interrupt,
  // since it appears together with the RXNE flag, which generates the interrupt itself.
  // TODO but this has not been verified on hardware
  if (sr & LL_USART_ISR_FE)
  {
    uart_unit->error_status = sr;
    LL_USART_ReceiveData8(uart_unit->UART);
    LL_USART_ClearFlag_FE(uart_unit->UART);
  }

  if (sr & LL_USART_ISR_PE)
  {
    uart_unit->error_status = sr;
    LL_USART_ReceiveData8(uart_unit->UART);
    LL_USART_ClearFlag_PE(uart_unit->UART);
  }
#else
  #warning "UART errors not processing for this MCU!"
#endif
}

/**
 * @brief Send a byte from the FIFO in polling mode
 * @param uart_unit - pointer to the UART driver instance
 * @return ERR_OK - byte sent successfully, otherwise - an error
 */
error_t uart_core_tx_polling_fifo_byte(uart_unit_t *uart_unit)
{
  error_t res = ERR_RESULT;

  if (UART_GET_SR(uart_unit->UART) & USART_SR_TXE)
  {
    uint8_t data;
    if (fifo_uint8_get((fifo_uint8_t *)&uart_unit->tx.fifo, &data) == ERR_OK)
    {
      UART_GET_TDR(uart_unit->UART) = data;
      res = ERR_OK;
    }
  }
  return res;
}

/**
 * @brief Send the first byte from the FIFO using interrupts
 * @note The caller must make sure an interrupt-driven transmission has not been started earlier!
 * @param uart_unit - pointer to the UART driver instance
 * @return ERR_OK - byte sent successfully, otherwise - an error
 */
error_t uart_core_tx_irq_first_fifo_byte(uart_unit_t *uart_unit)
{
  error_t res = ERR_RESULT;

  if (UART_GET_SR(uart_unit->UART) & USART_SR_TXE)
  {
    uint8_t data;
    if (fifo_uint8_get((fifo_uint8_t *)&uart_unit->tx.fifo, &data) == ERR_OK)
    {
      UART_IT_TXE_ENABLE(uart_unit->UART);
      UART_GET_TDR(uart_unit->UART) = data;
      res = ERR_OK;
    }
  }

  return res;
}
