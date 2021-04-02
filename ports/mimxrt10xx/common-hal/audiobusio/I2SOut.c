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

#include "common-hal/audiobusio/I2SOut.h"
#include "shared-bindings/audiobusio/I2SOut.h"

void i2sout_reset(void) {}
void audio_dma_background(void) {}

void common_hal_audiobusio_i2sout_construct(audiobusio_i2sout_obj_t *self,
    const mcu_pin_obj_t *bit_clock, const mcu_pin_obj_t *word_select, const mcu_pin_obj_t *data,
    bool left_justified)
{
}

void common_hal_audiobusio_i2sout_deinit(audiobusio_i2sout_obj_t *self)
{
}

bool common_hal_audiobusio_i2sout_deinited(audiobusio_i2sout_obj_t *self)
{
    return true;
}

void common_hal_audiobusio_i2sout_play(audiobusio_i2sout_obj_t *self, mp_obj_t sample, bool loop)
{
    self->playing = 1;
}

void common_hal_audiobusio_i2sout_stop(audiobusio_i2sout_obj_t *self)
{
    self->playing = 0;
}

bool common_hal_audiobusio_i2sout_get_playing(audiobusio_i2sout_obj_t *self)
{
    return self->playing;
}

void common_hal_audiobusio_i2sout_pause(audiobusio_i2sout_obj_t *self)
{
    self->paused = 1;
}

void common_hal_audiobusio_i2sout_resume(audiobusio_i2sout_obj_t *self)
{
    self->paused = 0;
}

bool common_hal_audiobusio_i2sout_get_paused(audiobusio_i2sout_obj_t *self)
{
    return self->paused;
}
