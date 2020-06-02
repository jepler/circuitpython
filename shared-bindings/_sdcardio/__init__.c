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

#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/_sdcardio/SPISDCard.h"

#if CIRCUITPY_SDCARDIO_SDIO
#include "shared-bindings/_sdcardio/SDIOSDCard.h"
#else
STATIC mp_obj_t sdcardio_sdiosdcard_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_raise_NotImplementedError(translate("No SDIO bus available"));
}

STATIC const mp_rom_map_elem_t sdcardio_sdiosdcard_locals_dict_table[] = {
};
STATIC MP_DEFINE_CONST_DICT(sdcardio_sdiosdcard_locals_dict, sdcardio_sdiosdcard_locals_dict_table);
const mp_obj_type_t sdcardio_sdiosdcard_type = {
   { &mp_type_type },
   .name = MP_QSTR_SDIO,
   .make_new = sdcardio_sdiosdcard_make_new,
   .locals_dict = (mp_obj_dict_t*)&sdcardio_sdiosdcard_locals_dict,
};
#endif

//| """Low-level routines for SD card I/O"""

STATIC const mp_rom_map_elem_t sdcardio_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__sdcardio) },
    { MP_ROM_QSTR(MP_QSTR_SPISDCard), MP_ROM_PTR(&sdcardio_SPISDCard_type) },
    { MP_ROM_QSTR(MP_QSTR_SDIOSDCard), MP_ROM_PTR(&sdcardio_sdiosdcard_type) },
};

STATIC MP_DEFINE_CONST_DICT(sdcardio_module_globals, sdcardio_module_globals_table);

const mp_obj_module_t sdcardio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&sdcardio_module_globals,
};
