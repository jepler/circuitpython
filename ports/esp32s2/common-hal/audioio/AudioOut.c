/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jeff Epler for Adafruit Industries
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

#include "common-hal/audioio/AudioOut.h"

// There is only a single DAC DMA, and placing these as file-level statics
// makes it easier to ensure that dma_buf is freed at soft-reset time.
#define BYTES_PER_FRAME (2)
#define FRAMES_PER_BUF (512)
#define DMA_BUFS (2)
#define DMA_BUF0 (dma_buf)
#define DMA_BUF1 (dma_buf + BYTES_PER_FRAME * FRAMES_PER_BUF)

static uint8_t *dma_buf;
static lldesc_t dma_desc[DMA_BUFS];

static void release_dma_buf() {
    if (dma_buf) {
        free(dma_buf);
    }
    dma_buf = NULL;
}

static uint32_t allocate_dma_buf() {
    if (dma_buf) {
        free(dma_buf);
    }
    dma_buf = calloc(BYTES_PER_FRAME * FRAMES_PER_BUF, DMA_BUFS);
    if (!dma_buf) {
        mp_raise_espidf_MemoryError();
    }
    dma_desc[0] = (lldesc_t) {
        .size = BYTES_PER_FRAME * FRAMES_PER_BUF,
        .length = BYTES_PER_FRAME * FRAMES_PER_BUF,
        .eof = 0,
        .owner = 1,
        .buf = DMA_BUF0,
        .qe.stqe_next = &dma_desc[1],
    };
    dma_desc[1] = (lldesc_t) {
        .size = BYTES_PER_FRAME * FRAMES_PER_BUF,
        .length = BYTES_PER_FRAME * FRAMES_PER_BUF,
        .eof = 0,
        .owner = 1,
        .buf = DMA_BUF1,
        .qe.stqe_next = &dma_desc[0],
    };
    return (uint32_t)&dma_desc[0];
}

void common_hal_audioio_audioout_construct(audioio_audioout_obj_t* self,
    const mcu_pin_obj_t* left_channel, const mcu_pin_obj_t* right_channel, uint16_t default_value) {
    mp_raise_NotImplementedError(NULL);
}

void common_hal_audioio_audioout_deinit(audioio_audioout_obj_t* self)
{
    if (!self->left_channel) {
        return;
    }
}

bool common_hal_audioio_audioout_deinited(audioio_audioout_obj_t* self)
{
    return !self->left_channel;
}

void common_hal_audioio_audioout_play(audioio_audioout_obj_t* self, mp_obj_t sample, bool loop)
{
    self->sample = sample;
    self->loop = loop;
    self->bytes_per_sample = audiosample_bits_per_sample(sample) / 8;
    self->channel_count = audiosample_channel_count(sample);
    bool single_buffer;
    bool samples_signed;
    uint32_t max_buffer_length;
    uint8_t spacing;
    audiosample_get_buffer_structure(sample, false, &single_buffer, &samples_signed,
                                     &max_buffer_length, &spacing);
    self->samples_signed = samples_signed;
    self->playing = true;
    self->paused = false;
    self->stopping = false;
    self->sample_data = self->sample_end = NULL;
    // We always output stereo so output twice as many bits.
    // uint16_t bits_per_sample_output = bits_per_sample * 2;

    audiosample_reset_buffer(self->sample, false, 0);

    mp_raise_NotImplementedError(NULL);
}

void common_hal_audioio_audioout_stop(audioio_audioout_obj_t* self)
{
    self->sample = NULL;
    self->paused = false;
    self->playing = false;
    self->stopping = false;
}

bool common_hal_audioio_audioout_get_playing(audioio_audioout_obj_t* self)
{
    return self->playing && !self->stopping;
}

void common_hal_audioio_audioout_pause(audioio_audioout_obj_t* self)
{
    if (!self->paused) {
        self->paused = true;
        // DMA stop
    }
    mp_raise_NotImplementedError(NULL);
}

void common_hal_audioio_audioout_resume(audioio_audioout_obj_t* self)
{
    if (self->paused) {
        self->paused = false;
        // DMA start
    }
    mp_raise_NotImplementedError(NULL);
}

bool common_hal_audioio_audioout_get_paused(audioio_audioout_obj_t* self)
{
    return self->paused;
}
