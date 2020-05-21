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


#ifndef MICROPY_INCLUDED_ATMEL_SAMD_BINDINGS_SAMD_REFERENCE_H
#define MICROPY_INCLUDED_ATMEL_SAMD_BINDINGS_SAMD_REFERENCE_H

#include "py/obj.h"
#include "py/objproperty.h"

MP_DECLARE_CONST_FUN_OBJ_0(reference_voltage_get_obj);
MP_DECLARE_CONST_FUN_OBJ_1(reference_voltage_set_obj);
MP_DECLARE_CONST_FUN_OBJ_0(dac_reference_get_obj);
MP_DECLARE_CONST_FUN_OBJ_1(dac_reference_set_obj);

extern const mp_obj_property_t reference_voltage_obj;
extern const mp_obj_type_t reference_voltage_type;

extern const mp_obj_property_t dac_reference_obj;
extern const mp_obj_type_t dac_reference_type;

extern int dac_reference_get_default_value(void);
#endif
