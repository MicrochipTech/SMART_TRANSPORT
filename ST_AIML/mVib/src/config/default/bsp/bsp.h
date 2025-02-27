/*******************************************************************************
  Board Support Package Header File.

  Company:
    Microchip Technology Inc.

  File Name:
    bsp.h

  Summary:
    Board Support Package Header File 

  Description:
    This file contains constants, macros, type definitions and function
    declarations 
*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2023 Microchip Technology Inc. and its subsidiaries.
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

#ifndef BSP_H
#define BSP_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "device.h"
#include "peripheral/gpio/plib_gpio.h"

// *****************************************************************************
// *****************************************************************************
// Section: BSP Macros
// *****************************************************************************
// *****************************************************************************
#define WBZ451_CURIOSITY
#define BOARD_NAME    "WBZ451-CURIOSITY"

/*** OUTPUT PIO Macros for RED_LED ***/
#define BSP_RED_LED_PIN        GPIO_PIN_PB0
#define BSP_RED_LED_Get()      ((GPIOB_REGS->GPIO_PORT >> 0) & 0x1)
#define BSP_RED_LED_On()       (GPIOB_REGS->GPIO_LATSET = (1UL<<0))
#define BSP_RED_LED_Off()      (GPIOB_REGS->GPIO_LATCLR = (1UL<<0))
#define BSP_RED_LED_Toggle()   (GPIOB_REGS->GPIO_LATINV = (1UL<<0))

/*** OUTPUT PIO Macros for GREEN_LED ***/
#define BSP_GREEN_LED_PIN        GPIO_PIN_PB3
#define BSP_GREEN_LED_Get()      ((GPIOB_REGS->GPIO_PORT >> 3) & 0x1)
#define BSP_GREEN_LED_On()       (GPIOB_REGS->GPIO_LATSET = (1UL<<3))
#define BSP_GREEN_LED_Off()      (GPIOB_REGS->GPIO_LATCLR = (1UL<<3))
#define BSP_GREEN_LED_Toggle()   (GPIOB_REGS->GPIO_LATINV = (1UL<<3))

/*** OUTPUT PIO Macros for BLUE_LED ***/
#define BSP_BLUE_LED_PIN        GPIO_PIN_PB5
#define BSP_BLUE_LED_Get()      ((GPIOB_REGS->GPIO_PORT >> 5) & 0x1)
#define BSP_BLUE_LED_On()       (GPIOB_REGS->GPIO_LATSET = (1UL<<5))
#define BSP_BLUE_LED_Off()      (GPIOB_REGS->GPIO_LATCLR = (1UL<<5))
#define BSP_BLUE_LED_Toggle()   (GPIOB_REGS->GPIO_LATINV = (1UL<<5))

/*** OUTPUT PIO Macros for USER_LED ***/
#define BSP_USER_LED_PIN        GPIO_PIN_PB7
#define BSP_USER_LED_Get()      ((GPIOB_REGS->GPIO_PORT >> 7) & 0x1)
#define BSP_USER_LED_On()       (GPIOB_REGS->GPIO_LATCLR = (1UL<<7))        //sankar
#define BSP_USER_LED_Off()      (GPIOB_REGS->GPIO_LATSET = (1UL<<7))
#define BSP_USER_LED_Toggle()   (GPIOB_REGS->GPIO_LATINV = (1UL<<7))




// *****************************************************************************
// *****************************************************************************
// Section: Interface Routines
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Function:
    void BSP_Initialize(void)

  Summary:
    Performs the necessary actions to initialize a board

  Description:
    This function initializes the LED and Switch ports on the board.  This
    function must be called by the user before using any APIs present on this
    BSP.

  Precondition:
    None.

  Parameters:
    None

  Returns:
    None.

  Example:
    <code>
    BSP_Initialize();
    </code>

  Remarks:
    None
*/

void BSP_Initialize(void);

#endif // BSP_H

/*******************************************************************************
 End of File
*/
