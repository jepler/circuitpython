/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Scott Shawcroft
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

#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_BUSIO_SDIO_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_BUSIO_SDIO_H

#include "py/obj.h"

#include "common-hal/microcontroller/Pin.h"
#include "common-hal/_sdcardio/SDIOSDCard.h"

// Type object used in Python. Should be shared between ports.
extern const mp_obj_type_t sdcardio_sdiosdcard_type;

// Construct an underlying SDIO object.
extern void common_hal_sdcardio_sdio_construct(sdcardio_sdiosdcard_obj_t *self,
    const mcu_pin_obj_t * clock, const mcu_pin_obj_t * command,
    uint8_t num_data, mcu_pin_obj_t ** data, uint32_t frequency);

extern void common_hal_sdcardio_sdio_deinit(sdcardio_sdiosdcard_obj_t *self);
extern bool common_hal_sdcardio_sdio_deinited(sdcardio_sdiosdcard_obj_t *self);

extern bool common_hal_sdcardio_sdio_configure(sdcardio_sdiosdcard_obj_t *self, uint32_t baudrate, uint8_t width);

extern void common_hal_sdcardio_sdio_unlock(sdcardio_sdiosdcard_obj_t *self);

// Return actual SDIO bus frequency.
uint32_t common_hal_sdcardio_sdio_get_frequency(sdcardio_sdiosdcard_obj_t* self);

// Return SDIO bus width.
uint8_t common_hal_sdcardio_sdio_get_width(sdcardio_sdiosdcard_obj_t* self);

// Return number of device blocks
uint32_t common_hal_sdcardio_sdio_get_count(sdcardio_sdiosdcard_obj_t* self);

// Read or write blocks
int common_hal_sdcardio_sdio_readblocks(sdcardio_sdiosdcard_obj_t* self, uint32_t start_block, mp_buffer_info_t *bufinfo);
int common_hal_sdcardio_sdio_writeblocks(sdcardio_sdiosdcard_obj_t* self, uint32_t start_block, mp_buffer_info_t *bufinfo);

// This is used by the supervisor to claim SDIO devices indefinitely.
extern void common_hal_sdcardio_sdio_never_reset(sdcardio_sdiosdcard_obj_t *self);

#endif // MICROPY_INCLUDED_SHARED_BINDINGS_BUSIO_SDIO_H
