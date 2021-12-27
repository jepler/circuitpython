/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
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

#include "shared/runtime/buffer_helper.h"
#include "py/binary.h"

static void normalize_buffer_bounds(mp_buffer_info_t *bufinfo, int32_t start, int32_t end) {
    #if defined(CIRCUITPY_BOUNDS_ARE_ELEMENTS) // incompatible change to potentially be made in CP8
    int stride_in_bytes = mp_binary_get_size('@', bufinfo->typecode, NULL);

    start *= stride_in_bytes;
    end *= stride_in_bytes;
    #endif

    if (end < 0) {
        end += bufinfo->len;
    } else if (((size_t)end) > bufinfo->len) {
        end = bufinfo->len;
    }
    if (start < 0) {
        start += bufinfo->len;
    }
    if (end < start) {
        bufinfo->len = 0;
    } else {
        bufinfo->len = end - start;
    }
}

void get_normalized_buffer_raise(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags, int32_t start, int32_t end) {
    mp_get_buffer_raise(obj, bufinfo, flags);
    normalize_buffer_bounds(bufinfo, start, end);
}
