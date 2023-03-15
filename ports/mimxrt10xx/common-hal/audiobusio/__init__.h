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

// Where required we use identifier names that are required by NXP's
// API, even though they do not conform to the naming standards that Adafruit
// strives to adhere to. https://www.adafruit.com/blacklivesmatter
#include "drivers/fsl_sai.h"
#include "py/obj.h"

#include "supervisor/background_callback.h"

I2S_Type *SAI_GetPeripheral(int idx);
uint32_t SAI_GetInstance(I2S_Type *base);
typedef struct {
    mp_obj_t *sample;
    background_callback_t callback;
} i2s_t;


void port_i2s_allocate_init(i2s_t *self, bool left_justified);
void port_i2s_reset_instance(int i);
void i2s_reset(void);
void port_i2s_play(i2s_t *self, mp_obj_t sample, bool loop);
void port_i2s_stop(i2s_t *self);
bool port_i2s_playing(i2s_t *self);
bool port_i2s_paused(i2s_t *self);
void port_i2s_pause(i2s_t *self);
void port_i2s_resume(i2s_t *self);

// some uses (imagecapture) can only operate on i2s0 and need their own init code
void port_i2s_allocate_i2s0(void);
