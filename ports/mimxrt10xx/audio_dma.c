/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Jeff Epler for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpstate.h"

#include "audio_dma.h"

#include "fsl_dmamux.h"
#include "fsl_sai_edma.h"

/* Clock pre divider for sai1 clock source */
#define AUDIO_CLOCK_SOURCE_PRE_DIVIDER (3U)
/* Clock divider for sai1 clock source */
#define AUDIO_CLOCK_SOURCE_DIVIDER (1U)
/* Get frequency of sai1 clock */
#define AUDIO_CLK_FREQ                                                        \
    (CLOCK_GetFreq(kCLOCK_PllUsb1) / (AUDIO_CLOCK_SOURCE_DIVIDER + 1U) / \
    (AUDIO_CLOCK_SOURCE_PRE_DIVIDER + 1U))

#define AUDIO_TX_EDMA_CHANNEL (0U)
#define AUDIO_RX_EDMA_CHANNEL (1U)

#define BUFFER_NUMBER (2U)
#define BUFFER_SIZE (512U)

// TODO Use cache clean/invalidate so we can use CP heap memory instead
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t Buffer[BUFFER_NUMBER * BUFFER_SIZE], 4);
AT_NONCACHEABLE_SECTION_INIT(static edma_handle_t edma_handles[AUDIO_DMA_CHANNEL_COUNT]);
AT_NONCACHEABLE_SECTION_INIT(static sai_edma_handle_t tx_handles[AUDIO_DMA_CHANNEL_COUNT]);

static void fill_buffer(audio_dma_t *self) {
    uint8_t *sample_data = NULL;
    uint32_t sample_buffer_length = 0;

    void *sample_buffer = Buffer + BUFFER_SIZE * (self->buffer_count++ % 2);

    (void)audiorebuffer_fill_s16(&self->rebuffer, (int16_t *)sample_buffer, BUFFER_SIZE / sizeof(int16_t) / 2);

    sai_transfer_t xfer;
    xfer.data = sample_data;
    xfer.dataSize = sample_buffer_length;
    SAI_TransferSendEDMA(self->sai, &tx_handles[self->dma_channel], &xfer);
}

void audio_dma_callback(void *self_in) {
    audio_dma_t *self = self_in;
    fill_buffer(self);
}

void audio_dma_play(audio_dma_t *self, mp_obj_t sample, bool loop) {
    if (self->rebuffer.sample_obj) {
        audio_dma_stop(self);
    }
    audiorebuffer_set_sample(&self->rebuffer, sample, loop);
    fill_buffer(self);
}

void audio_dma_reset(void) {
    for (int i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        DMAMUX_DisableChannel(DMAMUX, i);
        EDMA_ResetChannel(DMA0, i);
        MP_STATE_PORT(playing_audio)[i] = NULL;
    }
}

static uint8_t audio_dma_get_available_channel(void) {
    for (int i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        if (!MP_STATE_PORT(playing_audio)[i]) {
            return i;
        }
    }
    mp_raise_RuntimeError(translate("All DMA channels in use"));
}

static void callback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData) {
    audio_dma_t *self = userData;

    if (status != kStatus_SAI_RxError) {
        background_callback_add(&self->callback, audio_dma_callback, self);
    }
}


uint8_t audio_dma_allocate_channel(audio_dma_t *self) {
    int channel = audio_dma_get_available_channel();
    self->dma_channel = channel;
    MP_STATE_PORT(playing_audio)[channel] = self;

    edma_config_t dmaConfig = {0};
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_CreateHandle(&edma_handles[channel], DMA0, channel);

    SAI_TransferTxCreateHandleEDMA(self->sai, &tx_handles[channel], callback, self, &edma_handles[channel]);
    return channel;
}
