/**
 * @file can.c
 * @brief CAN bus driver with automatic baud rate detection and flexible RX filtering.
 */

#include "config.h"

#include "drivers/can.h"
#include "drivers/gpio.h"
#include "drivers/tick_timer.h"
#include "utils/assert.h"
#include <string.h>

/// Extended ID flag bit
#define CAN_IDE_BIT 0x0004

/// Number of hardware FIFO buffers
#define CAN_FIFO_AMOUNT 2

/// Number of available filters
#define CAN_FILTER_BANK_SIZE 28

/**
 * @defgroup CAN_SHIFT_FILTER
 * Shifts for writing into the Filter Bank
 * @{
 */
/// Shift of the extended CAN message ID within the high part of "Filter Identifier Mask"
#define CAN_EXT_ID_FILTER_S_HI 13

/// Shift of the extended CAN message ID within the low part of "Filter Identifier Mask"
#define CAN_EXT_ID_FILTER_S_LO 3

/// Shift of the standard CAN message ID within "Filter Identifier Mask"
#define CAN_STD_ID_FILTER_S 5
/** @} */

/// Maximum possible ID for an extended packet
#define CAN_MAXIMUM_ID_EXT 0x1FFFFFFF

/// Maximum possible ID for a standard packet
#define CAN_MAXIMUM_ID_STD 0x7FF

/**
 * @defgroup CAN_TIMING
 * CAN bus timing configuration.
 * The timing values must satisfy the following condition:
 * Tbit * CAN_SPEED_PRESCALLER_1US * Tapb_can = 1 us
 * , where Tbit - duration of one CAN bit in CAN module time quanta
 *          Tbit = CAN_SJW_DEFAULT + CAN_BS1_DEFAULT + CAN_BS2_DEFAULT
 *       CAN_SPEED_PRESCALLER_1US - value of the corresponding define
 *       Tapb_can - clock period of the CAN module, us
 * @{
 */
#define CAN_SJW_DEFAULT CAN_SJW_1tq
#define CAN_BS1_DEFAULT CAN_BS1_12tq
#define CAN_BS2_DEFAULT CAN_BS2_8tq
/** @} */

/// Number of bus errors after which automatic CAN baud rate tuning starts
#define CAN_CRITICAL_ERROR_NUMBER 10

typedef struct
{
  CAN_TypeDef *CAN;
  gpio_pin_mode_t gpio_pin_mode;
  IRQn_Type RX_IRQn;
  IRQn_Type TX_IRQn;
} can_hw_item_t;

void can_rcc_cmd(uint8_t can_number, bool new_state);
void can_gpio_init(can_init_structure_t *init_struct);
void can_nvic_init(uint8_t can_number);
void can_init_core(can_t *can, uint16_t prescaler);
void can_change_speed(can_t *can);
error_t can_speed_detect_process(can_t *can);
void can_tx_first_msg(can_t *can);
void can_convert_rx_msg_from_std_lib_to_app(can_msg_t *out_msg, CanRxMsg *in_msg);
void can_convert_tx_msg_from_app_to_std_lib(CanTxMsg *out_msg, can_msg_t *in_msg);
error_t can_fifo_get(can_fifo_t *fifo, can_msg_t *msg);
error_t can_fifo_put(can_fifo_t *fifo, can_msg_t *msg);
void can_rx_common_irq_handler(can_t *can, CanRxMsg *rx_msg);

/// Prescalers used for CAN bus baud rate tuning
const uint16_t can_speed_prescaler[] = {
    CAN_SPEED_PRESCALLER_1US,  CAN_SPEED_PRESCALLER_2US, CAN_SPEED_PRESCALLER_5US,
    CAN_SPEED_PRESCALLER_10US, CAN_SPEED_PRESCALLER_END,
};

const can_hw_item_t can_hw_items[CAN_AMOUNT] = {
    {CAN1, GPIO_MODE_CAN1, CAN1_RX0_IRQn, CAN1_TX_IRQn},
    {CAN2, GPIO_MODE_CAN2, CAN2_RX0_IRQn, CAN2_TX_IRQn},
};

/**
 * @brief Fill in the CAN driver initialization structure
 * @param init_struct - initialization parameters
 */
void can_structure_init(can_init_structure_t *init_struct)
{
  memset(init_struct, 0, sizeof(can_init_structure_t));
}

