#pragma once

/**
 * @file fw_info.h
 * @brief Firmware information structure placed at the end of the image for the bootloader.
 */

#include <stdint.h>

/// Structure size
#define FW_INFO_STRUCT_SIZE 128

#define FW_INFO_MAGIC_NUMBER 0x2B3386EC

#define PLATFORM_TYPE_STR_SIZE 32

#define FW_VERSION_STR_SIZE 64

#pragma pack(push, 1)

/// Structure holding information about the device type and firmware, intended for the bootloader
/// @note !!! When the structure fields change, FW_INFO_STRUCT_VERSION must be incremented
typedef union
{
  struct
  {
    /// Magic number
    /// Makes the structure easy to spot in hex editors
    /// and serves as an extra check when flashing through the bootloader
    /// @ref FW_INFO_MAGIC_NUMBER
    uint32_t magic_number;

    /// Structure version
    uint32_t version;

    /// Platform identifier
    uint8_t platform_type[PLATFORM_TYPE_STR_SIZE];

    /// Selected board type (identifier) (@ref HW_TYPE_LIST)
    uint32_t hw_type;

    /// Firmware version (and build number)
    uint8_t fw_version[FW_VERSION_STR_SIZE];

    uint8_t reserved[16];

    /// Checksum of the whole firmware
    /// Computed and inserted during the automated build.
    /// Must be the last field in the structure.
    /// Since the structure itself is also the last thing in the firmware, integrity is easy
    /// to check: the checksum of the whole firmware file must be equal to 0.
    /// Computed with the "crc32_ethernet" algorithm
    uint32_t fw_crc;
  };

  uint8_t buff[FW_INFO_STRUCT_SIZE];
} fw_info_t;

#pragma pack(pop)

#if defined(__GNUC__)

extern const uint32_t __app_rom_begin;

  /// Firmware area start address from the linker file
  #define FW_ROM_BEGIN_ADDRESS ((uint32_t *)&__app_rom_begin)

extern const uint32_t __mcu_rom_begin;

  /// Flash area start address from the linker file
  #define MCU_ROM_BEGIN_ADDRESS ((uint32_t *)&__mcu_rom_begin)

  #define FW_INFO_SECTION_PREFIX __attribute((__section__(".fw_info")))
  #define FW_INFO_SECTION_SUFFIX

#else
  #error Only GCC (arm-none-eabi-gcc) is supported
#endif

extern const fw_info_t fw_info;
