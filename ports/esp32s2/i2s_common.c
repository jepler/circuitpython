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

#include "py/runtime.h"

#include "i2s_common.h"
#include "esp_error.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "shared-module/audiocore/__init__.h"

#define I2S_QUEUE_SIZE (3)

static i2s_t *i2s_instance[I2S_NUM_MAX];
static QueueHandle_t i2s_queues[I2S_NUM_MAX];
static TaskHandle_t i2s_tasks[I2S_NUM_MAX];

static int8_t port_i2s_allocate(bool analog) {
#if defined(I2S_NUM_1)
    if(!analog) {
        if(!i2s_instance[1]) return 1;
    }
#endif
    if(!i2s_instance[0]) return 0;

    mp_raise_RuntimeError(translate("Peripheral in use"));
}

void port_i2s_reset_instance(int i) {
    assert(i >= 0 && i < I2S_NUM_MAX);
    if (i2s_tasks[i]) {
        vTaskDelete(i2s_tasks[i]);
    }
    i2s_tasks[i] = NULL;

    (void)i2s_driver_uninstall(i);
    i2s_instance[i] = NULL;
}

void port_i2s_reset(void) {
    for (int i=0; i < I2S_NUM_MAX; i++) {
        port_i2s_reset_instance(i);
    }
}

static void i2s_fill_buffer(i2s_t *self) {
    if (self->paused || !self->sample) {
        i2s_zero_dma_buffer(self->instance);
        return;
    }
    while (!self->stopping) {
        if (self->sample_data == self->sample_end) {
            uint32_t sample_buffer_length;
            audioio_get_buffer_result_t get_buffer_result =
                audiosample_get_buffer(self->sample, false, 0,
                                       &self->sample_data, &sample_buffer_length);
            self->sample_end = self->sample_data + sample_buffer_length;
            if (get_buffer_result == GET_BUFFER_DONE) {
                if (self->loop) {
                    audiosample_reset_buffer(self->sample, false, 0);
                } else {
                    self->stopping = true;
                    break;
                }
            }
            if (get_buffer_result == GET_BUFFER_ERROR || sample_buffer_length == 0) {
                self->stopping = true;
                break;
            }
        }
        size_t bytes_written = 0;
        size_t bytecount = self->sample_end - self->sample_data;
printf("bytecount=%d %p..%p\n", (int)bytecount, self->sample_data, self->sample_end);
        for (size_t i=0; i < bytecount && i < 16; i++) {
            printf("%02x ", self->sample_data[i]);
        }
        printf("\n");
        if (self->samples_signed) {
            if (self->bytes_per_sample == 2) {
                ESP_CALL_RAISE_NAME("i2s_write", i2s_write(self->instance, self->sample_data, bytecount, &bytes_written, 0));
            } else {
                ESP_CALL_RAISE_NAME("i2s_write_expand", i2s_write_expand(self->instance, self->sample_data, bytecount, 8, 16, &bytes_written, 0));
            }
        } else {
#define STACK_BUFFER_SIZE (64)
            bytecount = MIN(STACK_BUFFER_SIZE, bytecount);
            if (self->bytes_per_sample == 2) {
                uint16_t *samples = (void*)self->sample_data;
                uint16_t signed_samples[STACK_BUFFER_SIZE / sizeof(uint16_t)];
                for (size_t i=0; i<bytecount/sizeof(int16_t); i++) {
                    signed_samples[i] = samples[i] - 0x8000;
                }
                ESP_CALL_RAISE_NAME("i2s_write", i2s_write(self->instance, signed_samples, bytecount, &bytes_written, 0));
            } else {
                uint8_t *samples = (void*)self->sample_data;
                uint8_t signed_samples[STACK_BUFFER_SIZE / sizeof(uint8_t)];
                for (size_t i=0; i<bytecount/sizeof(int8_t); i++) {
                    signed_samples[i] = samples[i] - 0x80;
                }
                ESP_CALL_RAISE_NAME("i2s_write_expand", i2s_write_expand(self->instance, signed_samples, bytecount, 8, 16, &bytes_written, 0));
            }
        }
        self->sample_data += bytes_written;
        printf("wrote %d\n", bytes_written);
        // We have filled the DMA buffer
        if (!bytes_written) {
            break;
        }
    }
}

static void i2s_callback_fun(void *self_in) {
    i2s_t *self = self_in;
    i2s_fill_buffer(self);
}

static void i2s_event_task(void *self_in) {
// printf("i2s_task_event @%p\n", self_in);

    i2s_t *self = self_in;
    while(true) {
        i2s_event_type_t event;
        BaseType_t result = xQueueReceive(i2s_queues[self->instance], &event, portMAX_DELAY);
        if (result && event == I2S_EVENT_TX_DONE) {
            background_callback_add(&self->callback, i2s_callback_fun, self_in);
        }
    }
}

void port_i2s_allocate_init(i2s_t *self, bool analog) {
    self->instance = port_i2s_allocate(analog);

    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 2,
        .dma_buf_len = 1024,
        .use_apll = false,
    };
    ESP_CALL_RAISE(i2s_driver_install(self->instance, &i2s_config, I2S_QUEUE_SIZE, &i2s_queues[self->instance]));

    if (!xTaskCreate(i2s_event_task, "I2S_task", 3 * configMINIMAL_STACK_SIZE, self, CONFIG_PTHREAD_TASK_PRIO_DEFAULT, &i2s_tasks[self->instance])) {
        mp_raise_OSError_msg(translate("xTaskCreate failed"));
    }
    i2s_instance[self->instance] = self;

}


void port_i2s_play(i2s_t *self, mp_obj_t sample, bool loop) {
    self->sample = sample;
    self->bytes_per_sample = audiosample_bits_per_sample(sample) / 8;
    bool single_buffer;
    bool samples_signed;
    uint32_t max_buffer_length;
    uint8_t spacing;
    audiosample_get_buffer_structure(sample, false, &single_buffer, &samples_signed,
                                     &max_buffer_length, &spacing);
    self->samples_signed = samples_signed;
    self->loop = loop;
    self->playing = true;
    self->paused = false;
    self->stopping = false;
    // We always output stereo so output twice as many bits.
    // uint16_t bits_per_sample_output = bits_per_sample * 2;
    ESP_CALL_RAISE(i2s_set_sample_rates(self->instance, audiosample_sample_rate(sample)));
    i2s_fill_buffer(self);
}

bool port_i2s_playing(i2s_t *self) {
    return self->playing && !self->stopping;
}

bool port_i2s_paused(i2s_t *self) {
    return self->paused;
}

void port_i2s_stop(i2s_t *self) {
    self->sample = NULL;
    self->paused = false;
    self->playing = false;
    self->stopping = false;
}

void port_i2s_pause(i2s_t *self) {
    if (!self->paused) {
        self->paused = true;
        ESP_CALL_RAISE(i2s_stop(self->instance));
    }
}

void port_i2s_resume(i2s_t *self) {
    if (self->paused) {
        self->paused = false;
        ESP_CALL_RAISE(i2s_start(self->instance));
    }
}
