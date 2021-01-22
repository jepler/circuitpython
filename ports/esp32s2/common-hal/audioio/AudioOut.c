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

// Portions copied from esp-idf are
// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>

#include "py/runtime.h"

#include "common-hal/audioio/AudioOut.h"
#include "shared-bindings/audioio/AudioOut.h"
#include "bindings/espidf/__init__.h"

#include "esp_system.h"
#include "esp_intr_alloc.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "soc/periph_defs.h"
#include "soc/spi_reg.h"
#include "soc/adc_periph.h"
#include "soc/dac_periph.h"
#include "soc/lldesc.h"
#include "soc/system_reg.h"

static const char *TAG = "AudioOut";

// Including "hal/adc_hal.h" fails:
//    In file included from esp-idf/components/soc/include/hal/adc_hal.h:4,
//                     from common-hal/audioio/AudioOut.c:53:
//    esp-idf/components/soc/src/esp32s2/include/hal/adc_ll.h:10:10: fatal error: regi2c_ctrl.h: No such file or directory
//     #include "regi2c_ctrl.h"
//              ^~~~~~~~~~~~~~~
// (regi2c_ctrl.h is in soc/src/esp32s2/ and *NOT* in an include/ directory)
// so the required prototype is given here
extern void adc_hal_digi_clk_config(const adc_digi_clk_t *clk);

// There is only a single DAC DMA, and placing these as file-level statics
// makes it easier to ensure that dma_buf is freed at soft-reset time.
#define BYTES_PER_FRAME (2)
#define FRAMES_PER_BUF (256)
#define BYTES_PER_BUF (BYTES_PER_FRAME * FRAMES_PER_BUF)
#define DMA_BUFS (2)
#define DMA_BUF(i) (dma_buf + i * BYTES_PER_FRAME * FRAMES_PER_BUF)

static uint8_t *dma_buf;
static lldesc_t dma_desc[DMA_BUFS];

// For ADC, INTERVAL must be >= 40 and 9 <= div_num < 255.  As far as I can tell, settings for DAC are all less stringent, or maybe just not as well documented.
// In any case, for a list of common frequencies (48kHz/{1,2,3,6,12}, 44.1kHz/{1,2,4}), choosing an INTERVAL of 100 allows us to get within <20ppm.
// The lowest sample rate (without increasing INTERVAL) is about 3140Hz.  The integer math is "exact" (finds the best fractional divisor), assuming that F_APB % INTERVAL == 0

#define F_APB (80000000)
#define INTERVAL (100)

static adc_digi_clk_t choose_digi_clk(int fs, int fr) {
    int div_num = fr / fs;
    int ferr = fr - div_num * fs;
    if (ferr == 0) {
        return (adc_digi_clk_t) {
            .use_apll = false,
            .div_num = div_num - 1,
            .div_a = 0,
            .div_b = 1,
        };
    }
    int best_den = 0, best_num = 0, best_err2 = INT_MAX;
    for (int den=1; den<=63; den++) {
        int div1 = (uint64_t)fr * den / fs;
        int ferr2 = fr - div1 * fs / den;
        if (abs(ferr2) < best_err2) {
            best_den = den;
            best_num = div1 % den;
            best_err2 = ferr2;
        }
    }

    if (div_num > 256) {
        mp_raise_ValueError(translate("Invalid sample rate"));
    }

    return (adc_digi_clk_t) {
        .use_apll = false,
        .div_num = div_num - 1,
        .div_a = best_num,
        .div_b = best_den,
    };
}

/**
 * SPI DMA type.
 */
typedef enum {
    DMA_ONLY_ADC_INLINK = BIT(1),   /*!<Select ADC-DMA config. */
    DMA_ONLY_DAC_OUTLINK = BIT(2),  /*!<Select DAC-DMA config. */
    DMA_BOTH_ADC_DAC,               /*!<Select DAC-DMA and ADC-DMA config. */
#define DMA_BOTH_ADC_DAC (DMA_ONLY_ADC_INLINK | DMA_ONLY_DAC_OUTLINK)
} spi_dma_link_type_t;

/**
 * Register SPI-DMA interrupt handler.
 *
 * @param handler       Handler.
 * @param handler_arg   Handler parameter.
 * @param intr_mask     DMA interrupt type mask.
 */
