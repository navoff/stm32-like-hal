/**
 * @file fifo.c
 * @brief Byte and generic-item FIFO ring buffer implementation.
 */

#include "config.h"

#include <string.h>

#include "utils/assert.h"

#include "utils/fifo.h"

/**
 * @brief Initialize a uint8_t FIFO buffer
 * @param fifo - FIFO buffer instance
 * @param size - maximum number of items in the buffer (bytes)
 * @param buf - pointer to the buffer
 */
void fifo_uint8_init(fifo_uint8_t *fifo, uint16_t size, uint8_t *buf)
{
  hal_assert(buf != NULL);

  fifo->buf = buf;
  fifo->size = size;
  fifo_uint8_flush(fifo);
}

/**
 * @brief Flush the FIFO buffer (reset counters and contents)
 * @param fifo - FIFO buffer instance
 */
void fifo_uint8_flush(fifo_uint8_t *fifo)
{
  fifo->head = fifo->tail = fifo->idx = 0;
  memset(fifo->buf, 0, fifo->size);
}

uint16_t fifo_uint8_get_free_size(fifo_uint8_t *fifo)
{
  return (fifo->size - fifo->idx);
}

/**
 * @brief Put a data byte into the FIFO buffer
 * @param fifo - FIFO buffer instance
 * @param data - data byte
 * @return ERR_OK - data byte put successfully
 *         ERR_BOF - data byte could not be put, buffer is full
 */
error_t fifo_uint8_put(fifo_uint8_t *fifo, uint8_t data)
{
  error_t res = ERR_OK;

  if (fifo->idx >= fifo->size)
  {
    res = ERR_BOF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    fifo->buf[fifo->head++] = data;

    if (fifo->head >= fifo->size)
    {
      fifo->head = 0;
    }

    fifo->idx++;

    LEAVE_CRITICAL(save);
  }

  return res;
}

error_t fifo_uint8_put_array(fifo_uint8_t *fifo, uint8_t *array, uint16_t size)
{
  error_t res = ERR_OK;

  if ((fifo->idx + size) >= fifo->size)
  {
    res = ERR_BOF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    for (uint16_t i = 0; i < size; i++)
    {
      fifo->buf[fifo->head++] = array[i];

      if (fifo->head >= fifo->size)
      {
        fifo->head = 0;
      }
    }

    fifo->idx += size;

    LEAVE_CRITICAL(save);
  }

  return res;
}

error_t fifo_uint8_put_str(fifo_uint8_t *fifo, char *str)
{
  error_t res = ERR_OK;

  uint32_t save;
  ENTER_CRITICAL(save);

  while (1)
  {
    if (*str == '\0')
    {
      break;
    }

    if (fifo->idx >= fifo->size)
    {
      res = ERR_BOF;
      break;
    }

    fifo->buf[fifo->head] = *str++;
    fifo->head++;

    if (fifo->head >= fifo->size)
    {
      fifo->head = 0;
    }

    fifo->idx++;
  }

  LEAVE_CRITICAL(save);

  return res;
}

/**
 * @brief Get a data byte from the FIFO buffer
 * @param fifo - FIFO buffer instance
 * @param data - pointer where the extracted data byte will be stored
 * @return ERR_OK - data byte extracted successfully
 *         ERR_BUF - data byte could not be extracted, buffer is empty
 */
error_t fifo_uint8_get(fifo_uint8_t *fifo, uint8_t *data)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    *data = fifo->buf[fifo->tail++];

    if (fifo->tail >= fifo->size)
    {
      fifo->tail = 0;
    }

    fifo->idx--;

    LEAVE_CRITICAL(save);
  }

  return res;
}

error_t fifo_uint8_del(fifo_uint8_t *fifo)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    fifo->tail++;

    if (fifo->tail >= fifo->size)
    {
      fifo->tail = 0;
    }

    fifo->idx--;

    LEAVE_CRITICAL(save);
  }

  return res;
}

error_t fifo_uint8_read(fifo_uint8_t *fifo, uint8_t *data)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    *data = fifo->buf[fifo->tail];

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Initialize a generic-type FIFO buffer
 * @param fifo - FIFO buffer instance
 * @param size - maximum number of items in the buffer
 * @param buf - pointer to the buffer
 * @param item_size - size of one item in the buffer
 */
void fifo_init(fifo_t *fifo, uint16_t size, void *buf, uint8_t item_size)
{
  fifo->buf = buf;
  fifo->size = size;
  fifo->item_size = item_size;
  fifo_flush(fifo);
}

