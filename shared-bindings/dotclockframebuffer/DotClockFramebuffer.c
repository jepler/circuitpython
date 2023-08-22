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

#include "shared-bindings/dotclockframebuffer/DotClockFramebuffer.h"

#include "py/binary.h"
#include "py/objarray.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/util.h"
#include "shared-module/displayio/__init__.h"
#include "supervisor/shared/translate/translate.h"

//| class DotClockFramebuffer:
//|     """Manage updating a display over 8-bit parallel bus in the background while Python code runs. This
//|     protocol may be referred to as 8080-I Series Parallel Interface in datasheets. It doesn't handle
//|     display initialization."""
//|
//|     def __init__(
//|         self,
//|         *,
//|         de: microcontroller.Pin,
//|         vsync: microcontroller.Pin,
//|         hsync: microcontroller.Pin,
//|         dclk: microcontroller.Pin,
//|         red: Tuple[microcontroller.Pin],
//|         green: Tuple[microcontroller.Pin],
//|         blue: Tuple[microcontroller.Pin],
//|         frequency: int,
//|         width: int,
//|         height: int,
//|         hsync_pulse_width: int,
//|         hsync_back_porch: int,
//|         hsync_front_porch: int,
//|         hsync_idle_low: bool,
//|         vsync_back_porch: int,
//|         vsync_front_porch: int,
//|         vsync_idle_low: bool,
//|         de_idle_high: bool,
//|         pclk_active_high: bool,
//|         pclk_idle_high: bool,
//|     ) -> None:
//|         """Create a DotClockFramebuffer object associated with the given pins.
//|
//|         The pins are then in use by the display until `displayio.release_displays()`
//|         is called even after a reload. (It does this so CircuitPython can use the display after your
//|         code is done.) So, the first time you initialize a display bus in code.py you should call
//|         :py:func:`displayio.release_displays` first, otherwise it will error after the first code.py run.
//|
//|         When a board has dedicated dot clock framebuffer pins and/or timings, they are intended to be used in the constructor with ``**`` dictionary unpacking like so:
//|         ``DotClockFramebuffer(**board.TFT_PINS, **board.TFT_TIMINGS)``
//|
//|         TFT connection parameters:
//|         :param microcontroller.Pin de: The "data enable" input to the display
//|         :param microcontroller.Pin vsync: The "vertical sync" input to the display
//|         :param microcontroller.Pin hsync: The "horizontal sync" input to the display
//|         :param microcontroller.Pin dclk: The "data clock" input to the display
//|         :param ~tuple red: The red data pins, most significant pin first.
//|         :param ~tuple green: The green data pins, most significant pin first.
//|         :param ~tuple blue: The blue data pins, most significant pin first.
//|
//|         TFT timing parameters:
//|         :param int frequency: The requested data clock frequency in Hz.
//|         :param int width: The visible width of the display, in pixels
//|         :param int height: The visible height of the display, in pixels
//|         :param int hsync_pulse_width: Horizontal sync width in pixels
//|         :param int hsync_back_porch: Horizontal back porch, number of pixels between hsync and start of line active data
//|         :param int hsync_front_porch: Horizontal front porch, number of pixels between the end of active data and the next hsync
//|         :param int vsync_back_porch: Vertical back porch, number of lines between vsync and start of frame
//|         :param int vsync_front_porch: Vertical front porch, number of lines between the end of frame and the next vsync
//|         :param int hsync_idle_low: True if the hsync signal is low in IDLE state
//|         :param int vsync_idle_low: True if the vsync signal is low in IDLE state
//|         :param int de_idle_high: True if the de signal is high in IDLE state
//|         :param int pclk_active_high: True if the display data is clocked out at the rising edge of dclk
//|         :param int pclk_idle_high: True if the dclk stays at high level in IDLE phase
//|         """
//|         ...
STATIC mp_obj_t dotclockframebuffer_framebuffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_de, ARG_vsync, ARG_hsync, ARG_dclk, ARG_red, ARG_green, ARG_blue,
           ARG_frequency, ARG_width, ARG_height,
           ARG_hsync_pulse_width, ARG_hsync_back_porch, ARG_hsync_front_porch, ARG_hsync_idle_low,
           ARG_vsync_pulse_width, ARG_vsync_back_porch, ARG_vsync_front_porch, ARG_vsync_idle_low,
           ARG_de_idle_high, ARG_pclk_active_high, ARG_pclk_idle_high,};

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_de, MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_obj = mp_const_none } },
        { MP_QSTR_vsync, MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_obj = mp_const_none } },
        { MP_QSTR_hsync, MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_obj = mp_const_none } },
        { MP_QSTR_dclk, MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_obj = mp_const_none } },
        { MP_QSTR_red, MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_obj = mp_const_none } },
        { MP_QSTR_green, MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_obj = mp_const_none } },
        { MP_QSTR_blue, MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_obj = mp_const_none } },

        { MP_QSTR_frequency, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },

        { MP_QSTR_hsync_pulse_width, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_hsync_back_porch, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_hsync_front_porch, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_hsync_idle_low, MP_ARG_BOOL | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_bool = false } },

        { MP_QSTR_vsync_pulse_width, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_vsync_back_porch, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_vsync_front_porch, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_int = 0 } },
        { MP_QSTR_vsync_idle_low, MP_ARG_BOOL | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_bool = false } },

        { MP_QSTR_de_idle_high, MP_ARG_BOOL | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_bool = false } },
        { MP_QSTR_pclk_active_high, MP_ARG_BOOL | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_bool = false } },
        { MP_QSTR_pclk_idle_high, MP_ARG_BOOL | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, {.u_bool = false } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const mcu_pin_obj_t *de = validate_obj_is_free_pin(args[ARG_de].u_obj, MP_QSTR_de);
    const mcu_pin_obj_t *vsync = validate_obj_is_free_pin(args[ARG_vsync].u_obj, MP_QSTR_vsync);
    const mcu_pin_obj_t *hsync = validate_obj_is_free_pin(args[ARG_hsync].u_obj, MP_QSTR_hsync);
    const mcu_pin_obj_t *dclk = validate_obj_is_free_pin(args[ARG_dclk].u_obj, MP_QSTR_dclk);

    uint8_t num_red, num_green, num_blue;
    const mcu_pin_obj_t *red_pins[8], *green_pins[8], *blue_pins[8];

    validate_list_is_free_pins(MP_QSTR_red, red_pins, (mp_int_t)MP_ARRAY_SIZE(red_pins), args[ARG_red].u_obj, &num_red);
    validate_list_is_free_pins(MP_QSTR_green, green_pins, (mp_int_t)MP_ARRAY_SIZE(green_pins), args[ARG_green].u_obj, &num_green);
    validate_list_is_free_pins(MP_QSTR_blue, blue_pins, (mp_int_t)MP_ARRAY_SIZE(blue_pins), args[ARG_blue].u_obj, &num_blue);

    dotclockframebuffer_framebuffer_obj_t *self = &allocate_display_bus_or_raise()->dotclock;
    self->base.type = &dotclockframebuffer_framebuffer_type;

    common_hal_dotclockframebuffer_framebuffer_construct(
        self, de, vsync, hsync, dclk, red_pins, num_red, blue_pins, num_blue, green_pins, num_green,
        args[ARG_frequency].u_int, args[ARG_width].u_int, args[ARG_height].u_int,
        args[ARG_hsync_pulse_width].u_int, args[ARG_hsync_back_porch].u_int, args[ARG_hsync_front_porch].u_int, args[ARG_hsync_idle_low].u_bool,
        args[ARG_vsync_pulse_width].u_int, args[ARG_vsync_back_porch].u_int, args[ARG_vsync_front_porch].u_int, args[ARG_vsync_idle_low].u_bool,
        args[ARG_de_idle_high].u_bool,
        args[ARG_pclk_active_high].u_bool,
        args[ARG_pclk_idle_high].u_bool
        );

    return self;
}