esp_err_t adc_dac_dma_isr_register(intr_handler_t handler, void* handler_arg, uint32_t intr_mask);

/**
 * Deregister SPI-DMA interrupt handler.
 *
 * @param handler       Handler.
 * @param handler_arg   Handler parameter.
 */
esp_err_t adc_dac_dma_isr_deregister(intr_handler_t handler, void* handler_arg);

/**
 * Reset DMA linker pointer and start DMA.
 *
 * @param type     DMA linker type. See ``spi_dma_link_type_t``.
 * @param dma_addr DMA linker addr.
 * @param int_msk  DMA interrupt type mask.
 */
void adc_dac_dma_linker_start(spi_dma_link_type_t type, void *dma_addr, uint32_t int_msk);

/**
 * Deinit SPI3 DMA. Disable interrupt, stop DMA trans.
 */
void adc_dac_dma_linker_deinit(void);

typedef struct adc_dac_dma_isr_handler_ {
    uint32_t mask;
    intr_handler_t handler;
    void* handler_arg;
    SLIST_ENTRY(adc_dac_dma_isr_handler_) next;
} adc_dac_dma_isr_handler_t;

static SLIST_HEAD(adc_dac_dma_isr_handler_list_, adc_dac_dma_isr_handler_) s_adc_dac_dma_isr_handler_list =
        SLIST_HEAD_INITIALIZER(s_adc_dac_dma_isr_handler_list);
portMUX_TYPE s_isr_handler_list_lock = portMUX_INITIALIZER_UNLOCKED;
static intr_handle_t s_adc_dac_dma_isr_handle;

static IRAM_ATTR void adc_dac_dma_isr_default(void* arg)
{
    uint32_t status = REG_READ(SPI_DMA_INT_ST_REG(3));
    adc_dac_dma_isr_handler_t* it;
    portENTER_CRITICAL_ISR(&s_isr_handler_list_lock);
    SLIST_FOREACH(it, &s_adc_dac_dma_isr_handler_list, next) {
        if (it->mask & status) {
            portEXIT_CRITICAL_ISR(&s_isr_handler_list_lock);
            (*it->handler)(it->handler_arg);
            portENTER_CRITICAL_ISR(&s_isr_handler_list_lock);
        }
    }
    portEXIT_CRITICAL_ISR(&s_isr_handler_list_lock);
    REG_WRITE(SPI_DMA_INT_CLR_REG(3), status);
}

static esp_err_t adc_dac_dma_isr_ensure_installed(void)
{
    esp_err_t err = ESP_OK;
    portENTER_CRITICAL(&s_isr_handler_list_lock);
    if (s_adc_dac_dma_isr_handle) {
        goto out;
    }
    REG_WRITE(SPI_DMA_INT_ENA_REG(3), 0);
    REG_WRITE(SPI_DMA_INT_CLR_REG(3), UINT32_MAX);
    err = esp_intr_alloc(ETS_SPI3_DMA_INTR_SOURCE, 0, &adc_dac_dma_isr_default, NULL, &s_adc_dac_dma_isr_handle);
    if (err != ESP_OK) {
        goto out;
    }

out:
    portEXIT_CRITICAL(&s_isr_handler_list_lock);
    return err;
}

esp_err_t adc_dac_dma_isr_register(intr_handler_t handler, void* handler_arg, uint32_t intr_mask)
{
    esp_err_t err = adc_dac_dma_isr_ensure_installed();
    if (err != ESP_OK) {
        return err;
    }

    adc_dac_dma_isr_handler_t* item = malloc(sizeof(*item));
    if (item == NULL) {
        return ESP_ERR_NO_MEM;
    }
    item->handler = handler;
    item->handler_arg = handler_arg;
    item->mask = intr_mask;
    portENTER_CRITICAL(&s_isr_handler_list_lock);
    SLIST_INSERT_HEAD(&s_adc_dac_dma_isr_handler_list, item, next);
    portEXIT_CRITICAL(&s_isr_handler_list_lock);
    return ESP_OK;
}

