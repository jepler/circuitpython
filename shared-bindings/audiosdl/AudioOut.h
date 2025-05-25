// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "shared-module/audiosdl/AudioOut.h"

typedef struct audiosdl_audioout_obj audiosdl_audioout_obj_t;

extern const mp_obj_type_t audiosdl_audioout_type;

void common_hal_audiosdl_audioout_construct(audiosdl_audioout_obj_t *self,
    mp_int_t sample_rate);

void common_hal_audiosdl_audioout_deinit(audiosdl_audioout_obj_t *self);
bool common_hal_audiosdl_audioout_deinited(audiosdl_audioout_obj_t *self);
void common_hal_audiosdl_audioout_play(audiosdl_audioout_obj_t *self, mp_obj_t sample, bool loop);
void common_hal_audiosdl_audioout_stop(audiosdl_audioout_obj_t *self);
bool common_hal_audiosdl_audioout_get_playing(audiosdl_audioout_obj_t *self);
void common_hal_audiosdl_audioout_pause(audiosdl_audioout_obj_t *self);
void common_hal_audiosdl_audioout_resume(audiosdl_audioout_obj_t *self);
bool common_hal_audiosdl_audioout_get_paused(audiosdl_audioout_obj_t *self);
