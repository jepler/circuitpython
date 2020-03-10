/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Scott Shawcroft for Adafruit Industries
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

#ifndef MICROPY_INCLUDED_SHARED_MODULE_DISPLAYIO_FRAMEBUFFERDISPLAY_H
#define MICROPY_INCLUDED_SHARED_MODULE_DISPLAYIO_FRAMEBUFFERDISPLAY_H

#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/displayio/Group.h"
#include "shared-bindings/pulseio/PWMOut.h"

#include "shared-module/displayio/area.h"
#include "shared-module/displayio/display_core.h"

typedef struct {
    mp_obj_base_t base;
    displayio_display_core_t core;
    union {
        digitalio_digitalinout_obj_t backlight_inout;
        pulseio_pwmout_obj_t backlight_pwm;
    };
    mp_obj_t framebuffer, callback;
    mp_buffer_info_t bufinfo;
    uint64_t last_backlight_refresh;
    uint64_t last_refresh_call;
    mp_float_t current_brightness;
    uint16_t brightness_command;
    uint16_t native_frames_per_second;
    uint16_t native_ms_per_frame;
    bool auto_refresh;
    bool first_manual_refresh;
    bool auto_brightness;
    bool updating_backlight;
} displayio_framebufferdisplay_obj_t;

void displayio_framebufferdisplay_background(displayio_framebufferdisplay_obj_t* self);
void release_framebufferdisplay(displayio_framebufferdisplay_obj_t* self);
void reset_framebufferdisplay(displayio_framebufferdisplay_obj_t* self);

void displayio_framebufferdisplay_collect_ptrs(displayio_framebufferdisplay_obj_t* self);

mp_obj_t common_hal_displayio_framebufferdisplay_get_framebuffer(displayio_framebufferdisplay_obj_t* self);

#endif // MICROPY_INCLUDED_SHARED_MODULE_DISPLAYIO_FRAMEBUFFERDISPLAY_H
