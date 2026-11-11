/**
 * @file iwdg.c
 * @brief Independent watchdog (IWDG) driver.
 */

#include "iwdg.h"
#include "config.h"

#if !defined(IWDG_LSI_FREQ_MEASURE_EN)
  /// Whether LSI frequency measurement will be performed using TIM5
  #define IWDG_LSI_FREQ_MEASURE_EN 0
#endif

#if !defined(IWDG_TIMEOUT_MAX)
  /// Maximum IWDG timeout, ms
  #define IWDG_TIMEOUT_MAX 10000
#endif

/// Nominal LSI frequency, Hz
/// @note Actual frequency may differ
#if defined(GD32E11X) || defined(STM32F1XX) || defined(STM32F30X)
  #define IWDG_LSI_FREQ 40000
#else
  #define IWDG_LSI_FREQ 32000
#endif

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX) || defined(STM32L4XX) ||        \
    defined(STM32F0XX) || defined(GD32L23X) || defined(GD32E11X)
  /// Largest prescaler code. Code n divides the LSI clock by (4 << n), from 4 to 256; the
  /// IWDG_Prescaler_x, LL_IWDG_PRESCALER_x and FWDGT_PSC_DIVx constants are these codes
  #define IWDG_PRESCALER_CODE_MAX 6

  /// Default Reload counter value
  #define IWDG_DEFAULT_RLR 0x0FFF

  /// Largest reload value written by the driver: the counter is 12-bit, and 0xFFF is left out
  /// for iwdg_is_enabled()
  #define IWDG_RELOAD_MAX (IWDG_DEFAULT_RLR - 1)
#else
  #error This MCU not supported
#endif

void iwdg_lsi_enable(void);
uint32_t iwdg_get_lsi_freq(void);

struct
{
  /// Timer value at the start of LSI frequency determination
  uint32_t timer_start_value;

  /// Timer value at the end of LSI frequency determination
  uint32_t timer_stop_value;

  /// Timer capture flag at the start of LSI frequency determination
  bool is_timer_start_ready;

  /// Timer capture flag at the end of LSI frequency determination
  bool is_timer_stop_ready;
} iwdg_lsi_freq;

void iwdg_stop_in_halted_mode(void)
{
#if defined(STM32F1XX)

  DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP;

#elif defined(STM32F0XX) || defined(STM32F30X) || defined(STM32F4XX)

  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;

#elif defined(STM32L4XX)

  DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;

#elif defined(GD32E11X) || defined(GD32L23X)

#else
  #error This MCU not supported
#endif
}

void iwdg_init(uint32_t timeout)
{
  if (timeout >= IWDG_TIMEOUT_MAX)
  {
    timeout = IWDG_TIMEOUT_MAX;
  }

  iwdg_lsi_enable();

  // LSI ticks in the timeout; fits in 32 bits for IWDG_TIMEOUT_MAX
  uint32_t ticks = timeout * iwdg_get_lsi_freq() / 1000;

  // The smallest prescaler whose reload value fits in the 12-bit counter gives the finest step
  uint8_t prescaler_code = 0;
  while (((ticks >> 2 >> prescaler_code) > IWDG_RELOAD_MAX) &&
         (prescaler_code < IWDG_PRESCALER_CODE_MAX))
  {
    prescaler_code++;
  }

  uint32_t reload_value = ticks >> 2 >> prescaler_code;
  // Never write 0xFFF: it is the reset value, and its change shows that IWDG was enabled
  if (reload_value > IWDG_RELOAD_MAX)
  {
    reload_value = IWDG_RELOAD_MAX;
  }

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(prescaler_code);
  IWDG_SetReload((uint16_t)reload_value);
  IWDG_ReloadCounter();
  IWDG_Enable();

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_IWDG_EnableWriteAccess(IWDG);
  LL_IWDG_SetPrescaler(IWDG, prescaler_code);

  LL_IWDG_SetReloadCounter(IWDG, reload_value);

  LL_IWDG_ReloadCounter(IWDG);
  LL_IWDG_Enable(IWDG);

#elif defined(GD32E11X) || defined(GD32L23X)
  fwdgt_config((uint16_t)reload_value, prescaler_code);
  fwdgt_enable();

#else
  #error This MCU not supported
#endif
}

