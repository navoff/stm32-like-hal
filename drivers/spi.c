/**
 * @file spi.c
 * @brief SPI master driver with per-device settings and optional DMA transfers.
 *
 * Usage:
 *   1. Fill in the spi_core_init_struct_t structure with the SPI settings.
 *   2. Call spi_core_init() to create an instance of spi_core_t, which is needed
 *      to work with every slave device connected to this SPI.
 *   3. Create an instance of spi_device_t for each slave device.
 *      Each instance must be initialized with spi_device_init().
 *   4. To exchange data with each slave device use the functions
 *      spi_device_tx_rx_byte()
 *      spi_device_rx_array()
 *      spi_device_tx_array()
 *      spi_device_tx_rx_register()
 *      spi_device_tx_rx_array_registers()
 *   5. In general, settings of different slave devices connected to the same SPI may
 *      differ. Therefore, before each transfer the settings of the spi_core_t
 *      instance are automatically checked against the settings of the particular
 *      spi_device_t instance and the SPI is reinitialized if needed.
 */

#include "config.h"

#include <string.h>

#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "drivers/tick_timer.h"
#include "utils/assert.h"
#include "utils/errors.h"
#include "utils/macros.h"

#include "drivers/spi.h"

#if !defined(FEATURE_ENABLE_SPI1)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_SPI1 0
#endif

#if !defined(FEATURE_ENABLE_SPI2)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_SPI2 0
#endif

#if !defined(FEATURE_ENABLE_SPI3)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_SPI3 0
#endif

#if !defined(FEATURE_ENABLE_SPI4)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_SPI4 0
#endif

#define SPI_AMOUNT 4

#if defined(STM32F1XX) || defined(STM32F4XX) || defined(STM32L4XX) || defined(STM32F0XX)
  /// Shift to convert the clock division factor into the prescaler value
  /// see the SPIx_CR1 register
  #define SPI_PRESCALER_OFFSET 3
#else
  #error This MCU not supported
#endif

typedef struct
{
  SPI_TypeDef *spi;
  gpio_pin_mode_t gpio_pin_mode;
} spi_hw_item_t;

void spi_rcc_cmd(uint8_t spi_number, bool new_state);
void spi_core_gpio_init(spi_core_init_struct_t *init_struct);
void spi_core_dma_init(spi_core_t *spi_core, spi_core_init_struct_t *init_struct);
static inline bool spi_device_is_need_apply_new_settings(spi_device_t *spi_device);
void spi_core_apply_settings(spi_core_t *spi_core, spi_device_settings_t *spi_device_settings);
uint16_t spi_driver_get_prescaler(SPI_TypeDef *spi, uint32_t desired_frequency);

const spi_hw_item_t spi_hw_items[SPI_AMOUNT] = {
#if (FEATURE_ENABLE_SPI1)
    {.spi = SPI1, .gpio_pin_mode = GPIO_MODE_SPI1},
#else
    {.spi = NULL},
#endif
#if (FEATURE_ENABLE_SPI2)
    {.spi = SPI2, .gpio_pin_mode = GPIO_MODE_SPI2},
#else
    {.spi = NULL},
#endif
#if (FEATURE_ENABLE_SPI3)
    {.spi = SPI3, .gpio_pin_mode = GPIO_MODE_SPI3},
#else
    {.spi = NULL},
#endif
#if (FEATURE_ENABLE_SPI4)
    {.spi = SPI4, .gpio_pin_mode = GPIO_MODE_SPI4},
#else
    {.spi = NULL},
#endif
};

/**
 * @brief Initialize the SPI driver
 * @param init_struct - SPI settings structure
 * @param spi_core - SPI driver instance
 */
void spi_core_init(spi_core_t *spi_core, spi_core_init_struct_t *init_struct)
{
  hal_assert((init_struct != NULL) && (spi_core != NULL));
  hal_assert((init_struct->spi_number > 0) && (init_struct->spi_number <= SPI_AMOUNT));
  hal_assert(init_struct->tx_mode < SPI_LINE_MODE__COUNT);
  hal_assert(init_struct->rx_mode < SPI_LINE_MODE__COUNT);

  spi_core->spi = spi_hw_items[init_struct->spi_number - 1].spi;

  if (spi_core->spi == NULL)
  {
    hal_assert(false);
    return;
  }

#if defined(STM32F1XX) || defined(STM32F4XX)
  SPI_I2S_DeInit(spi_core->spi);
#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_SPI_DeInit(spi_core->spi);
#else
  #error This MCU not supported
#endif

  spi_rcc_cmd(init_struct->spi_number, true);

  spi_core_gpio_init(init_struct);

#if defined(STM32F1XX) || defined(STM32F4XX)
  SPI_NSSInternalSoftwareConfig(spi_core->spi, SPI_NSSInternalSoft_Set);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_SPI_SetNSSMode(spi_core->spi, LL_SPI_NSS_SOFT);

#else
  #error This MCU not supported
#endif

  spi_core->tx_mode = init_struct->tx_mode;
  spi_core->rx_mode = init_struct->rx_mode;

  if ((spi_core->tx_mode == SPI_LINE_MODE__DMA_POLLING) ||
      (spi_core->rx_mode == SPI_LINE_MODE__DMA_POLLING))
  {
    spi_core_dma_init(spi_core, init_struct);
  }

  spi_core->is_busy = false;
  spi_core->is_error_detected = false;
  spi_core->current_device_settings = NULL;
}

