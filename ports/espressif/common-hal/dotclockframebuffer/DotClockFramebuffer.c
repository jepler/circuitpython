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

#include <stdint.h>
typedef struct _mp_print_t mp_print_t;
extern const mp_print_t mp_plat_print;
int mp_printf(const mp_print_t *print, const char *fmt, ...);
void mp_hal_delay_ms(unsigned);
uint32_t supervisor_ticks_ms32(void);

#include "esp_intr_alloc.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_pm.h"
#include "esp_private/gdma.h"
#include "hal/dma_types.h"
#include "hal/lcd_hal.h"
#include "hal/lcd_ll.h"
#include "soc/lcd_periph.h"

// extract from esp-idf esp_lcd_rgb_panel.c
typedef struct
{
    esp_lcd_panel_t base;                                      // Base class of generic lcd panel
    int panel_id;                                              // LCD panel ID
    lcd_hal_context_t hal;                                     // Hal layer object
    size_t data_width;                                         // Number of data lines (e.g. for RGB565, the data width is 16)
    size_t sram_trans_align;                                   // Alignment for framebuffer that allocated in SRAM
    size_t psram_trans_align;                                  // Alignment for framebuffer that allocated in PSRAM
    int disp_gpio_num;                                         // Display control GPIO, which is used to perform action like "disp_off"
    intr_handle_t intr;                                        // LCD peripheral interrupt handle
    esp_pm_lock_handle_t pm_lock;                              // Power management lock
    size_t num_dma_nodes;                                      // Number of DMA descriptors that used to carry the frame buffer
    uint8_t *fb;                                               // Frame buffer
    size_t fb_size;                                            // Size of frame buffer
    int data_gpio_nums[SOC_LCD_RGB_DATA_WIDTH];                // GPIOs used for data lines, we keep these GPIOs for action like "invert_color"
    size_t resolution_hz;                                      // Peripheral clock resolution
    esp_lcd_rgb_timing_t timings;                              // RGB timing parameters (e.g. pclk, sync pulse, porch width)
    gdma_channel_handle_t dma_chan;                            // DMA channel handle
    esp_lcd_rgb_panel_frame_trans_done_cb_t on_frame_trans_done; // Callback, invoked after frame trans done
    void *user_ctx;                                            // Reserved user's data of callback functions
    int x_gap;                                                 // Extra gap in x coordinate, it's used when calculate the flush window
    int y_gap;                                                 // Extra gap in y coordinate, it's used when calculate the flush window
    struct
    {
        unsigned int disp_en_level : 1; // The level which can turn on the screen by `disp_gpio_num`
        unsigned int stream_mode : 1; // If set, the LCD transfers data continuously, otherwise, it stops refreshing the LCD when transaction done
        unsigned int fb_in_psram : 1; // Whether the frame buffer is in PSRAM
    } flags;
    dma_descriptor_t dma_nodes[]; // DMA descriptor pool of size `num_dma_nodes`
} esp_rgb_panel_t;


#include "esp_log.h"
#define TAG "LCD"

#include "components/esp_rom/include/esp_rom_sys.h"


#define FASTHERE() (mp_printf(&mp_plat_print, "%s:%d [%8u]\n", __FILE__, __LINE__, (unsigned int)supervisor_ticks_ms32()), (void)0)
#define HERE() (mp_printf(&mp_plat_print, "%s:%d [%8u]\n", __FILE__, __LINE__, (unsigned int)supervisor_ticks_ms32()), mp_hal_delay_ms(10), (void)0)
#define SAY(fmt, ...) (mp_printf(&mp_plat_print, "%s:%d: [%8u] " fmt "\n", __FILE__, __LINE__, (unsigned int)supervisor_ticks_ms32(),##__VA_ARGS__), mp_hal_delay_ms(100), (void)0)
#define VAL(fmt, arg) (SAY(#arg "=" fmt, arg))

