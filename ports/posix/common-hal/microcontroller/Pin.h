#pragma once

#include "py/obj.h"

typedef struct _mcu_pin_obj_t {
    mp_obj_base_t base;
    int number;
} mcu_pin_obj_t;