#if defined(STM32F4XX)
void spi_core_dma_init(spi_core_t *spi_core, spi_core_init_struct_t *init_struct)
{
  hal_assert((init_struct->dma.number > 0) && (init_struct->dma.number <= DMA_AMOUNT));

  dma_rcc_cmd(init_struct->dma.number, true);

  DMA_InitTypeDef dma_init_struct;
  dma_init_struct.DMA_BufferSize = 0;
  dma_init_struct.DMA_FIFOMode = DMA_FIFOMode_Disable;
  dma_init_struct.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  dma_init_struct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  dma_init_struct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma_init_struct.DMA_Mode = DMA_Mode_Normal;
  dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t)(&(spi_core->spi->DR));
  dma_init_struct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  dma_init_struct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  dma_init_struct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma_init_struct.DMA_Priority = DMA_Priority_High;

  if (init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    hal_assert(init_struct->dma.tx_channel_idx < DMA_CHANNEL_AMOUNT);
    hal_assert(init_struct->dma.tx_stream_idx < DMA_STREAM_AMOUNT);

    spi_core->dma.tx_stream =
        dma_stream_hw_items[init_struct->dma.number][init_struct->dma.tx_stream_idx].stream;
    spi_core->dma.tx_flag_tcif =
        dma_stream_hw_items[init_struct->dma.number][init_struct->dma.tx_stream_idx].flag_tcif;
    dma_init_struct.DMA_Channel = dma_channels[init_struct->dma.tx_channel_idx];
    dma_init_struct.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    dma_init_struct.DMA_Memory0BaseAddr = (uint32_t)NULL;
    DMA_Init(spi_core->dma.tx_stream, &dma_init_struct);
  }

  if (init_struct->rx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    // DMA receive works only if DMA transmit mode is enabled
    hal_assert(init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING);
    hal_assert(init_struct->dma.rx_channel_idx < DMA_CHANNEL_AMOUNT);
    hal_assert(init_struct->dma.rx_stream_idx < DMA_STREAM_AMOUNT);

    spi_core->dma.rx_stream =
        dma_stream_hw_items[init_struct->dma.number][init_struct->dma.rx_stream_idx].stream;
    spi_core->dma.rx_flag_tcif =
        dma_stream_hw_items[init_struct->dma.number][init_struct->dma.rx_stream_idx].flag_tcif;
    dma_init_struct.DMA_Channel = dma_channels[init_struct->dma.rx_channel_idx];
    dma_init_struct.DMA_DIR = DMA_DIR_PeripheralToMemory;
    dma_init_struct.DMA_Memory0BaseAddr = (uint32_t)NULL;
    DMA_Init(spi_core->dma.rx_stream, &dma_init_struct);
  }
}

#elif defined(STM32F1XX)
void spi_core_dma_init(spi_core_t *spi_core, spi_core_init_struct_t *init_struct)
{
  hal_assert((init_struct->dma.number > 0) && (init_struct->dma.number <= DMA_AMOUNT));

  dma_rcc_cmd(init_struct->dma.number, true);

  DMA_InitTypeDef dma_init_struct;
  dma_init_struct.DMA_BufferSize = 0;
  dma_init_struct.DMA_MemoryBaseAddr = (uint32_t)NULL;
  dma_init_struct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma_init_struct.DMA_Mode = DMA_Mode_Normal;
  dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t)(&(spi_core->spi->DR));
  dma_init_struct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  dma_init_struct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma_init_struct.DMA_M2M = DMA_M2M_Disable;
  dma_init_struct.DMA_Priority = DMA_Priority_High;

  uint8_t dma_number = init_struct->dma.number;
  if (init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    hal_assert(init_struct->dma.tx_channel_idx < DMA_CHANNEL_AMOUNT);

    spi_core->dma.tx_channel =
        dma_channel_hw_items[dma_number][init_struct->dma.tx_channel_idx].channel;
    spi_core->dma.tx_flag_tcif =
        dma_channel_hw_items[dma_number][init_struct->dma.tx_channel_idx].tcif;
    hal_assert(spi_core->dma.tx_channel != NULL);

    dma_init_struct.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_DeInit(spi_core->dma.tx_channel);
    DMA_Init(spi_core->dma.tx_channel, &dma_init_struct);
  }

  if (init_struct->rx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    // DMA receive works only if DMA transmit mode is enabled
    hal_assert(init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING);
    hal_assert(init_struct->dma.rx_channel_idx < DMA_CHANNEL_AMOUNT);

    spi_core->dma.rx_channel =
        dma_channel_hw_items[dma_number][init_struct->dma.rx_channel_idx].channel;
    spi_core->dma.rx_flag_tcif =
        dma_channel_hw_items[dma_number][init_struct->dma.rx_channel_idx].tcif;
    hal_assert(spi_core->dma.rx_channel != NULL);

    dma_init_struct.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_DeInit(spi_core->dma.rx_channel);
    DMA_Init(spi_core->dma.rx_channel, &dma_init_struct);
  }
}

