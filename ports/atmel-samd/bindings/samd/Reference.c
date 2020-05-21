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

#include "py/obj.h"
#include "py/runtime.h"

#include "bindings/samd/Reference.h"

typedef struct {
    mp_obj_base_t base;
    int16_t value;
    int16_t name;
} cp_enum_obj_t;

#define MAKE_ENUM_VALUE(type, prefix, name, value) \
const cp_enum_obj_t prefix ## name ## _obj = { \
    { &type }, value, MP_QSTR_ ## name, \
}

#define MAKE_ENUM_MAP_ENTRY(prefix, name) \
    { MP_ROM_QSTR(MP_QSTR_ ## name), MP_ROM_PTR(&prefix ## name ## _obj) }

mp_obj_t cp_enum_find(const mp_obj_dict_t *dict, int value) {
    for (int i=0; i<dict->map.used; i++) {
        const cp_enum_obj_t *v = dict->map.table[i].value;
        if (v->value == value) {
            return (mp_obj_t)v;
        }
    }
    return mp_const_none;
}

int cp_enum_value(const mp_obj_type_t *type, mp_obj_t *obj) {
    if (!MP_OBJ_IS_TYPE(obj, type)) {
        mp_raise_TypeError_varg(translate("Expected a %q"), type->name);
    }
    return ((cp_enum_obj_t*)MP_OBJ_TO_PTR(obj))->value;
}

void cp_enum_obj_print_helper(uint16_t module, const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cp_enum_obj_t *self = self_in;
    mp_printf(print, "%q.%q.%q", module, self->base.type->name, self->name);
}

const mp_obj_type_t reference_voltage_type;

MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V1_0, 0);
MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V1_1, 1);
MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V1_2, 2);
MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V1_25, 3);
MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V2_0, 4);
MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V2_2, 5);
MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V2_4, 6);
MAKE_ENUM_VALUE(reference_voltage_type, reference_voltage_, V2_5, 7);

STATIC const mp_rom_map_elem_t reference_voltage_locals_table[] = {
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V1_0),
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V1_1),
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V1_2),
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V1_25),
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V2_0),
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V2_2),
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V2_4),
    MAKE_ENUM_MAP_ENTRY(reference_voltage_, V2_5),
};
STATIC MP_DEFINE_CONST_DICT(reference_voltage_locals_dict, reference_voltage_locals_table);

STATIC void reference_voltage_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cp_enum_obj_print_helper(MP_QSTR_samd, print, self_in, kind);
}

const mp_obj_type_t reference_voltage_type = {
    { &mp_type_type },
    .name = MP_QSTR_ReferenceVoltage,
    .print = reference_voltage_print,
    .locals_dict = (mp_obj_t)&reference_voltage_locals_dict,
};

STATIC mp_obj_t reference_voltage_get(void) {
    return cp_enum_find(&reference_voltage_locals_dict, SUPC->VREF.bit.SEL);
}
MP_DEFINE_CONST_FUN_OBJ_0(reference_voltage_get_obj, reference_voltage_get);

STATIC mp_obj_t reference_voltage_set(mp_obj_t arg) {
    int i = cp_enum_value(&reference_voltage_type, arg);
    SUPC->VREF.bit.SEL = i;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(reference_voltage_set_obj, reference_voltage_set);

const mp_obj_property_t reference_voltage_obj = {
    .base.type = &mp_type_property,
    .proxy = {(mp_obj_t)&reference_voltage_get_obj,
              (mp_obj_t)&reference_voltage_set_obj,
              (mp_obj_t)&mp_const_none_obj}
};

const mp_obj_type_t dac_reference_type;

MAKE_ENUM_VALUE(dac_reference_type, dac_reference_, VREFAU, 0);
MAKE_ENUM_VALUE(dac_reference_type, dac_reference_, VDDANA, 1);
MAKE_ENUM_VALUE(dac_reference_type, dac_reference_, VREFAB, 2);
MAKE_ENUM_VALUE(dac_reference_type, dac_reference_, INTREF, 3);

STATIC const mp_rom_map_elem_t dac_reference_locals_table[] = {
    MAKE_ENUM_MAP_ENTRY(dac_reference_, VREFAU),
    MAKE_ENUM_MAP_ENTRY(dac_reference_, VDDANA),
    MAKE_ENUM_MAP_ENTRY(dac_reference_, VREFAB),
    MAKE_ENUM_MAP_ENTRY(dac_reference_, INTREF),
};
STATIC MP_DEFINE_CONST_DICT(dac_reference_locals_dict, dac_reference_locals_table);

STATIC void dac_reference_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    cp_enum_obj_print_helper(MP_QSTR_samd, print, self_in, kind);
}

const mp_obj_type_t dac_reference_type = {
    { &mp_type_type },
    .name = MP_QSTR_DacReference,
    .print = dac_reference_print,
    .locals_dict = (mp_obj_t)&dac_reference_locals_dict,
};

STATIC int dac_reference_value_default;

int dac_reference_get_default_value(void) {
    return dac_reference_value_default;
}

STATIC mp_obj_t dac_reference_get(void) {
    return cp_enum_find(&dac_reference_locals_dict, dac_reference_value_default);
}
MP_DEFINE_CONST_FUN_OBJ_0(dac_reference_get_obj, dac_reference_get);

STATIC mp_obj_t dac_reference_set(mp_obj_t arg) {
    int i = cp_enum_value(&dac_reference_type, arg);
    dac_reference_value_default = i;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(dac_reference_set_obj, dac_reference_set);


const mp_obj_property_t dac_reference_obj = {
    .base.type = &mp_type_property,
    .proxy = {(mp_obj_t)&dac_reference_get_obj,
              (mp_obj_t)&dac_reference_set_obj,
              (mp_obj_t)&mp_const_none_obj}
};
