#pragma once

/**
 * @file usb_types.h
 * @brief USB descriptor type, class, subclass and protocol constants.
 */

/** @defgroup bDescriptorType
 * @{
 */
#define USB_DESCRIPTOR_TYPE__DEVICE 0x01
#define USB_DESCRIPTOR_TYPE__CONFIGURATION 0x02
#define USB_DESCRIPTOR_TYPE__STRING 0x03
#define USB_DESCRIPTOR_TYPE__INTERFACE 0x04
#define USB_DESCRIPTOR_TYPE__ENDPOINT 0x05
#define USB_DESCRIPTOR_TYPE__DEVICE_QUALIFIER 0x06
#define USB_DESCRIPTOR_TYPE__OTHER_SPEED_CNFG 0x07
#define USB_DESCRIPTOR_TYPE__BOS 0x0F
#define USB_DESCRIPTOR_TYPE__HID 0x21
#define USB_DESCRIPTOR_TYPE__HID_REPORT 0x22
#define USB_DESCRIPTOR_TYPE__CS_INTERFACE 0x24
/** @} */

/** @defgroup bDescriptorSubtype
 * @{
 */
/// Header functional descriptor
#define USB_DESCRIPTOR_SUBTYPE__HFD 0x00
/// Call management descriptor
#define USB_DESCRIPTOR_SUBTYPE__CMD 0x01
/// Abstract control management descriptor
#define USB_DESCRIPTOR_SUBTYPE__ACMD 0x02
/// Union interface functional descrtptor
#define USB_DESCRIPTOR_SUBTYPE__UIFD 0x06
/** @} */

/** @defgroup bDeviceClass
 * @{
 */
/// Use class information in the Interface Descriptors
#define USB_DEVICE_CLASS__DEVICE 0x00
/// Communications and CDC Control
#define USB_DEVICE_CLASS__COMM_AND_CDC_CTRL 0x02
/// HID (Human Interface Device)
#define USB_DEVICE_CLASS__HID 0x03
/// Mass Storage
#define USB_DEVICE_CLASS__MS 0x08
/// CDC-Data
#define USB_DEVICE_CLASS__CDC 0x0A
/** @} */

/** @defgroup CDC
 * @{
 */
/// Abstract Control Model
#define USB_EP_TYPE__CTRL 0x00
#define USB_EP_TYPE__ISOCHRONOUS 0x01
#define USB_EP_TYPE__BULK 0x02
#define USB_EP_TYPE__INTERRUPT 0x03
/** @} */

/** @defgroup CDC bInterfaceSubClass
 * @{
 */
/// Abstract Control Model
#define USB_CDC_SUB_CLASS__ABST_CNTRL_MODEL 0x02
/** @} */

/** @defgroup HID bInterfaceSubClass
 * @{
 */
#define USB_HID_SUB_CLASS__NO_BOOT 0x00
#define USB_HID_SUB_CLASS__BOOT 0x01
/** @} */

/** @defgroup CDC bInterfaceProtocol
 * @{
 */
/// No special protocol
#define USB_CDC_INTERFACE_PROTOCOL__VOID 0x00
/// Common AT commands
#define USB_CDC_INTERFACE_PROTOCOL__AT 0x01
/** @} */
