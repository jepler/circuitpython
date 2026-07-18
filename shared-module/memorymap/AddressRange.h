// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 microDev
// SPDX-FileCopyrightText: Copyright (c) 2023 Bob Abeles
//
// SPDX-License-Identifier: MIT

#pragma once

#include "py/obj.h"
#include <stdint.h>

typedef struct {
    uint8_t *start_address;
    uint32_t length : 30;
    uint32_t readonly : 1;
    uint32_t aligned32 : 1;
} memorymap_range_t;

typedef struct {
    mp_obj_base_t base;
    memorymap_range_t range;
} memorymap_addressrange_obj_t;
