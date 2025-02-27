
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

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "kbutils.h"
#include "imfcc.h"
#include "imfcc_rom.h"
#include "kbalgorithms.h"

int32_t fg_frequency_mfe(kb_model_t *kb_model, int16_data_t *cols_to_use, float_data_t *params, FLOAT *pFV)
{

#define FG_MFE_NUM_PARAMS 1
#define FG_MFE_COEFFICIENT_COUNT_PARAM_IDX 0

    int32_t icol, irow, i, len;
    ringb *rb;
    int32_t start_index;

#if SML_DEBUG
    // we dont need frame shift because it will be taken care by windowing segmenter
    if (!kb_model || kb_model->sg_length <= 0 || !cols_to_use || cols_to_use->size <= 0 || !params || params->size != FG_MFE_NUM_PARAMS || !pFV)
        return 0;
#endif

    // num_cols has to be 1 else prior will be corrupted. design of pre_emphasis_prior need to be revisited
    for (icol = 0; icol < cols_to_use->size; icol++)
    {
        rb = kb_model->pdata_buffer->data + cols_to_use->data[icol];
        start_index = kb_model->sg_index & rb->mask;
        if (kb_model->sg_length > 512)
        {
            len = 512;
        }
        else
        {
            len = kb_model->sg_length;
        }

        for (irow = 0; irow < len; irow++)
        {
            sortedData[irow] = MOD_READ_RINGBUF(rb, start_index++);
            // printf("sortedData[%3d]: %10d\n", irow, sortedData[irow]);
        }

        IMFE_ProcessFrame((int16_t *)sortedData,
                          512,
                          len);

        // at this point, the tempbuff is overwritten with dct_outputs in int32, which the feature we need
        int32_t *mfe = (int32_t *)sortedData;

        for (i = 0; i < params->data[FG_MFE_COEFFICIENT_COUNT_PARAM_IDX]; i++)
        {
            *pFV++ = (FLOAT)mfe[i];
        }
    }

    // return the first nth coefficient
    return params->data[FG_MFE_COEFFICIENT_COUNT_PARAM_IDX];
}
