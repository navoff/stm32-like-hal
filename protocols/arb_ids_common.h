#pragma once

/**
 * @file arb_ids_common.h
 * @brief System and component identifiers of the ARB protocol.
 */

/// Identifier of an unknown device
#define ARB_COMPONENT_ID__UNDEF 0xFF

/// Boot loader identifier
#define ARB_COMPONENT_ID__BOOT_LOADER 0x01

/**
 * @defgroup ARB_BOOT_LOADER
 * ARB boot loader packet identifiers
 * @{
 */
/// Packet identifier "Leave the boot loader"
#define ARB_BOOT_LOADER_PACKET_ID__LEAVE_LOADER 0x00

/// Packet identifier "Prepare the boot loader for firmware transfer"
#define ARB_BOOT_LOADER_PACKET_ID__START_LOAD_FW 0x01

/// Packet identifier "Transfer a firmware segment"
#define ARB_BOOT_LOADER_PACKET_ID__LOAD_FW_SEGMENT 0x02

/// Packet identifier "Apply the transferred firmware"
#define ARB_BOOT_LOADER_PACKET_ID__APPLY_FW 0x03

/// Packet identifier "Command execution result"
#define ARB_BOOT_LOADER_PACKET_ID__COMMAND_RESULT 0x10
/** @} */

/// Request for the firmware transfer status
#define ARB_PACKET_ID__GET_LOADER_STATUS 0xFB

/// Firmware transfer status
#define ARB_PACKET_ID__LOADER_STATUS 0xFC

/// Packet identifier "Enter the boot loader"
/// Same for all components
#define ARB_PACKET_ID__GO_TO_LOADER 0xFD

/// Packet identifier "Request brief device (component) info"
/// Same for all components
#define ARB_PACKET_ID__GET_BRIEF_DEVICE_INFO 0xFE

/// Packet identifier "Brief device (component) info"
/// Same for all components
#define ARB_PACKET_ID__BRIEF_DEVICE_INFO 0xFF