#elif defined(STM32L4XX)
void spi_core_dma_init(spi_core_t *spi_core, spi_core_init_struct_t *init_struct)
{
  hal_assert((init_struct->dma.number > 0) && (init_struct->dma.number <= DMA_AMOUNT));

  dma_rcc_cmd(init_struct->dma.number, true);

  LL_DMA_InitTypeDef dma_init_struct;
  dma_init_struct.PeriphOrM2MSrcAddress = (uint32_t)(&(spi_core->spi->DR));
  dma_init_struct.MemoryOrM2MDstAddress = (uint32_t)NULL;
  dma_init_struct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  dma_init_struct.Mode = LL_DMA_MODE_NORMAL;
  dma_init_struct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
  dma_init_struct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
  dma_init_struct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
  dma_init_struct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
  dma_init_struct.NbData = 0;
  dma_init_struct.Priority = LL_DMA_PRIORITY_HIGH;

  // the request channel selection is uniquely determined by the DMA and SPI numbers
  if (init_struct->dma.number == 1)
  {
    dma_init_struct.PeriphRequest = LL_DMA_REQUEST_1;
  }
  else if (init_struct->dma.number == 2)
  {
    if (init_struct->spi_number == 1)
    {
      dma_init_struct.PeriphRequest = LL_DMA_REQUEST_4;
    }
    else if (init_struct->spi_number == 3)
    {
      dma_init_struct.PeriphRequest = LL_DMA_REQUEST_3;
    }
    else
    {
      hal_assert(false);
    }
  }
  else
  {
    hal_assert(false);
  }

  uint8_t dma_number = init_struct->dma.number;
  if (init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    hal_assert(init_struct->dma.tx_channel_idx < DMA_CHANNEL_AMOUNT);

    dma_init_struct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;

    spi_core->dma.tx_dma = dma_get_by_number(dma_number);
    spi_core->dma.tx_ll_dma_channel =
        dma_channel_hw_items[dma_number][init_struct->dma.tx_channel_idx].ll_channel;
    spi_core->dma.tx_flag_tcif =
        dma_channel_hw_items[dma_number][init_struct->dma.tx_channel_idx].tcif;
    LL_DMA_Init(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel, &dma_init_struct);
  }

  if (init_struct->rx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    // DMA receive works only if DMA transmit mode is enabled
    hal_assert(init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING);
    hal_assert(init_struct->dma.rx_channel_idx < DMA_CHANNEL_AMOUNT);

    dma_init_struct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;

    spi_core->dma.rx_dma = dma_get_by_number(dma_number);
    spi_core->dma.rx_ll_dma_channel =
        dma_channel_hw_items[dma_number][init_struct->dma.rx_channel_idx].ll_channel;
    spi_core->dma.rx_flag_tcif =
        dma_channel_hw_items[dma_number][init_struct->dma.rx_channel_idx].tcif;
    LL_DMA_Init(spi_core->dma.rx_dma, spi_core->dma.rx_ll_dma_channel, &dma_init_struct);
  }
}

#elif defined(STM32F0XX)
void spi_core_dma_init(spi_core_t *spi_core, spi_core_init_struct_t *init_struct)
{
  hal_assert((init_struct->dma.number > 0) && (init_struct->dma.number <= DMA_AMOUNT));

  dma_rcc_cmd(init_struct->dma.number, true);

  LL_DMA_InitTypeDef dma_init_struct;
  dma_init_struct.PeriphOrM2MSrcAddress = (uint32_t)(&(spi_core->spi->DR));
  dma_init_struct.MemoryOrM2MDstAddress = (uint32_t)NULL;
  dma_init_struct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  dma_init_struct.Mode = LL_DMA_MODE_NORMAL;
  dma_init_struct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
  dma_init_struct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
  dma_init_struct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
  dma_init_struct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE;
  dma_init_struct.NbData = 0;
  dma_init_struct.Priority = LL_DMA_PRIORITY_HIGH;

  uint8_t dma_number = init_struct->dma.number;
  if (init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    hal_assert(init_struct->dma.tx_channel_idx < DMA_CHANNEL_AMOUNT);

    dma_init_struct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;

    spi_core->dma.tx_dma = dma_get_by_number(dma_number);
    spi_core->dma.tx_ll_dma_channel =
        dma_channel_hw_items[dma_number][init_struct->dma.tx_channel_idx].ll_channel;
    spi_core->dma.tx_flag_tcif =
        dma_channel_hw_items[dma_number][init_struct->dma.tx_channel_idx].tcif;
    LL_DMA_Init(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel, &dma_init_struct);
  }

  if (init_struct->rx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    // DMA receive works only if DMA transmit mode is enabled
    hal_assert(init_struct->tx_mode == SPI_LINE_MODE__DMA_POLLING);
    hal_assert(init_struct->dma.rx_channel_idx < DMA_CHANNEL_AMOUNT);

    dma_init_struct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;

    spi_core->dma.rx_dma = dma_get_by_number(dma_number);
    spi_core->dma.rx_ll_dma_channel =
        dma_channel_hw_items[dma_number][init_struct->dma.rx_channel_idx].ll_channel;
    spi_core->dma.rx_flag_tcif =
        dma_channel_hw_items[dma_number][init_struct->dma.rx_channel_idx].tcif;
    LL_DMA_Init(spi_core->dma.rx_dma, spi_core->dma.rx_ll_dma_channel, &dma_init_struct);
  }
}

#else
  #error This MCU not supported
#endif