/**
 * @brief Initialize CAN
 * @param can - CAN driver instance
 * @param init_struct - initialization parameters
 */
void can_init(can_t *can, can_init_structure_t *init_struct)
{
  hal_assert((init_struct != NULL) && (can != NULL));
  hal_assert((init_struct->can_number > 0) && (init_struct->can_number <= CAN_AMOUNT));
  hal_assert((init_struct->rx_buf != 0) && (init_struct->rx_buf_size != 0));
  hal_assert((init_struct->tx_buf != 0) && (init_struct->tx_buf_size != 0));

  can_fifo_clear(&can->rx_fifo);
  can->rx_fifo.buf = init_struct->rx_buf;
  can->rx_fifo.size = init_struct->rx_buf_size;

  can_fifo_clear(&can->tx_fifo);
  can->tx_fifo.buf = init_struct->tx_buf;
  can->tx_fifo.size = init_struct->tx_buf_size;

  can_rcc_cmd(init_struct->can_number, true);
  can_gpio_init(init_struct);
  can_nvic_init(init_struct->can_number);

  can->can = can_hw_items[init_struct->can_number - 1].CAN;
  can->prescaler_idx = 0;
  can->need_speed_autodetect = init_struct->need_speed_autodetect;
  can->mode = init_struct->mode;
  uint16_t prescaler;
  if (can->need_speed_autodetect)
  {
    prescaler = can_speed_prescaler[can->prescaler_idx];
    can->state = CAN_ST__SPEED_AUTODETECT;
  }
  else
  {
    prescaler = init_struct->prescaler;
    can->state = CAN_ST__NORMAL;
  }

  can_init_core(can, prescaler);

  can_reset_filters(0);

  can->stats.rx_msg_cnt = 0;
  can->stats.tx_msg_cnt = 0;
  can->can_number = init_struct->can_number;
  can->rx_callback = init_struct->rx_callback;
  can->speed_autodetect_fin_callback = init_struct->speed_autodetect_fin_callback;
}

/**
 * @brief Main CAN processing function
 */
void can_process(can_t *can)
{
  switch (can->state)
  {
    case CAN_ST__SPEED_AUTODETECT:
      if (can_speed_detect_process(can) == ERR_OK)
      {
        can->state = CAN_ST__NORMAL;

        if (can->speed_autodetect_fin_callback != NULL)
        {
          can->speed_autodetect_fin_callback();
        }
      }
      break;

    case CAN_ST__NORMAL:
    {
      can_msg_t rx_message;
      while (can_fifo_get(&can->rx_fifo, &rx_message) == ERR_OK)
      {
        if (can->rx_callback != NULL)
        {
          can->rx_callback(&rx_message);
        }
      }

      if ((can->tx_fifo.idx > 0) && (can->tx_in_progress == false))
      {
        can_tx_first_msg(can);
      }

      if (can->need_speed_autodetect)
      {
        if (CAN_GetReceiveErrorCounter(can->can) > CAN_CRITICAL_ERROR_NUMBER)
        {
          can->stats.rx_msg_cnt = 0;
          can->stats.tx_msg_cnt = 0;
          can->state = CAN_ST__SPEED_AUTODETECT;
        }
      }
    }
    break;
  }
}

/**
 * @brief Initialize CAN pins
 * @param init_struct - initialization parameters
 */
void can_gpio_init(can_init_structure_t *init_struct)
{
  hal_assert((init_struct->can_number > 0) && (init_struct->can_number <= CAN_AMOUNT));

  gpio_rcc_cmd(init_struct->tx_gpio_idx, true);
  gpio_rcc_cmd(init_struct->rx_gpio_idx, true);

  gpio_pin_set_mode(init_struct->tx_gpio_idx, init_struct->tx_pin_idx,
                    can_hw_items[init_struct->can_number - 1].gpio_pin_mode);
  gpio_pin_set_mode(init_struct->rx_gpio_idx, init_struct->rx_pin_idx,
                    can_hw_items[init_struct->can_number - 1].gpio_pin_mode);
}

/**
 * @brief Initialize CAN interrupts
 * @param can_number - CAN number
 */
void can_nvic_init(uint8_t can_number)
{
  hal_assert((can_number > 0) && (can_number <= CAN_AMOUNT));

  NVIC_InitTypeDef nvic_init_struct;
  nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 0;
  nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
  nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;

  nvic_init_struct.NVIC_IRQChannel = can_hw_items[can_number - 1].RX_IRQn;
  NVIC_Init(&nvic_init_struct);

  nvic_init_struct.NVIC_IRQChannel = can_hw_items[can_number - 1].TX_IRQn;
  NVIC_Init(&nvic_init_struct);
}

