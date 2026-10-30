/**
 * @file usb_desc_device.c
 * @brief USB device, configuration, HID report and string descriptors.
 */

#include "config.h"

#if (FEATURE_ENABLE_USB)

  #include "usb_conf.h"
  #include "usb_types.h"

  #include "usb_desc_device.h"

  /// LangID = 0x0409: U.S. English
  #define USBD_LANGID_STRING 0x409

const uint8_t Device_StringLangID[DEVICE_STRING_LANDID_SIZE] = {
    DEVICE_STRING_LANDID_SIZE, USB_DESCRIPTOR_TYPE__STRING, USBD_LANGID_STRING & 0xFF,
    USBD_LANGID_STRING >> 8};

/* USB Standard Device Descriptor */
const uint8_t DeviceDescriptor[DEVICE_DESCRIPTOR_SIZE] = {
    DEVICE_DESCRIPTOR_SIZE, /* bLength */
    USB_DESCRIPTOR_TYPE__DEVICE, /* bDescriptorType */
    0x00,
    0x02, /* bcdUSB = 2.00 */
    USB_DEVICE_CLASS__DEVICE, /* bDeviceClass */
    0x00, /* bDeviceSubClass */
    0x00, /* bDeviceProtocol */
    USB_MAX_PACKET_SIZE, /* bMaxPacketSize */
    VENDOR_ID & 0xFF,
    VENDOR_ID >> 8, /* idVendor */
    PRODUCT_ID & 0xFF,
    PRODUCT_ID >> 8, /* idProduct */
    0x00,
    0x02, /* bcdDevice = 2.00 */
    1, /* Index of string descriptor describing manufacturer */
    2, /* Index of string descriptor describing product */
    3, /* Index of string descriptor describing the device's serial number */
    0x01 /* bNumConfigurations */
};

/* USB Standard Device Descriptor */
const uint8_t DeviceQualifierDescription[DEVICE_QUALIFIER_DESCRIPTOR_SIZE] = {
    DEVICE_QUALIFIER_DESCRIPTOR_SIZE, /* bLength */
    USB_DESCRIPTOR_TYPE__DEVICE_QUALIFIER, /* bDescriptorType */
    0x00,
    0x02, /* bcdUSB = 2.00 */
    USB_DEVICE_CLASS__DEVICE, /* bDeviceClass */
    0x00, /* bDeviceSubClass */
    0x00, /* bDeviceProtocol */
    USB_MAX_PACKET_SIZE, /* bMaxPacketSize */
    0x01, /* bNumConfigurations */
    0x00, /* bReserved */
};

