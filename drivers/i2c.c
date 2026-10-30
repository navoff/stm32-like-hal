/**
 * @file i2c.c
 * @brief I2C master/slave driver with interrupt-driven transactions.
 */

#include "config.h"

#include <stdbool.h>
#include <stdlib.h>

#include "drivers/tick_timer.h"
#include "utils/assert.h"

#include "drivers/i2c.h"

#if !defined(FEATURE_ENABLE_I2C1)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_I2C1 0
#endif

#if !defined(FEATURE_ENABLE_I2C2)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_I2C2 0
#endif

#if !defined(FEATURE_ENABLE_I2C3)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_I2C3 0
#endif

#if !defined(FEATURE_ENABLE_I2C4)
  #error Need to define in config-file:   \
    #define FEATURE_ENABLE_I2C4 0
#endif

#define I2C_AMOUNT 4

#if defined(STM32F4XX)
  #define I2C_IT_MASK (I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR)
#endif

typedef struct
{
  I2C_TypeDef *i2c;
  gpio_pin_mode_t gpio_pin_mode;
#if defined(STM32F4XX) || defined(STM32L4XX)
  IRQn_Type irq_ev;
  IRQn_Type irq_er;
#elif defined(STM32F0XX)
  IRQn_Type irq;
#else
  #error Not implemented for this MCU
#endif

} i2c_hw_item_t;

void i2c_rcc_cmd(uint8_t i2c_number, bool new_state);
void i2c_core_gpio_init(i2c_core_init_struct_t *init_struct);
void i2c_core_nvic_init(i2c_core_t *i2c_core, i2c_core_init_struct_t *init_struct);
void i2c_read_data_from_reg(i2c_core_t *i2c_core);
void i2c_write_data_to_reg(i2c_core_t *i2c_core);
void i2c_flush_tx_rx(i2c_core_t *i2c_core);
void i2c_set_none_state_and_call_callback(i2c_core_t *i2c_core, i2c_rx_tx_end_callback_t callback);

void i2c_start_transaction(i2c_core_t *i2c_core, i2c_transaction_t *transaction, bool restart);
void i2c_finish_transaction(i2c_core_t *i2c_core, error_t result);
void i2c_generate_stop_or_start_next_transaction(i2c_core_t *i2c_core);

#if defined(STM32F4XX) || defined(STM32L4XX)
const i2c_hw_item_t i2c_hw_items[I2C_AMOUNT] = {
  #if (FEATURE_ENABLE_I2C1)
    {.i2c = I2C1, .gpio_pin_mode = GPIO_MODE_I2C1, .irq_ev = I2C1_EV_IRQn, .irq_er = I2C1_ER_IRQn},
  #else
    {.i2c = NULL},
  #endif
  #if (FEATURE_ENABLE_I2C2)
    {.i2c = I2C2, .gpio_pin_mode = GPIO_MODE_I2C2, .irq_ev = I2C2_EV_IRQn, .irq_er = I2C2_ER_IRQn},
  #else
    {.i2c = NULL},
  #endif
  #if (FEATURE_ENABLE_I2C3)
    {.i2c = I2C3, .gpio_pin_mode = GPIO_MODE_I2C3, .irq_ev = I2C3_EV_IRQn, .irq_er = I2C3_ER_IRQn},
  #else
    {.i2c = NULL},
  #endif
  #if (FEATURE_ENABLE_I2C4)
    {.i2c = I2C4, .gpio_pin_mode = GPIO_MODE_I2C4, .irq_ev = I2C4_EV_IRQn, .irq_er = I2C4_ER_IRQn},
  #else
    {.i2c = NULL},
  #endif
};

#elif defined(STM32F0XX)
const i2c_hw_item_t i2c_hw_items[I2C_AMOUNT] = {
  #if (FEATURE_ENABLE_I2C1)
    {.i2c = I2C1, .gpio_pin_mode = GPIO_MODE_I2C1, .irq = I2C1_IRQn},
  #else
    {.i2c = NULL},
  #endif
  #if (FEATURE_ENABLE_I2C2)
    {.i2c = I2C2, .gpio_pin_mode = GPIO_MODE_I2C2, .irq = I2C2_IRQn},
  #else
    {.i2c = NULL},
  #endif
};

