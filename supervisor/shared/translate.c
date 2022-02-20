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

#include "supervisor/shared/translate.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef NO_QSTR
#include "genhdr/compression.generated.h"
#endif

#include "py/misc.h"
#include "py/mpprint.h"
#include "supervisor/serial.h"

#include "lib/uzlib/tinf.h"

void serial_write_compressed(const compressed_string_t *compressed) {
    mp_printf(MP_PYTHON_PRINTER, "%S", compressed);
}

typedef struct {
    uint16_t max_length;
    uint8_t data[];
} compressed_message_data;

extern const compressed_message_data compressed_messages;
enum { dict_sz = 1 << 10 };

typedef struct {
    TINF_DATA decomp;
    uint8_t *ptr;
} message_reader;

STATIC int read_src_messages(TINF_DATA *data) {
    message_reader *reader = (message_reader *)data;
    return *reader->ptr++;
}

static void decompress_common(message_reader *o, char *dict, size_t id) {
    memset(o, 0, sizeof(*o));
    o->decomp.readSource = read_src_messages;
    o->ptr = (uint8_t *)compressed_messages.data;
    uzlib_uncompress_init(&o->decomp, dict, dict_sz);

    // count NULs until reaching our message..
    for (size_t i = 0; i < id;) {
        uint8_t b;
        o->decomp.dest = &b;
        o->decomp.dest_limit = o->decomp.dest + 1;
        int st = uzlib_uncompress(&o->decomp);
        (void)st;
        size_t n = o->decomp.dest - &b;
        (void)n;
        assert(st == 0);
        assert(n == 1);
        if (!b) {
            i++;
        }
    }
}

uint16_t decompress_max_length(void) {
    return compressed_messages.max_length;
}

uint16_t decompress_length(const compressed_string_t *compressed) {
    size_t id = (size_t)compressed;
    message_reader o;
    char dict[dict_sz];
    decompress_common(&o, dict, id);

    uint8_t b;
    uint16_t result = 0;
    do {
        o.decomp.dest = &b;
        o.decomp.dest_limit = o.decomp.dest + 1;
        int st = uzlib_uncompress(&o.decomp);
        (void)st;
        size_t n = o.decomp.dest - &b;
        (void)n;
        assert(st == 0);
        assert(n == 1);
        result++;
    } while (b);

    return compressed_messages.max_length;
}

void decompress(const compressed_string_t *compressed, char *decompressed, size_t decompress_len) {
    size_t id = (size_t)compressed;
    message_reader o;
    char dict[dict_sz];
    decompress_common(&o, dict, id);

    o.decomp.dest = (uint8_t *)decompressed;
    o.decomp.dest_limit = (uint8_t *)decompressed + decompress_len;
    int st = uzlib_uncompress(&o.decomp);
    (void)st;
    assert(st == 0);
}

inline
// gcc10 -flto has issues with this being always_inline for debug builds.
#if CIRCUITPY_DEBUG < 1
__attribute__((always_inline))
#endif
const compressed_string_t *translate(const char *original) {
    #ifndef NO_QSTR
    #define QDEF(id, hash, len, str)
    #define TRANSLATION_DATA(...)
    #define TRANSLATION(id, idx) if (strcmp(original, id) == 0) { return (compressed_string_t *)idx; } else
    #include "genhdr/qstrdefs.generated.h"
#undef TRANSLATION
#undef TRANSLATION_DATA
#undef QDEF
    #endif
    return NULL;
}

#pragma GCC diagnostic ignored "-Wsign-compare"
#if !MICROPY_PY_UZLIB
#include "lib/uzlib/tinflate.c"
#endif

#ifndef NO_QSTR
__attribute__((section("translationdata")))
const compressed_message_data compressed_messages = {
#define QDEF(id, hash, len, str)
#define TRANSLATION(id, idx)
#define TRANSLATION_DATA(...) __VA_ARGS__
    #include "genhdr/qstrdefs.generated.h"
#undef TRANSLATION_DATA
#undef TRANSLATION
#undef QDEF
};
#endif
