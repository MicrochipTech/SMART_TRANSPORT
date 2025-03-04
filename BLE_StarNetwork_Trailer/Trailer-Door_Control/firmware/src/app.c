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
#ifdef APP_OTA_DFU_ENABLE
#include "app_dfu_handler.h"
#include "app_ble/app_otaps_handler.h"
#endif


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
#define BLE_SENSOR_VERSION  "0.1.0.5"



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
// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


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
#ifdef APP_OTA_DFU_ENABLE
    APP_OTA_HDL_Init();
#endif
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
            //appData.appQueue = xQueueCreate( APP_QUEUE_LENGTH, sizeof(APP_Msg_T) );

            APP_Init();
            if (appInitialized)
            {
                TCC1_PWMStart();
                TCC1_PWM24bitDutySet(TCC1_CHANNEL0,46);
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
                else if(p_appMsg->msgId== APP_MSG_TRS_BLE_SENSOR_INT)
                {
                    APP_TRPS_Sensor_Button_Handler(); 
                }
                else if(p_appMsg->msgId== APP_TIMER_ID_4_MSG)
                {
                    if(APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_BLE_SMART_TRUCK ,DOOR_STAT_REPORT_NFY))
                    {
                        SYS_CONSOLE_PRINT("[BLE] Sent PERIODIC DOOR data\r\n");
                    }
                }
#ifdef ENABLE_TOUCH
                else if(p_appMsg->msgId== APP_MSG_TOUCH_MEAS)
                {
                    APP_TouchMeasurement();
                }
#endif
#ifdef APP_OTA_DFU_ENABLE
                else if (p_appMsg->msgId == APP_TIMER_BLE_DFU_TIMEOUT_MSG)
                {
                    if (APP_DFU_HDL_GetDfuMode() == APP_DFU_MODE_OTA)
                        APP_OTA_HDL_Timeout();
                }
                else if (p_appMsg->msgId == APP_TIMER_BLE_DFU_REBOOT_MSG)
                {
                    uint8_t count = PDS_GetPendingItemsCount();

                    if ((!count) && (APP_DFU_HDL_GetDfuMode() == APP_DFU_MODE_OTA))
                        APP_DFU_HDL_SoftwareReset();
                }
#endif
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
