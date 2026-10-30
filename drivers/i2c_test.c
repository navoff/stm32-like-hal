/**
 * @file i2c_test.c
 * @brief Loopback self-test for the I2C driver in master and slave modes.
 */

#include "config.h"

#include <stdbool.h>
#include <stdlib.h>

#include "drivers/i2c.h"
#include "utils/assert.h"

#include "drivers/i2c_test.h"

void i2c_test_master_tx_4b_end(uint8_t *buf, uint16_t count);
void i2c_test_master_tx_2b_end(uint8_t *buf, uint16_t count);
void i2c_test_master_tx_1b_end(uint8_t *buf, uint16_t count);
void i2c_test_master_rx_4b_end(uint8_t *buf, uint16_t count);
void i2c_test_master_rx_2b_end(uint8_t *buf, uint16_t count);
void i2c_test_master_rx_1b_end(uint8_t *buf, uint16_t count);

void i2c_test_slave_rx_prepare(uint8_t **p_buf, uint16_t *p_size);
void i2c_test_slave_rx_4b_end(uint8_t *buf, uint16_t count);
void i2c_test_slave_rx_2b_end(uint8_t *buf, uint16_t count);
void i2c_test_slave_rx_1b_end(uint8_t *buf, uint16_t count);
void i2c_test_slave_tx_4b_prepare(uint8_t **p_buf, uint16_t *p_size);
void i2c_test_slave_tx_4b_end(uint8_t *buf, uint16_t count);
void i2c_test_slave_tx_2b_prepare(uint8_t **p_buf, uint16_t *p_size);
void i2c_test_slave_tx_2b_end(uint8_t *buf, uint16_t count);
void i2c_test_slave_tx_1b_prepare(uint8_t **p_buf, uint16_t *p_size);
void i2c_test_slave_tx_1b_end(uint8_t *buf, uint16_t count);

struct
{
  bool master_test_ok;
  bool slave_test_ok;

  i2c_core_t *i2c_core;
  uint16_t slave_address;
  uint8_t buf[16];
} i2c_test;

void i2c_test_slave_start(i2c_core_t *i2c_core)
{
  i2c_test.slave_test_ok = false;

  i2c_test.i2c_core = i2c_core;
  i2c_test.i2c_core->prepare_slave_transmit_callback = NULL;
  i2c_test.i2c_core->transmit_end_callback = NULL;

  i2c_test.i2c_core->prepare_slave_receive_callback = i2c_test_slave_rx_prepare;
  i2c_test.i2c_core->receive_end_callback = i2c_test_slave_rx_4b_end;
}

void i2c_test_master_start(i2c_core_t *i2c_core, uint16_t slave_address)
{
  i2c_test.master_test_ok = false;
  i2c_test.slave_address = slave_address;
  i2c_test.i2c_core = i2c_core;

  i2c_test.i2c_core->prepare_slave_transmit_callback = NULL;
  i2c_test.i2c_core->prepare_slave_receive_callback = NULL;
  i2c_test.i2c_core->receive_end_callback = NULL;

  // start transmitting 4 bytes in master mode
  i2c_test.i2c_core->transmit_end_callback = i2c_test_master_tx_4b_end;
  for (int i = 0; i < 4; i++)
    i2c_test.buf[i] = i + 0x40;
  i2c_master_write_buf_req(i2c_test.i2c_core, i2c_test.slave_address, i2c_test.buf, 4);
}

void i2c_test_slave_rx_prepare(uint8_t **p_buf, uint16_t *p_size)
{
  (*p_buf) = i2c_test.buf;
  (*p_size) = 16;
}

void i2c_test_slave_rx_4b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 4);
  for (int i = 0; i < 4; i++)
    hal_assert(i2c_test.buf[i] == (i + 0x40));

  i2c_test.i2c_core->receive_end_callback = i2c_test_slave_rx_2b_end;
}

void i2c_test_slave_rx_2b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 2);
  for (int i = 0; i < 2; i++)
    hal_assert(i2c_test.buf[i] == (i + 0x20));

  i2c_test.i2c_core->receive_end_callback = i2c_test_slave_rx_1b_end;
}

void i2c_test_slave_rx_1b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 1);
  for (int i = 0; i < 1; i++)
    hal_assert(i2c_test.buf[i] == (i + 0x10));

  i2c_test.i2c_core->prepare_slave_receive_callback = NULL;
  i2c_test.i2c_core->receive_end_callback = NULL;

  i2c_test.i2c_core->prepare_slave_transmit_callback = i2c_test_slave_tx_4b_prepare;
  i2c_test.i2c_core->transmit_end_callback = i2c_test_slave_tx_4b_end;
}

