/*
 * This file is part of the Micro Python project, http://micropython.org/
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

#include "py/obj.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "common-hal/_protomatter/Protomatter.h"
#include "shared-bindings/_protomatter/Protomatter.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/util.h"
#include "shared-module/displayio/FramebufferDisplay.h"

//| .. currentmodule:: _protomatter
//|
//| :class:`protomatter` --  Driver for HUB75-style RGB LED matrices
//| ================================================================
//|

extern Protomatter_core *_PM_protoPtr;

STATIC mp_obj_t protomatter_protomatter_deinit(mp_obj_t self_in);

STATIC uint8_t validate_pin(mp_obj_t obj) {
    mcu_pin_obj_t *result = validate_obj_is_free_pin(obj);
    return common_hal_mcu_pin_number(result);
}

STATIC void validate_pins(qstr what, uint8_t* pin_nos, mp_int_t max_pins, mp_obj_t seq, uint8_t *count_out) {
    mp_int_t len = MP_OBJ_SMALL_INT_VALUE(mp_obj_len(seq));
    if (len > max_pins) {
        mp_raise_ValueError_varg(translate("At most %d %q may be specified"), max_pins, what);
    }
    *count_out = len;
    for (mp_int_t i=0; i<len; i++) {
        pin_nos[i] = validate_pin(mp_obj_subscr(seq, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_SENTINEL));
    }
}

STATIC void claim_pins(mp_obj_t seq) {
    mp_int_t len = MP_OBJ_SMALL_INT_VALUE(mp_obj_len(seq));
    for (mp_int_t i=0; i<len; i++) {
        common_hal_mcu_pin_claim(mp_obj_subscr(seq, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_SENTINEL));
    }
}

//| :class:`~_protomatter.Protomatter` displays an in-memory framebuffer to an LED matrix.
//|
//| .. class:: Protomatter(width, bit_depth, rgb_pins, addr_pins, clock_pin, latch_pin, oe_pin, *, doublebuffer=True, framebuffer=None)
//|
//|   Create a Protomatter object with the given attributes.  The height of
//|   the display is determined by the number of rgb and address pins:
//|   len(rgb_pins)/3 * 2 ** len(address_pins).  With 6 RGB pins and 4
//|   address lines, the display will be 32 pixels tall.
//|
//|   Up to 30 RGB pins and 8 address pins are supported.
//|
//|   The RGB pins must be within a single "port" and performance and memory
//|   usage are best when they are all within "close by" bits of the port.
//|   The clock pin must also be on the same port as the RGB pins.  See the
//|   documentation of the underlying protomatter C library for more
//|   information.  Generally, Adafruit's interface boards are designed so
//|   that these requirements are met when matched with the intended
//|   microcontroller board.  For instance, the Feather M4 Express works
//|   together with the RGB Matrix Feather.
//|
//|   When specified as True, double buffering can reduce some flickering of
//|   the matrix; however, this increases memory usage.
//|
//|   The framebuffer is in "RGB565" format.  If a framebuffer is not
//|   passed in, one is allocated and initialized to all black.  To update
//|   the content, modify the framebuffer and call swapbuffers.
//|
//|   If doublebuffer is False, some memory is saved, but the display may
//|   flicker during updates.
//|

STATIC mp_obj_t protomatter_protomatter_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_bit_width, ARG_bit_depth, ARG_rgb_list, ARG_addr_list,
        ARG_clock_pin, ARG_latch_pin, ARG_oe_pin, ARG_doublebuffer, ARG_framebuffer };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_bit_depth, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_rgb_pins, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_addr_pins, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_clock_pin, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_latch_pin, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_oe_pin, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_doublebuffer, MP_ARG_BOOL, { .u_bool = true } },
        { MP_QSTR_framebuffer, MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Because interrupt handlers point directly at protomatter objects,
    // it is NOT okay to move them to the long-lived pool later.  Allocate
    // them there to begin with, since generally they'll be long-lived anyway.
    protomatter_protomatter_obj_t *self =
            m_new_ll_obj(protomatter_protomatter_obj_t);
    self->base.type = &protomatter_Protomatter_type;

    validate_pins(MP_QSTR_rgb_pins, self->rgb_pins, MP_ARRAY_SIZE(self->rgb_pins), args[ARG_rgb_list].u_obj, &self->rgb_count);
    validate_pins(MP_QSTR_addr_pins, self->addr_pins, MP_ARRAY_SIZE(self->addr_pins), args[ARG_addr_list].u_obj, &self->addr_count);
    self->clock_pin = validate_pin(args[ARG_clock_pin].u_obj);
    self->oe_pin = validate_pin(args[ARG_oe_pin].u_obj);
    self->latch_pin = validate_pin(args[ARG_latch_pin].u_obj);

    self->timer = common_hal_protomatter_timer_allocate();
    if (self->timer == NULL) {
        mp_raise_ValueError(translate("No timer available"));
    }

    self->width = args[ARG_bit_width].u_int;
    self->bufsize = 2 * args[ARG_bit_width].u_int * self->rgb_count / 3 * (1 << self->addr_count);
    claim_pins(args[ARG_rgb_list].u_obj);
    claim_pins(args[ARG_addr_list].u_obj);
    common_hal_mcu_pin_claim(args[ARG_clock_pin].u_obj);
    common_hal_mcu_pin_claim(args[ARG_oe_pin].u_obj);
    common_hal_mcu_pin_claim(args[ARG_latch_pin].u_obj);

    self->framebuffer = args[ARG_framebuffer].u_obj;
    if (self->framebuffer == mp_const_none) {
        self->framebuffer = mp_obj_new_bytearray_of_zeros(self->bufsize);
    }
    mp_get_buffer_raise(self->framebuffer, &self->bufinfo, MP_BUFFER_READ);
    // verify that the matrix is big enough
    mp_get_index(mp_obj_get_type(self->framebuffer), self->bufinfo.len, MP_OBJ_NEW_SMALL_INT(self->bufsize-1), false);

    ProtomatterStatus stat = _PM_init(&self->core,
        args[ARG_bit_width].u_int, args[ARG_bit_depth].u_int,
        self->rgb_count/6, self->rgb_pins,
        self->addr_count, self->addr_pins,
        self->clock_pin, self->latch_pin, self->oe_pin,
        args[ARG_doublebuffer].u_bool, self->timer);

    if (stat == PROTOMATTER_OK) {
        _PM_protoPtr = &self->core;
        common_hal_mcu_disable_interrupts();
        common_hal_protomatter_timer_enable(self->timer);
        stat = _PM_begin(&self->core);
        _PM_convert_565(&self->core, self->bufinfo.buf, self->width);
        common_hal_mcu_enable_interrupts();
        _PM_swapbuffer_maybe(&self->core);
    }

    if (stat != PROTOMATTER_OK) {
        // XXX this deinit() actually makes crashy-crashy
        // can trigger it by sending inappropriate pins
        protomatter_protomatter_deinit(self);
        switch (stat) {
        case PROTOMATTER_ERR_PINS:
            mp_raise_ValueError(translate("Invalid pin"));
            break;
        case PROTOMATTER_ERR_ARG:
            mp_raise_ValueError(translate("Invalid argument"));
            break;
        case PROTOMATTER_ERR_MALLOC: /// should have already been signaled as NLR
        default:
            mp_raise_msg_varg(&mp_type_RuntimeError,
                translate("Protomatter internal error #%d"), (int)stat);
            break;
        }
    }

    self->paused = 0;

    return MP_OBJ_FROM_PTR(self);
}

STATIC void free_pin(uint8_t *pin) {
    if (*pin != COMMON_HAL_MCU_NO_PIN) {
        common_hal_mcu_pin_reset_number(*pin);
    }
    *pin = COMMON_HAL_MCU_NO_PIN;
}

STATIC void free_pin_seq(uint8_t *seq, int count) {
    for (int i=0; i<count; i++) {
        free_pin(&seq[i]);
    }
}

//|   .. method:: deinit
//|
//|     Free the resources (pins, timers, etc.) associated with this
//|     protomatter instance.  After deinitialization, no further operations
//|     may be performed.
//|
STATIC mp_obj_t protomatter_protomatter_deinit(mp_obj_t self_in) {
    protomatter_protomatter_obj_t *self = (protomatter_protomatter_obj_t*)self_in;
    if (self->timer) {
        common_hal_protomatter_timer_free(self->timer);
        self->timer = 0;
    }

    free_pin_seq(self->rgb_pins, self->rgb_count);
    free_pin_seq(self->addr_pins, self->addr_count);
    free_pin(&self->clock_pin);
    free_pin(&self->latch_pin);
    free_pin(&self->oe_pin);

    if (self->core.rgbPins) {
        _PM_free(&self->core);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(protomatter_protomatter_deinit_obj, protomatter_protomatter_deinit);

static void check_for_deinit(protomatter_protomatter_obj_t *self) {
    if (!self->core.rgbPins) {
        raise_deinited_error();
    }
}

//|   .. attribute:: paused
//|
//|     When paused, the matrix is not driven and all its LEDs are unlit.
//|
STATIC mp_obj_t protomatter_protomatter_get_paused(mp_obj_t self_in) {
    protomatter_protomatter_obj_t *self = (protomatter_protomatter_obj_t*)self_in;
    check_for_deinit(self);
    return mp_obj_new_bool(self->paused);
}
MP_DEFINE_CONST_FUN_OBJ_1(protomatter_protomatter_get_paused_obj, protomatter_protomatter_get_paused);

STATIC mp_obj_t protomatter_protomatter_set_paused(mp_obj_t self_in, mp_obj_t value_in)  {
    protomatter_protomatter_obj_t *self = (protomatter_protomatter_obj_t*)self_in;
    check_for_deinit(self);
    bool paused = mp_obj_is_true(value_in);
    if (paused && !self->paused) {
        _PM_stop(&self->core);
    } else if (!paused && self->paused) {
        _PM_resume(&self->core);
    }
    self->paused = paused;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(protomatter_protomatter_set_paused_obj, protomatter_protomatter_set_paused);

const mp_obj_property_t protomatter_protomatter_paused_obj = {
    .base.type = &mp_type_property,
    .proxy = {(mp_obj_t)&protomatter_protomatter_get_paused_obj,
              (mp_obj_t)&protomatter_protomatter_set_paused_obj,
              (mp_obj_t)&mp_const_none_obj},
};

//|   .. attribute:: framebuffer
//|
//|     Retrieve the framebuffer object.  Modify this object and then
//|     call swapbuffers to update the display.
//|
STATIC mp_obj_t protomatter_protomatter_get_framebuffer(mp_obj_t self_in) {
    protomatter_protomatter_obj_t *self = (protomatter_protomatter_obj_t*)self_in;
    check_for_deinit(self);
    return self->framebuffer;
}
MP_DEFINE_CONST_FUN_OBJ_1(protomatter_protomatter_get_framebuffer_obj, protomatter_protomatter_get_framebuffer);

const mp_obj_property_t protomatter_protomatter_framebuffer_obj = {
    .base.type = &mp_type_property,
    .proxy = {(mp_obj_t)&protomatter_protomatter_get_framebuffer_obj,
              (mp_obj_t)&mp_const_none_obj,
              (mp_obj_t)&mp_const_none_obj},
};

//|   .. method:: write(buf)
//|
//|     Transmits the color data in the buffer to the pixels so that they are shown.
//|
//|     The data in the buffer must be in "RGB565" format.  This means
//|     that it is organized as a series of 16-bit numbers where the highest 5
//|     bits are interpreted as red, the next 6 as green, and the final 5 as
//|     blue.  The object can be any buffer, but `array.array` and `ulab.array`
//|     objects are most often useful.
//|
STATIC mp_obj_t protomatter_protomatter_swapbuffers(mp_obj_t self_in) {
    protomatter_protomatter_obj_t *self = (protomatter_protomatter_obj_t*)self_in;
    check_for_deinit(self);

    _PM_convert_565(&self->core, self->bufinfo.buf, self->width);
    _PM_swapbuffer_maybe(&self->core);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(protomatter_protomatter_swapbuffers_obj, protomatter_protomatter_swapbuffers);

STATIC const mp_rom_map_elem_t protomatter_protomatter_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&protomatter_protomatter_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_framebuffer), MP_ROM_PTR(&protomatter_protomatter_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_paused), MP_ROM_PTR(&protomatter_protomatter_paused_obj) },
    { MP_ROM_QSTR(MP_QSTR_swapbuffers), MP_ROM_PTR(&protomatter_protomatter_swapbuffers_obj) },
};
STATIC MP_DEFINE_CONST_DICT(protomatter_protomatter_locals_dict, protomatter_protomatter_locals_dict_table);

STATIC void protomatter_protomatter_get_bufinfo(mp_obj_t self_in, mp_buffer_info_t *bufinfo) {
    protomatter_protomatter_obj_t *self = (protomatter_protomatter_obj_t*)self_in;
    check_for_deinit(self);
    
    *bufinfo = self->bufinfo;
}

STATIC void protomatter_protomatter_swapbuffers_void(mp_obj_t self_in) {
    protomatter_protomatter_swapbuffers(self_in);
}

STATIC void protomatter_protomatter_set_brightness(mp_obj_t self_in, mp_float_t value) {
    protomatter_protomatter_set_paused(self_in, mp_obj_new_bool(value <= 0));
}

STATIC const framebuffer_p_t protomatter_protomatter_proto = {
    MP_PROTO_IMPLEMENT(MP_QSTR_protocol_framebuffer)
    .get_bufinfo = protomatter_protomatter_get_bufinfo,
    .set_brightness = protomatter_protomatter_set_brightness,
    .swapbuffers = protomatter_protomatter_swapbuffers_void,
};

const mp_obj_type_t protomatter_Protomatter_type = {
    { &mp_type_type },
    .name = MP_QSTR_Protomatter,
    .make_new = protomatter_protomatter_make_new,
    .protocol = &protomatter_protomatter_proto,
    .locals_dict = (mp_obj_dict_t*)&protomatter_protomatter_locals_dict,
};