#else
  #error Not implemented for this MCU
#endif

void i2c_core_init(i2c_core_t *i2c_core, i2c_core_init_struct_t *init_struct)
{
  hal_assert((init_struct != NULL) && (i2c_core != NULL));
  hal_assert((init_struct->i2c_number > 0) && (init_struct->i2c_number <= I2C_AMOUNT));

  i2c_core->state = I2C_ST__NONE;
  i2c_core->own_address = init_struct->own_address;
  i2c_core->prepare_slave_transmit_callback = init_struct->prepare_slave_transmit_callback;
  i2c_core->prepare_slave_receive_callback = init_struct->prepare_slave_receive_callback;
  i2c_core->transmit_end_callback = init_struct->transmit_end_callback;
  i2c_core->receive_end_callback = init_struct->receive_end_callback;
  i2c_core->transactions_end_callback = init_struct->transactions_end_callback;
  i2c_flush_tx_rx(i2c_core);

  i2c_core->i2c = i2c_hw_items[init_struct->i2c_number - 1].i2c;
  if (i2c_core->i2c == NULL)
  {
    hal_assert(false);
    return;
  }

  i2c_rcc_cmd(init_struct->i2c_number, true);
  i2c_core_gpio_init(init_struct);

#if defined(STM32F4XX)
  I2C_DeInit(i2c_core->i2c);

  I2C_InitTypeDef i2c_init_struct;
  I2C_StructInit(&i2c_init_struct);
  i2c_init_struct.I2C_Mode = I2C_Mode_I2C;
  i2c_init_struct.I2C_DutyCycle = I2C_DutyCycle_2;
  i2c_init_struct.I2C_OwnAddress1 = init_struct->own_address;
  i2c_init_struct.I2C_Ack = I2C_Ack_Enable;
  i2c_init_struct.I2C_ClockSpeed = init_struct->freq;
  i2c_init_struct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_Init(i2c_core->i2c, &i2c_init_struct);

  I2C_AnalogFilterCmd(i2c_core->i2c, ENABLE);
  I2C_DigitalFilterConfig(i2c_core->i2c, 0x0F);

  i2c_core_nvic_init(i2c_core, init_struct);

  I2C_Cmd(i2c_core->i2c, ENABLE);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);

  LL_I2C_DeInit(i2c_core->i2c);

  LL_I2C_InitTypeDef i2c_init_struct;
  LL_I2C_StructInit(&i2c_init_struct);
  i2c_init_struct.PeripheralMode = LL_I2C_MODE_I2C;
  i2c_init_struct.Timing = init_struct->freq;
  i2c_init_struct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  i2c_init_struct.DigitalFilter = 0x0F;
  i2c_init_struct.OwnAddress1 = init_struct->own_address;
  i2c_init_struct.TypeAcknowledge = LL_I2C_ACK;
  i2c_init_struct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(i2c_core->i2c, &i2c_init_struct);

  i2c_core_nvic_init(i2c_core, init_struct);

  LL_I2C_Enable(i2c_core->i2c);

#else
  #error Not implemented for this MCU
#endif
}

void i2c_core_gpio_init(i2c_core_init_struct_t *init_struct)
{
  gpio_rcc_cmd(init_struct->scl_gpio_idx, true);
  gpio_rcc_cmd(init_struct->sda_gpio_idx, true);

  gpio_pin_set_mode(init_struct->scl_gpio_idx, init_struct->scl_pin_idx,
                    i2c_hw_items[init_struct->i2c_number - 1].gpio_pin_mode);

  gpio_pin_set_mode(init_struct->sda_gpio_idx, init_struct->sda_pin_idx,
                    i2c_hw_items[init_struct->i2c_number - 1].gpio_pin_mode);
}

