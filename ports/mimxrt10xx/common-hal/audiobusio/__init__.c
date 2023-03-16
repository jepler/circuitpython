/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jeff Epler for Adafruit Industries
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

#include "py/runtime.h"

#include "common-hal/audiobusio/__init__.h"
#include "shared-module/audiocore/__init__.h"

#define AUDIO_BUFFER_FRAME_COUNT (128) // in uint32_t; there are 4, giving 2048 bytes. In all they hold 10ms @ stereo 16-bit 48kHz before all buffers drain

static I2S_Type *const i2s_instances[] = I2S_BASE_PTRS;
static uint8_t i2s_in_use;

static I2S_Type *SAI_GetPeripheral(int idx) {
    if (idx < 0 || idx >= (int)MP_ARRAY_SIZE(i2s_instances)) {
        return NULL;
    }
    return i2s_instances[idx];
}

#if 0
static int SAI_GetInstance(I2S_Type *base) {
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < MP_ARRAY_SIZE(i2s_instances); instance++)
    {
        if (i2s_instances[instance] == base) {
            return instance;
        }
    }

    return -1;
}
#endif

static bool i2s_queue_available(i2s_t *self) {
    return !self->handle.saiQueue[self->handle.queueUser].data;
}

static void i2s_fill_buffer(i2s_t *self) {
    if (!self->peripheral) {
        return;
    }
    while (i2s_queue_available(self)) {
        uint32_t *buffer = self->buffers[self->buffer_idx];
        uint32_t *ptr = buffer, *end = buffer + AUDIO_BUFFER_FRAME_COUNT;
        self->buffer_idx = (self->buffer_idx + 1) % SAI_XFER_QUEUE_SIZE;

        while (self->playing && !self->paused && ptr < end) {
            if (self->sample_data == self->sample_end) {
                if (self->stopping) {
                    // non-looping sample, previously returned GET_BUFFER_DONE
                    self->playing = false;
                    break;
                }
                uint32_t sample_buffer_length;
                audioio_get_buffer_result_t get_buffer_result =
                    audiosample_get_buffer(self->sample, false, 0,
                        &self->sample_data, &sample_buffer_length);
                self->sample_end = self->sample_data + sample_buffer_length;
                if (get_buffer_result == GET_BUFFER_DONE) {
                    if (self->loop) {
                        audiosample_reset_buffer(self->sample, false, 0);
                    } else {
                        self->stopping = true; // TODO does this cut off the end of the audio?
                        break;
                    }
                }
                if (get_buffer_result == GET_BUFFER_ERROR || sample_buffer_length == 0) {
                    self->stopping = true;
                    break;
                }
            }
            size_t input_bytecount = self->sample_end - self->sample_data;
            size_t bytes_per_input_frame = self->channel_count * self->bytes_per_sample;
            size_t framecount = MIN((size_t)(end - ptr), input_bytecount / bytes_per_input_frame);

#define SAMPLE_TYPE(is_signed, channel_count, bytes_per_sample) ((is_signed) | ((channel_count) << 1) | ((bytes_per_sample) << 3))

            switch (SAMPLE_TYPE(self->samples_signed, self->channel_count, self->bytes_per_sample)) {

                case SAMPLE_TYPE(true, 2, 2):
                    memcpy(ptr, self->sample_data, 4 * framecount);
                    break;

                case SAMPLE_TYPE(false, 2, 2):
                    audiosample_convert_u16s_s16s((int16_t *)ptr, (uint16_t *)(void *)self->sample_data, framecount);
                    break;

                case SAMPLE_TYPE(true, 1, 2):
                    audiosample_convert_s16m_s16s((int16_t *)ptr, (int16_t *)(void *)self->sample_data, framecount);
                    break;

                case SAMPLE_TYPE(false, 1, 2):
                    audiosample_convert_u16m_s16s((int16_t *)ptr, (uint16_t *)(void *)self->sample_data, framecount);
                    break;

                case SAMPLE_TYPE(true, 2, 1):
                    audiosample_convert_s8s_s16s((int16_t *)ptr, (int8_t *)(void *)self->sample_data, framecount);
                    memcpy(ptr, self->sample_data, 4 * framecount);
                    break;

                case SAMPLE_TYPE(false, 2, 1):
                    audiosample_convert_u8s_s16s((int16_t *)ptr, (uint8_t *)(void *)self->sample_data, framecount);
                    break;

                case SAMPLE_TYPE(true, 1, 1):
                    audiosample_convert_s8m_s16s((int16_t *)ptr, (int8_t *)(void *)self->sample_data, framecount);
                    break;

                case SAMPLE_TYPE(false, 1, 1):
                    audiosample_convert_u8m_s16s((int16_t *)ptr, (uint8_t *)(void *)self->sample_data, framecount);
                    break;
            }
            self->sample_data += bytes_per_input_frame * framecount; // in bytes
            ptr += framecount; // in frames
        }
        mp_printf(&mp_plat_print, "filling i2s queue including %d silence\n", (int)(end - ptr));
        // Fill any remaining portion of the buffer with 'no sound'
        memset(ptr, 0, (end - ptr) * sizeof(uint32_t));
        sai_transfer_t xfer = {
            .data = (uint8_t *)buffer,
            .dataSize = AUDIO_BUFFER_FRAME_COUNT * sizeof(uint32_t),
        };
        SAI_TransferSendNonBlocking(self->peripheral, &self->handle, &xfer);
    }
}