const uint8_t DeviceConfigDescriptor[DEVICE_CONFIG_DESCRIPTOR_SIZE] = {
    /* Configuration Descriptor */
    COMMON_CONFIG_DESCRIPTOR_SIZE, /* bLength */
    USB_DESCRIPTOR_TYPE__CONFIGURATION, /* bDescriptorType */
    DEVICE_CONFIG_DESCRIPTOR_SIZE, 0, /* wTotalLength */
    DEVICE_INTERFACE_NUM, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0xC0, /* bmAttributes: self powered */
    0x32, /* MaxPower 100 mA */

  #if (USE_USB_HID_EN)
    /* Interface Descriptor */
    0x09, /* bLength */
    USB_DESCRIPTOR_TYPE__INTERFACE, /* bDescriptorType */
    HID_INTERFACE_IDX, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x02, /* bNumEndpoints */
    USB_DEVICE_CLASS__HID, /* bInterfaceClass */
    USB_HID_SUB_CLASS__NO_BOOT, /* bInterfaceSubClass */
    0x00, /* nInterfaceProtocol */
    0, /* iInterface */

    /* HID Descriptor */
    /* The position must match the HID_DESCRIPTOR_OFFSET value */
    HID_DESCRIPTOR_SIZE, /* bLength */
    USB_DESCRIPTOR_TYPE__HID, /* bDescriptorType */
    0x10, 0x01, /* bcdHID */
    0x00, /* bCountryCode */
    0x01, /* bNumDescriptor */
    USB_DESCRIPTOR_TYPE__HID_REPORT, /* bDescriptorType */
    HID_REPORT_DESCRIPTOR_SIZE, 0x00, /* wItemLength: Total length of Report descriptor */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    USB_DESCRIPTOR_TYPE__ENDPOINT, /* bDescriptorType */
    HID_EP_IN_ADDR, /* bEndpointAddress */
    USB_EP_TYPE__INTERRUPT, /* bmAttributes */
    APP_HID_TX_BUF_SIZE, 0x00, /* wMaxPacketSize */
    0x01, /* bInterval */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    USB_DESCRIPTOR_TYPE__ENDPOINT, /* bDescriptorType */
    HID_EP_OUT_ADDR, /* bEndpointAddress */
    USB_EP_TYPE__INTERRUPT, /* bmAttributes */
    APP_HID_RX_BUF_SIZE, 0x00, /* wMaxPacketSize */
    0x01, /* bInterval */
  #endif

  #if (USE_USB_VCP_EN)
    /* Interface Descriptor */
    0x09, /* bLength */
    USB_DESCRIPTOR_TYPE__INTERFACE, /* bDescriptorType */
    VCP_INTERFACE_IDX, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x01, /* bNumEndpoints */
    USB_DEVICE_CLASS__COMM_AND_CDC_CTRL, /* bInterfaceClass */
    USB_CDC_SUB_CLASS__ABST_CNTRL_MODEL, /* bInterfaceSubClass */
    USB_CDC_INTERFACE_PROTOCOL__AT, /* bInterfaceProtocol */
    0x00, /* iInterface */

    /* Header Functional Descriptor */
    0x05, /* bLength */
    USB_DESCRIPTOR_TYPE__CS_INTERFACE, /* bDescriptorType */
    0x00, /* bDescriptorSubtype */
    0x10, /* bcdCDC */
    0x01,

    /* Call Management Functional Descriptor */
    0x05, /* bFunctionLength */
    USB_DESCRIPTOR_TYPE__CS_INTERFACE, /* bDescriptorType */
    USB_DESCRIPTOR_SUBTYPE__CMD, /* bDescriptorSubtype */
    0x00, /* bmCapabilities */
    0x01, /* bDataInterface */

    /* ACM Functional Descriptor */
    0x04, /* bFunctionLength */
    USB_DESCRIPTOR_TYPE__CS_INTERFACE, /* bDescriptorType */
    USB_DESCRIPTOR_SUBTYPE__ACMD, /* bDescriptorSubtype */
    0x02, /* bmCapabilities */

    /* Union Functional Descriptor */
    0x05, /* bFunctionLength */
    USB_DESCRIPTOR_TYPE__CS_INTERFACE, /* bDescriptorType */
    USB_DESCRIPTOR_SUBTYPE__UIFD, /* bDescriptorSubtype */
    0x00, /* bMasterInterface: Communication class interface */
    0x01, /* bSlaveInterface0: Data Class Interface */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    USB_DESCRIPTOR_TYPE__ENDPOINT, /* bDescriptorType */
    USB_EP_ADDR__VCP_INT_IN, /* bEndpointAddress */
    USB_EP_TYPE__INTERRUPT, /* bmAttributes */
    USB_EP_SIZE__VCP_INT_IN, 0x00, /* wMaxPacketSize */
    0xFF, /* bInterval */

    /* Interface Descriptor */
    0x09, /* bLength */
    USB_DESCRIPTOR_TYPE__INTERFACE, /* bDescriptorType */
    (VCP_INTERFACE_IDX + 1), /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    0x02, /* bNumEndpoints */
    USB_DEVICE_CLASS__CDC, /* bInterfaceClass */
    0, /* bInterfaceSubClass */
    USB_CDC_INTERFACE_PROTOCOL__AT, /* bInterfaceProtocol */
    0x00, /* iInterface */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    USB_DESCRIPTOR_TYPE__ENDPOINT, /* bDescriptorType */
    USB_EP_ADDR__VCP_DATA_IN, /* bEndpointAddress */
    USB_EP_TYPE__BULK, /* bmAttributes */
    USB_EP_SIZE__VCP_DATA_IN, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */

    /* Endpoint Descriptor */
    0x07, /* bLength */
    USB_DESCRIPTOR_TYPE__ENDPOINT, /* bDescriptorType */
    USB_EP_ADDR__VCP_DATA_OUT, /* bEndpointAddress */
    USB_EP_TYPE__BULK, /* bmAttributes */
    USB_EP_SIZE__VCP_DATA_OUT, 0x00, /* wMaxPacketSize */
    0x00, /* bInterval */
  #endif
};

