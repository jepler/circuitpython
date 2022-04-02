/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Dan Halbert for Adafruit Industries
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

#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_UTIL_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_UTIL_H

#include "py/runtime.h"
#include "py/gc.h"

#include "shared-bindings/util.h"
#include "supervisor/shared/translate.h"

// If so, deinit() has already been called on the object, so complain.
void raise_deinited_error(void) {
    mp_raise_ValueError(translate("Object has been deinitialized and can no longer be used. Create a new object."));
}

static void deinited_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    raise_deinited_error();
}

static mp_obj_t deinited_unary_op(mp_unary_op_t op, mp_obj_t unused) {
    raise_deinited_error();
}

static mp_obj_t deinited_binary_op(mp_binary_op_t op, mp_obj_t unused1, mp_obj_t unused2) {
    raise_deinited_error();
}


static mp_obj_t deinited_call(mp_obj_t fun, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    raise_deinited_error();
}

static mp_obj_t deinited_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value) {
    raise_deinited_error();
}

const mp_obj_type_t mp_type_DeinitedType = {
    { &mp_type_type },
    .flags = MP_TYPE_FLAG_EXTENDED,
    .name = MP_QSTR_DeinitedType,
    .attr = deinited_attr,
    MP_TYPE_EXTENDED_FIELDS(
        .unary_op = deinited_unary_op,
        .binary_op = deinited_binary_op,
        .call = deinited_call,
        .subscr = deinited_subscr,
        )
};

void deinit_object(mp_obj_base_t *ptr) {
    if (gc_alloc_possible() && VERIFY_PTR((void *)ptr)) {
        gc_realloc(ptr, sizeof(mp_obj_base_t), false); // shrink allocation if possible
    }
    ptr->type = &mp_type_DeinitedType;
}

#endif // MICROPY_INCLUDED_SHARED_BINDINGS_UTIL_H
