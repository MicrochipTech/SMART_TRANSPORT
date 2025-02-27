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
  Middleware Device Firmware Udpate Source File

  Company:
    Microchip Technology Inc.

  File Name:
    mw_dfu.c

  Summary:
    This file contains the Middleware Device Firmware Udpate functions for application user.

  Description:
    This file contains the Middleware Device Firmware Udpate functions for application user.
 *******************************************************************************/


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "configuration.h"
#include "osal/osal_freertos.h"
#include "peripheral/nvm/plib_nvm.h"
#include "peripheral/dmac/plib_dmac.h"
#include "mba_error_defs.h"
#include "mw_dfu.h"

#ifdef MW_DFU_EXTERNAL_FLASH_ENABLE
#include "driver/sst26/drv_sst26.h"
#endif

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

#define MW_DFU_FW_EFLASH_0_START_ADDR          0x01000000UL
#define MW_DFU_FW_EFLASH_1_START_ADDR          0x01040000UL
#define MW_DFU_FW_PAGE_SIZE                    0x1000U
#define MW_DFU_FW_QUAD_WORD_SIZE               0x10U

#define MW_DFU_META_SEQ_OFFSET_V1              0x00U
#define MW_DFU_META_CHR_OFFSET_V1              0x06U
#define MW_DFU_META_SRC_OFFSET_V1              0x14U
#define MW_DFU_META_LEN_OFFSET_V1              0x1CU
#define MW_DFU_CRC_OFFSET_V1                   0x10U

#define MW_DFU_META_CHR_OFFSET_V3              0x08U
#define MW_DFU_META_SEQ_OFFSET_V3              0x1CU
#define MW_DFU_META_SRC_OFFSET_V3              0x2CU
#define MW_DFU_META_LEN_OFFSET_V3              0x34U
#define MW_DFU_CRC_OFFSET_V3                   0x20U



#define MW_DFU_FW_OFFSET                       0x200U

#define MISCDIAG (*((volatile unsigned int *) (0x44000440) ))

#define MW_DFU_IDENT_SIZE                      0x10U


#define MW_DFU_FW_EXT_FLASH_START_ADDR         0x00000000UL
#define MW_DFU_FW_EXT_FLASH_SLOT_SIZE          0x80000UL
#define MW_DFU_FW_EXT_SECTOR_SIZE              0x1000U

#define MW_DFU_EXT_FLASH_ID                    0x004326BFUL

#define MW_DFU_HDR_COHERENCE                   0x4D434850UL
#define MW_DFU_EXT_HDR_COHERENCE               0x5048434DUL

enum
{
    MW_DFU_STATE_IDLE=0x00U,
    MW_DFU_STATE_CONFIG,
    MW_DFU_STATE_FW_START,
    MW_DFU_STATE_FW_UPDATE
};

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

typedef uint16_t (*MW_DFU_FwOpStart)(void);
typedef uint16_t (*MW_DFU_FwOpUpdate)(uint16_t length, uint8_t *p_content);
typedef uint16_t (*MW_DFU_FwOpValiate)(uint16_t cfmValue);
typedef uint16_t (*MW_DFU_FwOpActivate)();
typedef uint16_t (*MW_DFU_FwOpRead)(uint32_t offset, uint16_t length, uint8_t *p_content);

typedef struct MW_DFU_FwOperation_T
{
    MW_DFU_FwOpStart      start;
    MW_DFU_FwOpUpdate     update;
    MW_DFU_FwOpValiate    validate;
    MW_DFU_FwOpActivate   activate;
    MW_DFU_FwOpRead       read;
} MW_DFU_FwOperation_T;

typedef struct MW_DFU_ExtFlashHdr_T
{
  uint32_t  coherence;
  uint8_t   mdRev;
  uint8_t   plDecMthd;
  uint8_t   plDecKey;
  uint8_t   reserved;
  uint32_t  seqNum;
  uint16_t  crc16;
  uint16_t  checksum;
} MW_DFU_ExtFlashHdr_T;


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
static uint16_t mw_dfu_FwImageStartInt(void);
static uint16_t mw_dfu_FwImageUpdateInt(uint16_t length, uint8_t *p_content);
static uint16_t mw_dfu_FwImageValidateInt(uint16_t fwImageCrc);
static uint16_t mw_dfu_FwImageActivateInt(void);
static uint16_t mw_dfu_FwImageReadInt(uint32_t offset, uint16_t length, uint8_t *p_content);

