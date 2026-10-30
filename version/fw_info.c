/**
 * @file fw_info.c
 * @brief Holds the information about the project type and firmware intended for the bootloader.
 *        The file must not contain any variables other than fw_info.
 *        The linker script must place the .fw_info section at the end of the application
 *        flash area (see docs/arb_protocol.md); fw_crc is stamped after the build by
 *        version/add_crc.py. The object lives in a static library, so the application must
 *        force it in with -Wl,--undefined=fw_info.
 */

#include "config.h"

#include "fw_info.h"
#include "utils/assert.h"
#include <stddef.h>

#if !defined(FW_INFO_STRUCT_VERSION)
  /// Information structure version
  #define FW_INFO_STRUCT_VERSION 1
#endif

#if defined(DEBUG)
  #define FW_VERSION_SUFFIX ".debug"
#else
  #define FW_VERSION_SUFFIX ""
#endif

#define FW_VERSION_FULL FW_VERSION FW_VERSION_SUFFIX

FW_INFO_SECTION_PREFIX const fw_info_t fw_info FW_INFO_SECTION_SUFFIX = {
    .magic_number = FW_INFO_MAGIC_NUMBER,
    .version = FW_INFO_STRUCT_VERSION,
    .platform_type = PLATFORM_TYPE,
    .hw_type = HW_TYPE,
    .fw_version = FW_VERSION_FULL,
    .reserved = {0},
    .fw_crc = 0xFFFFFFFF,
};

HAL_STATIC_ASSERT(offsetof(fw_info_t, fw_crc) == (FW_INFO_STRUCT_SIZE - sizeof(fw_info.fw_crc)),
                  "Field with crc must by last in the structure");
HAL_STATIC_ASSERT(sizeof(fw_info) == FW_INFO_STRUCT_SIZE, "Struct size doesn't matched");