#include "py/objarray.h"
#include "shared-bindings/dotclockframebuffer/DotClockFramebuffer.h"
#include "common-hal/dotclockframebuffer/DotClockFramebuffer.h"
#include "bindings/espidf/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "py/runtime.h"
#include "components/driver/include/driver/gpio.h"
#include "components/driver/include/driver/periph_ctrl.h"
#include "components/driver/include/esp_private/gdma.h"
#include "components/esp_rom/include/esp_rom_gpio.h"
#include "components/hal/esp32s3/include/hal/lcd_ll.h"
#include "components/hal/include/hal/gpio_hal.h"
#include "components/soc/esp32s3/include/soc/lcd_cam_struct.h"
#include "esp_heap_caps.h"

#define LCD_RGB_ISR_IRAM_SAFE (1)
#define LCD_RGB_INTR_ALLOC_FLAGS     (ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_INTRDISABLED)

#define common_hal_mcu_pin_number_maybe(x) ((x) ? common_hal_mcu_pin_number((x)) : -1)

static void claim_and_record(const mcu_pin_obj_t *pin, uint64_t *used_pins_mask) {
    if (pin) {
        int number = common_hal_mcu_pin_number(pin);
        *used_pins_mask |= (UINT64_C(1) << number);
        claim_pin_number(number);
        never_reset_pin_number(number);
    }
}

static int valid_pin(const mcu_pin_obj_t *pin, qstr name) {
    int result = common_hal_mcu_pin_number(pin);
    if (result == NO_PIN) {
        mp_raise_ValueError_varg(translate("Invalid %q pin"), name);
    }
    return result;
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

    if (num_red != 5 || num_green != 6 || num_blue != 5) {
        mp_raise_ValueError(translate("Must provide 5/6/5 RGB pins"));
    }

    claim_and_record(de, &self->used_pins_mask);
    claim_and_record(vsync, &self->used_pins_mask);
    claim_and_record(hsync, &self->used_pins_mask);
    claim_and_record(dclk, &self->used_pins_mask);

    for (size_t i = 0; i < num_red; i++) {
        claim_and_record(red[i], &self->used_pins_mask);
    }
    for (size_t i = 0; i < num_green; i++) {
        claim_and_record(green[i], &self->used_pins_mask);
    }
    for (size_t i = 0; i < num_blue; i++) {
        claim_and_record(blue[i], &self->used_pins_mask);
    }

    esp_lcd_rgb_panel_config_t *cfg = &self->panel_config;
    cfg->timings.pclk_hz = frequency;
    cfg->timings.h_res = width;
    cfg->timings.v_res = height;
    cfg->timings.hsync_pulse_width = hsync_pulse_width;
    cfg->timings.hsync_back_porch = hsync_back_porch;
    cfg->timings.hsync_front_porch = hsync_front_porch;
    cfg->timings.vsync_pulse_width = vsync_pulse_width;
    cfg->timings.vsync_back_porch = vsync_back_porch;
    cfg->timings.vsync_front_porch = vsync_front_porch;
    cfg->timings.flags.hsync_idle_low = hsync_idle_low;
    cfg->timings.flags.vsync_idle_low = hsync_idle_low;
    cfg->timings.flags.de_idle_high = de_idle_high;
    cfg->timings.flags.pclk_active_neg = !pclk_active_high;
    cfg->timings.flags.pclk_idle_high = pclk_idle_high;

    cfg->data_width = 16;
    cfg->sram_trans_align = 8;
    cfg->psram_trans_align = 64;
    cfg->hsync_gpio_num = valid_pin(hsync, MP_QSTR_hsync);
    cfg->vsync_gpio_num = valid_pin(vsync, MP_QSTR_vsync);
    cfg->de_gpio_num = valid_pin(de, MP_QSTR_de);
    cfg->pclk_gpio_num = valid_pin(dclk, MP_QSTR_dclk);

    cfg->data_gpio_nums[0] = valid_pin(blue[0], MP_QSTR_blue);
    cfg->data_gpio_nums[1] = valid_pin(blue[1], MP_QSTR_blue);
    cfg->data_gpio_nums[2] = valid_pin(blue[2], MP_QSTR_blue);
    cfg->data_gpio_nums[3] = valid_pin(blue[3], MP_QSTR_blue);
    cfg->data_gpio_nums[4] = valid_pin(blue[4], MP_QSTR_blue);

    cfg->data_gpio_nums[5] = valid_pin(green[0], MP_QSTR_green);
    cfg->data_gpio_nums[6] = valid_pin(green[1], MP_QSTR_green);
    cfg->data_gpio_nums[7] = valid_pin(green[2], MP_QSTR_green);
    cfg->data_gpio_nums[8] = valid_pin(green[3], MP_QSTR_green);
    cfg->data_gpio_nums[9] = valid_pin(green[4], MP_QSTR_green);
    cfg->data_gpio_nums[10] = valid_pin(green[5], MP_QSTR_green);

    cfg->data_gpio_nums[11] = valid_pin(red[0], MP_QSTR_red);
    cfg->data_gpio_nums[12] = valid_pin(red[1], MP_QSTR_red);
    cfg->data_gpio_nums[13] = valid_pin(red[2], MP_QSTR_red);
    cfg->data_gpio_nums[14] = valid_pin(red[3], MP_QSTR_red);
    cfg->data_gpio_nums[15] = valid_pin(red[4], MP_QSTR_red);

    cfg->disp_gpio_num = GPIO_NUM_NC;

    cfg->flags.disp_active_low = 0;
    cfg->flags.relax_on_idle = 0;
    cfg->flags.fb_in_psram = 1; // allocate frame buffer in PSRAM

    HERE();
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&self->panel_config, &self->panel_handle));
    HERE();
    ESP_ERROR_CHECK(esp_lcd_panel_reset(self->panel_handle));
    HERE();
    ESP_ERROR_CHECK(esp_lcd_panel_init(self->panel_handle));
    HERE();

    uint16_t color = 0;
    ESP_ERROR_CHECK(self->panel_handle->draw_bitmap(self->panel_handle, 0, 0, 1, 1, &color));
    HERE();

    esp_rgb_panel_t *_rgb_panel = __containerof(self->panel_handle, esp_rgb_panel_t, base);
    HERE();

    self->frequency = frequency;
    HERE();
    self->refresh_rate = frequency / (width + hsync_front_porch + hsync_back_porch) / (height + vsync_front_porch + vsync_back_porch);
    HERE();
    self->bufinfo.buf = _rgb_panel->fb;
    HERE();
    self->bufinfo.len = 2 * width * height;
    self->bufinfo.typecode = 'H' | MP_OBJ_ARRAY_TYPECODE_FLAG_RW;

    memset(self->bufinfo.buf, 0xaa, width * height);
    HERE();
    memset(self->bufinfo.buf + width * height, 0x55, width * height);
    HERE();

