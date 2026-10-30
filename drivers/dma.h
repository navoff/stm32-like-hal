#pragma once

/**
 * @file dma.h
 * @brief DMA controller tables and clock control for the supported MCU families.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(STM32F0XX) || defined(STM32F1XX) || defined(GD32L23X)
  /// Number of supported DMA controllers
  #define DMA_AMOUNT 1
#elif defined(GD32E11X) || defined(STM32F30X) || defined(STM32F4XX) || defined(STM32L4XX)
  /// Number of supported DMA controllers
  #define DMA_AMOUNT 2
#else
  #error "This MCU not supported"
#endif

/// Number of DMA channels
#define DMA_CHANNEL_AMOUNT 8

/// Undefined (invalid) DMA number value
#define DMA_NUMBER_UNDEF 0xFF

/// Undefined (invalid) DMA channel index value
#define DMA_CHANNEL_IDX_UNDEF 0xFF

#if defined(STM32F1XX) || defined(STM32F30X)

  /// Convert an SPL DMAx_IT_xxx define into a DMAx_FLAG_xxx define
  #define DMA_DRIVER_GET_FLAG_FROM_IT_FLAG(x) (x)

/// DMA channel description
typedef struct
{
  /// DMA channel
  DMA_Channel_TypeDef *channel;

  /// Interrupt line for this DMA channel
  IRQn_Type interrupt;

  /// Transfer complete flag
  uint32_t tcif;

  /// Half transfer flag
  uint32_t htif;

  /// Transfer error flag
  uint32_t teif;

} dma_channel_hw_item_t;

#elif defined(STM32L4XX) || defined(STM32F0XX)

  /// Convert an SPL DMAx_IT_xxx define into a DMAx_FLAG_xxx define
  #define DMA_DRIVER_GET_FLAG_FROM_IT_FLAG(x) (x)

/// DMA channel description
typedef struct
{
  /// DMA channel
  uint32_t ll_channel;

  /// Interrupt line for this DMA channel
  IRQn_Type interrupt;

  /// Transfer complete flag
  uint32_t tcif;

  /// Half transfer flag
  uint32_t htif;

  /// Transfer error flag
  uint32_t teif;

} dma_channel_hw_item_t;

#elif defined(STM32F4XX)

  /// Number of DMA streams
  #define DMA_STREAM_AMOUNT 8

typedef struct
{
  DMA_Stream_TypeDef *stream;
  uint32_t flag_tcif;
} dma_steam_hw_item_t;

#elif defined(GD32E11X) || defined(GD32L23X)

  #define DMA_CHANNEL_UNDEF 0xFF

/// DMA channel description
typedef struct
{
  /// DMA channel: DMA_CHx
  dma_channel_enum channel;
} dma_channel_hw_item_t;

typedef struct
{
  /// DMAx instance (x=0,1)
  uint32_t dma;

  rcu_periph_enum rcu_dma;
} dma_hw_item_t;
#else
  #error This MCU not supported
#endif

#if defined(GD32E11X) || defined(GD32L23X)
  #define check_dma_number(number) hal_assert(number < DMA_AMOUNT)
#else
  #define check_dma_number(number) hal_assert((number > 0) && (number <= DMA_AMOUNT))
#endif

void dma_rcc_cmd(uint8_t dma_number, bool new_state);

#if defined(STM32F1XX) || defined(STM32F30X) || defined(STM32L4XX) || defined(STM32F0XX)
// TODO replace this function with a "dma_hw_item_t dma_hw_items" array
// Add a "uint32_t RCC_AHBPeriph" field to that array,
// which would greatly simplify the "dma_rcc_cmd" function
DMA_TypeDef *dma_get_by_number(uint8_t dma_number);

extern const dma_channel_hw_item_t dma_channel_hw_items[][DMA_CHANNEL_AMOUNT];

#elif defined(STM32F4XX)
DMA_TypeDef *dma_get_by_number(uint8_t dma_number);

extern const dma_steam_hw_item_t dma_stream_hw_items[][DMA_STREAM_AMOUNT];
extern const uint32_t dma_channels[];

#elif defined(GD32E11X) || defined(GD32L23X)

extern const dma_channel_hw_item_t dma_channel_hw_items[][DMA_CHANNEL_AMOUNT];
extern const dma_hw_item_t dma_hw_items[];
#else
  #error This MCU not supported
#endif

#ifdef __cplusplus
}
#endif
