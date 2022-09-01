/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
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

#include "common-hal/wifi/__init__.h"
#include "shared-bindings/wifi/__init__.h"

#include "shared-bindings/ipaddress/IPv4Address.h"
#include "shared-bindings/wifi/Monitor.h"
#include "shared-bindings/wifi/Radio.h"

#include "py/mpstate.h"
#include "py/runtime.h"

wifi_radio_obj_t common_hal_wifi_radio_obj;

#include "supervisor/port.h"
#include "supervisor/shared/title_bar.h"
#include "supervisor/workflow.h"

static bool wifi_inited;
static bool wifi_ever_inited;
static bool wifi_user_initiated;

void common_hal_wifi_init(bool user_initiated) {
    wifi_radio_obj_t *self = &common_hal_wifi_radio_obj;

    if (wifi_inited) {
        if (user_initiated && !wifi_user_initiated) {
            common_hal_wifi_radio_set_enabled(self, true);
        }
        return;
    }
    wifi_inited = true;
    wifi_user_initiated = user_initiated;

    if (!wifi_ever_inited) {
    }
    wifi_ever_inited = true;

    // set station mode to avoid the default SoftAP
    common_hal_wifi_radio_start_station(self);
    // start wifi
    common_hal_wifi_radio_set_enabled(self, true);
}

void wifi_user_reset(void) {
    if (wifi_user_initiated) {
        // wifi_reset();
        wifi_user_initiated = false;
    }
}

void wifi_reset(void) {
    if (!wifi_inited) {
        return;
    }
    common_hal_wifi_monitor_deinit(MP_STATE_VM(wifi_monitor_singleton));
    wifi_radio_obj_t *radio = &common_hal_wifi_radio_obj;
    common_hal_wifi_radio_set_enabled(radio, false);
    supervisor_workflow_request_background();
}

void common_hal_wifi_gc_collect(void) {
    common_hal_wifi_radio_gc_collect(&common_hal_wifi_radio_obj);
}
