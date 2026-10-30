/**
 * @file uart_core_gd32.c
 * @brief GD32 (E11x/L23x) UART core: peripheral, GPIO, DMA and IRQ setup.
 */

#include "config.h"

#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "utils/assert.h"
#include "utils/errors.h"
#include <stddef.h>

#include "drivers/uart/uart_core.h"

#if !defined(FEATURE_ENABLE_UART0)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_UART0 0
#endif

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

#if defined(GD32L23X)
  #if !defined(FEATURE_ENABLE_LPUART)
    #error Need to define in config-file:   \
      #define FEATURE_ENABLE_LPUART 0
  #endif
#endif

typedef struct
{
  /// UART instance UARTx (x=0,1,...)
  uint32_t uart;
  gpio_pin_mode_t gpio_pin_mode;
  rcu_periph_enum rcu_periph;
  IRQn_Type irqn;
#if defined(GD32L23X)
  uint32_t dma_request_tx;
  uint32_t dma_request_rx;
#endif
} uart_hw_item_t;

uart_hw_item_t const uart_hw_items[UART_AMOUNT] = {
#if (FEATURE_ENABLE_UART0)
    {
        .uart = USART0,
        .gpio_pin_mode = GPIO_MODE_USART0,
        .rcu_periph = RCU_USART0,
        .irqn = USART0_IRQn,
  #if defined(GD32L23X)
        .dma_request_tx = DMA_REQUEST_USART0_TX,
        .dma_request_rx = DMA_REQUEST_USART0_RX,
  #endif
    },
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART1)
    {
        .uart = USART1,
        .gpio_pin_mode = GPIO_MODE_USART1,
        .rcu_periph = RCU_USART1,
        .irqn = USART1_IRQn,
  #if defined(GD32L23X)
        .dma_request_tx = DMA_REQUEST_USART1_TX,
        .dma_request_rx = DMA_REQUEST_USART1_RX,
  #endif
    },
#else
    {.uart = 0},
#endif

#if defined(GD32E11X)
  #if (FEATURE_ENABLE_UART2)
    {
        .uart = USART2,
        .gpio_pin_mode = GPIO_MODE_USART2,
        .rcu_periph = RCU_USART2,
        .irqn = USART2_IRQn,
    },
  #else
    {.uart = 0},
  #endif
#elif defined(GD32L23X)
  #if (FEATURE_ENABLE_UART2)
    #error UART2 is absent on this MCU
  #else
    {.uart = 0},
  #endif
#else
  #error This MCU not supported
#endif

#if (FEATURE_ENABLE_UART3)
    {
        .uart = UART3,
        .gpio_pin_mode = GPIO_MODE_UART3,
        .rcu_periph = RCU_UART3,
        .irqn = UART3_IRQn,
  #if defined(GD32L23X)
        .dma_request_tx = DMA_REQUEST_UART3_TX,
        .dma_request_rx = DMA_REQUEST_UART3_RX,
  #endif
    },
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART4)
    {
        .uart = UART4,
        .gpio_pin_mode = GPIO_MODE_UART4,
        .rcu_periph = RCU_UART4,
        .irqn = UART4_IRQn,
  #if defined(GD32L23X)
        .dma_request_tx = DMA_REQUEST_UART4_TX,
        .dma_request_rx = DMA_REQUEST_UART4_RX,
  #endif
    },
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART5)
  #error Not supported
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART6)
  #error Not supported
#else
    {.uart = 0},
#endif

#if (FEATURE_ENABLE_UART7)
  #error Not supported
#else
    {.uart = 0},
#endif
};

uart_unit_t uart_units[UART_AMOUNT];

bool uart_core_is_need_over8(uint8_t uart_number, uint32_t baudrate);
void uart_core_rcc_cmd(uint8_t uart_number, bool new_state);

void uart_core_rx_irq_init(uart_unit_t *uart_unit, IRQn_Type IRQn);
void uart_core_tx_irq_init(IRQn_Type IRQn);

#if defined(GD32E11X)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_channel_idx);
#elif defined(GD32L23X)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_channel_idx,
                           uint32_t dma_request_rx);
#else
  #error This MCU not supported
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
  hal_assert(uart_init_struct->uart_number < UART_AMOUNT);
  hal_assert(uart_init_struct->rx.mode < UART_LINE_MODE__COUNT);
  hal_assert(uart_init_struct->tx.mode < UART_LINE_MODE__COUNT);

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

