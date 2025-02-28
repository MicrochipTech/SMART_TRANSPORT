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

/*******************************************************************************
  Middleware Device Firmware Udpate Header File

  Company:
    Microchip Technology Inc.

  File Name:
    mw_dfu.h

  Summary:
    This file contains the BLE Device Firmware Udpate functions for application user.

  Description:
    This file contains the BLE Device Firmware Udpate functions for application user.
 *******************************************************************************/


/**
 * @addtogroup MW_DFU
 * @{
 * @brief Header file for the Middleware Device Firmware Update library.
 * @note Definitions and prototypes for the Middleware Device Firmware Update stack layer application programming interface.
 */

#ifndef MW_DFU_H
#define MW_DFU_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

//#define MW_DFU_EXTERNAL_FLASH_ENABLE        /* Enable External Flash */

/**@addtogroup MW_DFU_DEFINES Defines
 * @{ */

/**@defgroup MW_DFU_MAX_IMAGE_SIZE Maximum image size
 * @brief The definition of DFU maximum image size.
 * @{ */
#define MW_DFU_MAX_SIZE_FW_IMAGE_INT          (262144UL)           /**< Maximum size of firmware image for internal flash in bytes. */
#define MW_DFU_MAX_SIZE_FW_IMAGE_EXT          (524288UL)           /**< Maximum size of firmware image for external flash in bytes. */
/** @} */


/**@defgroup MW_DFU_MAX_BLOCK_LEN Maximum block len
 * @brief The definition of maximum block length.
 * @{ */
#define MW_DFU_MAX_BLOCK_LEN                   (0x400U)           /**< Maximum block length. */
/** @} */

/**@defgroup MW_DFU_FLASH_TYPE Flash type
 * @brief The definition of DFU flash type.
 * @{ */
#define MW_DFU_FLASH_INTERNAL                 (0x01U)            /**< DFU to internal flash. */
#define MW_DFU_FLASH_EXTERNAL                 (0x03U)            /**< DFU to external flash. */
/** @} */


/**@} */ //MW_DFU_DEFINES


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
/**@addtogroup MW_DFU_STRUCTS Structures
 * @{ */

/**@brief Structure of DFU info which is used to indicate the image information. */
typedef struct MW_DFU_Info_T
{
    uint32_t fwImageSize;                                      /**< Firmware image size: @ref MW_DFU_MAX_IMAGE_SIZE. It must be 16-bytes aligned. */
    uint8_t  fwFlashType;                                      /**< Firmware flash type: @ref MW_DFU_FLASH_TYPE */
} MW_DFU_Info_T;

/**@} */ //MW_DFU_STRUCTS

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
/**@addtogroup MW_DFU_FUNS Functions
 * @{ */

/**
 *@brief The API is used to configure the device information for DFU process.
 *
 *@param[in] p_dfuInfo            Pointer to the structure of the image information for DFU. Refer to @ref MW_DFU_Info_T for detail structure info.
 *
 *
 *@retval MBA_RES_SUCCESS         Configure successfully.
 *@retval MBA_RES_INVALID_PARA    Invalid parameters. One of the following reasons:\n
 *                                - The image size exceeds the maximum image size.\n
 *                                - The image size is not 16-bytes aligned.\n
 *                                - Invalid flash type. See @ref MW_DFU_FLASH_TYPE for valid values.\n
 *@retval MBA_RES_FAIL            Failed to access external flash.
 */
uint16_t MW_DFU_Config(MW_DFU_Info_T *p_dfuInfo);

/**
 *@brief The API is used to start or restart firmware image update procedure. 
 *       The state machine and parameters of this module would be reset after this API is called. 
 *
 *
 *@retval MBA_RES_SUCCESS         Start or restart firmware image update procedure successfully.
 *@retval MBA_RES_BAD_STATE       This API cannot be executed in current DFU process state.
 *@retval MBA_RES_FAIL            Failed to access external flash.
 */
uint16_t MW_DFU_FwImageStart(void);

/**
 *@brief The API is used to update the fragment of firmware image to flash. 
 *       The API should be called multiple times to udpate all fragments of firmware image to flash.
 *
 *@param[in] length               The length of image fragment to update, unit: byte. It must be 16-bytes aligned for internal flash.
 *                                For external flash, it must be 256-bytes aligned unless it is the last fragment.
 *@param[in] p_content            Pointer to the image fragment.
 *
 *
 *@retval MBA_RES_SUCCESS         Update the fragment of firmware image successfully.
 *@retval MBA_RES_INVALID_PARA    Invalid parameters. The length exceeds the image size or length exceeds @ref MW_DFU_MAX_BLOCK_LEN or length does not meet aligned requirement. 
 *@retval MBA_RES_BAD_STATE       This API cannot be executed in current DFU process state.
 *@retval MBA_RES_OOM             No available memory.
 *@retval MBA_RES_FAIL            Fail to update fragment to flash.
 */
uint16_t MW_DFU_FwImageUpdate(uint16_t length, uint8_t *p_content);

/**
 *@brief The API is used to validate the udpated MCU image. Only supports CRC validation currently. 
 *
 *@param[in] fwCfmValue           For internal flash, it should be firmware image crc value. For external flash, it should be firmware image checksum value.
 *
 *@return MBA_RES_SUCCESS         Validation successfully.
 *@return MBA_RES_FAIL            Validation failure.
 */
uint16_t MW_DFU_FwImageValidate(uint16_t fwCfmValue);


/**
 *@brief The API is used to activate the new firmware after system reboot.
 *
 *
 *@return MBA_RES_SUCCESS         Activation successfully.
 *@return MBA_RES_BAD_STATE       This API cannot be executed in current DFU process state.
 *@retval MBA_RES_OOM             No available memory.
 *@retval MBA_RES_FAIL            Failed to access flash.
 */
uint16_t MW_DFU_FwImageActivate(void);


/**
 *@brief The API is used to read the fragment of firmware image from flash. 
 *@note  Before the new firmware image is activated, the first 16 bytes image fragment read from flash is invalid.
 *
 *
 *@param[in] offset               The flash offset to start reading image fragment.
 *@param[in] length               The length of image fragment to read, unit: byte.
 *@param[in] p_content            Pointer to the image fragment buffer.
 *
 *
 *@retval MBA_RES_SUCCESS         Read the fragment of firmware image successfully.
 *@retval MBA_RES_INVALID_PARA    Invalid parameters. The offset + length exceed @ref MW_DFU_MAX_SIZE_FW_IMAGE or length exceed @ref MW_DFU_MAX_BLOCK_LEN. 
 *@retval MBA_RES_FAIL            Failed to access flash.
 */
uint16_t MW_DFU_FwImageRead(uint32_t offset, uint16_t length, uint8_t *p_content);


/**@} */ //MW_DFU_FUNS

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif
/**
 @}
*/

