#pragma once

/**
 * @file mcu.h
 * @brief MCU reset, vector table relocation and reset cause status.
 */

#include <stdint.h>

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  #define MCU_WAS_IWDG_RESET() RCC_GetFlagStatus(RCC_FLAG_IWDGRST)

#elif defined(STM32L4XX) || defined(STM32F0XX)
  #define MCU_WAS_IWDG_RESET() LL_RCC_IsActiveFlag_IWDGRST()
#else
  #error This MCU not supported
#endif

#pragma pack(push, 1)
typedef union
{
  struct
  {
    /// Reset pin was driven low
    uint16_t nrst_pin_reset : 1;

    /// Reset caused by WWDG
    uint16_t wwdg_reset : 1;

    /// Reset caused by IWDG
    uint16_t iwdg_reset : 1;

    /// Software reset
    uint16_t sw_reset : 1;

    /// Reset caused by an illegal entry into one of the low-power modes
    uint16_t low_power_reset : 1;

    /// Power-on/power-down reset
    uint16_t por_pdr_reset : 1;

    /// Brown-out reset
    uint16_t bod_reset : 1;
  };

  uint16_t msk;
} mcu_reset_status_t;
#pragma pack(pop)

/**
 * Software reset of the microcontroller
 */
void mcu_reset(void);
void mcu_set_vector_table_in_flash(uint32_t address);
void mcu_get_and_clear_reset_status(mcu_reset_status_t *reset_status);
