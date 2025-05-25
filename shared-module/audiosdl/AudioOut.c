#include "py/runtime.h"
#include "shared-bindings/audiosdl/AudioOut.h"
#include "shared-module/audiosdl/AudioOut.h"
#include "shared-module/audiocore/__init__.h"
#include "SDL.h"

static void audioout_buf_callback_fun(void *self_in) {
    audiosdl_audioout_obj_t *self = self_in;

    if (!self->stream || !self->dev || !self->sample) {
        return;
    }

    while (SDL_AudioStreamAvailable(self->stream) < 4 * self->blocksize_bytes) {
        uint8_t *raw_sample_buf;
        uint32_t raw_sample_buf_size_bytes;
        audioio_get_buffer_result_t get_buffer_result = audiosample_get_buffer(self->sample,
            false, 0, &raw_sample_buf, &raw_sample_buf_size_bytes);
        if (get_buffer_result == GET_BUFFER_ERROR) {
            common_hal_audiosdl_audioout_stop(self);
            return;
        }
        SDL_AudioStreamPut(self->stream, raw_sample_buf, raw_sample_buf_size_bytes);
        if (get_buffer_result == GET_BUFFER_DONE) {
            if (self->loop) {
                audiosample_reset_buffer(self->sample, true, 0);
            } else {
                common_hal_audiosdl_audioout_stop(self);
                return;
            }
        }
    }
}

static void audio_callback(void *self_in, Uint8 *stream_in, int len_bytes) {
    audiosdl_audioout_obj_t *self = self_in;

    if (!self->dev || !self->stream) {
        return;
    }

    int avail_bytes = SDL_AudioStreamAvailable(self->stream);
    int n_copy_bytes = MIN(avail_bytes, len_bytes);

    SDL_AudioStreamGet(self->stream, stream_in, n_copy_bytes);
    if (n_copy_bytes < len_bytes) {
        // set any remaining samples to zero, it causes a playback gap
        memset(stream_in + n_copy_bytes, 0, len_bytes - n_copy_bytes);
    }

    // now we need more samples!
    background_callback_add(&self->cb, audioout_buf_callback_fun, self);
}

static void sdl_init_maybe(void) {
    static bool once;
    if (!once) {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
        once = true;
    }
}

void common_hal_audiosdl_audioout_construct(audiosdl_audioout_obj_t *self,
    mp_int_t sample_rate) {

    sdl_init_maybe();

    SDL_AudioSpec desired, obtained;

    SDL_zero(desired);
    desired.freq = sample_rate;
    desired.channels = 1;
    desired.samples = 512;
    desired.userdata = self;
    desired.callback = audio_callback;
    desired.format = AUDIO_S16;

    self->dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    if (!self->dev) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%s failed: %s\n"),
            "SDL_OpenAudioDevice", SDL_GetError());
    }

    self->blocksize_bytes = 2 * sizeof(uint16_t) * obtained.samples;

    self->stream = SDL_NewAudioStream(
        // source format of the stream
        obtained.format, 2, obtained.freq,
        // destination format of the stream
        obtained.format, 2, obtained.freq);

    if (!self->stream) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("failed: %s\n"),
            "SDL_NewAudioStream",
            SDL_GetError());
    }

    self->sample = mp_const_none;
}

void common_hal_audiosdl_audioout_deinit(audiosdl_audioout_obj_t *self) {
    if (self->dev) {
        SDL_AudioDeviceID dev = self->dev;
        SDL_PauseAudioDevice(self->dev, 0);
        self->dev = 0;
        SDL_CloseAudioDevice(dev);
    }
    if (self->stream) {
        SDL_AudioStream *stream = self->stream;
        self->stream = NULL;
        SDL_FreeAudioStream(stream);
    }
    self->sample = mp_const_none;
}

bool common_hal_audiosdl_audioout_deinited(audiosdl_audioout_obj_t *self) {
    return !self->dev;
}

void common_hal_audiosdl_audioout_play(audiosdl_audioout_obj_t *self, mp_obj_t sample, bool loop) {
    self->sample = sample;
    self->loop = loop;
    SDL_PauseAudioDevice(self->dev, 0);
    // now we need more samples!
    background_callback_add(&self->cb, audioout_buf_callback_fun, self);
}

void common_hal_audiosdl_audioout_stop(audiosdl_audioout_obj_t *self) {
    self->sample = mp_const_none;
}

bool common_hal_audiosdl_audioout_get_playing(audiosdl_audioout_obj_t *self) {
    return self->sample != mp_const_none;
}

void common_hal_audiosdl_audioout_pause(audiosdl_audioout_obj_t *self) {
    SDL_PauseAudioDevice(self->dev, 1);
    self->paused = true;
}

void common_hal_audiosdl_audioout_resume(audiosdl_audioout_obj_t *self) {
    if (self->sample) {
        SDL_PauseAudioDevice(self->dev, 0);
    }
    self->paused = false;
    // now we need more samples!
    background_callback_add(&self->cb, audioout_buf_callback_fun, self);
}

bool common_hal_audiosdl_audioout_get_paused(audiosdl_audioout_obj_t *self) {
    return self->paused;
}