void iwdg_update(void)
{
#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  IWDG_ReloadCounter();

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_IWDG_ReloadCounter(IWDG);

#elif defined(GD32E11X) || defined(GD32L23X)
  fwdgt_counter_reload();

#else
  #error This MCU not supported
#endif
}

bool iwdg_is_enabled(void)
{
#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX) || defined(STM32L4XX) ||        \
    defined(STM32F0XX)
  return (IWDG->RLR != IWDG_DEFAULT_RLR);
#elif defined(GD32E11X) || defined(GD32L23X)
  return (FWDGT_RLD != IWDG_DEFAULT_RLR);
#else
  #error This MCU not supported
#endif
}

/**
 * @brief Enable LSI and wait for successful activation
 */
void iwdg_lsi_enable(void)
{
#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32F4XX)
  RCC_LSICmd(ENABLE);
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {
  }

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_RCC_LSI_Enable();
  while (LL_RCC_LSI_IsReady() == 0)
  {
  }

#elif defined(GD32L23X)
  rcu_osci_on(RCU_IRC32K);
  while (SUCCESS != rcu_osci_stab_wait(RCU_IRC32K))
  {
  }

#elif defined(GD32E11X)
#else
  #error This MCU not supported
#endif
}

/**
 * @brief Measure LSI frequency using TIM5 timer
 * @return LSI frequency
 */
uint32_t iwdg_get_lsi_freq(void)
{
#if (IWDG_LSI_FREQ_MEASURE_EN)
  #if defined(STM32F4XX)
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
  TIM_RemapConfig(TIM5, TIM5_LSI);
  TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);

  TIM_ICInitTypeDef TIM_ICInitStructure;
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM5, &TIM_ICInitStructure);

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  TIM_Cmd(TIM5, ENABLE);

  // reset flags
  TIM5->SR = 0;

  iwdg_lsi_freq.is_timer_start_ready = false;
  iwdg_lsi_freq.is_timer_stop_ready = false;

  TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);

  while (iwdg_lsi_freq.is_timer_stop_ready == false)
  {
  }

  TIM_DeInit(TIM5);

  uint32_t period = iwdg_lsi_freq.timer_stop_value - iwdg_lsi_freq.timer_start_value;

  RCC_ClocksTypeDef RCC_ClockFreq;
  RCC_GetClocksFreq(&RCC_ClockFreq);

  uint32_t res = RCC_ClockFreq.PCLK1_Frequency / period;
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0)
  {
    res *= 2;
  }

  return res;

  #else // #if defined(STM32F4XX)
    #error This MCU not supported
  #endif

#else // #if (IWDG_LSI_FREQ_MEASURE_EN)

  return IWDG_LSI_FREQ;
#endif
}

#if (IWDG_LSI_FREQ_MEASURE_EN)
  #if defined(STM32F4XX)
void TIM5_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM5, TIM_IT_CC4) != RESET)
  {
    if (iwdg_lsi_freq.is_timer_start_ready == false)
    {
      iwdg_lsi_freq.timer_start_value = TIM_GetCapture4(TIM5);
      iwdg_lsi_freq.is_timer_start_ready = true;
    }
    else if (iwdg_lsi_freq.is_timer_stop_ready == false)
    {
      iwdg_lsi_freq.timer_stop_value = TIM_GetCapture4(TIM5);
      iwdg_lsi_freq.is_timer_stop_ready = true;
    }

    TIM_ClearITPendingBit(TIM5, TIM_IT_CC4);
  }
}
  #else
    #error This MCU not supported
  #endif
#endif
