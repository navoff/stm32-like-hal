/**
 * @file uart.c
 * @brief UART driver: RS-485 state machine, Tx/Rx processing and the public API.
 */

#include "config.h"

#include <stdio.h>

#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "drivers/tick_timer.h"
#include "drivers/uart/uart_core.h"
#include "utils/assert.h"
#include "utils/unit_tests.h"

#include "drivers/uart/uart.h"

void uart_process(uart_unit_t *uart_unit);
void uart_irq_handler(uart_unit_t *uart_unit);

void uart_rs485_process(uart_unit_t *uart_unit);
void uart_rs485_tx_initiate(uart_unit_t *uart_unit);

bool uart_is_can_tx(uart_unit_t *uart_unit);

void uart_tx_initiate(uart_unit_t *uart_unit);

void uart_rx_process(uart_unit_t *uart_unit);
void uart_tx_process(uart_unit_t *uart_unit);

/**
 * @brief Process the state of all UARTs
 */
void uart_all_process(void)
{
  for (uint8_t i = 0; i < UART_UNITS_AMOUNT; i++)
  {
    uart_process(&uart_units[i]);
  }
}

/**
 * @brief Main UART service function
 * @param uart_unit - pointer to the UART driver instance
 */
void uart_process(uart_unit_t *uart_unit)
{
  if (uart_unit->inited == 0)
    return;

  if ((uart_unit->rx.mode != UART_LINE_MODE__IRQ) && (uart_unit->tx.mode != UART_LINE_MODE__IRQ))
  {
    uint16_t sr = UART_GET_SR(uart_unit->UART);

#if (UNIT_TEST_ERROR_ORE)
    unit_test_uart_ore_process(sr);
#endif

    uart_core_error_handler(uart_unit, sr);
  }

  if (uart_unit->rs485.settings.is_enable)
  {
    uart_rs485_process(uart_unit);
  }

  uart_tx_process(uart_unit);

  uart_rx_process(uart_unit);
}

/**
 * @brief Main UART service function
 * @param uart_number - UART number (as in the datasheet)
 */
void uart_x_process(uint8_t uart_number)
{
  check_uart_number(uart_number);

  uart_process(&uart_units[uart_number]);
}

/**
 * @brief Process the RS-485 state
 * @param uart_unit - pointer to the UART driver instance
 */
void uart_rs485_process(uart_unit_t *uart_unit)
{
  uart_rs485_t *rs485 = &uart_unit->rs485;

uart_rs485_process_sm:

  switch (uart_unit->state)
  {
    case UART_STATE__WAIT:
      if (rs485->event == UART_RS485_EVENT__TX_INITIATE)
      {
        rs485->event = UART_RS485_EVENT__NONE;
        START_TIMER(rs485->timestamp_ms);

        uart_unit->state = UART_STATE__PAUSE_BEFORE_DIR;
        goto uart_rs485_process_sm;
      }
      break;

    case UART_STATE__PAUSE_BEFORE_DIR:
      if (CHECK_TIMER(rs485->timestamp_ms, rs485->settings.pause_before_dir_on))
      {
        START_TIMER(rs485->timestamp_ms);
        gpio_pin_cmd(rs485->settings.dir_gpio_idx, rs485->settings.dir_pin_idx, true);

        uart_unit->state = UART_STATE__PAUSE_BEFORE_TX;
        goto uart_rs485_process_sm;
      }
      break;

    case UART_STATE__PAUSE_BEFORE_TX:
      if (CHECK_TIMER(rs485->timestamp_ms, rs485->settings.pause_before_tx))
      {
        START_TIMER(rs485->timestamp_ms);
        uart_unit->state = UART_STATE__READY_TX;
        goto uart_rs485_process_sm;
      }
      break;

    case UART_STATE__READY_TX:

      if (rs485->event == UART_RS485_EVENT__TX_INITIATE)
      {
        rs485->event = UART_RS485_EVENT__NONE;
      }

      // Since in RS-485 mode the transmission is started from the superloop, make sure there is
      // no more data waiting to be sent before moving to the UART_STATE__PAUSE_AFTER_TX state.
      // Important for rs485->settings.pause_after_tx = 0
      if (CHECK_TIMER(rs485->timestamp_ms, rs485->settings.pause_after_tx) &&
          (uart_unit->tx.fifo.idx == 0))
      {
        uart_unit->state = UART_STATE__PAUSE_AFTER_TX;
        goto uart_rs485_process_sm;
      }
      break;

    case UART_STATE__TX:
      // Wait until all pending bytes are transmitted
      break;

    case UART_STATE__PAUSE_AFTER_TX:
      gpio_pin_cmd(rs485->settings.dir_gpio_idx, rs485->settings.dir_pin_idx, false);

      uart_unit->state = UART_STATE__WAIT;
      break;
  }
}

