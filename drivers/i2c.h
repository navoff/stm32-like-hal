#pragma once

/**
 * @file i2c.h
 * @brief I2C master/slave driver with interrupt-driven transactions.
 */

#include <stdbool.h>
#include <stdint.h>

#include "drivers/gpio.h"
#include "utils/errors.h"

typedef void (*i2c_slave_callback_t)(uint8_t **p_buf, uint16_t *p_size);
typedef void (*i2c_rx_tx_end_callback_t)(uint8_t *buf, uint16_t count);
typedef void (*i2c_transactions_end_callback_t)(void *i2c_core, error_t result);

/// I2C master initialization data
typedef struct
{
  /// Own device address on the bus
  uint16_t own_address;

  /// For the STM32F4xx family
  ///  - bus frequency, Hz
  /// For the STM32L4xx, STM32F0xx families
  ///  - TIMINGR register value,
  ///    use the __LL_I2C_CONVERT_TIMINGS macro to set it correctly
  /// @note The field is used only in master mode
  uint32_t freq;

  /// Number of the I2C controller register structure
  uint8_t i2c_number;

  /// SDA port index
  uint8_t sda_gpio_idx;

  /// SDA pin index
  uint16_t sda_pin_idx;

  /// SCL port index
  uint8_t scl_gpio_idx;

  /// SCL pin index
  uint16_t scl_pin_idx;

  /// Callback invoked in slave mode
  /// when the master requests to read data from the slave.
  /// In this function the pointer and size of the buffer
  /// to be sent to the master can be set.
  i2c_slave_callback_t prepare_slave_transmit_callback;

  /// Callback invoked in slave mode
  /// when the master requests to write data to the slave device.
  /// In this function the pointer and size of the buffer
  /// that receives the data from the master can be set.
  i2c_slave_callback_t prepare_slave_receive_callback;

  /// Callback invoked in both master and slave mode
  /// when data transmission is finished (STOP condition detected).
  /// Not all data may have been sent,
  /// so the callback arguments must be checked
  i2c_rx_tx_end_callback_t transmit_end_callback;

  /// Callback invoked in both master and slave mode
  /// when data reception is finished (STOP condition detected).
  /// The number of received bytes is passed in the arguments.
  i2c_rx_tx_end_callback_t receive_end_callback;

  /// Callback invoked in master mode
  /// when the transactions have finished (successfully or with an error)
  i2c_transactions_end_callback_t transactions_end_callback;
} i2c_core_init_struct_t;

/// Possible I2C driver states
typedef enum
{
  I2C_ST__NONE,

  I2C_ST__START,

  I2C_ST__SLAVE_TRANSMIT,
  I2C_ST__SLAVE_RECEIVE,

  I2C_ST__MASTER_SEND_TRANSMIT_REQ,
  I2C_ST__MASTER_TRANSMIT,

  I2C_ST__MASTER_SEND_RECEIVE_REQ,
  I2C_ST__MASTER_RECEIVE,
  I2C_ST__MASTER_RECEIVE_LAST_1_BYTE,
  I2C_ST__MASTER_RECEIVE_LAST_2_BYTE,

  I2C_ST__MASTER_TRASACTIONS,
} i2c_state_t;

typedef enum
{
  I2C_TRANSACTION_TYPE__WRITE,
  I2C_TRANSACTION_TYPE__READ,
} i2c_transaction_type_t;

typedef struct
{
  /// Required amount of data to transmit/receive
  /// May be 0.
  uint16_t size;

  /// Buffer that receives the incoming data or supplies the data to send
  /// May be NULL
  uint8_t *buf;

  i2c_transaction_type_t type;
} i2c_transaction_t;

