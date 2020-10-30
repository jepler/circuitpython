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

#pragma once

#include "supervisor/background_callback.h"
#include "common-hal/microcontroller/Pin.h"

#include "py/obj.h"

#include "driver/i2s.h"

// Some boards don't implement I2SOut, so suppress any routines from here.
#if CIRCUITPY_AUDIOBUSIO_I2SOUT

typedef struct {
    mp_obj_base_t base;
    mp_obj_t *sample;
    bool left_justified;
    bool loop;
    bool paused;
    bool playing;
    bool stopping;
    bool samples_signed;
    int8_t bytes_per_sample;
    int8_t instance;
    uint16_t buffer_length;
    uint8_t *sample_data, *sample_end;
    const mcu_pin_obj_t *bit_clock;
    const mcu_pin_obj_t *word_select;
    const mcu_pin_obj_t *data;
    i2s_config_t i2s_config;
    i2s_pin_config_t i2s_pin_config;
    background_callback_t callback;
} audiobusio_i2sout_obj_t;

#endif