/**
 * @brief Initialize the CAN registers themselves
 * @param can - CAN driver instance
 * @param prescaler - CAN bus clock prescaler value
 */
void can_init_core(can_t *can, uint16_t prescaler)
{
  hal_assert(can->mode < CAN_MODE__COUNT);

  CAN_InitTypeDef can_init_structure;

  CAN_ITConfig(can->can, CAN_IT_FMP0, DISABLE);

  /* CAN register init */
  CAN_DeInit(can->can);

  CAN_StructInit(&can_init_structure);

  /* CAN cell init */
  can_init_structure.CAN_TTCM = DISABLE;
  can_init_structure.CAN_ABOM = ENABLE;
  can_init_structure.CAN_AWUM = ENABLE;
  can_init_structure.CAN_NART = ENABLE;
  can_init_structure.CAN_RFLM = ENABLE;
  can_init_structure.CAN_TXFP = ENABLE;

  if (can->mode == CAN_MODE__RX_TX_ACK)
  {
    can_init_structure.CAN_Mode = CAN_Mode_Normal;
  }
  else if (can->mode == CAN_MODE__RX)
  {
    can_init_structure.CAN_Mode = CAN_Mode_Silent;
  }
  can_init_structure.CAN_SJW = CAN_SJW_DEFAULT;

  /* CAN baud rate setup */
  can_init_structure.CAN_BS1 = CAN_BS1_DEFAULT;
  can_init_structure.CAN_BS2 = CAN_BS2_DEFAULT;

  can_init_structure.CAN_Prescaler = prescaler;

  CAN_Init(can->can, &can_init_structure);

  /* Enable FIFO 0 message pending Interrupt */
  CAN_ITConfig(can->can, CAN_IT_FMP0, ENABLE);

  CAN_DBGFreeze(can->can, DISABLE); // For Debug
}

/**
 * @brief Enable/disable the CAN controller module clock
 * @param can_number - CAN controller number
 * @param new_state - new module state
 */
void can_rcc_cmd(uint8_t can_number, bool new_state)
{
  hal_assert((can_number > 0) && (can_number <= CAN_AMOUNT));

  switch (can_number)
  {
    case 1:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, new_state);
      break;
    case 2:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, new_state);
      break;
  }
}

/**
 * @brief CAN baud rate autodetection process
 * @return Autodetection status
 *         ERR_OK - CAN baud rate detected successfully
 *         ERR_BUSY - the process has not finished yet
 */
error_t can_speed_detect_process(can_t *can)
{
  if (can->stats.rx_msg_cnt > 0)
  {
    return ERR_OK;
  }
  else
  {
    uint8_t err = CAN_GetLastErrorCode(can->can);
    if (err != CAN_ErrorCode_NoErr)
    {
      can_change_speed(can);
      can_set_broadcast_filter(CAN_FIFO0, 0, CAN_ID_TYPE__STD);

      if (err == CAN_ErrorCode_ACKErr)
      {
        // No acknowledgement received (nobody on the bus)
      }
    }
    return ERR_BUSY;
  }
}

/**
 * @brief Reset the filter (i.e. configure it to accept all packets)
 * @param fifo_idx - hardware FIFO index (0 to @ref CAN_FIFO_AMOUNT - 1)
 * @param filter_idx - index of the filter to configure (0 to @ref CAN_FILTER_BANK_SIZE - 1)
 * @param msg_id_type - message identifier type (@ref can_id_type_t)
 */
