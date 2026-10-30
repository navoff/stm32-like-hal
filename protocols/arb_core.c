/**
 * @file arb_core.c
 * @brief Core of the ARB binary protocol.
 * The function set does not depend on the modules present in a device and
 * is common to all devices.
 */

#include "config.h"

#include <string.h>

#include "utils/assert.h"
#include "utils/checksum.h"

#include "protocols/arb_core.h"

/// Packet header size, including the protocol identifier
#define ARB_HEADER_SIZE sizeof(arb_packet_header_t)

/// Checksum size
#define ARB_CHECK_SUM_SIZE 2

/// Minimum packet size, including the protocol identifier
/// This is the size of a packet with an "empty" data field
#define ARB_PACKET_MIN_SIZE (ARB_HEADER_SIZE + ARB_CHECK_SUM_SIZE)

/// Packet delimiter
#define ARB_DELIMITER_BYTE 0x7E

/// Escape byte used to remove the packet delimiter from the data stream
#define ARB_ESCAPE_BYTE 0x7D

/// Mask used to build an escape sequence
#define ARB_ESCAPE_MSK 0x20

/// Initial value for checksum calculation
#define ARB_CHECKSUM_INIT_VALUE CHECKSUM_CRC16_IBM_INIT_VALUE

/// Checksum calculation function for the "current" algorithm
#define arb_checksum_calculate checksum_crc16_ibm_calculate

/// Checksum update function for the "current" algorithm
#define arb_checksum_accumulate checksum_crc16_ibm_accumulate

void arb_rx_buf_process(arb_t *arb);

/**
 * @brief Fill the initialization structure with default values
 * @param init_struct - pointer to the initialization structure
 */
void arb_struct_init(arb_init_struct_t *init_struct)
{
  memset(init_struct, 0, sizeof(arb_init_struct_t));
}

/**
 * @brief Initialize the ARB protocol according to the initialization structure
 * @param arb - ARB protocol instance
 * @param init_struct - initialization structure
 */
void arb_init(arb_t *arb, arb_init_struct_t *init_struct)
{
  hal_assert((arb != NULL) && (init_struct != NULL));
  hal_assert((init_struct->rx_buf != NULL) && (init_struct->rx_buf_size != 0));
  hal_assert(init_struct->rx_byte != NULL);
  hal_assert(init_struct->tx_byte != NULL);
  hal_assert(init_struct->is_ready_tx != NULL);

  arb->rx.flag_escape = 0;
  arb->rx.idx = 0;
  arb->rx.buf = init_struct->rx_buf;
  arb->rx.buf_size = init_struct->rx_buf_size;
  arb->rx.rx_byte = init_struct->rx_byte;
  arb->rx.rx_callback = init_struct->rx_callback;
  arb->tx_byte = init_struct->tx_byte;
  arb->is_ready_tx = init_struct->is_ready_tx;
}

/**
 * Input stream processing function.
 * Searches the input stream for packet delimiters and strips escape sequences.
 *
 * @param arb - ARB protocol instance
 */
void arb_rx_process(arb_t *arb)
{
  uint8_t data;

  while (1)
  {
    if (arb->rx.rx_byte(&data) != ERR_OK)
    {
      break;
    }

    // packet delimiter received
    if (data == ARB_DELIMITER_BYTE)
    {
      // an escape byte was received just before
      if (arb->rx.flag_escape)
      {
        arb->rx.flag_escape = 0;
        arb->rx.idx = 0;
      }
      else
      {
        if (arb->rx.idx > 0)
        {
          // possibly a complete packet
          arb_rx_buf_process(arb);
          arb->rx.idx = 0;
        }
      }
    }
    // escape byte received
    else if (data == ARB_ESCAPE_BYTE)
    {
      if (arb->rx.flag_escape)
      {
        arb->rx.flag_escape = 0;
        arb->rx.idx = 0;
      }
      else
      {
        arb->rx.flag_escape = 1;
      }
    }
    else
    {
      if (arb->rx.flag_escape)
      {
        arb->rx.flag_escape = 0;
        data ^= ARB_ESCAPE_MSK;
      }

      if (arb->rx.idx < arb->rx.buf_size)
      {
        // store the data
        arb->rx.buf[arb->rx.idx++] = data;
      }
    }
  }
}

/**
 * "Primary" processing of a received packet.
 * Checks that the length and the checksum are valid.
 *
 * @param arb - ARB protocol instance
 */
void arb_rx_buf_process(arb_t *arb)
{
  do
  {
    if (arb->rx.idx < ARB_PACKET_MIN_SIZE)
    {
      // received data is shorter than the minimum packet length
      break;
    }

    arb_packet_header_t header;
    memcpy(&header, arb->rx.buf, ARB_HEADER_SIZE);

    if (header.protocol_id != ARB_PROTOCOL_ID_V1)
    {
      // unknown protocol identifier
      break;
    }

    uint16_t sum = arb_checksum_calculate(arb->rx.buf, arb->rx.idx);
    if (sum != 0)
    {
      // invalid checksum
      break;
    }

    uint8_t *data = &arb->rx.buf[ARB_HEADER_SIZE];

    arb->rx.rx_callback(arb, &header, data, arb->rx.idx - ARB_PACKET_MIN_SIZE);

  } while (0);
}

/**
 * @brief Build a packet from the given buffer and queue it for transmission
 * @param arb - ARB protocol instance
 * @param header - packet header
 * @param buf - buffer to transmit
 * @param size - buffer size, bytes
 * @note Escape sequences are inserted while the packet is built.
 *       The payload size is not transmitted, it follows from the frame length
 */
void arb_tx_packet(arb_t *arb, arb_packet_header_t *header, uint8_t *buf, uint16_t size)
{
  if (arb == NULL)
  {
    return;
  }
  if (arb->tx_byte == NULL)
  {
    return;
  }

  arb->tx_byte(ARB_DELIMITER_BYTE);

  if (header->protocol_id != ARB_PROTOCOL_ID_V1)
  {
    return;
  }

  uint16_t check_sum = ARB_CHECKSUM_INIT_VALUE;

  for (uint8_t i = 0; i < ARB_HEADER_SIZE; i++)
  {
    uint8_t data = ((uint8_t *)header)[i];
    check_sum = arb_checksum_accumulate(check_sum, data);

    if ((data == ARB_DELIMITER_BYTE) || (data == ARB_ESCAPE_BYTE))
    {
      arb->tx_byte(ARB_ESCAPE_BYTE);
      data ^= ARB_ESCAPE_MSK;
    }

    arb->tx_byte(data);
  }

  for (uint16_t i = 0; i < size; i++)
  {
    uint8_t data = buf[i];

    check_sum = arb_checksum_accumulate(check_sum, data);

    if ((data == ARB_DELIMITER_BYTE) || (data == ARB_ESCAPE_BYTE))
    {
      arb->tx_byte(ARB_ESCAPE_BYTE);
      data ^= ARB_ESCAPE_MSK;
    }

    arb->tx_byte(data);
  }

  for (uint8_t i = 0; i < ARB_CHECK_SUM_SIZE; i++)
  {
    uint8_t data = check_sum & 0xFF;
    if ((data == ARB_DELIMITER_BYTE) || (data == ARB_ESCAPE_BYTE))
    {
      arb->tx_byte(ARB_ESCAPE_BYTE);
      data ^= ARB_ESCAPE_MSK;
    }
    arb->tx_byte(data);

    check_sum >>= 8;
  }

  arb->tx_byte(ARB_DELIMITER_BYTE);
}
