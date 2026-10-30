#pragma once

/**
 * @file nvram_settings_template.h
 * @brief Structure with the user settings stored in NVRAM; it is project-specific.
 * @note The file must be renamed to "nvram_settings.h" and placed in the "application"
 *       part of the project.
 */

#include <stdint.h>

#include "utils/errors.h"

/// NVRAM settings version
#define NVRAM_SETTINGS_VERSION 1

#pragma pack(push, 1)

/// Device settings structure
/// New fields must be added at the end
/// When adding fields, increment the settings version NVRAM_SETTINGS_VERSION
typedef struct
{
  uint8_t example_field_1;
  struct
  {
    uint16_t example_field_2;
    uint16_t example_field_3;
  } example_struct_2;
} nvram_settings_t;

#pragma pack(pop)

error_t nvram_settings_init(void);
void nvram_settings_process(void);
void nvram_settings_set_default(void);

extern const nvram_settings_t nvram_settings_default;
