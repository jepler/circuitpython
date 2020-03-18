/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
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

#include "shared-bindings/displayio/FramebufferDisplay.h"

#include <stdint.h>
#include <string.h>

#include "py/gc.h"
#include "py/runtime.h"
#include "shared-bindings/time/__init__.h"
#include "shared-module/displayio/display_core.h"

#include "tick.h"

// Helper to ensure we have the native super class instead of a subclass.
STATIC displayio_framebufferdisplay_obj_t* native_framebuf(mp_obj_t framebuf_obj) {
    mp_obj_t native_framebuf = mp_instance_cast_to_native_base(framebuf_obj, &displayio_framebufferdisplay_type);
    mp_obj_assert_native_inited(native_framebuf);
    return MP_OBJ_TO_PTR(native_framebuf);
}

void common_hal_displayio_framebufferdisplay_construct(displayio_framebufferdisplay_obj_t* self,
    busio_framebuffer_obj_t* framebuffer, uint16_t device_address, const mcu_pin_obj_t* reset) {

    // Reset the display before probing
    self->reset.base.type = &mp_type_NoneType;
    if (reset != NULL) {
        self->reset.base.type = &digitalio_digitalinout_type;
        common_hal_digitalio_digitalinout_construct(&self->reset, reset);
        common_hal_digitalio_digitalinout_switch_to_output(&self->reset, true, DRIVE_MODE_PUSH_PULL);
        common_hal_never_reset_pin(reset);
        common_hal_displayio_framebufferdisplay_reset(self);
    }

    // Probe the bus to see if a device acknowledges the given address.
    if (!common_hal_busio_framebuffer_probe(framebuffer, device_address)) {
        mp_raise_ValueError_varg(translate("Unable to find Framebuffer Display at %x"), device_address);
    }

    // Write to the device and return 0 on success or an appropriate error code from mperrno.h
    self->bus = framebuffer;
    common_hal_busio_framebuffer_never_reset(self->bus);
    // Our object is statically allocated off the heap so make sure the bus object lives to the end
    // of the heap as well.
    gc_never_free(self->bus);

    self->address = device_address;
}

void common_hal_displayio_framebufferdisplay_deinit(displayio_framebufferdisplay_obj_t* self) {
    if (self->bus == &self->inline_bus) {
        common_hal_busio_framebuffer_deinit(self->bus);
    }

    common_hal_reset_pin(self->reset.pin);
}

bool common_hal_displayio_framebufferdisplay_reset(mp_obj_t self_in) {
    return false;
}

bool common_hal_displayio_framebufferdisplay_bus_free(mp_obj_t self_in) {
    return true;
}

bool common_hal_displayio_framebufferdisplay_begin_transaction(mp_obj_t obj) {
}

void common_hal_displayio_framebufferdisplay_set_region_to_update(mp_obj_t self_in, displayio_area_t *area) {
    displayio_framebufferdisplay_obj_t* self = native_framebuf(self_in);
    self->ptr = self->bufinfo.buf + (area.y1 * self->width + area.x1) * self.bpp;
    self->area_in_bytes = (area.x2 - area.x1) * self.bpp;
}

void common_hal_displayio_framebufferdisplay_send(mp_obj_t self_in, display_byte_type_t data_type, display_chip_select_behavior_t chip_select, uint8_t *data, uint32_t data_length) {
    if (data_type != DISPLAY_DATA) {
        return;
    }
    displayio_framebufferdisplay_obj_t* self = native_framebuf(self_in);
    while (data_length != 0) {
        uint32_t length = min(data_length, self->row_in_bytes);
        memcpy(self->ptr, data, length);
        data_length -= length;
        data += length;
        self->ptr += self->rowstride;
    }
}

void common_hal_displayio_framebufferdisplay_end_transaction(mp_obj_t self_in) {
}

void common_hal_displayio_framebufferdisplay_finish_refresh(mp_obj_t self_in) {
    displayio_framebufferdisplay_obj_t* self = native_framebuf(self_in);
    mp_obj_t dest[2 + 1];
    mp_load_method(self_in, MP_QSTR__transmit, dest);

    dest[2] = self->buffer;

    mp_call_method_n_kw(1, 0, dest);
}
