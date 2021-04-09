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

#include "fsl_dmaux.h"
#include "fsl_sai_edma.h"

void audio_dma_init(void) {
    DMAMUX_Init(DMA0);
}

void audio_dma_background(void) {
}

void audio_dma_reset(void) {
    for (int i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        DMAMUX_DisableChannel(DMA0, i);
        playing_audio[i] = NULL;
    }
}

static uint8_t audio_dma_get_available_channel(void) {
    for (int i = 0; i < AUDIO_DMA_CHANNEL_COUNT; i++) {
        if (!playing_audio[i]) {
            return i;
        }
    }
    mp_raise_RuntimeError("All DMA channels in use");
}

uint8_t audio_dma_allocate_channel(void) {

    DMAMUX_SetSource (DMA0, channel
