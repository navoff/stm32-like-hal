/**
 * @file usb_conf.h
 * @author Vladimir Postnov
 * @date 6 Mar 2019
 * @brief
 * @addtogroup
 * @{
*/

#ifndef USB_CONF_H_
#define USB_CONF_H_

#include "usb_config.h"

#define USB_EP_ADDR__CTRL_OUT                       0x00U
#define USB_EP_SIZE__CTRL_OUT                       64

#define USB_EP_ADDR__CTRL_IN                        0x80U
#define USB_EP_SIZE__CTRL_IN                        USB_MAX_PACKET_SIZE

#define USB_EP_ADDR__VCP_DATA_OUT                   0x01U
#define USB_EP_SIZE__VCP_DATA_OUT                   APP_USB_RX_BUF_SIZE

#define USB_EP_ADDR__VCP_DATA_IN                    0x81U
#define USB_EP_SIZE__VCP_DATA_IN                    APP_USB_TX_BUF_SIZE

#define USB_EP_ADDR__VCP_INT_IN                     0x82U
#define USB_EP_SIZE__VCP_INT_IN                     VCP_INT_BUF_SIZE

#ifndef CDC_HS_BINTERVAL
  #define CDC_HS_BINTERVAL                          0x10U
#endif /* CDC_HS_BINTERVAL */

#ifndef CDC_FS_BINTERVAL
  #define CDC_FS_BINTERVAL                          0x10U
#endif /* CDC_FS_BINTERVAL */

#define RX_FIFO_FS_SIZE                             USB_EP_SIZE__CTRL_OUT
#define TX0_FIFO_FS_SIZE                            USB_EP_SIZE__CTRL_IN
#define TX1_FIFO_FS_SIZE                            USB_EP_SIZE__VCP_DATA_IN
#define TX2_FIFO_FS_SIZE                            USB_EP_SIZE__VCP_INT_IN
#define TX3_FIFO_FS_SIZE                            0

#endif /* USB_CONF_H_ */

/** @} */
