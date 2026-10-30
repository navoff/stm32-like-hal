#pragma once

/**
 * @file usb_desc_device.h
 * @brief USB device, configuration, HID report and string descriptor declarations.
 */

#include <stdint.h>

#include "config.h"

#define DEVICE_DESCRIPTOR_SIZE 18
#define DEVICE_QUALIFIER_DESCRIPTOR_SIZE 10

#define DEVICE_STRING_LANDID_SIZE 4
#define DEVICE_STRING_VENDOR_SIZE 16
#define DEVICE_STRING_PRODUCT_SIZE 14
#define DEVICE_STRING_SERIAL_SIZE 24

#define DEVICE_INTERFACE_IDX -1

#if (USE_USB_HID_EN)
  #define HID_CONFIG_DESCRIPTOR_SIZE 32
  #define HID_INTERFACE_NUM 1
  #define HID_INTERFACE_IDX DEVICE_INTERFACE_IDX + 1
#else
  #define HID_CONFIG_DESCRIPTOR_SIZE 0
  #define HID_INTERFACE_NUM 0
  #define HID_INTERFACE_IDX DEVICE_INTERFACE_IDX
#endif
#define HID_REPORT_DESCRIPTOR_SIZE 43
#define HID_DESCRIPTOR_OFFSET 18
#define HID_DESCRIPTOR_SIZE 9

#if (USE_USB_VCP_EN)
  #define VCP_CONFIG_DESCRIPTOR_SIZE 58
  #define VCP_INTERFACE_NUM 2
  #define VCP_INTERFACE_IDX HID_INTERFACE_IDX + 1
#else
  #define VCP_CONFIG_DESCRIPTOR_SIZE 0
  #define VCP_INTERFACE_NUM 0
  #define VCP_INTERFACE_IDX HID_INTERFACE_IDX
#endif

#define COMMON_CONFIG_DESCRIPTOR_SIZE 9

#define DEVICE_CONFIG_DESCRIPTOR_SIZE                                                              \
  (HID_CONFIG_DESCRIPTOR_SIZE + VCP_CONFIG_DESCRIPTOR_SIZE + COMMON_CONFIG_DESCRIPTOR_SIZE)

#define DEVICE_INTERFACE_NUM (HID_INTERFACE_NUM + VCP_INTERFACE_NUM)

extern const uint8_t DeviceDescriptor[DEVICE_DESCRIPTOR_SIZE];
extern const uint8_t HID_ReportDescriptor[HID_REPORT_DESCRIPTOR_SIZE];
extern const uint8_t DeviceQualifierDescription[DEVICE_QUALIFIER_DESCRIPTOR_SIZE];
extern const uint8_t DeviceConfigDescriptor[DEVICE_CONFIG_DESCRIPTOR_SIZE];
extern const uint8_t Device_StringLangID[DEVICE_STRING_LANDID_SIZE];

#if defined(STM32F2XX) || defined(STM32F4XX) || defined(STM32F7XX) || defined(STM32L4XX) ||        \
    defined(STM32F0XX)

extern const char usbd_manufacturer_string[];
extern const char usbd_product_hs_string[];
extern const char usbd_serialnumber_hs_string[];
extern const char usbd_product_fs_string[];
extern const char usbd_serialnumber_fs_string[];
extern const char usbd_configuration_hs_string[];
extern const char usbd_interface_hs_string[];
extern const char usbd_configuration_fs_string[];
extern const char usbd_interface_fs_string[];

#elif defined(STM32F1XX) || defined(STM32F30X)

extern const uint8_t Device_StringVendor[];
extern const uint8_t Device_StringProduct[];
extern const uint8_t Device_StringSerial[];

#else

  #error "Controller type is not defined"

#endif