void adc_dac_dma_linker_start(spi_dma_link_type_t type, void *dma_addr, uint32_t int_msk)
{
    REG_SET_BIT(DPORT_PERIP_CLK_EN_REG, DPORT_APB_SARADC_CLK_EN_M);
    REG_SET_BIT(DPORT_PERIP_CLK_EN_REG, DPORT_SPI3_DMA_CLK_EN_M);
    REG_SET_BIT(DPORT_PERIP_CLK_EN_REG, DPORT_SPI3_CLK_EN);
    REG_CLR_BIT(DPORT_PERIP_RST_EN_REG, DPORT_SPI3_DMA_RST_M);
    REG_CLR_BIT(DPORT_PERIP_RST_EN_REG, DPORT_SPI3_RST_M);
    REG_WRITE(SPI_DMA_INT_CLR_REG(3), 0xFFFFFFFF);
    REG_WRITE(SPI_DMA_INT_ENA_REG(3), int_msk | REG_READ(SPI_DMA_INT_ENA_REG(3)));
    if (type & DMA_ONLY_ADC_INLINK) {
        REG_SET_BIT(SPI_DMA_IN_LINK_REG(3), SPI_INLINK_STOP);
        REG_CLR_BIT(SPI_DMA_IN_LINK_REG(3), SPI_INLINK_START);
        SET_PERI_REG_BITS(SPI_DMA_IN_LINK_REG(3), SPI_INLINK_ADDR, (uint32_t)dma_addr, 0);
        REG_SET_BIT(SPI_DMA_CONF_REG(3), SPI_IN_RST);
        REG_CLR_BIT(SPI_DMA_CONF_REG(3), SPI_IN_RST);
        REG_CLR_BIT(SPI_DMA_IN_LINK_REG(3), SPI_INLINK_STOP);
        REG_SET_BIT(SPI_DMA_IN_LINK_REG(3), SPI_INLINK_START);
    }
    if (type & DMA_ONLY_DAC_OUTLINK) {
        REG_SET_BIT(SPI_DMA_OUT_LINK_REG(3), SPI_OUTLINK_STOP);
        REG_CLR_BIT(SPI_DMA_OUT_LINK_REG(3), SPI_OUTLINK_START);
        SET_PERI_REG_BITS(SPI_DMA_OUT_LINK_REG(3), SPI_OUTLINK_ADDR, (uint32_t)dma_addr, 0);
        REG_SET_BIT(SPI_DMA_CONF_REG(3), SPI_OUT_RST);
        REG_CLR_BIT(SPI_DMA_CONF_REG(3), SPI_OUT_RST);
        REG_CLR_BIT(SPI_DMA_OUT_LINK_REG(3), SPI_OUTLINK_STOP);
        REG_SET_BIT(SPI_DMA_OUT_LINK_REG(3), SPI_OUTLINK_START);
    }
}

static void adc_dac_dma_linker_stop(spi_dma_link_type_t type)
{
    if (type & DMA_ONLY_ADC_INLINK) {
        REG_SET_BIT(SPI_DMA_IN_LINK_REG(3), SPI_INLINK_STOP);
        REG_CLR_BIT(SPI_DMA_IN_LINK_REG(3), SPI_INLINK_START);
    }
    if (type & DMA_ONLY_DAC_OUTLINK) {
        REG_SET_BIT(SPI_DMA_OUT_LINK_REG(3), SPI_OUTLINK_STOP);
        REG_CLR_BIT(SPI_DMA_OUT_LINK_REG(3), SPI_OUTLINK_START);
    }
}

