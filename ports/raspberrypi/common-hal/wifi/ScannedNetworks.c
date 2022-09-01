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
#include "common-hal/wifi/__init__.h"
#include "shared-bindings/wifi/__init__.h"
#include "shared-bindings/wifi/Network.h"
#include "shared-bindings/wifi/Radio.h"
#include "shared-bindings/wifi/ScannedNetworks.h"

static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    wifi_scannednetworks_obj_t *self = common_hal_wifi_radio_obj.current_scan;
    // scan ended or something
    if (!self) {
        return 0;
    }

    wifi_network_obj_t *entry = m_new_obj(wifi_network_obj_t);
    entry->base.type = &wifi_network_type;
    entry->record = *result;

    mp_obj_t dest[3];
    mp_load_method(self->results, MP_QSTR_append, dest);
    dest[2] = entry;
    mp_call_method_n_kw(1, 0, dest);

    return 0;
}

mp_obj_t common_hal_wifi_scannednetworks_next(wifi_scannednetworks_obj_t *self) {
    // no results available, wait for some
    while (!mp_obj_is_true(self->results) && cyw43_wifi_scan_active(&cyw43_state)) {
        RUN_BACKGROUND_TASKS;
        if (mp_hal_is_interrupted()) {
            return mp_const_none;
        }
        cyw43_arch_poll();
    }

    if (!mp_obj_is_true(self->results)) {
        common_hal_wifi_radio_obj.current_scan = NULL;
        return mp_const_none;
    }

    // return an available result
    mp_obj_t dest[2];
    mp_load_method(self->results, MP_QSTR_popleft, dest);
    return mp_call_method_n_kw(0, 0, dest);
}

void wifi_scannednetworks_deinit(wifi_scannednetworks_obj_t *self) {
    // there's actually no way to stop an ongoing scan in cyw43!
    common_hal_wifi_radio_obj.current_scan = NULL;
}

void wifi_scannednetworks_start_scan(wifi_scannednetworks_obj_t *self) {
    cyw43_wifi_scan_options_t scan_options = {0};
    CHECK_CYW_RESULT(cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result));
}
