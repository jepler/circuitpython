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

// These helpers move MicroPython objects and their sub-objects to the long lived portion of the
// heap.

#ifndef MICROPY_INCLUDED_PY_GC_LONG_LIVED_H
#define MICROPY_INCLUDED_PY_GC_LONG_LIVED_H

#include "py/objfun.h"
#include "py/objproperty.h"
#include "py/objstr.h"

#if CIRCUITPY_LONG_LIVED_GC
mp_obj_t make_obj_long_lived(mp_obj_t obj);
mp_obj_t make_obj_long_lived_depth(mp_obj_t obj, uint8_t depth);
#else
#define make_obj_long_lived(obj) (obj)
#define make_obj_long_lived_depth(obj, depth) (obj)
#endif

#endif // MICROPY_INCLUDED_PY_GC_LONG_LIVED_H
