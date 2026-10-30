#pragma once

/**
 * @file can.h
 * @brief CAN bus driver with automatic baud rate detection and message FIFOs.
 */

#include <stdbool.h>
#include <stdint.h>

#include "utils/errors.h"

#define CAN_AMOUNT 2

/// Maximum number of bytes in a CAN message
#define CAN_MAX_PACKET_SIZE 8

/**
 * @defgroup CAN_SPEED_PRESCALLERS
 * CAN bus clock prescaler values.
 * Used together with @ref CAN_TIMING.
 * @{
 */
#define CAN_SPEED_PRESCALLER_1US 2
#define CAN_SPEED_PRESCALLER_2US (CAN_SPEED_PRESCALLER_1US * 2)
#define CAN_SPEED_PRESCALLER_5US (CAN_SPEED_PRESCALLER_1US * 5)
#define CAN_SPEED_PRESCALLER_10US (CAN_SPEED_PRESCALLER_1US * 10)
#define CAN_SPEED_PRESCALLER_END 0
/** @} */

/// CAN message identifier types
typedef enum
{
  /// Standard identifier
  CAN_ID_TYPE__STD = 0x00,

  /// Extended identifier
  CAN_ID_TYPE__EXT = 0x01,

  CAN_ID_TYPE__COUNT,
} can_id_type_t;

/// CAN operating modes
typedef enum
{
  /// Receive, transmit and acknowledge are all enabled
  CAN_MODE__RX_TX_ACK = 0,

  /// Receive only. Transmit is disabled. No acknowledgements are sent
  CAN_MODE__RX,

  CAN_MODE__COUNT,
} can_mode_t;

#pragma pack(push, 1)

/// Application-level CAN message structure
/// Not to be confused with the library structures CanRxMsg and CanTxMsg
typedef struct
{
  /// Packet timestamp
  /// For received packets it is set with START_TIMER, [ms]
  uint32_t timestamp_ms;

  /// CAN module number
  uint8_t can_number;

  /// Message identifier
  uint32_t id;

  /// Message identifier type
  can_id_type_t id_type;

  /// Number of data bytes in the message
  uint8_t dlc;

  /// Message data
  uint8_t data[CAN_MAX_PACKET_SIZE];
} can_msg_t;

/// Callback type for the received message handler
typedef void (*can_rx_callback_t)(can_msg_t *msg);

/// Callback type invoked when CAN baud rate autodetection finishes
typedef void (*can_speed_autodetect_fin_callback_t)(void);

/// CAN driver initialization parameters
typedef struct
{
  /// CAN module number
  uint8_t can_number;

  /// Whether CAN baud rate autodetection is performed
  bool need_speed_autodetect;

  /// CAN operating mode
  can_mode_t mode;

  /// CAN bus clock prescaler,
  /// which defines the CAN baud rate @ref CAN_SPEED_PRESCALLERS
  uint16_t prescaler;

  /// RX port index
  uint8_t rx_gpio_idx;

  /// RX pin index
  uint8_t rx_pin_idx;

  /// TX port index
  uint8_t tx_gpio_idx;

  /// TX pin index
  uint8_t tx_pin_idx;

  /// Pointer to the transmit buffer
  can_msg_t *tx_buf;

  /// Transmit buffer size
  uint16_t tx_buf_size;

  /// Pointer to the receive buffer
  can_msg_t *rx_buf;

  /// Receive buffer size
  uint16_t rx_buf_size;

  /// Callback for handling a received CAN message
  can_rx_callback_t rx_callback;

  /// Callback invoked when CAN baud rate autodetection finishes
  can_speed_autodetect_fin_callback_t speed_autodetect_fin_callback;
} can_init_structure_t;

#pragma pack(pop)

/// FIFO buffer for storing CAN messages
typedef struct
{
  /// Number of messages that did not fit into the buffer (buffer overflow)
  uint32_t bof_cnt;

  /// Buffer index where the next element will be written.
  uint16_t head;

  /// Buffer index from which the next element will be read.
  uint16_t tail;

  /// Current number of elements in the buffer.
  uint16_t idx;

  /// Buffer size.
  uint16_t size;

  /// Packet storage buffer
  can_msg_t *buf;
} can_fifo_t;

/// CAN driver instance
typedef struct
{
  /// Whether CAN baud rate autodetection is performed
  bool need_speed_autodetect;

  /// CAN operating mode
  can_mode_t mode;

  /// Message transmission is in progress
  bool tx_in_progress;

  /// CAN module number
  uint8_t can_number;

  /// Receive FIFO of CAN messages
  can_fifo_t rx_fifo;

  /// Transmit FIFO of CAN messages
  can_fifo_t tx_fifo;

  struct
  {
    /// Timestamp of the last received CAN message
    uint32_t last_rx_msg_timestamp;

    /// Timestamp of the last transmitted CAN message
    uint32_t last_tx_msg_timestamp;

    /// Number of received CAN messages
    uint32_t rx_msg_cnt;

    /// Number of transmitted CAN messages
    uint32_t tx_msg_cnt;
  } stats;

  /// Index of the current CAN bus clock prescaler in the @ref can_speed_prescaler array
  uint8_t prescaler_idx;

  /// CAN driver state
  enum
  {
    /// Baud rate autodetection in progress
    CAN_ST__SPEED_AUTODETECT,

    /// Normal operation (receive/transmit)
    CAN_ST__NORMAL,
  } state;

  /// Pointer to the CAN controller register structure
  CAN_TypeDef *can;

  /// Callback for handling a received CAN message
  can_rx_callback_t rx_callback;

  /// Callback invoked when CAN baud rate autodetection finishes
  can_speed_autodetect_fin_callback_t speed_autodetect_fin_callback;
} can_t;

void can_structure_init(can_init_structure_t *init_struct);
void can_init(can_t *can, can_init_structure_t *init_struct);
void can_process(can_t *can);
error_t can_tx_msg(can_t *can, can_msg_t *msg);
error_t can_rx_msg(can_t *can, can_msg_t *msg);
void can_rx_irq_handler(can_t *can);
void can_tx_irq_handler(can_t *can);

void can_fifo_clear(can_fifo_t *fifo);
error_t can_fifo_read(can_fifo_t *fifo, can_msg_t *msg);
error_t can_fifo_del(can_fifo_t *fifo);
error_t can_fifo_get(can_fifo_t *fifo, can_msg_t *msg);
error_t can_fifo_put(can_fifo_t *fifo, can_msg_t *msg);

void can_reset_filters(uint8_t fifo_idx);
void can_set_filter_id(uint8_t fifo_idx, uint8_t filter_idx, can_id_type_t msg_id_type,
                       uint32_t msg_id);
void can_set_broadcast_filter(uint8_t fifo_idx, uint8_t filter_idx, can_id_type_t msg_id_type);
