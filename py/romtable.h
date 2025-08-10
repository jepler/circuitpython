/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Jeff Epler
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
#ifndef MICROPY_INCLUDED_PY_ROMTABLE_H
#define MICROPY_INCLUDED_PY_ROMTABLE_H

#include <boost/preprocessor/facilities/apply.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#define MP_TABLE_ENTRY(r, x, element) { \
        MP_ROM_QSTR(BOOST_PP_TUPLE_ELEM(0, element)), \
        BOOST_PP_TUPLE_ELEM(1, element) (BOOST_PP_TUPLE_ELEM(2, element)) \
},

#define MP_ROM_TABLE_MERGED(storage, name, contents) const storage mp_rom_map_elem_t name[] = { \
        BOOST_PP_SEQ_FOR_EACH(MP_TABLE_ENTRY, _, contents) \
}
#define MP_ROM_TABLE MP_ROM_TABLE_MERGED

// A hypothetical future format for ROM tables
#define MP_TABLE_KEY(r, x, element) BOOST_PP_TUPLE_ELEM(0, element),
#define MP_TABLE_VALUE(r, x, element) \
    BOOST_PP_EXPAND(BOOST_PP_TUPLE_ELEM(1, element) (BOOST_PP_TUPLE_ELEM(2, element))),
#define ROM_TABLE_SEPARATE(storage, name, contents) \
    const storage struct { \
        qstr_short_t keys[BOOST_PP_SEQ_SIZE(contents)], \
        mp_rom_obj_t values[BOOST_PP_SEQ_SIZE(contents)], \
    } name = { \
        { BOOST_PP_SEQ_FOR_EACH(MP_TABLE_KEY, _, contents) }, \
        { BOOST_PP_SEQ_FOR_EACH(MP_TABLE_VALUE, _, contents) }, \
    }

#endif