static void dac_callback_fun(void *self_in) {
    audioio_audioout_obj_t *self = self_in;

    uint8_t *buf = DMA_BUF(self->fill_buffer), *ebuf = buf + BYTES_PER_BUF;

    if (!self->playing || self->paused || !self->sample || self->stopping) {
        memset(buf, 0, BYTES_PER_BUF);
        return;
    }
    while (!self->stopping && buf != ebuf) {
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

        size_t sample_bytecount = self->sample_end - self->sample_data;
        size_t sample_bytesperframe = self->channel_count * self->bytes_per_sample;

        size_t output_bytecount = ebuf - buf;
        const size_t output_bytesperframe = 2;

        size_t framecount = MIN(output_bytecount / output_bytesperframe, sample_bytecount / sample_bytesperframe);

        if (self->samples_signed) {
            if (self->channel_count == 1) {
                if (self->bytes_per_sample == 1) {
                    audiosample_convert_s8m_u8s(buf, (int8_t*)(void*)self->sample_data, framecount);
                } else {
                    audiosample_convert_s16m_u8s(buf, (int16_t*)(void*)self->sample_data, framecount);
                }
            } else {
                if (self->bytes_per_sample == 1) {
                    audiosample_convert_s8s_u8s(buf, (int8_t*)(void*)self->sample_data, framecount);
                } else {
                    audiosample_convert_s16s_u8s(buf, (int16_t*)(void*)self->sample_data, framecount);
                }
            }
        } else {
            if (self->channel_count == 1) {
                if (self->bytes_per_sample == 1) {
                    audiosample_convert_u8m_u8s(buf, (uint8_t*)(void*)self->sample_data, framecount);
                } else {
                    audiosample_convert_u16m_u8s(buf, (uint16_t*)(void*)self->sample_data, framecount);
                }
            } else {
                if (self->bytes_per_sample == 1) {
                    memcpy(buf, (uint8_t*)(void*)self->sample_data, framecount * output_bytesperframe);
                } else {
                    audiosample_convert_u16s_u8s(buf, (uint16_t*)(void*)self->sample_data, framecount);
                }
            }
        }
        self->sample_data += framecount * sample_bytesperframe;
        buf += framecount * output_bytesperframe;
    }
    if (buf != ebuf) {
        memset(buf, 0, ebuf-buf);
    }
}

/** ADC-DMA ISR handler. */
static IRAM_ATTR void dac_dma_isr(void * self_in)
{
    audioio_audioout_obj_t* self = self_in;

    uint32_t int_st = REG_READ(SPI_DMA_INT_ST_REG(3));
    REG_WRITE(SPI_DMA_INT_CLR_REG(3), int_st);

    self->fill_buffer = !self->fill_buffer;
    background_callback_add_from_isr(&self->callback, dac_callback_fun, self_in);

    ESP_EARLY_LOGV(TAG, "int msk%x, raw%x", int_st, REG_READ(SPI_DMA_INT_RAW_REG(3)));
}


static void release_dma_buf(void) {
    if (dma_buf) {
        adc_dac_dma_linker_stop(DMA_BOTH_ADC_DAC);
        REG_WRITE(SPI_DMA_INT_CLR_REG(3), 0xFFFFFFFF);
        REG_WRITE(SPI_DMA_INT_ENA_REG(3), 0);

        free(dma_buf);
        dma_buf = NULL;
    }
}

static uint32_t allocate_dma_buf(void) {
    if (!dma_buf) {
        dma_buf = calloc(BYTES_PER_FRAME * FRAMES_PER_BUF, DMA_BUFS);
        if (!dma_buf) {
            mp_raise_espidf_MemoryError();
        }
        dma_desc[0] = (lldesc_t) {
            .size = BYTES_PER_FRAME * FRAMES_PER_BUF,
            .length = BYTES_PER_FRAME * FRAMES_PER_BUF,
            .eof = 0,
            .owner = 1,
            .buf = DMA_BUF(0),
            .qe.stqe_next = &dma_desc[1],
        };
        dma_desc[1] = (lldesc_t) {
            .size = BYTES_PER_FRAME * FRAMES_PER_BUF,
            .length = BYTES_PER_FRAME * FRAMES_PER_BUF,
            .eof = 0,
            .owner = 1,
            .buf = DMA_BUF(1),
            .qe.stqe_next = &dma_desc[0],
        };

        for(int i=0; i<2*FRAMES_PER_BUF; i++) {
            int j = i / 2;
            dma_buf[i*2] = j % 256;
            dma_buf[i*2 + 1] = (255-i) % 256;
        }
    }
    return (uint32_t)&dma_desc[0];
}