static void check_for_deinit(dotclockframebuffer_framebuffer_obj_t *self) {
    if (common_hal_dotclockframebuffer_framebuffer_deinitialized(self)) {
        raise_deinited_error();
    }
}

//|     refresh_rate: float
//|     """The pixel refresh rate of the display, in Hz"""
STATIC mp_obj_t dotclockframebuffer_framebuffer_get_refresh_rate(mp_obj_t self_in) {
    dotclockframebuffer_framebuffer_obj_t *self = (dotclockframebuffer_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_dotclockframebuffer_framebuffer_get_refresh_rate(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(dotclockframebuffer_framebuffer_get_refresh_rate_obj, dotclockframebuffer_framebuffer_get_refresh_rate);
MP_PROPERTY_GETTER(dotclockframebuffer_framebuffer_refresh_rate_obj,
    (mp_obj_t)&dotclockframebuffer_framebuffer_get_refresh_rate_obj);

//|     frequency: int
//|     """The pixel frequency of the display, in Hz"""
STATIC mp_obj_t dotclockframebuffer_framebuffer_get_frequency(mp_obj_t self_in) {
    dotclockframebuffer_framebuffer_obj_t *self = (dotclockframebuffer_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_dotclockframebuffer_framebuffer_get_frequency(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(dotclockframebuffer_framebuffer_get_frequency_obj, dotclockframebuffer_framebuffer_get_frequency);
MP_PROPERTY_GETTER(dotclockframebuffer_framebuffer_frequency_obj,
    (mp_obj_t)&dotclockframebuffer_framebuffer_get_frequency_obj);

//|     width: int
//|     """The width of the display, in pixels"""
STATIC mp_obj_t dotclockframebuffer_framebuffer_get_width(mp_obj_t self_in) {
    dotclockframebuffer_framebuffer_obj_t *self = (dotclockframebuffer_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_dotclockframebuffer_framebuffer_get_width(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(dotclockframebuffer_framebuffer_get_width_obj, dotclockframebuffer_framebuffer_get_width);
MP_PROPERTY_GETTER(dotclockframebuffer_framebuffer_width_obj,
    (mp_obj_t)&dotclockframebuffer_framebuffer_get_width_obj);

//|     height: int
//|     """The height of the display, in pixels"""
//|
STATIC mp_obj_t dotclockframebuffer_framebuffer_get_height(mp_obj_t self_in) {
    dotclockframebuffer_framebuffer_obj_t *self = (dotclockframebuffer_framebuffer_obj_t *)self_in;
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_dotclockframebuffer_framebuffer_get_height(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(dotclockframebuffer_framebuffer_get_height_obj, dotclockframebuffer_framebuffer_get_height);

MP_PROPERTY_GETTER(dotclockframebuffer_framebuffer_height_obj,
    (mp_obj_t)&dotclockframebuffer_framebuffer_get_height_obj);

STATIC mp_int_t dotclockframebuffer_framebuffer_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    dotclockframebuffer_framebuffer_obj_t *self = (dotclockframebuffer_framebuffer_obj_t *)self_in;
    // a readonly framebuffer would be unusual but not impossible
    if ((flags & MP_BUFFER_WRITE) && !(self->bufinfo.typecode & MP_OBJ_ARRAY_TYPECODE_FLAG_RW)) {
        return 1;
    }
    *bufinfo = self->bufinfo;
    bufinfo->typecode = 'H';
    return 0;
}

// These version exists so that the prototype matches the protocol,
// avoiding a type cast that can hide errors
STATIC void dotclockframebuffer_framebuffer_swapbuffers(mp_obj_t self_in, uint8_t *dirty_row_bitmap) {
    (void)dirty_row_bitmap;
}

STATIC void dotclockframebuffer_framebuffer_deinit_proto(mp_obj_t self_in) {
    common_hal_dotclockframebuffer_framebuffer_deinit(self_in);
}

STATIC float dotclockframebuffer_framebuffer_get_brightness_proto(mp_obj_t self_in) {
    return 1.0f;
}

STATIC bool dotclockframebuffer_framebuffer_set_brightness_proto(mp_obj_t self_in, mp_float_t value) {
    return false;
}

STATIC int dotclockframebuffer_framebuffer_get_width_proto(mp_obj_t self_in) {
    return common_hal_dotclockframebuffer_framebuffer_get_width(self_in);
}

STATIC int dotclockframebuffer_framebuffer_get_height_proto(mp_obj_t self_in) {
    return common_hal_dotclockframebuffer_framebuffer_get_height(self_in);
}

STATIC int dotclockframebuffer_framebuffer_get_color_depth_proto(mp_obj_t self_in) {
    return 16;
}

STATIC int dotclockframebuffer_framebuffer_get_bytes_per_cell_proto(mp_obj_t self_in) {
    return 1;
}

STATIC int dotclockframebuffer_framebuffer_get_native_frames_per_second_proto(mp_obj_t self_in) {
    return common_hal_dotclockframebuffer_framebuffer_get_refresh_rate(self_in);
}

STATIC void dotclockframebuffer_framebuffer_get_bufinfo(mp_obj_t self_in, mp_buffer_info_t *bufinfo) {
    dotclockframebuffer_framebuffer_obj_t *self = (dotclockframebuffer_framebuffer_obj_t *)self_in;

    *bufinfo = self->bufinfo;
}


STATIC const framebuffer_p_t dotclockframebuffer_framebuffer_proto = {
    MP_PROTO_IMPLEMENT(MP_QSTR_protocol_framebuffer)
    .get_bufinfo = dotclockframebuffer_framebuffer_get_bufinfo,
    .set_brightness = dotclockframebuffer_framebuffer_set_brightness_proto,
    .get_brightness = dotclockframebuffer_framebuffer_get_brightness_proto,
    .get_width = dotclockframebuffer_framebuffer_get_width_proto,
    .get_height = dotclockframebuffer_framebuffer_get_height_proto,
    .get_color_depth = dotclockframebuffer_framebuffer_get_color_depth_proto,
    .get_bytes_per_cell = dotclockframebuffer_framebuffer_get_bytes_per_cell_proto,
    .get_native_frames_per_second = dotclockframebuffer_framebuffer_get_native_frames_per_second_proto,
    .swapbuffers = dotclockframebuffer_framebuffer_swapbuffers,
    .deinit = dotclockframebuffer_framebuffer_deinit_proto,
};


STATIC const mp_rom_map_elem_t dotclockframebuffer_framebuffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&dotclockframebuffer_framebuffer_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&dotclockframebuffer_framebuffer_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_frequency), MP_ROM_PTR(&dotclockframebuffer_framebuffer_frequency_obj) },
    { MP_ROM_QSTR(MP_QSTR_refresh_rate), MP_ROM_PTR(&dotclockframebuffer_framebuffer_refresh_rate_obj) },
};
STATIC MP_DEFINE_CONST_DICT(dotclockframebuffer_framebuffer_locals_dict, dotclockframebuffer_framebuffer_locals_dict_table);

const mp_obj_type_t dotclockframebuffer_framebuffer_type = {
    { &mp_type_type },
    .name = MP_QSTR_DotClockFramebuffer,
    .make_new = dotclockframebuffer_framebuffer_make_new,
    .locals_dict = (mp_obj_dict_t *)&dotclockframebuffer_framebuffer_locals_dict,
    MP_TYPE_EXTENDED_FIELDS(
        .buffer_p = { .get_buffer = dotclockframebuffer_framebuffer_get_buffer, },
        .protocol = &dotclockframebuffer_framebuffer_proto,
        ),
};
