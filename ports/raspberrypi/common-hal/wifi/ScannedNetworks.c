/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Dan Halbert for Adafruit Industries
 * Copyright (c) 2018 Artur Pacholec
 * Copyright (c) 2017 Glenn Ruben Bakke
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

#include <string.h>

#include "shared/runtime/interrupt_char.h"
#include "py/gc.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "shared-bindings/wifi/__init__.h"
#include "shared-bindings/wifi/Network.h"
#include "shared-bindings/wifi/Radio.h"
#include "shared-bindings/wifi/ScannedNetworks.h"

static void wifi_scannednetworks_done(wifi_scannednetworks_obj_t *self) {
    self->done = true;
    if (self->results != NULL) {
        // Check to see if the heap is still active. If not, it'll be freed automatically.
        if (gc_alloc_possible()) {
            m_free(self->results);
        }
        self->results = NULL;
    }
}

static bool wifi_scannednetworks_wait_for_scan(wifi_scannednetworks_obj_t *self) {
    return false;
}

mp_obj_t common_hal_wifi_scannednetworks_next(wifi_scannednetworks_obj_t *self) {
    return mp_const_none;
}

// We don't do a linear scan so that we look at a variety of spectrum up front.
static uint8_t scan_pattern[] = {6, 1, 11, 3, 9, 13, 2, 4, 8, 12, 5, 7, 10, 14};

void wifi_scannednetworks_scan_next_channel(wifi_scannednetworks_obj_t *self) {
}

void wifi_scannednetworks_deinit(wifi_scannednetworks_obj_t *self) {
    // if a scan is active, make sure and clean up the idf's buffer of results.
    if (self->scanning) {
        // esp_wifi_scan_stop();
        if (wifi_scannednetworks_wait_for_scan(self)) {
        }
    }
    wifi_scannednetworks_done(self);
}
