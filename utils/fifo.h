#pragma once

/**
 * @file fifo.h
 * @brief Byte and generic-item FIFO ring buffers.
 */

#include <stdint.h>

#include "utils/errors.h"

#pragma pack(push, 1)

typedef struct
{
  /// Pointer to the item buffer
  uint8_t *buf;

  /// Index of the first empty slot in the buffer,
  /// i.e. the index where the next item will be put
  uint16_t head;

  /// Index of the first occupied slot in the buffer
  uint16_t tail;

  /// Number of items currently in the buffer
  uint16_t idx;

  /// Maximum number of items in the buffer (bytes)
  uint16_t size;
} fifo_uint8_t;

typedef struct
{
  /// Pointer to the item buffer
  void *buf;

  /// Index of the first empty slot in the buffer,
  /// i.e. the index where the next item will be put
  uint16_t head;

  /// Index of the first occupied slot in the buffer
  uint16_t tail;

  /// Number of items currently in the buffer
  uint16_t idx;

  /// Maximum number of items in the buffer
  uint16_t size;

  /// Size of one buffer item
  uint8_t item_size;
} fifo_t;

#pragma pack(pop)

void fifo_uint8_init(fifo_uint8_t *fifo, uint16_t size, uint8_t *buf);
void fifo_uint8_flush(fifo_uint8_t *fifo);
uint16_t fifo_uint8_get_free_size(fifo_uint8_t *fifo);
error_t fifo_uint8_put(fifo_uint8_t *fifo, uint8_t data);
error_t fifo_uint8_put_array(fifo_uint8_t *fifo, uint8_t *array, uint16_t size);
error_t fifo_uint8_put_str(fifo_uint8_t *fifo, char *str);
error_t fifo_uint8_get(fifo_uint8_t *fifo, uint8_t *data);
error_t fifo_uint8_del(fifo_uint8_t *fifo);
error_t fifo_uint8_read(fifo_uint8_t *fifo, uint8_t *data);
error_t fifo_uint8_rx_dma_circular_update(fifo_uint8_t *fifo, uint16_t dma_ndtr);

void fifo_init(fifo_t *fifo, uint16_t size, void *buf, uint8_t item_size);
void fifo_flush(fifo_t *fifo);
error_t fifo_put(fifo_t *fifo, void *item);
void fifo_put_force(fifo_t *fifo, void *item);
error_t fifo_get(fifo_t *fifo, void *item);
error_t fifo_del(fifo_t *fifo);
void *fifo_peek(fifo_t *fifo);
