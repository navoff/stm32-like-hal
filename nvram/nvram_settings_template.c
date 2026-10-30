/**
 * @file nvram_settings_template.c
 * @brief Template of the project-specific NVRAM settings wrapper.
 * @note The file must be renamed to "nvram_settings.c" and placed in the "application"
 *       part of the project.
 */

#include "config.h"

#include <stdbool.h>

#include "nvram/nvram.h"
#include "utils/assert.h"

#include "nvram_settings.h"

const nvram_settings_t nvram_settings_default = {
    .example_field_1 = 0,
    .example_struct_2.example_field_2 = 1,
    .example_struct_2.example_field_3 = 2,
};

nvram_t nvram_settings;

error_t nvram_settings_init(void)
{
  hal_assert(sizeof(nvram_settings_t) <= NVRAM_USER_SIZE);

  nvram_init_struct_t init_struct = {0};
  error_t res = ERR_RESULT;

  // Initialize the area for storing settings
#if (FEATURE_ENABLE_INT_NVRAM)
  // address and segment count come from the linker script
  res = nvram_struct_init(&init_struct, NVRAM_DRIVER__INT_FLASH);
#elif (FEATURE_ENABLE_EXT_NOR_XXX5_NVRAM)
  // the external flash driver (ext_flash_xxx5) must be ready by this point
  res = nvram_struct_init(&init_struct, NVRAM_DRIVER__EXT_NOR_XXX5);
  init_struct.address_begin = 0;
  init_struct.segment_amount = 1;
#endif

  hal_assert(res == ERR_OK);
  if (res != ERR_OK)
  {
    return res;
  }

  init_struct.version = NVRAM_SETTINGS_VERSION;
  if (nvram_init(&nvram_settings, &init_struct) != ERR_OK)
  {
    nvram_settings_set_default();
  }

  return ERR_OK;
}

void nvram_settings_process(void)
{
  nvram_process(&nvram_settings);
}

void nvram_settings_set_default(void)
{
  nvram_write_default_struct(&nvram_settings, (uint8_t *)&nvram_settings_default,
                             sizeof(nvram_settings_t));
}
