/*******************************************************************************
  Application BLE Sensor Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_sensor.c

  Summary:
    This file contains the Application Transparent Server Role functions for this project.

  Description:
    This file contains the Application Transparent Server Role functions for this project.
    The implementation of demo mode is included in this file.
 *******************************************************************************/

//DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
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
//DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>
#include "app_trpc.h"
#include "app_ble.h"
#include "Sensors/inc/rgb_led.h"
#include "Sensors/inc/temp_sensor.h"
#include "app_timer/app_timer.h"
#include "peripheral/eic/plib_eic.h"
#include "system/console/sys_console.h"
#include "app.h"
#include "app_adv.h"
#include "app_ble_conn_handler.h"
#include "app_ble_sensor.h"
#include "app_error_defs.h"
#include "definitions.h"

// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************

/**@brief BLE sensor read periodic timer */
uint32_t bleSensTimer = 0;
APP_TRPS_SensorData_T bleSensorData = {LED_OFF,{GREEN_H,GREEN_S,GREEN_V},{0,0}};
APP_TRPS_SensorData_T bleSensorPeriDevData = {LED_OFF,{GREEN_H,GREEN_S,GREEN_V},{0,0}};
float lastNotifiedTemp = -50.0, lastAdvTemp = -50.0;
bool b_button_debounce = false;

// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************
static uint8_t APP_TRPS_Sensor_LED_Ctrl(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Sensor_LED_Ctrl(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Sensor_LED_GetStatus(uint8_t *p_cmd);
static uint8_t APP_TRPS_Sensor_Color_Ctrl(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Sensor_RGB_Color_GetStatus(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Sensor_Color_Ctrl(uint8_t *p_cmd);
static void APP_TRPS_Sensor_Button_Callback(uintptr_t context);
static uint8_t APP_TRPS_MRML_Update(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Discover(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Disconnect(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Chat_Send(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_Chat_Print(uint8_t *p_cmd);
static uint8_t APP_TRPS_MRML_GetConn(uint8_t *p_cmd);
APP_TRPS_Temperature smartTruckThermostat = { 0,0,false , {0,0},{0,0}};
APP_TRPS_Solar smartSolarPanel;

APP_TRPS_CmdResp_T appTrpsSensorCmdResp[] = 
{
    BLE_SENSOR_DEFINE_CTRL_CMD_RESP()
};

APP_TRPS_NotifyData_T appTrpsSensorNotify[] = 
{
    BLE_SENSOR_DEFINE_CTRL_NOTIFY()
};

APP_TRPS_CmdResp_T appTrpsMRMLCmdResp[] = 
{
    MRML_DEFINE_CTRL_CMD_RESP()
};

APP_TRPS_NotifyData_T appTrpsMRMLNotify[] = 
{
    MRML_DEFINE_CTRL_NOTIFY()
};

APP_TRPS_CmdResp_T appTrpsSTCmdResp[] = 
{
    ST_DEFINE_CTRL_CMD_RESP()
};

APP_TRPS_NotifyData_T appTrpsSTNotify[] = 
{
    ST_DEFINE_CTRL_NOTIFY()
};


//static uint8_t APP_TRPS_Temperature_Ctrl(uint8_t *p_cmd)
//{
////    uint16_t set_temperature =  ((p_cmd[3]<<8)|p_cmd[4]);
//    smartTruckThermostat.hvacStatus=p_cmd[3];
//    smartTruckThermostat.setTemp.lsb=p_cmd[4];
//    smartTruckThermostat.setTemp.msb=p_cmd[5];
//    smartTruckThermostat.currTemp.lsb=p_cmd[6];
//    smartTruckThermostat.currTemp.msb=p_cmd[7];
////    APP_Msg_T appMsg;
////    appMsg.msgId = APP_BLE_SET_RGB_COLOR;
////    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
//    SYS_CONSOLE_PRINT("[BLE] Set point %d\n\r",((p_cmd[4]<<8)|p_cmd[5]));
//    SYS_CONSOLE_PRINT("[BLE] Current temp %d\n\r",((p_cmd[6]<<8)|p_cmd[7]));
//    return SUCCESS;
//}

/* BLE Sensor RGB LED On/Off control through Mobile app */
static uint8_t APP_TRPS_Sensor_LED_Ctrl(uint8_t *p_cmd)
{
    if (p_cmd[3] == LED_OFF)
    {
        RGB_LED_Off();
        bleSensorData.rgbOnOffStatus = LED_OFF;
        SYS_CONSOLE_MESSAGE("[BLE] LED OFF\n\r");                
    }    
    else if (p_cmd[3] == LED_ON)
    {
        if(bleSensorData.rgbOnOffStatus == LED_OFF)
            RGB_LED_SetLedColorHSV(bleSensorData.RGB_color.Hue,bleSensorData.RGB_color.Saturation,bleSensorData.RGB_color.Value);
        bleSensorData.rgbOnOffStatus = LED_ON;
        SYS_CONSOLE_MESSAGE("[BLE] LED ON\n\r");                
    }
    return SUCCESS;
}

/* BLE Sensor RGB LED On/Off control through Mobile app */
static uint8_t APP_TRPS_MRML_Sensor_LED_Ctrl(uint8_t *p_cmd)
{
    SYS_CONSOLE_MESSAGE("[BLE] Sensor LED ctrl request\r\n");  
    APP_Msg_T appMsg;
    appMsg.msgData[0]=0x10;
    appMsg.msgData[1]=p_cmd[3];
    appMsg.msgData[2]=p_cmd[4];
    appMsg.msgData[3]=p_cmd[5];
    appMsg.msgId = APP_BLE_SET_LED_ONOFF;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    return SUCCESS;
}

/* MRML Discover command through Mobile app */
static uint8_t APP_TRPS_MRML_Discover(uint8_t *p_cmd)
{
    SYS_CONSOLE_MESSAGE("[BLE] Discover request\r\n");  
    APP_Msg_T appMsg;
    appMsg.msgId = APP_MSG_BLE_SCAN_START_EVT;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
//    APP_TIMER_SetTimer(APP_TIMER_SCAN_DELAY, 10000, false);
    return SUCCESS;
}

extern uint8_t APP_GetConnectedDevice_Count(void);

/* MRML Discover command through Mobile app */
static uint8_t APP_TRPS_MRML_GetConn(uint8_t *p_cmd)
{
    uint8_t periDeviceCount=APP_GetConnectedDevice_Count();
    if(periDeviceCount!=0)
    {
        APP_Msg_T appMsg;
        appMsg.msgId = APP_MSG_BLE_GET_CONN_EVT;
//        appMsg.msgData[0]=periDeviceCount;
        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    }
    return SUCCESS;
}

static uint8_t APP_TRPS_MRML_Chat_Send(uint8_t *p_cmd)
{
    SYS_CONSOLE_MESSAGE("[BLE] Chat Send request\r\n");  
    uint16_t connHandle = (uint16_t)((p_cmd[3]<<8) | p_cmd[4]);
    uint16_t result=BLE_TRSPC_SendData(connHandle,p_cmd[5],(uint8_t *)&p_cmd[6]);
    if(result == MBA_RES_SUCCESS)
    {
        SYS_CONSOLE_MESSAGE("Chat Send Success\r\n");
    }
    else
    {
        SYS_CONSOLE_PRINT("Chat Send to Conn hdl: 0x%x Failed: 0x%X\r\n",connHandle, result);
    }
    return SUCCESS;
}

static uint8_t APP_TRPS_MRML_Chat_Print(uint8_t *p_cmd)
{
    SYS_CONSOLE_MESSAGE("[BLE] Chat Send request\r\n");  
//    uint16_t connHandle = (uint16_t)((p_cmd[3]<<8) | p_cmd[4]);
    uint8_t dataLenIndex = 5;
    uint8_t dataLen = p_cmd[dataLenIndex];
    SYS_CONSOLE_PRINT("TRPC Data Sent \r\n");
    for (int i = dataLenIndex + 1; i <= dataLenIndex + dataLen; i++) 
    {
        SYS_CONSOLE_PRINT("%c", p_cmd[i]);
    }
    SYS_CONSOLE_PRINT("\r\n");
    return SUCCESS;
}


/* MRML Update command through Mobile app */
static uint8_t APP_TRPS_MRML_Update(uint8_t *p_cmd)
{
    SYS_CONSOLE_MESSAGE("[BLE] Update request\r\n");  
    APP_Msg_T appMsg;
    appMsg.msgData[0]=p_cmd[3];
    appMsg.msgData[1]=p_cmd[4];
    appMsg.msgId = APP_BLE_SEND_UPDATE;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    return SUCCESS;
}

static uint8_t APP_TRPS_MRML_Disconnect(uint8_t *p_cmd)
{
    SYS_CONSOLE_PRINT("[BLE] Disconnect request:%x\r\n",((p_cmd[3]<<8) | p_cmd[4]));  
    uint16_t result=BLE_GAP_Disconnect(((p_cmd[3]<<8) | p_cmd[4]),GAP_STATUS_LOCAL_HOST_TERMINATE_CONNECTION);
    if(result==0)
    {
        return SUCCESS;
    }
    else
    {
        return OPERATION_FAILED;
    }
}


static uint8_t APP_TRPS_MRML_Sensor_LED_GetStatus(uint8_t *p_cmd)
{
    APP_Msg_T appMsg;
    appMsg.msgData[0]=p_cmd[3];
    appMsg.msgData[1]=p_cmd[4];
    SYS_CONSOLE_PRINT("[BLE] LED Get Status request:%x\r\n",((p_cmd[3]<<8) | p_cmd[4]));
    appMsg.msgId = APP_BLE_GET_LED_ONOFF;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    return SUCCESS;
}

static uint8_t APP_TRPS_MRML_Sensor_RGB_Color_GetStatus(uint8_t *p_cmd)
{
    APP_Msg_T appMsg;
    appMsg.msgData[0]=p_cmd[3];
    appMsg.msgData[1]=p_cmd[4];
    SYS_CONSOLE_PRINT("[BLE] RGB Color Get Status request:%x\r\n",((p_cmd[3]<<8) | p_cmd[4]));
    appMsg.msgId = APP_BLE_GET_RGB_COLOR;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    return SUCCESS;
}

static uint8_t APP_TRPS_MRML_Sensor_Color_Ctrl(uint8_t *p_cmd)
{
    SYS_CONSOLE_MESSAGE("[BLE] Sensor LED Color ctrl request\r\n");  
    APP_Msg_T appMsg;
    appMsg.msgData[0]=0x12;
    appMsg.msgData[1]=p_cmd[3];
    appMsg.msgData[2]=p_cmd[4];
    appMsg.msgData[3]=p_cmd[5];
    appMsg.msgData[4]=p_cmd[6];
    appMsg.msgData[5]=p_cmd[7];
    SYS_CONSOLE_PRINT("H: %x \r\n S: %x \r\n V: %x \r\n ",appMsg.msgData[3],appMsg.msgData[4],appMsg.msgData[5]);
    appMsg.msgId = APP_BLE_SET_RGB_COLOR;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    return SUCCESS;
}


/* Callback for BLE Sensor LED on/off through on board button */
static void APP_TRPS_Sensor_Button_Callback(uintptr_t context)
{
    APP_Msg_T appMsg;
    
    if(!b_button_debounce)
    {
        appMsg.msgId = APP_MSG_TRS_BLE_SENSOR_INT;

        OSAL_QUEUE_SendISR(&appData.appQueue, &appMsg);
        
        b_button_debounce = true;
    }    
}

/* BLE Sensor LED on/off control through on board button */
void APP_TRPS_Sensor_Button_Handler(void)
{
    b_button_debounce = false;
    
    if(bleSensorData.rgbOnOffStatus == LED_OFF)
    {
        RGB_LED_SetLedColorHSV(bleSensorData.RGB_color.Hue,bleSensorData.RGB_color.Saturation,bleSensorData.RGB_color.Value);
        bleSensorData.rgbOnOffStatus = LED_ON;
        SYS_CONSOLE_MESSAGE("[BLE] Button LED ON\n\r");
    }
    else
    {
        RGB_LED_Off();
        bleSensorData.rgbOnOffStatus = LED_OFF;
        SYS_CONSOLE_MESSAGE("[BLE] Button LED OFF\n\r");
    }
        
    if ( APP_GetBleState() == APP_BLE_STATE_CONNECTED)
    {
        APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_SMART_TRUCK,RGB_ONOFF_STATUS_NFY);
    }
    else
    {       
        APP_ADV_Init();  
    }
}

/* BLE Sensor RGB LED color control through Mobile app */
static uint8_t APP_TRPS_Sensor_Color_Ctrl(uint8_t *p_cmd)
{
    bleSensorData.RGB_color.Hue =  p_cmd[3];
    bleSensorData.RGB_color.Saturation =  p_cmd[4];
    bleSensorData.RGB_color.Value =  p_cmd[5];
    if(bleSensorData.rgbOnOffStatus == LED_ON)
    {    
        RGB_LED_SetLedColorHSV(bleSensorData.RGB_color.Hue,bleSensorData.RGB_color.Saturation,bleSensorData.RGB_color.Value);
        SYS_CONSOLE_MESSAGE("[BLE] COLOR SET\n\r");        
    }
    return SUCCESS;
}

/* Periodic 100ms once timer handler to read sensor data*/
void APP_TRPS_Sensor_TimerHandler(void)
{
    volatile float temperature=0, tempBack;
    uint16_t tempS = 0;
	//bleSensTimer++;

    //if( bleSensTimer%10)  // temp sensor read every 1sec once
	{
        tempBack = temperature = MCP9700_Temp_Celsius();
        
        if(temperature < 0)
        {
            temperature = temperature * (-1.0);
        
            tempS = (uint16_t) (temperature * 10);  // 1 decimal place
            
            tempS = tempS | 0x8000;  // Set the MSB to indicate negative temperature          
        }
        else
        {       
            tempS = (uint16_t) (temperature * 10);  // 1 decimal place         
        }
        
        bleSensorData.tempSens.lsb = (uint8_t) tempS;
        bleSensorData.tempSens.msb = (uint8_t) (tempS>>8);  

        if ( APP_GetBleState() == APP_BLE_STATE_CONNECTED){
            if( (tempBack > (lastNotifiedTemp + 1)) || (tempBack < (lastNotifiedTemp-1)) )  //+/- 1�C above, only then do the notification
            {
                if(APP_TRPS_SendNotification(APP_TRP_VENDOR_OPCODE_SMART_TRUCK,TEMP_SENSOR_NFY)== APP_RES_SUCCESS)
                {
                    lastNotifiedTemp = tempBack;
                    SYS_CONSOLE_PRINT("[BLE] Temperature Notified: %.2f DegC\n\r",lastNotifiedTemp);
                }    
            }
        }
        else
        {
            if( (tempBack > (lastAdvTemp + 1)) || (tempBack < (lastAdvTemp-1)) )  //+/- 1�C above, only then update advertisement payload
            {       
                APP_ADV_Init();
                lastAdvTemp = tempBack;                
            }
        }
    }
}

/* Init BLE Sensor Specific */
void APP_TRPS_Sensor_Init(void)
{
    /* Init TRPS profile with BLE sensor specific command structure*/
//    APP_TRPS_Init(APP_TRP_VENDOR_OPCODE_BLE_SENSOR,appTrpsSensorCmdResp,appTrpsSensorNotify,BLE_SENSOR_CMD_RESP_LST_SIZE,BLE_SENSOR_NOTIFY_LST_SIZE);
    
    /* Init TRPS profile with BLE sensor specific command structure*/
    APP_TRPS_Init(APP_TRP_VENDOR_OPCODE_MRML, appTrpsMRMLCmdResp,appTrpsMRMLNotify,MRML_CMD_RESP_LST_SIZE,MRML_NOTIFY_LST_SIZE);
    
    /* Init TRPS profile with BLE sensor specific command structure*/
    APP_TRPS_Init(APP_TRP_VENDOR_OPCODE_SMART_TRUCK, appTrpsMRMLCmdResp,appTrpsMRMLNotify,ST_CMD_RESP_LST_SIZE,ST_CMD_RESP_LST_SIZE);
    
    /* Init Periodic application timer to do BLE sensor specific measurement like read temp sensor handled in APP_TRPS_Sensor_TimerHandler() */
    APP_TIMER_SetTimer(APP_TIMER_BLE_SENSOR, APP_TIMER_1S, true);
    
    /* Register external button interrupt callback   */    
    EIC_CallbackRegister(BUTTON_1,APP_TRPS_Sensor_Button_Callback,0);
}

/* Do the BLE Sensor specific on disconnection  */
void APP_TRPS_Sensor_DiscEvtProc(void)
{
    lastNotifiedTemp = -50.0;    
}

/* Fill Adv Beacon with BLE Sensor specific */
void APP_TRPS_Sensor_Beacon(uint8_t* ptr_data)
{ 
    uint8_t idx=0;
    //Service Data
    ptr_data[idx++] = APP_ADV_ADD_DATA_CLASS_BYTE;
    ptr_data[idx++] = APP_ADV_PROD_TYPE_BLE_SENSOR_MRML;//APP_ADV_PROD_TYPE_BLE_SENSOR;
    ptr_data[idx++] = RGB_ONOFF_STATUS_NFY;
    ptr_data[idx++] =  bleSensorData.rgbOnOffStatus;
    ptr_data[idx++] = TEMP_SENSOR_NFY;
    ptr_data[idx++] =  bleSensorData.tempSens.msb; // MSB
    ptr_data[idx++] =  bleSensorData.tempSens.lsb;  // LSB     
}