#ifdef MW_DFU_EXTERNAL_FLASH_ENABLE
static uint16_t mw_dfu_FwImageStartExt(void);
static uint16_t mw_dfu_FwImageUpdateExt(uint16_t length, uint8_t *p_content);
static uint16_t mw_dfu_FwImageValidateExt(uint16_t fwImageChksum);
static uint16_t mw_dfu_FwImageActivateExt(void);
static uint16_t mw_dfu_FwImageReadExt(uint32_t offset, uint16_t length, uint8_t *p_content);
#endif
// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************

static uint8_t s_dfuState = MW_DFU_STATE_IDLE;
static uint32_t s_dfuSizeInfo;
static uint32_t s_dfuAddr;
static uint32_t *sp_dfuIdent = NULL;
static bool s_dfuIsV1Fw;

static const MW_DFU_FwOperation_T *sp_dfuFwOp;

static const MW_DFU_FwOperation_T s_dfuFwOpInt = 
{
    mw_dfu_FwImageStartInt,
    mw_dfu_FwImageUpdateInt,
    mw_dfu_FwImageValidateInt,
    mw_dfu_FwImageActivateInt,
    mw_dfu_FwImageReadInt
};

#ifdef MW_DFU_EXTERNAL_FLASH_ENABLE
static uint32_t s_dfuStartAddr;       /* ony used for external flash. */

static const MW_DFU_FwOperation_T s_dfuFwOpExt = 
{
    mw_dfu_FwImageStartExt,
    mw_dfu_FwImageUpdateExt,
    mw_dfu_FwImageValidateExt,
    mw_dfu_FwImageActivateExt,
    mw_dfu_FwImageReadExt
};

#endif


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static uint16_t mw_dfu_FwImageStartInt(void)
{
    s_dfuAddr = MW_DFU_FW_EFLASH_1_START_ADDR;
    s_dfuState = MW_DFU_STATE_FW_START;

    return MBA_RES_SUCCESS;
}

