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
#include "app_trpc.h"
#include "app_trps.h"
#include "stdio.h"
// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
#define BLE_SENSOR_VERSION  "1.0.0.0"
extern uint8_t trpsPeripheralDiscoverResp[60];
extern uint8_t APP_GetConnectedDevice_Count(void);
extern bool APP_Check_DeviceAddressByMAC(BLE_GAP_Addr_T *addr);
extern APP_TRPS_SensorData_T bleSensorPeriDevData;
uint8_t trpsPeripheralGetConnRespLen;
bool scanStart;
uint16_t update_conn_hdl;
uint8_t trpsDiscoverRespLen;
uint16_t serv_data;
bool entRmt=false;
bool alarmOn=false;
bool alarmStat=false;
APP_TRPS_GPIO_STATUS_T gpio_on_stat=LED_INIT;
APP_TRPS_GPIO_STATUS_T gpio_off_stat=LED_INIT;
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
#define APP_RNBD_REMOTE_CMD_DISABLE         0x46
#define APP_RNBD_REMOTE_CMD_ENABLE          0x59
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
    APP_mLink_SensorData_Init();
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
    char formattedBuffer[10];
    static uint8_t upd_evt_idx;
    extern PeriDevRGBData bleSensorPeriDevRGB;
    extern PeriDevLedOnOffData bleSensorPeriDevLED;
    extern PeriDevLedTempData bleSensorPeriDevTemp;
    extern APP_TRPS_Temperature smartTruckThermostat;
    extern APP_TRPS_Solar smartSolarPanel;
    extern PeriDevDoorData bleSensorDoorPos;
    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            //appData.appQueue = xQueueCreate( APP_QUEUE_LENGTH, sizeof(APP_Msg_T) );

            APP_Init();
            RTC_Timer32Start();
            SYS_CONSOLE_MESSAGE("Scanning Started\r\n");
            BLE_GAP_SetScanningEnable(true, BLE_GAP_SCAN_FD_DISABLE, BLE_GAP_SCAN_MODE_OBSERVER, 1200);   //mmr
            OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
            if (appInitialized)
            {
                APP_TIMER_SetTimer(APP_TIMER_ID_4,APP_TIMER_10S,true);
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
                else if(p_appMsg->msgId == APP_TIMER_OTA_TIMEOUT_MSG)
                {
                    APP_OTA_Timeout_Handler(); 
                }
                else if(p_appMsg->msgId== APP_TIMER_OTA_REBOOT_MSG)
                {
                    APP_OTA_Reboot_Handler(); 
                }
                else if(p_appMsg->msgId==APP_MSG_RMT_BLE_IO_ON)
                {
                    uint8_t formattedLength;
                    uint16_t rnconn_hdl=APP_GetConnHandleByServData(0xA4);
                    uint8_t rmt_data[15];
                    sprintf((char *)rmt_data,"|O,0001,0001\r\n");
                    uint16_t result=BLE_TRSPC_SendData(rnconn_hdl,15,rmt_data);
                    if (result == MBA_RES_SUCCESS)
                    {   
                        SYS_CONSOLE_MESSAGE("AL On\r\n");
                        gpio_on_stat=LED_ONGNG;
                    }
                    strcpy(formattedBuffer, "AL:ON");
                    formattedLength = 5;
                    APP_Msg_T appMsg;
                    appMsg.msgData[0] = (uint8_t)formattedLength;
                    memcpy(&appMsg.msgData[1], formattedBuffer, formattedLength);
                    appMsg.msgId = APP_BLE_SEND_DISPLAY;
                    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    
                }
                else if(p_appMsg->msgId==APP_MSG_RMT_BLE_IO_ERR)
                {
                    uint16_t rnconn_hdl=APP_GetConnHandleByServData(0xA4);
                    uint8_t rmt_data[3];
                    sprintf((char *)rmt_data,"\r\n");
                    uint16_t result=BLE_TRSPC_SendData(rnconn_hdl,2,rmt_data);
                    if (result == MBA_RES_SUCCESS)
                    {   
                        SYS_CONSOLE_MESSAGE("Data\r\n");
                    }
                    if(gpio_on_stat==LED_ONGNG)
                    {
                        APP_TIMER_SetTimer(APP_TIMER_ID_6,APP_TIMER_30MS,false);
                    }
                    if(gpio_off_stat==LED_ONGNG)
                    {
                        APP_TIMER_SetTimer(APP_TIMER_ID_7,APP_TIMER_30MS,false);
                    }
                }
                else if(p_appMsg->msgId==APP_MSG_RMT_BLE_IO_OFF)
                {
//                    static uint8_t tryCnt=0;
                    uint8_t formattedLength;
                    uint16_t rnconn_hdl=APP_GetConnHandleByServData(0xA4);
                    uint8_t rmt_data[15];
                    sprintf((char *)rmt_data,"|O,0001,0000\r\n");
                    uint16_t result=BLE_TRSPC_SendData(rnconn_hdl,15,rmt_data);
                    if (result == MBA_RES_SUCCESS)
                    {   
                        SYS_CONSOLE_MESSAGE("AL OFF\r\n");
                        gpio_off_stat=LED_ONGNG;
                    }
//                    if(tryCnt%2!=0)
//                    {
//                        APP_TIMER_SetTimer(APP_TIMER_ID_7,APP_TIMER_1S,false);
//                        alarmOn=false;
//                    }
                    strcpy(formattedBuffer, "AL:OFF");  // open
                    formattedLength = 6; // Include null terminator
                    APP_Msg_T appMsg;
                    appMsg.msgData[0] = (uint8_t)formattedLength;
                    memcpy(&appMsg.msgData[1], formattedBuffer, formattedLength);
                    appMsg.msgId = APP_BLE_SEND_DISPLAY;
                    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                }
                else if(p_appMsg->msgId==APP_MSG_BLE_ENT_RMT_MODE)
                {
                    uint16_t conn_hdl=APP_GetConnHandleByServData(0xA4);
                    uint8_t rmt_cmd[4]={0x30,0x30,0x30,0x30};
                    uint16_t result=BLE_TRSPC_SendVendorCommand(conn_hdl,APP_RNBD_REMOTE_CMD_ENABLE,4,rmt_cmd);
                    
                    if (result == MBA_RES_SUCCESS)
                    {   
                        SYS_CONSOLE_PRINT("ENT rmt\r\n");
                    }
                }
                else if(p_appMsg->msgId==APP_MSG_BLE_EXT_RMT_MODE)
                {
                    uint16_t conn_hdl=APP_GetConnHandleByServData(0xA4);
                    uint8_t rmt_cmd[4]={0x30,0x30,0x30,0x30};
                    uint16_t result=BLE_TRSPC_SendVendorCommand(conn_hdl,APP_RNBD_REMOTE_CMD_DISABLE,0,rmt_cmd);
                    if (result == MBA_RES_SUCCESS)
                    {   
                        SYS_CONSOLE_PRINT("EXIT rmt\r\n");
                    }
                }
                else if(p_appMsg->msgId== APP_MSG_BLE_GET_CONN_EVT)
                {
                    uint8_t periDevCount= APP_GetConnectedDevice_Count();//p_appMsg->msgData[0];
                    SYS_CONSOLE_PRINT("[BLE]Connection count:%d\r\n",periDevCount); 
                    static uint8_t get_conn_stat =0;
                    static uint8_t prev_get_conn_stat =0;
                    int8_t rssi;
                    if(get_conn_stat!=prev_get_conn_stat)
                    {
                        periDevCount=periDevCount-get_conn_stat;
                    }
                    if(periDevCount!=0)
                    {
                        APP_BLE_ConnList_T *conn_dev_data;
                        uint16_t conn_hdl= APP_GetConnHandleByIndex(periDevCount);
                        conn_dev_data=APP_GetConnInfoByConnHandle(conn_hdl);
                        uint8_t trpsPeripheralGetConnRespData[50];
                        
                        uint8_t *trpsGetConnRespPtr;
                        trpsGetConnRespPtr=trpsPeripheralGetConnRespData;
                        *trpsGetConnRespPtr++=(uint8_t)(conn_dev_data->connData.handle >> 8);
                        *trpsGetConnRespPtr++=(uint8_t)(conn_dev_data->connData.handle & 0xFF);
                        *trpsGetConnRespPtr++=0x09;
                        trpsPeripheralGetConnRespLen=3;
                        *trpsGetConnRespPtr++=conn_dev_data->connData.localName.nameLen;//0x04; //Len of Local Name
                        trpsPeripheralGetConnRespLen+=(conn_dev_data->connData.localName.nameLen+1);
                        memcpy(trpsGetConnRespPtr,conn_dev_data->connData.localName.localName,conn_dev_data->connData.localName.nameLen);
                        trpsGetConnRespPtr +=conn_dev_data->connData.localName.nameLen;
                        memcpy(trpsGetConnRespPtr,conn_dev_data->connData.remoteAddr.addr,GAP_MAX_BD_ADDRESS_LEN);
                        memcpy(trpsGetConnRespPtr,conn_dev_data->connData.remoteAddr.addr,GAP_MAX_BD_ADDRESS_LEN);
                        trpsGetConnRespPtr += GAP_MAX_BD_ADDRESS_LEN;
                        trpsPeripheralGetConnRespLen+=GAP_MAX_BD_ADDRESS_LEN-1;
                        BLE_GAP_GetRssi(update_conn_hdl,&rssi);
                        *trpsGetConnRespPtr=rssi;
                        trpsPeripheralGetConnRespLen++;
                        memcpy(trpsPeripheralGetConnResp,trpsPeripheralGetConnRespData,trpsPeripheralGetConnRespLen);
                        SYS_CONSOLE_PRINT("TRPS:GET_CONN Len:%d \r\n",trpsPeripheralGetConnRespLen); 
                        uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,GET_CONNECTION_RESP);
                        if(result== 0)
                        {
                            SYS_CONSOLE_MESSAGE("TRPS:GET_CONN Sent data\r\n");
                            prev_get_conn_stat=get_conn_stat;
                            get_conn_stat=get_conn_stat+1;
                            APP_TIMER_SetTimer(APP_TIMER_ID_4, APP_TIMER_1S,false);
                        }
                        else
                        {
                            SYS_CONSOLE_PRINT("TRPS:GET_CONN Send Failed %x \r\n",result); 
                            prev_get_conn_stat=get_conn_stat;
                            APP_TIMER_SetTimer(APP_TIMER_ID_4, APP_TIMER_1S,false);
                        }
                    }
                    else
                    {
                        prev_get_conn_stat=0;
                        get_conn_stat=0;
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_SEND_DISC_RESP)
                {
                    uint16_t connHandle=(uint16_t)((p_appMsg->msgData[1]<<8)| p_appMsg->msgData[2]);
                    int8_t rssi;
                    APP_BLE_ConnList_T *conn_dev_data;
                    conn_dev_data= APP_GetConnInfoByConnHandle(connHandle);
                    uint8_t trpsPeripheralGetConnRespData[50];
                    uint8_t trpsPeripheralGetConnRespLen;
                    uint8_t *trpsGetConnRespPtr;
                    trpsGetConnRespPtr=trpsPeripheralGetConnRespData;
                    *trpsGetConnRespPtr++=(uint8_t)(conn_dev_data->connData.handle >> 8);
                    *trpsGetConnRespPtr++=(uint8_t)(conn_dev_data->connData.handle & 0xFF);
                    *trpsGetConnRespPtr++=0x09;
                    trpsPeripheralGetConnRespLen=3;
                    *trpsGetConnRespPtr++=conn_dev_data->connData.localName.nameLen;//0x04; //Len of Local Name
                    trpsPeripheralGetConnRespLen+=(conn_dev_data->connData.localName.nameLen+1);
                    memcpy(trpsGetConnRespPtr,conn_dev_data->connData.localName.localName,conn_dev_data->connData.localName.nameLen);
                    trpsGetConnRespPtr +=conn_dev_data->connData.localName.nameLen;
                    memcpy(trpsGetConnRespPtr,conn_dev_data->connData.remoteAddr.addr,GAP_MAX_BD_ADDRESS_LEN);
                    memcpy(trpsGetConnRespPtr,conn_dev_data->connData.remoteAddr.addr,GAP_MAX_BD_ADDRESS_LEN);
                    trpsGetConnRespPtr += GAP_MAX_BD_ADDRESS_LEN;
                    trpsPeripheralGetConnRespLen+=GAP_MAX_BD_ADDRESS_LEN-1;
                    BLE_GAP_GetRssi(connHandle,&rssi);
                    *trpsGetConnRespPtr=rssi;
                    trpsPeripheralGetConnRespLen++;
                    trpsDiscoverRespLen=trpsPeripheralGetConnRespLen;
                    SYS_CONSOLE_PRINT("Discover data len: %d\r\n",trpsDiscoverRespLen);
                    memcpy(trpsPeripheralDiscoverResp,&trpsPeripheralGetConnRespData,trpsPeripheralGetConnRespLen);
                    uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,DISCOVER_RESP);
                    if(result== 0)
                    {
                    }
                    else
                    {
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_SET_TEMP_NTY)
                {
                    uint16_t connHandle=APP_GetConnHandleByServData(0xA2);
                    uint16_t result = MBA_RES_SUCCESS;
                    uint8_t payload_cmd[4]={0x03,0x14,0x00,0x00};
                    payload_cmd[2]=p_appMsg->msgData[0];
                    payload_cmd[3]=p_appMsg->msgData[1];
                    result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR,sizeof(payload_cmd), payload_cmd);
                    if(result== 0)
                    {
                    }
                    else
                    {
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_SET_DOOR_POS)
                {
                    uint16_t connHandle=APP_GetConnHandleByServData(0xA6);
                    uint16_t result = MBA_RES_SUCCESS;
                    uint8_t payload_cmd[3]={0x03,0x15,0x00};
                    payload_cmd[2]=p_appMsg->msgData[0];
                    result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR,sizeof(payload_cmd), payload_cmd);
                    if(result== 0)
                    {
                    }
                    else
                    {
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_SET_LED_ONOFF)
                {
                    char formattedBuffer[10];
                    uint8_t buffer_len;
                    uint16_t connHandle=APP_GetConnHandleByServData(0xA1);
                    uint16_t result = MBA_RES_SUCCESS;
                    uint8_t payload_cmd[3]={0x02,0x10,0x00};
                    payload_cmd[2]=p_appMsg->msgData[3];
                    bleSensorPeriDevLED.rgbOnOffStatus=p_appMsg->msgData[3];
                    result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR,sizeof(payload_cmd), payload_cmd);
                    GPIO_RB1_Toggle();
                    if(result== 0)
                    {
                    }
                    else
                    { 
                    }
                    result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,RGB_ONOFF_STATUS_MRML_NFY);
                    if(result== 0)
                    {
                    }
                    else
                    { 
                    }
                    if(p_appMsg->msgData[3]==1)
                    {
                        strcpy(formattedBuffer, "LT:ON\r\n");
                        buffer_len = 7;
                    } 
                    else 
                    {
                        strcpy(formattedBuffer, "LT:OFF\r\n"); 
                        buffer_len = 8;
                    }
                    
                    APP_Msg_T appMsg;
                    appMsg.msgData[0] = (uint8_t)buffer_len;
                    memcpy(&appMsg.msgData[1], formattedBuffer, buffer_len);
                    appMsg.msgId = APP_BLE_SEND_DISPLAY;
                    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                }
                else if(p_appMsg->msgId== APP_BLE_GET_LED_ONOFF)
                {
                    uint16_t connHandle=(uint16_t)((p_appMsg->msgData[0]<<8)| p_appMsg->msgData[1]);
                    uint16_t result = MBA_RES_SUCCESS;
                    uint8_t payload_cmd[2]={0x01,0x11};
                        result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR,sizeof(payload_cmd), payload_cmd);
                    if(result== 0)
                    {
                    }
                    else
                    { 
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_SET_RGB_COLOR)
                {
                    uint16_t connHandle=(uint16_t)((p_appMsg->msgData[1]<<8)| p_appMsg->msgData[2]);
                    uint16_t result = MBA_RES_SUCCESS;
                    uint8_t payload_cmd[5]={0x04,0x12,0x00,0x00,0x00};
                    payload_cmd[2]=p_appMsg->msgData[3];
                    payload_cmd[3]=p_appMsg->msgData[4];
                    payload_cmd[4]=p_appMsg->msgData[5];
                        result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR,sizeof(payload_cmd), payload_cmd);
                    if(result== 0)
                    {
                    }
                    else
                    { 
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_GET_RGB_COLOR)
                {
                    uint16_t connHandle=(uint16_t)((p_appMsg->msgData[0]<<8)| p_appMsg->msgData[1]);
                    uint16_t result = MBA_RES_SUCCESS;
                    uint8_t payload_cmd[2]={0x01,0x13};
                        result = BLE_TRSPC_SendVendorCommand(connHandle, APP_TRP_VENDOR_OPCODE_BLE_SENSOR,sizeof(payload_cmd), payload_cmd);
                    if(result== 0)
                    {
                    }
                    else
                    {
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_SEND_UPDATE)
                {
                    update_conn_hdl=((p_appMsg->msgData[0]<<8)|p_appMsg->msgData[1]);
                    APP_BLE_ConnList_T *conn_dev_data;
                    conn_dev_data= APP_GetConnInfoByConnHandle(update_conn_hdl);
                    if(conn_dev_data->connData.role!=01 && conn_dev_data->connData.service_data==0xA1)
                    {
                        uint8_t payload_cmd[2] = {0x01, 0x11}; //Get RGB LED Status
                        uint8_t result = BLE_TRSPC_SendVendorCommand(update_conn_hdl, APP_TRP_VENDOR_OPCODE_BLE_SENSOR, sizeof(payload_cmd), payload_cmd);
                        if(result== 0)
                        {
                            upd_evt_idx=0x1;
                        }
                        else
                        { 
                        }
                    }
                    else
                    {
                        appMsg->msgId = APP_BLE_SEND_UPDATE_RESP;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    }
                    
                }
                else if(p_appMsg->msgId== APP_BLE_SEND_DISPLAY)
                {
                    uint16_t dispConnHdl=APP_GetConnHandleByServData(0xA7);
                    if(dispConnHdl!=0xffff)
                    {
                        uint16_t result=BLE_TRSPC_SendData(dispConnHdl,p_appMsg->msgData[0],&p_appMsg->msgData[1]);
                        if(result == MBA_RES_SUCCESS)
                        {
//                            SYS_CONSOLE_MESSAGE("Sent data to Display\r\n");
                        }
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_DISC_NTY)
                {
                    extern uint8_t disconnect_conn_hdl[2];
                    disconnect_conn_hdl[0]=p_appMsg->msgData[0];
                    disconnect_conn_hdl[1]=p_appMsg->msgData[1];
                    uint8_t result = APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,DISCONNECT_RESP);
                    if(result== 0)
                    {
//                        SYS_CONSOLE_PRINT("TRPS:Sent data\r\n");
                    }
                    else
                    {
//                       SYS_CONSOLE_PRINT("TRPS:Send Failed\r\n"); 
                    }
                }
                else if(p_appMsg->msgId== APP_BLE_SEND_UPDATE_RESP)
                {
                    
                    int8_t update_rsp_rssi;
                    uint16_t sensTemperature=0;
                    uint8_t rgbOnOffStatus=0;
                    APP_BLE_ConnList_T *conn_dev_data;
                    conn_dev_data= APP_GetConnInfoByConnHandle(update_conn_hdl);
                    APP_MultiLink_SensorData_T *remoteSensorDataItem;
                    uint16_t rssi_result=BLE_GAP_GetRssi(update_conn_hdl,&update_rsp_rssi);
                    if(rssi_result!=0)
                    {
//                        SYS_CONSOLE_PRINT("RSSI:Send Failed\r\n");
                    }
                    remoteSensorDataItem = APP_mLink_GetSensDataByConnHandle(update_conn_hdl);
                    if(remoteSensorDataItem != NULL)
                    {
                        sensTemperature = (uint16_t)(remoteSensorDataItem->mLinkSensorData.tempSens.lsb<<8 | remoteSensorDataItem->mLinkSensorData.tempSens.msb);
                        rgbOnOffStatus= remoteSensorDataItem->mLinkSensorData.rgbOnOffStatus;
                    }
                    extern uint8_t trpsPeripheralUpdateResp[50];
                    uint8_t *trpsPeripheralUpdateRespPtr=trpsPeripheralUpdateResp;
                    *trpsPeripheralUpdateRespPtr++=(uint8_t)(update_conn_hdl >> 8);
                    *trpsPeripheralUpdateRespPtr++=(uint8_t)(update_conn_hdl & 0xFF);
                    if(conn_dev_data->connData.role==BLE_GAP_ROLE_CENTRAL && conn_dev_data->connData.service_data!=0x4)         //00-Peripheral  
                    {                                                                                                           //01-Central
                        *trpsPeripheralUpdateRespPtr++=0x01;                                                                    //02-Multi-Link
                    }                                                                                                           //03-Multi-Role  
                    else if(conn_dev_data->connData.role==BLE_GAP_ROLE_CENTRAL && conn_dev_data->connData.service_data!=0x4)    //04-MRML                                                                                   
                    {
                        *trpsPeripheralUpdateRespPtr++=0x04;     //0x00; //Central device
                    }
                    else 
                    {
                        *trpsPeripheralUpdateRespPtr++=0x0;
                    }
//                    SYS_CONSOLE_PRINT("SERVICE DATA:%x\r\n",conn_dev_data->connData.service_data);
                    if(((conn_dev_data->connData.service_data== 0xA1) |(conn_dev_data->connData.service_data== 0x02) | (conn_dev_data->connData.service_data== 0x05) | (conn_dev_data->connData.service_data== 0x07)))
                    {
                        *trpsPeripheralUpdateRespPtr++=(uint8_t)(0x10);         //10- BLE Sensor enable
//                        SYS_CONSOLE_MESSAGE("TRPS: Sensor enable\r\n");
                    }
                    else if((conn_dev_data->connData.service_data== 0xA2) |(conn_dev_data->connData.service_data== 0xA3) |(conn_dev_data->connData.service_data== 0xA4) |(conn_dev_data->connData.service_data== 0xA5)|(conn_dev_data->connData.service_data== 0xA6)|(conn_dev_data->connData.service_data== 0xA7))
                    {
                        *trpsPeripheralUpdateRespPtr++=(uint8_t)(0x01);         //10- BLE Sensor enable
//                        SYS_CONSOLE_MESSAGE("TRPS: Sensor and Chat enable\r\n");
                    }
                    else if(conn_dev_data->connData.service_data== 0x01)
                    {
                        *trpsPeripheralUpdateRespPtr++=(uint8_t)(0x01);         //01- BLE Chat enable
                    }
                    else
                    {
                        *trpsPeripheralUpdateRespPtr++=(uint8_t)(0x00);          //00- BLE enable none
                    }
                    *trpsPeripheralUpdateRespPtr++=rgbOnOffStatus;
                    *trpsPeripheralUpdateRespPtr++=(uint8_t)(sensTemperature >> 8);
                    *trpsPeripheralUpdateRespPtr++=(uint8_t)(sensTemperature & 0xFF);
                    *trpsPeripheralUpdateRespPtr++=(uint8_t)(update_rsp_rssi);
                    uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,UPDATE_RESP);
                    if(result== 0)
                    {
//                        SYS_CONSOLE_MESSAGE("TRPS:UPDATE_RESP Sent data\r\n");
                    }
                    else
                    {
//                       SYS_CONSOLE_MESSAGE("TRPS:UPDATE_RESP Send Failed \r\n"); 
                    }
                }
                else if(p_appMsg->msgId == APP_MSG_TRSPC_RX_EVT)
                {
                    if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == TEMP_SENSOR_NFY) )
                    {
                        uint16_t connHandle = (uint16_t)(p_appMsg->msgData[1]<<8 | p_appMsg->msgData[0]);
//                        SYS_CONSOLE_PRINT("Temperature[0x%X]: %d.%dC\r\n", connHandle, p_appMsg->msgData[7],  p_appMsg->msgData[6]/10 );
                        APP_mLink_Temperature_Update(connHandle, (uint16_t)(p_appMsg->msgData[6]<<8 | p_appMsg->msgData[7]));
                        bleSensorPeriDevTemp.lsb_conn_hdl=p_appMsg->msgData[0];
                        bleSensorPeriDevTemp.msb_conn_hdl=p_appMsg->msgData[1];
                        bleSensorPeriDevTemp.msb_temp=p_appMsg->msgData[6];
                        bleSensorPeriDevTemp.lsb_temp=p_appMsg->msgData[7];
                        uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,TEMP_SENSOR_MRML_NFY);
                        if(result== 0)
                        {
//                            SYS_CONSOLE_MESSAGE("TRPS:Sent data\r\n");
                        }
                        else
                        {
//                           SYS_CONSOLE_MESSAGE("TRPS:Send Failed \r\n"); 
                        }
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == TEMP_REPORT_NFY) )
                    {
                        uint16_t whole_part;
                        uint16_t fractional_part;
                        char temp_str[30];  // Buffer for temperature strings
                        smartTruckThermostat.lsb_conn_hdl = p_appMsg->msgData[0];
                        smartTruckThermostat.msb_conn_hdl = p_appMsg->msgData[1];
                        smartTruckThermostat.hvacStatus = p_appMsg->msgData[6];
                        smartTruckThermostat.setTemp.msb = p_appMsg->msgData[7];
                        smartTruckThermostat.setTemp.lsb = p_appMsg->msgData[8];
                        smartTruckThermostat.currTemp.msb = p_appMsg->msgData[9];
                        smartTruckThermostat.currTemp.lsb = p_appMsg->msgData[10];

                        uint8_t buffer[50];
                        uint8_t buffer_ind = 0;  // Initialize buffer_ind

                        buffer[1] = 0xB3;
                        buffer[2] = p_appMsg->msgData[1];
                        buffer[3] = p_appMsg->msgData[0];

                        uint8_t buffer1[] = {0x48, 0x76, 0x61, 0x63, 0x3A};
                        memcpy(&buffer[5], buffer1, sizeof(buffer1));
                        buffer_ind = 5 + sizeof(buffer1);  // Correctly set buffer_ind after memcpy
                        buffer[buffer_ind++] = smartTruckThermostat.hvacStatus + '0';
                        buffer[buffer_ind++] = 0x0A;
//                        buffer[buffer_ind++] = 0x0D;

                        uint16_t set_temp = (smartTruckThermostat.setTemp.msb << 8) | smartTruckThermostat.setTemp.lsb;
                        whole_part = set_temp / 10;
                        fractional_part = set_temp % 10;
                        sprintf(temp_str, "ST:%u.%uC\n", whole_part, fractional_part);
                        size_t temp_str_len = strlen(temp_str);  // Get the length of the set temperature string
                        memcpy(&buffer[buffer_ind], temp_str, temp_str_len);
                        buffer_ind += temp_str_len;
//                        SYS_CONSOLE_PRINT("SET Temperature:%dC\r\n",set_temp);
                        uint16_t curr_temp = (smartTruckThermostat.currTemp.msb << 8) | smartTruckThermostat.currTemp.lsb;
                        whole_part = curr_temp / 10;
                        fractional_part = curr_temp % 10;
                        sprintf(temp_str, "TS:%u.%uC", whole_part, fractional_part);  // Reuse temp_str for current temperature
                        temp_str_len = strlen(temp_str);  // Get the length of the current temperature string
                        memcpy(&buffer[buffer_ind], temp_str, temp_str_len);
                        buffer_ind += temp_str_len;

                        buffer[4] = buffer_ind - 5;  // Correctly set buffer[4] to the length of the payload
                        buffer[0] = buffer_ind + 4;  // Correctly set buffer[0] to the total length

                        uint16_t result = BLE_TRSPS_SendChat(APP_TRP_VENDOR_OPCODE_MRML, buffer[0], buffer);
                        if (result == MBA_RES_SUCCESS) 
                        {
//                            SYS_CONSOLE_MESSAGE("Temp Chat Send Success\r\n");
                        }

                        APP_Msg_T appMsg;
                        appMsg.msgData[0] = temp_str_len;
                        memcpy(&appMsg.msgData[1], temp_str,temp_str_len);
                        appMsg.msgId = APP_BLE_SEND_DISPLAY;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);

                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == SOLAR_REPORT_NFY) )
                    {
                        char formattedBuffer[13];
                        smartSolarPanel.lsb_conn_hdl = p_appMsg->msgData[0];
                        smartSolarPanel.msb_conn_hdl = p_appMsg->msgData[1];
                        smartSolarPanel.solar_volt.msb = p_appMsg->msgData[6];
                        smartSolarPanel.solar_volt.lsb = p_appMsg->msgData[7];
                        uint16_t voltage = (smartSolarPanel.solar_volt.msb << 8) | smartSolarPanel.solar_volt.lsb;
                        uint16_t whole_part = voltage / 100;
                        uint16_t fractional_part = voltage % 100;
                        int formattedLength = snprintf(formattedBuffer, sizeof(formattedBuffer), "SR:%u.%02uV", whole_part, fractional_part);
                        if (formattedLength < 0 || formattedLength >= sizeof(formattedBuffer)) 
                        {
                            return;
                        }
                        uint8_t buffer[30];
                        buffer[0] = 4+formattedLength ;
                        buffer[1] = 0xB3;
                        buffer[2] = p_appMsg->msgData[1];
                        buffer[3] = p_appMsg->msgData[0];
                        buffer[4] = (uint8_t)formattedLength;
                        memcpy(&buffer[5], formattedBuffer, formattedLength);
                        uint16_t result = BLE_TRSPS_SendChat(APP_TRP_VENDOR_OPCODE_MRML, 5 + formattedLength, buffer);
                        if (result == MBA_RES_SUCCESS) 
                        {
//                            SYS_CONSOLE_MESSAGE("Solar Chat Send Success\r\n");
                        } 
                        else 
                        {
//                            SYS_CONSOLE_MESSAGE("Solar Chat Send Failed\r\n");
                        }
                        APP_Msg_T appMsg;
                        appMsg.msgData[0] = (uint8_t)formattedLength;
                        memcpy(&appMsg.msgData[1], formattedBuffer, formattedLength);
                        appMsg.msgId = APP_BLE_SEND_DISPLAY;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == DOOR_REPORT_NFY) )
                    {
                        uint8_t buffer[30];
                        uint8_t formattedLength;
                        char formattedBuffer[15];
                        bleSensorDoorPos.lsb_conn_hdl = p_appMsg->msgData[0];
                        bleSensorDoorPos.msb_conn_hdl = p_appMsg->msgData[1];
                        bleSensorDoorPos.door_pos = p_appMsg->msgData[6];

                        if (bleSensorDoorPos.door_pos == 0) 
                        {
                            strcpy(formattedBuffer, "DR:CLOSE\r\n"); // close
                            formattedLength = 10; // Include null terminator
                        } 
                        else 
                        {
                            strcpy(formattedBuffer, "DR:OPEN\r\n");  // open
                            formattedLength = 9; // Include null terminator
                        }

                        buffer[0] = 4+formattedLength;
                        buffer[1] = 0xB3;
                        buffer[2] = p_appMsg->msgData[1];
                        buffer[3] = p_appMsg->msgData[0];
                        buffer[4] = formattedLength;
                        memcpy(&buffer[5], formattedBuffer, formattedLength);

                        uint16_t result = BLE_TRSPS_SendChat(APP_TRP_VENDOR_OPCODE_MRML, 5 + formattedLength, buffer);
                        if (result == MBA_RES_SUCCESS)
                        {
//                            SYS_CONSOLE_MESSAGE("Door Chat Send Success\r\n");
                        }
                        else
                        {
//                            SYS_CONSOLE_MESSAGE("Door Chat Send Failed\r\n");
                        }

                        APP_Msg_T appMsg;
                        appMsg.msgData[0] = formattedLength;
                        memcpy(&appMsg.msgData[1], formattedBuffer, formattedLength);
                        appMsg.msgId = APP_BLE_SEND_DISPLAY;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == AIML_REPORT_NFY) )
                    {
//                        APP_Msg_T appMsg2;
                        uint8_t buffer[30];
                        uint8_t formattedLength;
                        char formattedBuffer[15];
                        bleSensorMotorStat.lsb_conn_hdl = p_appMsg->msgData[0];
                        bleSensorMotorStat.msb_conn_hdl = p_appMsg->msgData[1];
                        bleSensorMotorStat.motor_stat = p_appMsg->msgData[6];
                        if (bleSensorMotorStat.motor_stat == 2) 
                        {
                            strcpy(formattedBuffer, "AM:Motor off\r\n"); // close
                            formattedLength = 14;
                        } 
                        else if(bleSensorMotorStat.motor_stat == 3)
                        {
                            strcpy(formattedBuffer, "AM:Motor on\r\n");
                            formattedLength = 13;
                        }
                        else if(bleSensorMotorStat.motor_stat == 1)
                        {
                            strcpy(formattedBuffer, "AM:Blade\r\n"); 
                            formattedLength = 10;
                            if (entRmt==true && alarmOn==false) 
                            {
                                APP_TIMER_SetTimer(APP_TIMER_ID_6,APP_TIMER_2S,false);
                            }
                            else if(entRmt==false && alarmOn==false)
                            {
                                APP_TIMER_SetTimer(APP_TIMER_ID_5,APP_TIMER_30MS,false);
                            }
                            else
                            {
                            }
                        }
                        else
                        {
                            strcpy(formattedBuffer, "AM:Anomaly\r\n");
                            formattedLength = 10; 
                        }

                        buffer[0] = 4+formattedLength;
                        buffer[1] = 0xB3;
                        buffer[2] = p_appMsg->msgData[1];
                        buffer[3] = p_appMsg->msgData[0];
                        buffer[4] = formattedLength;
                        memcpy(&buffer[5], formattedBuffer, formattedLength);

                        uint16_t result = BLE_TRSPS_SendChat(APP_TRP_VENDOR_OPCODE_MRML, 5 + formattedLength, buffer);
                        if (result == MBA_RES_SUCCESS)
                        {
                        }
                        else
                        {
                        }

                        APP_Msg_T appMsg;
                        appMsg.msgData[0] = formattedLength;
                        memcpy(&appMsg.msgData[1], formattedBuffer, formattedLength);
                        appMsg.msgId = APP_BLE_SEND_DISPLAY;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                        
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == RGB_ONOFF_GET_RSP) )
                    {
                        uint16_t connHandle = (uint16_t)(p_appMsg->msgData[1]<<8 | p_appMsg->msgData[0]);
                        bleSensorPeriDevLED.lsb_conn_hdl=p_appMsg->msgData[0];
                        bleSensorPeriDevLED.msb_conn_hdl=p_appMsg->msgData[1];
                        bleSensorPeriDevLED.rgbOnOffStatus=p_appMsg->msgData[7];
                        uint8_t result= MBA_RES_SUCCESS;
                        if(p_appMsg->msgData[6] == 0)
                        {
//                            SYS_CONSOLE_PRINT("OnOff Value[0x%X]: 0x%X\r\n", connHandle, p_appMsg->msgData[7]);
                            APP_mLink_rgbOnOffStatus_Update(connHandle, p_appMsg->msgData[7]);
                            bleSensorPeriDevData.rgbOnOffStatus=p_appMsg->msgData[7];
                            if(upd_evt_idx==0)
                                result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,RGB_ONOFF_GET_RSP);
                            if(result== 0)
                            {
//                                SYS_CONSOLE_MESSAGE("TRPS:Sent data\r\n");
                            }
                            else
                            {
//                               SYS_CONSOLE_MESSAGE("TRPS:Send Failed \r\n"); 
                            }
                            upd_evt_idx=upd_evt_idx&0xF;
                        }
                        else
                        {
//                            SYS_CONSOLE_PRINT("Get OnOff Value Failed\r\n");
                        }
                        
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == RGB_ONOFF_SET_RSP) ) //onoff response
                    {
                        GPIO_RB1_Toggle();
                        bleSensorPeriDevLED.lsb_conn_hdl=p_appMsg->msgData[0];
                        bleSensorPeriDevLED.msb_conn_hdl=p_appMsg->msgData[1];
                        bleSensorPeriDevLED.rgbOnOffStatus=p_appMsg->msgData[6];
                        uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,RGB_ONOFF_SET_RSP);
                        if(result== 0)
                        {
//                            SYS_CONSOLE_MESSAGE("TRPS:Sent data\r\n");
                        }
                        else
                        {
//                           SYS_CONSOLE_MESSAGE("TRPS:Send Failed \r\n"); 
                        }
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == RGB_COLOR_SET_RSP) ) //color response
                    {
                        bleSensorPeriDevRGB.lsb_conn_hdl=p_appMsg->msgData[0];
                        bleSensorPeriDevRGB.msb_conn_hdl=p_appMsg->msgData[1];
                        bleSensorPeriDevRGB.Hue=p_appMsg->msgData[7];
                        bleSensorPeriDevRGB.Saturation=p_appMsg->msgData[8];
                        bleSensorPeriDevRGB.Value=p_appMsg->msgData[9];
                        uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,RGB_COLOR_SET_RSP);
                        if(result== 0)
                        {
//                            SYS_CONSOLE_MESSAGE("TRPS:Sent data\r\n");
                        }
                        else
                        {
//                           SYS_CONSOLE_MESSAGE("TRPS:Send Failed \r\n"); 
                        }
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == RGB_COLOR_GET_RSP) ) //color response
                    {
                        if(p_appMsg->msgData[6] == 0)
                        {
                            bleSensorPeriDevRGB.lsb_conn_hdl=p_appMsg->msgData[0];
                            bleSensorPeriDevRGB.msb_conn_hdl=p_appMsg->msgData[1];
                            bleSensorPeriDevRGB.Hue=p_appMsg->msgData[7];
                            bleSensorPeriDevRGB.Saturation=p_appMsg->msgData[8];
                            bleSensorPeriDevRGB.Value=p_appMsg->msgData[9];
                            uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,RGB_COLOR_GET_RSP);
                            if(result== 0)
                            {
//                                SYS_CONSOLE_MESSAGE("TRPS:Sent data\r\n");
                            }
                            else
                            {
//                               SYS_CONSOLE_MESSAGE("TRPS:Send Failed \r\n"); 
                            }
                            
                        }
                        else
                        {
//                            SYS_CONSOLE_PRINT("Get OnOff Value Failed\r\n");
                        }
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == RGB_ONOFF_STATUS_NFY) ) //color response
                    {
                        char formattedBuffer[10];
                        uint8_t buffer_len;
                        bleSensorPeriDevLED.lsb_conn_hdl=p_appMsg->msgData[0];
                        bleSensorPeriDevLED.msb_conn_hdl=p_appMsg->msgData[1];
                        bleSensorPeriDevLED.rgbOnOffStatus=p_appMsg->msgData[6];
                        uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,RGB_ONOFF_STATUS_MRML_NFY);
                        if(result== 0)
                        {
//                            SYS_CONSOLE_MESSAGE("TRPS:Sent data\r\n");
                        }
                        else
                        {
//                           SYS_CONSOLE_MESSAGE("TRPS:Send Failed \r\n"); 
                        }
                        if(bleSensorPeriDevLED.rgbOnOffStatus==1)
                        {
                            strcpy(formattedBuffer, "LT:ON\r\n"); // close
                            buffer_len = 7; // Include null terminator
                        } 
                        else 
                        {
                            strcpy(formattedBuffer, "LT:OFF\r\n");  // open
                            buffer_len = 8; // Include null terminator
                        }
                    
                        APP_Msg_T appMsg;
                        appMsg.msgData[0] = (uint8_t)buffer_len;
                        memcpy(&appMsg.msgData[1], formattedBuffer, buffer_len);
                        appMsg.msgId = APP_BLE_SEND_DISPLAY;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    }
                    else if( (p_appMsg->msgData[3] == APP_TRP_VENDOR_OPCODE_BLE_SENSOR) && 
                            (p_appMsg->msgData[5] == RGB_COLOR_NFY) ) //color response
                    {
                        bleSensorPeriDevRGB.lsb_conn_hdl=p_appMsg->msgData[0];
                        bleSensorPeriDevRGB.msb_conn_hdl=p_appMsg->msgData[1];
                        bleSensorPeriDevRGB.Hue=p_appMsg->msgData[6];
                        bleSensorPeriDevRGB.Saturation=p_appMsg->msgData[7];
                        bleSensorPeriDevRGB.Value=p_appMsg->msgData[8];
                        uint8_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,RGB_COLOR_MRML_NFY);
                        if(result== 0)
                        {
//                            SYS_CONSOLE_MESSAGE("TRPS:Sent data\r\n");
                        }
                        else
                        {
//                           SYS_CONSOLE_MESSAGE("TRPS:Send Failed \r\n"); 
                        }
                    }
                    else
                    {
//                        SYS_CONSOLE_PRINT("Unknown TRPC Vendor Command: 0x%x - 0x%x\r\n", p_appMsg->msgData[3], p_appMsg->msgData[5]);
                    }
                    if(upd_evt_idx!=0)
                    {
                        upd_evt_idx=0;
                        appMsg->msgId = APP_BLE_SEND_UPDATE_RESP;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    }
                }
                else if(p_appMsg->msgId == APP_MSG_BLE_SCAN_START_EVT)      //APP_TIMER_SCAN_DELAY
                {                  
                    SYS_CONSOLE_MESSAGE("Scanning Started\r\n");
                    BLE_GAP_SetScanningEnable(true, BLE_GAP_SCAN_FD_DISABLE, BLE_GAP_SCAN_MODE_OBSERVER, 1200);   //mmr
                }
                else if(p_appMsg->msgId == APP_TIMER_ID_4_MSG)      //APP_TIMER_SCAN_DELAY
                { 
                    uint8_t formattedLength;
//                    char formattedBuffer[15];
                    if(alarmStat) 
                    {
                        strcpy(formattedBuffer, "AL:ON");
                        formattedLength = 5;
                    } 
                    else 
                    {
                        strcpy(formattedBuffer, "AL:OFF");
                        formattedLength = 6;
                    }
                    SYS_CONSOLE_PRINT("Al:%d\r\n", alarmStat);
                    APP_Msg_T appMsg;
                    appMsg.msgData[0] = formattedLength;
                    memcpy(&appMsg.msgData[1], formattedBuffer, formattedLength);
                    appMsg.msgId = APP_BLE_SEND_DISPLAY;
                    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                }
                else if(p_appMsg->msgId == APP_MSG_BLE_RECIEVE_CHAT_EVT)      //APP_TIMER_SCAN_DELAY
                {          
                    uint8_t buffer[160];
                    buffer[0]=p_appMsg->msgData[2]+4;
                    buffer[1]=0xB3;
                    memcpy(&buffer[2],p_appMsg->msgData,p_appMsg->msgData[2]+3);
                    uint16_t result=BLE_TRSPS_SendChat(APP_TRP_VENDOR_OPCODE_MRML,p_appMsg->msgData[2]+5,buffer);
                    if(result == MBA_RES_SUCCESS)
                    {
//                        SYS_CONSOLE_MESSAGE("Chat Send Success\r\n");
                    }
                    else
                    {
//                        SYS_CONSOLE_PRINT("Chat Send Failed: 0x%X\r\n", result);
                    }
                    char tempBuffer[15];
                    for (size_t i = 0; i < p_appMsg->msgData[2]; i++) 
                    {
                        tempBuffer[i] = (char)p_appMsg->msgData[3+i];
                    }
                    tempBuffer[p_appMsg->msgData[2]] = '\0';
                    // Define the substrings to search for
                    char *ltSubString = "LT:";
                    char *stSubString = "ST:";
                    char *drSubString = "DR:";
                    char *rmtSubString = "RMT>";
                    char *errSubString = "Err";
                    char *aokSubString = "AOK";
                    char *cmpstr = NULL;
                    APP_Msg_T appMsg;
                    // Check for "LT:"
                    cmpstr = strstr(tempBuffer, ltSubString);
                    if (cmpstr != NULL) 
                    {
                        cmpstr += strlen(ltSubString);
                        if (strstr(cmpstr, "ON") != NULL) 
                        {
                            appMsg.msgData[3]=0x01;
                        } 
                        else if (strncmp(cmpstr, "OFF", 3) == 0) 
                        {
                            appMsg.msgData[3]=0x00;
                        } 
                        else 
                        {
                        }
                        appMsg.msgId = APP_BLE_SET_LED_ONOFF;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    } 
                    // Check for "ST:"
                    cmpstr = strstr(tempBuffer, stSubString);
                    if (cmpstr != NULL)
                    {
                        cmpstr += strlen(stSubString);
                        char valueStr[10];
                        strncpy(valueStr, cmpstr, 9);
                        valueStr[9] = '\0'; 
                        int intValue = (int)(atof(valueStr) * 10);

                        appMsg.msgData[0] = (intValue >> 8) & 0xFF;
                        appMsg.msgData[1] = intValue & 0xFF; 
                        appMsg.msgId = APP_BLE_SET_TEMP_NTY;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    } 
                    // Check for "DR:"
                    cmpstr = strstr(tempBuffer, drSubString);
                    if (cmpstr != NULL) 
                    {
                        cmpstr += strlen(drSubString);
                        if (strncmp(cmpstr, "OPEN", 4) == 0) 
                        {
                            appMsg.msgData[0]=0x01;
                        } 
                        else if (strncmp(cmpstr, "CLOSE", 5) == 0)
                        {
                            appMsg.msgData[0]=0x00;
                        } 
                        else 
                        {
                            SYS_CONSOLE_PRINT("DR: Unknown value\n");
                        }
                        appMsg.msgId = APP_BLE_SET_DOOR_POS;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                    } 
                    // Check for "AOK"
                    cmpstr = strstr(tempBuffer, aokSubString);
                    if (cmpstr != NULL) 
                    {
                        SYS_CONSOLE_PRINT("AOK\r\n");
                        if(gpio_on_stat==LED_ONGNG)
                        {
                            gpio_on_stat=LED_SUCC;
                            alarmStat=true;
                            APP_TIMER_SetTimer(APP_TIMER_ID_7,APP_TIMER_3S,false);
                        }
                        if(gpio_off_stat==LED_ONGNG)
                        {
                            gpio_off_stat=LED_SUCC;
                            alarmOn=false;
                            alarmStat=false;
                        }
                    }
                    // Check for "Err"
                    cmpstr = strstr(tempBuffer, errSubString);
                    if (cmpstr != NULL) 
                    {
                        appMsg.msgId = APP_MSG_RMT_BLE_IO_ERR;
                        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                        SYS_CONSOLE_PRINT("Err Rmt\r\n");
                    }
                    // Check for "RMT>"
                    cmpstr = strstr(tempBuffer, rmtSubString);
                    if (cmpstr != NULL) 
                    {
                        if (entRmt==false) 
                        {
                            entRmt=true;
                            alarmOn=true;
                            APP_TIMER_SetTimer(APP_TIMER_ID_6,APP_TIMER_2S,false);
//                            SYS_CONSOLE_PRINT("IO ON\r\n");
                        }
                    }
                }
                else if(p_appMsg->msgId == APP_MSG_BLE_SCAN_EVT)
                {
                    uint16_t connStatus;
                    BLE_GAP_EvtAdvReport_T addrDevAddr;
                    BLE_GAP_CreateConnParams_T createConnParam_t;
                    if(APP_GetConnectedDevice_Count() <= (BLE_GAP_MAX_LINK_NBR-1) )// || (APP_Check_DeviceAddressByMAC(&addrDevAddr.addr) == false))
                    {
                        memcpy(&addrDevAddr, &p_appMsg->msgData, sizeof(BLE_GAP_EvtAdvReport_T));
                    
                        createConnParam_t.scanInterval = 0x3C; // 37.5 ms 
                        createConnParam_t.scanWindow = 0x1E; // 18.75 ms
                        createConnParam_t.filterPolicy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
                        createConnParam_t.peerAddr.addrType = addrDevAddr.addr.addrType;
                        memcpy(createConnParam_t.peerAddr.addr, addrDevAddr.addr.addr, GAP_MAX_BD_ADDRESS_LEN);
                        createConnParam_t.connParams.intervalMin = 0x50; // 20ms
                        createConnParam_t.connParams.intervalMax = 0x50; // 20ms
                        createConnParam_t.connParams.latency = 0;
                        createConnParam_t.connParams.supervisionTimeout = 0x48; // 720ms
                        connStatus = BLE_GAP_CreateConnection(&createConnParam_t);      //0x04->MBA_RES_NO_RESOURCE-Exceed maximum connections
                        if(connStatus == MBA_RES_SUCCESS)
                        {
                            SYS_CONSOLE_PRINT("Connecting to BLE Device: RSSI:%ddBm, \r\n", addrDevAddr.rssi);
                            extern APP_BLE_AdvRpt advData;
                            memcpy(advData.remoteAddr.addr,addrDevAddr.addr.addr,GAP_MAX_BD_ADDRESS_LEN);
                        }
                        else if(connStatus==0x10C)
                        {
                            APP_Msg_T appMsg;
                            appMsg.msgId = APP_MSG_BLE_SCAN_EVT;
                            memcpy(appMsg.msgData, &p_appMsg->msgData, sizeof(BLE_GAP_EvtAdvReport_T));
                            OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
                        }
                        else
                        {
                            SYS_CONSOLE_PRINT("Connecting to BLE Device: - Failed: 0x%X\r\n", connStatus);
                        }
                    }
                    else
                    {
                        USER_LED_Set();
                        BLE_GAP_SetScanningEnable(false, BLE_GAP_SCAN_FD_ENABLE, BLE_GAP_SCAN_MODE_GENERAL_DISCOVERY, 0);
                        SYS_CONSOLE_PRINT("Scanning Stopped: %d\r\n", APP_GetConnectedDevice_Count());
                        scanStart = true;
                        APP_TIMER_StopTimer(APP_TIMER_ADV_CTRL);
                        USER_LED_Set(); 
                        SYS_CONSOLE_PRINT("Scanning Time out\r\n");
                        uint16_t result=APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_MRML,SCAN_TIMEOUT_NFY);
                        if(result== 0)
                        {
                        }
                        else
                        {
                        }
                    }
                    
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