void spi_rcc_cmd(uint8_t spi_number, bool new_state)
{
  hal_assert((spi_number > 0) && (spi_number <= SPI_AMOUNT));

#if defined(STM32F1XX) || defined(STM32F4XX)
  switch (spi_number)
  {
    case 1:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, new_state);
      break;
    case 2:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, new_state);
      break;
  #if defined(RCC_APB1Periph_SPI3)
    case 3:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, new_state);
      break;
  #endif
  #if defined(RCC_APB2Periph_SPI4)
    case 4:
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI4, new_state);
      break;
  #endif
    default:
      hal_assert(false);
      break;
  }

#elif defined(STM32L4XX) || defined(STM32F0XX)
  if (new_state)
  {
    switch (spi_number)
    {
  #if (FEATURE_ENABLE_SPI1)
      case 1:
        __HAL_RCC_SPI1_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_SPI2)
      case 2:
        __HAL_RCC_SPI2_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_SPI3)
      case 3:
        __HAL_RCC_SPI3_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_SPI4)
      case 4:
        __HAL_RCC_SPI4_CLK_ENABLE();
        break;
  #endif
    }
  }
  else
  {
    switch (spi_number)
    {
  #if (FEATURE_ENABLE_SPI1)
      case 1:
        __HAL_RCC_SPI1_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_SPI2)
      case 2:
        __HAL_RCC_SPI2_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_SPI3)
      case 3:
        __HAL_RCC_SPI3_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_SPI4)
      case 4:
        __HAL_RCC_SPI4_CLK_DISABLE();
        break;
  #endif
    }
  }
#else
  #error This MCU not supported
#endif
}

/**
 * @brief Initialize the SPI MISO, MOSI and CLK pins
 * @param init_struct - SPI initialization structure
 */
void spi_core_gpio_init(spi_core_init_struct_t *init_struct)
{
  gpio_rcc_cmd(init_struct->miso_gpio_idx, true);
  gpio_rcc_cmd(init_struct->mosi_gpio_idx, true);
  gpio_rcc_cmd(init_struct->clk_gpio_idx, true);

  gpio_pin_set_mode(init_struct->miso_gpio_idx, init_struct->miso_pin_idx,
                    spi_hw_items[init_struct->spi_number - 1].gpio_pin_mode);

  gpio_pin_set_mode(init_struct->mosi_gpio_idx, init_struct->mosi_pin_idx,
                    spi_hw_items[init_struct->spi_number - 1].gpio_pin_mode);

  gpio_pin_set_mode(init_struct->clk_gpio_idx, init_struct->clk_pin_idx,
                    spi_hw_items[init_struct->spi_number - 1].gpio_pin_mode);
}

/**
 * @brief Initialize an SPI slave device
 * @param spi_device - SPI slave device instance
 * @param init_struct - slave settings structure
 */
void spi_device_init(spi_device_t *spi_device)
{
  hal_assert(spi_device != NULL);
  hal_assert(spi_device->spi_core != NULL);

  gpio_rcc_cmd(spi_device->cs_gpio_idx, true);
  gpio_pin_set_mode(spi_device->cs_gpio_idx, spi_device->cs_pin_idx, GPIO_MODE_OUTPUT_PP);
  gpio_pin_cmd(spi_device->cs_gpio_idx, spi_device->cs_pin_idx, true);

  spi_core_apply_settings(spi_device->spi_core, &spi_device->settings);
}

/**
 * @brief Get the SPI prescaler from the desired frequency
 * @param spi - SPI register structure
 * @param desired_frequency - desired SPI frequency
 * @return The closest prescaler value (@ref SPI_BaudRate_Prescaler)
 */
uint16_t spi_driver_get_prescaler(SPI_TypeDef *spi, uint32_t desired_frequency)
{
  uint32_t rcc_spi_clock = 0;

#if defined(STM32F1XX) || defined(STM32F4XX)
  hal_assert(IS_SPI_ALL_PERIPH(spi));

  RCC_ClocksTypeDef rcc_clocks;
  RCC_GetClocksFreq(&rcc_clocks);

  switch ((uint32_t)spi)
  {
    case SPI1_BASE:
  #if defined(SPI4_BASE)
    case SPI4_BASE:
  #endif
      rcc_spi_clock = rcc_clocks.PCLK2_Frequency;
      break;
    case SPI2_BASE:
  #if defined(SPI3_BASE)
    case SPI3_BASE:
  #endif
      rcc_spi_clock = rcc_clocks.PCLK1_Frequency;
      break;
    default:
      hal_assert(0);
      break;
  }

#elif defined(STM32L4XX)
  LL_RCC_ClocksTypeDef rcc_clocks;
  LL_RCC_GetSystemClocksFreq(&rcc_clocks);

  switch ((uint32_t)spi)
  {
    case SPI1_BASE:
      rcc_spi_clock = rcc_clocks.PCLK2_Frequency;
      break;
    case SPI2_BASE:
    case SPI3_BASE:
      rcc_spi_clock = rcc_clocks.PCLK1_Frequency;
      break;
    default:
      hal_assert(0);
      break;
  }

#elif defined(STM32F0XX)
  HAL_UNUSED(spi);
  LL_RCC_ClocksTypeDef rcc_clocks;
  LL_RCC_GetSystemClocksFreq(&rcc_clocks);
  rcc_spi_clock = rcc_clocks.PCLK1_Frequency;

#else
  #error This MCU not supported
#endif

  // The divider can only be a power of two.
  // Find the closest divider value.
  hal_assert(rcc_spi_clock != 0);
  uint32_t divider_2pow = 2;
  uint32_t n = 0;
  while ((rcc_spi_clock >> (n + 1)) > desired_frequency)
  {
    if (divider_2pow < 256)
    {
      divider_2pow <<= 1;
      n++;
    }
    else
    {
      break;
    }
  }

  // Now convert it into an SPI_BaudRatePrescaler_x register value
  return (n << SPI_PRESCALER_OFFSET);
}

