/**
  ******************************************************************************
  * @file    usbd_cdc_core.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides the high layer firmware functions to manage the 
  *          following functionalities of the USB CDC Class:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as CDC Device (and enumeration for each implemented memory interface)
  *           - OUT/IN data transfer
  *           - Command IN transfer (class requests management)
  *           - Error management
  *           
  *  @verbatim
  *      
  *          ===================================================================      
  *                                CDC Class Driver Description
  *          =================================================================== 
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus 
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as CDC device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class

  *           @note
  *             For the Abstract Control Model, this core allows only transmitting the requests to
  *             lower layer dispatcher (ie. usbd_cdc_vcp.c/.h) which should manage each request and
  *             perform relative actions.
  * 
  *           These aspects may be enriched or modified for a specific user application.
  *          
  *            This driver doesn't implement the following aspects of the specification 
  *            (but it is possible to manage these features with some modifications on this driver):
  *             - Any class-specific aspect relative to communication classes should be managed by user application.
  *             - All communication classes other than PSTN are not managed
  *      
  *  @endverbatim
  *                                  
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

/* Includes ------------------------------------------------------------------*/
#include "config.h"

#if (FEATURE_ENABLE_USB == 1)

#include "usbd_cdc_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usbd_def.h"

#include "app_usb.h"
#include "usb_desc_device.h"

#pragma pack(1)
typedef enum
{
  SOF,
  PrepareRxFromSOF,
  RxLockedFromSOF,
  DataOut,
  Overflow,
  PrepareRxFromDataOut,
  RxLockedFromDataOut,
  ErrorSync,
  TxPacket,
  TxRemainLen,
} test_event_id_t;

typedef struct
{
  test_event_id_t id;
  uint16_t param;
  uint16_t tick;
} test_event_t;
#pragma pack()


