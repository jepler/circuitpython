/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#include "shared-bindings/displayio/FramebufferDisplay.h"

#include <stdint.h>
#include <string.h>

#include "lib/utils/context_manager_helpers.h"
#include "py/binary.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/util.h"
#include "shared-module/displayio/__init__.h"
#include "supervisor/shared/translate.h"

//| .. currentmodule:: displayio
//|
//| :class:`FramebufferDisplay` -- Manage updating a display over Framebuffer
//| ==========================================================================
//|
//| Manage updating a display over Framebuffer in the background while Python code runs.
//| It doesn't handle display initialization.
//|
//| .. class:: FramebufferDisplay(framebuffer_bus, *, device_address, reset=None)
//|
//|   Create a FramebufferDisplay object associated with the given Framebuffer bus and reset pin.
//|
//|   The Framebuffer bus and pins are then in use by the display until `displayio.release_displays()` is
//|   called even after a reload. (It does this so CircuitPython can use the display after your code
//|   is done.) So, the first time you initialize a display bus in code.py you should call
//|   :py:func`displayio.release_displays` first, otherwise it will error after the first code.py run.
//|
//|   :param busio.Framebuffer framebuffer_bus: The Framebuffer bus that make up the clock and data lines
//|   :param int device_address: The Framebuffer address of the device
//|   :param microcontroller.Pin reset: Reset pin. When None only software reset can be used
//|
STATIC mp_obj_t displayio_framebufferdisplay_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_framebuffer };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_framebuffer, MP_ARG_REQUIRED | MP_ARG_OBJ },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    displayio_framebufferdisplay_obj_t* self = NULL;
    mp_obj_t framebuffer = args[ARG_framebuffer_bus].u_obj;
    for (uint8_t i = 0; i < CIRCUITPY_DISPLAY_LIMIT; i++) {
        if (displays[i].framebufferdisplay_bus.base.type == NULL ||
            displays[i].framebufferdisplay_bus.base.type == &mp_type_NoneType) {
            self = &displays[i].framebufferdisplay_bus;
            self->base.type = &displayio_framebufferdisplay_type;
            break;
        }
    }
    if (self == NULL) {
        mp_raise_RuntimeError(translate("Too many display busses"));
    }

    common_hal_displayio_framebufferdisplay_construct(self, MP_OBJ_TO_PTR(framebuffer));
    return self;
}

STATIC const mp_rom_map_elem_t displayio_framebufferdisplay_locals_dict_table[] = {
};
STATIC MP_DEFINE_CONST_DICT(displayio_framebufferdisplay_locals_dict, displayio_framebufferdisplay_locals_dict_table);

const mp_obj_type_t displayio_framebufferdisplay_type = {
    { &mp_type_type },
    .name = MP_QSTR_FramebufferDisplay,
    .make_new = displayio_framebufferdisplay_make_new,
    .locals_dict = (mp_obj_dict_t*)&displayio_framebufferdisplay_locals_dict,
};
