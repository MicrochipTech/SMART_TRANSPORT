/*******************************************************************************
  Application BLE Profile Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_otaps_handler.c

  Summary:
    This file contains the Application BLE functions for this project.

  Description:
    This file contains the Application BLE functions for this project.
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

/*******************************************************************************
  Application BLE Profile Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_otaps_handler.c

  Summary:
    This file contains the Application BLE functions for this project.

  Description:
    This file contains the Application BLE functions for this project.
 *******************************************************************************/


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <app.h>
#include "app_ble_conn_handler.h"
#include "app_dfu_handler.h"
#include "app_timer/app_timer.h"
#include "app_error_defs.h"
#include "app_otaps_handler.h"
#include "ble_dm/ble_dm.h"
#include "ble_dis/ble_dis.h"
#include "ble_util/mw_dfu.h"

#ifdef APP_OTA_DFU_ENABLE
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
//Chimera Image ID
#define RNBD451             0x9B000001
#define WBZ451_BLE_UART     0x9B000003
#define WBZ451_BLE_SENSOR   0x9B000004

//pic32cxbz3 Image ID
#define RNBD351             0x9E000001
#define RNBD350             0x9E000002
#define WBZ351_BLE_UART     0x9E000003
#define WBZ351_BLE_SENSOR   0x9E000004

#define APP_OTA_RES_SUCCESS 0x0000
#define APP_OTA_RES_FAIL    0x0001

// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
static uint16_t s_connHandle, s_fwImageCrc;

// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

void APP_OtapsEvtHandler(BLE_OTAPS_Event_T *p_event)
{
    switch(p_event->eventId)
    {
        case BLE_OTAPS_EVT_UPDATE_REQ:
        {
            uint8_t appVerison[BLE_ATT_DEFAULT_MTU_LEN] = {'\0'};
            uint16_t result = APP_RES_FAIL, appVerisonLength = BLE_ATT_DEFAULT_MTU_LEN;
            uint32_t fwImageId = 0;
            BLE_OTAPS_DevInfo_T devInfo = {0};

            result = GATTS_GetHandleValue(DIS_HDL_CHARVAL_FW_REV, &appVerison[0], &appVerisonLength);
            if (result == APP_RES_SUCCESS)
            {
                devInfo.fwImageVer = (appVerison[0]-'0') << 24 | (appVerison[2]-'0') << 16 | (appVerison[4]-'0') << 8 | (appVerison[6]-'0');
            }
            
            s_connHandle = p_event->eventField.evtUpdateReq.connHandle;
            fwImageId = p_event->eventField.evtUpdateReq.fwImageId;
            s_fwImageCrc = p_event->eventField.evtUpdateReq.fwImageCrc16;
            
            if (fwImageId != WBZ351_BLE_SENSOR)
            {
                BLE_OTAPS_UpdateResponse(s_connHandle, false, &devInfo);
            }
            else
            {
                APP_DFU_HDL_SetDfuMode(APP_DFU_MODE_OTA);
                BLE_OTAPS_UpdateResponse(s_connHandle, true, &devInfo);
                APP_DFU_HDL_Prepare(s_connHandle);
            }
        }
        break;
        
        case BLE_OTAPS_EVT_START_IND:
        {
            APP_DFU_HDL_Start();
        }
        break;
        
        case BLE_OTAPS_EVT_UPDATING_IND:
        {
            APP_DFU_HDL_Updating();
        }
        break;
        
        case BLE_OTAPS_EVT_COMPLETE_IND:
        {
            if (p_event->eventField.evtCompleteInd.errStatus == APP_OTA_RES_SUCCESS)
            {
                APP_DFU_HDL_Complete();

                if (MW_DFU_FwImageValidate(s_fwImageCrc) == APP_OTA_RES_SUCCESS)
                {
                    //After reset, the new FW will activate.
                    if (MW_DFU_FwImageActivate() != APP_OTA_RES_SUCCESS)
                    {
                        BLE_OTAPS_CompleteResponse(false);
                        APP_DFU_HDL_ErrorHandle(s_connHandle);
                    }
                    else
                        BLE_OTAPS_CompleteResponse(true);
                }
                else
                {
                    BLE_OTAPS_CompleteResponse(false);
                    APP_DFU_HDL_ErrorHandle(s_connHandle);
                }
            }
            else
            {
                BLE_OTAPS_CompleteResponse(false);
                APP_DFU_HDL_ErrorHandle(s_connHandle);
            }
        }
        break;
        
        case BLE_OTAPS_EVT_RESET_IND:
        {
            APP_DFU_HDL_Reset(APP_TIMER_500MS);
        }
        break;
        
        default:
        break;
    }
}

void APP_OTA_HDL_Timeout(void)
{
    //When OTA timeout, stop OTA procedure.
    BLE_OTAPS_UpdateStop();
    APP_DFU_HDL_SetDfuMode(APP_DFU_MODE_IDLE);
    APP_DFU_HDL_ErrorHandle(s_connHandle);
}

void APP_OTA_HDL_Init(void)
{
    uint16_t result;
    uint8_t initVector[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    uint8_t key[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 
                    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    
    result = BLE_OTAPS_Init();
    if (result == APP_RES_SUCCESS)
    {
        BLE_OTAPS_EventRegister(APP_OtapsEvtHandler);
        BLE_OTAPS_SetEncrytionInfo(initVector, key);
    }
    
    APP_DFU_HDL_SetDfuMode(APP_DFU_MODE_IDLE);
}

void APP_OTA_HDL_DiscEvtProc(uint16_t connHandle)
{
    if ((s_connHandle == connHandle) && (APP_DFU_HDL_GetDfuMode() == APP_DFU_MODE_OTA))
        APP_DFU_HDL_ErrorHandle(s_connHandle);
}

#endif
/*******************************************************************************
 End of File
 */