/**
 * @brief Initiate the RS-485 transmission process
 * @param uart_unit - pointer to the UART driver instance
 */
void uart_rs485_tx_initiate(uart_unit_t *uart_unit)
{
  uart_unit->rs485.event = UART_RS485_EVENT__TX_INITIATE;
}

/**
 * @brief Result of preparing the RS-485 line for data transmission
 * @param uart_unit - pointer to the UART driver instance
 * @return true - line is ready, false - not ready
 */
bool uart_is_can_tx(uart_unit_t *uart_unit)
{
  return ((uart_unit->state == UART_STATE__TX) || (uart_unit->state == UART_STATE__READY_TX));
}

/**
 * @brief UART interrupt handler
 * @param uart_unit - pointer to the UART driver instance
 */
void uart_irq_handler(uart_unit_t *uart_unit)
{
  uint32_t sr = UART_GET_SR(uart_unit->UART);

  uart_core_error_handler(uart_unit, sr);

  if (uart_unit->rx.mode == UART_LINE_MODE__IRQ)
  {
    if (UART_IS_RX_NOT_EMPTY(sr))
    {
#if (UNIT_TEST_ERROR_ORE)
      unit_test_uart_ore_cause();
#endif

      uint8_t data = UART_GET_RDR(uart_unit->UART);
      fifo_uint8_put((fifo_uint8_t *)&uart_unit->rx.fifo, data);
    }
  }

  if (uart_unit->tx.mode == UART_LINE_MODE__IRQ)
  {
    if (UART_IS_TX_BUF_EMPTY_INT_EN(uart_unit->UART) && UART_IS_TX_EMPTY(sr))
    {
      uint8_t data;
      if (fifo_uint8_get((fifo_uint8_t *)&uart_unit->tx.fifo, &data) == ERR_OK)
      {
        UART_GET_TDR(uart_unit->UART) = data;
      }
      else
      {
        UART_IT_TXE_DISABLE(uart_unit->UART);
        UART_IT_TC_ENABLE(uart_unit->UART);
      }
    }

    if (UART_IS_TX_COMPLETE_INT_EN(uart_unit->UART) && UART_IS_TX_COMPLETE(sr))
    {
      UART_CLEAR_TC_FLAG(uart_unit->UART);
      UART_IT_TC_DISABLE(uart_unit->UART);

      if (uart_unit->tx.fifo.idx == 0)
      {
        if (uart_unit->rs485.settings.is_enable)
        {
          START_TIMER(uart_unit->rs485.timestamp_ms);
        }

        uart_unit->state = UART_STATE__READY_TX;
      }
      else
      {
        UART_IT_TXE_ENABLE(uart_unit->UART);
      }
    }
  }
}

/**
 * @brief Main process that controls data transmission
 * @param uart_unit - pointer to the UART driver instance
 */
void uart_tx_process(uart_unit_t *uart_unit)
{
  if (uart_is_can_tx(uart_unit) == false)
  {
    return;
  }

  if (uart_unit->tx.mode == UART_LINE_MODE__POLLING)
  {
    while (uart_unit->tx.fifo.idx > 0)
    {
      uart_unit->state = UART_STATE__TX;

      uart_core_tx_polling_fifo_byte(uart_unit);
    }

    if (uart_unit->rs485.settings.is_enable)
    {
      START_TIMER(uart_unit->rs485.timestamp_ms);
    }

    uart_unit->state = UART_STATE__READY_TX;
  }
  else if (uart_unit->tx.mode == UART_LINE_MODE__IRQ)
  {
    if (uart_unit->rs485.settings.is_enable)
    {
      if (uart_unit->state == UART_STATE__READY_TX)
      {
        uart_tx_initiate(uart_unit);
      }
    }

    // Transmission is controlled from the interrupt
  }
  else if (uart_unit->tx.mode == UART_LINE_MODE__DMA)
  {
    // todo Not implemented yet
    hal_assert(0);
  }
}

/**
 * @brief Process UART reception
 * @param uart_unit - pointer to the UART driver instance
 */
