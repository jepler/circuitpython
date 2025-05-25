// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/audiosdl/__init__.h"
#include "shared-bindings/audiosdl/AudioOut.h"

//| """Support for audio output
//|
//| The `audiosdl` module contains classes to provide access to audio IO using SDL.
//|
//| All classes change hardware state and should be deinitialized when they
//| are no longer needed if the program continues after use. To do so, either
//| call :py:meth:`!deinit` or use a context manager. See
//| :ref:`lifetime-and-contextmanagers` for more info.
//|
//| Since CircuitPython 5, `RawSample` and `WaveFile` are moved
//| to :mod:`audiocore`, and `Mixer` is moved to :mod:`audiomixer`."""

static const mp_rom_map_elem_t audiosdl_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_audiosdl) },
    { MP_ROM_QSTR(MP_QSTR_AudioOut), MP_ROM_PTR(&audiosdl_audioout_type) },
};

static MP_DEFINE_CONST_DICT(audiosdl_module_globals, audiosdl_module_globals_table);

const mp_obj_module_t audiosdl_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&audiosdl_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_audiosdl, audiosdl_module);
