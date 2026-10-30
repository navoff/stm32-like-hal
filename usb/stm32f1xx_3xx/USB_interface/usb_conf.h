/**
  ******************************************************************************
  * @file    usb_conf.h
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Virtual COM Port Demo configuration  header
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_CONF_H
#define __USB_CONF_H

#include "config.h"
#include "usb_config.h"
#include "usb_regs.h"



#define __ALIGN_BEGIN
#define __ALIGN_END

/*-------------------------------------------------------------*/
/* EP_NUM */
/* defines how many endpoints are used by the device */
/*-------------------------------------------------------------*/

#define EP_NUM                              5

#define BTABLE_ADDRESS                      0x00


#define USB_EP__CTRL                        ENDP0
#define USB_EP__HID                         ENDP1
#define USB_EP__VCP_DATA                    ENDP3
#define USB_EP__VCP_INT                     ENDP4

#define USB_EP_ADDR__HID_IN                 EP1_IN
#define USB_EP_ADDR__HID_OUT                EP1_OUT
#define USB_EP_ADDR__VCP_DATA_IN            EP3_IN
#define USB_EP_ADDR__VCP_DATA_OUT           EP3_OUT
#define USB_EP_ADDR__VCP_INT_IN             EP4_IN

#define USB_EP_SIZE__CTRL_OUT               USB_MAX_PACKET_SIZE
#define USB_EP_SIZE__CTRL_IN                USB_MAX_PACKET_SIZE
#define USB_EP_SIZE__HID_OUT                APP_USB_HID_RX_BUF_SIZE
#define USB_EP_SIZE__HID_IN                 APP_USB_HID_TX_BUF_SIZE
#define USB_EP_SIZE__VCP_DATA_OUT           APP_USB_RX_BUF_SIZE
#define USB_EP_SIZE__VCP_DATA_IN            APP_USB_TX_BUF_SIZE
#define USB_EP_SIZE__VCP_INT_IN             VCP_INT_BUF_SIZE

#define USB_EP_RXADDR__CTRL                 0x40
#define USB_EP_TXADDR__CTRL                 (USB_EP_RXADDR__CTRL + USB_EP_SIZE__CTRL_OUT)

#define USB_EP_RXADDR__HID                  (USB_EP_TXADDR__CTRL + USB_EP_SIZE__CTRL_IN)
#define USB_EP_TXADDR__HID                  (USB_EP_RXADDR__HID + USB_EP_SIZE__HID_OUT)

#define USB_EP_RXADDR__VCP_DATA_OUT         (USB_EP_TXADDR__HID + USB_EP_SIZE__HID_IN)
#define USB_EP_TXADDR__VCP_DATA_IN          (USB_EP_RXADDR__VCP_DATA_OUT + USB_EP_SIZE__VCP_DATA_OUT)
#define USB_EP_TXADDR__VCP_INT              (USB_EP_TXADDR__VCP_DATA_IN + USB_EP_SIZE__VCP_DATA_IN)



/*-------------------------------------------------------------*/
/* -------------------   ISTR events  -------------------------*/
/*-------------------------------------------------------------*/
/* IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */
#define IMR_MSK (CNTR_CTRM  | CNTR_SOFM | CNTR_WKUPM | CNTR_SUSPM | CNTR_RESETM | CNTR_ESOFM)

/*#define CTR_CALLBACK*/
/*#define DOVR_CALLBACK*/
/*#define ERR_CALLBACK*/
/*#define WKUP_CALLBACK*/
/*#define SUSP_CALLBACK*/
/*#define RESET_CALLBACK*/
#define SOF_CALLBACK
/*#define ESOF_CALLBACK*/
/* CTR service routines */
/* associated to defined endpoints */
#define  EP4_IN_Callback   NOP_Process
#define  EP5_IN_Callback   NOP_Process
#define  EP6_IN_Callback   NOP_Process
#define  EP7_IN_Callback   NOP_Process

#define  EP4_OUT_Callback   NOP_Process
#define  EP5_OUT_Callback   NOP_Process
#define  EP6_OUT_Callback   NOP_Process
#define  EP7_OUT_Callback   NOP_Process

#endif /* __USB_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
