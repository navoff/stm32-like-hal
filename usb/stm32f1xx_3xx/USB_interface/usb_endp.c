/**
  ******************************************************************************
  * @file    usb_endp.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Endpoint routines
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

#include "config.h"
#if (FEATURE_ENABLE_USB == 1)

#include "usb_lib.h"
#include "usb_mem.h"
#include "usb_istr.h"
#include "usb_endp.h"
#include "usb_prop.h"
#include "app_usb.h"


#if (USE_USB_HID_EN)
uint8_t timestamp_idx;
uint32_t timestamp_buf[16];
uint8_r rx_buffer[16];
uint32_t tx_report;
uint8_t PrevXferComplete;
#endif

void EP_HID_IN_Callback(void)
{
  #if (USE_USB_HID_EN)
  PrevXferComplete--;
  BL_START_MS_TIMER(timestamp_buf[timestamp_idx]);
  timestamp_idx++;
  if (timestamp_idx >= 16) timestamp_idx = 0;
  #endif
}

void EP_HID_OUT_Callback(void)
{
  #if (USE_USB_HID_EN)
  USB_SIL_Read(HID_EP_OUT_ADDR, rx_buffer);
  SetEPRxStatus(HID_EP_NUM, EP_RX_VALID);
  
  tx_report = 1;
  #endif
}

void EP_MS_IN_Callback(void)
{
  #if (USE_USB_MS_EN)
  Mass_Storage_In();
  #endif
}

void EP_MS_OUT_Callback(void)
{
  #if (USE_USB_MS_EN)
  Mass_Storage_Out();
  #endif
}

void EP_VCP_IN_Callback(void)
{
  #if (USE_USB_VCP_EN)
  app_usb_IN(0);
  #endif
}

void EP_VCP_OUT_Callback(void)
{
  #if (USE_USB_VCP_EN)
  app_usb_OUT(0);
  #endif
}

void SOF_Callback(void)
{
  if(bDeviceState == CONFIGURED)
  {
    #if (USE_USB_VCP_EN)
    app_usb_SOF(0);
    #endif
  }
}

#endif // (FEATURE_ENABLE_USB == 1)

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

