/**
  ******************************************************************************
  * @file    usbd_cdc.c
  * @author  MCD Application Team
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
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

  /* BSPDependencies
  - "stm32xxxxx_{eval}{discovery}{nucleo_144}.c"
  - "stm32xxxxx_{eval}{discovery}_io.c"
  EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "app_usb.h"
#include "usbd_cdc.h"
#include "usbd_ctlreq.h"
#include "usb_config.h"
#include "usb_desc_device.h"
#include "usb_types.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_CDC
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_CDC_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_CDC_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_CDC_Private_Macros
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_CDC_Private_FunctionPrototypes
  * @{
  */


static uint8_t  USBD_CDC_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

static uint8_t  USBD_CDC_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

static uint8_t  USBD_CDC_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

static uint8_t  USBD_CDC_DataIn (USBD_HandleTypeDef *pdev,
                                 uint8_t epnum);

static uint8_t  USBD_CDC_DataOut (USBD_HandleTypeDef *pdev,
                                 uint8_t epnum);

static uint8_t  USBD_CDC_SOF (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev);

static const uint8_t  *USBD_CDC_GetCfgDesc (uint16_t *length);

const uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor (uint16_t *length);

/**
  * @}
  */

/** @defgroup USBD_CDC_Private_Variables
  * @{
  */


/* CDC interface class callbacks structure */
USBD_ClassTypeDef  USBD_CDC =
{
  USBD_CDC_Init,
  USBD_CDC_DeInit,
  USBD_CDC_Setup,
  NULL,                 /* EP0_TxSent, */
  USBD_CDC_EP0_RxReady,
  USBD_CDC_DataIn,
  USBD_CDC_DataOut,
  USBD_CDC_SOF,
  NULL,
  NULL,
  USBD_CDC_GetCfgDesc,
  USBD_CDC_GetCfgDesc,
  USBD_CDC_GetCfgDesc,
  USBD_CDC_GetDeviceQualifierDescriptor,
};

USBD_CDC_HandleTypeDef USBD_CDC_Handle;

/**
  * @}
  */

/** @defgroup USBD_CDC_Private_Functions
  * @{
  */

/**
  * @brief  USBD_CDC_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0U;
  USBD_CDC_HandleTypeDef   *hcdc;

  /* Open EP IN */
  USBD_LL_OpenEP(pdev, USB_EP_ADDR__VCP_DATA_IN, USB_EP_TYPE__BULK, USB_EP_SIZE__VCP_DATA_IN);
  pdev->ep_in[USB_EP_ADDR__VCP_DATA_IN & 0xFU].is_used = 1U;

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev, USB_EP_ADDR__VCP_DATA_OUT, USB_EP_TYPE__BULK, USB_EP_SIZE__VCP_DATA_OUT);
  pdev->ep_out[USB_EP_ADDR__VCP_DATA_OUT & 0xFU].is_used = 1U;

  /* Open Command IN EP */
  USBD_LL_OpenEP(pdev, USB_EP_ADDR__VCP_INT_IN, USB_EP_TYPE__INTERRUPT, USB_EP_SIZE__VCP_INT_IN);
  pdev->ep_in[USB_EP_ADDR__VCP_INT_IN & 0xFU].is_used = 1U;

  pdev->pClassData = &USBD_CDC_Handle;

  if(pdev->pClassData == NULL)
  {
    ret = 1U;
  }
  else
  {
    hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

    /* Init  physical Interface components */
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Init();

    /* Init Xfer states */
    hcdc->TxState = 0U;
    hcdc->RxState = 0U;
  }
  return ret;
}

/**
  * @brief  USBD_CDC_Init
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_CDC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0U;

  /* Close EP IN */
  USBD_LL_CloseEP(pdev, USB_EP_ADDR__VCP_DATA_IN);
  pdev->ep_in[USB_EP_ADDR__VCP_DATA_IN & 0xFU].is_used = 0U;

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, USB_EP_ADDR__VCP_DATA_OUT);
  pdev->ep_out[USB_EP_ADDR__VCP_DATA_OUT & 0xFU].is_used = 0U;

  /* Close Command IN EP */
  USBD_LL_CloseEP(pdev, USB_EP_ADDR__VCP_INT_IN);
  pdev->ep_in[USB_EP_ADDR__VCP_INT_IN & 0xFU].is_used = 0U;

  /* DeInit  physical Interface components */
  if(pdev->pClassData != NULL)
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->DeInit();
    pdev->pClassData = NULL;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_Setup
  *         Handle the CDC specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_CDC_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    if (req->wLength)
    {
      if (req->bmRequest & 0x80U)
      {
        ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
                                                          (uint8_t *)(void *)hcdc->data,
                                                          req->wLength);

          USBD_CtlSendData (pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
      }
      else
      {
        hcdc->CmdOpCode = req->bRequest;
        hcdc->CmdLength = (uint8_t)req->wLength;

        USBD_CtlPrepareRx (pdev, (uint8_t *)(void *)hcdc->data, req->wLength);
      }
    }
    else
    {
      ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(req->bRequest,
                                                        (uint8_t *)(void *)req, 0U);
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, (uint8_t *)(void *)&status_info, 2U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED)
      {
        USBD_CtlSendData (pdev, &ifalt, 1U);
      }
      else
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE:
      if (pdev->dev_state != USBD_STATE_CONFIGURED)
      {
        USBD_CtlError (pdev, req);
			  ret = USBD_FAIL;
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
      break;
    }
    break;

  default:
    USBD_CtlError (pdev, req);
    ret = USBD_FAIL;
    break;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  app_usb_IN(pdev);
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  USBD_CDC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  app_usb_OUT(pdev);
  return USBD_OK;
}

static uint8_t  USBD_CDC_SOF (USBD_HandleTypeDef *pdev)
{
  app_usb_SOF(pdev);
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_EP0_RxReady
  *         Handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_CDC_EP0_RxReady (USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef   *hcdc = (USBD_CDC_HandleTypeDef*) pdev->pClassData;

  if((pdev->pUserData != NULL) && (hcdc->CmdOpCode != 0xFFU))
  {
    ((USBD_CDC_ItfTypeDef *)pdev->pUserData)->Control(hcdc->CmdOpCode,
                                                      (uint8_t *)(void *)hcdc->data,
                                                      (uint16_t)hcdc->CmdLength);
      hcdc->CmdOpCode = 0xFFU;

  }
  return USBD_OK;
}

/**
  * @brief  USBD_CDC_GetCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static const uint8_t *USBD_CDC_GetCfgDesc (uint16_t *length)
{
  *length = sizeof (DeviceConfigDescriptor);
  return DeviceConfigDescriptor;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
const uint8_t  *USBD_CDC_GetDeviceQualifierDescriptor (uint16_t *length)
{
  *length = sizeof (DeviceQualifierDescription);
  return DeviceQualifierDescription;
}

/**
* @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CD  Interface callback
  * @retval status
  */
uint8_t  USBD_CDC_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                      USBD_CDC_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if(fops != NULL)
  {
    pdev->pUserData= fops;
    ret = USBD_OK;
  }

  return ret;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
