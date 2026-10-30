/**
 * @file dma.c
 * @brief DMA controller tables and clock control for the supported MCU families.
 */

#include "config.h"

#include <string.h>

#include "utils/assert.h"
#include "utils/macros.h"

#include "drivers/dma.h"

#if !defined(FEATURE_ENABLE_DMA0)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_DMA0 0
#endif

#if !defined(FEATURE_ENABLE_DMA1)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_DMA1 0
#endif

#if !defined(FEATURE_ENABLE_DMA2)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_DMA2 0
#endif

#if defined(STM32F1XX) || defined(STM32F30X)
const dma_channel_hw_item_t dma_channel_hw_items[DMA_AMOUNT + 1][DMA_CHANNEL_AMOUNT] = {
    // fake dma0
    {
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
    },

  #if (DMA_AMOUNT >= 1)
    #if (FEATURE_ENABLE_DMA1)
    {
        {DMA1_Channel1, DMA1_Channel1_IRQn, DMA1_IT_TC1, DMA1_IT_HT1, DMA1_IT_TE1},
        {DMA1_Channel2, DMA1_Channel2_IRQn, DMA1_IT_TC2, DMA1_IT_HT2, DMA1_IT_TE2},
        {DMA1_Channel3, DMA1_Channel3_IRQn, DMA1_IT_TC3, DMA1_IT_HT3, DMA1_IT_TE3},
        {DMA1_Channel4, DMA1_Channel4_IRQn, DMA1_IT_TC4, DMA1_IT_HT4, DMA1_IT_TE4},
        {DMA1_Channel5, DMA1_Channel5_IRQn, DMA1_IT_TC5, DMA1_IT_HT5, DMA1_IT_TE5},
        {DMA1_Channel6, DMA1_Channel6_IRQn, DMA1_IT_TC6, DMA1_IT_HT6, DMA1_IT_TE6},
        {DMA1_Channel7, DMA1_Channel7_IRQn, DMA1_IT_TC7, DMA1_IT_HT7, DMA1_IT_TE7},
    },
    #else
    {
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
    },
    #endif // #if (FEATURE_ENABLE_DMA1)
  #endif

  #if (DMA_AMOUNT >= 2)
    #if (FEATURE_ENABLE_DMA2)
    {
        {DMA2_Channel1, DMA2_Channel1_IRQn, DMA2_IT_TC1, DMA2_IT_HT1, DMA1_IT_TE1},
        {DMA2_Channel2, DMA2_Channel2_IRQn, DMA2_IT_TC2, DMA2_IT_HT2, DMA2_IT_TE2},
        {DMA2_Channel3, DMA2_Channel3_IRQn, DMA2_IT_TC3, DMA2_IT_HT3, DMA2_IT_TE3},
        {DMA2_Channel4, DMA2_Channel4_5_IRQn, DMA2_IT_TC4, DMA2_IT_HT4, DMA2_IT_TE4},
        {DMA2_Channel5, DMA2_Channel4_5_IRQn, DMA2_IT_TC5, DMA2_IT_HT5, DMA2_IT_TE5},
        {.channel = 0},
        {.channel = 0},
    },
    #else
    {
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
        {.channel = 0},
    },
    #endif // #if (FEATURE_ENABLE_DMA2)
  #endif
};

#elif defined(STM32L4XX)
const dma_channel_hw_item_t dma_channel_hw_items[DMA_AMOUNT + 1][DMA_CHANNEL_AMOUNT] = {
    // fake dma0
    {
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
    },

  #if (DMA_AMOUNT >= 1)
    #if (FEATURE_ENABLE_DMA1)
    {
        {LL_DMA_CHANNEL_1, DMA1_Channel1_IRQn, DMA_ISR_TCIF1, DMA_ISR_HTIF1, DMA_ISR_TEIF1},
        {LL_DMA_CHANNEL_2, DMA1_Channel2_IRQn, DMA_ISR_TCIF2, DMA_ISR_HTIF2, DMA_ISR_TEIF2},
        {LL_DMA_CHANNEL_3, DMA1_Channel3_IRQn, DMA_ISR_TCIF3, DMA_ISR_HTIF3, DMA_ISR_TEIF3},
        {LL_DMA_CHANNEL_4, DMA1_Channel4_IRQn, DMA_ISR_TCIF4, DMA_ISR_HTIF4, DMA_ISR_TEIF4},
        {LL_DMA_CHANNEL_5, DMA1_Channel5_IRQn, DMA_ISR_TCIF5, DMA_ISR_HTIF5, DMA_ISR_TEIF5},
        {LL_DMA_CHANNEL_6, DMA1_Channel6_IRQn, DMA_ISR_TCIF6, DMA_ISR_HTIF6, DMA_ISR_TEIF6},
        {LL_DMA_CHANNEL_7, DMA1_Channel7_IRQn, DMA_ISR_TCIF7, DMA_ISR_HTIF7, DMA_ISR_TEIF7},
    },
    #else
    {
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
    },
    #endif // #if (FEATURE_ENABLE_DMA1)
  #endif

  #if (DMA_AMOUNT >= 2)
    #if (FEATURE_ENABLE_DMA2)
    {
        {LL_DMA_CHANNEL_1, DMA2_Channel1_IRQn, DMA_ISR_TCIF1, DMA_ISR_HTIF1, DMA_ISR_TEIF1},
        {LL_DMA_CHANNEL_2, DMA2_Channel2_IRQn, DMA_ISR_TCIF2, DMA_ISR_HTIF2, DMA_ISR_TEIF2},
        {LL_DMA_CHANNEL_3, DMA2_Channel3_IRQn, DMA_ISR_TCIF3, DMA_ISR_HTIF3, DMA_ISR_TEIF3},
        {LL_DMA_CHANNEL_4, DMA2_Channel4_IRQn, DMA_ISR_TCIF4, DMA_ISR_HTIF4, DMA_ISR_TEIF4},
        {LL_DMA_CHANNEL_5, DMA2_Channel5_IRQn, DMA_ISR_TCIF5, DMA_ISR_HTIF5, DMA_ISR_TEIF5},
        {LL_DMA_CHANNEL_6, DMA2_Channel6_IRQn, DMA_ISR_TCIF6, DMA_ISR_HTIF6, DMA_ISR_TEIF6},
        {LL_DMA_CHANNEL_7, DMA2_Channel7_IRQn, DMA_ISR_TCIF7, DMA_ISR_HTIF7, DMA_ISR_TEIF7},
    },
    #else
    {
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
    },
    #endif // #if (FEATURE_ENABLE_DMA2)
  #endif
};