#if defined(GD32E11X) || defined(GD32L23X)
  usart_deinit(uart_unit->UART);
  usart_word_length_set(uart_unit->UART, USART_WL_8BIT);
  usart_parity_config(uart_unit->UART, USART_PM_NONE);
  usart_stop_bit_set(uart_unit->UART, USART_STB_1BIT);
  usart_baudrate_set(uart_unit->UART, uart_init_struct->baudrate);
  usart_receive_config(uart_unit->UART, USART_RECEIVE_ENABLE);
  usart_transmit_config(uart_unit->UART, USART_TRANSMIT_ENABLE);
#else
  #error This MCU not supported
#endif

  uart_unit->state = UART_STATE__READY_TX;

  // Initialize RS-485
  if (uart_init_struct->rs485.is_enable)
  {
    // not supported yet
    hal_assert(false);
  }

  uart_unit->rx.mode = uart_init_struct->rx.mode;
  if (uart_unit->rx.mode == UART_LINE_MODE__DMA)
  {
#if defined(GD32E11X)
    uart_core_rx_dma_init(uart_unit, uart_init_struct->rx.dma_number,
                          uart_init_struct->rx.dma_channel_idx);
#elif defined(GD32L23X)
    uart_core_rx_dma_init(uart_unit, uart_init_struct->rx.dma_number,
                          uart_init_struct->rx.dma_channel_idx, uart_hw->dma_request_rx);
#else
  #error This MCU not supported
#endif
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

#if defined(GD32E11X) || defined(GD32L23X)
  usart_enable(uart_unit->UART);
#else
  #error This MCU not supported
#endif

  uart_unit->inited = 1;
}

/**
 * @brief Reset the UART to its initial state.
 *        The DMA channels/streams in use are reset as well
 * @param uart_number - UART number (0 to UART_AMOUNT-1)
 */
