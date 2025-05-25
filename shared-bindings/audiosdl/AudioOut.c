// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2025 Jeff Epler
//
// SPDX-License-Identifier: MIT

#include "py/runtime.h"
#include "py/objproperty.h"
#include "shared-bindings/audiosdl/AudioOut.h"
#include "shared-bindings/util.h"
#include "shared/runtime/context_manager_helpers.h"

//|
//| class AudioOut:
//|     """Output an analog audio signal using SDL"""
//|
//|     def __init__(
//|         self,
//|         sample_rate: int):
//|         """Create an SDL audio device with the given sample rate
//|
//|         Due to technical limitations, the sample rate and audio sample format
//|         are fixed at construction time. The audio data is always 16-bit
//|         signed.""
static mp_obj_t audiosdl_audioout_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_sample_rate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_sample_rate, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    // create AudioOut object from the given pin
    audiosdl_audioout_obj_t *self = mp_obj_malloc_with_finaliser(audiosdl_audioout_obj_t, &audiosdl_audioout_type);
    common_hal_audiosdl_audioout_construct(self, args[ARG_sample_rate].u_int);

    return self;
}

//|     def deinit(self) -> None:
//|         """Deinitialises the AudioOut and releases any hardware resources for reuse."""
//|         ...
//|
static mp_obj_t audiosdl_audioout_deinit(mp_obj_t self_in) {
    audiosdl_audioout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_audiosdl_audioout_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(audiosdl_audioout_deinit_obj, audiosdl_audioout_deinit);

static void check_for_deinit(audiosdl_audioout_obj_t *self) {
    if (common_hal_audiosdl_audioout_deinited(self)) {
        raise_deinited_error();
    }
}
//|     def __enter__(self) -> AudioOut:
//|         """No-op used by Context Managers."""
//|         ...
//|
//  Provided by context manager helper.

//|     def __exit__(self) -> None:
//|         """Automatically deinitializes the hardware when exiting a context. See
//|         :ref:`lifetime-and-contextmanagers` for more info."""
//|         ...
//|
//  Provided by context manager helper.

//|     def play(self, sample: circuitpython_typing.AudioSample, *, loop: bool = False) -> None:
//|         """Plays the sample once when loop=False and continuously when loop=True.
//|         Does not block. Use `playing` to block.
//|
//|         Sample must be an `audiocore.WaveFile`, `audiocore.RawSample`, `audiomixer.Mixer` or `audiomp3.MP3Decoder`.
//|
//|         The sample itself should consist of 16 bit samples. Microcontrollers with a lower output
//|         resolution will use the highest order bits to output. For example, the SAMD21 has a 10 bit
//|         DAC that ignores the lowest 6 bits when playing 16 bit samples."""
//|         ...
//|
static mp_obj_t audiosdl_audioout_obj_play(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_sample, ARG_loop };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_sample,    MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_ROM_NONE } },
        { MP_QSTR_loop,      MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
    };
    audiosdl_audioout_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    check_for_deinit(self);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t sample = args[ARG_sample].u_obj;
    common_hal_audiosdl_audioout_play(self, sample, args[ARG_loop].u_bool);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(audiosdl_audioout_play_obj, 1, audiosdl_audioout_obj_play);

//|     def stop(self) -> None:
//|         """Stops playback and resets to the start of the sample."""
//|         ...
//|
static mp_obj_t audiosdl_audioout_obj_stop(mp_obj_t self_in) {
    audiosdl_audioout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    common_hal_audiosdl_audioout_stop(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(audiosdl_audioout_stop_obj, audiosdl_audioout_obj_stop);

//|     playing: bool
//|     """True when an audio sample is being output even if `paused`. (read-only)"""
//|
static mp_obj_t audiosdl_audioout_obj_get_playing(mp_obj_t self_in) {
    audiosdl_audioout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return mp_obj_new_bool(common_hal_audiosdl_audioout_get_playing(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(audiosdl_audioout_get_playing_obj, audiosdl_audioout_obj_get_playing);

MP_PROPERTY_GETTER(audiosdl_audioout_playing_obj,
    (mp_obj_t)&audiosdl_audioout_get_playing_obj);

//|     def pause(self) -> None:
//|         """Stops playback temporarily while remembering the position. Use `resume` to resume playback."""
//|         ...
//|
static mp_obj_t audiosdl_audioout_obj_pause(mp_obj_t self_in) {
    audiosdl_audioout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    if (!common_hal_audiosdl_audioout_get_playing(self)) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Not playing"));
    }
    common_hal_audiosdl_audioout_pause(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(audiosdl_audioout_pause_obj, audiosdl_audioout_obj_pause);

//|     def resume(self) -> None:
//|         """Resumes sample playback after :py:func:`pause`."""
//|         ...
//|
static mp_obj_t audiosdl_audioout_obj_resume(mp_obj_t self_in) {
    audiosdl_audioout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);

    if (common_hal_audiosdl_audioout_get_paused(self)) {
        common_hal_audiosdl_audioout_resume(self);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(audiosdl_audioout_resume_obj, audiosdl_audioout_obj_resume);

//|     paused: bool
//|     """True when playback is paused. (read-only)"""
//|
//|
static mp_obj_t audiosdl_audioout_obj_get_paused(mp_obj_t self_in) {
    audiosdl_audioout_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_for_deinit(self);
    return mp_obj_new_bool(common_hal_audiosdl_audioout_get_paused(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(audiosdl_audioout_get_paused_obj, audiosdl_audioout_obj_get_paused);

MP_PROPERTY_GETTER(audiosdl_audioout_paused_obj,
    (mp_obj_t)&audiosdl_audioout_get_paused_obj);

static const mp_rom_map_elem_t audiosdl_audioout_locals_dict_table[] = {
    // Methods
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&audiosdl_audioout_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&audiosdl_audioout_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&default___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&audiosdl_audioout_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&audiosdl_audioout_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&audiosdl_audioout_pause_obj) },
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&audiosdl_audioout_resume_obj) },

    // Properties
    { MP_ROM_QSTR(MP_QSTR_playing), MP_ROM_PTR(&audiosdl_audioout_playing_obj) },
    { MP_ROM_QSTR(MP_QSTR_paused), MP_ROM_PTR(&audiosdl_audioout_paused_obj) },
};
static MP_DEFINE_CONST_DICT(audiosdl_audioout_locals_dict, audiosdl_audioout_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    audiosdl_audioout_type,
    MP_QSTR_AudioOut,
    MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS,
    make_new, audiosdl_audioout_make_new,
    locals_dict, &audiosdl_audioout_locals_dict
    );