void uart_rx_process(uart_unit_t *uart_unit)
{
  if (uart_unit->rx.mode == UART_LINE_MODE__DMA)
  {
    fifo_uint8_t *fifo = (fifo_uint8_t *)&uart_unit->rx.fifo;

#if defined(STM32F1XX)
    uint16_t dma_ndtr = uart_unit->rx.dma_channel->CNDTR;
#elif defined(STM32F4XX)
    uint16_t dma_ndtr = uart_unit->rx.dma_stream->NDTR;
#elif defined(STM32L4XX)
    uint16_t dma_ndtr = LL_DMA_GetDataLength(uart_unit->rx.dma, uart_unit->rx.ll_dma_channel);
#elif defined(GD32E11X)
    uint16_t dma_ndtr = dma_transfer_number_get(uart_unit->rx.dma, uart_unit->rx.dma_channel);
#elif defined(GD32L23X)
    uint16_t dma_ndtr = dma_transfer_number_get(uart_unit->rx.dma_channel);
#else
  #error This MCU not supported
#endif

    if (fifo_uint8_rx_dma_circular_update(fifo, dma_ndtr) == ERR_BOF)
    {
      uart_unit->rx.was_overflow = 1;
    }
  }
}

/**
 * @brief Start the UART data transmission process
 * @param uart_unit - pointer to the UART driver instance
 */
void uart_tx_initiate(uart_unit_t *uart_unit)
{
  if (uart_unit->rs485.settings.is_enable)
  {
    uart_rs485_tx_initiate(uart_unit);
  }

  if (uart_unit->state != UART_STATE__READY_TX)
  {
    // Not ready to start transmitting yet
    return;
  }

  if (uart_unit->tx.mode == UART_LINE_MODE__POLLING)
  {
    // Data will be sent from the superloop when the process gets its turn
  }
  else if (uart_unit->tx.mode == UART_LINE_MODE__IRQ)
  {
    uint32_t save;
    ENTER_CRITICAL(save);
    if (uart_core_tx_irq_first_fifo_byte(uart_unit) == ERR_OK)
    {
      uart_unit->state = UART_STATE__TX;
    }
    LEAVE_CRITICAL(save);
  }
  else if (uart_unit->tx.mode == UART_LINE_MODE__DMA)
  {
    // todo Not implemented yet
    hal_assert(0);
  }
}

void uart_struct_init(uart_init_struct_t *uart_init_struct)
{
  uart_core_struct_init(uart_init_struct);
}

/**
 * @brief Initialize the UART
 * @param uart_number - UART number (1 to UART_AMOUNT)
 * @param uart_init_struct - initialization structure
 */
void uart_x_init(uint8_t uart_number, uart_init_struct_t *uart_init_struct)
{
  check_uart_number(uart_number);

  uart_core_init((uart_unit_t *)&uart_units[uart_number], uart_init_struct);
}

/**
 * @brief Reset the UART to its initial state
 * @param uart_number - UART number (1 to UART_AMOUNT)
 */
void uart_x_deinit(uint8_t uart_number)
{
  uart_core_deinit(uart_number);
}

/**
 * @brief Check whether a UART data transmission is currently in progress.
 *        The check takes into account the driver mode (interrupt, dma, etc.)
 *        as well as the presence of unsent data in the buffer.
 * @param uart_number - UART number (1 to UART_AMOUNT)
 * @return true - data transmission is currently in progress.
 */