/**
 * @brief Flush the FIFO buffer (reset counters and contents)
 * @param fifo - FIFO buffer instance
 */
void fifo_flush(fifo_t *fifo)
{
  fifo->head = fifo->tail = fifo->idx = 0;
  memset(fifo->buf, 0, fifo->size * fifo->item_size);
}

/**
 * @brief Put a data item into the FIFO buffer
 * @param fifo - FIFO buffer instance
 * @param item - pointer to the data item
 * @return ERR_OK - data item put successfully
 *         ERR_BOF - data item could not be put, buffer is full
 */
error_t fifo_put(fifo_t *fifo, void *item)
{
  error_t res = ERR_OK;

  if (fifo->idx >= fifo->size)
  {
    res = ERR_BOF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    memcpy((void *)(((uint32_t)fifo->buf) + fifo->head * fifo->item_size), item, fifo->item_size);
    fifo->head++;

    if (fifo->head >= fifo->size)
    {
      fifo->head = 0;
    }

    fifo->idx++;

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Put a data item into the FIFO buffer.
 *        If there is no room in the buffer, the oldest item is overwritten.
 * @param fifo - FIFO buffer instance
 * @param item - pointer to the data item
 */
void fifo_put_force(fifo_t *fifo, void *item)
{
  uint32_t save;
  ENTER_CRITICAL(save);

  memcpy((void *)(((uint32_t)fifo->buf) + fifo->head * fifo->item_size), item, fifo->item_size);

  fifo->head++;
  if (fifo->head >= fifo->size)
  {
    fifo->head = 0;
  }

  if (fifo->idx < fifo->size)
  {
    fifo->idx++;
  }
  else
  {
    fifo->tail = fifo->head;
  }

  LEAVE_CRITICAL(save);
}

/**
 * @brief Get a data item from the FIFO buffer
 * @param fifo - FIFO buffer instance
 * @param data - pointer where the extracted data item will be stored
 * @return ERR_OK - data item extracted successfully
 *         ERR_BUF - data item could not be extracted, buffer is empty
 */
error_t fifo_get(fifo_t *fifo, void *item)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    memcpy(item, (void *)(((uint32_t)fifo->buf) + fifo->tail * fifo->item_size), fifo->item_size);
    fifo->tail++;

    if (fifo->tail >= fifo->size)
    {
      fifo->tail = 0;
    }

    fifo->idx--;

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Drop a data item from the FIFO buffer
 * @param fifo - FIFO buffer instance
 * @return ERR_OK - data item removed successfully
 *         ERR_BUF - data item could not be removed, buffer is empty
 */
error_t fifo_del(fifo_t *fifo)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    fifo->tail++;

    if (fifo->tail >= fifo->size)
    {
      fifo->tail = 0;
    }

    fifo->idx--;

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Get a pointer to the first data item in the FIFO buffer
 * @param fifo - FIFO buffer instance
 * @return non-NULL - pointer to the data item obtained successfully
 *         NULL - data item could not be peeked, buffer is empty
 */
void *fifo_peek(fifo_t *fifo)
{
  if (fifo->idx == 0)
  {
    return NULL;
  }

  return (void *)(((uint32_t)fifo->buf) + fifo->tail * fifo->item_size);
}

/**
 * @brief Update FIFO indexes when receiving via DMA in circular mode
 * @param fifo - pointer to the FIFO buffer
 * @param dma_ndtr - current value of the DMAx->NDTR register
 * @return ERR_OK - indexes updated successfully,
 *         ERR_BOF - buffer overflow occurred
 */
error_t fifo_uint8_rx_dma_circular_update(fifo_uint8_t *fifo, uint16_t dma_ndtr)
{
  error_t res = ERR_OK;

  uint16_t current_head = fifo->size - dma_ndtr;

  if (current_head != fifo->head)
  {
    uint16_t rx_amount;
    if (current_head >= fifo->head)
    {
      rx_amount = current_head - fifo->head;
    }
    else
    {
      rx_amount = current_head + fifo->size - fifo->head;
    }

    // if the DMA buffer has overflowed
    if ((rx_amount + fifo->idx) > fifo->size)
    {
      fifo->head = current_head;
      fifo->tail = current_head;
      fifo->idx = 0;

      res = ERR_BOF;
    }
    else
    {
      fifo->head = current_head;
      fifo->idx += rx_amount;
    }
  }

  return res;
}
