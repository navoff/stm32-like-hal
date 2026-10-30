#pragma once

/**
 * @file spi.h
 * @brief SPI master driver with per-device settings and optional DMA transfers.
 */

#include <stdbool.h>
#include <stdint.h>

#include "utils/errors.h"

/// Data size
typedef enum
{
  SPI_DATA_SIZE__8 = 0,
  SPI_DATA_SIZE__16,
  SPI_DATA_SIZE__COUNT,
} spi_data_size_t;

/// Clock polarity
typedef enum
{
  SPI_POLARITY__LOW = 0,
  SPI_POLARITY__HIGH,
  SPI_POLARITY__COUNT,
} spi_clk_polarity_t;

/// Clock phase
typedef enum
{
  SPI_PHASE__1EDGE = 0,
  SPI_PHASE__2EDGE,
  SPI_PHASE__COUNT,
} spi_clk_phase_t;

/// Bit order in a frame
typedef enum
{
  SPI_FIRST_BIT__MSB = 0,
  SPI_FIRST_BIT__LSB,
  SPI_FIRST_BIT__COUNT,
} spi_first_bit_t;

typedef enum
{
  SPI_LINE_MODE__POLLING = 0,
  SPI_LINE_MODE__DMA_POLLING,
  SPI_LINE_MODE__COUNT,
} spi_line_mode_t;

/// SPI master initialization data
typedef struct
{
  /// Number of the SPI controller register structure
  uint8_t spi_number;

  /// MISO port index
  uint8_t miso_gpio_idx;

  /// MISO pin index
  uint16_t miso_pin_idx;

  /// MOSI port index
  uint8_t mosi_gpio_idx;

  /// MOSI pin index
  uint16_t mosi_pin_idx;

  /// CLK port index
  uint8_t clk_gpio_idx;

  /// CLK pin index
  uint16_t clk_pin_idx;

  /// Transmitter mode
  spi_line_mode_t tx_mode;

  /// Receiver mode
  /// @note DMA receive mode works only if DMA transmit mode is enabled
  spi_line_mode_t rx_mode;

  /// DMA mode settings
  struct
  {
    /// Number
    uint8_t number;

    /// Transmitter channel index
    uint8_t tx_channel_idx;

#if defined(STM32F4XX)
    /// Transmitter stream index
    uint8_t tx_stream_idx;
#endif

    /// Receiver channel index
    uint8_t rx_channel_idx;

#if defined(STM32F4XX)
    /// Receiver stream index
    uint8_t rx_stream_idx;
#endif
  } dma;
} spi_core_init_struct_t;

/// SPI settings
typedef struct
{
  /// SPI frequency
  uint32_t frequency;

  /// Number of bits in a frame
  spi_data_size_t data_size;

  /// Bit order in a frame
  spi_first_bit_t first_bit;

  /// Clock polarity
  spi_clk_polarity_t polarity;

  /// Clock phase
  spi_clk_phase_t phase;
} spi_device_settings_t;

/// Data structure for SPI data exchange
typedef struct
{
#if defined(GD32E11X) || defined(GD32L23X)
  uint32_t spi;
#else
  /// SPI controller register structure
  SPI_TypeDef *spi;
#endif

  /// Transmitter mode
  spi_line_mode_t tx_mode;

  /// Receiver mode
  spi_line_mode_t rx_mode;

  struct
  {
#if defined(STM32F4XX)
    /// DMA transmit stream
    DMA_Stream_TypeDef *tx_stream;

    /// DMA receive stream
    DMA_Stream_TypeDef *rx_stream;

#elif defined(STM32F1XX)
    /// DMA transmit channel
    DMA_Channel_TypeDef *tx_channel;

    /// DMA receive channel
    DMA_Channel_TypeDef *rx_channel;

#elif defined(STM32L4XX) || defined(STM32F0XX)
    /// DMA channel of the form LL_DMA_CHANNEL_x
    uint32_t tx_ll_dma_channel;

    /// DMA instance
    DMA_TypeDef *tx_dma;

    /// DMA channel of the form LL_DMA_CHANNEL_x
    uint32_t rx_ll_dma_channel;

    /// DMA instance
    DMA_TypeDef *rx_dma;
#endif

    uint32_t tx_flag_tcif;
    uint32_t rx_flag_tcif;
  } dma;

  /// Whether a data exchange is currently in progress
  bool is_busy;

  /// Whether an error was detected
  bool is_error_detected;

  /// SPI settings
  spi_device_settings_t *current_device_settings;
} spi_core_t;

/// SPI slave device
typedef struct
{
  /// SPI master driver instance
  spi_core_t *spi_core;

  /// CS port index
  uint8_t cs_gpio_idx;

  /// CS pin index
  uint8_t cs_pin_idx;

  /// SPI settings
  spi_device_settings_t settings;
} spi_device_t;

void spi_core_init(spi_core_t *spi_core, spi_core_init_struct_t *init_struct);
void spi_device_init(spi_device_t *spi_device);

uint8_t spi_device_tx_rx_byte(spi_device_t *spi_device, uint8_t data);
void spi_device_rx_array(spi_device_t *spi_device, uint8_t *buf, uint16_t size);
void spi_device_tx_array(spi_device_t *spi_device, uint8_t *buf, uint16_t size);
uint8_t spi_device_tx_rx_register(spi_device_t *spi_device, uint8_t reg, uint8_t value);
void spi_device_tx_rx_array_registers(spi_device_t *spi_device, uint8_t start_reg, uint8_t *buffer,
                                      uint16_t size);
