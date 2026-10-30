/**
 * @file ext_flash_hw.c
 * @brief SPI transport layer for the external flash drivers.
 */

#include "config.h"

#include "drivers/gpio.h"
#include "drivers/spi.h"

#include "drivers/ext_flash_hw.h"

#define EXT_FLASH_SPI_FIRST_BIT SPI_FIRST_BIT__MSB
#define EXT_FLASH_SPI_CLOCK_POLARITY SPI_POLARITY__LOW
#define EXT_FLASH_SPI_CLOCK_PHASE SPI_PHASE__1EDGE
#define EXT_FLASH_SPI_DATA_SIZE SPI_DATA_SIZE__8
#if !defined(EXT_FLASH_SPI_FREQUENCY)
  #define EXT_FLASH_SPI_FREQUENCY 50000000U
#endif

void ext_flash_hw_enable(void);
void ext_flash_hw_disable(void);

spi_device_t ext_flash_hw_spi_device;

void ext_flash_hw_init(spi_core_t *spi_core, uint8_t cs_gpio_idx, uint8_t cs_pin_idx)
{
  ext_flash_hw_spi_device.spi_core = spi_core;
  ext_flash_hw_spi_device.cs_gpio_idx = cs_gpio_idx;
  ext_flash_hw_spi_device.cs_pin_idx = cs_pin_idx;

  ext_flash_hw_spi_device.settings.frequency = EXT_FLASH_SPI_FREQUENCY;
  ext_flash_hw_spi_device.settings.data_size = EXT_FLASH_SPI_DATA_SIZE;
  ext_flash_hw_spi_device.settings.first_bit = EXT_FLASH_SPI_FIRST_BIT;
  ext_flash_hw_spi_device.settings.phase = EXT_FLASH_SPI_CLOCK_PHASE;
  ext_flash_hw_spi_device.settings.polarity = EXT_FLASH_SPI_CLOCK_POLARITY;

  spi_device_init(&ext_flash_hw_spi_device);
}

void ext_flash_hw_enable(void)
{
  gpio_pin_cmd(ext_flash_hw_spi_device.cs_gpio_idx, ext_flash_hw_spi_device.cs_pin_idx, false);
}

void ext_flash_hw_disable(void)
{
  gpio_pin_cmd(ext_flash_hw_spi_device.cs_gpio_idx, ext_flash_hw_spi_device.cs_pin_idx, true);
}

uint8_t ext_flash_hw_tx_rx_byte(uint8_t data)
{
  return spi_device_tx_rx_byte(&ext_flash_hw_spi_device, data);
}

void ext_flash_hw_rx_array(uint8_t *buf, uint16_t size)
{
  spi_device_rx_array(&ext_flash_hw_spi_device, buf, size);
}

void ext_flash_hw_tx_array(uint8_t *buf, uint16_t size)
{
  spi_device_tx_array(&ext_flash_hw_spi_device, buf, size);
}
