/**
 * @file app_usb.c
 * @brief USB CDC (VCP) application layer: TX/RX FIFOs on top of the ST USB device stacks.
 */

#include "config.h"

#if (FEATURE_ENABLE_USB)

  #include <stdbool.h>
  #include <string.h>

  #include "utils/assert.h"
  #include "utils/errors.h"

  #if defined(STM32F2XX) || defined(STM32F4XX)
    #include "usbd_cdc_vcp.h"
    #include "usbd_desc.h"
    #include "usbd_usr.h"

  #elif defined(STM32F1XX) || defined(STM32F30X)
    #include "usb_init.h"
    #include "usb_prop.h"

  #elif defined(STM32L4XX) || defined(STM32F0XX)
    #include "usb_device.h"
    #include "usbd_cdc.h"

  #else
    #error Controller type is not handled
  #endif

  #include "app_usb.h"

  #define APP_USB_DEBUG_EN 0

  #pragma pack(1)
typedef enum
{
  SOF,
  PrepareRxFromSOF,
  RxLockedFromSOF,
  DataOut,
  Overflow,
  PrepareRxFromDataOut,
  RxLockedFromDataOut,
  ErrorSync,
  TxPacket,
  TxRemainLen,
  RxWrongLen,
} test_event_id_t;

typedef struct
{
  test_event_id_t id;
  uint16_t param;
  uint16_t tick;
} test_event_t;
  #pragma pack()

void app_usb_tx_packet(void *param);
void app_usb_rx_fifo_refresh_lock(void);

  #if (APP_USB_DEBUG_EN)

RAM_APP_SEC test_event_t test_event_buf[256];
RAM_APP_SEC uint8_t test_event_idx;
RAM_APP_SEC uint8_t rx_num_idx;
RAM_APP_SEC uint8_t rx_num;

    #define NEW_EVENT(event)                                                                       \
      test_event_buf[test_event_idx].id = event;                                                   \
      test_event_buf[test_event_idx++].tick = TIM3->CNT
    #define NEW_EVENT_PARAM(event, par)                                                            \
      test_event_buf[test_event_idx].id = event;                                                   \
      test_event_buf[test_event_idx].param = par;                                                  \
      test_event_buf[test_event_idx++].tick = TIM3->CNT

  #else

    #define NEW_EVENT(event)                                                                       \
      {                                                                                            \
      }
    #define NEW_EVENT_PARAM(event, par)                                                            \
      {                                                                                            \
      }
  #endif

  #if defined(STM32F2XX) || defined(STM32F4XX)

USB_OTG_CORE_HANDLE USB_OTG_dev;

  #elif defined(STM32F1XX) || defined(STM32F30X)

  #elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)

USBD_HandleTypeDef USBD_Device;

  #else
    #error "Controller type is not defined"
  #endif

/// Linear transmit buffer, filled right before a USB packet is formed.
uint8_t app_usb_tx_buf[APP_USB_TX_BUF_SIZE];
/// Linear receive buffer.
uint8_t app_usb_rx_buf[APP_USB_RX_BUF_SIZE];

/// Transmit FIFO buffer.
uint8_t app_usb_tx_fifo_buf[APP_USB_TX_FIFO_BUF_SIZE];
/// Receive FIFO buffer.
uint8_t app_usb_rx_fifo_buf[APP_USB_RX_FIFO_BUF_SIZE];

app_usb_t app_usb;

