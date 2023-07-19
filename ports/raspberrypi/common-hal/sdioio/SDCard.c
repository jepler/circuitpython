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
#include <stdbool.h>

#include "py/mperrno.h"
#include "py/runtime.h"

#include "bindings/rp2pio/__init__.h"
#include "bindings/rp2pio/StateMachine.h"
#include "common-hal/microcontroller/Pin.h"
#include "peripherals/include/pico/sd_card.h"
#include "shared-bindings/microcontroller/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/sdioio/SDCard.h"
#include "shared-bindings/util.h"
#include "shared/runtime/interrupt_char.h"
#include "supervisor/shared/translate/translate.h"

#ifndef DEBUG_SDIO
#define DEBUG_SDIO (0)
#endif

#if DEBUG_SDIO
#define DEBUG_PRINT(...) ((void)mp_printf(&mp_plat_print, __VA_ARGS__))
#define DEBUG_PRINT_OBJ(o) ((void)mp_obj_print_helper(&mp_plat_print, (mp_obj_t)o, PRINT_REPR))
#else
#define DEBUG_PRINT(...) ((void)0)
#define DEBUG_PRINT_OBJ(...) ((void)0)
#endif
#define DEBUG_PRINT_OBJ_NL(o) (DEBUG_PRINT_OBJ(o), DEBUG_PRINT("\n"))

STATIC int map_sdio_error(const char *msg, int r) {
    DEBUG_PRINT("%s -> %d\n", msg, r);
    switch (r) {
        case SD_OK:
            return 0;
        case SD_ERR_STUCK:
            return -MP_EINTR;
        case SD_ERR_CRC:
            return -MP_EIO;
        default:
        case SD_ERR_BAD_PARAM:
            return -MP_EINVAL;
    }
}


void common_hal_sdioio_sdcard_construct(sdioio_sdcard_obj_t *self,
    const mcu_pin_obj_t *clock, const mcu_pin_obj_t *command,
    uint8_t num_data, const mcu_pin_obj_t **data, uint32_t frequency) {


    if (!common_hal_rp2pio_pins_are_sequential(num_data, data)) {
        mp_raise_RuntimeError(translate("Pins must be sequential GPIO pins"));
    }

    self->sdio = (pico_pio_sdio) {
        .clk = common_hal_mcu_pin_number(clock),
        .cmd = common_hal_mcu_pin_number(command),
        .dat0 = common_hal_mcu_pin_number(data[0]),
        .allow_four_data_pins = (num_data == 4),
    };

    int r = sd_init(&self->sdio);
    if (r == 0) {
        claim_pin(clock);
        claim_pin(command);
        for (size_t i = 0; i < num_data; i++) {
            claim_pin(data[i]);
        }
        claim_pin(command);
        return;
    }

    /* (convert negative errno value to back to positive for exception */
    mp_raise_OSError(-map_sdio_error(__func__, r));
}

uint32_t common_hal_sdioio_sdcard_get_count(sdioio_sdcard_obj_t *self) {
    return self->sdio.capacity;
}

uint32_t common_hal_sdioio_sdcard_get_frequency(sdioio_sdcard_obj_t *self) {
    return 0; // self->frequency;
}

uint8_t common_hal_sdioio_sdcard_get_width(sdioio_sdcard_obj_t *self) {
    return self->sdio.bus_width == bw_wide ? 4 : 0;
}

STATIC void check_for_deinit(sdioio_sdcard_obj_t *self) {
    if (common_hal_sdioio_sdcard_deinited(self)) {
        raise_deinited_error();
    }
}

STATIC void check_whole_block(mp_buffer_info_t *bufinfo) {
    if (bufinfo->len % SD_SECTOR_SIZE) {
        mp_raise_ValueError(translate("Buffer length must be a multiple of 512"));
    }
}

__attribute__((unused)) STATIC void debug_print_state(sdioio_sdcard_obj_t *self, const char *what, int r) {
    #if DEBUG_SDIO
    DEBUG_PRINT("%s: %d\n", what, r);
    #endif
}

static int sdcard_readblocks_sync(pico_pio_sdio *self, uint32_t *data, uint32_t sector_num, uint sector_count) {
    int rc = sd_readblocks_async(self, data, sector_num, sector_count);
    if (rc != 0) {
        return rc;
    }
    while (!sd_scatter_read_complete(self, &rc)) {
        RUN_BACKGROUND_TASKS;
        if (mp_hal_is_interrupted()) {
            return SD_ERR_STUCK;
        }
    }
    return rc;
}
static int sdcard_writeblocks_sync(pico_pio_sdio *self, const uint32_t *data, uint32_t sector_num, uint sector_count) {
    int rc = sd_writeblocks_async(self, data, sector_num, sector_count);
    if (rc != 0) {
        return rc;
    }
    while (!sd_write_complete(self, &rc)) {
        RUN_BACKGROUND_TASKS;
        if (mp_hal_is_interrupted()) {
            return SD_ERR_STUCK;
        }
    }
    return rc;
}

int common_hal_sdioio_sdcard_writeblocks(sdioio_sdcard_obj_t *self, uint32_t start_block, mp_buffer_info_t *bufinfo) {
    check_for_deinit(self);
    check_whole_block(bufinfo);
    int r = sdcard_writeblocks_sync(&self->sdio, bufinfo->buf, start_block, bufinfo->len / SD_SECTOR_SIZE);
    return map_sdio_error(__func__, r);
}

int common_hal_sdioio_sdcard_readblocks(sdioio_sdcard_obj_t *self, uint32_t start_block, mp_buffer_info_t *bufinfo) {
    check_for_deinit(self);
    check_whole_block(bufinfo);
    int r = sdcard_readblocks_sync(&self->sdio, bufinfo->buf, start_block, bufinfo->len / SD_SECTOR_SIZE);
    return map_sdio_error(__func__, r);
}

bool common_hal_sdioio_sdcard_configure(sdioio_sdcard_obj_t *self, uint32_t frequency, uint8_t bits) {
    check_for_deinit(self);
    return true;
}

bool common_hal_sdioio_sdcard_deinited(sdioio_sdcard_obj_t *self) {
    return !self->sdio.sd_pio;
}

void common_hal_sdioio_sdcard_deinit(sdioio_sdcard_obj_t *self) {
    if (self->sdio.sd_pio) {
        reset_pin_number(self->sdio.clk);
        reset_pin_number(self->sdio.cmd);
        reset_pin_number(self->sdio.dat0);
        if (self->sdio.allow_four_data_pins) {
            reset_pin_number(self->sdio.dat0 + 1);
            reset_pin_number(self->sdio.dat0 + 2);
            reset_pin_number(self->sdio.dat0 + 3);
        }
        sd_deinit(&self->sdio);
    }
}

void common_hal_sdioio_sdcard_never_reset(sdioio_sdcard_obj_t *self) {
}
