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

#pragma once

#include "py/obj.h"

#include "fsl_sai.h"
#include "fsl_sai_edma.h"

#include "shared-module/audiocore/__init__.h"
#include "supervisor/background_callback.h"

typedef struct {
    I2S_Type *sai;
    audiocore_rebuffer_t rebuffer;
    background_callback_t callback;
    sai_edma_handle_t *handle;
    uint8_t dma_channel, i2s_channel;
    uint8_t buffer_count;
    bool paused;
} audio_dma_t;

typedef enum {
    AUDIO_DMA_OK,
    AUDIO_DMA_DMA_BUSY,
    AUDIO_DMA_MEMORY_ERROR,
} audio_dma_result;

uint32_t audiosample_sample_rate(mp_obj_t sample_obj);
uint8_t audiosample_bits_per_sample(mp_obj_t sample_obj);
uint8_t audiosample_channel_count(mp_obj_t sample_obj);

void audio_dma_init(audio_dma_t *dma);
void audio_dma_reset(void);

uint8_t audio_dma_allocate_channel(audio_dma_t *self, bool transmit, int peripheral, int channel);
void audio_dma_free_channel(uint8_t channel);

// This sets everything up but doesn't start the timer.
// Sample is the python object for the sample to play.
// loop is true if we should loop the sample.
void audio_dma_play(audio_dma_t *dma, mp_obj_t sample, bool loop);

void audio_dma_disable_channel(uint8_t channel);
void audio_dma_enable_channel(uint8_t channel);
void audio_dma_stop(audio_dma_t *dma);
bool audio_dma_get_playing(audio_dma_t *dma);
void audio_dma_pause(audio_dma_t *dma);
void audio_dma_resume(audio_dma_t *dma);
bool audio_dma_get_paused(audio_dma_t *dma);

void audio_dma_background(void);