/**
 * @brief Write/read one byte
 * @param spi_device - SPI slave device instance
 * @param data - byte to write
 * @return Received byte
 */
uint8_t spi_device_tx_rx_byte(spi_device_t *spi_device, uint8_t data)
{
  if (spi_device_is_need_apply_new_settings(spi_device))
  {
    spi_core_apply_settings(spi_device->spi_core, &spi_device->settings);
  }

  uint8_t receive_byte;

#if defined(STM32F1XX) || defined(STM32F4XX)
  if (SPI_I2S_GetFlagStatus(spi_device->spi_core->spi, SPI_I2S_FLAG_RXNE) == SET)
  {
    SPI_I2S_ReceiveData(spi_device->spi_core->spi);
  }

  SPI_I2S_SendData(spi_device->spi_core->spi, data);
  while (SPI_I2S_GetFlagStatus(spi_device->spi_core->spi, SPI_I2S_FLAG_RXNE) != SET)
  {
  }
  receive_byte = SPI_I2S_ReceiveData(spi_device->spi_core->spi);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  while (LL_SPI_IsActiveFlag_BSY(spi_device->spi_core->spi))
  {
  }

  LL_SPI_TransmitData8(spi_device->spi_core->spi, data);
  while (LL_SPI_IsActiveFlag_RXNE(spi_device->spi_core->spi) == 0)
  {
  }
  receive_byte = LL_SPI_ReceiveData8(spi_device->spi_core->spi);

  while (LL_SPI_IsActiveFlag_BSY(spi_device->spi_core->spi))
  {
  }

#else
  #error This MCU not supported
#endif

  return receive_byte;
}

void spi_device_rx_array(spi_device_t *spi_device, uint8_t *buf, uint16_t size)
{
  spi_core_t *spi_core = spi_device->spi_core;

  if (spi_device_is_need_apply_new_settings(spi_device))
  {
    spi_core_apply_settings(spi_core, &spi_device->settings);
  }

#if defined(STM32F1XX)
  if (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_RXNE) == SET)
  {
    SPI_I2S_ReceiveData(spi_core->spi);
  }

  if (spi_core->rx_mode == SPI_LINE_MODE__POLLING)
  {
    for (uint32_t i = 0; i < size; i++)
    {
      SPI_I2S_SendData(spi_core->spi, 0x00);
      while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_RXNE) != SET)
      {
      }
      buf[i] = SPI_I2S_ReceiveData(spi_core->spi);
    }
  }
  else if (spi_core->rx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    // Receive channel: peripheral -> buf
    spi_core->dma.rx_channel->CMAR = (uint32_t)buf;
    spi_core->dma.rx_channel->CNDTR = size;
    DMA_Cmd(spi_core->dma.rx_channel, ENABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Rx, ENABLE);

    // Transmit channel clocks out dummy bytes from one byte without memory increment
    uint8_t tx_dummy = 0;
    spi_core->dma.tx_channel->CMAR = (uint32_t)&tx_dummy;
    spi_core->dma.tx_channel->CNDTR = size;
    spi_core->dma.tx_channel->CCR &= ~DMA_CCR1_MINC;
    DMA_Cmd(spi_core->dma.tx_channel, ENABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, ENABLE);

    while (DMA_GetFlagStatus(spi_core->dma.rx_flag_tcif) == RESET)
    {
    }

    while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_BSY) == SET)
    {
    }

    DMA_ClearFlag(spi_core->dma.tx_flag_tcif);
    DMA_Cmd(spi_core->dma.tx_channel, DISABLE);
    spi_core->dma.tx_channel->CCR |= DMA_CCR1_MINC;
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, DISABLE);

    DMA_ClearFlag(spi_core->dma.rx_flag_tcif);
    DMA_Cmd(spi_core->dma.rx_channel, DISABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Rx, DISABLE);
  }

