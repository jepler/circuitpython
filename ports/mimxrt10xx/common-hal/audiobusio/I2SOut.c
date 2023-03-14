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

#include <stdint.h>
#include <string.h>

#include "mpconfigport.h"

// Some boards don't implement I2SOut, so suppress any routines from here.
#if CIRCUITPY_AUDIOBUSIO_I2SOUT

#include "py/gc.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "common-hal/audiobusio/I2SOut.h"
#include "shared-bindings/audiobusio/I2SOut.h"
#include "shared-bindings/audiocore/RawSample.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/shared/translate/translate.h"

// Note that where required we use identifier names that are required by NXP's
// API, even though they do not conform to the naming standards that Adafruit
// strives to adhere to. https://www.adafruit.com/blacklivesmatter
#include "drivers/fsl_sai.h"

static void release_buffers(audiobusio_i2sout_obj_t *self) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(self->buffers); i++) {
        gc_free(self->buffers[i]);
        self->buffers[i] = NULL;
    }
}

// Caller validates that pins are free.
void common_hal_audiobusio_i2sout_construct(audiobusio_i2sout_obj_t *self,
    const mcu_pin_obj_t *bit_clock, const mcu_pin_obj_t *word_select,
    const mcu_pin_obj_t *data, bool left_justified) {

// TODO determine SAI1 vs SAI3
// set up pinmuxes
// set up clocks
// set up etc

    I2S_Type peripheral = SAI1;
    sai_config_t config;
    SAI_TxGetDefaultConfig(&config);
    SAI_TxInit(peripheral, &config);

    self->peripheral = peripheral;
    self->bit_clock = bit_clock;
    self->word_select = word_select;
    self->data = data;
    claim_pin(bit_clock);
    claim_pin(word_select);
    claim_pin(data);
}

bool common_hal_audiobusio_i2sout_deinited(audiobusio_i2sout_obj_t *self) {
    return self->peripheral != NULL;
}

void common_hal_audiobusio_i2sout_deinit(audiobusio_i2sout_obj_t *self) {
    if (common_hal_audiobusio_i2sout_deinited(self)) {
        return;
    }

    SAI_Deinit(self->peripheral);
    self->peripheral = NULL;

    if (self->bit_clock) {
        reset_pin_number(self->bit_clock->number);
    }
    self->bit_clock = NULL;

    if (self->word_select) {
        reset_pin_number(self->word_select->number);
    }
    self->word_select = NULL;

    if (self->data) {
        reset_pin_number(self->data->number);
    }
    self->data = NULL;

    release_buffers(self);
}

void common_hal_audiobusio_i2sout_play(audiobusio_i2sout_obj_t *self,
    mp_obj_t sample, bool loop) {
    if (common_hal_audiobusio_i2sout_get_playing(self)) {
        common_hal_audiobusio_i2sout_stop(self);
    }
    port_i2s_play(&self->peripheral, sample, loop);
}

void common_hal_audiobusio_i2sout_pause(audiobusio_i2sout_obj_t *self) {
    self->paused = true;
}

void common_hal_audiobusio_i2sout_resume(audiobusio_i2sout_obj_t *self) {
    self->paused = false;
}

bool common_hal_audiobusio_i2sout_get_paused(audiobusio_i2sout_obj_t *self) {
    return self->paused;
}

void common_hal_audiobusio_i2sout_stop(audiobusio_i2sout_obj_t *self) {
    port_i2s_stop(&self->peripheral);
    release_buffers(self);
}

bool common_hal_audiobusio_i2sout_get_playing(audiobusio_i2sout_obj_t *self) {
    return self->playing;
}

#endif // CIRCUITPY_AUDIOBUSIO_I2SOUT
