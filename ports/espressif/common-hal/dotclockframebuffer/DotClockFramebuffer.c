/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Jeff Epler for Adafruit Industries
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

#include "shared-bindings/dotclockframebuffer/DotClockFramebuffer.h"
#include "py/runtime.h"

void common_hal_dotclockframebuffer_framebuffer_construct(dotclockframebuffer_framebuffer_obj_t *self,
    const mcu_pin_obj_t *de,
    const mcu_pin_obj_t *vsync,
    const mcu_pin_obj_t *hsync,
    const mcu_pin_obj_t *dclk,
    const mcu_pin_obj_t **red, uint8_t num_red,
    const mcu_pin_obj_t **green, uint8_t num_green,
    const mcu_pin_obj_t **blue, uint8_t num_blue,
    int frequency, int width, int height,
    int hsync_pulse_width, int hsync_back_porch, int hsync_front_porch, bool hsync_idle_low,
    int vsync_pulse_width, int vsync_back_porch, int vsync_front_porch, bool vsync_idle_low,
    bool de_idle_high, bool pclk_active_high, bool pclk_idle_high) {
    mp_raise_NotImplementedError(NULL);
}


void common_hal_dotclockframebuffer_framebuffer_deinit(dotclockframebuffer_framebuffer_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

bool common_hal_dotclockframebuffer_framebuffer_deinitialized(dotclockframebuffer_framebuffer_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}


mp_int_t common_hal_dotclockframebuffer_framebuffer_get_width(dotclockframebuffer_framebuffer_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_height(dotclockframebuffer_framebuffer_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_frequency(dotclockframebuffer_framebuffer_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_refresh_rate(dotclockframebuffer_framebuffer_obj_t *self) {
    mp_raise_NotImplementedError(NULL);
}