void app_usb_init(void)
{
  #if (APP_USB_DEBUG_EN)
  test_event_idx = 0;
  #endif

  app_usb.tx_in_prog = 0;
  app_usb.tx_fifo.head = app_usb.tx_fifo.tail = app_usb.tx_fifo.idx = 0;
  app_usb.tx_fifo.buf = app_usb_tx_fifo_buf;
  app_usb.tx_fifo.size = APP_USB_TX_FIFO_BUF_SIZE;
  app_usb.tx_fifo.overflow = 0;

  app_usb.rx_locked = 0;
  app_usb.rx_fifo.head = app_usb.rx_fifo.tail = app_usb.rx_fifo.idx = 0;
  app_usb.rx_fifo.buf = app_usb_rx_fifo_buf;
  app_usb.rx_fifo.size = APP_USB_RX_FIFO_BUF_SIZE;
  app_usb.rx_fifo.overflow = 0;

  app_usb.wait_rx = 0;
  app_usb.last_tx_size = 0;

  #if defined(STM32F2XX) || defined(STM32F4XX)
  USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

  #elif defined(STM32F1XX) || defined(STM32F30X)
  USB_Init();

  #elif defined(STM32L4XX) || defined(STM32F0XX)

  MX_USB_DEVICE_Init();

  #else
    #error Controller type is not handled
  #endif
}

void app_usb_disable_irq(void)
{
  #if defined(STM32F2XX) || defined(STM32F4XX)
  NVIC_DisableIRQ(OTG_FS_IRQn);

  #elif defined(STM32F1XX) || defined(STM32F30X)
  hal_assert(false);

  #elif defined(STM32L4XX)
  NVIC_DisableIRQ(OTG_FS_IRQn);

  #elif defined(STM32F0XX)
  NVIC_DisableIRQ(USB_IRQn);

  #else
    #error Controller type is not handled
  #endif
}

void app_usb_SOF(void *param)
{
  if (app_usb.tx_in_prog == 0)
  {
    if (app_usb.tx_fifo.idx)
    {
      app_usb.tx_in_prog = 1;

      app_usb_tx_packet(param);
    }
  }

  NEW_EVENT(SOF);

  app_usb_rx_fifo_refresh_lock();

  uint32_t save;
  ENTER_CRITICAL(save);

  if ((app_usb.rx_locked == 0) && (app_usb.wait_rx == 0))
  {
    NEW_EVENT(PrepareRxFromSOF);

    app_usb.wait_rx = 1;

  #if defined(STM32F2XX) || defined(STM32F4XX)
    DCD_EP_PrepareRx(param, USB_EP_ADDR__VCP_DATA_OUT, app_usb_rx_buf, APP_USB_RX_BUF_SIZE);
  #elif defined(STM32F1XX) || defined(STM32F30X)
    SetEPRxValid(USB_EP__VCP_DATA);
  #elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)
    USBD_LL_PrepareReceive(param, USB_EP_ADDR__VCP_DATA_OUT, app_usb_rx_buf, APP_USB_RX_BUF_SIZE);
  #else
    #error "Controller type is not defined"
  #endif
  }
  else
  {
    NEW_EVENT(RxLockedFromSOF);
  }

  LEAVE_CRITICAL(save);
}

void app_usb_IN(void *param)
{
  if (app_usb.tx_in_prog)
  {
    if (app_usb.tx_fifo.idx == 0)
    {
      app_usb.tx_in_prog = 0;
    }

    app_usb_tx_packet(param);
  }
}

