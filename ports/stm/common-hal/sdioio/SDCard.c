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

#include "shared-bindings/sdioio/SDCard.h"
#include "py/mperrno.h"
#include "py/runtime.h"

#include "shared-bindings/microcontroller/__init__.h"
#include "boards/board.h"
#include "supervisor/shared/translate.h"
#include "common-hal/microcontroller/Pin.h"

#ifndef DEBUG_SDIO
#define DEBUG_SDIO (0)
#endif

#if DEBUG_SDIO
#define DEBUG_PRINT(...) ((void)mp_printf(&mp_plat_print, __VA_ARGS__))
#else
#define DEBUG_PRINT(...) ((void)0)
#endif

void common_hal_sdioio_sdcard_construct(sdioio_sdcard_obj_t *self,
        const mcu_pin_obj_t * clock, const mcu_pin_obj_t * command,
        uint8_t num_data, mcu_pin_obj_t ** data, uint32_t frequency) {
    // constructor arguments are ignored, and the hard-wired SD slot on the board is used.

    GPIO_InitTypeDef  GPIO_InitStruct;

    // /* GPIOC and GPIOD Periph clock enable */
    // RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | SD_DETECT_GPIO_CLK, ENABLE);

    /* Configure PC.08, PC.09, PC.10, PC.11 pins: D0, D1, D2, D3 pins */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = 12; // is there an identifier for this?
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure PD.02 CMD line */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Configure PC.12 pin: CLK pin */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

#define SD_DETECT_PIN GPIO_PIN_12
#define SD_DETECT_GPIO_PORT GPIOB

    /*!< Configure SD_SPI_DETECT_PIN pin: SD Card detect pin */
    GPIO_InitStruct.Pin = SD_DETECT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SD_DETECT_GPIO_PORT, &GPIO_InitStruct);

    __HAL_RCC_SDIO_CLK_ENABLE();

    self->handle.Init.ClockDiv = SDIO_TRANSFER_CLK_DIV;
    self->handle.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    self->handle.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    self->handle.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    self->handle.Init.BusWide = SDIO_BUS_WIDE_1B;
    self->handle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    self->handle.Instance = SDIO;

    HAL_StatusTypeDef r = HAL_SD_Init(&self->handle);
    if (r != HAL_OK) {
        mp_raise_ValueError_varg(translate("SDIO Init Error %d"), (int)r);
    }

    HAL_SD_CardInfoTypeDef info;
    r = HAL_SD_GetCardInfo(&self->handle, &info);
    if (r != HAL_OK) {
        mp_raise_ValueError_varg(translate("SDIO GetCardInfo Error %d"), (int)r);
    }

    if ((r = HAL_SD_ConfigWideBusOperation(&self->handle, SDIO_BUS_WIDE_4B)) == HAL_SD_ERROR_NONE) {
        DEBUG_PRINT("Switched bus to 4B mode\n");
        self->handle.Init.BusWide = SDIO_BUS_WIDE_4B;
        self->num_data = 4;
    } else {
        DEBUG_PRINT("WideBus_Enable returned %r, leaving at 1B mode\n", (int)r);
        self->num_data = 1;
    }

    self->capacity = info.BlockNbr * (info.BlockSize / 512);
    self->frequency = 25000000;

    return;
}

uint32_t common_hal_sdioio_sdcard_get_count(sdioio_sdcard_obj_t *self) {
    return self->capacity;
}

uint32_t common_hal_sdioio_sdcard_get_frequency(sdioio_sdcard_obj_t *self) {
    return self->frequency; // self->frequency;
}

uint8_t common_hal_sdioio_sdcard_get_width(sdioio_sdcard_obj_t *self) {
    return self->num_data; // self->width;
}

STATIC void check_for_deinit(sdioio_sdcard_obj_t *self) {
}

STATIC void check_whole_block(mp_buffer_info_t *bufinfo) {
    if (bufinfo->len % 512) {
        mp_raise_ValueError(translate("Buffer must be a multiple of 512 bytes"));
    }
}

STATIC void wait_write_complete(sdioio_sdcard_obj_t *self) {
    if (self->state_programming) {
        HAL_SD_CardStateTypedef st = HAL_SD_CARD_PROGRAMMING;
        // This waits up to 60s for programming to complete.  This seems like
        // an extremely long time, but this is the timeout that micropython's
        // implementation uses
        for (int i=0; i < 60000 && st == HAL_SD_CARD_PROGRAMMING; i++) {
            st = HAL_SD_GetCardState(&self->handle);
            HAL_Delay(1);
        };
        self->state_programming = false;
    }
}

STATIC void debug_print_state(sdioio_sdcard_obj_t *self, const char *what) {
#if DEBUG_SDIO
    HAL_SD_CardStateTypedef st = HAL_SD_GetCardState(&self->handle);
    DEBUG_PRINT("%s, st=0x%x State=0x%x ErrorCode=0x%x\n", what, (int)st, self->handle.State, self->handle.ErrorCode);
#endif
}

int common_hal_sdioio_sdcard_writeblocks(sdioio_sdcard_obj_t *self, uint32_t start_block, mp_buffer_info_t *bufinfo) {
    check_for_deinit(self);
    check_whole_block(bufinfo);
    wait_write_complete(self);
    self->state_programming = true;
    common_hal_mcu_disable_interrupts();
    HAL_StatusTypeDef r = HAL_SD_WriteBlocks(&self->handle, bufinfo->buf, start_block, bufinfo->len / 512, 1000);
    common_hal_mcu_enable_interrupts();
    if (r != HAL_OK) {
        debug_print_state(self, "after writeblocks error");
        return -EIO;
    }
    // debug_print_state(self, "after writeblocks OK");
    // debug_print_state(self, "after writeblocks complete");
    return 0;
}

int common_hal_sdioio_sdcard_readblocks(sdioio_sdcard_obj_t *self, uint32_t start_block, mp_buffer_info_t *bufinfo) {
    check_for_deinit(self);
    check_whole_block(bufinfo);
    wait_write_complete(self);
    common_hal_mcu_disable_interrupts();
    HAL_StatusTypeDef r = HAL_SD_ReadBlocks(&self->handle, bufinfo->buf, start_block, bufinfo->len / 512, 1000);
    common_hal_mcu_enable_interrupts();
    if (r != HAL_OK) {
        debug_print_state(self, "after readblocks error");
        return -EIO;
    }
    return 0;
}

bool common_hal_sdioio_sdcard_configure(sdioio_sdcard_obj_t *self, uint32_t frequency, uint8_t bits) {
    check_for_deinit(self);
    return true;
}

bool common_hal_sdioio_sdcard_deinited(sdioio_sdcard_obj_t *self) {
    return false;
}

void common_hal_sdioio_sdcard_deinit(sdioio_sdcard_obj_t *self) {
}

void common_hal_sdioio_sdcard_never_reset(sdioio_sdcard_obj_t *self) {
}
