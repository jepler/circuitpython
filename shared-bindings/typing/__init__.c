/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * SPDX-FileCopyrightText: Copyright (c) 2022 Dan Halbert for Adafruit Industries
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

//| """Typing features module
//|
//| This module is intended to be a compatible subset of `Python's module
//| of the same name <https://docs.python.org/3.10/library/typing.html>`_
//| """
//|
//| TYPE_CHECKING: bool = False
//| """
//| indicates that expensive type checks should be skipped.
//|
//| In CircuitPython using the exact following sequence prevents the body
//| of the 'if' statement (including the qstr identifiers it uses)
//| from being included in the mpy file at all::
//|
//|     from typing import TYPE_CHECKING
//|     if TYPE_CHECKING:
//|         import module_may_not_even_exist_in_circuitpython
//|
//| Together with ``from __future__ import typing`` this can help
//| reduce the memory impact of type annotations in CircuitPython
//| programs.
//| """

STATIC const mp_rom_map_elem_t future_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR___future__) },

    { MP_ROM_QSTR(MP_QSTR_TYPE_CHECKING), mp_const_false },
};

STATIC MP_DEFINE_CONST_DICT(future_module_globals, future_module_globals_table);

const mp_obj_module_t future_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&future_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR___future__, future_module, CIRCUITPY_FUTURE);