static uint16_t mw_dfu_FwImageUpdateInt(uint16_t length, uint8_t *p_content)
{
    uint32_t * p_data;
    uint32_t addr;
    uint8_t srcOffset, seqOffset;

    /* Check if image content length and offset are legal */
    if ((s_dfuAddr + length > MW_DFU_FW_EFLASH_1_START_ADDR + s_dfuSizeInfo) || (length > MW_DFU_MAX_BLOCK_LEN)
        || (length & (MW_DFU_FW_QUAD_WORD_SIZE-1U)) || (length == 0U))
    {
        return MBA_RES_INVALID_PARA;
    }

    /* make sure nvm is not busy now */
    while(NVM_IsBusy()==true)
    {

    }

    /* if start from page boundary or write cross page, erase new page */
    if ((s_dfuAddr & (MW_DFU_FW_PAGE_SIZE - 1U)) == 0U 
    || (s_dfuAddr & ~(MW_DFU_FW_PAGE_SIZE - 1U)) < ((s_dfuAddr + length - 1U)  & ~(MW_DFU_FW_PAGE_SIZE - 1U)))
    {
        if (!NVM_PageErase((s_dfuAddr + length - 1U) & ~(MW_DFU_FW_PAGE_SIZE - 1U)))
        {
            return MBA_RES_FAIL;
        }

        while(NVM_IsBusy()==true)
        {

        }
        
        if (NVM_ErrorGet() != NVM_ERROR_NONE)
        {
            return MBA_RES_FAIL;
        }
    }

    /* if need to check and revise sequnce number or src address */
    if (s_dfuAddr <= MW_DFU_FW_EFLASH_1_START_ADDR + MW_DFU_META_SRC_OFFSET_V3)
    {
        
        if (*(uint32_t *)(p_content + MW_DFU_META_CHR_OFFSET_V1) == MW_DFU_HDR_COHERENCE)
        {
            seqOffset = MW_DFU_META_SEQ_OFFSET_V1;
            srcOffset = MW_DFU_META_SRC_OFFSET_V1;
            s_dfuIsV1Fw = true;
        }
        else if (*(uint32_t *)(p_content + MW_DFU_META_CHR_OFFSET_V3) == MW_DFU_HDR_COHERENCE)
        {
            seqOffset = MW_DFU_META_SEQ_OFFSET_V3;
            srcOffset = MW_DFU_META_SRC_OFFSET_V3;

            s_dfuIsV1Fw = false;
        }
        else
        {
            return MBA_RES_INVALID_PARA;
        }


        /* revise sequence number */
        if (s_dfuAddr <= MW_DFU_FW_EFLASH_1_START_ADDR + seqOffset && s_dfuAddr + length > MW_DFU_FW_EFLASH_1_START_ADDR + seqOffset)
        {
            uint32_t seq;

            if (s_dfuIsV1Fw == true)
            {
                while(NVM_IsBusy()==true)
                {
                }
                
                if (!NVM_Read(&seq, 4, MW_DFU_FW_EFLASH_0_START_ADDR + MW_DFU_META_SEQ_OFFSET_V1))
                {
                    return MBA_RES_FAIL;
                }

                *(uint32_t *)(p_content + MW_DFU_FW_EFLASH_1_START_ADDR + MW_DFU_META_SEQ_OFFSET_V1 - s_dfuAddr) = --seq;
            }
            else
            {
                seq = 0xFFFFFFFFU;
            }
            
            *(uint32_t *)(p_content + MW_DFU_FW_EFLASH_1_START_ADDR + seqOffset - s_dfuAddr) = seq;
        }

        /* revise src address */
        if (s_dfuAddr <= MW_DFU_FW_EFLASH_1_START_ADDR + srcOffset && s_dfuAddr + length > MW_DFU_FW_EFLASH_1_START_ADDR + srcOffset)
        {
            if((MISCDIAG & 0x0000C000) == 0x0000C000)
            {
                *(uint32_t *)(p_content + MW_DFU_FW_EFLASH_1_START_ADDR + srcOffset - s_dfuAddr) = MW_DFU_FW_EFLASH_0_START_ADDR + MW_DFU_FW_OFFSET;
            }
            else
            {
                *(uint32_t *)(p_content + MW_DFU_FW_EFLASH_1_START_ADDR + srcOffset - s_dfuAddr) = MW_DFU_FW_EFLASH_1_START_ADDR + MW_DFU_FW_OFFSET;
            }
        }
    }
    
    p_data = (uint32_t *)p_content;
    addr = s_dfuAddr; 

    /* backup first 16 bytes */
    if (s_dfuAddr == MW_DFU_FW_EFLASH_1_START_ADDR)
    {
        if (sp_dfuIdent == NULL)
        {
            sp_dfuIdent = OSAL_Malloc(MW_DFU_IDENT_SIZE);
            if (sp_dfuIdent == NULL)
            {
                return MBA_RES_OOM;
            }
        }
        
        (void)memcpy(sp_dfuIdent, p_data, MW_DFU_IDENT_SIZE);

        addr += MW_DFU_IDENT_SIZE;
        p_data += MW_DFU_IDENT_SIZE/sizeof(*p_data);
    }
    
    for (;addr < s_dfuAddr + length; addr += MW_DFU_FW_QUAD_WORD_SIZE)
    {
        if(!NVM_QuadWordWrite(p_data, addr))
        {
            return MBA_RES_FAIL;
        }

        while(NVM_IsBusy()==true)
        {
        }

        if (NVM_ErrorGet() != NVM_ERROR_NONE)
        {
            return MBA_RES_FAIL;
        }

        p_data += 4;
    }

    s_dfuAddr = addr;

    s_dfuState = MW_DFU_STATE_FW_UPDATE;

    return MBA_RES_SUCCESS;
}