#elif defined(STM32F0XX)
const dma_channel_hw_item_t dma_channel_hw_items[DMA_AMOUNT + 1][DMA_CHANNEL_AMOUNT] = {
    // fake dma0
    {
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
    },

  #if (FEATURE_ENABLE_DMA1)
    {
        {LL_DMA_CHANNEL_1, DMA1_Channel1_IRQn, DMA_ISR_TCIF1, DMA_ISR_HTIF1, DMA_ISR_TEIF1},
        {LL_DMA_CHANNEL_2, DMA1_Channel2_3_IRQn, DMA_ISR_TCIF2, DMA_ISR_HTIF2, DMA_ISR_TEIF2},
        {LL_DMA_CHANNEL_3, DMA1_Channel2_3_IRQn, DMA_ISR_TCIF3, DMA_ISR_HTIF3, DMA_ISR_TEIF3},
        {LL_DMA_CHANNEL_4, DMA1_Channel4_5_6_7_IRQn, DMA_ISR_TCIF4, DMA_ISR_HTIF4, DMA_ISR_TEIF4},
        {LL_DMA_CHANNEL_5, DMA1_Channel4_5_6_7_IRQn, DMA_ISR_TCIF5, DMA_ISR_HTIF5, DMA_ISR_TEIF5},
        {LL_DMA_CHANNEL_6, DMA1_Channel4_5_6_7_IRQn, DMA_ISR_TCIF6, DMA_ISR_HTIF6, DMA_ISR_TEIF6},
        {LL_DMA_CHANNEL_7, DMA1_Channel4_5_6_7_IRQn, DMA_ISR_TCIF7, DMA_ISR_HTIF7, DMA_ISR_TEIF7},
    },
  #else
    {
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
        {.ll_channel = 0},
    },
  #endif // #if (FEATURE_ENABLE_DMA1)

  #if (FEATURE_ENABLE_DMA2)
    #error This MCU does not support DMA2
  #endif // #if (FEATURE_ENABLE_DMA2)
};

#elif defined(STM32F4XX)
const dma_steam_hw_item_t dma_stream_hw_items[DMA_AMOUNT + 1][DMA_STREAM_AMOUNT] = {
    // fake dma0
    {
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
    },

  #if (FEATURE_ENABLE_DMA1)
    {
        {DMA1_Stream0, DMA_FLAG_TCIF0},
        {DMA1_Stream1, DMA_FLAG_TCIF1},
        {DMA1_Stream2, DMA_FLAG_TCIF2},
        {DMA1_Stream3, DMA_FLAG_TCIF3},
        {DMA1_Stream4, DMA_FLAG_TCIF4},
        {DMA1_Stream5, DMA_FLAG_TCIF5},
        {DMA1_Stream6, DMA_FLAG_TCIF6},
        {DMA1_Stream7, DMA_FLAG_TCIF7},
    },
  #else
    {
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
    },

  #endif
  #if (FEATURE_ENABLE_DMA2)
    {
        {DMA2_Stream0, DMA_FLAG_TCIF0},
        {DMA2_Stream1, DMA_FLAG_TCIF1},
        {DMA2_Stream2, DMA_FLAG_TCIF2},
        {DMA2_Stream3, DMA_FLAG_TCIF3},
        {DMA2_Stream4, DMA_FLAG_TCIF4},
        {DMA2_Stream5, DMA_FLAG_TCIF5},
        {DMA2_Stream6, DMA_FLAG_TCIF6},
        {DMA2_Stream7, DMA_FLAG_TCIF7},
    }
  #else
    {
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
        {.stream = 0},
    }
  #endif
};

