#pragma once

/**
 * @file uart_types.h
 * @brief UART driver public types: line modes, RS-485 settings and the init structure.
 */

#include <stdbool.h>
#include <stdint.h>

/// Data transmit/receive modes
typedef enum
{
  UART_LINE_MODE__POLLING = 0,
  UART_LINE_MODE__DMA,
  UART_LINE_MODE__IRQ,
  UART_LINE_MODE__COUNT,
} uart_line_mode_t;

/// Rx/Tx line configuration structure
typedef struct
{
  uart_line_mode_t mode;
  uint8_t gpio_idx;
  uint16_t pin_idx;
  uint8_t dma_number;
  uint8_t dma_channel_idx;
#if defined(STM32F4XX)
  uint8_t dma_stream_idx;
#endif
  uint8_t *buf;
  uint16_t buf_size;

} uart_init_line_struct_t;

/// RS485 configuration structure
typedef struct
{
  /// Whether RS-485 is enabled
  bool is_enable;

  /// GPIO port index of the DIR pin
  uint8_t dir_gpio_idx;
  /// Pin index of the DIR pin
  uint8_t dir_pin_idx;

  /// Pause before switching the DIR pin to On, [ms]
  uint32_t pause_before_dir_on;
  /// Pause after switching the DIR pin to On and before starting transmission, [ms]
  uint32_t pause_before_tx;
  /// Pause after transmission and before switching the DIR pin to Off, [ms]
  uint32_t pause_after_tx;

} uart_init_rs485_struct_t;

typedef struct
{
  uint32_t baudrate;
  /// UART number. Range: 1 to UART_AMOUNT
  /// @note For GD32 the range is 0 to (UART_AMOUNT - 1)
  uint8_t uart_number;
  uart_init_line_struct_t tx;
  uart_init_line_struct_t rx;
  uart_init_rs485_struct_t rs485;
} uart_init_struct_t;
