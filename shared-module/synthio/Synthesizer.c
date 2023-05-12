/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Artyom Skrobov
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

#include "py/runtime.h"
#include "shared-bindings/synthio/Synthesizer.h"
#include "shared-bindings/synthio/Note.h"
#include "shared-module/synthio/Note.h"



void common_hal_synthio_synthesizer_construct(synthio_synthesizer_obj_t *self,
    uint32_t sample_rate, int channel_count, mp_obj_t waveform_obj, mp_obj_t filter_obj,
    mp_obj_t envelope_obj) {

    synthio_synth_init(&self->synth, sample_rate, channel_count, waveform_obj, filter_obj, envelope_obj);
}

void common_hal_synthio_synthesizer_deinit(synthio_synthesizer_obj_t *self) {
    synthio_synth_deinit(&self->synth);
}
bool common_hal_synthio_synthesizer_deinited(synthio_synthesizer_obj_t *self) {
    return synthio_synth_deinited(&self->synth);
}

uint32_t common_hal_synthio_synthesizer_get_sample_rate(synthio_synthesizer_obj_t *self) {
    return self->synth.sample_rate;
}
uint8_t common_hal_synthio_synthesizer_get_bits_per_sample(synthio_synthesizer_obj_t *self) {
    return SYNTHIO_BITS_PER_SAMPLE;
}
uint8_t common_hal_synthio_synthesizer_get_channel_count(synthio_synthesizer_obj_t *self) {
    return self->synth.channel_count;
}

void synthio_synthesizer_reset_buffer(synthio_synthesizer_obj_t *self,
    bool single_channel_output, uint8_t channel) {
    synthio_synth_reset_buffer(&self->synth, single_channel_output, channel);
}

audioio_get_buffer_result_t synthio_synthesizer_get_buffer(synthio_synthesizer_obj_t *self,
    bool single_channel_output, uint8_t channel, uint8_t **buffer, uint32_t *buffer_length) {
    if (common_hal_synthio_synthesizer_deinited(self)) {
        *buffer_length = 0;
        return GET_BUFFER_ERROR;
    }
    self->synth.span.dur = SYNTHIO_MAX_DUR;
    synthio_synth_synthesize(&self->synth, buffer, buffer_length, single_channel_output ? channel : 0);
    return GET_BUFFER_MORE_DATA;
}

void synthio_synthesizer_get_buffer_structure(synthio_synthesizer_obj_t *self, bool single_channel_output,
    bool *single_buffer, bool *samples_signed, uint32_t *max_buffer_length, uint8_t *spacing) {
    return synthio_synth_get_buffer_structure(&self->synth, single_channel_output, single_buffer, samples_signed, max_buffer_length, spacing);
}

void common_hal_synthio_synthesizer_release_all(synthio_synthesizer_obj_t *self) {
    for (size_t i = 0; i < CIRCUITPY_SYNTHIO_MAX_CHANNELS; i++) {
        if (self->synth.span.note_obj[i] != SYNTHIO_SILENCE) {
            synthio_span_change_note(&self->synth, self->synth.span.note_obj[i], SYNTHIO_SILENCE);
        }
    }
}

STATIC mp_obj_t validate_note(mp_obj_t note_in) {
    if (mp_obj_is_small_int(note_in)) {
        mp_arg_validate_int_range(mp_obj_get_int(note_in), 0, 127, MP_QSTR_note);
    } else {
        const mp_obj_type_t *note_type = mp_obj_get_type(note_in);
        if (note_type != &synthio_note_type) {
            mp_raise_TypeError_varg(translate("%q must be of type %q or %q, not %q"), MP_QSTR_note, MP_QSTR_int, MP_QSTR_Note, note_type->name);
        }
    }
    return note_in;
}

void common_hal_synthio_synthesizer_release(synthio_synthesizer_obj_t *self, mp_obj_t to_release) {
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t iterable = mp_getiter(to_release, &iter_buf);
    mp_obj_t item;
    while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
        synthio_span_change_note(&self->synth, validate_note(item), SYNTHIO_SILENCE);
    }
}

void common_hal_synthio_synthesizer_press(synthio_synthesizer_obj_t *self, mp_obj_t to_press) {
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t iterable = mp_getiter(to_press, &iter_buf);
    mp_obj_t note_obj;
    while ((note_obj = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
        note_obj = validate_note(note_obj);
        if (!mp_obj_is_small_int(note_obj)) {
            synthio_note_obj_t *note = MP_OBJ_TO_PTR(note_obj);
            synthio_note_start(note, self->synth.sample_rate);
        }
        synthio_span_change_note(&self->synth, SYNTHIO_SILENCE, note_obj);
    }
}

mp_obj_t common_hal_synthio_synthesizer_get_pressed_notes(synthio_synthesizer_obj_t *self) {
    int count = 0;
    for (int chan = 0; chan < CIRCUITPY_SYNTHIO_MAX_CHANNELS; chan++) {
        if (self->synth.span.note_obj[chan] != SYNTHIO_SILENCE && SYNTHIO_NOTE_IS_PLAYING(&self->synth, chan)) {
            count += 1;
        }
    }
    mp_obj_tuple_t *result = MP_OBJ_TO_PTR(mp_obj_new_tuple(count, NULL));
    for (size_t chan = 0, j = 0; chan < CIRCUITPY_SYNTHIO_MAX_CHANNELS; chan++) {
        if (self->synth.span.note_obj[chan] != SYNTHIO_SILENCE && SYNTHIO_NOTE_IS_PLAYING(&self->synth, chan)) {
            result->items[j++] = self->synth.span.note_obj[chan];
        }
    }
    return MP_OBJ_FROM_PTR(result);
}