uint32_t const dma_channels[DMA_CHANNEL_AMOUNT] = {
    DMA_Channel_0, DMA_Channel_1, DMA_Channel_2, DMA_Channel_3,
    DMA_Channel_4, DMA_Channel_5, DMA_Channel_6, DMA_Channel_7,
};
#elif defined(GD32E11X)
const dma_channel_hw_item_t dma_channel_hw_items[DMA_AMOUNT][DMA_CHANNEL_AMOUNT] = {
  #if (FEATURE_ENABLE_DMA0)
    {
        {DMA_CH0},
        {DMA_CH1},
        {DMA_CH2},
        {DMA_CH3},
        {DMA_CH4},
        {DMA_CH5},
        {DMA_CH6},
        {DMA_CHANNEL_UNDEF},
    },
  #else
    {
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
    },
  #endif // #if (FEATURE_ENABLE_DMA0)

  #if (FEATURE_ENABLE_DMA1)
    {
        {DMA_CH0},
        {DMA_CH1},
        {DMA_CH2},
        {DMA_CH3},
        {DMA_CH4},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
    },
  #else
    {
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
    },
  #endif // #if (FEATURE_ENABLE_DMA1)
};

const dma_hw_item_t dma_hw_items[] = {
  #if (FEATURE_ENABLE_DMA0)
    {.dma = DMA0, .rcu_dma = RCU_DMA0},
  #else
    {.dma = 0},
  #endif
  #if (FEATURE_ENABLE_DMA1)
    {.dma = DMA1, .rcu_dma = RCU_DMA1},
  #else
    {.dma = 0},
  #endif
};
#elif defined(GD32L23X)
const dma_channel_hw_item_t dma_channel_hw_items[DMA_AMOUNT][DMA_CHANNEL_AMOUNT] = {
  #if (FEATURE_ENABLE_DMA0)
    {
        {DMA_CH0},
        {DMA_CH1},
        {DMA_CH2},
        {DMA_CH3},
        {DMA_CH4},
        {DMA_CH5},
        {DMA_CH6},
        {DMA_CHANNEL_UNDEF},
    },
  #else
    {
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
        {DMA_CHANNEL_UNDEF},
    },
  #endif // #if (FEATURE_ENABLE_DMA0)
};
const dma_hw_item_t dma_hw_items[] = {
  #if (FEATURE_ENABLE_DMA0)
    {.dma = DMA, .rcu_dma = RCU_DMA},
  #else
    {.dma = 0},
  #endif
};
#else
  #error This MCU not supported
#endif

void dma_rcc_cmd(uint8_t dma_number, bool new_state)
{
  check_dma_number(dma_number);

#if (defined(STM32F1XX) || defined(STM32F30X))

  switch (dma_number)
  {
  #if (FEATURE_ENABLE_DMA1)
    case 1:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_DMA2)
    case 2:
      RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, new_state);
      break;
  #endif
  }

#elif (defined(STM32L4XX))
  if (new_state)
  {
    switch (dma_number)
    {
  #if (FEATURE_ENABLE_DMA1)
      case 1:
        __HAL_RCC_DMA1_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_DMA2)
      case 2:
        __HAL_RCC_DMA2_CLK_ENABLE();
        break;
  #endif
    }
  }
  else
  {
    switch (dma_number)
    {
  #if (FEATURE_ENABLE_DMA1)
      case 1:
        __HAL_RCC_DMA1_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_DMA2)
      case 2:
        __HAL_RCC_DMA2_CLK_DISABLE();
        break;
  #endif
    }
  }

#elif (defined(STM32F4XX))
  switch (dma_number)
  {
  #if (FEATURE_ENABLE_DMA1)
    case 1:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_DMA2)
    case 2:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, new_state);
      break;
  #endif
  }

#elif (defined(STM32F0XX))
  HAL_UNUSED(dma_number);
  #if (FEATURE_ENABLE_DMA1)
  if (new_state)
  {
    __HAL_RCC_DMA1_CLK_ENABLE();
  }
  else
  {
    __HAL_RCC_DMA1_CLK_DISABLE();
  }
  #endif

#elif (defined(STM32F4XX))
  switch (dma_number)
  {
  #if (FEATURE_ENABLE_DMA1)
    case 1:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_DMA2)
    case 2:
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, new_state);
      break;
  #endif
  }
#elif (defined(GD32E11X) || defined(GD32L23X))
  if (new_state)
  {
    rcu_periph_clock_enable(dma_hw_items[dma_number].rcu_dma);
  }
  else
  {
    rcu_periph_clock_disable(dma_hw_items[dma_number].rcu_dma);
  }
#else
  #error This MCU not supported
#endif
}

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32L4XX) || defined(STM32F0XX) ||        \
    defined(STM32F4XX)
DMA_TypeDef *dma_get_by_number(uint8_t dma_number)
{
  #if (FEATURE_ENABLE_DMA1)
  if (dma_number == 1)
  {
    return DMA1;
  }
  #endif
  #if (FEATURE_ENABLE_DMA2) && (!defined(STM32F0XX))
  if (dma_number == 2)
  {
    return DMA2;
  }
  #endif

  return NULL;
}
#endif