void can_set_broadcast_filter(uint8_t fifo_idx, uint8_t filter_idx, can_id_type_t msg_id_type)
{
  hal_assert((fifo_idx < CAN_FIFO_AMOUNT) && (filter_idx < CAN_FILTER_BANK_SIZE));
  hal_assert(msg_id_type < CAN_ID_TYPE__COUNT);

  CAN_FilterInitTypeDef can_filter_init_structure;

  can_filter_init_structure.CAN_FilterNumber = filter_idx;
  can_filter_init_structure.CAN_FilterMode = CAN_FilterMode_IdMask;
  can_filter_init_structure.CAN_FilterScale = CAN_FilterScale_32bit;

  if (msg_id_type == CAN_ID_TYPE__STD)
  {
    can_filter_init_structure.CAN_FilterIdHigh = 0x0000;
    can_filter_init_structure.CAN_FilterMaskIdHigh = 0x0000;
    can_filter_init_structure.CAN_FilterIdLow = 0x0000;
    can_filter_init_structure.CAN_FilterMaskIdLow = 0x0000;
  }
  else if (msg_id_type == CAN_ID_TYPE__EXT)
  {
    can_filter_init_structure.CAN_FilterIdHigh = 0x0000;
    can_filter_init_structure.CAN_FilterIdLow = 0x0000 | CAN_IDE_BIT;
    can_filter_init_structure.CAN_FilterMaskIdHigh = 0x0000;
    can_filter_init_structure.CAN_FilterMaskIdLow = 0x0000 | CAN_IDE_BIT;
  }

  can_filter_init_structure.CAN_FilterFIFOAssignment = fifo_idx;
  can_filter_init_structure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&can_filter_init_structure);
}

/**
 * @brief Set a filter by ID
 * @param fifo_idx - hardware FIFO index (0 to @ref CAN_FIFO_AMOUNT)
 * @param filter_idx - number of the filter to configure (0 to @ref CAN_FILTER_BANK_SIZE)
 * @param msg_id_type - message identifier type (@ref can_id_type_t)
 * @param msg_id - message identifier
 */
void can_set_filter_id(uint8_t fifo_idx, uint8_t filter_idx, can_id_type_t msg_id_type,
                       uint32_t msg_id)
{
  hal_assert((fifo_idx < CAN_FIFO_AMOUNT) && (filter_idx < CAN_FILTER_BANK_SIZE));
  hal_assert(msg_id_type < CAN_ID_TYPE__COUNT);

  CAN_FilterInitTypeDef can_filter_init_struct;

  can_filter_init_struct.CAN_FilterNumber = filter_idx;
  can_filter_init_struct.CAN_FilterMode = CAN_FilterMode_IdList;
  can_filter_init_struct.CAN_FilterScale = CAN_FilterScale_32bit;

  if ((msg_id_type == CAN_ID_TYPE__STD) && (msg_id < CAN_MAXIMUM_ID_STD))
  {
    can_filter_init_struct.CAN_FilterIdHigh = (uint16_t)(msg_id << CAN_STD_ID_FILTER_S);
    can_filter_init_struct.CAN_FilterIdLow = 0x0000;
  }
  else if ((msg_id_type == CAN_ID_TYPE__EXT) && (msg_id < CAN_MAXIMUM_ID_EXT))
  {
    can_filter_init_struct.CAN_FilterIdHigh = (uint16_t)(msg_id >> CAN_EXT_ID_FILTER_S_HI);
    can_filter_init_struct.CAN_FilterIdLow =
        (uint16_t)(msg_id << CAN_EXT_ID_FILTER_S_LO) | CAN_IDE_BIT;
  }

  can_filter_init_struct.CAN_FilterMaskIdHigh = 0x0000;
  can_filter_init_struct.CAN_FilterMaskIdLow = 0x0000;
  can_filter_init_struct.CAN_FilterFIFOAssignment = fifo_idx;
  can_filter_init_struct.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&can_filter_init_struct);
}

/**
 * @brief Reset all configured filters
 * @param fifo_idx - hardware FIFO index (0 to CAN_FIFO_AMOUNT)
 */
void can_reset_filters(uint8_t fifo_idx)
{
  hal_assert(fifo_idx < CAN_FIFO_AMOUNT);

  CAN_FilterInitTypeDef can_filter_init_structure;

  can_filter_init_structure.CAN_FilterMode = CAN_FilterMode_IdMask;
  can_filter_init_structure.CAN_FilterScale = CAN_FilterScale_32bit;
  can_filter_init_structure.CAN_FilterIdHigh = 0x0000;
  can_filter_init_structure.CAN_FilterIdLow = 0x0000;
  can_filter_init_structure.CAN_FilterMaskIdHigh = 0x0000;
  can_filter_init_structure.CAN_FilterMaskIdLow = 0x0000;
  can_filter_init_structure.CAN_FilterFIFOAssignment = fifo_idx;
  can_filter_init_structure.CAN_FilterActivation = DISABLE;

  for (uint8_t i = 0; i < CAN_FILTER_BANK_SIZE; i++)
  {
    can_filter_init_structure.CAN_FilterNumber = i;
    CAN_FilterInit(&can_filter_init_structure);
  }
}