static uint16_t mw_dfu_FwImageValidateInt(uint16_t fwImageCrc)
{
    DMAC_CRC_SETUP  crcSetup;
    uint32_t src = 0x00000000;
    uint8_t srcOffset, crcOffset;
    uint32_t fwLen = 0, result;

    if (s_dfuIsV1Fw == true)
    {
        crcOffset = MW_DFU_CRC_OFFSET_V1;
        srcOffset = MW_DFU_META_SRC_OFFSET_V1;
        result = mw_dfu_FwImageReadInt(MW_DFU_META_LEN_OFFSET_V1, 4, (uint8_t *)&fwLen);
    }
    else
    {
        crcOffset = MW_DFU_CRC_OFFSET_V3;
        srcOffset = MW_DFU_META_SRC_OFFSET_V3;
        result = mw_dfu_FwImageReadInt(MW_DFU_META_LEN_OFFSET_V3, 4, (uint8_t *)&fwLen);
    }

    fwLen += MW_DFU_FW_OFFSET;

    if (fwLen > MW_DFU_MAX_SIZE_FW_IMAGE_INT || result != MBA_RES_SUCCESS)
    {
        return MBA_RES_FAIL;
    }

    crcSetup.polynomial_type = DMAC_CRC_TYPE_16;
    crcSetup.crc_mode = DMAC_CRC_MODE_DEFAULT;
    crcSetup.seed = 0xFFFF;

    crcSetup.seed = DMAC_CRCCalculate((void *)(MW_DFU_FW_EFLASH_1_START_ADDR + crcOffset), srcOffset - crcOffset, crcSetup);
    crcSetup.seed = DMAC_CRCCalculate(&src, 4, crcSetup);  /* Set FW_IMG_SRC_ADR field as 0x0000000. */
    crcSetup.seed = DMAC_CRCCalculate((void *)(MW_DFU_FW_EFLASH_1_START_ADDR + srcOffset + 4U), fwLen - srcOffset - 4U, crcSetup); 
    
    if (crcSetup.seed != fwImageCrc)
    {
        return MBA_RES_FAIL;
    }
    
    return MBA_RES_SUCCESS;
}

static uint16_t mw_dfu_FwImageActivateInt(void)
{
    if(!NVM_QuadWordWrite(sp_dfuIdent, MW_DFU_FW_EFLASH_1_START_ADDR))
    {
        return MBA_RES_FAIL;
    }

    while(NVM_IsBusy()==true)
    {
    }

    if (NVM_ErrorGet() != NVM_ERROR_NONE)
    {
        return MBA_RES_FAIL;
    }

    OSAL_Free(sp_dfuIdent);
    sp_dfuIdent = NULL;

    s_dfuState = MW_DFU_STATE_CONFIG;

    return MBA_RES_SUCCESS;
}

static uint16_t mw_dfu_FwImageReadInt(uint32_t offset, uint16_t length, uint8_t *p_content)
{
    //Check read range 
    if ((offset + length > MW_DFU_MAX_SIZE_FW_IMAGE_INT) || (length > MW_DFU_MAX_BLOCK_LEN))
    {
        return MBA_RES_INVALID_PARA;
    }

    while(NVM_IsBusy()==true)
    {
    }

    if (!NVM_Read((uint32_t *)p_content, length, MW_DFU_FW_EFLASH_1_START_ADDR + offset))
    {
        return MBA_RES_FAIL;
    }
    
    return MBA_RES_SUCCESS;
}

#ifdef MW_DFU_EXTERNAL_FLASH_ENABLE
static bool mw_dfu_validExtHdr(MW_DFU_ExtFlashHdr_T * p_hdr)
{
    uint8_t *p_data;
    uint16_t chksum, idx;
    uint32_t seq;

    if (p_hdr->coherence != MW_DFU_EXT_HDR_COHERENCE 
        || p_hdr->mdRev != 0x01U || p_hdr->plDecMthd > 0x01U || p_hdr->seqNum == 0U)
    {
        return false;
    }

    p_data = (uint8_t *)p_hdr;
    seq = p_hdr->seqNum;
    p_hdr->seqNum = 0;
    chksum = 0;
    for (idx = 0; idx < 14U; idx++)
    {
        chksum += p_data[idx];
    }

    p_hdr->seqNum = seq;

    if (chksum != p_hdr->checksum)
    {
        return false;
    }

    return true;
}

