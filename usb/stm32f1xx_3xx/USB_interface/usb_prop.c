/**
  ******************************************************************************
  * @file    usb_prop.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   All processing related to Virtual Com Port Demo
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

/* Includes ------------------------------------------------------------------*/
#include "usb_types.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc_device.h"

RESULT Device_Data_Setup(uint8_t);
RESULT Device_NoData_Setup(uint8_t);
RESULT Device_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *Device_GetDeviceDescriptor(uint16_t );
uint8_t *Device_GetConfigDescriptor(uint16_t);
uint8_t *Device_GetStringDescriptor(uint16_t);

uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length);
uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length);

uint8_t *HID_GetReportDescriptor(uint16_t Length);
uint8_t *HID_GetHIDDescriptor(uint16_t Length);
void HID_SetProtocol(void);
uint8_t *HID_GetProtocolValue(uint16_t Length);
uint8_t *HID_SetReport_Feature(uint16_t Length);

LINE_CODING linecoding;
__IO uint32_t bDeviceState;
uint32_t ProtocolValue;
uint8_t Report_Buf[2];

/* -------------------------------------------------------------------------- */
/*  Structures initializations */
/* -------------------------------------------------------------------------- */

const DEVICE Device_Table =
{
    EP_NUM,
    1
};

const DEVICE_PROP Device_Property =
{
    Device_Init,
    Device_Reset,
    Device_Status_In,
    Device_Status_Out,
    Device_Data_Setup,
    Device_NoData_Setup,
    Device_Get_Interface_Setting,
    Device_GetDeviceDescriptor,
    Device_GetConfigDescriptor,
    Device_GetStringDescriptor,
    0,
    USB_MAX_PACKET_SIZE,
};

const USER_STANDARD_REQUESTS User_Standard_Requests =
{
    Device_GetConfiguration,
    Device_SetConfiguration,
    Device_GetInterface,
    Device_SetInterface,
    Device_GetStatus,
    Device_ClearFeature,
    Device_SetEndPointFeature,
    Device_SetDeviceFeature,
    Device_SetDeviceAddress,
};

const ONE_DESCRIPTOR Device_Descriptor =
{
    (uint8_t*) DeviceDescriptor,
    DEVICE_DESCRIPTOR_SIZE
};

const ONE_DESCRIPTOR Config_Descriptor =
{
    (uint8_t*) DeviceConfigDescriptor,
    DEVICE_CONFIG_DESCRIPTOR_SIZE
};

const ONE_DESCRIPTOR HID_Report_Descriptor =
{
    (uint8_t *)HID_ReportDescriptor,
    HID_REPORT_DESCRIPTOR_SIZE
};

const ONE_DESCRIPTOR HID_Hid_Descriptor =
{
    (uint8_t*)DeviceConfigDescriptor + HID_DESCRIPTOR_OFFSET,
    HID_DESCRIPTOR_SIZE
};

const ONE_DESCRIPTOR String_Descriptor[4] =
{
    {(uint8_t*)Device_StringLangID, DEVICE_STRING_LANDID_SIZE},
    {(uint8_t*)Device_StringVendor, DEVICE_STRING_VENDOR_SIZE},
    {(uint8_t*)Device_StringProduct, DEVICE_STRING_PRODUCT_SIZE},
    {(uint8_t*)Device_StringSerial, DEVICE_STRING_SERIAL_SIZE}
};

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : Virtual_Com_Port_init.
* Description    : Virtual COM Port Mouse init routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_Init(void)
{
  pInformation->Current_Configuration = 0;

  /* Perform basic device initialization operations */
  USB_SIL_Init();

  bDeviceState = UNCONNECTED;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_Reset
* Description    : Virtual_Com_Port Mouse reset routine
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_Reset(void)
{
  /* Set Virtual_Com_Port DEVICE as not configured */
  pInformation->Current_Configuration = 0;

  /* Current Feature initialization */
  pInformation->Current_Feature = DeviceConfigDescriptor[7];

  /* Set Virtual_Com_Port DEVICE with the default Interface*/
  pInformation->Current_Interface = 0;

  SetBTABLE(BTABLE_ADDRESS);

  SetEPType(USB_EP__CTRL, EP_CONTROL);
  SetEPTxStatus(USB_EP__CTRL, EP_TX_STALL);
  SetEPRxAddr(USB_EP__CTRL, USB_EP_RXADDR__CTRL);
  SetEPTxAddr(USB_EP__CTRL, USB_EP_TXADDR__CTRL);
  Clear_Status_Out(USB_EP__CTRL);
  SetEPRxCount(USB_EP__CTRL, Device_Property.MaxPacketSize);
  SetEPRxValid(USB_EP__CTRL);

  #if (USE_USB_HID_EN)
  SetEPType(HID_EP_NUM, EP_INTERRUPT);
  SetEPTxAddr(HID_EP_NUM, USB_EP_TXADDR__HID);
  SetEPRxAddr(HID_EP_NUM, USB_EP_RXADDR__HID);
  SetEPTxCount(HID_EP_NUM, APP_HID_TX_BUF_SIZE);
  SetEPRxCount(HID_EP_NUM, APP_HID_RX_BUF_SIZE);
  SetEPTxStatus(HID_EP_NUM, EP_TX_NAK);
  SetEPRxStatus(HID_EP_NUM, EP_RX_VALID);
  #endif

  #if (USE_USB_VCP_EN)
  SetEPType(USB_EP__VCP_INT, EP_INTERRUPT);
  SetEPTxAddr(USB_EP__VCP_INT, USB_EP_TXADDR__VCP_INT);
  SetEPRxStatus(USB_EP__VCP_INT, EP_RX_DIS);
  SetEPTxStatus(USB_EP__VCP_INT, EP_TX_NAK);

  SetEPType(USB_EP__VCP_DATA, EP_BULK);
  SetEPTxAddr(USB_EP__VCP_DATA, USB_EP_TXADDR__VCP_DATA_IN);
  SetEPRxAddr(USB_EP__VCP_DATA, USB_EP_RXADDR__VCP_DATA_OUT);
  SetEPRxCount(USB_EP__VCP_DATA, APP_USB_RX_BUF_SIZE);

  SetEPTxStatus(USB_EP__VCP_DATA, EP_TX_NAK);
  SetEPRxStatus(USB_EP__VCP_DATA, EP_RX_VALID);
  #endif

  /* Set this device to response on default address */
  SetDeviceAddress(0);
  
  bDeviceState = ATTACHED;
}

void Device_SetConfiguration(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  if (pInfo->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED;
  }
}

void Device_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}