void uart_core_deinit(uint8_t uart_number)
{
  hal_assert(uart_number < UART_AMOUNT);

  uart_hw_item_t const *uart_hw = &uart_hw_items[uart_number];
  hal_assert(uart_hw->uart != 0);

  uart_unit_t *uart_unit = &uart_units[uart_number];

  uart_unit->inited = 0;

  NVIC_DisableIRQ(uart_hw->irqn);

#if defined(GD32L23X) || defined(GD32E11X)
  usart_deinit(uart_unit->UART);
#else
  #error This MCU not supported
#endif
  uart_core_rcc_cmd(uart_number, false);

  if (uart_unit->rx.mode == UART_LINE_MODE__DMA)
  {
#if defined(GD32L23X) || defined(GD32E11X)
    dma_deinit(uart_unit->rx.dma_channel);
#else
  #error This MCU not supported
#endif
  }

  if (uart_unit->tx.mode == UART_LINE_MODE__DMA)
  {
#if defined(GD32L23X) || defined(GD32E11X)
    dma_deinit(uart_unit->tx.dma_channel);
#else
  #error This MCU not supported
#endif
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
/**
 * @brief Control the UART clock
 * @param uart_number - UART number from 0 to (UART_AMOUNT - 1)
 * @param new_state - clock line state
 */
void uart_core_rcc_cmd(uint8_t uart_number, bool new_state)
{
  hal_assert(uart_number < UART_AMOUNT);

  uart_hw_item_t const *uart_hw = &uart_hw_items[uart_number];
  hal_assert(uart_hw->uart != 0);

#if defined(GD32E11X) || defined(GD32L23X)
  if (new_state)
  {
    rcu_periph_clock_enable(uart_hw->rcu_periph);
  }
  else
  {
    rcu_periph_clock_disable(uart_hw->rcu_periph);
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
#if defined(GD32E11X)
  nvic_irq_enable(IRQn, 0, 0);
#elif defined(GD32L23X)
  nvic_irq_enable(IRQn, 0);
#else
  #error This MCU not supported
#endif
  UART_IT_RXNE_ENABLE(uart_unit->UART);
}

/**
 * @brief Initialize interrupt-driven transmission
 * @param IRQn - interrupt
 */
void uart_core_tx_irq_init(IRQn_Type IRQn)
{
#if defined(GD32E11X)
  nvic_irq_enable(IRQn, 0, 0);
#elif defined(GD32L23X)
  nvic_irq_enable(IRQn, 0);
#else
  #error This MCU not supported
#endif
}

#if defined(GD32E11X)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_channel_idx)
#elif defined(GD32L23X)
void uart_core_rx_dma_init(uart_unit_t *uart_unit, uint8_t dma_number, uint8_t dma_channel_idx,
                           uint32_t dma_request_rx)
#else
  #error This MCU not supported
#endif
{
  check_dma_number(dma_number);
  hal_assert(dma_channel_idx < DMA_CHANNEL_AMOUNT);

  dma_rcc_cmd(dma_number, true);

  uint32_t dma = dma_hw_items[dma_number].dma;
  hal_assert(dma != 0);
  uart_unit->rx.dma = dma;

  dma_channel_enum dma_channel = dma_channel_hw_items[dma_number][dma_channel_idx].channel;
  hal_assert(dma_channel != DMA_CHANNEL_UNDEF);

  uart_unit->rx.dma_channel = dma_channel;

#if defined(GD32E11X)
  dma_deinit(dma, dma_channel);
#elif defined(GD32L23X)
  dma_deinit(dma_channel);
#else
  #error This MCU not supported
#endif

  dma_parameter_struct dma_parameter = {0};
  dma_struct_para_init(&dma_parameter);
  dma_parameter.periph_addr = (uint32_t)(&UART_GET_RDR(uart_unit->UART));
  dma_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
  dma_parameter.memory_addr = (uint32_t)uart_unit->rx.fifo.buf;
  dma_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;
  dma_parameter.number = (uint32_t)uart_unit->rx.fifo.size;
  dma_parameter.priority = DMA_PRIORITY_MEDIUM;
  dma_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
  dma_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
  dma_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
#if defined(GD32L23X)
  dma_parameter.request = dma_request_rx;
#endif

#if defined(GD32E11X)
  dma_init(dma, dma_channel, &dma_parameter);
  dma_circulation_enable(dma, dma_channel);
#elif defined(GD32L23X)
  dma_init(dma_channel, &dma_parameter);
  dma_circulation_enable(dma_channel);
#else
  #error This MCU not supported
#endif

  usart_dma_receive_config(uart_unit->UART, USART_RECEIVE_DMA_ENABLE);

#if defined(GD32E11X)
  dma_channel_enable(dma, dma_channel);
#elif defined(GD32L23X)
  dma_channel_enable(dma_channel);
#else
  #error This MCU not supported
#endif
}

/**
 * @brief Handle UART line errors
 * @param uart_unit - UART driver instance
 * @param sr - status register value
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
#if defined(GD32E11X) || defined(GD32L23X)

  #if defined(GD32E11X)
  if (sr & USART_STAT0_ORERR)
  #elif defined(GD32L23X)
  if (sr & USART_STAT_ORERR)
  #else
    #error This MCU not supported
  #endif
  {
    uart_unit->error_status = sr;
    usart_data_receive(uart_unit->UART);
    usart_flag_clear(uart_unit->UART, USART_FLAG_ORERR);
  }

  // An FE error does not generate an interrupt,
  // since it appears together with the RXNE flag, which generates the interrupt itself.
  // TODO taken from STM32F1xx, but not verified on hardware
  #if defined(GD32E11X)
  if (sr & USART_STAT0_FERR)
  #elif defined(GD32L23X)
  if (sr & USART_STAT_FERR)
  #else
    #error This MCU not supported
  #endif
  {
    uart_unit->error_status = sr;
    usart_data_receive(uart_unit->UART);
    usart_flag_clear(uart_unit->UART, USART_FLAG_FERR);
  }

  #if defined(GD32E11X)
  if (sr & USART_STAT0_PERR)
  #elif defined(GD32L23X)
  if (sr & USART_STAT_PERR)
  #else
    #error This MCU not supported
  #endif
  {
    uart_unit->error_status = sr;
    usart_data_receive(uart_unit->UART);
    usart_flag_clear(uart_unit->UART, USART_FLAG_PERR);
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
  uint32_t sr = UART_GET_SR(uart_unit->UART);

  if (UART_IS_TX_EMPTY(sr))
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
  uint32_t sr = UART_GET_SR(uart_unit->UART);

  if (UART_IS_TX_EMPTY(sr))
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
