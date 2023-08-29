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

#include "esp_log.h"
#define TAG "LCD"

#include "components/esp_rom/include/esp_rom_sys.h"


#define FASTHERE() (mp_printf(&mp_plat_print, "%s:%d [%8u]\n", __FILE__, __LINE__, (unsigned int)supervisor_ticks_ms32()), (void)0)
#define HERE() (mp_printf(&mp_plat_print, "%s:%d [%8u]\n", __FILE__, __LINE__, (unsigned int)supervisor_ticks_ms32()), mp_hal_delay_ms(10), (void)0)
#define SAY(fmt, ...) (mp_printf(&mp_plat_print, "%s:%d: [%8u] " fmt "\n", __FILE__, __LINE__, (unsigned int)supervisor_ticks_ms32(),##__VA_ARGS__), mp_hal_delay_ms(100), (void)0)
#define VAL(fmt, arg) (SAY(#arg "=" fmt, arg))

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
    }
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

    esp_lcd_rgb_panel_config_t *_panel_config = (esp_lcd_rgb_panel_config_t *)heap_caps_calloc(1, sizeof(esp_lcd_rgb_panel_config_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    esp_lcd_panel_handle_t _panel_handle = NULL;

    claim_and_record(de, &self->used_pins_mask);
    /// and so on for other pins TODO

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(_panel_config, &_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(_panel_handle));

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
    return self->frame_count++;
    // return self->frequency;
}

mp_int_t common_hal_dotclockframebuffer_framebuffer_get_refresh_rate(dotclockframebuffer_framebuffer_obj_t *self) {
    uint32_t clocks_per_line =
        self->timing.h_res + self->timing.hsync_back_porch + self->timing.hsync_front_porch;
    uint32_t lines_per_frame =
        self->timing.v_res + self->timing.vsync_back_porch + self->timing.vsync_front_porch;
    uint32_t clocks_per_frame = clocks_per_line * lines_per_frame;
    return self->frequency / clocks_per_frame;
}
