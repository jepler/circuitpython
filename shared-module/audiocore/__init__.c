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

#include <string.h>

#include "shared-module/audioio/__init__.h"

#include "py/obj.h"
#include "shared-bindings/audiocore/RawSample.h"
#include "shared-bindings/audiocore/WaveFile.h"
#include "shared-module/audiocore/RawSample.h"
#include "shared-module/audiocore/WaveFile.h"

#include "shared-bindings/audiomixer/Mixer.h"
#include "shared-module/audiomixer/Mixer.h"

uint32_t audiosample_sample_rate(mp_obj_t sample_obj) {
    const audiosample_p_t *proto = mp_proto_get_or_throw(MP_QSTR_protocol_audiosample, sample_obj);
    return proto->sample_rate(MP_OBJ_TO_PTR(sample_obj));
}

uint8_t audiosample_bits_per_sample(mp_obj_t sample_obj) {
    const audiosample_p_t *proto = mp_proto_get_or_throw(MP_QSTR_protocol_audiosample, sample_obj);
    return proto->bits_per_sample(MP_OBJ_TO_PTR(sample_obj));
}

uint8_t audiosample_channel_count(mp_obj_t sample_obj) {
    const audiosample_p_t *proto = mp_proto_get_or_throw(MP_QSTR_protocol_audiosample, sample_obj);
    return proto->channel_count(MP_OBJ_TO_PTR(sample_obj));
}

void audiosample_reset_buffer(mp_obj_t sample_obj, bool single_channel, uint8_t audio_channel) {
    const audiosample_p_t *proto = mp_proto_get_or_throw(MP_QSTR_protocol_audiosample, sample_obj);
    proto->reset_buffer(MP_OBJ_TO_PTR(sample_obj), single_channel, audio_channel);
}

audioio_get_buffer_result_t audiosample_get_buffer(mp_obj_t sample_obj,
    bool single_channel,
    uint8_t channel,
    uint8_t **buffer, uint32_t *buffer_length) {
    const audiosample_p_t *proto = mp_proto_get_or_throw(MP_QSTR_protocol_audiosample, sample_obj);
    return proto->get_buffer(MP_OBJ_TO_PTR(sample_obj), single_channel, channel, buffer, buffer_length);
}

void audiosample_get_buffer_structure(mp_obj_t sample_obj, bool single_channel,
    bool *single_buffer, bool *samples_signed,
    uint32_t *max_buffer_length, uint8_t *spacing) {
    const audiosample_p_t *proto = mp_proto_get_or_throw(MP_QSTR_protocol_audiosample, sample_obj);
    proto->get_buffer_structure(MP_OBJ_TO_PTR(sample_obj), single_channel, single_buffer,
        samples_signed, max_buffer_length, spacing);
}

void audiosample_convert_u8m_s16s(int16_t *buffer_out, const uint8_t *buffer_in, size_t nframes) {
    for (; nframes--;) {
        int16_t sample = (*buffer_in++ - 0x80) << 8;
        *buffer_out++ = sample;
        *buffer_out++ = sample;
    }
}


void audiosample_convert_u8s_s16s(int16_t *buffer_out, const uint8_t *buffer_in, size_t nframes) {
    size_t nsamples = 2 * nframes;
    for (; nsamples--;) {
        int16_t sample = (*buffer_in++ - 0x80) << 8;
        *buffer_out++ = sample;
    }
}

void audiosample_convert_s8m_s16s(int16_t *buffer_out, const int8_t *buffer_in, size_t nframes) {
    for (; nframes--;) {
        int16_t sample = (*buffer_in++) << 8;
        *buffer_out++ = sample;
        *buffer_out++ = sample;
    }
}


void audiosample_convert_s8s_s16s(int16_t *buffer_out, const int8_t *buffer_in, size_t nframes) {
    size_t nsamples = 2 * nframes;
    for (; nsamples--;) {
        int16_t sample = (*buffer_in++) << 8;
        *buffer_out++ = sample;
    }
}


void audiosample_convert_u16m_s16s(int16_t *buffer_out, const uint16_t *buffer_in, size_t nframes) {
    for (; nframes--;) {
        int16_t sample = *buffer_in++ - 0x8000;
        *buffer_out++ = sample;
        *buffer_out++ = sample;
    }
}


void audiosample_convert_u16s_s16s(int16_t *buffer_out, const uint16_t *buffer_in, size_t nframes) {
    size_t nsamples = 2 * nframes;
    for (; nsamples--;) {
        int16_t sample = *buffer_in++ - 0x8000;
        *buffer_out++ = sample;
    }
}