const uint8_t HID_ReportDescriptor[HID_REPORT_DESCRIPTOR_SIZE] = {
    0x06, 0xFF, 0x00, /* USAGE_PAGE (Vendor Page: 0xFF00) */
    0x09, 0x01, /* USAGE (Demo Kit)               */
    0xa1, 0x01, /* COLLECTION (Application)       */

    0x85, 0x01, /*     REPORT_ID (1)        */
    0x09, 0x01, /*     USAGE (LED 1)              */
    0x15, 0x00, /*     LOGICAL_MINIMUM (0)        */
    0x25, 0x01, /*     LOGICAL_MAXIMUM (1)        */
    0x75, 0x08, /*     REPORT_SIZE (8)            */
    0x95, 0x01, /*     REPORT_COUNT (1)           */
    0xB1, 0x82, /*     FEATURE (Data,Var,Abs,Vol) */
    0x85, 0x01, /*     REPORT_ID (1)              */
    0x09, 0x01, /*     USAGE (LED 1)              */
    0x91, 0x82, /*     OUTPUT (Data,Var,Abs,Vol)  */

    0x85, 0x07, /*     REPORT_ID (7)              */
    0x09, 0x07, /*     USAGE (ADC IN)             */
    0x15, 0x00, /*     LOGICAL_MINIMUM (0)        */
    0x26, 0xff, 0x01, /*     LOGICAL_MAXIMUM (255)      */
    0x75, 0x08, /*     REPORT_SIZE (8)            */
    0x95, 0x3F, /*     REPORT_COUNT (8)           */
    0x81, 0x82, /*     INPUT (Data,Var,Abs,Vol)   */

    0xc0 /*     END_COLLECTION              */
};

  #if defined(STM32F2XX) || defined(STM32F4XX) || defined(STM32F7XX) || defined(STM32L4XX) ||      \
      defined(STM32F0XX)

const char usbd_manufacturer_string[] = "Company";

const char usbd_product_hs_string[] = "Device";
const char usbd_serialnumber_hs_string[] = "00000000050B";

const char usbd_product_fs_string[] = "Device";
const char usbd_serialnumber_fs_string[] = "00000000050C";

const char usbd_configuration_hs_string[] = "VCP Config";
const char usbd_interface_hs_string[] = "VCP Interface";
const char usbd_configuration_fs_string[] = "VCP Config";
const char usbd_interface_fs_string[] = "VCP Interface";

  #elif defined(STM32F1XX) || defined(STM32F30X)

const uint8_t Device_StringVendor[DEVICE_STRING_VENDOR_SIZE] = {
    DEVICE_STRING_VENDOR_SIZE, /* Size of Vendor string */
    USB_DESCRIPTOR_TYPE__STRING, /* bDescriptorType*/
    'C',
    0,
    'o',
    0,
    'm',
    0,
    'p',
    0,
    'a',
    0,
    'n',
    0,
    'y',
    0,
};

const uint8_t Device_StringProduct[DEVICE_STRING_PRODUCT_SIZE] = {
    DEVICE_STRING_PRODUCT_SIZE, /* bLength */
    USB_DESCRIPTOR_TYPE__STRING, /* bDescriptorType */
    'D',
    0,
    'e',
    0,
    'v',
    0,
    'i',
    0,
    'c',
    0,
    'e',
    0};

const uint8_t Device_StringSerial[DEVICE_STRING_SERIAL_SIZE] = {
    DEVICE_STRING_SERIAL_SIZE, /* bLength */
    USB_DESCRIPTOR_TYPE__STRING, /* bDescriptorType */
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0,
    '0',
    0};

  #else
    #error "Controller type is not defined"
  #endif

#endif // (FEATURE_ENABLE_USB == 1)
