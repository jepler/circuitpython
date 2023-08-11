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

#include "shared-bindings/dotclockframebuffer/DotClockFramebuffer.h"
#include "common-hal/dotclockframebuffer/DotClockFramebuffer.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "py/runtime.h"
#include "components/esp_rom/include/esp_rom_gpio.h"
#include "components/hal/include/hal/gpio_hal.h"
#include "components/driver/include/driver/gpio.h"
#include "components/driver/include/driver/periph_ctrl.h"
#include "components/soc/esp32s3/include/soc/lcd_cam_struct.h"
#include "components/driver/include/esp_private/gdma.h"
#include "esp_heap_caps.h"

#define common_hal_mcu_pin_number_maybe(x) ((x) ? common_hal_mcu_pin_number((x)) : -1)

static void pinmux(int8_t pin, uint8_t signal) {
    esp_rom_gpio_connect_out_signal(pin, signal, false, false);
    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
    gpio_set_drive_capability((gpio_num_t)pin, GPIO_DRIVE_CAP_MAX);
}

static void claim_and_pinmux(const mcu_pin_obj_t *pin, uint8_t signal, uint64_t *used_pins_mask) {
    int number = common_hal_mcu_pin_number(pin);
    *used_pins_mask |= (UINT64_C(1) << number);
    claim_pin_number(number);
    pinmux(number, signal);
}

void common_hal_dotclockframebuffer_framebuffer_construct(dotclockframebuffer_framebuffer_obj_t *self,
    const mcu_pin_obj_t *de,
    const mcu_pin_obj_t *vsync,
    const mcu_pin_obj_t *hsync,
    const mcu_pin_obj_t *dclk,
    const mcu_pin_obj_t **red, uint8_t num_red,
    const mcu_pin_obj_t **green, uint8_t num_green,
    const mcu_pin_obj_t **blue, uint8_t num_blue,
    int frequency, int width, int height,
    int hsync_pulse_width, int hsync_back_porch, int hsync_front_porch, bool hsync_idle_low,
    int vsync_pulse_width, int vsync_back_porch, int vsync_front_porch, bool vsync_idle_low,
    bool de_idle_high, bool pclk_active_high, bool pclk_idle_high) {

    // LCD_CAM isn't enabled by default -- MUST begin with this:
    periph_module_enable(PERIPH_LCD_CAM_MODULE);
    periph_module_reset(PERIPH_LCD_CAM_MODULE);

    // Reset LCD bus
    LCD_CAM.lcd_user.lcd_reset = 1;
    mp_hal_delay_us(100);

    // Configure LCD clock
    LCD_CAM.lcd_clock.clk_en = 1;           // Enable clock
    LCD_CAM.lcd_clock.lcd_clk_sel = 3;      // PLL160M source
    LCD_CAM.lcd_clock.lcd_clkm_div_a = 1;   // 1/1 fractional divide,
    LCD_CAM.lcd_clock.lcd_clkm_div_b = 1;   // plus '7' below yields...
    LCD_CAM.lcd_clock.lcd_clkm_div_num = 7; // 1:8 prescale (20 MHz CLK)
    LCD_CAM.lcd_clock.lcd_ck_out_edge = 0;  // PCLK low in first half of cycle
    LCD_CAM.lcd_clock.lcd_ck_idle_edge = 0; // PCLK low idle
    LCD_CAM.lcd_clock.lcd_clk_equ_sysclk = 1; // PCLK = CLK (ignore CLKCNT_N)

    // Configure frame format. Some of these could probably be skipped and
    // use defaults, but being verbose for posterity...
    LCD_CAM.lcd_ctrl.lcd_rgb_mode_en = 1;  // RGB modd (not i8080)
    LCD_CAM.lcd_rgb_yuv.lcd_conv_bypass = 0; // Disable RGB/YUV converter
    LCD_CAM.lcd_misc.lcd_next_frame_en = 1; // Do NOT auto-frame
    LCD_CAM.lcd_data_dout_mode.val = 0;    // No data delays
    LCD_CAM.lcd_user.lcd_always_out_en = 0; // Only when requested
    LCD_CAM.lcd_user.lcd_8bits_order = 0;  // Do not swap bytes
    LCD_CAM.lcd_user.lcd_bit_order = 0;    // Do not reverse bit order
    LCD_CAM.lcd_user.lcd_2byte_en = 1;     // 16-bit data mode
                                           //
    LCD_CAM.lcd_user.lcd_dummy = 0;        // No dummy phase(s) @ LCD start
    LCD_CAM.lcd_user.lcd_dummy_cyclelen = 0; // minimum dummy phases
    LCD_CAM.lcd_user.lcd_cmd = 0;          // No command at LCD start
    LCD_CAM.lcd_user.lcd_cmd_2_cycle_en = 0;
    LCD_CAM.lcd_user.lcd_update = 1;

    // TODO set frequency via fractional scaler
    self->timing.pclk_hz = frequency;
    self->timing.h_res = width;
    self->timing.v_res = height;
    self->timing.hsync_pulse_width = hsync_pulse_width;
    self->timing.hsync_back_porch = hsync_back_porch;
    self->timing.hsync_front_porch = hsync_front_porch;
    self->timing.vsync_pulse_width = vsync_pulse_width;
    self->timing.vsync_back_porch = vsync_back_porch;
    self->timing.vsync_front_porch = vsync_front_porch;
    self->timing.flags.hsync_idle_low = hsync_idle_low;
    self->timing.flags.vsync_idle_low = vsync_idle_low;
    self->timing.flags.de_idle_high = de_idle_high;
    self->timing.flags.pclk_active_neg = !pclk_active_high;
    self->timing.flags.pclk_idle_high = pclk_idle_high;

    // install interrupt service, (LCD peripheral shares the interrupt source with Camera by different mask)
    const int isr_flags = LCD_RGB_INTR_ALLOC_FLAGS | ESP_INTR_FLAG_SHARED;
    esp_err_t ret = esp_intr_alloc_intrstatus(ETS_LCD_CAM_INTR_SOURCE, isr_flags,
        (uint32_t)lcd_ll_get_interrupt_status_reg(LCD_CAM),
        LCD_LL_EVENT_VSYNC_END, lcd_default_isr_handler, rgb_panel, &rgb_panel->intr);
    // handle error in ret

    lcd_ll_set_blank_cycles(LCD_CAM, 1, 1); // RGB panel always has a front and back blank (porch region)
    lcd_ll_set_horizontal_timing(LCD_CAM, self->timings.hsync_pulse_width,
        self->timings.hsync_back_porch, self->timings.h_res,
        self->timings.hsync_front_porch);
    lcd_ll_set_vertical_timing(LCD_CAM, self->timings.vsync_pulse_width,
        self->timings.vsync_back_porch, self->timings.v_res,
        self->timings.vsync_front_porch);

    // output hsync even in porch region
    lcd_ll_enable_output_hsync_in_porch_region(LCD_CAM, true);
    // generate the hsync at the very beginning of line
    lcd_ll_set_hsync_position(LCD_CAM, 0);
    // restart flush by hardware has some limitation, instead, the driver will restart the flush in the VSYNC end interrupt by software
    lcd_ll_enable_auto_next_frame(LCD_CAM, false);
    // trigger interrupt on the end of frame
    lcd_ll_enable_interrupt(LCD_CAM, LCD_LL_EVENT_VSYNC_END, true);
    // enable intr
    esp_intr_enable(rgb_panel->intr);

    self->config.data_width = 16;
    self->config.hsync_gpio_num = common_hal_mcu_pin_number(hsync);
    self->config.vsync_gpio_num = common_hal_mcu_pin_number(vsync);
    self->config.de_gpio_num = common_hal_mcu_pin_number(de);
    self->config.pclk_gpio_num = common_hal_mcu_pin_number(dclk);

    claim_and_pinmux(hsync, LCD_H_SYNC_IDX, &self->used_pins_mask);
    claim_and_pinmux(vsync, LCD_V_SYNC_IDX, &self->used_pins_mask);
    claim_and_pinmux(de, LCD_H_ENABLE_IDX, &self->used_pins_mask);
    claim_and_pinmux(dclk, LCD_PCLK_IDX, &self->used_pins_mask);

    for (size_t i = 0; i < num_red && i < 5; i++) {
        size_t j = i + LCD_DATA_OUT11_IDX + i; // these are sequential right??
        claim_and_pinmux(red[i], j, &self->used_pins_mask);
    }
    for (size_t i = 0; i < num_green && i < 6; i++) {
        size_t j = i + LCD_DATA_OUT5_IDX + i;
        claim_and_pinmux(green[i], j, &self->used_pins_mask);
    }
    for (size_t i = 0; i < num_blue && i < 6; i++) {
        size_t j = LCD_DATA_OUT0_IDX + i;
        claim_and_pinmux(blue[i], j, &self->used_pins_mask);
    }
    self->config.disp_gpio_num = -1;

    mp_raise_NotImplementedError(NULL);
}


