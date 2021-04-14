/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Jeff Epler for Adafruit Industries
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

#include "common-hal/audiobusio/I2SOut.h"

#include "shared-bindings/audiobusio/I2SOut.h"
#include "shared-bindings/microcontroller/Pin.h"

#include "supervisor/shared/translate.h"

#include "periph.h"

static void config_periph_pin(const mcu_sai_obj_t *periph) {
    IOMUXC_SetPinMux(
        periph->pin->mux_reg, periph->mux_mode,
        periph->input_reg, periph->input_idx,
        periph->pin->cfg_reg,
        1);

    IOMUXC_SetPinConfig(0, 0, 0, 0,
        periph->pin->cfg_reg,
        IOMUXC_SW_PAD_CTL_PAD_HYS(0)
        | IOMUXC_SW_PAD_CTL_PAD_PUS(1)
        | IOMUXC_SW_PAD_CTL_PAD_PUE(1)
        | IOMUXC_SW_PAD_CTL_PAD_PKE(1)
        | IOMUXC_SW_PAD_CTL_PAD_ODE(0)
        | IOMUXC_SW_PAD_CTL_PAD_SPEED(1)
        | IOMUXC_SW_PAD_CTL_PAD_DSE(6)
        | IOMUXC_SW_PAD_CTL_PAD_SRE(0));
}

static const mcu_sai_obj_t *find_sai_pin(const mcu_pin_obj_t *pin, const mcu_sai_obj_t *table, size_t n, int index, int pin_name) {
    for(size_t i=0; i<n; i++) {
        if(pin != table[i].pin) { continue; }
        if(index != -1 && index != table[i].sai) { continue; }
        return &table[i];
    }
    mp_raise_ValueError_varg(translate("Invalid %q pin"), pin_name);
}

void i2sout_reset(void) {}
void audio_dma_background(void) {}

void common_hal_audiobusio_i2sout_construct(audiobusio_i2sout_obj_t *self,
    const mcu_pin_obj_t *bit_clock, const mcu_pin_obj_t *word_select, const mcu_pin_obj_t *data,
    bool left_justified)
{
    self->data = find_sai_pin(data, mcu_sai_tx_data_list, MP_ARRAY_SIZE(mcu_sai_tx_data_list), -1, MP_QSTR_data);
    self->bit_clock = find_sai_pin(bit_clock, mcu_sai_tx_bclk_list, MP_ARRAY_SIZE(mcu_sai_tx_bclk_list), self->data->sai, MP_QSTR_bit_clock);
    self->word_select = find_sai_pin(word_select, mcu_sai_tx_sync_list, MP_ARRAY_SIZE(mcu_sai_tx_sync_list), self->data->sai, MP_QSTR_word_select);
    self->index = self->data->sai;
    self->left_justified = left_justified;

    audio_dma_allocate_channel(&self->dma, true, self->index, self->data->channel);

    common_hal_never_reset_pin(bit_clock);
    common_hal_never_reset_pin(word_select);
    common_hal_never_reset_pin(data);

    config_periph_pin(self->data);
    config_periph_pin(self->bit_clock);
    config_periph_pin(self->word_select);
}

void common_hal_audiobusio_i2sout_deinit(audiobusio_i2sout_obj_t *self)
{
    // release EDMA channel

    if (self->data) {
        common_hal_reset_pin(self->data->pin);
    }
    self->data = NULL;

    if (self->bit_clock) {
        common_hal_reset_pin(self->bit_clock->pin);
    }
    self->bit_clock = NULL;

    if (self->word_select) {
        common_hal_reset_pin(self->word_select->pin);
    }
    self->word_select = NULL;
}

bool common_hal_audiobusio_i2sout_deinited(audiobusio_i2sout_obj_t *self)
{
    return !self->data;
}

void common_hal_audiobusio_i2sout_play(audiobusio_i2sout_obj_t *self, mp_obj_t sample, bool loop)
{
    audio_dma_play(&self->dma, sample, loop);
}

void common_hal_audiobusio_i2sout_stop(audiobusio_i2sout_obj_t *self)
{
    audio_dma_stop(&self->dma);
}

bool common_hal_audiobusio_i2sout_get_playing(audiobusio_i2sout_obj_t *self)
{
    return audio_dma_get_playing(&self->dma);
}

void common_hal_audiobusio_i2sout_pause(audiobusio_i2sout_obj_t *self)
{
    return audio_dma_pause(&self->dma);
}

void common_hal_audiobusio_i2sout_resume(audiobusio_i2sout_obj_t *self)
{
    return audio_dma_resume(&self->dma);
}

bool common_hal_audiobusio_i2sout_get_paused(audiobusio_i2sout_obj_t *self)
{
    return audio_dma_get_paused(&self->dma);
}