void i2c_core_nvic_init(i2c_core_t *i2c_core, i2c_core_init_struct_t *init_struct)
{
  const i2c_hw_item_t *hw_item = &i2c_hw_items[init_struct->i2c_number - 1];

#if defined(STM32F4XX)
  NVIC_SetPriority(hw_item->irq_ev, 0);
  NVIC_EnableIRQ(hw_item->irq_ev);
  NVIC_SetPriority(hw_item->irq_er, 0);
  NVIC_EnableIRQ(hw_item->irq_er);

  I2C_ITConfig(i2c_core->i2c, I2C_IT_MASK, ENABLE);

#elif defined(STM32L4XX)
  NVIC_SetPriority(hw_item->irq_ev, 0);
  NVIC_EnableIRQ(hw_item->irq_ev);
  NVIC_SetPriority(hw_item->irq_er, 0);
  NVIC_EnableIRQ(hw_item->irq_er);

  LL_I2C_EnableIT_ADDR(i2c_core->i2c);
  LL_I2C_EnableIT_NACK(i2c_core->i2c);
  LL_I2C_EnableIT_ERR(i2c_core->i2c);
  LL_I2C_EnableIT_STOP(i2c_core->i2c);

#elif defined(STM32F0XX)
  NVIC_SetPriority(hw_item->irq, 0);
  NVIC_EnableIRQ(hw_item->irq);

  LL_I2C_EnableIT_ADDR(i2c_core->i2c);
  LL_I2C_EnableIT_NACK(i2c_core->i2c);
  LL_I2C_EnableIT_ERR(i2c_core->i2c);
  LL_I2C_EnableIT_STOP(i2c_core->i2c);

#else
  #error Not implemented for this MCU
#endif
}

void i2c_rcc_cmd(uint8_t i2c_number, bool new_state)
{
  hal_assert((i2c_number > 0) && (i2c_number <= I2C_AMOUNT));

#if defined(STM32F4XX)
  switch (i2c_number)
  {
  #if (FEATURE_ENABLE_I2C1)
    case 1:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_I2C2)
    case 2:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, new_state);
      break;
  #endif
  #if (FEATURE_ENABLE_I2C3)
    case 3:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, new_state);
      break;
  #endif
    default:
      hal_assert(false);
  }

#elif defined(STM32L4XX) || defined(STM32F0XX)
  if (new_state)
  {
    switch (i2c_number)
    {
  #if (FEATURE_ENABLE_I2C1)
      case 1:
        __HAL_RCC_I2C1_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_I2C2)
      case 2:
        __HAL_RCC_I2C2_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_I2C3)
      case 3:
        __HAL_RCC_I2C3_CLK_ENABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_I2C4)
      case 4:
        __HAL_RCC_I2C4_CLK_ENABLE();
        break;
  #endif
      default:
        hal_assert(false);
    }
  }
  else
  {
    switch (i2c_number)
    {
  #if (FEATURE_ENABLE_I2C1)
      case 1:
        __HAL_RCC_I2C1_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_I2C2)
      case 2:
        __HAL_RCC_I2C2_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_I2C3)
      case 3:
        __HAL_RCC_I2C3_CLK_DISABLE();
        break;
  #endif
  #if (FEATURE_ENABLE_I2C4)
      case 4:
        __HAL_RCC_I2C4_CLK_DISABLE();
        break;
  #endif
      default:
        hal_assert(false);
    }
  }
#else
  #error Not implemented for this MCU
#endif
}

