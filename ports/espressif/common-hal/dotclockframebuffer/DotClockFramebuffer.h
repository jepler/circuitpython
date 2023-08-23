/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Jeff Epler for Adafruit Industries
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

#pragma once

#include "py/obj.h"

#include "soc/soc_caps.h"
#include "esp_private/gdma.h"
#include "hal/dma_types.h"
#include "components/esp_hw_support/include/esp_intr_alloc.h"

typedef struct {
    unsigned int pclk_hz;           /*!< Frequency of pixel clock */
    unsigned int h_res;             /*!< Horizontal resolution, i.e. the number of pixels in a line */
    unsigned int v_res;             /*!< Vertical resolution, i.e. the number of lines in the frame  */
    unsigned int hsync_pulse_width; /*!< Horizontal sync width, unit: PCLK period */
    unsigned int hsync_back_porch;  /*!< Horizontal back porch, number of PCLK between hsync and start of line active data */
    unsigned int hsync_front_porch; /*!< Horizontal front porch, number of PCLK between the end of active data and the next hsync */
    unsigned int vsync_pulse_width; /*!< Vertical sync width, unit: number of lines */
    unsigned int vsync_back_porch;  /*!< Vertical back porch, number of invalid lines between vsync and start of frame */
    unsigned int vsync_front_porch; /*!< Vertical front porch, number of invalid lines between the end of frame and the next vsync */
    struct {
        unsigned int hsync_idle_low : 1;  /*!< The hsync signal is low in IDLE state */
        unsigned int vsync_idle_low : 1;  /*!< The vsync signal is low in IDLE state */
        unsigned int de_idle_high : 1;    /*!< The de signal is high in IDLE state */
        unsigned int pclk_active_neg : 1; /*!< Whether the display data is clocked out at the falling edge of PCLK */
        unsigned int pclk_idle_high : 1;  /*!< The PCLK stays at high level in IDLE phase */
    } flags;                             /*!< LCD RGB timing flags */
} dotclock_timing_t;

typedef struct {
    // lcd_clock_source_t clk_src;   /*!< Clock source for the RGB LCD peripheral */
    size_t data_width;            /*!< Number of data lines */
    // size_t sram_trans_align;      /*!< Alignment for framebuffer that allocated in SRAM */
    // size_t psram_trans_align;     /*!< Alignment for framebuffer that allocated in PSRAM */
    int hsync_gpio_num;           /*!< GPIO used for HSYNC signal */
    int vsync_gpio_num;           /*!< GPIO used for VSYNC signal */
    int de_gpio_num;              /*!< GPIO used for DE signal, set to -1 if it's not used */
    int pclk_gpio_num;            /*!< GPIO used for PCLK signal */
    int data_gpio_nums[SOC_LCD_RGB_DATA_WIDTH]; /*!< GPIOs used for data lines */
    int disp_gpio_num; /*!< GPIO used for display control signal, set to -1 if it's not used */
    // void *user_ctx; /*!< User data which would be passed to on_frame_trans_done's user_ctx */
    struct {
        unsigned int disp_active_low : 1; /*!< If this flag is enabled, a low level of display control signal can turn the screen on; vice versa */
        unsigned int relax_on_idle : 1;   /*!< If this flag is enabled, the host won't refresh the LCD if nothing changed in host's frame buffer (this is useful for LCD with built-in GRAM) */
        // unsigned int fb_in_psram: 1;     /*!< If this flag is enabled, the frame buffer will be allocated from PSRAM preferentially */
    } flags;                             /*!< LCD RGB panel configuration flags */
} dotclock_config_t;

typedef struct dotclockframebuffer_framebuffer_obj {
    mp_obj_base_t base;
    mp_buffer_info_t bufinfo;
    uint32_t frequency, refresh_rate;
    dotclock_timing_t timing;
    dotclock_config_t config;
    uint64_t used_pins_mask;
    gdma_channel_handle_t dma_channel;
    size_t n_dma_nodes;
    dma_descriptor_t *dma_nodes;
    intr_handle_t intr;
    uint16_t *fb;
    volatile int32_t frame_count;
} dotclockframebuffer_framebuffer_obj_t;