#elif defined(STM32F4XX)
  if (SPI_I2S_GetFlagStatus(spi_device->spi_core->spi, SPI_I2S_FLAG_RXNE) == SET)
  {
    SPI_I2S_ReceiveData(spi_device->spi_core->spi);
  }

  if (spi_core->rx_mode == SPI_LINE_MODE__POLLING)
  {
    for (uint32_t i = 0; i < size; i++)
    {
      SPI_I2S_SendData(spi_device->spi_core->spi, 0x00);
      while (SPI_I2S_GetFlagStatus(spi_device->spi_core->spi, SPI_I2S_FLAG_RXNE) != SET)
      {
      }
      buf[i] = SPI_I2S_ReceiveData(spi_device->spi_core->spi);
    }
  }
  else if (spi_core->rx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    DMA_SetCurrDataCounter(spi_core->dma.rx_stream, size);
    DMA_MemoryTargetConfig(spi_core->dma.rx_stream, (uint32_t)buf, DMA_Memory_0);

    DMA_Cmd(spi_core->dma.rx_stream, ENABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Rx, ENABLE);

    uint8_t tx_buf[1] = {0};
    DMA_SetCurrDataCounter(spi_core->dma.tx_stream, size);
    DMA_MemoryTargetConfig(spi_core->dma.tx_stream, (uint32_t)tx_buf, DMA_Memory_0);
    // DMA_MemoryIncConfig(spi_core->dma.tx_stream, DMA_MemoryInc_Disable);
    hal_assert(false); // TODO need to decide with commented code upper

    DMA_Cmd(spi_core->dma.tx_stream, ENABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, ENABLE);

    while (DMA_GetFlagStatus(spi_core->dma.rx_stream, spi_core->dma.rx_flag_tcif) == RESET)
    {
    }

    while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_BSY) == SET)
    {
    }

    DMA_ClearFlag(spi_core->dma.tx_stream, spi_core->dma.tx_flag_tcif);
    DMA_Cmd(spi_core->dma.tx_stream, DISABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, DISABLE);

    DMA_ClearFlag(spi_core->dma.rx_stream, spi_core->dma.rx_flag_tcif);
    DMA_Cmd(spi_core->dma.rx_stream, DISABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Rx, DISABLE);
  }

#elif defined(STM32L4XX) || defined(STM32F0XX)
  if (spi_core->rx_mode == SPI_LINE_MODE__POLLING)
  {
    for (uint32_t i = 0; i < size; i++)
    {
      LL_SPI_TransmitData8(spi_device->spi_core->spi, 0x00);
      while (LL_SPI_IsActiveFlag_RXNE(spi_device->spi_core->spi) == 0)
      {
      }
      buf[i] = LL_SPI_ReceiveData8(spi_device->spi_core->spi);
    }
  }
  else if (spi_core->rx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    LL_DMA_SetDataLength(spi_core->dma.rx_dma, spi_core->dma.rx_ll_dma_channel, size);
    LL_DMA_SetMemoryAddress(spi_core->dma.rx_dma, spi_core->dma.rx_ll_dma_channel, (uint32_t)buf);

    LL_DMA_EnableChannel(spi_core->dma.rx_dma, spi_core->dma.rx_ll_dma_channel);
    LL_SPI_EnableDMAReq_RX(spi_core->spi);

    uint8_t tx_buf[1] = {0};
    LL_DMA_SetDataLength(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel, size);
    LL_DMA_SetMemoryAddress(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel,
                            (uint32_t)tx_buf);
    LL_DMA_SetMemoryIncMode(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel,
                            LL_DMA_MEMORY_NOINCREMENT);

    LL_DMA_EnableChannel(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel);
    LL_SPI_EnableDMAReq_TX(spi_core->spi);

    while ((spi_core->dma.rx_dma->ISR & spi_core->dma.rx_flag_tcif) == 0)
    {
    }

    while (LL_SPI_IsActiveFlag_BSY(spi_device->spi_core->spi))
    {
    }

    // formally a DMA_IFCR_CTCIFx flag should be used here instead of DMA_ISR_TCIFx,
    // but they are identical, so no separate tx_flag_ctcif field was made
    spi_core->dma.tx_dma->IFCR = spi_core->dma.tx_flag_tcif;
    LL_DMA_DisableChannel(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel);
    LL_SPI_DisableDMAReq_TX(spi_core->spi);

    // formally a DMA_IFCR_CTCIFx flag should be used here instead of DMA_ISR_TCIFx,
    // but they are identical, so no separate tx_flag_ctcif field was made
    spi_core->dma.rx_dma->IFCR = spi_core->dma.rx_flag_tcif;
    LL_DMA_DisableChannel(spi_core->dma.rx_dma, spi_core->dma.rx_ll_dma_channel);
    LL_SPI_DisableDMAReq_RX(spi_core->spi);
  }

#else
  #error This MCU not supported
#endif
}

void spi_device_tx_array(spi_device_t *spi_device, uint8_t *buf, uint16_t size)
{
  spi_core_t *spi_core = spi_device->spi_core;

  if (spi_device_is_need_apply_new_settings(spi_device))
  {
    spi_core_apply_settings(spi_core, &spi_device->settings);
  }

#if defined(STM32F1XX)
  if (spi_core->tx_mode == SPI_LINE_MODE__POLLING)
  {
    for (uint32_t i = 0; i < size; i++)
    {
      while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_TXE) != SET)
      {
      }
      SPI_I2S_SendData(spi_core->spi, buf[i]);
    }
  }
  else if (spi_core->tx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    spi_core->dma.tx_channel->CMAR = (uint32_t)buf;
    spi_core->dma.tx_channel->CNDTR = size;
    DMA_Cmd(spi_core->dma.tx_channel, ENABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, ENABLE);

    while (DMA_GetFlagStatus(spi_core->dma.tx_flag_tcif) == RESET)
    {
    }

    DMA_ClearFlag(spi_core->dma.tx_flag_tcif);
    DMA_Cmd(spi_core->dma.tx_channel, DISABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, DISABLE);
  }

  while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_BSY) == SET)
  {
  }

  // Drop the bytes received during the transmit and clear the overrun flag (read DR, then SR)
  while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_RXNE) == SET)
  {
    SPI_I2S_ReceiveData(spi_core->spi);
  }
  SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_OVR);