void common_hal_audioio_audioout_construct(audioio_audioout_obj_t* self,
    const mcu_pin_obj_t* left_channel, const mcu_pin_obj_t* right_channel, uint16_t default_value) {

mp_printf(&mp_plat_print, "left channel number %d vs dac channel %d\n", left_channel->number, (int)DAC_CHANNEL_1_GPIO_NUM);
    if (left_channel->number != (int) DAC_CHANNEL_1_GPIO_NUM) {
        mp_raise_ValueError(translate("Invalid pin for left channel"));
    }

    if (right_channel && right_channel->number != (int) DAC_CHANNEL_2_GPIO_NUM) {
        mp_raise_ValueError(translate("Invalid pin for right channel"));
    }

#if 0
    if (!spi_bus_is_free(SPI3_HOST)) {
        mp_raise_ValueError(translate("SPI3 in use"));
    }
    // TODO: check and claim SPI3 peripheral
#endif

    uint32_t dma_addr = allocate_dma_buf();


// enable DAC channels

    claim_pin(left_channel);
    if (right_channel) {
        claim_pin(right_channel);
    }

    // arbitrary sample rate of 1kHz, this is overridden later
    const dac_digi_config_t cfg = {
        .mode = right_channel ? DAC_CONV_ALTER : DAC_CONV_NORMAL,
        .interval = 10,
        .dig_clk.use_apll = false,  // APB clk
        .dig_clk.div_num = 39,
        .dig_clk.div_b = 1,
        .dig_clk.div_a = 0,
    };

    dac_digi_controller_config(&cfg);

    // const uint32_t int_mask = SPI_OUT_EOF_INT_ENA;
    const uint32_t int_mask = SPI_OUT_DONE_INT_ENA | SPI_OUT_EOF_INT_ENA | SPI_OUT_TOTAL_EOF_INT_ENA;


    dac_output_enable(DAC_CHANNEL_1);
    if (right_channel) {
        dac_output_enable(DAC_CHANNEL_2);
    }
    CHECK_ESP_RESULT(adc_dac_dma_isr_register(dac_dma_isr, (void *)self, int_mask));
    adc_dac_dma_linker_start(DMA_ONLY_DAC_OUTLINK, (void *)dma_addr, int_mask);

    // The first buffer output will be buffer 0.  When the IRQ fires, it will
    // have started outputting buffer 1 and the buffer to fill will be 0.  The
    // IRQ toggles fill_buffer, so setting it to 1 here ensures its value is 0
    // the first time we start to re-fill
    self->fill_buffer = 1;

    self->left_channel = left_channel;
    self->right_channel = right_channel;

    dac_digi_start();
}

void common_hal_audioio_audioout_deinit(audioio_audioout_obj_t* self)
{
    if (common_hal_audioio_audioout_deinited(self)) {
        return;
    }

    release_dma_buf();

    if (self->left_channel) {
        reset_pin_number(self->left_channel->number);
    }
    self->left_channel = NULL;

    if (self->right_channel) {
        reset_pin_number(self->right_channel->number);
    }
    self->right_channel = NULL;
}

bool common_hal_audioio_audioout_deinited(audioio_audioout_obj_t* self)
{
    return !self->left_channel;
}

void common_hal_audioio_audioout_play(audioio_audioout_obj_t* self, mp_obj_t sample, bool loop)
{
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
    self->paused = false;
    self->stopping = false;
    self->sample_data = self->sample_end = NULL;

    audiosample_reset_buffer(self->sample, false, 0);

    adc_digi_clk_t clk = choose_digi_clk(F_APB / INTERVAL, audiosample_sample_rate(sample));

    adc_hal_digi_clk_config(&clk);

    self->playing = true;
}

void common_hal_audioio_audioout_stop(audioio_audioout_obj_t* self)
{
    self->sample = NULL;
    self->paused = false;
    self->playing = false;
    self->stopping = false;
}

bool common_hal_audioio_audioout_get_playing(audioio_audioout_obj_t* self)
{
    return self->playing && !self->stopping;
}

void common_hal_audioio_audioout_pause(audioio_audioout_obj_t* self)
{
    if (!self->paused) {
        self->paused = true;
        // DMA stop
    }
    mp_raise_NotImplementedError(NULL);
}

void common_hal_audioio_audioout_resume(audioio_audioout_obj_t* self)
{
    if (self->paused) {
        self->paused = false;
        // DMA start
    }
    mp_raise_NotImplementedError(NULL);
}

bool common_hal_audioio_audioout_get_paused(audioio_audioout_obj_t* self)
{
    return self->paused;
}
