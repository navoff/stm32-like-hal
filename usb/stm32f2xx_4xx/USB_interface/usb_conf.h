/**
  ******************************************************************************
  * @file    usb_conf.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   General low level driver configuration
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
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
#ifndef __USB_CONF__H__
#define __USB_CONF__H__

/* Includes ------------------------------------------------------------------*/

#include "config.h"
#include "usb_config.h"
#include "usb_regs.h"

/** @addtogroup USB_OTG_DRIVER
  * @{
  */
  
/** @defgroup USB_CONF
  * @brief USB low level driver configuration file
  * @{
  */ 

/** @defgroup USB_CONF_Exported_Defines
  * @{
  */ 

/* USB Core and PHY interface configuration.
   Tip: To avoid modifying these defines each time you need to change the USB
        configuration, you can declare the needed define in your toolchain
        compiler preprocessor.
   */
/****************** USB OTG FS PHY CONFIGURATION *******************************
*  The USB OTG FS Core supports one on-chip Full Speed PHY.
*  
*  The USE_EMBEDDED_PHY symbol is defined in the project compiler preprocessor 
*  when FS core is used.
*******************************************************************************/
#ifndef USE_USB_OTG_FS
 #define USE_USB_OTG_FS
#endif /* USE_USB_OTG_FS */

#ifdef USE_USB_OTG_FS 
 #define USB_OTG_FS_CORE
#endif

/****************** USB OTG HS PHY CONFIGURATION *******************************
*  The USB OTG HS Core supports two PHY interfaces:
*   (i)  An ULPI interface for the external High Speed PHY: the USB HS Core will 
*        operate in High speed mode
*   (ii) An on-chip Full Speed PHY: the USB HS Core will operate in Full speed mode
*
*  You can select the PHY to be used using one of these two defines:
*   (i)  USE_ULPI_PHY: if the USB OTG HS Core is to be used in High speed mode 
*   (ii) USE_EMBEDDED_PHY: if the USB OTG HS Core is to be used in Full speed mode
*
*  Notes: 
*   - The USE_ULPI_PHY symbol is defined in the project compiler preprocessor as 
*     default PHY when HS core is used.
*   - On STM322xG-EVAL and STM324xG-EVAL boards, only configuration(i) is available.
*     Configuration (ii) need a different hardware, for more details refer to your
*     STM32 device datasheet.
*******************************************************************************/

#define USB_EP_ADDR__CTRL_OUT                   EP0_OUT
#define USB_EP_SIZE__CTRL_OUT                   128

#define USB_EP_ADDR__CTRL_IN                    EP0_IN
#define USB_EP_SIZE__CTRL_IN                    USB_MAX_PACKET_SIZE

#define USB_EP_ADDR__VCP_DATA_OUT               EP1_OUT
#define USB_EP_SIZE__VCP_DATA_OUT               APP_USB_RX_BUF_SIZE

#define USB_EP_ADDR__VCP_DATA_IN                EP1_IN
#define USB_EP_SIZE__VCP_DATA_IN                APP_USB_TX_BUF_SIZE

#define USB_EP_ADDR__VCP_INT_IN                 EP2_IN
#define USB_EP_SIZE__VCP_INT_IN                 VCP_INT_BUF_SIZE

#define RX_FIFO_FS_SIZE                         USB_EP_SIZE__CTRL_OUT
#define TX0_FIFO_FS_SIZE                        USB_EP_SIZE__CTRL_IN
#define TX1_FIFO_FS_SIZE                        USB_EP_SIZE__VCP_DATA_IN
#define TX2_FIFO_FS_SIZE                        USB_EP_SIZE__VCP_INT_IN
#define TX3_FIFO_FS_SIZE                        0

#define USE_DEVICE_MODE

#define __ALIGN_BEGIN
#define __ALIGN_END

/* __packed keyword used to decrease the data type alignment to 1-byte */
#if defined (__CC_ARM)         /* ARM Compiler */
  #define __packed    __packed
#elif defined (__ICCARM__)     /* IAR Compiler */
  #define __packed    __packed
#elif defined   ( __GNUC__ )   /* GNU Compiler */                        
  #define __packed    __attribute__ ((__packed__))
#elif defined   (__TASKING__)  /* TASKING Compiler */
  #define __packed    __unaligned
#endif /* __CC_ARM */

/**
  * @}
  */ 


/** @defgroup USB_CONF_Exported_Types
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USB_CONF_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USB_CONF_Exported_Variables
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USB_CONF_Exported_FunctionsPrototype
  * @{
  */ 
/**
  * @}
  */ 


#endif //__USB_CONF__H__


/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

