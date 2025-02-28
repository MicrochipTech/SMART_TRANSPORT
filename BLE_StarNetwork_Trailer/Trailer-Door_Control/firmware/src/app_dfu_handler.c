/*******************************************************************************
  Application DFU Handler Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_dfu_handler.c

  Summary:
    This file contains the Application DFU handler for this project.

  Description:
    This file contains the Application DFU handler for this project.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
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


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "app.h"
#include "app_adv.h"
#include "app_dfu_handler.h"
#include "app_ble_conn_handler.h"
#include "app_error_defs.h"
#include "app_trps.h"
#include "app_timer/app_timer.h"
#include "peripheral/rcon/plib_rcon.h"
#include "ble_gap.h"
#include "ble_dm/ble_dm.h"


#ifdef APP_OTA_DFU_ENABLE
// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static uint8_t s_dfuMode;

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************

void APP_DFU_HDL_SetDfuMode(APP_DFU_HDL_Mode_T mode)
{
    s_dfuMode = mode;
}

APP_DFU_HDL_Mode_T APP_DFU_HDL_GetDfuMode(void)
{
    return s_dfuMode;
}

void APP_DFU_HDL_Prepare(uint16_t dfuHandle)
{
    uint8_t i;
    uint16_t connHandle;
    APP_BLE_ConnList_T *p_connList = NULL;
    APP_BLE_ConnList_T *p_dfuConnLink = NULL;
    BLE_DM_ConnParamUpdate_T params;

    APP_ADV_Stop();

    p_dfuConnLink = APP_GetConnInfoByConnHandle(dfuHandle);

    if (p_dfuConnLink == NULL)
        return;

    //Disconnect all others except proceeding DFU one
    for (i=0; i< BLE_GAP_MAX_LINK_NBR; i++)
    {
        connHandle = APP_GetConnHandleByIndex(i);
        if (connHandle == 0xFFFF)
            continue;
        
        p_connList = APP_GetConnInfoByConnHandle(connHandle);
        if (p_connList == NULL)
            continue;
    
        if (p_connList->linkState >= APP_BLE_STATE_CONNECTED && 
            dfuHandle != connHandle)
        {
            BLE_GAP_Disconnect(connHandle, GAP_DISC_REASON_REMOTE_TERMINATE);
        }
    }

    if (dfuHandle)
    {
        //speed up transmission
        params.intervalMin = 0x08;  //10ms
        params.intervalMax = 0x10;  //20ms
        params.latency = 0;
        params.timeout = 0xC8;//2000ms
        BLE_DM_ConnectionParameterUpdate(dfuHandle, &params);
    }

    APP_TIMER_SetTimer(APP_TIMER_DFU_TIMEOUT, APP_TIMER_30S, false); 
}

void APP_DFU_HDL_Start(void)
{
    APP_TIMER_StopTimer(APP_TIMER_DFU_TIMEOUT);
    APP_TIMER_SetTimer(APP_TIMER_DFU_TIMEOUT, APP_TIMER_5S, false);
}

void APP_DFU_HDL_Updating(void)
{
    APP_TIMER_ResetTimer(APP_TIMER_DFU_TIMEOUT);
}

void APP_DFU_HDL_Complete(void)
{
    APP_TIMER_StopTimer(APP_TIMER_DFU_TIMEOUT);
}

void APP_DFU_HDL_ErrorHandle(uint16_t dfuHandle)
{
    APP_BLE_ConnList_T *p_dfuConnLink = NULL;
    BLE_DM_ConnParamUpdate_T params;

    p_dfuConnLink = APP_GetConnInfoByConnHandle(dfuHandle);

    if (p_dfuConnLink == NULL)
        return;

    APP_TIMER_StopTimer(APP_TIMER_DFU_TIMEOUT);
    APP_DFU_HDL_SetDfuMode(APP_DFU_MODE_IDLE);

    if (dfuHandle)
    {
        //restore connection parameter
        params.intervalMin = 0x10;  //20ms
        params.intervalMax = 0x10;  //20ms
        params.latency = 0;
        params.timeout = 0x48;//720ms
        BLE_DM_ConnectionParameterUpdate(dfuHandle, &params);
    }
}

void APP_DFU_HDL_Reset(uint16_t delaytime)
{
    APP_TIMER_SetTimer(APP_TIMER_DFU_REBOOT, delaytime, false);
}

void APP_DFU_HDL_SoftwareReset(void)
{
    RCON_SoftwareReset();
}

#endif
/*******************************************************************************
 End of File
 */