#elif defined(STM32F4XX)
  if (spi_core->tx_mode == SPI_LINE_MODE__POLLING)
  {
    for (uint32_t i = 0; i < size; i++)
    {
      while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_TXE) != SET)
      {
      }
      SPI_I2S_SendData(spi_core->spi, buf[i]);
    }
    while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_BSY) == SET)
    {
    }
    while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_RXNE) != SET)
    {
    }
  }
  else if (spi_core->tx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    DMA_SetCurrDataCounter(spi_core->dma.tx_stream, size);
    DMA_MemoryTargetConfig(spi_core->dma.tx_stream, (uint32_t)buf, DMA_Memory_0);
    // DMA_MemoryIncConfig(spi_core->dma.tx_stream, DMA_MemoryInc_Enable);
    hal_assert(false); // TODO need to decide with commented code upper

    DMA_Cmd(spi_core->dma.tx_stream, ENABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, ENABLE);

    while (DMA_GetFlagStatus(spi_core->dma.tx_stream, spi_core->dma.tx_flag_tcif) == RESET)
    {
    }

    while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_BSY) == SET)
    {
    }

    while (SPI_I2S_GetFlagStatus(spi_core->spi, SPI_I2S_FLAG_RXNE) == SET)
    {
      SPI_I2S_ReceiveData(spi_device->spi_core->spi);
    }

    DMA_ClearFlag(spi_core->dma.tx_stream, spi_core->dma.tx_flag_tcif);
    DMA_Cmd(spi_core->dma.tx_stream, DISABLE);
    SPI_I2S_DMACmd(spi_core->spi, SPI_I2S_DMAReq_Tx, DISABLE);
  }

#elif defined(STM32L4XX) || defined(STM32F0XX)
  if (spi_core->tx_mode == SPI_LINE_MODE__POLLING)
  {
    for (uint32_t i = 0; i < size; i++)
    {
      while (LL_SPI_IsActiveFlag_TXE(spi_device->spi_core->spi) == 0)
      {
      }
      LL_SPI_TransmitData8(spi_core->spi, buf[i]);
    }
    while (LL_SPI_IsActiveFlag_BSY(spi_device->spi_core->spi))
    {
    }

    while (LL_SPI_IsActiveFlag_RXNE(spi_device->spi_core->spi))
    {
      LL_SPI_ReceiveData8(spi_device->spi_core->spi);
    }
  }
  else if (spi_core->tx_mode == SPI_LINE_MODE__DMA_POLLING)
  {
    LL_DMA_SetDataLength(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel, size);
    LL_DMA_SetMemoryAddress(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel, (uint32_t)buf);
    LL_DMA_SetMemoryIncMode(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel,
                            LL_DMA_MEMORY_INCREMENT);

    LL_DMA_EnableChannel(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel);
    LL_SPI_EnableDMAReq_TX(spi_core->spi);

    while ((spi_core->dma.tx_dma->ISR & spi_core->dma.tx_flag_tcif) == 0)
    {
    }

    while (LL_SPI_IsActiveFlag_BSY(spi_device->spi_core->spi))
    {
    }

    while (LL_SPI_IsActiveFlag_RXNE(spi_device->spi_core->spi))
    {
      LL_SPI_ReceiveData8(spi_device->spi_core->spi);
    }

    // formally a DMA_IFCR_CTCIFx flag should be used here instead of DMA_ISR_TCIFx,
    // but they are identical, so no separate tx_flag_ctcif field was made
    spi_core->dma.tx_dma->IFCR = spi_core->dma.tx_flag_tcif;

    LL_DMA_DisableChannel(spi_core->dma.tx_dma, spi_core->dma.tx_ll_dma_channel);
    LL_SPI_DisableDMAReq_TX(spi_core->spi);
  }

#else
  #error This MCU not supported
#endif
}

/**
 * @brief Write/read a register value
 * @param spi_device - SPI slave device instance
 * @param reg - register address
 * @param value - byte to write to the register
 * @return Byte read from the register
 */
uint8_t spi_device_tx_rx_register(spi_device_t *spi_device, uint8_t reg, uint8_t value)
{
  uint8_t result;

  gpio_pin_cmd(spi_device->cs_gpio_idx, spi_device->cs_pin_idx, false);

  spi_device_tx_rx_byte(spi_device, reg);
  result = spi_device_tx_rx_byte(spi_device, value);

  gpio_pin_cmd(spi_device->cs_gpio_idx, spi_device->cs_pin_idx, true);

  return result;
}

/**
 * @brief Continuously write/read several consecutive registers
 * @param spi_device - SPI slave device instance
 * @param start_reg - address of the first register in the sequence
 * @param buffer - pointer to the buffer for reading/writing register values
 * @param size - number of registers to write/read
 */
void spi_device_tx_rx_array_registers(spi_device_t *spi_device, uint8_t start_reg, uint8_t *buffer,
                                      uint16_t size)
{
  gpio_pin_cmd(spi_device->cs_gpio_idx, spi_device->cs_pin_idx, false);

  spi_device_tx_rx_byte(spi_device, start_reg);

  for (uint16_t i = 0; i < size; i++)
  {
    buffer[i] = spi_device_tx_rx_byte(spi_device, buffer[i]);
  }

  gpio_pin_cmd(spi_device->cs_gpio_idx, spi_device->cs_pin_idx, true);
}