error_t i2c_master_write_buf_req(i2c_core_t *i2c_core, uint16_t slave_address, uint8_t *buf,
                                 uint16_t size)
{
  if (i2c_core->state != I2C_ST__NONE)
  {
    return ERR_BUSY;
  }

  i2c_core->next_state = I2C_ST__MASTER_SEND_TRANSMIT_REQ;
  i2c_core->slave_address = slave_address;
  i2c_core->tx_rx.count = 0;
  i2c_core->tx_rx.size = size;
  i2c_core->tx_rx.buf = buf;

#if defined(STM32F4XX)
  i2c_core->state = I2C_ST__START;

  I2C_GenerateSTART(i2c_core->i2c, ENABLE);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  i2c_core->state = I2C_ST__MASTER_TRANSMIT;

  LL_I2C_EnableIT_TX(i2c_core->i2c);
  LL_I2C_HandleTransfer(i2c_core->i2c, i2c_core->slave_address, LL_I2C_ADDRSLAVE_7BIT,
                        i2c_core->tx_rx.size, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

#else
  #error Not implemented for this MCU
#endif

  return ERR_OK;
}

error_t i2c_master_read_buf_req(i2c_core_t *i2c_core, uint16_t slave_address, uint8_t *buf,
                                uint16_t size)
{
  if (i2c_core->state != I2C_ST__NONE)
  {
    return ERR_BUSY;
  }

  i2c_core->next_state = I2C_ST__MASTER_SEND_RECEIVE_REQ;
  i2c_core->slave_address = slave_address;
  i2c_core->tx_rx.count = 0;
  i2c_core->tx_rx.size = size;
  i2c_core->tx_rx.buf = buf;

#if defined(STM32F4XX)
  i2c_core->state = I2C_ST__START;

  if (i2c_core->tx_rx.size > 2)
  {
    I2C_NACKPositionConfig(i2c_core->i2c, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(i2c_core->i2c, ENABLE);
  }
  else if (i2c_core->tx_rx.size == 2)
  {
    I2C_NACKPositionConfig(i2c_core->i2c, I2C_NACKPosition_Next);
    I2C_AcknowledgeConfig(i2c_core->i2c, DISABLE);
  }
  else
  {
    I2C_NACKPositionConfig(i2c_core->i2c, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(i2c_core->i2c, DISABLE);
  }
  I2C_GenerateSTART(i2c_core->i2c, ENABLE);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  i2c_core->state = I2C_ST__MASTER_RECEIVE;

  LL_I2C_EnableIT_RX(i2c_core->i2c);
  LL_I2C_HandleTransfer(i2c_core->i2c, i2c_core->slave_address, LL_I2C_ADDRSLAVE_7BIT,
                        i2c_core->tx_rx.size, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

#else
  #error Not implemented for this MCU
#endif

  return ERR_OK;
}

void i2c_start_transaction(i2c_core_t *i2c_core, i2c_transaction_t *transaction, bool restart)
{
#if defined(STM32L4XX) || defined(STM32F0XX)
  uint32_t request;
  uint32_t end_mode = restart ? LL_I2C_MODE_SOFTEND : LL_I2C_MODE_AUTOEND;

  if (transaction->type == I2C_TRANSACTION_TYPE__WRITE)
  {
    request = restart ? LL_I2C_GENERATE_RESTART_7BIT_WRITE : LL_I2C_GENERATE_START_WRITE;
    LL_I2C_EnableIT_TC(i2c_core->i2c);
    LL_I2C_EnableIT_TX(i2c_core->i2c);
  }
  else
  {
    request = restart ? LL_I2C_GENERATE_RESTART_7BIT_READ : LL_I2C_GENERATE_START_READ;
    LL_I2C_EnableIT_RX(i2c_core->i2c);
  }

  i2c_core->transactions.current_type = transaction->type;
  i2c_core->tx_rx.count = 0;
  i2c_core->tx_rx.size = transaction->size;
  i2c_core->tx_rx.buf = transaction->buf;
  i2c_core->tx_rx.was_nack = false;

  // reset the internal I2C state (including the internal shift register)
  LL_I2C_Disable(i2c_core->i2c);
  LL_I2C_Enable(i2c_core->i2c);

  LL_I2C_HandleTransfer(i2c_core->i2c, i2c_core->slave_address, LL_I2C_ADDRSLAVE_7BIT,
                        transaction->size, end_mode, request);
#else
  #error Not implemented for this MCU
#endif
}

void i2c_generate_stop_or_start_next_transaction(i2c_core_t *i2c_core)
{
  if ((i2c_core->transactions.count + 1) >= i2c_core->transactions.size)
  {
    LL_I2C_GenerateStopCondition(i2c_core->i2c);
  }
  else
  {
    i2c_core->transactions.count++;
    i2c_start_transaction(i2c_core, &i2c_core->transactions.buf[i2c_core->transactions.count],
                          true);
  }
}

void i2c_finish_transaction(i2c_core_t *i2c_core, error_t result)
{
  if (i2c_core->state == I2C_ST__NONE)
  {
    return;
  }

  i2c_core->state = I2C_ST__NONE;
  i2c_core->transactions.count = 0;
  i2c_core->transactions.size = 0;

  if (i2c_core->transactions_end_callback != NULL)
  {
    i2c_core->transactions_end_callback(i2c_core, result);
  }
}

error_t i2c_master_transactions_req(i2c_core_t *i2c_core, uint16_t slave_address,
                                    i2c_transaction_t *transactions, uint16_t size,
                                    bool restart_enable)
{
  if (i2c_core->state != I2C_ST__NONE)
  {
    return ERR_BUSY;
  }

  i2c_core->slave_address = slave_address;
  i2c_core->transactions.count = 0;
  i2c_core->transactions.size = size;
  i2c_core->transactions.buf = transactions;
  i2c_core->transactions.restart_enable = restart_enable;

#if defined(STM32F4XX)
  #error Not implemented for this MCU

#elif defined(STM32L4XX) || defined(STM32F0XX)
  i2c_core->state = I2C_ST__MASTER_TRASACTIONS;
  i2c_start_transaction(i2c_core, &i2c_core->transactions.buf[0], restart_enable);

#else
  #error Not implemented for this MCU
#endif

  return ERR_OK;
}

void i2c_read_data_from_reg(i2c_core_t *i2c_core)
{
#if defined(STM32F4XX)
  uint8_t data = I2C_ReceiveData(i2c_core->i2c);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  uint8_t data = LL_I2C_ReceiveData8(i2c_core->i2c);

#else
  #error Not implemented for this MCU
#endif

  if ((i2c_core->tx_rx.buf != NULL) && (i2c_core->tx_rx.count < i2c_core->tx_rx.size))
  {
    i2c_core->tx_rx.buf[i2c_core->tx_rx.count] = data;
    i2c_core->tx_rx.count++;
  }
}

void i2c_write_data_to_reg(i2c_core_t *i2c_core)
{
  uint8_t data = 0;
  if ((i2c_core->tx_rx.buf != NULL) && (i2c_core->tx_rx.count < i2c_core->tx_rx.size))
  {
    data = i2c_core->tx_rx.buf[i2c_core->tx_rx.count];
    i2c_core->tx_rx.count++;
  }

#if defined(STM32F4XX)
  I2C_SendData(i2c_core->i2c, data);

#elif defined(STM32L4XX) || defined(STM32F0XX)
  LL_I2C_TransmitData8(i2c_core->i2c, data);
#else
  #error Not implemented for this MCU
#endif
}

void i2c_flush_tx_rx(i2c_core_t *i2c_core)
{
  i2c_core->tx_rx.buf = NULL;
  i2c_core->tx_rx.count = 0;
  i2c_core->tx_rx.size = 0;
}

void i2c_set_none_state_and_call_callback(i2c_core_t *i2c_core, i2c_rx_tx_end_callback_t callback)
{
  uint8_t *buf = i2c_core->tx_rx.buf;
  uint16_t count = i2c_core->tx_rx.count;
  if (count > i2c_core->tx_rx.size)
  {
    count = i2c_core->tx_rx.size;
  }

  i2c_flush_tx_rx(i2c_core);
  i2c_core->state = I2C_ST__NONE;

  if (callback != NULL)
  {
    callback(buf, count);
  }
}

#if defined(STM32F4XX)
void i2c_ev_irq_hander(i2c_core_t *i2c_core)
{
  uint32_t event = I2C_GetLastEvent(i2c_core->i2c);

  switch (i2c_core->state)
  {
    case I2C_ST__NONE:
      if (event == I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED)
      {
        i2c_core->state = I2C_ST__SLAVE_RECEIVE;

        i2c_flush_tx_rx(i2c_core);
        if (i2c_core->prepare_slave_receive_callback != NULL)
        {
          i2c_core->prepare_slave_receive_callback(&i2c_core->tx_rx.buf, &i2c_core->tx_rx.size);
        }
      }

      if (event == I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED)
      {
        i2c_core->state = I2C_ST__SLAVE_TRANSMIT;

        i2c_flush_tx_rx(i2c_core);
        if (i2c_core->prepare_slave_transmit_callback != NULL)
        {
          i2c_core->prepare_slave_transmit_callback(&i2c_core->tx_rx.buf, &i2c_core->tx_rx.size);
        }
      }
      break;

    case I2C_ST__SLAVE_RECEIVE:
      if ((event == I2C_EVENT_SLAVE_BYTE_RECEIVED) ||
          (event == I2C_EVENT_SLAVE_BYTE_RECEIVED_AND_BTF))
      {
        i2c_read_data_from_reg(i2c_core);
      }

      if (event == I2C_EVENT_SLAVE_STOP_DETECTED)
      {
        // clear the STOPF flag
        I2C_Cmd(i2c_core->i2c, DISABLE);
        I2C_Cmd(i2c_core->i2c, ENABLE);

        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->receive_end_callback);
      }
      break;

    case I2C_ST__SLAVE_TRANSMIT:
      if ((event == I2C_EVENT_SLAVE_BYTE_TRANSMITTED) ||
          (event == I2C_EVENT_SLAVE_BYTE_TRANSMITTING))
      {
        i2c_write_data_to_reg(i2c_core);
      }

      if (event == I2C_EVENT_SLAVE_ACK_FAILURE)
      {
        I2C_ClearFlag(i2c_core->i2c, I2C_FLAG_AF);

        // the last byte was not actually sent, it is still in the shift register
        i2c_core->tx_rx.count--;
        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->transmit_end_callback);
      }
      break;

    case I2C_ST__START:
      // START generated successfully - the bus is free, transmission can begin
      if (event == I2C_EVENT_MASTER_MODE_SELECT)
      {
        if (i2c_core->next_state == I2C_ST__MASTER_SEND_TRANSMIT_REQ)
        {
          I2C_Send7bitAddress(i2c_core->i2c, i2c_core->slave_address, I2C_Direction_Transmitter);
        }
        else
        {
          I2C_Send7bitAddress(i2c_core->i2c, i2c_core->slave_address, I2C_Direction_Receiver);
        }
        i2c_core->state = i2c_core->next_state;
      }
      break;

    case I2C_ST__MASTER_SEND_TRANSMIT_REQ:
      if (event == I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)
      {
        i2c_write_data_to_reg(i2c_core);
        i2c_core->state = I2C_ST__MASTER_TRANSMIT;
      }

      // if nobody answered
      if (I2C_GetFlagStatus(i2c_core->i2c, I2C_FLAG_AF))
      {
        I2C_ClearFlag(i2c_core->i2c, I2C_FLAG_AF);
        I2C_GenerateSTOP(i2c_core->i2c, ENABLE);
        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->receive_end_callback);
      }
      break;

    case I2C_ST__MASTER_SEND_RECEIVE_REQ:
      if (event == I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)
      {
        if (i2c_core->tx_rx.size > 2)
        {
          i2c_core->state = I2C_ST__MASTER_RECEIVE;
        }
        else if (i2c_core->tx_rx.size == 2)
        {
          i2c_core->state = I2C_ST__MASTER_RECEIVE_LAST_2_BYTE;
        }
        else
        {
          i2c_core->state = I2C_ST__MASTER_RECEIVE_LAST_1_BYTE;
        }
      }

      // if nobody answered
      if (I2C_GetFlagStatus(i2c_core->i2c, I2C_FLAG_AF))
      {
        I2C_ClearFlag(i2c_core->i2c, I2C_FLAG_AF);
        I2C_GenerateSTOP(i2c_core->i2c, ENABLE);
        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->receive_end_callback);
      }
      break;

    case I2C_ST__MASTER_TRANSMIT:
      if (event == I2C_EVENT_MASTER_BYTE_TRANSMITTED)
      {
        if (i2c_core->tx_rx.count >= i2c_core->tx_rx.size)
        {
          I2C_GenerateSTOP(i2c_core->i2c, ENABLE);
          i2c_set_none_state_and_call_callback(i2c_core, i2c_core->transmit_end_callback);
        }
        else
        {
          i2c_write_data_to_reg(i2c_core);
        }
      }
      break;

    case I2C_ST__MASTER_RECEIVE_LAST_1_BYTE:
      if (event == I2C_EVENT_MASTER_BYTE_RECEIVED)
      {
        I2C_GenerateSTOP(i2c_core->i2c, ENABLE);
        i2c_read_data_from_reg(i2c_core);
        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->receive_end_callback);
      }
      break;

    case I2C_ST__MASTER_RECEIVE_LAST_2_BYTE:
      if (event == I2C_EVENT_MASTER_BYTE_RECEIVED_AND_BTF)
      {
        I2C_GenerateSTOP(i2c_core->i2c, ENABLE);
        i2c_read_data_from_reg(i2c_core);
        i2c_read_data_from_reg(i2c_core);
        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->receive_end_callback);
      }
      break;

    case I2C_ST__MASTER_RECEIVE:
    {
      uint16_t wait_number = i2c_core->tx_rx.count + 1;
      if (wait_number < (i2c_core->tx_rx.size - 2))
      {
        if ((event == I2C_EVENT_MASTER_BYTE_RECEIVED) ||
            (event == I2C_EVENT_MASTER_BYTE_RECEIVED_AND_BTF))
        {
          i2c_read_data_from_reg(i2c_core);
        }
      }
      else
      {
        if (event == I2C_EVENT_MASTER_BYTE_RECEIVED_AND_BTF)
        {
          I2C_AcknowledgeConfig(i2c_core->i2c, DISABLE);
          i2c_read_data_from_reg(i2c_core);
          i2c_core->state = I2C_ST__MASTER_RECEIVE_LAST_2_BYTE;
        }
      }
    }
    break;
  }
}

