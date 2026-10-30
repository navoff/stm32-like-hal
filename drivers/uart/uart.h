#pragma once

/**
 * @file uart.h
 * @brief UART driver public API: init, polling process and FIFO-based Tx/Rx.
 */

#include <stdbool.h>
#include <stdint.h>

#include "drivers/uart/uart_types.h"
#include "utils/errors.h"
#include "utils/fifo.h"

void uart_all_process(void);

void uart_struct_init(uart_init_struct_t *uart_init_struct);

void uart_x_init(uint8_t uart_number, uart_init_struct_t *uart_init_struct);
void uart_x_deinit(uint8_t uart_number);
void uart_x_process(uint8_t uart_number);
bool uart_x_tx_is_in_progress(uint8_t uart_number);
void uart_x_cmd(uint8_t uart_number, bool state);

error_t uart_x_tx_byte(uint8_t uart_number, uint8_t data);
error_t uart_x_tx_array(uint8_t uart_number, uint8_t *array, uint16_t size);
error_t uart_x_tx_str(uint8_t uart_number, char *str);

error_t uart_x_rx_byte(uint8_t uart_number, uint8_t *data);

fifo_uint8_t *uart_x_get_rx_fifo(uint8_t uart_number);
fifo_uint8_t *uart_x_get_tx_fifo(uint8_t uart_number);

uint16_t uart_x_get_tx_fifo_empty_count(uint8_t uart_number);
uint16_t uart_x_get_tx_fifo_idx(uint8_t uart_number);
uint16_t uart_x_get_rx_fifo_idx(uint8_t uart_number);