/// Data structure for I2C data exchange
typedef struct
{
  /// I2C controller register structure
  I2C_TypeDef *i2c;

  /// Current I2C driver state
  i2c_state_t state;

  i2c_state_t next_state;

  /// Callback invoked in slave mode
  /// when the master requests to read data from the slave.
  /// In this function the pointer and size of the buffer
  /// to be sent to the master can be set.
  i2c_slave_callback_t prepare_slave_transmit_callback;

  /// Callback invoked in slave mode
  /// when the master requests to write data to the slave device.
  /// In this function the pointer and size of the buffer
  /// that receives the data from the master can be set.
  i2c_slave_callback_t prepare_slave_receive_callback;

  /// Callback invoked in both master and slave mode
  /// when data transmission is finished (STOP condition detected).
  /// Not all data may have been sent,
  /// so the callback arguments must be checked
  i2c_rx_tx_end_callback_t transmit_end_callback;

  /// Callback invoked in both master and slave mode
  /// when data reception is finished (STOP condition detected).
  /// The number of received bytes is passed in the arguments.
  i2c_rx_tx_end_callback_t receive_end_callback;

  /// Callback invoked in master mode
  /// when the transactions have finished (successfully or with an error)
  i2c_transactions_end_callback_t transactions_end_callback;

  /// Own device address on the bus
  uint16_t own_address;

  /// Slave device address.
  /// Used only in slave mode.
  uint16_t slave_address;

  struct
  {
    /// Number of bytes read from/written to the I2C data register
    /// May differ from the actual number of bytes sent on the bus
    uint16_t count;

    /// Required amount of data to transmit/receive
    /// May be 0.
    uint16_t size;

    /// Buffer that receives the incoming data or supplies the data to send
    /// May be NULL
    uint8_t *buf;

    bool was_nack;
  } tx_rx;

  struct
  {
    /// Pointer to the array of transactions to execute
    i2c_transaction_t *buf;

    /// Array size
    uint8_t size;

    /// Number of completed transactions
    uint8_t count;

    /// Type of the transaction currently in progress
    i2c_transaction_type_t current_type;

    /// Whether a RESTART is generated between transactions
    bool restart_enable;
  } transactions;
} i2c_core_t;

/**
 * @brief Initialize the I2C driver
 * @param i2c_core - pointer to the driver instance
 * @param init_struct - initialization structure
 */
void i2c_core_init(i2c_core_t *i2c_core, i2c_core_init_struct_t *init_struct);

/**
 * @brief Default I2C interrupt handler
 * @param i2c_core - pointer to the driver instance
 *
 * This handler must be called from the corresponding interrupt handlers:
 *   I2Cx_ER_IRQHandler
 *   I2Cx_EV_IRQHandler
 * , where x is the I2C number
 */
void i2c_ev_irq_hander(i2c_core_t *i2c_core);

/**
 * @brief Request to read data from a slave device in master mode
 * @param i2c_core - pointer to the driver instance
 * @param slave_address - address of the slave device the data is read from
 *        Only 7-bit addressing is supported.
 * @param buf - pointer to the receive buffer
 * @param size - number of bytes to read
 * @return ERR_OK - request sent successfully, otherwise - error
 *
 * This function can be used together with the callback "i2c_core->receive_end_callback"
 */
error_t i2c_master_read_buf_req(i2c_core_t *i2c_core, uint16_t slave_address, uint8_t *buf,
                                uint16_t size);

/**
 * @brief Request to transmit data to a slave device in master mode
 * @param i2c_core - pointer to the driver instance
 * @param slave_address - address of the slave device the data is sent to
 *        Only 7-bit addressing is supported.
 * @param buf - pointer to the transmit buffer
 * @param size - buffer size
 * @return ERR_OK - request sent successfully, otherwise - error
 *
 * This function can be used together with the callback "i2c_core->transmit_end_callback"
 */
error_t i2c_master_write_buf_req(i2c_core_t *i2c_core, uint16_t slave_address, uint8_t *buf,
                                 uint16_t size);

/**
 * @brief Request to execute several transactions in master mode
 * @param i2c_core - pointer to the driver instance
 * @param slave_address - address of the slave device the data is sent to
 *        Only 7-bit addressing is supported.
 * @param transactions - pointer to the array of transactions to execute
 *        The array must reside in static memory.
 * @param size - number of transactions
 * @param restart_enable - whether a RESTART is generated between transactions;
 *        if not set, each transaction ends with a STOP
 *        and the next one starts with a START
 * @return ERR_OK - request sent successfully, otherwise - error
 *
 * This function can be used together with the callback "i2c_core->transactions_end_callback"
 */
error_t i2c_master_transactions_req(i2c_core_t *i2c_core, uint16_t slave_address,
                                    i2c_transaction_t *transactions, uint16_t size,
                                    bool restart_enable);
