#pragma once

/**
 * @file usb_config.h
 * @brief USB packet and application buffer size configuration.
 */

#define USB_MAX_PACKET_SIZE 64

#define APP_USB_TX_BUF_SIZE USB_MAX_PACKET_SIZE
#define APP_USB_TX_FIFO_BUF_SIZE 512

#define APP_USB_RX_BUF_SIZE USB_MAX_PACKET_SIZE
#define APP_USB_RX_FIFO_BUF_SIZE 512

#define APP_HID_RX_BUF_SIZE 2
#define APP_HID_TX_BUF_SIZE 64

#define VCP_INT_BUF_SIZE 8

#if (USE_USB_HID_EN)
  #define APP_USB_HID_TX_BUF_SIZE USB_MAX_PACKET_SIZE
  #define APP_USB_HID_RX_BUF_SIZE 2
#else
  #define APP_USB_HID_TX_BUF_SIZE 0
  #define APP_USB_HID_RX_BUF_SIZE 0
#endif

#if (USE_USB_MS_EN)
  #define APP_USB_MS_TX_BUF_SIZE USB_MAX_PACKET_SIZE
  #define APP_USB_MS_RX_BUF_SIZE USB_MAX_PACKET_SIZE
#else
  #define APP_USB_MS_TX_BUF_SIZE 0
  #define APP_USB_MS_RX_BUF_SIZE 0
#endif