void app_usb_OUT(void *param)
{
  uint32_t save;
  ENTER_CRITICAL(save);

  app_usb_fifo_t *fifo = &app_usb.rx_fifo;

  #if defined(STM32F2XX) || defined(STM32F4XX)
  uint16_t rx_len =
      ((USB_OTG_CORE_HANDLE *)param)->dev.out_ep[USB_EP_ADDR__VCP_DATA_OUT].xfer_count;
  #elif defined(STM32F1XX) || defined(STM32F30X)
  uint16_t rx_len = USB_SIL_Read(USB_EP_ADDR__VCP_DATA_OUT, app_usb_rx_buf);
  #elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)
  uint16_t rx_len = USBD_LL_GetRxDataSize(param, USB_EP_ADDR__VCP_DATA_OUT);
  #endif

  NEW_EVENT_PARAM(DataOut, rx_len);

  app_usb.wait_rx = 0;

  if (rx_len == 0)
  {
    NEW_EVENT_PARAM(RxWrongLen, 0);
  }
  else if (rx_len > APP_USB_RX_BUF_SIZE)
  {
    NEW_EVENT_PARAM(RxWrongLen, rx_len);
    rx_len = 0;
  }

  app_usb_rx_fifo_refresh_lock();

  if (rx_len > (fifo->size - fifo->idx))
  {
    rx_len = fifo->size - fifo->idx;
    fifo->overflow = 1;

    NEW_EVENT(Overflow);
  }

  if ((fifo->head + rx_len) <= APP_USB_RX_FIFO_BUF_SIZE)
  {
    memcpy(&fifo->buf[fifo->head], app_usb_rx_buf, rx_len);
    fifo->head += rx_len;
    if (fifo->head >= APP_USB_RX_FIFO_BUF_SIZE)
    {
      fifo->head = 0;
    }
  }
  else
  {
    uint16_t rx_len_part_1 = fifo->size - fifo->head;
    uint16_t rx_len_part_2 = rx_len - rx_len_part_1;
    memcpy(&fifo->buf[fifo->head], &app_usb_rx_buf[0], rx_len_part_1);
    memcpy(&fifo->buf[0], &app_usb_rx_buf[rx_len_part_1], rx_len_part_2);
    fifo->head = rx_len_part_2;
  }
  fifo->idx += rx_len;

  if ((app_usb.rx_locked == 0) && (app_usb.wait_rx == 0))
  {
    NEW_EVENT(PrepareRxFromDataOut);

    app_usb.wait_rx = 1;

  #if defined(STM32F2XX) || defined(STM32F4XX)
    DCD_EP_PrepareRx(param, USB_EP_ADDR__VCP_DATA_OUT, app_usb_rx_buf, APP_USB_RX_BUF_SIZE);
  #elif defined(STM32F1XX) || defined(STM32F30X)
    SetEPRxValid(USB_EP__VCP_DATA);
  #elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)
    USBD_LL_PrepareReceive(param, USB_EP_ADDR__VCP_DATA_OUT, app_usb_rx_buf, APP_USB_RX_BUF_SIZE);
  #else
    #error "Controller type is not defined"
  #endif
  }
  else
  {
    NEW_EVENT(RxLockedFromDataOut);
  }

  LEAVE_CRITICAL(save);
}

error_t usb_is_plugged(void)
{
  error_t res = ERR_OK;

  #if defined(STM32F2XX) || defined(STM32F4XX)
  if (USB_OTG_dev.dev.device_status != USB_OTG_CONFIGURED)
  {
    res = ERR_RESULT;
  }
  #elif defined(STM32F1XX) || defined(STM32F30X)
  if (bDeviceState != CONFIGURED)
  {
    res = ERR_RESULT;
  }
  #elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)
  if (USBD_Device.dev_state != USBD_STATE_CONFIGURED)
  {
    res = ERR_RESULT;
  }
  #else
    #error "Controller type is not defined"
  #endif

  return res;
}

void app_usb_rx_fifo_refresh_lock(void)
{
  uint32_t save;
  ENTER_CRITICAL(save);

  if (app_usb.rx_fifo.idx > (app_usb.rx_fifo.size - APP_USB_RX_BUF_SIZE * 2))
  {
    app_usb.rx_locked = 1;
  }
  else
  {
    app_usb.rx_locked = 0;
  }

  LEAVE_CRITICAL(save);
}

