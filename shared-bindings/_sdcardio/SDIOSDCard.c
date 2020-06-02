/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Scott Shawcroft
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

// This file contains all of the Python API definitions for the
// _sdcardio.SDIOSDCard class.

#if CIRCUITPY_SDCARDIO_SDIO
#include <string.h>

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/_sdcardio/SDIOSDCard.h"
#include "shared-bindings/util.h"

#include "lib/utils/buffer_helper.h"
#include "lib/utils/context_manager_helpers.h"
#include "py/mperrno.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "supervisor/shared/translate.h"

//| class SDIOSDCard:
//|     """SD Card Block Interface with SDIO
//|
//|     Controls an SD card over SDIO.  SDIO is a parallel protocol designed
//|     for SD cards.  It uses a clock pin, a command pin, and 1 or 4
//|     data pins.  It can be operated at a high frequency such as
//|     25MHz.  Usually an SDIOSDCard object is used with ``storage.VfsFat``
//|     to allow file I/O to an SD card."""
//|
//|     def __init__(*, clock, command, data, frequency):
//|         """Construct an SDIO SD Card object with the given properties
//|
//|         :param ~microcontroller.Pin clock: the pin to use for the clock.
//|         :param ~microcontroller.Pin command: the pin to use for the command.
//|         :param data: A sequence of pins to use for data.
//|         :param frequency: The frequency of the bus in Hz
//|
//|         Example usage:
//|
//|         .. code-block:: python
//|
//|             import os
//|
//|             import board
//|             import _sdcardio
//|             import storage
//|
//|             sd = _sdcardio.SDIOSDCard(
//|                 clock=board.SDIO_CLOCK,
//|                 command=board.SDIO_COMMAND,
//|                 data=board.SDIO_DATA,
//|                 frequency=25000000)
//|             vfs = storage.VfsFat(sd)
//|             storage.mount(vfs, '/sd')
//|             os.listdir('/sd')"""
//|         ...
//|

STATIC mp_obj_t sdcardio_sdiosdcard_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    sdcardio_sdiosdcard_obj_t *self = m_new_obj(sdcardio_sdiosdcard_obj_t);
    self->base.type = &sdcardio_sdiosdcard_type;
    enum { ARG_clock, ARG_command, ARG_data, ARG_frequency, NUM_ARGS };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_clock, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ },
        { MP_QSTR_command, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ },
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ },
        { MP_QSTR_frequency, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT },
    };
    MP_STATIC_ASSERT( MP_ARRAY_SIZE(allowed_args) == NUM_ARGS );
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];

    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const mcu_pin_obj_t* clock = validate_obj_is_free_pin(args[ARG_clock].u_obj);
    const mcu_pin_obj_t* command = validate_obj_is_free_pin(args[ARG_command].u_obj);
    mcu_pin_obj_t *data_pins[MP_ARRAY_SIZE(((sdcardio_sdiosdcard_obj_t*)0)->data)];
    uint8_t num_data;
    validate_list_is_free_pins(MP_QSTR_data, data_pins, MP_ARRAY_SIZE(data_pins), args[ARG_data].u_obj, &num_data);

    common_hal_sdcardio_sdio_construct(self, clock, command, num_data, data_pins, args[ARG_frequency].u_int);
    return MP_OBJ_FROM_PTR(self);
}

STATIC void check_for_deinit(sdcardio_sdiosdcard_obj_t *self) {
    if (common_hal_sdcardio_sdio_deinited(self)) {
        raise_deinited_error();
    }
}

//|     def configure(*, frequency=0, width=0) -> None:
//|         """Configures the SDIO bus.
//|
//|         :param int frequency: the desired clock rate in Hertz. The actual clock rate may be higher or lower due to the granularity of available clock settings.  Check the `frequency` attribute for the actual clock rate.
//|         :param int width: the number of data lines to use.  Must be 1 or 4 and must also not exceed the number of data lines at construction
//|
//|         .. note:: Leaving a value unspecified or 0 means the current setting is kept"""
//|
STATIC mp_obj_t sdcardio_sdiosdcard_configure(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_frequency, ARG_width, NUM_ARGS };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_frequency, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_width, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    sdcardio_sdiosdcard_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    check_for_deinit(self);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    MP_STATIC_ASSERT( MP_ARRAY_SIZE(allowed_args) == NUM_ARGS );
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t frequency = args[ARG_frequency].u_int;
    if (frequency < 0) {
        mp_raise_ValueError_varg(translate("Invalid %q"), MP_QSTR_baudraste);
    }

    uint8_t width = args[ARG_width].u_int;
    if (width != 0 && width != 1 && width != 4) {
        mp_raise_ValueError_varg(translate("Invalid %q"), MP_QSTR_width);
    }

    if (!common_hal_sdcardio_sdio_configure(self, frequency, width)) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(sdcardio_sdiosdcard_configure_obj, 1, sdcardio_sdiosdcard_configure);

