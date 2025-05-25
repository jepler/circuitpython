#pragma once

#include "py/ringbuf.h"
#include "supervisor/background_callback.h"
#include "SDL_audio.h"

typedef struct audiosdl_audioout_obj {
    mp_obj_base_t base;
    SDL_AudioDeviceID dev;
    SDL_AudioStream *stream;
    int blocksize_bytes;
    mp_obj_t sample;
    bool paused, loop;
    background_callback_t cb;
} audiosdl_audioout_obj_t;
