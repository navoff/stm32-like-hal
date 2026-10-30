#pragma once

/**
 * @file app_usb.h
 * @brief USB CDC (VCP) application layer: TX/RX FIFOs on top of the ST USB device stacks.
 */

#include <stdint.h>

#include "config.h"
#include "utils/errors.h"

#if defined(STM32F2XX) || defined(STM32F4XX)
  #include "usb_core.h"
  #include "usb_dcd.h"
  #include "usbd_conf.h"

#elif defined(STM32F1XX) || defined(STM32F30X)

  #include "usb_desc_device.h"
  #include "usb_endp.h"
  #include "usb_istr.h"
  #include "usb_lib.h"
  #include "usb_mem.h"

#elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)

  #include "usb_config.h"
  #include "usbd_def.h"
  #include "usbd_ioreq.h"

#else
  #error "Controller type is not defined"
#endif

typedef struct
{
  uint8_t *buf;
  uint16_t idx;
  uint16_t tail;
  uint16_t head;
  uint16_t size;
  uint8_t overflow;
} app_usb_fifo_t;

typedef struct
{
  /// Transmit FIFO buffer.
  app_usb_fifo_t tx_fifo;
  /// Receive FIFO buffer.
  app_usb_fifo_t rx_fifo;
  /// Buffer transmission is in progress.
  uint8_t tx_in_prog;
  /// Reception is paused while the buffer is being drained.
  uint8_t rx_locked;
  uint8_t last_tx_size;
  /// The endpoint has been armed to receive a buffer.
  uint8_t wait_rx;
} app_usb_t;

error_t usb_tx_fifo_put_one(uint8_t data);
uint16_t usb_tx_fifo_get_empty(void);
void usb_tx_fifo_flush(void);

error_t usb_rx_fifo_get_one(uint8_t *data);
uint16_t usb_rx_fifo_get_idx(void);
void usb_rx_fifo_flush(void);
error_t usb_is_plugged(void);

void app_usb_init(void);
void app_usb_disable_irq(void);
void app_usb_SOF(void *param);
void app_usb_IN(void *param);
void app_usb_OUT(void *param);

#if defined(STM32F2XX) || defined(STM32F4XX)

extern USB_OTG_CORE_HANDLE USB_OTG_dev;

#elif defined(STM32F1XX) || defined(STM32F30X)

#elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)

extern USBD_HandleTypeDef USBD_Device;

#else
  #error "Controller type is not defined"
#endif
