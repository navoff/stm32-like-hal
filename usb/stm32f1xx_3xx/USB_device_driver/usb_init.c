/**
  ******************************************************************************
  * @file    usb_init.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    28-August-2012
  * @brief   Initialization routines & global variables
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
#include "utils/assert.h"
#include "drivers/gpio.h"
#include "usb_lib.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/*  The number of current endpoint, it will be used to specify an endpoint */
uint8_t	EPindex;
/*  The number of current device, it is an index to the Device_Table */
/* uint8_t	Device_no; */
/*  Points to the DEVICE_INFO structure of current device */
/*  The purpose of this register is to speed up the execution */
DEVICE_INFO *pInformation;
/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */
DEVICE_PROP *pProperty;
/*  Temporary save the state of Rx & Tx status. */
/*  Whenever the Rx or Tx state is changed, its value is saved */
/*  in this variable first and will be set to the EPRB or EPRA */
/*  at the end of interrupt process */
uint16_t	SaveState ;
uint16_t  wInterrupt_Mask;
DEVICE_INFO	Device_Info;
USER_STANDARD_REQUESTS  *pUser_Standard_Requests;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : USB_Interrupts_Config
* Description    : Configures the USB interrupts
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Interrupts_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* 2 bit for pre-emption priority, 2 bits for subpriority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

    /* Enable the USB Wake-up interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_Init(&NVIC_InitStructure);
}

void USB_GPIO_Config(void)
{
  gpio_rcc_cmd(0, ENABLE);
  gpio_pin_set_mode(0, 11, GPIO_MODE_USB);
  gpio_pin_set_mode(0, 12, GPIO_MODE_USB);


  #if (FEATURE_ENABLE_USB && defined(STM32F1XX))
  #warning Need to test

  // Pull-up resistor control should go here
  gpio_pin_set_mode(0, 15, GPIO_MODE_OUTPUT_PP);
  gpio_pin_cmd(0, 15, ENABLE);
  #endif
}

void USB_Clk_Config(void)
{
  #if defined(STM32F30X)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  #endif
  // USBCLK = PLLCLK / 1.5
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
  // Enable USB clock
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

/*******************************************************************************
* Function Name  : USB_Init
* Description    : USB system initialization
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_Init(void)
{
  USB_Clk_Config();
  USB_GPIO_Config();

  pInformation = &Device_Info;
  pInformation->ControlState = 2;
  pProperty = (DEVICE_PROP*)&Device_Property;
  pUser_Standard_Requests = (USER_STANDARD_REQUESTS*)&User_Standard_Requests;
  /* Initialize devices one by one */
  pProperty->Init();

  // Configure the EXTI line 18 connected internally to the USB IP
  EXTI_ClearITPendingBit(EXTI_Line18);
  EXTI_InitTypeDef EXTI_InitStructure;
  EXTI_InitStructure.EXTI_Line = EXTI_Line18;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  USB_Interrupts_Config();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