//|     def count() -> int:
//|         """Returns the total number of sectors
//|
//|         Due to technical limitations, this is a function and not a property.
//|
//|         :return: The number of 512-byte blocks, as a number"""
//|
STATIC mp_obj_t sdcardio_sdiosdcard_count(mp_obj_t self_in) {
    sdcardio_sdiosdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_sdcardio_sdio_get_count(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(sdcardio_sdiosdcard_count_obj, sdcardio_sdiosdcard_count);

//|     def readblocks(start_block: int, buf: bytearray) -> None:
//|
//|         """Read one or more blocks from the card
//|
//|         :param int start_block: The block to start reading from
//|         :param bytearray buf: The buffer to write into.  Length must be multiple of 512.
//|
//|         :return: None"""
mp_obj_t sdcardio_sdiosdcard_readblocks(mp_obj_t self_in, mp_obj_t start_block_in, mp_obj_t buf_in) {
    uint32_t start_block = mp_obj_get_int(start_block_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);
    sdcardio_sdiosdcard_obj_t *self = (sdcardio_sdiosdcard_obj_t*)self_in;
    int result = common_hal_sdcardio_sdio_readblocks(self, start_block, &bufinfo);
    if (result < 0) {
        mp_raise_OSError(-result);
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(sdcardio_sdiosdcard_readblocks_obj, sdcardio_sdiosdcard_readblocks);

//|     def writeblocks(start_block: int, buf: bytearray) -> None:
//|
//|         """Write one or more blocks to the card
//|
//|         :param int start_block: The block to start writing from
//|         :param bytearray buf: The buffer to read from.  Length must be multiple of 512.
//|
//|         :return: None"""
//|
mp_obj_t sdcardio_sdiosdcard_writeblocks(mp_obj_t self_in, mp_obj_t start_block_in, mp_obj_t buf_in) {
    uint32_t start_block = mp_obj_get_int(start_block_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);
    sdcardio_sdiosdcard_obj_t *self = (sdcardio_sdiosdcard_obj_t*)self_in;
    int result = common_hal_sdcardio_sdio_writeblocks(self, start_block, &bufinfo);
    if (result < 0) {
        mp_raise_OSError(-result);
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(sdcardio_sdiosdcard_writeblocks_obj, sdcardio_sdiosdcard_writeblocks);

//|     @property
//|     def frequency(self) -> int:
//|         """The actual SDIO bus frequency. This may not match the frequency
//|         requested due to internal limitations."""
//|         ...
//|
STATIC mp_obj_t sdcardio_sdiosdcard_obj_get_frequency(mp_obj_t self_in) {
    sdcardio_sdiosdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_sdcardio_sdio_get_frequency(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(sdcardio_sdiosdcard_get_frequency_obj, sdcardio_sdiosdcard_obj_get_frequency);

const mp_obj_property_t sdcardio_sdiosdcard_frequency_obj = {
    .base.type = &mp_type_property,
    .proxy = {(mp_obj_t)&sdcardio_sdiosdcard_get_frequency_obj,
              (mp_obj_t)&mp_const_none_obj,
              (mp_obj_t)&mp_const_none_obj},
};

//|     @property
//|     def width(self) -> int:
//|         """The actual SDIO bus width, in bits"""
//|         ...
//|
STATIC mp_obj_t sdcardio_sdiosdcard_obj_get_width(mp_obj_t self_in) {
    sdcardio_sdiosdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return MP_OBJ_NEW_SMALL_INT(common_hal_sdcardio_sdio_get_width(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(sdcardio_sdiosdcard_get_width_obj, sdcardio_sdiosdcard_obj_get_width);

const mp_obj_property_t sdcardio_sdiosdcard_width_obj = {
    .base.type = &mp_type_property,
    .proxy = {(mp_obj_t)&sdcardio_sdiosdcard_get_width_obj,
              (mp_obj_t)&mp_const_none_obj,
              (mp_obj_t)&mp_const_none_obj},
};

//|     def deinit() -> None:
//|         """Disable permanently.
//|
//|         :return: None"""
STATIC mp_obj_t sdcardio_sdiosdcard_obj_deinit(mp_obj_t self_in) {
    sdcardio_sdiosdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_sdcardio_sdio_deinit(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(sdcardio_sdiosdcard_deinit_obj, sdcardio_sdiosdcard_obj_deinit);

//|     def __enter__(self, ) -> Any:
//|         """No-op used by Context Managers.
//|         Provided by context manager helper."""
//|         ...
//|

//|     def __exit__(self, ) -> Any:
//|         """Automatically deinitializes the hardware when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
//|
STATIC mp_obj_t sdcardio_sdiosdcard_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_sdcardio_sdio_deinit(args[0]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sdcardio_sdiosdcard_obj___exit___obj, 4, 4, sdcardio_sdiosdcard_obj___exit__);

STATIC const mp_rom_map_elem_t sdcardio_sdiosdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&sdcardio_sdiosdcard_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&sdcardio_sdiosdcard_obj___exit___obj) },

    { MP_ROM_QSTR(MP_QSTR_configure), MP_ROM_PTR(&sdcardio_sdiosdcard_configure_obj) },
    { MP_ROM_QSTR(MP_QSTR_frequency), MP_ROM_PTR(&sdcardio_sdiosdcard_frequency_obj) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&sdcardio_sdiosdcard_width_obj) },

    { MP_ROM_QSTR(MP_QSTR_count), MP_ROM_PTR(&sdcardio_sdiosdcard_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&sdcardio_sdiosdcard_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&sdcardio_sdiosdcard_writeblocks_obj) },

// Methods in STM HAL:
// InitCard
// ReadBlocks
// WriteBlocks
// Erase
// GetCardState
// GetCardCID
// GetCardCSD
// GetCardInfo
// GetState
// GetError
// Abort

};
STATIC MP_DEFINE_CONST_DICT(sdcardio_sdiosdcard_locals_dict, sdcardio_sdiosdcard_locals_dict_table);

const mp_obj_type_t sdcardio_sdiosdcard_type = {
   { &mp_type_type },
   .name = MP_QSTR_SDIO,
   .make_new = sdcardio_sdiosdcard_make_new,
   .locals_dict = (mp_obj_dict_t*)&sdcardio_sdiosdcard_locals_dict,
};
#endif