static uint16_t mw_dfu_FwImageStartExt(void)
{
    DRV_HANDLE hdl;
    MW_DFU_ExtFlashHdr_T hdr;
    uint32_t seq;

    
    hdl = DRV_SST26_Open(DRV_SST26_INDEX, DRV_IO_INTENT_READWRITE);
    if (hdl == DRV_HANDLE_INVALID)
    {
        return MBA_RES_FAIL;
    }

    if (!DRV_SST26_Read(hdl, &hdr, sizeof(hdr), MW_DFU_FW_EXT_FLASH_START_ADDR))
    {
        DRV_SST26_Close(hdl);
        return MBA_RES_FAIL;
    }

    while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
    {
    }

    s_dfuAddr = MW_DFU_FW_EXT_FLASH_START_ADDR;

    if (mw_dfu_validExtHdr(&hdr) && hdr.seqNum != 0xFFFFFFFFUL)
    {
        seq = hdr.seqNum;
        
        if (!DRV_SST26_Read(hdl, &hdr, sizeof(hdr), MW_DFU_FW_EXT_FLASH_START_ADDR + MW_DFU_FW_EXT_FLASH_SLOT_SIZE))
        {
            DRV_SST26_Close(hdl);
            return MBA_RES_FAIL;
        }

        while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
        {
        }

        if (!mw_dfu_validExtHdr(&hdr) || seq < hdr.seqNum)
        {
            s_dfuAddr = MW_DFU_FW_EXT_FLASH_START_ADDR + MW_DFU_FW_EXT_FLASH_SLOT_SIZE;
        }
    }

    DRV_SST26_Close(hdl);

    s_dfuState = MW_DFU_STATE_FW_START;
    s_dfuStartAddr = s_dfuAddr;

    return MBA_RES_SUCCESS;
}

static uint16_t mw_dfu_FwImageUpdateExt(uint16_t length, uint8_t *p_content)
{
    uint32_t * p_data;
    uint32_t addr;
    DRV_HANDLE hdl;
    
    /* check if image content length and offset are legal */
    if ((s_dfuAddr + length > s_dfuStartAddr + s_dfuSizeInfo) || (length > MW_DFU_MAX_BLOCK_LEN)
        || ((s_dfuAddr + length != s_dfuStartAddr + s_dfuSizeInfo) && (length & (DRV_SST26_PAGE_SIZE-1)))
        || (length == 0U))
    {
        return MBA_RES_INVALID_PARA;
    }

    hdl = DRV_SST26_Open(DRV_SST26_INDEX, DRV_IO_INTENT_READWRITE);
    if (hdl == DRV_HANDLE_INVALID)
    {
        return MBA_RES_FAIL;
    }

    /* if start from page boundary or write cross page, erase new page */
    if ((s_dfuAddr & (MW_DFU_FW_EXT_SECTOR_SIZE - 1U)) == 0U
    || (s_dfuAddr & ~(MW_DFU_FW_EXT_SECTOR_SIZE - 1U)) < ((s_dfuAddr + length - 1U)  & ~(MW_DFU_FW_EXT_SECTOR_SIZE - 1U)))
    {
        if (!DRV_SST26_SectorErase(hdl, (s_dfuAddr + length - 1U) & ~(MW_DFU_FW_EXT_SECTOR_SIZE - 1U)))
        {
            DRV_SST26_Close(hdl);
            return MBA_RES_FAIL;
        }

        while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
        {
        }
    }

    
    p_data = (uint32_t *)p_content;
    addr = s_dfuAddr; 

    /* for activate later, we need to change first 16 bytes to 0xFF */
    if (s_dfuAddr == s_dfuStartAddr)
    {
        uint8_t *p_buf;
        bool ret;
        
        p_buf = OSAL_Malloc(DRV_SST26_PAGE_SIZE);
        if (p_buf == NULL)
        {
            DRV_SST26_Close(hdl);
            return MBA_RES_OOM;
        }
        (void)memset(p_buf, 0xFF, MW_DFU_IDENT_SIZE);
        (void)memcpy(p_buf + MW_DFU_IDENT_SIZE, (uint8_t *)p_data + MW_DFU_IDENT_SIZE, DRV_SST26_PAGE_SIZE - MW_DFU_IDENT_SIZE);

        ret = DRV_SST26_PageWrite(hdl, p_buf, addr);
        OSAL_Free(p_buf);

        if(!ret)
        {
            DRV_SST26_Close(hdl);
            return MBA_RES_FAIL;
        }

        while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
        {
        }

        addr += DRV_SST26_PAGE_SIZE;
        p_data += DRV_SST26_PAGE_SIZE/sizeof(*p_data);


        if (sp_dfuIdent == NULL)
        {
            sp_dfuIdent = OSAL_Malloc(MW_DFU_IDENT_SIZE);
            if (sp_dfuIdent == NULL)
            {
                DRV_SST26_Close(hdl);
                return MBA_RES_OOM;
            }
        }
        
        (void)memcpy((void *)sp_dfuIdent, (void *)p_content, MW_DFU_IDENT_SIZE);
    }

    
    for (;addr < s_dfuAddr + length; addr += DRV_SST26_PAGE_SIZE)
    {
        if(!DRV_SST26_PageWrite(hdl, p_data, addr))
        {
            DRV_SST26_Close(hdl);
            return MBA_RES_FAIL;
        }

        while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
        {
        }

        p_data += DRV_SST26_PAGE_SIZE/sizeof(*p_data);
    }

    s_dfuAddr = addr;

    s_dfuState = MW_DFU_STATE_FW_UPDATE;

    DRV_SST26_Close(hdl);

    return MBA_RES_SUCCESS;
}

