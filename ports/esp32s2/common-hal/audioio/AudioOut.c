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

#include "esp_error.h"

// Some boards don't implement I2SOut, so suppress any routines from here.
#if CIRCUITPY_AUDIOBUSIO_I2SOUT

#include "extmod/vfs_fat.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "common-hal/audiobusio/I2SOut.h"
#include "shared-bindings/audiobusio/I2SOut.h"
#include "shared-bindings/audiocore/RawSample.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/shared/translate.h"

#include "driver/i2s.h"

// Caller validates that pins are free.
void common_hal_audioio_audioout_construct(audioio_audioout_obj_t* self,
    const mcu_pin_obj_t* left_channel, const mcu_pin_obj_t* right_channel, uint16_t quiescent_value) {

    port_i2s_allocate_init(&self->peripheral, true);

    if (left_channel != pin_GPIO17) {
        mp_raise_ValueError(translate("Left channel must be GPIO17"));
    }
    if (right_channel && right_channel != pin_GPIO18) {
        mp_raise_ValueError(translate("Right channel must be GPIO18 or None"));
    }

    if (right_channel) {
        i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    } else {
        i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
    }
}

bool common_hal_audioio_audioout_deinited(audioio_audioout_obj_t* self) {
    return self->peripheral.instance == -1;
}

void common_hal_audioio_audioout_deinit(audioio_audioout_obj_t* self) {
    if (common_hal_audioio_audioout_deinited(self)) {
        return;
    }

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

    if (self->peripheral.instance >= 0) {
        port_i2s_reset_instance(self->peripheral.instance);
    }
    self->peripheral.instance = -1;
}

void common_hal_audioio_audioout_play(audioio_audioout_obj_t* self,
                                       mp_obj_t sample, bool loop) {
    if (common_hal_audioio_audioout_get_playing(self)) {
        common_hal_audioio_audioout_stop(self);
    }
    port_i2s_play(&self->peripheral, sample, loop);
}

void common_hal_audioio_audioout_pause(audioio_audioout_obj_t* self) {
    port_i2s_pause(&self->peripheral);
}

void common_hal_audioio_audioout_resume(audioio_audioout_obj_t* self) {
    port_i2s_resume(&self->peripheral);
}

bool common_hal_audioio_audioout_get_paused(audioio_audioout_obj_t* self) {
    return port_i2s_paused(&self->peripheral);
}

void common_hal_audioio_audioout_stop(audioio_audioout_obj_t* self) {
    port_i2s_stop(&self->peripheral);
}

bool common_hal_audioio_audioout_get_playing(audioio_audioout_obj_t* self) {
    return port_i2s_playing(&self->peripheral);
}

#endif // CIRCUITPY_AUDIOBUSIO_I2SOUT