void audiosample_convert_s16m_s16s(int16_t *buffer_out, const int16_t *buffer_in, size_t nframes) {
    for (; nframes--;) {
        int16_t sample = *buffer_in++;
        *buffer_out++ = sample;
        *buffer_out++ = sample;
    }
}

static audioio_get_buffer_result_t _get_buffer(audiocore_rebuffer_t *self) {
    if (!self->sample_obj) {
        self->sample_data = self->sample_end = NULL;
        return GET_BUFFER_ERROR;
    }

    uint32_t sample_buffer_length;
    audioio_get_buffer_result_t result = audiosample_get_buffer(self->sample_obj, false, 0, &self->sample_data, &sample_buffer_length);

    self->sample_end = self->sample_data + sample_buffer_length;

    if (result == GET_BUFFER_ERROR) {
        self->sample_obj = NULL;
        self->sample_data = self->sample_end = NULL;
        return GET_BUFFER_DONE;
    }
    if (result == GET_BUFFER_DONE) {
        if (self->loop) {
            audiosample_reset_buffer(self->sample_obj, false, 0);
            result = GET_BUFFER_MORE_DATA;
        } else {
            self->sample_obj = NULL;
        }
    }

    return result;
}

void audiorebuffer_set_sample(audiocore_rebuffer_t *self, mp_obj_t *sample, bool loop) {
    self->loop = loop;
    self->sample_obj = sample;
    self->sample_data = self->sample_end = NULL;

    if (!sample) {
        return;
    }

    int bytes_per_sample = audiosample_bits_per_sample(sample) / 8;
    int channel_count = audiosample_channel_count(sample);

    bool single_buffer;
    bool samples_signed;
    uint32_t max_buffer_length;
    uint8_t spacing;

    audiosample_get_buffer_structure(sample, false, &single_buffer, &samples_signed,
        &max_buffer_length, &spacing);

    self->sample_type = (samples_signed ? AUDIOSAMPLE_SIGNED : 0)
        | (bytes_per_sample == 2 ? AUDIOSAMPLE_16BIT : 0)
        | (channel_count == 2 ? AUDIOSAMPLE_STEREO : 0);

    self->bytes_per_frame = bytes_per_sample * channel_count;
}

void audiosample_convert_to_s16(int16_t *buffer_out, const void *buffer_in, size_t nframes, audiosample_sample_type_t sample_type) {
    switch (sample_type) {
        case AUDIOSAMPLE_U8M:
            audiosample_convert_u8m_s16s(buffer_out, buffer_in, nframes);
            break;
        case AUDIOSAMPLE_U8S:
            audiosample_convert_u8s_s16s(buffer_out, buffer_in, nframes);
            break;
        case AUDIOSAMPLE_S8M:
            audiosample_convert_s8m_s16s(buffer_out, buffer_in, nframes);
            break;
        case AUDIOSAMPLE_S8S:
            audiosample_convert_s8s_s16s(buffer_out, buffer_in, nframes);
            break;
        case AUDIOSAMPLE_U16M:
            audiosample_convert_u16m_s16s(buffer_out, buffer_in, nframes);
            break;
        case AUDIOSAMPLE_U16S:
            audiosample_convert_u16s_s16s(buffer_out, buffer_in, nframes);
            break;
        case AUDIOSAMPLE_S16M:
            audiosample_convert_s16m_s16s(buffer_out, buffer_in, nframes);
            break;
        case AUDIOSAMPLE_S16S:
            memcpy(buffer_out, buffer_in, nframes * 4);
            break;
    }
}


audioio_get_buffer_result_t audiorebuffer_fill_s16s(audiocore_rebuffer_t *self, int16_t *sample_buffer, size_t nframes) {
    audioio_get_buffer_result_t status = GET_BUFFER_MORE_DATA;
    int16_t *end = sample_buffer + 2 * nframes;

    // even if self->sample_obj is NULL _get_buffer returns GET_BUFFER_ERROR,
    // it's OK to enter this loop; sample_data/sample_end are guaranteed to get
    // set to the same (possibly NULL) pointer in this case.
    do {
        if (self->sample_data == self->sample_end) {
            status = _get_buffer(self);
        }

        int nframes_in_output = (end - sample_buffer) / 2;
        int nframes_in_sample = (self->sample_end - self->sample_data) / self->bytes_per_frame;
        int nframes_copy = MIN(nframes_in_output, nframes_in_sample);
        audiosample_convert_to_s16(sample_buffer, self->sample_data, nframes_copy, self->sample_type);
        sample_buffer += nframes_copy * 2;
        self->sample_data += nframes_copy * self->bytes_per_frame;
    } while (sample_buffer < end && status == GET_BUFFER_MORE_DATA);

    memset(sample_buffer, (end - sample_buffer) * sizeof(int16_t), 0);

    return status;
}
