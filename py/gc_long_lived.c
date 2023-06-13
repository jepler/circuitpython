/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries LLC
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

#include "py/emitglue.h"
#include "py/gc_long_lived.h"
#include "py/gc.h"
#include "py/mpstate.h"

STATIC mp_obj_fun_bc_t *make_fun_bc_long_lived(mp_obj_fun_bc_t *fun_bc, uint8_t max_depth);
STATIC mp_obj_property_t *make_property_long_lived(mp_obj_property_t *prop, uint8_t max_depth);
STATIC mp_obj_dict_t *make_dict_long_lived(mp_obj_dict_t *dict, uint8_t max_depth);
STATIC mp_obj_str_t *make_str_long_lived(mp_obj_str_t *str);
mp_obj_t make_obj_long_lived_depth(mp_obj_t obj, uint8_t max_depth);

STATIC mp_obj_fun_bc_t *make_fun_bc_long_lived(mp_obj_fun_bc_t *fun_bc, uint8_t max_depth) {
    #ifndef MICROPY_ENABLE_GC
    return fun_bc;
    #endif
    fun_bc->bytecode = gc_make_long_lived((byte *)fun_bc->bytecode);
    fun_bc->globals = make_dict_long_lived(fun_bc->globals, max_depth);
    for (uint32_t i = 0; i < gc_nbytes(fun_bc->const_table) / sizeof(mp_obj_t); i++) {
        // Skip things that aren't allocated on the heap (and hence have zero bytes.)
        if (gc_nbytes(MP_OBJ_TO_PTR(fun_bc->const_table[i])) == 0) {
            continue;
        }
        // Try to detect raw code.
        mp_raw_code_t *raw_code = MP_OBJ_TO_PTR(fun_bc->const_table[i]);
        if (raw_code->kind == MP_CODE_BYTECODE) {
            raw_code->fun_data = gc_make_long_lived((byte *)raw_code->fun_data);
            raw_code->const_table = gc_make_long_lived((byte *)raw_code->const_table);
        }
        ((mp_uint_t *)fun_bc->const_table)[i] = (mp_uint_t)make_obj_long_lived_depth(
            (mp_obj_t)fun_bc->const_table[i], max_depth);

    }
    fun_bc->const_table = gc_make_long_lived((mp_uint_t *)fun_bc->const_table);
    // extra_args stores keyword only argument default values.
    size_t words = gc_nbytes(fun_bc) / sizeof(mp_uint_t *);
    // Functions (mp_obj_fun_bc_t) have four pointers (base, globals, bytecode and const_table)
    // before the variable length extra_args so remove them from the length.
    for (size_t i = 0; i < words - 4; i++) {
        if (MP_OBJ_TO_PTR(fun_bc->extra_args[i]) == NULL) {
            continue;
        }
        if (mp_obj_is_type(fun_bc->extra_args[i], &mp_type_dict)) {
            fun_bc->extra_args[i] = MP_OBJ_FROM_PTR(make_dict_long_lived(MP_OBJ_TO_PTR(fun_bc->extra_args[i]), max_depth));
        } else {
            fun_bc->extra_args[i] = make_obj_long_lived_depth(fun_bc->extra_args[i], max_depth);
        }

    }
    return gc_make_long_lived(fun_bc);
}

STATIC mp_obj_property_t *make_property_long_lived(mp_obj_property_t *prop, uint8_t max_depth) {
    #ifndef MICROPY_ENABLE_GC
    return prop;
    #endif
    prop->proxy[0] = make_obj_long_lived_depth(prop->proxy[0], max_depth);
    prop->proxy[1] = make_obj_long_lived_depth(prop->proxy[1], max_depth);
    prop->proxy[2] = make_obj_long_lived_depth(prop->proxy[2], max_depth);
    return gc_make_long_lived(prop);
}

STATIC mp_obj_dict_t *make_dict_long_lived(mp_obj_dict_t *dict, uint8_t max_depth) {
    #ifndef MICROPY_ENABLE_GC
    return dict;
    #endif
    if (dict == &MP_STATE_VM(dict_main) || dict->map.is_fixed) {
        return dict;
    }
    // Don't recurse unnecessarily. Return immediately if we've already seen this dict.
    if (dict->map.scanning) {
        return dict;
    }
    // Mark that we're processing this dict.
    dict->map.scanning = 1;

    // Update all of the references first so that we reduce the chance of references to the old
    // copies.
    dict->map.table = gc_make_long_lived(dict->map.table);
    for (size_t i = 0; i < dict->map.alloc; i++) {
        if (mp_map_slot_is_filled(&dict->map, i)) {
            mp_obj_t value = dict->map.table[i].value;
            dict->map.table[i].value = make_obj_long_lived_depth(value, max_depth);
        }
    }
    dict = gc_make_long_lived(dict);
    // Done recursing through this dict.
    dict->map.scanning = 0;
    return dict;
}

mp_obj_str_t *make_str_long_lived(mp_obj_str_t *str) {
    str->data = gc_make_long_lived((byte *)str->data);
    return gc_make_long_lived(str);
}

mp_obj_t make_obj_long_lived_depth(mp_obj_t obj, uint8_t max_depth) {
    #ifndef MICROPY_ENABLE_GC
    return obj;
    #endif
    if (max_depth <= 0) {
        return obj;
    }
    max_depth -= 1;

    // This also catches the case that obj is a NULL pointer or is a
    // "non-object object" such as a number or a qstr.

    // If not in the GC pool, do nothing. This can happen (at least) when
    // there are frozen mp_type_bytes objects in ROM. This also catches the case that the object is MP_OBJ_NULL
    // and the case that it's not !mp_obj_is_obj
    void *ptr = MP_OBJ_TO_PTR(obj);
    if (!VERIFY_PTR((void *)ptr)) {
        return obj;
    }

    const mp_obj_type_t *type = ((mp_obj_base_t *)ptr)->type;

    if (type == &mp_type_fun_bc) {
        ptr = make_fun_bc_long_lived(ptr, max_depth);
    } else if (type == &mp_type_dict) {
        ptr = make_dict_long_lived(ptr, max_depth);
    } else if (type == &mp_type_property) {
        ptr = make_property_long_lived(ptr, max_depth);
    } else if (type == &mp_type_str || type == &mp_type_bytes) {
        ptr = make_str_long_lived(ptr);
    } else if (type != &mp_type_type) {
        // Types are already long lived during creation.
        ptr = gc_make_long_lived(ptr);
    }
    return MP_OBJ_FROM_PTR(ptr);
}

mp_obj_t make_obj_long_lived(mp_obj_t obj) {
    return make_obj_long_lived_depth(obj, 10);
}
