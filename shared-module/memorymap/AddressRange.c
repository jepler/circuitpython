// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 microDev
// SPDX-FileCopyrightText: Copyright (c) 2023 Bob Abeles
//
// SPDX-License-Identifier: MIT

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "py/runtime.h"

#include "shared-bindings/memorymap/AddressRange.h"
#include "shared-module/memorymap/AddressRange.h"

#include "common-hal/memorymap/ranges.h"

static bool validate_range(const memorymap_range_t *range, size_t start_index, size_t length, uint8_t **start_address_out) {
    uint8_t *range_start_address = range->start_address;
    uint8_t *range_end_address = range_start_address + range->length - 1;
    uint8_t *start_address = range_start_address + start_index;
    uint8_t *end_address = start_address + length - 1;
    if (start_address > range_end_address || start_address < range_start_address
        || end_address > range_end_address || end_address < range_start_address) {
        return false;
    }
    if (start_address_out) {
        *start_address_out = start_address;
    }
    return true;
}

static uint8_t *validate_range_raise(const memorymap_range_t *range, size_t start_index, size_t length) {
    uint8_t *start_address;
    if (!validate_range(range, start_index, length, &start_address)) {
        mp_raise_IndexError_varg(MP_ERROR_TEXT("%q index out of range"), MP_QSTR_memorymap);
    }
    return start_address;
}

void common_hal_memorymap_addressrange_construct(memorymap_addressrange_obj_t *self,
    uint8_t *start_address, size_t length) {
    for (size_t i = 0; i < MP_ARRAY_SIZE(memorymap_ranges); i++) {
        const memorymap_range_t *range = &memorymap_ranges[i];
        size_t start_index = start_address - range->start_address;
        if (!validate_range(range, start_index, length, NULL)) {
            break;
        }
        self->range = *range;
        self->range.start_address = start_address;
        self->range.length = length;
        return;
    }

    mp_raise_ValueError(MP_ERROR_TEXT("Address range not allowed"));
}

size_t common_hal_memorymap_addressrange_get_length(const memorymap_addressrange_obj_t *self) {
    return self->range.length;
}

void common_hal_memorymap_addressrange_set_bytes(const memorymap_addressrange_obj_t *self,
    size_t start_index, uint8_t *values, size_t length) {
    uint8_t *dest_addr = validate_range_raise(&self->range, start_index, length);
    if (self->range.readonly) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Unable to write to read-only memory"));
    } else if (self->range.aligned32) {
        if ((size_t)dest_addr & 0x03 || length & 0x03) {
            // Unaligned access or unaligned length not supported by RP2 for IO registers
            //
            // Note that string0 memcpy() cannot be used here, as it works in 32
            // bit quantities only when the source and destination are *both*
            // 32-bit aligned.
            mp_raise_RuntimeError(MP_ERROR_TEXT("Unable to access unaligned IO register"));
        } else {
            // Aligned access and length, use 32-bit writes
            uint32_t *dest_addr32 = (uint32_t *)dest_addr;
            size_t access_count = length >> 2;
            for (size_t i = 0; i < access_count; i++) {
                *dest_addr32++ = ((uint32_t *)values)[i];
            }
        }
    } else {
        memcpy(dest_addr, values, length);
    }
}

void common_hal_memorymap_addressrange_get_bytes(const memorymap_addressrange_obj_t *self,
    size_t start_index, size_t length, uint8_t *values) {
    uint8_t *src_addr = validate_range_raise(&self->range, start_index, length);
    if (self->range.aligned32) {
        if ((size_t)src_addr & 0x03 || length & 0x03) {
            // Unaligned access or unaligned length not supported by RP2 for IO registers
            mp_raise_RuntimeError(MP_ERROR_TEXT("Unable to access unaligned IO register"));
        } else {
            // Aligned access and length, use 32-bit reads
            //
            // Note that string0 memcpy() cannot be used here, as it works in 32
            // bit quantities only when the source and destination are *both*
            // 32-bit aligned.
            uint32_t *src_addr32 = (uint32_t *)src_addr;
            size_t access_count = length >> 2;
            for (size_t i = 0; i < access_count; i++) {
                ((uint32_t *)values)[i] = *src_addr32++;
            }
        }
    } else {
        memcpy(values, src_addr, length);
    }
}

mp_int_t shared_bindings_memorymap_get_buffer(const memorymap_addressrange_obj_t *self, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    if (flags & MP_BUFFER_WRITE && self->range.readonly) {
        return 1;
    }
    if (self->range.aligned32) {
        bufinfo->typecode = 'L';
    } else {
        bufinfo->typecode = 'B';
    }
    bufinfo->buf = self->range.start_address;
    bufinfo->len = self->range.length;
    return 0;
}
