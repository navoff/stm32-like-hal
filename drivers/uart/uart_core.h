#pragma once

/**
 * @file uart_core.h
 * @brief Internal UART driver structures and MCU-specific register access macros.
 */

#include <stdint.h>

#include "drivers/uart/uart_types.h"
#include "utils/fifo.h"

#define UART_AMOUNT 8

#if defined(GD32E11X) || defined(GD32L23X)
  /// Number of driver instances: GD32 numbers UARTs from 0
  #define UART_UNITS_AMOUNT UART_AMOUNT
#else
  /// Number of driver instances: STM32 numbers UARTs from 1, element 0 is unused
  #define UART_UNITS_AMOUNT (UART_AMOUNT + 1)
#endif

#define UART_NUMBER_UNDEF 0xFF

typedef struct
{
  uart_line_mode_t mode;

  fifo_uint8_t fifo;

#if defined(STM32F1XX)
  /// DMA channel
  DMA_Channel_TypeDef *dma_channel;

#elif defined(STM32F4XX)
  /// DMA stream
  DMA_Stream_TypeDef *dma_stream;

#elif defined(STM32L4XX)
  /// DMA channel as LL_DMA_CHANNEL_x
  uint32_t ll_dma_channel;

  /// DMA instance
  DMA_TypeDef *dma;

#elif defined(GD32E11X) || defined(GD32L23X)

  /// DMA instance DMAx (x=0,1)
  uint32_t dma;

  /// DMA channel: DMA_CHx (x=0..6), DMA1: DMA_CHx (x=0..4)
  dma_channel_enum dma_channel;
#else
  #error This MCU not supported
#endif
  uint8_t was_overflow;
} uart_line_t;

/// UART state
typedef enum
{
  /// Waiting for transmission to be started
  UART_STATE__WAIT,
  /// Pause before enabling DIR
  UART_STATE__PAUSE_BEFORE_DIR,
  /// Pause after enabling DIR and before starting transmission
  UART_STATE__PAUSE_BEFORE_TX,
  /// Ready to transmit data
  UART_STATE__READY_TX,
  /// Data transmission in progress
  UART_STATE__TX,
  /// Pause after the last byte is sent and before disabling DIR
  UART_STATE__PAUSE_AFTER_TX,
} uart_state_t;

/// RS-485 events to be processed
typedef enum
{
  /// Waiting for an event to process
  UART_RS485_EVENT__NONE,
  /// Event received to initiate an RS-485 transmission
  UART_RS485_EVENT__TX_INITIATE,
} uart_rs485_event_t;

/// RS-485 runtime state structure
typedef struct
{
  uint32_t timestamp_ms;

  uart_rs485_event_t event;

  uart_init_rs485_struct_t settings;
} uart_rs485_t;

/// UART driver runtime structure
typedef struct
{
  uart_line_t tx;
  uart_line_t rx;

  uart_state_t state;

  uart_rs485_t rs485;

  uint8_t tx_started;
  uint8_t inited;
  uint16_t error_status;

#if defined(GD32E11X) || defined(GD32L23X)
  uint32_t UART;
#else
  USART_TypeDef *UART;
#endif
} uart_unit_t;

#if defined(STM32F4XX) || defined(STM32F1XX)
  #define check_uart_number(number) hal_assert((number > 0) && (number <= UART_AMOUNT))

  #define UART_GET_SR(UART) (UART->SR)
  #define UART_GET_TDR(UART) (UART->DR)
  #define UART_GET_RDR(UART) (UART->DR)

  #define UART_IT_RXNE_ENABLE(UART) USART_ITConfig(UART, USART_IT_RXNE, ENABLE)

  #define UART_IT_TXE_ENABLE(UART) USART_ITConfig(UART, USART_IT_TXE, ENABLE)
  #define UART_IT_TXE_DISABLE(UART) USART_ITConfig(UART, USART_IT_TXE, DISABLE)

  #define UART_IT_TC_ENABLE(UART) USART_ITConfig(UART, USART_IT_TC, ENABLE)
  #define UART_IT_TC_DISABLE(UART) USART_ITConfig(UART, USART_IT_TC, DISABLE)

  #define UART_CLEAR_TC_FLAG(UART) USART_ClearITPendingBit(UART, USART_IT_TC)

  #define UART_IS_RX_NOT_EMPTY(status) ((status) & USART_SR_RXNE)
  #define UART_IS_TX_EMPTY(status) ((status) & USART_SR_TXE)
  #define UART_IS_TX_COMPLETE(status) ((status) & USART_SR_TC)

  #define UART_IS_TX_COMPLETE_INT_EN(UART) (UART->CR1 & USART_CR1_TCIE)
  #define UART_IS_TX_BUF_EMPTY_INT_EN(UART) (UART->CR1 & USART_CR1_TXEIE)

