#pragma once

/**
 * @file nvram_map.h
 * @brief Layout of an NVRAM segment: user settings plus system fields.
 */

#include <stdint.h>

#include "config.h"
#include "nvram_settings.h"

/// Segment validity marker
#define NVRAM_FLASH_SEGMENT_CORRECT 0xA5C3

#define NVRAM_SYSTEM_INFO_SIZE 10
#define NVRAM_CRC_SIZE 2
#define NVRAM_USER_SIZE (NVRAM_SIZE - NVRAM_SYSTEM_INFO_SIZE)

#pragma pack(push, 1)

/// Overall layout of an NVRAM segment.
/// Consists of the device settings structure and
/// the structure with the segment system fields
typedef union
{
  struct
  {
    union
    {
      uint8_t buf[NVRAM_USER_SIZE];
    } app;
    struct
    {
      /// User settings version
      uint16_t nvram_settings_version;
      /// Identifier of the current segment
      uint32_t segment_id;
      /// Marker that the segment was saved correctly
      uint16_t segment_correct;
      /// Checksum of the NVRAM contents
      uint16_t nvram_crc;
    } sys;
  };
  uint8_t buf[NVRAM_SIZE];
} nvram_map_t;

#pragma pack(pop)
