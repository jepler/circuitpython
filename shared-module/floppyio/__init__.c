/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Jeff Epler for Adafruit Industries
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

#include "shared-bindings/floppyio/__init__.h"
#include "common-hal/floppyio/__init__.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

#include <string.h>

__attribute__((optimize("O3")))
int common_hal_floppyio_flux_readinto(void *buf, size_t len, digitalio_digitalinout_obj_t *data, digitalio_digitalinout_obj_t *index) {
    uint32_t index_mask;
    volatile uint32_t *index_port = common_hal_digitalio_digitalinout_get_reg(data, DIGITALINOUT_REG_READ, &index_mask);

    uint32_t data_mask;
    volatile uint32_t *data_port = common_hal_digitalio_digitalinout_get_reg(data, DIGITALINOUT_REG_READ, &data_mask);

#define READ_INDEX() ((*index_port & index_mask))
#define READ_DATA() ((*data_port & data_mask))

    memset(buf, 0, len);

    uint8_t *pulses = buf, *pulses_ptr = pulses, *pulses_end = pulses + len;

    common_hal_mcu_disable_interrupts();

    // wait for index pulse low
    while (READ_INDEX()) { /* NOTHING */
    }

    bool last_index_state = READ_INDEX();
    unsigned index_transitions = 0;

    // if data line is low, wait till it rises
    while (!READ_DATA()) { /* NOTHING */
    }

    // and then till it drops down
    while (READ_DATA()) { /* NOTHING */
    }

    while (pulses_ptr < pulses_end) {
        bool index_state = READ_INDEX();
        if (!last_index_state && index_state) {
            index_transitions++;
            if (index_transitions == 2) {
                break;
            }
        }
        last_index_state = index_state;

        unsigned pulse_count = 3;
        while (!READ_DATA()) {
            pulse_count++;
        }

        while (READ_DATA()) {
            pulse_count++;
        }

        *pulses_ptr++ = MIN(255, pulse_count);
    }
    common_hal_mcu_enable_interrupts();

    return pulses_ptr - pulses;
}

__attribute__((optimize("O3")))
int common_hal_floppyio_rawbit_readinto(void *buf, size_t len, digitalio_digitalinout_obj_t *data_, digitalio_digitalinout_obj_t *index) {
    uint32_t index_mask;
    volatile uint32_t *index_port = common_hal_digitalio_digitalinout_get_reg(index, DIGITALINOUT_REG_READ, &index_mask);

    uint32_t data_mask;
    volatile uint32_t *data_port = common_hal_digitalio_digitalinout_get_reg(data_, DIGITALINOUT_REG_READ, &data_mask);

#define READ_INDEX() ((*index_port & index_mask))
#define READ_DATA() ((*data_port & data_mask))

    memset(buf, 0, len);

#define T2_5 (FLOPPYIO_SAMPLERATE * 5 / 2 / 1000000)
#define T3_5 (FLOPPYIO_SAMPLERATE * 7 / 2 / 1000000)
    uint32_t *data_start = buf, *data_ptr = data_start, *data_end = data_start + len / sizeof(*data_ptr);

    common_hal_mcu_disable_interrupts();

    // wait for index pulse low
    while (READ_INDEX()) { /* NOTHING */
    }

    bool last_index_state = READ_INDEX();
    unsigned index_transitions = 0;

    // if data line is low, wait till it rises
    while (!READ_DATA()) { /* NOTHING */
    }

    // and then till it drops down
    while (READ_DATA()) { /* NOTHING */
    }

    uint32_t data = 0, bitpos = 0;

#define PUT_BIT(x) do { \
        data = (data << 1) | x; \
        if (++bitpos % 32 == 0) { \
            *data_ptr++ = data; \
            if (data_ptr == data_end) { \
                break; \
            } \
        } \
} while (0)

    while (data_ptr < data_end) {
        bool index_state = READ_INDEX();
        if (!last_index_state && index_state) {
            index_transitions++;
            if (index_transitions == 2) {
                break;
            }
        }
        last_index_state = index_state;

        PUT_BIT(1);
        PUT_BIT(0);

        while (!READ_DATA()) {
        }

        int pulse_count = 0;
        while (READ_DATA() && pulse_count < T2_5) {
            pulse_count++;
        }
        if (pulse_count < T2_5) {
            continue;
        }
        PUT_BIT(0);

        while (READ_DATA() && pulse_count < T3_5) {
            pulse_count++;
        }
        if (pulse_count < T3_5) {
            continue;
        }
        PUT_BIT(0);
    }
    common_hal_mcu_enable_interrupts();

    return (data_ptr - data_start) * sizeof(*data_ptr);
}