/**
 * @brief Switch the CAN bus baud rate
 * @param can - CAN driver instance
 *
 * Switching uses the constant array of clock prescalers
 * and wraps around cyclically
 */
void can_change_speed(can_t *can)
{
  can->prescaler_idx++;
  if (can_speed_prescaler[can->prescaler_idx] == CAN_SPEED_PRESCALLER_END)
  {
    can->prescaler_idx = 0;
  }

  can_init_core(can, can_speed_prescaler[can->prescaler_idx]);
}

/**
 * @brief Send a CAN message
 * @param can - CAN driver instance
 * @param msg - pointer to the message to send
 * @return ERR_OK - message successfully queued into the transmit buffer,
 *         ERR_BOF - no room in the transmit buffer.
 */
error_t can_tx_msg(can_t *can, can_msg_t *msg)
{
  error_t res = can_fifo_put(&can->tx_fifo, msg);

  if (res == ERR_OK)
  {
    if ((can->state == CAN_ST__NORMAL) && (can->tx_fifo.idx > 0) && (can->tx_in_progress == false))
    {
      can_tx_first_msg(can);
    }
  }

  return res;
}

/**
 * @brief Start CAN message transmission
 * @param can - CAN driver instance
 */
void can_tx_first_msg(can_t *can)
{
  can->tx_in_progress = true;

  can_msg_t tx_message;
  can_fifo_get(&can->tx_fifo, &tx_message);

  CanTxMsg processed_tx_message;
  can_convert_tx_msg_from_app_to_std_lib(&processed_tx_message, &tx_message);

  CAN_ITConfig(can->can, CAN_IT_TME, ENABLE);
  CAN_Transmit(can->can, &processed_tx_message);
}

/**
 * @brief Get a received CAN message
 * @param can - CAN driver instance
 * @param msg - pointer where the received message is stored
 * @return ERR_OK - message read successfully,
 *         ERR_BUF - no received messages.
 */
error_t can_rx_msg(can_t *can, can_msg_t *msg)
{
  return can_fifo_get(&can->rx_fifo, msg);
}

/**
 * @brief Clear the CAN FIFO buffer.
 * @param fifo - pointer to the buffer
 * @note Only the buffer indices are reset, the array itself is not cleared
 */
void can_fifo_clear(can_fifo_t *fifo)
{
  fifo->head = 0;
  fifo->tail = 0;
  fifo->idx = 0;
  fifo->bof_cnt = 0;
}

/**
 * @brief Read a message from the CAN FIFO buffer. The message stays in the buffer
 * @param fifo - pointer to the buffer
 * @param msg - pointer where the CAN message from the buffer is copied
 * @return Read status, ERR_OK - success, ERR_BUF - on error
 */
error_t can_fifo_read(can_fifo_t *fifo, can_msg_t *msg)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    memcpy(msg, &fifo->buf[fifo->tail], sizeof(can_msg_t));

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Delete the oldest message from the CAN FIFO buffer
 * @param fifo - pointer to the buffer
 * @return Delete status, ERR_OK - success, ERR_BUF - on error
 */
error_t can_fifo_del(can_fifo_t *fifo)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    fifo->tail++;

    if (fifo->tail >= fifo->size)
    {
      fifo->tail = 0;
    }

    fifo->idx--;

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Get a message from the CAN FIFO buffer
 * @param fifo - pointer to the buffer
 * @param msg - pointer where the CAN message from the buffer is copied
 * @return Copy status, ERR_OK - success, ERR_BUF - on error
 */
error_t can_fifo_get(can_fifo_t *fifo, can_msg_t *msg)
{
  error_t res = ERR_OK;

  if (fifo->idx == 0)
  {
    res = ERR_BUF;
  }
  else
  {
    uint32_t save;
    ENTER_CRITICAL(save);

    memcpy(msg, &fifo->buf[fifo->tail], sizeof(can_msg_t));
    fifo->tail++;

    if (fifo->tail >= fifo->size)
    {
      fifo->tail = 0;
    }

    fifo->idx--;

    LEAVE_CRITICAL(save);
  }

  return res;
}

/**
 * @brief Add a message to the CAN FIFO buffer
 * @param fifo - pointer to the buffer
 * @param msg - CAN message
 * @return Copy status, ERR_OK - success, ERR_BOF - on error
 */