void i2c_test_slave_tx_4b_prepare(uint8_t **p_buf, uint16_t *p_size)
{
  (*p_buf) = i2c_test.buf;
  (*p_size) = 16;
  for (int i = 0; i < 4; i++)
    i2c_test.buf[i] = (i + 0x40);
}

void i2c_test_slave_tx_4b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 4);

  i2c_test.i2c_core->prepare_slave_transmit_callback = i2c_test_slave_tx_2b_prepare;
  i2c_test.i2c_core->transmit_end_callback = i2c_test_slave_tx_2b_end;
}

void i2c_test_slave_tx_2b_prepare(uint8_t **p_buf, uint16_t *p_size)
{
  (*p_buf) = i2c_test.buf;
  (*p_size) = 16;
  for (int i = 0; i < 2; i++)
    i2c_test.buf[i] = (i + 0x20);
}

void i2c_test_slave_tx_2b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 2);

  i2c_test.i2c_core->prepare_slave_transmit_callback = i2c_test_slave_tx_1b_prepare;
  i2c_test.i2c_core->transmit_end_callback = i2c_test_slave_tx_1b_end;
}

void i2c_test_slave_tx_1b_prepare(uint8_t **p_buf, uint16_t *p_size)
{
  (*p_buf) = i2c_test.buf;
  (*p_size) = 16;
  for (int i = 0; i < 1; i++)
    i2c_test.buf[i] = (i + 0x10);
}

void i2c_test_slave_tx_1b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 1);

  i2c_test.i2c_core->prepare_slave_transmit_callback = NULL;
  i2c_test.i2c_core->transmit_end_callback = NULL;

  i2c_test.slave_test_ok = true;
}

void i2c_test_master_tx_4b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 4);

  // start transmitting 2 bytes in master mode
  i2c_test.i2c_core->transmit_end_callback = i2c_test_master_tx_2b_end;
  for (int i = 0; i < 2; i++)
    i2c_test.buf[i] = i + 0x20;
  i2c_master_write_buf_req(i2c_test.i2c_core, i2c_test.slave_address, i2c_test.buf, 2);
}

void i2c_test_master_tx_2b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 2);

  // start transmitting 1 byte in master mode
  i2c_test.i2c_core->transmit_end_callback = i2c_test_master_tx_1b_end;
  for (int i = 0; i < 1; i++)
    i2c_test.buf[i] = i + 0x10;
  i2c_master_write_buf_req(i2c_test.i2c_core, i2c_test.slave_address, i2c_test.buf, 1);
}

void i2c_test_master_tx_1b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 1);

  // start receiving 4 bytes in master mode
  i2c_test.i2c_core->transmit_end_callback = NULL;
  i2c_test.i2c_core->receive_end_callback = i2c_test_master_rx_4b_end;
  i2c_master_read_buf_req(i2c_test.i2c_core, i2c_test.slave_address, i2c_test.buf, 4);
}

void i2c_test_master_rx_4b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 4);
  for (int i = 0; i < 4; i++)
    hal_assert(i2c_test.buf[i] == (i + 0x40));

  // start receiving 2 bytes in master mode
  i2c_test.i2c_core->receive_end_callback = i2c_test_master_rx_2b_end;
  i2c_master_read_buf_req(i2c_test.i2c_core, i2c_test.slave_address, i2c_test.buf, 2);
}

void i2c_test_master_rx_2b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 2);
  for (int i = 0; i < 2; i++)
    hal_assert(i2c_test.buf[i] == (i + 0x20));

  // start receiving 2 bytes in master mode
  i2c_test.i2c_core->receive_end_callback = i2c_test_master_rx_1b_end;
  i2c_master_read_buf_req(i2c_test.i2c_core, i2c_test.slave_address, i2c_test.buf, 1);
}

void i2c_test_master_rx_1b_end(uint8_t *buf, uint16_t count)
{
  hal_assert(count == 1);
  for (int i = 0; i < 1; i++)
    hal_assert(i2c_test.buf[i] == (i + 0x10));

  // start receiving 2 bytes in master mode
  i2c_test.i2c_core->receive_end_callback = NULL;

  i2c_test.master_test_ok = true;
}