static uint16_t mw_dfu_FwImageValidateExt(uint16_t fwImageChksum)
{

    uint16_t chksum;
    uint32_t offset, idx;
    uint8_t buf[16];


    chksum = 0;
    for (idx = 0; idx < MW_DFU_IDENT_SIZE; idx++)
    {
        chksum += *((uint8_t *)sp_dfuIdent + idx);
    }


    for (offset = MW_DFU_IDENT_SIZE; offset < s_dfuSizeInfo; offset += sizeof(buf))
    {
        mw_dfu_FwImageReadExt(offset, sizeof(buf), buf);

        for (idx = 0; idx < sizeof(buf); idx++)
        {
            chksum += buf[idx];
        }
    }

    chksum = 0xFFFFU - chksum + 1U;

    if (chksum != fwImageChksum)
    {
        return MBA_RES_FAIL;
    }
    
    return MBA_RES_SUCCESS;
}

static uint16_t mw_dfu_FwImageActivateExt(void)
{
    DRV_HANDLE hdl;
    uint8_t *p_ident;
    uint16_t status;

    p_ident = OSAL_Malloc(DRV_SST26_PAGE_SIZE);

    if (p_ident == NULL)
    {
        return MBA_RES_OOM;
    }

    status = MBA_RES_FAIL;
    hdl = DRV_SST26_Open(DRV_SST26_INDEX, DRV_IO_INTENT_READWRITE);
    if (hdl != DRV_HANDLE_INVALID)
    {
        if (DRV_SST26_Read(hdl, p_ident, DRV_SST26_PAGE_SIZE, s_dfuStartAddr)==true)
        {
            while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
            {
            }

            (void)memcpy((void *)p_ident, (void *)sp_dfuIdent, MW_DFU_IDENT_SIZE);
        
            if(DRV_SST26_PageWrite(hdl, p_ident, s_dfuStartAddr)==true)
            {
                while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
                {
                }

                OSAL_Free(sp_dfuIdent);
                sp_dfuIdent = NULL;
                
                s_dfuState = MW_DFU_STATE_CONFIG;

                status = MBA_RES_SUCCESS;
            }
        }
        DRV_SST26_Close(hdl);
    }

    OSAL_Free(p_ident);

    return status;
}

