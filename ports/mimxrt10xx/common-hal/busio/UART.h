/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Scott Shawcroft
 * Copyright (c) 2019 Artur Pacholec
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

#ifndef MICROPY_INCLUDED_MIMXRT10XX_COMMON_HAL_BUSIO_UART_H
#define MICROPY_INCLUDED_MIMXRT10XX_COMMON_HAL_BUSIO_UART_H

#include "common-hal/microcontroller/Pin.h"

#include "py/ringbuf.h"
#include "py/obj.h"
#include "periph.h"

#include "fsl_lpuart.h"

typedef struct {
    mp_obj_base_t base;
    LPUART_Type *uart;
    lpuart_handle_t handle;
    uint8_t *ringbuf;
    bool rx_ongoing;
    uint32_t baudrate;
    uint8_t character_bits;
    uint32_t timeout_ms;
    const mcu_periph_obj_t *rx;
    const mcu_periph_obj_t *tx;
    const mcu_periph_obj_t *cts;
    const mcu_periph_obj_t *rts;
} busio_uart_obj_t;

void uart_reset(void);

#endif // MICROPY_INCLUDED_MIMXRT10XX_COMMON_HAL_BUSIO_UART_H