/**
 * @brief Check whether the current SPI driver settings are suitable for the slave device
 * @param spi_device - SPI slave device instance
 */
static inline bool spi_device_is_need_apply_new_settings(spi_device_t *spi_device)
{
  if (spi_device->spi_core->current_device_settings == NULL)
  {
    return true;
  }

  if (spi_device->spi_core->current_device_settings != &spi_device->settings)
  {
    return true;
  }

  return false;
}

/**
 * @brief Update the SPI driver settings according to the slave device requirements
 * @param spi_device - SPI slave device instance
 */
void spi_core_apply_settings(spi_core_t *spi_core, spi_device_settings_t *spi_device_settings)
{
  spi_core->current_device_settings = spi_device_settings;

#if defined(STM32F1XX) || defined(STM32F4XX)
  SPI_InitTypeDef spi_init_struct;
  spi_init_struct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  spi_init_struct.SPI_Mode = SPI_Mode_Master;
  spi_init_struct.SPI_NSS = SPI_NSS_Soft;
  spi_init_struct.SPI_CRCPolynomial = 7;

  if (spi_device_settings->first_bit == SPI_FIRST_BIT__MSB)
  {
    spi_init_struct.SPI_FirstBit = SPI_FirstBit_MSB;
  }
  else if (spi_device_settings->first_bit == SPI_FIRST_BIT__LSB)
  {
    spi_init_struct.SPI_FirstBit = SPI_FirstBit_LSB;
  }

  if (spi_device_settings->data_size == SPI_DATA_SIZE__8)
  {
    spi_init_struct.SPI_DataSize = SPI_DataSize_8b;
  }
  else if (spi_device_settings->data_size == SPI_DATA_SIZE__16)
  {
    spi_init_struct.SPI_DataSize = SPI_DataSize_16b;
  }

  if (spi_device_settings->polarity == SPI_POLARITY__LOW)
  {
    spi_init_struct.SPI_CPOL = SPI_CPOL_Low;
  }
  else if (spi_device_settings->polarity == SPI_POLARITY__HIGH)
  {
    spi_init_struct.SPI_CPOL = SPI_CPOL_High;
  }

  if (spi_device_settings->phase == SPI_PHASE__1EDGE)
  {
    spi_init_struct.SPI_CPHA = SPI_CPHA_1Edge;
  }
  else if (spi_device_settings->phase == SPI_PHASE__2EDGE)
  {
    spi_init_struct.SPI_CPHA = SPI_CPHA_2Edge;
  }

  spi_init_struct.SPI_BaudRatePrescaler =
      spi_driver_get_prescaler(spi_core->spi, spi_device_settings->frequency);

  SPI_Cmd(spi_core->spi, DISABLE);

  SPI_Init(spi_core->spi, &spi_init_struct);

  SPI_Cmd(spi_core->spi, ENABLE);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_SPI_InitTypeDef spi_init_struct;
  spi_init_struct.TransferDirection = LL_SPI_FULL_DUPLEX;
  spi_init_struct.Mode = LL_SPI_MODE_MASTER;
  spi_init_struct.NSS = LL_SPI_NSS_SOFT;
  spi_init_struct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  spi_init_struct.CRCPoly = 0;

  if (spi_device_settings->first_bit == SPI_FIRST_BIT__MSB)
  {
    spi_init_struct.BitOrder = LL_SPI_MSB_FIRST;
  }
  else if (spi_device_settings->first_bit == SPI_FIRST_BIT__LSB)
  {
    spi_init_struct.BitOrder = LL_SPI_LSB_FIRST;
  }

  if (spi_device_settings->data_size == SPI_DATA_SIZE__8)
  {
    spi_init_struct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
  }
  else if (spi_device_settings->data_size == SPI_DATA_SIZE__16)
  {
    spi_init_struct.DataWidth = LL_SPI_DATAWIDTH_16BIT;
  }

  if (spi_device_settings->polarity == SPI_POLARITY__LOW)
  {
    spi_init_struct.ClockPolarity = LL_SPI_POLARITY_LOW;
  }
  else if (spi_device_settings->polarity == SPI_POLARITY__HIGH)
  {
    spi_init_struct.ClockPolarity = LL_SPI_POLARITY_HIGH;
  }

  if (spi_device_settings->phase == SPI_PHASE__1EDGE)
  {
    spi_init_struct.ClockPhase = LL_SPI_PHASE_1EDGE;
  }
  else if (spi_device_settings->phase == SPI_PHASE__2EDGE)
  {
    spi_init_struct.ClockPhase = LL_SPI_PHASE_2EDGE;
  }

  spi_init_struct.BaudRate =
      spi_driver_get_prescaler(spi_core->spi, spi_device_settings->frequency);

  LL_SPI_Disable(spi_core->spi);

  LL_SPI_Init(spi_core->spi, &spi_init_struct);

  if (spi_device_settings->data_size == SPI_DATA_SIZE__8)
  {
    LL_SPI_SetRxFIFOThreshold(spi_core->spi, LL_SPI_RX_FIFO_TH_QUARTER);
  }
  else if (spi_device_settings->data_size == SPI_DATA_SIZE__16)
  {
    LL_SPI_SetRxFIFOThreshold(spi_core->spi, LL_SPI_RX_FIFO_TH_HALF);
  }

  LL_SPI_Enable(spi_core->spi);

#else
  #error This MCU not supported
#endif
}
