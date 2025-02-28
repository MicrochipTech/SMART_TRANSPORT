/*******************************************************************************
  Application DFU Handler Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_dfu_handler.h

  Summary:
    This file contains the Application DFU Handler header file for this project.

  Description:
    This file contains the Application DFU Handler header file for this project.
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

#ifndef APP_DFU_HANDLER_H
#define APP_DFU_HANDLER_H

#ifdef APP_OTA_DFU_ENABLE
// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

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
typedef enum APP_DFU_HDL_Mode_T
{
    APP_DFU_MODE_IDLE   = 0x00,
    APP_DFU_MODE_OTA
} APP_DFU_HDL_Mode_T;

// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************

void APP_DFU_HDL_Prepare(uint16_t dfuHandle);
void APP_DFU_HDL_Start(void);
void APP_DFU_HDL_Updating(void);
void APP_DFU_HDL_Complete(void);
void APP_DFU_HDL_ErrorHandle(uint16_t dfuHandle);
void APP_DFU_HDL_Reset(uint16_t delaytime);
void APP_DFU_HDL_SetDfuMode(APP_DFU_HDL_Mode_T mode);
APP_DFU_HDL_Mode_T APP_DFU_HDL_GetDfuMode(void);
void APP_DFU_HDL_SoftwareReset(void);

#endif
#endif  // End of APP_DFU_HANDLER_H
/*******************************************************************************
 End of File
 */