static uint16_t mw_dfu_FwImageReadExt(uint32_t offset, uint16_t length, uint8_t *p_content)
{
    DRV_HANDLE hdl;
    
    //check read range 
    if ((offset + length > MW_DFU_MAX_SIZE_FW_IMAGE_EXT) || (length > MW_DFU_MAX_BLOCK_LEN))
    {
        return MBA_RES_INVALID_PARA;
    }

    hdl = DRV_SST26_Open(DRV_SST26_INDEX, DRV_IO_INTENT_READWRITE);
    if (hdl == DRV_HANDLE_INVALID)
    {
        return MBA_RES_FAIL;
    }

    if (!DRV_SST26_Read(hdl, (void*)p_content, length, s_dfuStartAddr + offset))
    {
        DRV_SST26_Close(hdl);
        return MBA_RES_FAIL;
    }

    while(DRV_SST26_TransferStatusGet(hdl) == DRV_SST26_TRANSFER_BUSY)
    {
    }

    DRV_SST26_Close(hdl);
    
    return MBA_RES_SUCCESS;
}
#endif

uint16_t MW_DFU_Config(MW_DFU_Info_T * p_dfuInfo)
{
    if ((p_dfuInfo->fwImageSize == 0U) || (p_dfuInfo->fwImageSize & (MW_DFU_FW_QUAD_WORD_SIZE-1U)))
    {
        return MBA_RES_INVALID_PARA;
    }

    if (p_dfuInfo->fwFlashType == MW_DFU_FLASH_INTERNAL)
    {
        if (p_dfuInfo->fwImageSize > MW_DFU_MAX_SIZE_FW_IMAGE_INT)
        {
            return MBA_RES_INVALID_PARA;
        }
    
        sp_dfuFwOp = &s_dfuFwOpInt;
    }
    #ifdef MW_DFU_EXTERNAL_FLASH_ENABLE
    else if (p_dfuInfo->fwFlashType == MW_DFU_FLASH_EXTERNAL)
    {
        uint32_t jedecId;
        DRV_HANDLE hdl;

        if (p_dfuInfo->fwImageSize > MW_DFU_MAX_SIZE_FW_IMAGE_EXT)
        {
            return MBA_RES_INVALID_PARA;
        }


        hdl = DRV_SST26_Open(DRV_SST26_INDEX, DRV_IO_INTENT_READWRITE);
        if (hdl == DRV_HANDLE_INVALID)
        {
            return MBA_RES_FAIL;
        }
        
        
        if (!DRV_SST26_ReadJedecId(hdl, &jedecId))
        {
            DRV_SST26_Close(hdl);
            return MBA_RES_FAIL;
        }

        DRV_SST26_Close(hdl);

        if ((jedecId & 0xFFFFFFUL) != MW_DFU_EXT_FLASH_ID)
        {
           return MBA_RES_FAIL;
        }

        sp_dfuFwOp = &s_dfuFwOpExt;
    }
    #endif
    else
    {
        return MBA_RES_INVALID_PARA;
    }

    s_dfuSizeInfo = p_dfuInfo->fwImageSize;

    s_dfuState = MW_DFU_STATE_CONFIG;

    return MBA_RES_SUCCESS;
}

uint16_t MW_DFU_FwImageStart(void)
{
    if (s_dfuState == MW_DFU_STATE_IDLE)
    {
        return MBA_RES_BAD_STATE;
    }

    return sp_dfuFwOp->start();
}

uint16_t MW_DFU_FwImageUpdate(uint16_t length, uint8_t *p_content)
{
    if ((s_dfuState != MW_DFU_STATE_FW_START) && (s_dfuState != MW_DFU_STATE_FW_UPDATE))
    {
        return MBA_RES_BAD_STATE;
    }

    return sp_dfuFwOp->update(length, p_content);
}

uint16_t MW_DFU_FwImageValidate(uint16_t fwCfmValue)
{
    return sp_dfuFwOp->validate(fwCfmValue);
}

uint16_t MW_DFU_FwImageActivate(void)
{
    if (s_dfuState != MW_DFU_STATE_FW_UPDATE)
    {
        return MBA_RES_BAD_STATE;
    }

    return sp_dfuFwOp->activate();
}


uint16_t MW_DFU_FwImageRead(uint32_t offset, uint16_t length, uint8_t *p_content)
{
    if (s_dfuState < MW_DFU_STATE_FW_START)
    {
        return MBA_RES_BAD_STATE;
    }

    return sp_dfuFwOp->read(offset, length, p_content);
}