void common_hal_dotclockframebuffer_framebuffer_deinit(dotclockframebuffer_framebuffer_obj_t *self) {
    if (common_hal_dotclockframebuffer_framebuffer_deinitialized(self)) {
        return;
    }

    // Reset LCD bus
    LCD_CAM.lcd_user.lcd_reset = 1;
    mp_hal_delay_us(100);

    periph_module_disable(PERIPH_LCD_CAM_MODULE);

    gdma_del_channel(self->dma_channel);
    heap_caps_free(self->dma_nodes);
    reset_pin_mask(self->used_pins_mask);
    self->used_pins_mask = 0;
}

bool common_hal_dotclockframebuffer_framebuffer_deinitialized(dotclockframebuffer_framebuffer_obj_t *self) {
    return self->used_pins_mask == 0;
}


mp_int_t common_hal_dotclockframebuffer_framebuffer_get_width(dotclockframebuffer_framebuffer_obj_t *self) {
    return self->timing.h_res;
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_height(dotclockframebuffer_framebuffer_obj_t *self) {
    return self->timing.v_res;
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_frequency(dotclockframebuffer_framebuffer_obj_t *self) {
    return self->frequency;
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_refresh_rate(dotclockframebuffer_framebuffer_obj_t *self) {
    uint32_t clocks_per_line =
        self->timing.h_res + self->timing.hsync_back_porch + self->timing.hsync_front_porch;
    uint32_t lines_per_frame =
        self->timing.v_res + self->timing.vsync_back_porch + self->timing.vsync_front_porch;
    uint32_t clocks_per_frame = clocks_per_line * lines_per_frame;
    return self->frequency / clocks_per_frame;
}