static void i2s_callback_fun(void *self_in) {
    i2s_t *self = self_in;
    i2s_fill_buffer(self);
}

static void i2s_transfer_callback(I2S_Type *base, sai_handle_t *handle, status_t status, void *self_in) {
    mp_printf(&mp_plat_print, "i2s_transfer_callback with %d\n", (int)status);
    i2s_t *self = self_in;
    if (status == kStatus_SAI_TxIdle) {
        // a block has been finished
        background_callback_add(&self->callback, i2s_callback_fun, self_in);
    }
}


void port_i2s_initialize(i2s_t *self, int instance, sai_config_t *config) {
    I2S_Type *peripheral = SAI_GetPeripheral(instance);
    if (!peripheral) {
        mp_raise_ValueError_varg(translate("Invalid %q"), MP_QSTR_I2SOut);
    }
    if (i2s_in_use & (1 << instance)) {
        mp_raise_ValueError_varg(translate("%q in use"), MP_QSTR_I2SOut);
    }
    for (size_t i = 0; i < MP_ARRAY_SIZE(self->buffers); i++) {
        self->buffers[i] = m_malloc(AUDIO_BUFFER_FRAME_COUNT * sizeof(uint32_t), false);
    }
    self->peripheral = peripheral;
    SAI_TransferTxCreateHandle(peripheral, &self->handle, i2s_transfer_callback, (void *)self);
    i2s_in_use |= (1 << instance);
}

bool port_i2s_deinited(i2s_t *self) {
    return !self->peripheral;
}

void port_i2s_deinit(i2s_t *self) {
    if (port_i2s_deinited(self)) {
        return;
    }
    SAI_TransferAbortSend(self->peripheral, &self->handle);
    self->peripheral = NULL;
    for (size_t i = 0; i < MP_ARRAY_SIZE(self->buffers); i++) {
        self->buffers[i] = NULL;
    }
}

void port_i2s_play(i2s_t *self, mp_obj_t sample, bool loop) {
    self->sample = sample;
    self->loop = loop;
    self->bytes_per_sample = audiosample_bits_per_sample(sample) / 8;
    self->channel_count = audiosample_channel_count(sample);
    bool single_buffer;
    bool samples_signed;
    uint32_t max_buffer_length;
    uint8_t spacing;
    audiosample_get_buffer_structure(sample, false, &single_buffer, &samples_signed,
        &max_buffer_length, &spacing);
    self->samples_signed = samples_signed;
    self->playing = true;
    self->paused = false;
    self->stopping = false;
    self->sample_data = self->sample_end = NULL;

    audiosample_reset_buffer(self->sample, false, 0);

// TODO
    #if 0
    uint32_t sample_rate = audiosample_sample_rate(sample);
    if (sample_rate != self->i2s_config.sample_rate) {
        CHECK_ESP_RESULT(i2s_set_sample_rates(self->instance, audiosample_sample_rate(sample)));
        self->i2s_config.sample_rate = sample_rate;
    }
    #endif
    background_callback_add(&self->callback, i2s_callback_fun, self);
}

bool port_i2s_get_playing(i2s_t *self) {
    return self->playing;
}

bool port_i2s_get_paused(i2s_t *self) {
    return self->paused;
}

void port_i2s_stop(i2s_t *self) {
    self->sample = NULL;
    self->paused = false;
    self->playing = false;
    self->stopping = false;
}

void port_i2s_pause(i2s_t *self) {
    self->paused = true;
}

void port_i2s_resume(i2s_t *self) {
    self->paused = false;
}

void i2s_reset() {
// this port relies on object finalizers for reset
}
