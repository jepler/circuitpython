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

#include "shared-bindings/wifi/Radio.h"
#include "shared-bindings/wifi/Network.h"

#include <string.h>

#include "common-hal/wifi/__init__.h"
#include "shared/runtime/interrupt_char.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "shared-bindings/ipaddress/IPv4Address.h"
#include "shared-bindings/wifi/ScannedNetworks.h"
#include "shared-bindings/wifi/AuthMode.h"
#include "shared-bindings/time/__init__.h"
#include "shared-module/ipaddress/__init__.h"

#if CIRCUITPY_MDNS
#include "components/mdns/include/mdns.h"
#endif

#define MAC_ADDRESS_LENGTH 6

NORETURN static void ro_attribute(int attr) {
    mp_raise_msg_varg(&mp_type_AttributeError, MP_ERROR_TEXT("type object '%q' has no attribute '%q'"), MP_QSTR_Radio, attr);
}

bool common_hal_wifi_radio_get_enabled(wifi_radio_obj_t *self) {
    return true;
}

void common_hal_wifi_radio_set_enabled(wifi_radio_obj_t *self, bool enabled) {
    if (!enabled) {
        ro_attribute(MP_QSTR_enabled);
    }
}

mp_obj_t common_hal_wifi_radio_get_hostname(wifi_radio_obj_t *self) {
    return mp_const_none;
}

void common_hal_wifi_radio_set_hostname(wifi_radio_obj_t *self, const char *hostname) {

}

mp_obj_t common_hal_wifi_radio_get_mac_address(wifi_radio_obj_t *self) {
    return mp_obj_new_bytes(cyw43_state.mac, MAC_ADDRESS_LENGTH);
}

void common_hal_wifi_radio_set_mac_address(wifi_radio_obj_t *self, const uint8_t *mac) {
}

mp_float_t common_hal_wifi_radio_get_tx_power(wifi_radio_obj_t *self) {
    return MICROPY_FLOAT_CONST(0.);
}

void common_hal_wifi_radio_set_tx_power(wifi_radio_obj_t *self, const mp_float_t tx_power) {
    ro_attribute(MP_QSTR_tx_power);

}

mp_obj_t common_hal_wifi_radio_get_mac_address_ap(wifi_radio_obj_t *self) {
    return common_hal_wifi_radio_get_mac_address(self);
}

void common_hal_wifi_radio_set_mac_address_ap(wifi_radio_obj_t *self, const uint8_t *mac) {
}

mp_obj_t common_hal_wifi_radio_start_scanning_networks(wifi_radio_obj_t *self) {
    if (self->current_scan) {
        mp_raise_RuntimeError(translate("Already scanning for wifi networks"));
    }
    if (!common_hal_wifi_radio_get_enabled(self)) {
        mp_raise_RuntimeError(translate("wifi is not enabled"));
    }
    wifi_scannednetworks_obj_t *scan = m_new_obj(wifi_scannednetworks_obj_t);
    scan->base.type = &wifi_scannednetworks_type;
    mp_obj_t args[] = { mp_const_empty_tuple, MP_OBJ_NEW_SMALL_INT(16) };
    scan->results = mp_type_deque.make_new(&mp_type_deque, 2, 0, args);
    self->current_scan = scan;
    wifi_scannednetworks_start_scan(scan);
    return scan;
}

void common_hal_wifi_radio_stop_scanning_networks(wifi_radio_obj_t *self) {
    self->current_scan = NULL;
}

void common_hal_wifi_radio_start_station(wifi_radio_obj_t *self) {
    cyw43_arch_enable_sta_mode();
}

void common_hal_wifi_radio_stop_station(wifi_radio_obj_t *self) {
}

void common_hal_wifi_radio_start_ap(wifi_radio_obj_t *self, uint8_t *ssid, size_t ssid_len, uint8_t *password, size_t password_len, uint8_t channel, uint8_t authmode, uint8_t max_connections) {
}

void common_hal_wifi_radio_stop_ap(wifi_radio_obj_t *self) {
}

wifi_radio_error_t common_hal_wifi_radio_connect(wifi_radio_obj_t *self, uint8_t *ssid, size_t ssid_len, uint8_t *password, size_t password_len, uint8_t channel, mp_float_t timeout, uint8_t *bssid, size_t bssid_len) {
    return 0;
}

mp_obj_t common_hal_wifi_radio_get_ap_info(wifi_radio_obj_t *self) {
    return mp_const_none;
}

mp_obj_t common_hal_wifi_radio_get_ipv4_gateway(wifi_radio_obj_t *self) {
    return mp_const_none;
}

mp_obj_t common_hal_wifi_radio_get_ipv4_gateway_ap(wifi_radio_obj_t *self) {
    return mp_const_none;
}

mp_obj_t common_hal_wifi_radio_get_ipv4_subnet(wifi_radio_obj_t *self) {
    return mp_const_none;
}

mp_obj_t common_hal_wifi_radio_get_ipv4_subnet_ap(wifi_radio_obj_t *self) {
    return mp_const_none;
}

uint32_t wifi_radio_get_ipv4_address(wifi_radio_obj_t *self) {
    return 0;
}

mp_obj_t common_hal_wifi_radio_get_ipv4_address(wifi_radio_obj_t *self) {
    return mp_const_none;
}

mp_obj_t common_hal_wifi_radio_get_ipv4_address_ap(wifi_radio_obj_t *self) {
    return mp_const_none;
}

mp_obj_t common_hal_wifi_radio_get_ipv4_dns(wifi_radio_obj_t *self) {
    return mp_const_none;
}

void common_hal_wifi_radio_set_ipv4_dns(wifi_radio_obj_t *self, mp_obj_t ipv4_dns_addr) {
}

void common_hal_wifi_radio_start_dhcp_client(wifi_radio_obj_t *self) {
}

void common_hal_wifi_radio_stop_dhcp_client(wifi_radio_obj_t *self) {
}

void common_hal_wifi_radio_set_ipv4_address(wifi_radio_obj_t *self, mp_obj_t ipv4, mp_obj_t netmask, mp_obj_t gateway, mp_obj_t ipv4_dns) {
}

mp_int_t common_hal_wifi_radio_ping(wifi_radio_obj_t *self, mp_obj_t ip_address, mp_float_t timeout) {
    return 0;
}

void common_hal_wifi_radio_gc_collect(wifi_radio_obj_t *self) {
    // Only bother to scan the actual object references.
    gc_collect_ptr(self->current_scan);
}