/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup usbd_cdc 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup usbd_cdc_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup usbd_cdc_Private_Defines
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup usbd_cdc_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup usbd_cdc_Private_FunctionPrototypes
  * @{
  */

/*********************************************
   CDC Device library callbacks
 *********************************************/
static uint8_t  usbd_cdc_Init        (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_cdc_DeInit      (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_cdc_Setup       (void  *pdev, USB_SETUP_REQ *req);
static uint8_t  usbd_cdc_EP0_RxReady  (void *pdev);
static uint8_t  usbd_cdc_DataIn      (void *pdev, uint8_t epnum);
static uint8_t  usbd_cdc_DataOut     (void *pdev, uint8_t epnum);
static uint8_t  usbd_cdc_SOF         (void *pdev);

/*********************************************
   CDC specific management functions
 *********************************************/
static uint8_t  *USBD_cdc_GetCfgDesc (uint8_t speed, uint16_t *length);
#ifdef USE_USB_OTG_HS  
static uint8_t  *USBD_cdc_GetOtherCfgDesc (uint8_t speed, uint16_t *length);
#endif
/**
  * @}
  */ 

/** @defgroup usbd_cdc_Private_Variables
  * @{
  */ 
extern const CDC_IF_Prop_TypeDef  APP_FOPS;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN static __IO uint32_t  usbd_cdc_AltSet  __ALIGN_END;

__ALIGN_BEGIN uint8_t CmdBuff[USB_EP_SIZE__VCP_INT_IN] __ALIGN_END ;

extern uint8_t app_usb_rx_buf[];

uint32_t cdcCmd;
uint32_t cdcLen;

/* CDC interface class callbacks structure */
const USBD_Class_cb_TypeDef  USBD_CDC_cb =
{
  usbd_cdc_Init,
  usbd_cdc_DeInit,
  usbd_cdc_Setup,
  NULL,                 /* EP0_TxSent, */
  usbd_cdc_EP0_RxReady,
  usbd_cdc_DataIn,
  usbd_cdc_DataOut,
  usbd_cdc_SOF,
  NULL,
  NULL,     
  USBD_cdc_GetCfgDesc,
#ifdef USE_USB_OTG_HS   
  USBD_cdc_GetOtherCfgDesc, /* use same cobfig as per FS */
#endif /* USE_USB_OTG_HS  */
};

/**
  * @}
  */ 

/** @defgroup usbd_cdc_Private_Functions
  * @{
  */ 

/**
  * @brief  usbd_cdc_Init
  *         Initilaize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_cdc_Init (void  *pdev, 
                               uint8_t cfgidx)
{
  cdcCmd = NO_CMD;

  /* Open EP IN */
  DCD_EP_Open(pdev, USB_EP_ADDR__VCP_DATA_IN, USB_EP_SIZE__VCP_DATA_IN, USB_OTG_EP_BULK);
  
  /* Open EP OUT */
  DCD_EP_Open(pdev, USB_EP_ADDR__VCP_DATA_OUT, USB_EP_SIZE__VCP_DATA_OUT, USB_OTG_EP_BULK);
  
  /* Open Command IN EP */
  DCD_EP_Open(pdev, USB_EP_ADDR__VCP_INT_IN, USB_EP_SIZE__VCP_INT_IN, USB_OTG_EP_INT);
  
  /* Initialize the Interface physical components */
  APP_FOPS.pIf_Init();

  /* Prepare Out endpoint to receive next packet */
  DCD_EP_PrepareRx(pdev, USB_EP_ADDR__VCP_DATA_OUT, app_usb_rx_buf, USB_EP_SIZE__VCP_DATA_OUT);
  
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_cdc_DeInit (void  *pdev, 
                                 uint8_t cfgidx)
{
  /* Open EP IN */
  DCD_EP_Close(pdev, USB_EP_ADDR__VCP_DATA_IN);
  
  /* Open EP OUT */
  DCD_EP_Close(pdev, USB_EP_ADDR__VCP_DATA_OUT);
  
  /* Open Command IN EP */
  DCD_EP_Close(pdev, USB_EP_ADDR__VCP_INT_IN);

  /* Restore default state of the Interface physical components */
  APP_FOPS.pIf_DeInit();
  
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  usbd_cdc_Setup (void  *pdev, 
                                USB_SETUP_REQ *req)
{
  uint16_t len = VCP_CONFIG_DESCRIPTOR_SIZE;
  uint8_t  *pbuf = (uint8_t *)DeviceConfigDescriptor + 9;
  
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    /* CDC Class Requests -------------------------------*/
  case USB_REQ_TYPE_CLASS :
      /* Check if the request is a data setup packet */
      if (req->wLength)
      {
        /* Check if the request is Device-to-Host */
        if (req->bmRequest & 0x80)
        {
          /* Get the data to be sent to Host from interface layer */
          APP_FOPS.pIf_Ctrl(req->bRequest, CmdBuff, req->wLength);
          
          /* Send the data to the host */
          USBD_CtlSendData (pdev, CmdBuff, req->wLength);
        }
        else /* Host-to-Device requeset */
        {
          /* Set the value of the current command to be processed */
          cdcCmd = req->bRequest;
          cdcLen = req->wLength;
          
          /* Prepare the reception of the buffer over EP0
          Next step: the received data will be managed in usbd_cdc_EP0_TxSent() 
          function. */
          USBD_CtlPrepareRx (pdev, CmdBuff, req->wLength);
        }
      }
      else /* No Data request */
      {
        /* Transfer the command to the interface layer */
        APP_FOPS.pIf_Ctrl(req->bRequest, NULL, 0);
      }
      
      return USBD_OK;
      
    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL;
    
      
      
    /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR: 
      if( (req->wValue >> 8) == CDC_DESCRIPTOR_TYPE)
      {
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        pbuf = (uint8_t *)usbd_cdc_Desc;   
#else
        pbuf = (uint8_t *)DeviceConfigDescriptor + 9 + (9 * USBD_ITF_MAX_NUM);
#endif 
        len = MIN(VCP_CONFIG_DESCRIPTOR_SIZE , req->wLength);
      }
      
      USBD_CtlSendData(pdev, pbuf, len);
      break;
      
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData(pdev, (uint8_t *)&usbd_cdc_AltSet, 1);
      break;
      
    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
      {
        usbd_cdc_AltSet = (uint8_t)(req->wValue);
      }
      else
      {
        /* Call the error management function (command will be nacked */
        USBD_CtlError (pdev, req);
      }
      break;
    }
  }
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_EP0_RxReady
  *         Data received on control endpoint
  * @param  pdev: device device instance
  * @retval status
  */
static uint8_t  usbd_cdc_EP0_RxReady (void  *pdev)
{ 
  if (cdcCmd != NO_CMD)
  {
    /* Process the data */
    APP_FOPS.pIf_Ctrl(cdcCmd, CmdBuff, cdcLen);
    
    /* Reset the command variable to default value */
    cdcCmd = NO_CMD;
  }
  
  return USBD_OK;
}

/**
  * @brief  usbd_audio_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_cdc_DataIn (void *pdev, uint8_t epnum)
{
  app_usb_IN(pdev);
  return USBD_OK;
}

/**
  * @brief  usbd_cdc_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_cdc_DataOut (void *pdev, uint8_t epnum)
{
  app_usb_OUT(pdev);
  return USBD_OK;
}

/**
  * @brief  usbd_audio_SOF
  *         Start Of Frame event management
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_cdc_SOF (void *pdev)
{
  app_usb_SOF(pdev);
  return USBD_OK;
}

/**
  * @brief  USBD_cdc_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_cdc_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (DeviceConfigDescriptor);
  return (uint8_t *)DeviceConfigDescriptor;
}

/**
  * @brief  USBD_cdc_GetCfgDesc 
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
#ifdef USE_USB_OTG_HS 
static uint8_t  *USBD_cdc_GetOtherCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_cdc_OtherCfgDesc);
  return usbd_cdc_OtherCfgDesc;
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

#endif // (FEATURE_ENABLE_USB == 1)

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
