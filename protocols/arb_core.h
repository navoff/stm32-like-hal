#pragma once

/**
 * @file arb_core.h
 * @brief Core of the ARB binary protocol.
 * The function set does not depend on the modules present in a device and
 * is common to all devices.
 */

#include <stdbool.h>
#include <stdint.h>

#include "utils/errors.h"

/// Identifier of ARB protocol version 1
#define ARB_PROTOCOL_ID_V1 0xFD

#pragma pack(push, 1)

/// Packet header structure
typedef struct
{
  /// Protocol identifier
  uint8_t protocol_id;

  /// Bus address of the device the packet is addressed to
  uint8_t address;

  /// Identifier of the component within the device
  uint8_t component_id;

  /// Packet identifier
  uint8_t packet_id;
} arb_packet_header_t;
#pragma pack(pop)

typedef error_t (*arb_rx_byte_func_t)(uint8_t *data);
typedef error_t (*arb_tx_byte_func_t)(uint8_t data);
typedef bool (*arb_is_ready_tx_func_t)(void);
struct arb_s;
typedef void (*arb_rx_callback_t)(struct arb_s *arb, arb_packet_header_t *header, uint8_t *buf,
                                  uint16_t size);

/// Initialization structure
typedef struct
{
  /// Pointer to the receive buffer
  uint8_t *rx_buf;

  /// Receive buffer size
  uint16_t rx_buf_size;

  /// Pointer to the function that fetches a byte from the receive buffer
  arb_rx_byte_func_t rx_byte;

  /// User application callback
  /// Called when a valid packet has been received
  arb_rx_callback_t rx_callback;

  /// Pointer to the function that puts a byte into the transmit buffer
  arb_tx_byte_func_t tx_byte;

  /// Function that checks whether the transmit buffer can accept more data
  arb_is_ready_tx_func_t is_ready_tx;
} arb_init_struct_t;

/// ARB protocol instance structure
typedef struct arb_s
{
  /// Structure handling the receive stream
  struct
  {
    /// Number of bytes in the receive buffer
    uint16_t idx;

    /// Pointer to the receive buffer
    uint8_t *buf;

    /// Receive buffer size
    uint16_t buf_size;

    /// Escape sequence detected flag
    uint8_t flag_escape;

    /// Pointer to the function that fetches a byte from the receive buffer
    arb_rx_byte_func_t rx_byte;

    /// User application callback
    /// Called when a valid packet has been received
    arb_rx_callback_t rx_callback;
  } rx;

  /// Pointer to the function that puts a byte into the transmit buffer
  arb_tx_byte_func_t tx_byte;

  /// Function that checks whether the transmit buffer can accept more data
  arb_is_ready_tx_func_t is_ready_tx;
} arb_t;

void arb_struct_init(arb_init_struct_t *init_struct);
void arb_init(arb_t *arb, arb_init_struct_t *init_struct);
void arb_rx_process(arb_t *arb);
void arb_tx_packet(arb_t *arb, arb_packet_header_t *header, uint8_t *buf, uint16_t size);