error_t can_fifo_put(can_fifo_t *fifo, can_msg_t *msg)
{
  error_t res = ERR_OK;

  uint32_t save;
  ENTER_CRITICAL(save);

  if (fifo->idx >= fifo->size)
  {
    fifo->bof_cnt++;
    res = ERR_BOF;
  }
  else
  {
    memcpy(&fifo->buf[fifo->head], msg, sizeof(can_msg_t));
    fifo->head++;

    if (fifo->head >= fifo->size)
    {
      fifo->head = 0;
    }

    fifo->idx++;
  }

  LEAVE_CRITICAL(save);

  return res;
}

/**
 * @brief Convert a received CAN message from the library format to the application format
 * @param out_msg - pointer to the converted message
 * @param in_msg - pointer to the message before conversion
 */
void can_convert_rx_msg_from_std_lib_to_app(can_msg_t *out_msg, CanRxMsg *in_msg)
{
  memcpy(out_msg->data, in_msg->Data, CAN_MAX_PACKET_SIZE);
  out_msg->dlc = in_msg->DLC;
  if (in_msg->IDE == CAN_Id_Extended)
  {
    out_msg->id = in_msg->ExtId;
    out_msg->id_type = CAN_ID_TYPE__EXT;
  }
  else
  {
    out_msg->id = in_msg->StdId;
    out_msg->id_type = CAN_ID_TYPE__STD;
  }
}

/**
 * @brief Convert a CAN message to transmit from the application format to the library format
 * @param out_msg - pointer to the converted message
 * @param in_msg - pointer to the message before conversion
 */
void can_convert_tx_msg_from_app_to_std_lib(CanTxMsg *out_msg, can_msg_t *in_msg)
{
  memcpy(out_msg->Data, in_msg->data, CAN_MAX_PACKET_SIZE);
  out_msg->DLC = in_msg->dlc;
  out_msg->RTR = CAN_RTR_Data;
  if (in_msg->id_type == CAN_ID_TYPE__EXT)
  {
    out_msg->IDE = CAN_Id_Extended;
    out_msg->ExtId = in_msg->id;
  }
  else
  {
    out_msg->IDE = CAN_Id_Standard;
    out_msg->StdId = in_msg->id;
  }
}

/**
 * @brief Common part of the CAN receive interrupt handlers for all FIFOs
 * @param can - CAN driver instance
 * @param rx_msg - received CAN message in the std-periph library format
 */
void can_rx_common_irq_handler(can_t *can, CanRxMsg *rx_msg)
{
  can_msg_t processed_rx_msg;

  START_TIMER(processed_rx_msg.timestamp_ms);
  processed_rx_msg.can_number = can->can_number;

  can_convert_rx_msg_from_std_lib_to_app(&processed_rx_msg, rx_msg);

  can_fifo_put(&can->rx_fifo, &processed_rx_msg);

  START_TIMER(can->stats.last_rx_msg_timestamp);
  can->stats.rx_msg_cnt++;
}

/**
 * @brief CAN FIFO0 receive interrupt handler
 * @param can - CAN driver instance
 */
void can_rx_irq_handler(can_t *can)
{
  if (CAN_GetITStatus(can->can, CAN_IT_FMP0) == SET)
  {
    CAN_ClearITPendingBit(can->can, CAN_IT_FMP0);

    CanRxMsg rx_msg;
    CAN_Receive(can->can, CAN_FIFO0, &rx_msg);

    can_rx_common_irq_handler(can, &rx_msg);
  }
}

/**
 * @brief CAN transmit interrupt handler
 * @param can - CAN driver instance
 */
void can_tx_irq_handler(can_t *can)
{
  // If mailbox0 is free
  if ((can->can->TSR & CAN_TSR_TME0) == CAN_TSR_TME0)
  {
    if (can->tx_fifo.idx != 0)
    {
      can_msg_t tx_message;
      can_fifo_get(&can->tx_fifo, &tx_message);

      CanTxMsg processed_tx_message;
      can_convert_tx_msg_from_app_to_std_lib(&processed_tx_message, &tx_message);

      CAN_Transmit(can->can, &processed_tx_message);
    }
    else
    {
      CAN_ITConfig(can->can, CAN_IT_TME, DISABLE);
      can->tx_in_progress = false;
    }
  }
}
