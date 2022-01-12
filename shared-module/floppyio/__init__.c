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

#include "hal_gpio.h"

#include "py/runtime.h"

#include "shared-bindings/time/__init__.h"
#include "shared-bindings/floppyio/__init__.h"
#include "common-hal/floppyio/__init__.h"
#include "shared-bindings/digitalio/DigitalInOut.h"

#pragma GCC optimize("-O3")

#define T0_5 (FLOPPYIO_SAMPLERATE * 1 / 2 / 1000000)
#define T0_25 (FLOPPYIO_SAMPLERATE * 1 / 4 / 1000000)
#define T1   (FLOPPYIO_SAMPLERATE * 2 / 2 / 1000000)
#define T1_5 (FLOPPYIO_SAMPLERATE * 3 / 2 / 1000000)
#define T2   (FLOPPYIO_SAMPLERATE * 4 / 2 / 1000000)
#define T2_5 (FLOPPYIO_SAMPLERATE * 5 / 2 / 1000000)
#define T3   (FLOPPYIO_SAMPLERATE * 6 / 2 / 1000000)
#define T3_5 (FLOPPYIO_SAMPLERATE * 7 / 2 / 1000000)

// LED is PA23 on Feather M4
#if 0
#define DEBUG(x) do { if (x) PORT->Group[0].OUTSET.reg = (1 << 23); else PORT->Group[0].OUTCLR.reg = (1 << 23); } while (0)
#define DEBUGTOG() do { PORT->Group[0].OUTTGL.reg = (1 << 23); } while (0)
#else
#define DEBUG(x) ((void)0)
#define DEBUGTOG() ((void)0)
#endif

struct mfm_io {
    volatile uint32_t *index_port;
    uint32_t index_mask;
    volatile uint32_t *data_port;
    uint32_t data_mask;
    unsigned index_state;
    unsigned index_count;
};

#define READ_DATA() (!!(*io->data_port & io->data_mask))
#define READ_INDEX() (!!(*io->index_port & io->index_mask))

#include "lib/libmfm/mfm_impl.h"

__attribute__((always_inline))
static inline mfm_io_symbol_t mfm_io_read_symbol(mfm_io_t *io) {
    unsigned pulse_count = 3;
    while (!READ_DATA()) {
        pulse_count++;
    }

    unsigned index_state = (io->index_state << 1) | READ_INDEX();
    if ((index_state & 3) == 2) { // a zero-to-one transition
        io->index_count++;
    }
    io->index_state = index_state;

    while (READ_DATA()) {
        pulse_count++;
    }

    mfm_io_symbol_t result = pulse_10;
    if (pulse_count > T2_5) {
        result++;
    }
    if (pulse_count > T3_5) {
        result++;
    }

    return result;
}

static void mfm_io_reset_sync_count(mfm_io_t *io) {
    io->index_count = 0;
}

__attribute__((optimize("O3"), always_inline))
inline static int mfm_io_get_sync_count(mfm_io_t *io) {
    return io->index_count;
}

#include <string.h>

__attribute__((optimize("O3")))
int common_hal_floppyio_flux_readinto(void *buf, size_t len, digitalio_digitalinout_obj_t *data, digitalio_digitalinout_obj_t *index) {
    uint32_t index_mask;
    volatile uint32_t *index_port = common_hal_digitalio_digitalinout_get_reg(index, DIGITALINOUT_REG_READ, &index_mask);

    uint32_t data_mask;
    volatile uint32_t *data_port = common_hal_digitalio_digitalinout_get_reg(data, DIGITALINOUT_REG_READ, &data_mask);

#undef READ_INDEX
#undef READ_DATA
#define READ_INDEX() (!!(*index_port & index_mask))
#define READ_DATA() (!!(*data_port & data_mask))

    memset(buf, 0, len);

    uint8_t *pulses = buf, *pulses_ptr = pulses, *pulses_end = pulses + len;

    common_hal_mcu_disable_interrupts();

    // wait for index pulse low
    while (READ_INDEX()) { /* NOTHING */
    }


    // if data line is low, wait till it rises
    while (!READ_DATA()) { /* NOTHING */
    }

    // and then till it drops down
    while (READ_DATA()) { /* NOTHING */
    }

    uint32_t index_state = 0;
    while (pulses_ptr < pulses_end) {
        index_state = (index_state << 1) | READ_INDEX();
        if ((index_state & 3) == 2) { // a zero-to-one transition
            break;
        }

        unsigned pulse_count = 3;
        while (!READ_DATA()) {
            pulse_count++;
        }

        while (pulse_count < 255) {
            if (!READ_DATA()) {
                break;
            }
            pulse_count++;
        }

        *pulses_ptr++ = MIN(255, pulse_count);
    }
    common_hal_mcu_enable_interrupts();

    return pulses_ptr - pulses;
}

int common_hal_floppyio_mfm_readinto(void *buf, size_t n_sectors, digitalio_digitalinout_obj_t *data, digitalio_digitalinout_obj_t *index) {
    mfm_io_t io;
    io.index_port = common_hal_digitalio_digitalinout_get_reg(index, DIGITALINOUT_REG_READ, &io.index_mask);
    io.data_port = common_hal_digitalio_digitalinout_get_reg(data, DIGITALINOUT_REG_READ, &io.data_mask);

    common_hal_time_delay_ms(100);

    #if 0
    gpio_set_pin_pull_mode(23, GPIO_PULL_OFF);
    gpio_set_pin_direction(23, GPIO_DIRECTION_OUT);
    #endif
    common_hal_mcu_disable_interrupts();
    uint8_t validity[n_sectors];
    int result = read_track(io, n_sectors, buf, validity);
    common_hal_mcu_enable_interrupts();

    return result;
}