#elif defined(STM32L4XX) || defined(STM32F0XX)
void i2c_ev_irq_hander(i2c_core_t *i2c_core)
{
  switch (i2c_core->state)
  {
    case I2C_ST__NONE:
      if (LL_I2C_IsActiveFlag_ADDR(i2c_core->i2c))
      {
        if (LL_I2C_GetAddressMatchCode(i2c_core->i2c) == i2c_core->own_address)
        {
          // if a read request was received
          if (LL_I2C_GetTransferDirection(i2c_core->i2c) == LL_I2C_DIRECTION_READ)
          {
            LL_I2C_ClearFlag_ADDR(i2c_core->i2c);
            LL_I2C_EnableIT_TX(i2c_core->i2c);
            i2c_core->state = I2C_ST__SLAVE_TRANSMIT;

            i2c_flush_tx_rx(i2c_core);
            if (i2c_core->prepare_slave_transmit_callback != NULL)
            {
              i2c_core->prepare_slave_transmit_callback(&i2c_core->tx_rx.buf,
                                                        &i2c_core->tx_rx.size);
            }
          }
          // if a write request was received
          else
          {
            LL_I2C_ClearFlag_ADDR(i2c_core->i2c);
            LL_I2C_EnableIT_RX(i2c_core->i2c);
            i2c_core->state = I2C_ST__SLAVE_RECEIVE;

            i2c_flush_tx_rx(i2c_core);
            if (i2c_core->prepare_slave_receive_callback != NULL)
            {
              i2c_core->prepare_slave_receive_callback(&i2c_core->tx_rx.buf, &i2c_core->tx_rx.size);
            }
          }
        }
      }

      if (LL_I2C_IsActiveFlag_STOP(i2c_core->i2c))
      {
        LL_I2C_ClearFlag_STOP(i2c_core->i2c);
      }
      break;

    case I2C_ST__SLAVE_TRANSMIT:
    case I2C_ST__MASTER_TRANSMIT:
      if (LL_I2C_IsActiveFlag_NACK(i2c_core->i2c))
      {
        LL_I2C_ClearFlag_NACK(i2c_core->i2c);
      }

      if (LL_I2C_IsActiveFlag_TXIS(i2c_core->i2c))
      {
        i2c_write_data_to_reg(i2c_core);
      }

      if (LL_I2C_IsActiveFlag_STOP(i2c_core->i2c))
      {
        LL_I2C_ClearFlag_STOP(i2c_core->i2c);
        LL_I2C_DisableIT_TX(i2c_core->i2c);
        LL_I2C_ClearFlag_TXE(i2c_core->i2c);

        if (i2c_core->state == I2C_ST__SLAVE_TRANSMIT)
        {
          // the last byte was not actually sent, it is still in the shift register
          i2c_core->tx_rx.count--;
        }
        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->transmit_end_callback);
      }
      break;

    case I2C_ST__SLAVE_RECEIVE:
    case I2C_ST__MASTER_RECEIVE:
      if (LL_I2C_IsActiveFlag_NACK(i2c_core->i2c))
      {
        LL_I2C_ClearFlag_NACK(i2c_core->i2c);
      }

      if (LL_I2C_IsActiveFlag_RXNE(i2c_core->i2c))
      {
        i2c_read_data_from_reg(i2c_core);
      }

      if (LL_I2C_IsActiveFlag_STOP(i2c_core->i2c))
      {
        LL_I2C_ClearFlag_STOP(i2c_core->i2c);
        LL_I2C_DisableIT_RX(i2c_core->i2c);
        i2c_set_none_state_and_call_callback(i2c_core, i2c_core->receive_end_callback);
      }
      break;

    case I2C_ST__MASTER_TRASACTIONS:
    {
      if (LL_I2C_IsActiveFlag_NACK(i2c_core->i2c))
      {
        LL_I2C_ClearFlag_NACK(i2c_core->i2c);

        LL_I2C_DisableIT_TX(i2c_core->i2c);
        LL_I2C_DisableIT_TC(i2c_core->i2c);
        LL_I2C_DisableIT_RX(i2c_core->i2c);
        i2c_core->tx_rx.was_nack = true;
      }

      if (i2c_core->transactions.current_type == I2C_TRANSACTION_TYPE__WRITE)
      {
        if (LL_I2C_IsActiveFlag_TXIS(i2c_core->i2c))
        {
          i2c_write_data_to_reg(i2c_core);
        }

        if (LL_I2C_IsActiveFlag_TC(i2c_core->i2c))
        {
          LL_I2C_DisableIT_TX(i2c_core->i2c);
          LL_I2C_DisableIT_TC(i2c_core->i2c);

          if (i2c_core->transactions.restart_enable)
          {
            i2c_generate_stop_or_start_next_transaction(i2c_core);
          }
        }
      }
      else if (i2c_core->transactions.current_type == I2C_TRANSACTION_TYPE__READ)
      {
        if (LL_I2C_IsActiveFlag_RXNE(i2c_core->i2c))
        {
          i2c_read_data_from_reg(i2c_core);

          if (i2c_core->tx_rx.count >= i2c_core->tx_rx.size)
          {
            LL_I2C_DisableIT_RX(i2c_core->i2c);

            if (i2c_core->transactions.restart_enable)
            {
              i2c_generate_stop_or_start_next_transaction(i2c_core);
            }
          }
        }
      }

      if (LL_I2C_IsActiveFlag_STOP(i2c_core->i2c))
      {
        LL_I2C_ClearFlag_STOP(i2c_core->i2c);

        bool was_error = false;

        if ((i2c_core->transactions.current_type == I2C_TRANSACTION_TYPE__WRITE) &&
            (i2c_core->tx_rx.was_nack))
        {
          was_error = true;
        }
        if (i2c_core->tx_rx.count < i2c_core->tx_rx.size)
        {
          was_error = true;
        }

        if (was_error)
        {
          i2c_finish_transaction(i2c_core, ERR_RESULT);
        }
        else
        {
          i2c_core->transactions.count++;
          if (i2c_core->transactions.count >= i2c_core->transactions.size)
          {
            i2c_finish_transaction(i2c_core, ERR_OK);
          }
          else
          {
            if (i2c_core->transactions.restart_enable == false)
            {
              i2c_start_transaction(
                  i2c_core, &i2c_core->transactions.buf[i2c_core->transactions.count], false);
            }
          }
        }
      }
    }
    break;

    default:
      break;
  }
}

#else
  #error Not implemented for this MCU
#endif