//  LCD_CAM.lcd_ctrl2.lcd_vsync_idle_pol = _vsync_polarity;
//  LCD_CAM.lcd_ctrl2.lcd_hsync_idle_pol = _hsync_polarity;
    HERE();

}


void common_hal_dotclockframebuffer_framebuffer_deinit(dotclockframebuffer_framebuffer_obj_t *self) {
    HERE();
    if (common_hal_dotclockframebuffer_framebuffer_deinitialized(self)) {
        return;
    }
    HERE();

    reset_pin_mask(self->used_pins_mask);
    HERE();
    self->used_pins_mask = 0;
    HERE();
    esp_lcd_panel_del(self->panel_handle);
    HERE();
}

bool common_hal_dotclockframebuffer_framebuffer_deinitialized(dotclockframebuffer_framebuffer_obj_t *self) {
    HERE();
    return self->used_pins_mask == 0;
}


mp_int_t common_hal_dotclockframebuffer_framebuffer_get_width(dotclockframebuffer_framebuffer_obj_t *self) {
    HERE();
    return self->panel_config.timings.h_res;
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_height(dotclockframebuffer_framebuffer_obj_t *self) {
    HERE();
    return self->panel_config.timings.v_res;
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_frequency(dotclockframebuffer_framebuffer_obj_t *self) {
    HERE();
    return self->frequency;
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_refresh_rate(dotclockframebuffer_framebuffer_obj_t *self) {
    HERE();
    return self->refresh_rate;
}
