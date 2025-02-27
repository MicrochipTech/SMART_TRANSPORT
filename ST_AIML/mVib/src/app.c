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
#include "app_config.h"
#include "app_user.h"
#include "ringbuffer.h"
#include "definitions.h"
#include "sensor.h"
#include "definitions.h"
#include "app_ble.h"
#include "kb.h"
#include "sml_recognition_run.h"
#include "ble_trsps/ble_trsps.h"

#undef TRAINING_MODE_ENABLED
//#define TRAINING_MODE_ENABLED

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
uint16_t conn_hdl = 0xFFFF;

static struct sensor_device_t sensor;
static snsr_data_t _snsr_buffer_data[SNSR_BUF_LEN][SNSR_NUM_AXES];
static ringbuffer_t snsr_buffer;
static volatile bool snsr_buffer_overrun = false;
uint8_t prev_data[5]={0};
// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

static void SNSR_ISR_HANDLER() 
{
    
    APP_Msg_T appMsg_BMI160_EINT;
    appMsg_BMI160_EINT.msgId = APP_MSG_BMI160_SENSOR_INT;

    OSAL_QUEUE_SendISR(&appData.appQueue, &appMsg_BMI160_EINT);

}

void Null_Handler() {
    // Do nothing
}

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


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
    int8_t app_failed = 0;

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            //appData.appQueue = xQueueCreate( 10, sizeof(APP_Msg_T) );
#ifndef TRAINING_MODE_ENABLED
            kb_model_init();