bool uart_x_tx_is_in_progress(uint8_t uart_number)
{
  check_uart_number(uart_number);

  bool res = false;
  uart_unit_t *uart_unit = &uart_units[uart_number];

  if (uart_unit->inited)
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    if (uart_unit->tx.fifo.idx != 0)
    {
      res = true;
    }
    else
    {
      if (uart_unit->rs485.settings.is_enable)
      {
        if (uart_unit->state != UART_STATE__WAIT)
        {
          res = true;
        }
      }
      else
      {
        if (uart_unit->state != UART_STATE__READY_TX)
        {
          res = true;
        }
      }
    }

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Send a byte over UART
 * @param uart_number - UART number (1 to UART_AMOUNT)
 * @param data - byte to send
 * @return ERR_OK - byte sent successfully, otherwise an error
 */
error_t uart_x_tx_byte(uint8_t uart_number, uint8_t data)
{
  check_uart_number(uart_number);

  error_t res = ERR_RESULT;
  if (uart_units[uart_number].inited)
  {
    res = fifo_uint8_put((fifo_uint8_t *)&uart_units[uart_number].tx.fifo, data);
    uart_tx_initiate(&uart_units[uart_number]);
  }

  return res;
}

/**
 * @brief Send an array over UART
 * @param uart_number - UART number (1 to UART_AMOUNT)
 * @param array - pointer to the array to send
 * @param size - array size
 * @return ERR_OK - array sent successfully, otherwise an error
 */
error_t uart_x_tx_array(uint8_t uart_number, uint8_t *array, uint16_t size)
{
  check_uart_number(uart_number);

  error_t res = ERR_RESULT;
  if (uart_units[uart_number].inited)
  {
    res = fifo_uint8_put_array((fifo_uint8_t *)&uart_units[uart_number].tx.fifo, array, size);
    uart_tx_initiate(&uart_units[uart_number]);
  }

  return res;
}

/**
 * @brief Send a null-terminated string over UART
 * @param uart_number - UART number (1 to UART_AMOUNT)
 * @param str - pointer to the string to send
 * @return ERR_OK - string sent successfully, otherwise an error
 */
error_t uart_x_tx_str(uint8_t uart_number, char *str)
{
  check_uart_number(uart_number);

  error_t res = ERR_RESULT;
  if (uart_units[uart_number].inited)
  {
    res = fifo_uint8_put_str((fifo_uint8_t *)&uart_units[uart_number].tx.fifo, str);
    uart_tx_initiate(&uart_units[uart_number]);
  }

  return res;
}

uint16_t uart_x_get_tx_fifo_empty_count(uint8_t uart_number)
{
  check_uart_number(uart_number);

  fifo_uint8_t *fifo = &uart_units[uart_number].tx.fifo;
  return (fifo->size - fifo->idx);
}

uint16_t uart_x_get_tx_fifo_idx(uint8_t uart_number)
{
  check_uart_number(uart_number);

  return uart_units[uart_number].tx.fifo.idx;
}
uint16_t uart_x_get_rx_fifo_idx(uint8_t uart_number)
{
  check_uart_number(uart_number);
  return uart_units[uart_number].rx.fifo.idx;
}
error_t uart_x_rx_byte(uint8_t uart_number, uint8_t *data)
{
  check_uart_number(uart_number);

  fifo_uint8_t *fifo = &uart_units[uart_number].rx.fifo;
  return fifo_uint8_get(fifo, data);
}

fifo_uint8_t *uart_x_get_rx_fifo(uint8_t uart_number)
{
  check_uart_number(uart_number);

  return (fifo_uint8_t *)&uart_units[uart_number].rx.fifo;
}

fifo_uint8_t *uart_x_get_tx_fifo(uint8_t uart_number)
{
  check_uart_number(uart_number);

  return (fifo_uint8_t *)&uart_units[uart_number].tx.fifo;
}

void uart_x_cmd(uint8_t uart_number, bool state)
{
  check_uart_number(uart_number);

#if defined(STM32F1XX) || defined(STM32F4XX)
  USART_Cmd(uart_units[uart_number].UART, state);

#elif defined(STM32L4XX)
  if (state)
  {
    LL_USART_Enable(uart_units[uart_number].UART);
  }
  else
  {
    LL_USART_Disable(uart_units[uart_number].UART);
  }

#elif defined(GD32E11X) || defined(GD32L23X)
  if (state)
  {
    usart_enable(uart_units[uart_number].UART);
  }
  else
  {
    usart_disable(uart_units[uart_number].UART);
  }

#else
  #error This MCU not supported
#endif
}

#if (FEATURE_ENABLE_UART0)
void USART0_IRQHandler(void)
{
  uart_irq_handler(&uart_units[0]);
}
#endif

#if (FEATURE_ENABLE_UART1)
void USART1_IRQHandler(void)
{
  uart_irq_handler(&uart_units[1]);
}
#endif

#if (FEATURE_ENABLE_UART2)
void USART2_IRQHandler(void)
{
  uart_irq_handler(&uart_units[2]);
}
#endif

#if (FEATURE_ENABLE_UART3)
void USART3_IRQHandler(void)
{
  uart_irq_handler(&uart_units[3]);
}
#endif

#if (FEATURE_ENABLE_UART4)
void UART4_IRQHandler(void)
{
  uart_irq_handler(&uart_units[4]);
}
#endif

#if (FEATURE_ENABLE_UART5)
void UART5_IRQHandler(void)
{
  uart_irq_handler(&uart_units[5]);
}
#endif

#if (FEATURE_ENABLE_UART6)
void USART6_IRQHandler(void)
{
  uart_irq_handler(&uart_units[6]);
}
#endif

#if (FEATURE_ENABLE_UART7)
void UART7_IRQHandler(void)
{
  uart_irq_handler(&uart_units[7]);
}
#endif

#if (FEATURE_ENABLE_UART8)
void UART8_IRQHandler(void)
{
  uart_irq_handler(&uart_units[8]);
}
#endif

#if (FEATURE_ENABLE_LPUART)
void LPUART_IRQHandler(void)
{
  uart_irq_handler(&uart_units[2]);
}
#endif
