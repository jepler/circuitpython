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

#include "shared-bindings/microcontroller/__init__.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpstate.h"

#include "audio_dma.h"

#include "fsl_dmamux.h"
#include "fsl_sai_edma.h"

static I2S_Type *const s_saiBases[] = I2S_BASE_PTRS;
static dma_request_source_t const dma_tx_request_sources[] = {
    [1] = kDmaRequestMuxSai1Tx,
    [3] = kDmaRequestMuxSai3Tx,
};

/* Clock pre divider for sai1 clock source */
#define AUDIO_CLOCK_SOURCE_PRE_DIVIDER (3U)
/* Clock divider for sai1 clock source */
#define AUDIO_CLOCK_SOURCE_DIVIDER (1U)
/* Get frequency of sai1 clock */
#define AUDIO_CLK_FREQ                                                        \
    (CLOCK_GetFreq(kCLOCK_Usb1PllClk) / (AUDIO_CLOCK_SOURCE_DIVIDER + 1U) / \
    (AUDIO_CLOCK_SOURCE_PRE_DIVIDER + 1U))

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

    (void)audiorebuffer_fill_s16s(&self->rebuffer, (int16_t *)sample_buffer, BUFFER_SIZE / sizeof(int16_t) / 2);

    sai_transfer_t xfer;
    xfer.data = sample_data;
    xfer.dataSize = sample_buffer_length;
    SAI_TransferSendEDMA(self->sai, &tx_handles[self->dma_channel], &xfer);
}

static void audio_dma_callback(void *self_in) {
    audio_dma_t *self = self_in;
    fill_buffer(self);
}

void audio_dma_stop(audio_dma_t *self) {
    common_hal_mcu_disable_interrupts();
    audiorebuffer_set_sample(&self->rebuffer, NULL, false);
    common_hal_mcu_enable_interrupts();
}

void audio_dma_play(audio_dma_t *self, mp_obj_t sample, bool loop) {
    if (self->rebuffer.sample_obj) {
        audio_dma_stop(self);
    }
    common_hal_mcu_disable_interrupts();

    audiorebuffer_set_sample(&self->rebuffer, sample, loop);

    {
        sai_transceiver_t sai_config;
        SAI_GetClassicI2SConfig(&sai_config, 16, kSAI_Stereo, 1U << self->i2s_channel);
        SAI_TransferTxSetConfigEDMA(self->sai, &tx_handles[self->dma_channel], &sai_config);
    }

    SAI_TxSetBitClockRate(self->sai, AUDIO_CLK_FREQ, audiosample_sample_rate(sample), 16, 2);
    fill_buffer(self);
    fill_buffer(self);
    common_hal_mcu_enable_interrupts();
}

void audio_dma_reset(void) {
    for (int i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        DMAMUX_DisableChannel(DMAMUX, i);
        EDMA_ResetChannel(DMA0, i);
        MP_STATE_PORT(playing_audio)[i] = NULL;
    }
    SAI_Deinit(SAI1);
    SAI_Deinit(SAI3);
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


uint8_t audio_dma_allocate_channel(audio_dma_t *self, bool transmit, int sai_peripheral, int i2s_channel) {
    int dma_channel = audio_dma_get_available_channel();
    self->sai = s_saiBases[sai_peripheral];
    self->dma_channel = dma_channel;
    self->i2s_channel = i2s_channel;
    MP_STATE_PORT(playing_audio)[dma_channel] = self;

    DMAMUX_SetSource(DMAMUX, dma_channel, dma_tx_request_sources[sai_peripheral]);
    DMAMUX_EnableChannel(DMAMUX, dma_channel);
    edma_config_t dmaConfig = {0};
    EDMA_GetDefaultConfig(&dmaConfig);
    EDMA_CreateHandle(&edma_handles[dma_channel], DMA0, dma_channel);

    SAI_Init(self->sai);
    SAI_TransferTxCreateHandleEDMA(self->sai, &tx_handles[dma_channel], callback, self, &edma_handles[dma_channel]);
    return dma_channel;
}

bool audio_dma_get_playing(audio_dma_t *self) {
    return self->rebuffer.sample_obj != NULL;
}

bool audio_dma_get_paused(audio_dma_t *self) {
    return self->paused;
}

void audio_dma_pause(audio_dma_t *self) {
    self->paused = true;
}

void audio_dma_resume(audio_dma_t *self) {
    self->paused = false;
}