#endif
            BSP_USER_LED_Off();
            BSP_RED_LED_Off();
            BSP_GREEN_LED_Off();
            BSP_BLUE_LED_Off();
            APP_BleStackInit();
            BLE_GAP_SetAdvEnable(0x01, 0);
            printf("Motor_Vibration_Detector\r\n");
            MIKRO_INT_CallbackRegister(Null_Handler);
            MIKRO_INT_Disable();
            
            /* Initialize the sensor data buffer */
            if (ringbuffer_init(&snsr_buffer, _snsr_buffer_data, sizeof(_snsr_buffer_data) / sizeof(_snsr_buffer_data[0]), sizeof(_snsr_buffer_data[0])))
            {        
                app_failed = 1;
            }

            /* Init and configure sensor */
            if (sensor_init(&sensor) != SNSR_STATUS_OK) {
                printf("ERROR: sensor init result = %d\n\r", sensor.status);
                app_failed = 1;
            }

            if (sensor_set_config(&sensor) != SNSR_STATUS_OK) {
                printf("ERROR: sensor configuration result = %d\n\r", sensor.status);
                app_failed = 1;
            }

            printf("sensor type is %s\n\r", SNSR_NAME);
            printf("sensor sample rate set at %dHz\n\r", SNSR_SAMPLE_RATE);
            
            #if SNSR_USE_ACCEL
                printf("Accelerometer axes %s%s%s enabled with range set at +/-%dGs\n\r", SNSR_USE_ACCEL_X ? "x" : "", SNSR_USE_ACCEL_Y ? "y" : "", SNSR_USE_ACCEL_Z ? "z" : "", SNSR_ACCEL_RANGE);
            #else
                printf("Accelerometer disabled\n\r");
            #endif

            #if SNSR_USE_GYRO
                printf("Gyrometer axes %s%s%s enabled with range set at %dDPS\n\r", SNSR_USE_GYRO_X ? "x" : "", SNSR_USE_GYRO_Y ? "y" : "", SNSR_USE_GYRO_Z ? "z" : "", SNSR_GYRO_RANGE);
            #else
                printf("Gyrometer disabled\n\r");
            #endif

            
            MIKRO_INT_Enable();
            vTaskDelay(pdMS_TO_TICKS(20)+1);
            MIKRO_INT_CallbackRegister(SNSR_ISR_HANDLER);
            
            if (appInitialized && !app_failed)    
            {
                p_appMsg->msgId = APP_MSG_BMI160_SENSOR_READ;
                OSAL_QUEUE_Send(&appData.appQueue, p_appMsg, 0);
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
                else if(p_appMsg->msgId==APP_MSG_BLE_STACK_LOG)
                {
                    // Pass BLE LOG Event Message to User Application for handling
                    APP_BleStackLogHandler((BT_SYS_LogEvent_T *)p_appMsg->msgData);
                }
                else if(p_appMsg->msgId==APP_MSG_BMI160_SENSOR_INT)
                {
                        /* Check if any errors we've flagged have been acknowledged */
                        if ((sensor.status != SNSR_STATUS_OK) || snsr_buffer_overrun)
                            return;

                        ringbuffer_size_t wrcnt;
                        snsr_data_t *ptr = ringbuffer_get_write_buffer(&snsr_buffer, &wrcnt);

                        if (wrcnt == 0)
                            snsr_buffer_overrun = true;
                        else if ((sensor.status = sensor_read(&sensor, ptr)) == SNSR_STATUS_OK)
                            ringbuffer_advance_write_index(&snsr_buffer, 1);
                }
                else if(p_appMsg->msgId==APP_MSG_BMI160_SENSOR_READ)
                {
                    if (sensor.status != SNSR_STATUS_OK)
                    {
                        printf("ERROR: Got a bad sensor status: %d\n", sensor.status);
                    }
                    else if (snsr_buffer_overrun == true)
                    {
                        printf("\n\n\nOverrun!\n\n\n");
                        MIKRO_INT_CallbackRegister(Null_Handler);
                        ringbuffer_reset(&snsr_buffer);
                        snsr_buffer_overrun = false;
                        MIKRO_INT_CallbackRegister(SNSR_ISR_HANDLER);
                    }
                    else if(ringbuffer_get_read_items(&snsr_buffer) >= SNSR_SAMPLES_PER_PACKET)
                    {
                        ringbuffer_size_t rdcnt;
                        int32_t ret;
                        SNSR_DATA_TYPE const *ptr = ringbuffer_get_read_buffer(&snsr_buffer, &rdcnt);
                        while (rdcnt >= SNSR_SAMPLES_PER_PACKET) 
                        {
#ifdef  TRAINING_MODE_ENABLED  
                            snsr_data_t const *scalarptr = (snsr_data_t const *) ptr;
                            SNSR_DATA_TYPE x = (*scalarptr++);
                            SNSR_DATA_TYPE y = (*scalarptr++);
                            SNSR_DATA_TYPE z = (*scalarptr++);
//                            SNSR_DATA_TYPE gx = (*scalarptr++);
//                            SNSR_DATA_TYPE gy = (*scalarptr++);
//                            SNSR_DATA_TYPE gz = (*scalarptr++);                          
//                            printf("%d,%d,%d,%d,%d,%d\r\n", x,y,z,gx,gy,gz);
//                            printf("    %d,    %d,    %d,\r\n", x,y,z);
                            uint8_t headerbyte = MDV_START_OF_FRAME;
                            SNSR_DATA_TYPE tempBuff[SNSR_NUM_AXES];
                            tempBuff[0] = x;
                            tempBuff[1] = y;
                            tempBuff[2] = z;
//                            tempBuff[3] = gx;
//                            tempBuff[4] = gy;
//                            tempBuff[5] = gz;
                            SERCOM0_USART_Write(&headerbyte, 1);
                            SERCOM0_USART_Write(tempBuff, sizeof(tempBuff));
                            headerbyte = ~headerbyte;
                            SERCOM0_USART_Write(&headerbyte, 1);
#else
//                            printf("dataP:%d n:%d\r\n",sizeof(snsr_datapacket_t),SNSR_NUM_AXES);
//                            printf("rdcnt: %d\r\n", rdcnt);

                            ret = sml_recognition_run((snsr_data_t *)ptr++, SNSR_NUM_AXES);
                            if (ret >= 0)
                            {
                                p_appMsg->msgId = APP_MSG_ML_PROCESS;
                                p_appMsg->msgData[0] = ret;
                                OSAL_QUEUE_Send(&appData.appQueue, p_appMsg, 0);
                            }
#endif
                            rdcnt -= SNSR_SAMPLES_PER_PACKET;
                            ringbuffer_advance_read_index(&snsr_buffer, SNSR_SAMPLES_PER_PACKET);
                        }
                    }
                    p_appMsg->msgId = APP_MSG_BMI160_SENSOR_READ;
                    OSAL_QUEUE_Send(&appData.appQueue, p_appMsg, 0);
                }
                else if(p_appMsg->msgId==APP_TIMER_ID_0_MSG)
                {
                    uint8_t count[4] = {0};
                    for (int i = 0; i < 5; i++) 
                    {
                        if (prev_data[i] < 4) 
                        {
                            count[prev_data[i]]++;
                        }
                    }
                    uint8_t max_value = 0;
                    uint8_t max_count = count[0];
                    for (int i = 1; i < 4; i++) 
                    {
                        if (count[i] > max_count) 
                        {
                            max_count = count[i];
                            max_value = i;
                        }
                    }
                    uint8_t payload[3]={0x02,0x47,0x00};
                    payload[2]=max_value;
                    BLE_TRSPS_SendVendorCommand(conn_hdl,0x8C,3,payload);
                    printf("Sent\r\n");
                }
                else if(p_appMsg->msgId==APP_MSG_ML_PROCESS)
                {
                    char temp_buff[15];
                    static uint8_t ind=0;
                    if(p_appMsg->msgData[0] == 2)
                    {
                        sprintf(temp_buff, "Motor Off\r\n");
                        printf("%s", temp_buff);
                        BSP_RED_LED_Off();
                        BSP_GREEN_LED_Off();
                        BSP_BLUE_LED_On();
                    }
                    else if(p_appMsg->msgData[0] == 3)
                    {
                        sprintf(temp_buff, "Motor On\r\n");
                        printf("%s", temp_buff);
                        BSP_RED_LED_Off();
                        BSP_GREEN_LED_On();
                        BSP_BLUE_LED_Off();
                    }
                    else if(p_appMsg->msgData[0] == 1)
                    {
                        sprintf(temp_buff, "Blade\r\n");
                        printf("%s", temp_buff);
                        BSP_RED_LED_On();
                        BSP_GREEN_LED_Off();
                        BSP_BLUE_LED_Off();
                    }
                    else
                    {
                        printf("Unknown:%d\r\n",p_appMsg->msgData[0]);
                        sprintf(temp_buff, "Anomaly: %d\r\n", p_appMsg->msgData[0]);
                        printf("%s", temp_buff);
                        BSP_RED_LED_On();
                        BSP_GREEN_LED_On();
                        BSP_BLUE_LED_On();
                    }
                    if(conn_hdl != 0xFFFF)
                    {
                        if(ind<=9)
                        {
                            prev_data[ind]=p_appMsg->msgData[0];
                            ind++;
                        }
                        else
                        {
                            ind=0;
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
