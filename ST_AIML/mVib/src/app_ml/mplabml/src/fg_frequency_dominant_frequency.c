
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

#include "kbalgorithms.h"
#include "fftr_utils.h"

#define NUM_PARAMS 1
#define SAMPLE_RATE_PARAM_IDX 0

static FLOAT calc_dominant_frequency(int16_t *input_data, int32_t len, FLOAT fs)
{
    int32_t i;
    struct compx_int16_t *data;

    data = fftr_rm_as(input_data, len);

    int32_t max = 0;
    int32_t ndx = 0;
    int32_t val;
    for (i = 1; i < NUM_FFTR_CMPX; i++)
    {

        val = (abs(data[i].real) + abs(data[i].imag));
        if (val > max)
        {
            max = val;
            ndx = i;
        }
    }

    return (FLOAT)(ndx * fs / len);
}

int32_t fg_frequency_dominant_frequency(kb_model_t *kb_model, int16_data_t *cols_to_use, float_data_t *params, FLOAT *pFV)
{

    int32_t icol;
    int32_t len;
    ringb *rb;
    int32_t start_index;

#if SML_DEBUG
    if (!kb_model || kb_model->sg_length <= 0 || !cols_to_use || cols_to_use->size <= 0 || params->size != NUM_PARAMS || !params || !pFV)
    {
        return 0;
    }
#endif

    FLOAT sample_rate = params->data[SAMPLE_RATE_PARAM_IDX];

    for (icol = 0; icol < cols_to_use->size; icol++)
    {
        int32_t i;
        rb = kb_model->pdata_buffer->data + cols_to_use->data[icol];
        start_index = kb_model->sg_index;
        if (kb_model->sg_length > NUM_FFTR)
        {
            len = NUM_FFTR;
        }
        else
        {
            len = kb_model->sg_length;
        }

        for (i = 0; i < len; i++)
        {
            sortedData[i] = MOD_READ_RINGBUF(rb, start_index++);
        }

        pFV[icol] = calc_dominant_frequency(sortedData, kb_model->sg_length, sample_rate);
    }

    return cols_to_use->size;
}
