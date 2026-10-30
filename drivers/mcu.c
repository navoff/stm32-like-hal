/**
 * @file mcu.c
 * @brief MCU reset, vector table relocation and reset cause status.
 */

#include "config.h"

#include "mcu.h"
#include "utils/assert.h"
#include "utils/macros.h"

void mcu_reset(void)
{
  NVIC_SystemReset();
}

/**
 * @brief Relocate the interrupt vector table within FLASH
 * @param address
 */
void mcu_set_vector_table_in_flash(uint32_t address)
{
#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, address);

#elif defined(STM32L4XX)
  SCB->VTOR = FLASH_BASE | (address & (uint32_t)0x1FFFFF80);

#elif defined(STM32F0XX)
  // Cortex-M0 doesnot supported vector table offset :(
  HAL_UNUSED(address);
  hal_assert(0);

#else
  #error This MCU not supported
#endif
}

/**
 * @brief Get and clear the last reset status
 * @param reset_status - pointer to the structure with the last reset flags
 */
void mcu_get_and_clear_reset_status(mcu_reset_status_t *reset_status)
{
  reset_status->msk = 0;

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
  {
    reset_status->nrst_pin_reset = 1;
  }

  if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET)
  {
    reset_status->wwdg_reset = 1;
  }

  if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
  {
    reset_status->iwdg_reset = 1;
  }

  if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET)
  {
    reset_status->sw_reset = 1;
  }

  if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) != RESET)
  {
    reset_status->low_power_reset = 1;
  }

  if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
  {
    reset_status->por_pdr_reset = 1;
  }

  #if defined(STM32F4XX)
  if ((RCC_GetFlagStatus(RCC_FLAG_BORRST) != RESET))
  {
    reset_status->bod_reset = 1;
  }
  #endif

  // Clear the reset cause flags
  RCC_ClearFlag();

#elif defined(STM32L4XX)

  reset_status->nrst_pin_reset = LL_RCC_IsActiveFlag_PINRST();
  reset_status->wwdg_reset = LL_RCC_IsActiveFlag_WWDGRST();
  reset_status->iwdg_reset = LL_RCC_IsActiveFlag_IWDGRST();
  reset_status->sw_reset = LL_RCC_IsActiveFlag_SFTRST();
  reset_status->low_power_reset = LL_RCC_IsActiveFlag_LPWRRST();
  reset_status->por_pdr_reset = 0;
  reset_status->bod_reset = LL_RCC_IsActiveFlag_BORRST();

#elif defined(STM32F0XX)

  reset_status->nrst_pin_reset = LL_RCC_IsActiveFlag_PINRST();
  reset_status->wwdg_reset = LL_RCC_IsActiveFlag_WWDGRST();
  reset_status->iwdg_reset = LL_RCC_IsActiveFlag_IWDGRST();
  reset_status->sw_reset = LL_RCC_IsActiveFlag_SFTRST();
  reset_status->low_power_reset = LL_RCC_IsActiveFlag_LPWRRST();
  reset_status->por_pdr_reset = LL_RCC_IsActiveFlag_PORRST();
  reset_status->bod_reset = 0;

  // Clear the reset cause flags
  LL_RCC_ClearResetFlags();

#else
  #error This MCU not supported
#endif
}
