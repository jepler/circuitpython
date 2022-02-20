/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
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

#ifndef MICROPY_INCLUDED_SUPERVISOR_TRANSLATE_H
#define MICROPY_INCLUDED_SUPERVISOR_TRANSLATE_H

#include <stdint.h>
#include <stddef.h>

// For legacy reasons, compressed string IDs are passed as a pointer
// to compressed_string_t.  However, they are actually identifiers,
// and there is no compressed_string structure.
typedef struct compressed_string compressed_string_t;

// Return the compressed, translated version of a source string
// Usually, due to LTO, this is optimized into a load of a constant
// pointer.
const compressed_string_t *translate(const char *c);
void serial_write_compressed(const compressed_string_t *compressed);
void decompress(const compressed_string_t *compressed, char *decompressed, size_t decompress_len);
uint16_t decompress_length(const compressed_string_t *compressed);
uint16_t decompress_max_length(void);


// Map MicroPython's error messages to our translations.
#if defined(MICROPY_ENABLE_DYNRUNTIME) && MICROPY_ENABLE_DYNRUNTIME
#define MP_ERROR_TEXT(x) (x)
#else
#define MP_ERROR_TEXT(x) translate(x)
#endif

#endif  // MICROPY_INCLUDED_SUPERVISOR_TRANSLATE_H