#elif defined(STM32L4XX)
  #define check_uart_number(number) hal_assert((number > 0) && (number <= UART_AMOUNT))

  #define UART_GET_SR(UART) (UART->ISR)
  #define UART_GET_TDR(UART) (UART->TDR)
  #define UART_GET_RDR(UART) (UART->RDR)

  #define UART_IT_RXNE_ENABLE(UART) LL_USART_EnableIT_RXNE(UART)

  #define UART_IT_TXE_ENABLE(UART) LL_USART_EnableIT_TXE(UART)
  #define UART_IT_TXE_DISABLE(UART) LL_USART_DisableIT_TXE(UART)

  #define UART_IT_TC_ENABLE(UART) LL_USART_EnableIT_TC(UART)
  #define UART_IT_TC_DISABLE(UART) LL_USART_DisableIT_TC(UART)

  #define UART_CLEAR_TC_FLAG(UART) LL_USART_ClearFlag_TC(UART)

  #define USART_SR_RXNE USART_ISR_RXNE
  #define USART_SR_TXE USART_ISR_TXE
  #define USART_SR_TC USART_ISR_TC

  #define UART_IS_RX_NOT_EMPTY(status) ((status) & USART_SR_RXNE)
  #define UART_IS_TX_EMPTY(status) ((status) & USART_SR_TXE)
  #define UART_IS_TX_COMPLETE(status) ((status) & USART_SR_TC)

  #define UART_IS_TX_COMPLETE_INT_EN(UART) (UART->CR1 & USART_CR1_TCIE)
  #define UART_IS_TX_BUF_EMPTY_INT_EN(UART) (UART->CR1 & USART_CR1_TXEIE)

#elif defined(GD32E11X) || defined(GD32L23X)
  /* TODO: add defines for GD32L23X lpuart */
  #define check_uart_number(number) hal_assert(number < UART_AMOUNT)

  #if defined(GD32E11X)
    #define UART_GET_SR(UART) USART_STAT0(UART)
  #elif defined(GD32L23X)
    #define UART_GET_SR(UART) USART_STAT(UART)
  #endif
  #define UART_GET_CR(UART) USART_CTL0(UART)

  #if defined(GD32E11X)
    #define UART_GET_TDR(UART) USART_DATA(UART)
    #define UART_GET_RDR(UART) USART_DATA(UART)
  #elif defined(GD32L23X)
    #define UART_GET_TDR(UART) USART_TDATA(UART)
    #define UART_GET_RDR(UART) USART_RDATA(UART)
  #endif

  #define UART_IT_RXNE_ENABLE(UART) usart_interrupt_enable(UART, USART_INT_RBNE)

  #define UART_IT_TXE_ENABLE(UART) usart_interrupt_enable(UART, USART_INT_TBE)
  #define UART_IT_TXE_DISABLE(UART) usart_interrupt_disable(UART, USART_INT_TBE)

  #define UART_IT_TC_ENABLE(UART) usart_interrupt_enable(UART, USART_INT_TC)
  #define UART_IT_TC_DISABLE(UART) usart_interrupt_disable(UART, USART_INT_TC)

  #define UART_CLEAR_TC_FLAG(UART) usart_interrupt_flag_clear(UART, USART_INT_FLAG_TC)

  #if defined(GD32E11X)
    #define UART_IS_RX_NOT_EMPTY(status) ((status) & USART_STAT0_RBNE)
    #define UART_IS_TX_EMPTY(status) ((status) & USART_STAT0_TBE)
    #define UART_IS_TX_COMPLETE(status) ((status) & USART_STAT0_TC)
  #elif defined(GD32L23X)
    #define UART_IS_RX_NOT_EMPTY(status) ((status) & USART_STAT_RBNE)
    #define UART_IS_TX_EMPTY(status) ((status) & USART_STAT_TBE)
    #define UART_IS_TX_COMPLETE(status) ((status) & USART_STAT_TC)
  #endif

  #define UART_IS_TX_COMPLETE_INT_EN(UART) (USART_CTL0(UART) & USART_CTL0_TCIE)
  #define UART_IS_TX_BUF_EMPTY_INT_EN(UART) (USART_CTL0(UART) & USART_CTL0_TBEIE)
#else
  #error This MCU not supported
#endif

void uart_core_struct_init(uart_init_struct_t *uart_init_struct);
void uart_core_init(uart_unit_t *uart_unit, uart_init_struct_t *uart_init_struct);
void uart_core_deinit(uint8_t uart_number);

void uart_core_error_handler(uart_unit_t *uart_unit, uint16_t sr);

error_t uart_core_tx_polling_fifo_byte(uart_unit_t *uart_unit);
error_t uart_core_tx_irq_first_fifo_byte(uart_unit_t *uart_unit);

extern uart_unit_t uart_units[];