void app_usb_tx_packet(void *param)
{
  uint32_t save;
  ENTER_CRITICAL(save);

  uint16_t tx_len = app_usb.tx_fifo.idx;
  if (tx_len > APP_USB_TX_BUF_SIZE)
  {
    tx_len = APP_USB_TX_BUF_SIZE;
  }

  // Another workaround: it was found that sending a 0-byte packet after a packet
  // shorter than 64 bytes hangs the driver - DataIN interrupts stop coming
  if ((tx_len == 0) && (app_usb.last_tx_size != 64))
  {
  }
  else
  {
    if ((app_usb.tx_fifo.tail + tx_len) <= app_usb.tx_fifo.size)
    {
      memcpy(app_usb_tx_buf, &app_usb.tx_fifo.buf[app_usb.tx_fifo.tail], tx_len);
      app_usb.tx_fifo.tail += tx_len;
      if (app_usb.tx_fifo.tail >= app_usb.tx_fifo.size)
      {
        app_usb.tx_fifo.tail -= app_usb.tx_fifo.size;
      }
    }
    else
    {
      uint16_t tx_len_part_1 = app_usb.tx_fifo.size - app_usb.tx_fifo.tail;
      uint16_t tx_len_part_2 = tx_len - tx_len_part_1;
      memcpy(&app_usb_tx_buf[0], &app_usb.tx_fifo.buf[app_usb.tx_fifo.tail], tx_len_part_1);
      memcpy(&app_usb_tx_buf[tx_len_part_1], &app_usb.tx_fifo.buf[0], tx_len_part_2);
      app_usb.tx_fifo.tail = tx_len_part_2;
    }
    app_usb.tx_fifo.idx -= tx_len;

  #if defined(STM32F2XX) || defined(STM32F4XX)
    DCD_EP_Tx(param, USB_EP_ADDR__VCP_DATA_IN, app_usb_tx_buf, tx_len);
  #elif defined(STM32F1XX) || defined(STM32F30X)
    UserToPMABufferCopy(app_usb_tx_buf, USB_EP_TXADDR__VCP_DATA_IN, tx_len);
    SetEPTxCount(USB_EP__VCP_DATA, tx_len);
    SetEPTxValid(USB_EP__VCP_DATA);
  #elif defined(STM32F7XX) || defined(STM32L4XX) || defined(STM32F0XX)
    USBD_LL_Transmit(param, USB_EP_ADDR__VCP_DATA_IN, app_usb_tx_buf, tx_len);
  #else
    #error "Controller type is not defined"
  #endif

    NEW_EVENT_PARAM(TxPacket, tx_len);
    app_usb.last_tx_size = tx_len;
  }

  LEAVE_CRITICAL(save);
}

error_t app_usb_fifo_put(app_usb_fifo_t *fifo, uint8_t data)
{
  error_t res = ERR_BOF;

  uint32_t save;
  ENTER_CRITICAL(save);

  if (fifo->idx < fifo->size)
  {
    fifo->buf[fifo->head++] = data;
    if (fifo->head >= fifo->size)
    {
      fifo->head = 0;
    }
    fifo->idx++;

    res = ERR_OK;
  }

  LEAVE_CRITICAL(save);

  return res;
}

error_t app_usb_fifo_get(app_usb_fifo_t *fifo, uint8_t *data)
{
  error_t res = ERR_BUF;

  uint32_t save;
  ENTER_CRITICAL(save);

  if (fifo->idx)
  {
    *data = fifo->buf[fifo->tail++];
    if (fifo->tail >= fifo->size)
    {
      fifo->tail = 0;
    }
    fifo->idx--;

    res = ERR_OK;
  }

  LEAVE_CRITICAL(save);

  return res;
}

error_t usb_tx_fifo_put_one(uint8_t data)
{
  return app_usb_fifo_put(&app_usb.tx_fifo, data);
}

error_t usb_rx_fifo_get_one(uint8_t *data)
{
  return app_usb_fifo_get(&app_usb.rx_fifo, data);
}

uint16_t usb_rx_fifo_get_idx(void)
{
  return app_usb.rx_fifo.idx;
}

void usb_rx_fifo_flush(void)
{
  uint32_t save;
  ENTER_CRITICAL(save);

  app_usb.rx_fifo.idx = 0;
  app_usb.rx_fifo.tail = app_usb.rx_fifo.head;

  LEAVE_CRITICAL(save);
}

void usb_tx_fifo_flush(void)
{
  uint32_t save;
  ENTER_CRITICAL(save);

  app_usb.tx_fifo.idx = 0;
  app_usb.tx_fifo.tail = app_usb.tx_fifo.head;

  LEAVE_CRITICAL(save);
}

/**
 * Get the amount of free space in the transmit buffer.
 */
uint16_t usb_tx_fifo_get_empty(void)
{
  return (app_usb.tx_fifo.size - app_usb.tx_fifo.idx);
}

#endif // (FEATURE_ENABLE_USB == 1)
