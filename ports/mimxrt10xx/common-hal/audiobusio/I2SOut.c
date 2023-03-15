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

#include <stdint.h>
#include <string.h>

#include "mpconfigport.h"

// Some boards don't implement I2SOut, so suppress any routines from here.
#if CIRCUITPY_AUDIOBUSIO_I2SOUT

#define AUDIO_BUFFER_SIZE (512) // in bytes; there are 4, giving 2048 bytes = 10ms @ stereo 16-bit 48kHz before all buffers drain

#include "py/gc.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "common-hal/audiobusio/I2SOut.h"
#include "shared-bindings/audiobusio/I2SOut.h"
#include "shared-bindings/audiocore/RawSample.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/shared/translate/translate.h"
#include "supervisor/shared/tick.c"

// Where required we use identifier names that are required by NXP's
// API, even though they do not conform to the naming standards that Adafruit
// strives to adhere to. https://www.adafruit.com/blacklivesmatter
#include "drivers/fsl_sai.h"

static void release_buffers(audiobusio_i2sout_obj_t *self) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(self->buffers); i++) {
        gc_free(self->buffers[i]);
        self->buffers[i] = NULL;
    }
}

STATIC void config_periph_pin(const mcu_periph_obj_t *periph) {
    if (!periph) {
        return;
    }
    if (periph->pin->mux_reg) {
        IOMUXC_SetPinMux(
            periph->pin->mux_reg, periph->mux_mode,
            periph->input_reg, periph->input_idx,
            0,
            0);
    }

    IOMUXC_SetPinConfig(0, 0, 0, 0,
        periph->pin->cfg_reg,
        IOMUXC_SW_PAD_CTL_PAD_HYS(0)
        | IOMUXC_SW_PAD_CTL_PAD_PUS(0)
        | IOMUXC_SW_PAD_CTL_PAD_PUE(0)
        | IOMUXC_SW_PAD_CTL_PAD_PKE(1)
        | IOMUXC_SW_PAD_CTL_PAD_ODE(0)
        | IOMUXC_SW_PAD_CTL_PAD_SPEED(2)
        | IOMUXC_SW_PAD_CTL_PAD_DSE(4)
        | IOMUXC_SW_PAD_CTL_PAD_SRE(0));
}

// Caller validates that pins are free.
void common_hal_audiobusio_i2sout_construct(audiobusio_i2sout_obj_t *self,
    const mcu_pin_obj_t *bit_clock, const mcu_pin_obj_t *word_select,
    const mcu_pin_obj_t *data, bool left_justified) {

    int instance = -1;
    const mcu_periph_obj_t *bclk_periph = find_pin_function(mcu_sai_tx_bclk_list, bit_clock, &instance, MP_QSTR_bit_clock);
    const mcu_periph_obj_t *sync_periph = find_pin_function(mcu_sai_tx_sync_list, bit_clock, &instance, MP_QSTR_word_select);
    const mcu_periph_obj_t *data_periph = find_pin_function(mcu_sai_tx_data0_list, bit_clock, &instance, MP_QSTR_data);

// TODO determine SAI1 vs SAI3
// set up pinmuxes
// set up clocks
// set up etc

    for (size_t i = 0; i < MP_ARRAY_SIZE(self->buffers); i++) {
        self->buffers[i] = m_malloc(AUDIO_BUFFER_SIZE, false);
    }

    I2S_Type *peripheral = SAI_GetPeripheral(instance);
    sai_config_t config;
    SAI_TxGetDefaultConfig(&config);
    SAI_TxInit(peripheral, &config);

    self->peripheral = peripheral;
    self->bit_clock = bit_clock;
    self->word_select = word_select;
    self->data = data;
    claim_pin(bit_clock);
    claim_pin(word_select);
    claim_pin(data);
    config_periph_pin(data_periph);
    config_periph_pin(sync_periph);
    config_periph_pin(bclk_periph);
    supervisor_enable_tick(); // object has a finaliser so we can rely on disable_tick being called in _deinit
}

bool common_hal_audiobusio_i2sout_deinited(audiobusio_i2sout_obj_t *self) {
    return self->peripheral != NULL;
}

void common_hal_audiobusio_i2sout_deinit(audiobusio_i2sout_obj_t *self) {
    if (common_hal_audiobusio_i2sout_deinited(self)) {
        return;
    }

    SAI_Deinit(self->peripheral);
    self->peripheral = NULL;

    common_hal_reset_pin(self->bit_clock);
    self->bit_clock = NULL;

    common_hal_reset_pin(self->word_select);
    self->word_select = NULL;

    common_hal_reset_pin(self->data);
    self->data = NULL;

    release_buffers(self);
    supervisor_disable_tick();
}

void common_hal_audiobusio_i2sout_play(audiobusio_i2sout_obj_t *self,
    mp_obj_t sample, bool loop) {
    if (common_hal_audiobusio_i2sout_get_playing(self)) {
        common_hal_audiobusio_i2sout_stop(self);
    }
    self->sample = sample;
    self->loop = loop;
    self->playing = true;
}

void common_hal_audiobusio_i2sout_pause(audiobusio_i2sout_obj_t *self) {
    self->paused = true;
}

void common_hal_audiobusio_i2sout_resume(audiobusio_i2sout_obj_t *self) {
    self->paused = false;
}

bool common_hal_audiobusio_i2sout_get_paused(audiobusio_i2sout_obj_t *self) {
    return self->paused;
}

void common_hal_audiobusio_i2sout_stop(audiobusio_i2sout_obj_t *self) {
    self->playing = False;
    self->sample = NULL;
}

bool common_hal_audiobusio_i2sout_get_playing(audiobusio_i2sout_obj_t *self) {
    return self->playing;
}

#endif // CIRCUITPY_AUDIOBUSIO_I2SOUT
