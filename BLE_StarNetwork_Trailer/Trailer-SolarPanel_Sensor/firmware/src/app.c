// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "app.h"
#include "definitions.h"
#include "app_ble.h"
#include "ble_dis/ble_dis.h"
#include "app_ble_conn_handler.h"
#include "app_ble_sensor.h"
#include "app_adv.h"
#include "system/console/sys_console.h"
#include "ble_otaps/ble_otaps.h"
#include "app_ota/app_ota_handler.h"
#include "app_timer/app_timer.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
#define BLE_SENSOR_VERSION  "1.0.0.0"



// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************





// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

float SP_Volt = 0.00;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

void adchs_ch1_callback(ADCHS_CHANNEL_NUM channel,uintptr_t context)
{
    uint16_t solar;
    SP_Volt = ((ADCHS_ChannelResultGet(ADCHS_CH5)*3.3)/4095)*3.0;      //voltage = (ADC value * reference voltage) / 4095     
    SYS_CONSOLE_PRINT(" SP Channel 1 PB1:%.2f Volt\r\n",SP_Volt);
    extern APP_TRPS_SolarData_T solar_volt;  
    solar= (uint8_t)(SP_Volt * 100);
    solar_volt.lsb=solar & 0x00ff;
    solar_volt.msb=solar>>8;
    
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/
uint32_t wbz451_silicon_revision = 0x00;

static void APP_Init(void)
{
   
    APP_BleStackInit();

    /* Add BLE Service */
    BLE_DIS_Add();

    APP_UpdateLocalName(0, NULL);
    APP_InitConnList();
    APP_ADV_Init();
    
    SYS_CONSOLE_MESSAGE("BLE Sensor Application: Version ");
    SYS_CONSOLE_PRINT(BLE_SENSOR_VERSION);
    SYS_CONSOLE_MESSAGE("\n\r[BLE} Advertisement Started\n\r");    

    APP_TRPS_Sensor_Init();
    
    APP_OTA_HDL_Init();
    
    wbz451_silicon_revision = 	DSU_REGS->DSU_DID;	
    SYS_CONSOLE_PRINT("\n\r[Device DID] 0x%x  \n\r", (DSU_REGS->DSU_DID)); 
    
    if(wbz451_silicon_revision & (1 << 29)) // A2 Silicon // if((wbz451_silicon_revision >> 28) == (0x02))
    {  
          /* PPS Output Remapping */
      PPS_REGS->PPS_RPB0G1R = 11U;
      PPS_REGS->PPS_RPB3G4R = 12U;
      PPS_REGS->PPS_RPB5G3R = 11U;
    }
    else if((wbz451_silicon_revision >> 28) ==  (0x00)) // A0 silicon
    {
      /* PPS Output Remapping */
      PPS_REGS->PPS_RPB0G1R = 21U;
      PPS_REGS->PPS_RPB3G4R = 21U;
      PPS_REGS->PPS_RPB5G3R = 22U;
    }	

}


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;


    appData.appQueue = xQueueCreate( 64, sizeof(APP_Msg_T) );
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    APP_Msg_T    appMsg[1];
    APP_Msg_T   *p_appMsg;
    p_appMsg=appMsg;

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            //appData.appQueue = xQueueCreate( 10, sizeof(APP_Msg_T) );

            APP_Init();
            RTC_Timer32Start();
            ADCHS_CallbackRegister(ADCHS_CH5,adchs_ch1_callback,0);
            APP_TIMER_SetTimer(APP_TIMER_SOLAR_PANEL, APP_TIMER_1S, true);
            APP_TIMER_SetTimer(APP_TIMER_SEND_BLE_DATA, APP_TIMER_5S, true);
            if (appInitialized)
            {

                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            if (OSAL_QUEUE_Receive(&appData.appQueue, &appMsg, OSAL_WAIT_FOREVER))
            {
                if(p_appMsg->msgId==APP_MSG_BLE_STACK_EVT)
                {
                    // Pass BLE Stack Event Message to User Application for handling
                    APP_BleStackEvtHandler((STACK_Event_T *)p_appMsg->msgData);
                }
                else if(p_appMsg->msgId== APP_TIMER_ADV_CTRL_MSG)
                {
                    APP_BLE_Adv_TimerHandler();
                }
                else if(p_appMsg->msgId== APP_TIMER_BLE_SENSOR_MSG)
                {
                    APP_TRPS_Sensor_TimerHandler();
                }
                else if(p_appMsg->msgId== APP_TIMER_SOLAR_PANEL_MSG)
                {
                    ADCHS_ChannelConversionStart(ADCHS_CH5);
                }
                else if(p_appMsg->msgId== APP_MSG_TRS_BLE_SENSOR_INT)
                {
                    APP_TRPS_Sensor_Button_Handler(); 
                }    
                else if(p_appMsg->msgId== APP_TIMER_SEND_BLE_MSG)
                {
                    if(APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_BLE_SMART_TRUCK ,SOLAR_VOLT_NFY)== MBA_RES_SUCCESS)
                    {
                        SYS_CONSOLE_PRINT("Sent\r\n");
                    }
                    else
                    {
                        SYS_CONSOLE_PRINT("Failed\r\n");
                    }
                } 
                else if(p_appMsg->msgId == APP_TIMER_OTA_TIMEOUT_MSG)
                {
                    APP_OTA_Timeout_Handler(); 
                }
                else if(p_appMsg->msgId== APP_TIMER_OTA_REBOOT_MSG)
                {
                    APP_OTA_Reboot_Handler(); 
                } 
            }
            break;
        }

        /* TODO: implement your application state machine.*/


        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