void Device_Status_In(void)
{

}

void Device_Status_Out(void)
{

}

RESULT Device_Data_Setup(uint8_t RequestNo)
{
  uint8_t *(*CopyRoutine)(uint16_t);

  CopyRoutine = NULL;

  if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
  {
    if (RequestNo == GET_DESCRIPTOR)
    {
      if (pInformation->USBwValue1 == USB_DESCRIPTOR_TYPE__HID_REPORT)
      {
        CopyRoutine = HID_GetReportDescriptor;
      }
      else if (pInformation->USBwValue1 == USB_DESCRIPTOR_TYPE__HID)
      {
        CopyRoutine = HID_GetHIDDescriptor;
      }
    }
  }
  else if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
  {
    if (RequestNo == HID_REQUEST__GET_PROTOCOL)
    {
      CopyRoutine = HID_GetProtocolValue;
    }
    else if (RequestNo == HID_REQUEST__SET_REPORT)
    {
      CopyRoutine = HID_SetReport_Feature;
    }

    if (RequestNo == GET_LINE_CODING)
    {
      CopyRoutine = Virtual_Com_Port_GetLineCoding;
    }
    else if (RequestNo == SET_LINE_CODING)
    {
      CopyRoutine = Virtual_Com_Port_SetLineCoding;
    }
  }

  if (CopyRoutine == NULL)
  {
    return USB_UNSUPPORT;
  }

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);
  return USB_SUCCESS;
}

RESULT Device_NoData_Setup(uint8_t RequestNo)
{
  if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
  {
    if (RequestNo == HID_REQUEST__SET_PROTOCOL)
    {
      HID_SetProtocol();
      return USB_SUCCESS;
    }

    if (RequestNo == SET_COMM_FEATURE)
    {
      return USB_SUCCESS;
    }
    else if (RequestNo == SET_CONTROL_LINE_STATE)
    {
      return USB_SUCCESS;
    }
  }

  return USB_UNSUPPORT;
}

uint8_t *Device_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, (PONE_DESCRIPTOR)&Device_Descriptor);
}

uint8_t *Device_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, (PONE_DESCRIPTOR)&Config_Descriptor);
}

uint8_t *Device_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  if (wValue0 > 4)
  {
    return NULL;
  }
  else
  {
    return Standard_GetDescriptorData(Length, (PONE_DESCRIPTOR)&String_Descriptor[wValue0]);
  }
}

RESULT Device_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
  if (AlternateSetting > 0)
  {
    return USB_UNSUPPORT;
  }
  else if (Interface > 1)
  {
    return USB_UNSUPPORT;
  }
  return USB_SUCCESS;
}

uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}

uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}

uint8_t *HID_GetReportDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, (ONE_DESCRIPTOR *)&HID_Report_Descriptor);
}

uint8_t *HID_GetHIDDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, (ONE_DESCRIPTOR *)&HID_Hid_Descriptor);
}

void HID_SetProtocol(void)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  ProtocolValue = wValue0;
}

uint8_t *HID_GetProtocolValue(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = 1;
    return NULL;
  }
  else
  {
    return (uint8_t *)(&ProtocolValue);
  }
}

uint8_t *HID_SetReport_Feature(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = 2;
    return NULL;
  }
  else
  {
    return Report_Buf;
  }
}

#endif // (FEATURE_ENABLE_USB == 1)

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

